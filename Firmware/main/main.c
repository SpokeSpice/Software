#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "nvs_flash.h"

#include "app-config.h"
#include "canvas.h"
#include "pattern.h"
#include "gif.h"
#include "playlist.h"
#include "hall-tracker.h"
#include "pins.h"
#include "utils.h"
#include "wifi.h"
#include "esp_spiffs.h"
#include "esp_timer.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

static const char *TAG = "SpokeSpice";

led_strip_handle_t led_strip[MAX_ARMS] = {};

static const char *sdcard_dir = "/sdcard";
static const char *spiffs_dir = "/spiffs";

enum {
    MODE_PATTERN,
    MODE_GIF,
};

static int mode = 0;

static void led(int i, bool on)
{
    gpio_set_level(i == 0 ? PIN_LED_1 : PIN_LED_2, on ? 1 : 0);
}

static void IRAM_ATTR hall_interrupt_handler(void *args)
{
    arm_config *arm = (arm_config *)args;

    hall_tracker_trigger(arm->angle);
}

static void update_strips_task_func(void *)
{
    uint32_t counter = 0;
    uint64_t last_change = 0;

    ESP_LOGI(TAG, "Starting stripe update task");

    // blink the LED
    led(1, (counter++ & 0x80) == 0); 

    while (true) {
        int angle = hall_tracker_current_angle();
        uint64_t now = esp_timer_get_time();

        if (angle < 0 && mode == MODE_GIF)
            mode = MODE_PATTERN;

        if (now - last_change > 1000000 * global_app_config->pattern_change_interval_seconds) {
            if (mode == MODE_PATTERN && angle> 0) {
                mode = MODE_GIF;
                playlist_next();
            } else {
                mode = MODE_PATTERN;
                pattern_next();
            }

            last_change = now;
        }

        switch (mode) {
        case MODE_PATTERN:
            pattern_tick();
            break;

        case MODE_GIF:
            gif_tick();

            angle = angle_normalize(angle + global_app_config->angle_offset);

            for (int i = 0; i < MAX_ARMS; i++) {
                led_strip_handle_t strip = led_strip[i];
                arm_config *config = &global_app_config->arm[i];

                if (strip != NULL)
                    canvas_render(strip, config, angle);
            }

            break;
        }
    }
}

esp_err_t spiffs_init()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = spiffs_dir,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS");
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS info");
        return ret;
    }

    ESP_LOGI(TAG, "SPIFFS: Total: %d, Used: %d", total, used);

    return ESP_OK;
}

esp_err_t sd_card_init()
{
    esp_err_t ret;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_SD_MOSI,
        .miso_io_num = PIN_SD_MISO,
        .sclk_io_num = PIN_SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ESP_ERROR_CHECK(gpio_set_pull_mode(PIN_SD_MOSI, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(PIN_SD_MISO, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(PIN_SD_CLK, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(PIN_SD_CS, GPIO_PULLUP_ONLY));

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_SD_CS;
    slot_config.gpio_cd = SDSPI_SLOT_NO_CD;
    slot_config.gpio_wp = SDSPI_SLOT_NO_WP;
    slot_config.gpio_int = SDSPI_SLOT_NO_INT;
    slot_config.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 3,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(sdcard_dir, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount filesystem.");
        return ret;
    }

    sdmmc_card_print_info(stdout, card);

    return ESP_OK;
}

void app_main(void)
{
    esp_err_t ret;

    //Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    load_app_config();
    hall_tracker_init();
    canvas_init();
    gif_init();

    ret = sd_card_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SD card initialized");
        playlist_init(sdcard_dir);
    }

    ret = spiffs_init();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS initialized");
        playlist_init(spiffs_dir);
    }

    gpio_install_isr_service(0);

    for (int i = 0; i < MAX_ARMS; i++) {
        arm_config *arm = &global_app_config->arm[i];

        if (arm->hall_sensor_pin >= 0) {
            ESP_ERROR_CHECK(gpio_set_direction(arm->hall_sensor_pin, GPIO_MODE_INPUT));
            ESP_ERROR_CHECK(gpio_set_pull_mode(arm->hall_sensor_pin, GPIO_PULLUP_ONLY));
            ESP_ERROR_CHECK(gpio_set_intr_type(arm->hall_sensor_pin, GPIO_INTR_NEGEDGE));
            ESP_ERROR_CHECK(gpio_isr_handler_add(arm->hall_sensor_pin, hall_interrupt_handler, arm));
        }

        if (arm->led_pin >= 0 && arm->num_leds > 0) {
            led_strip_config_t strip_config = {
                .strip_gpio_num = arm->led_pin,
                .max_leds = arm->num_leds,
            };
            led_strip_rmt_config_t rmt_config = {
                .resolution_hz = 20 * 1000 * 1000, // 10MHz
            };

            ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip[i]));
            ESP_ERROR_CHECK(led_strip_clear(led_strip[i]));
        }

        ESP_LOGI(TAG, "Arm %d: Hall sensor pin %d, LED pin %d, angle %d",
                 i, arm->hall_sensor_pin, arm->led_pin, arm->angle);
    }

    ESP_ERROR_CHECK(gpio_set_direction(PIN_LED_1, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(PIN_LED_2, GPIO_MODE_OUTPUT));

    ESP_ERROR_CHECK(gpio_set_direction(48, GPIO_MODE_OUTPUT));
    gpio_set_level(48, 1);

    // esp_intr_dump(stdout);

    xTaskCreate(update_strips_task_func, "Stripes", 4096, NULL, 1, NULL);
}
