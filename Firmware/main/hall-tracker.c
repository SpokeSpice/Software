#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_timer.h"
#include "esp_log.h"

#include "app-config.h"
#include "hall-tracker.h"
#include "utils.h"

static const char *TAG = "hall";

static int64_t last_timestamp; // in microseconds
static int last_angle;         // in degrees
static int last_angle_delta;   // in degrees
static int frequency;          // in millihertz

const int min_frequency = 1000;  // in millihertz
const int max_time_delta = 500; // in milliseconds

#define FREQ_MOVING_AVG_SIZE 10
static int freq_moving_avg[FREQ_MOVING_AVG_SIZE];
static int freq_moving_avg_index = 0;

static QueueHandle_t queue;
static SemaphoreHandle_t mutex;

struct hall_tracker_trigger_event {
    uint64_t timestamp;
    int angle;
};

void IRAM_ATTR hall_tracker_trigger(int angle) {
    struct hall_tracker_trigger_event event = {
        .timestamp = esp_timer_get_time(),
        .angle = angle
    };

    xQueueSendFromISR(queue, &event, NULL);
}

void hall_tracker_task_func(void *pvParameter) {
    struct hall_tracker_trigger_event event;

    while (true) {
        if (!xQueueReceive(queue, &event, portMAX_DELAY))
            continue;

        int f;

        xSemaphoreTake(mutex, portMAX_DELAY);

        if (event.angle == last_angle) {
            xSemaphoreGive(mutex);
            continue;
        }

        int angle_delta = (event.angle - last_angle + 360) % 360;

        if (angle_delta > 180)
            angle_delta -= 180;

        int64_t time_delta_ms = (event.timestamp - last_timestamp) / 1000;

        if (last_timestamp > 0 && time_delta_ms > 0) {
            f = (angle_delta * 1000000) / (360 * time_delta_ms);

            freq_moving_avg[freq_moving_avg_index++] = f;
            freq_moving_avg_index %= FREQ_MOVING_AVG_SIZE;

            int sum = 0;

            for (int i = 0; i < FREQ_MOVING_AVG_SIZE; i++)
                sum += freq_moving_avg[i];

            frequency = sum / FREQ_MOVING_AVG_SIZE;
        }

        last_angle = event.angle;
        last_timestamp = event.timestamp;
        last_angle_delta = angle_delta;

        xSemaphoreGive(mutex);
    }
}

// This function calls into the hall tracker to simulate a rotation
// of the arms. This is useful for testing the LED strips without
// having to rotate the wheel the arms are mounted on.
static void IRAM_ATTR simulate_rotation_task_func(void *)
{
    while (true) {
        for (int i = 0; i < MAX_ARMS; i++) {
            arm_config *config = &global_app_config->arm[i];

            portDISABLE_INTERRUPTS();
            hall_tracker_trigger(config->angle);
            portENABLE_INTERRUPTS();

            vTaskDelay(75 / portTICK_PERIOD_MS);
        }
    }
}

// Returns the estimated current angle in degrees, or -1 for unknown.
int hall_tracker_current_angle() {
    int angle;

    xSemaphoreTake(mutex, portMAX_DELAY);

    int64_t now = esp_timer_get_time();
    int64_t time_delta_ms = (now - last_timestamp) / 1000;

    if (time_delta_ms < max_time_delta && frequency > min_frequency) {
        int angle_delta = (360 * frequency * time_delta_ms) / 1000000;
        angle = (last_angle - angle_delta + 360) % 360;
    } else
        angle = -1;

    // ESP_LOGI(TAG, "angle: %d, time_delta_ms: %lld, frequency: %d", angle, time_delta_ms, frequency);

    xSemaphoreGive(mutex);

    return angle;
}

int hall_tracker_last_angle_delta() {
    int angle_delta;

    xSemaphoreTake(mutex, portMAX_DELAY);
    angle_delta = last_angle_delta;
    xSemaphoreGive(mutex);

    return angle_delta;
}

int hall_tracker_current_frequency()
{
    int f;

    xSemaphoreTake(mutex, portMAX_DELAY);
    f = frequency;
    xSemaphoreGive(mutex);

    return f;
}

void hall_tracker_init() {
    last_angle = 0.;
    last_timestamp = 0;

    mutex = xSemaphoreCreateMutex();
    queue = xQueueCreate(10, sizeof(struct hall_tracker_trigger_event));

    xTaskCreate(hall_tracker_task_func, "HAL tracker task", 2048, NULL, 10, NULL);
    // xTaskCreate(simulate_rotation_task_func, "Rotation simulator", 2048, NULL, 1, NULL);
}
