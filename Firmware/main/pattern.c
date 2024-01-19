#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "app-config.h"
#include "pattern.h"
#include "hardware.h"
#include "hsv2rgb.h"
#include "utils.h"

static const char *TAG = "patterns";

typedef void (*pattern_tick_func)(led_strip_handle_t strip, const arm_config *config, uint32_t counter);

static void rainbow_tick(led_strip_handle_t strip, const arm_config *config, uint32_t counter) {
    for (int i = 0; i < config->num_leds; i++) {
        int r, g, b;
        float a = angle_normalize(config->angle/50 - counter - (i * 3));

        HSVtoRGB(a, 100, 50, &r, &g, &b);
        led_strip_set_pixel(strip, i, r, g, b);
    }
}

static void chasing_lights_1_tick(led_strip_handle_t strip, const arm_config *config, uint32_t counter) {
    float a = angle_normalize(config->angle + counter/3);
    int peak = (counter % (config->num_leds * 3)) - config->num_leds;

    for (int i = 0; i < config->num_leds; i++) {
        int r, g, b;
        int d = abs(i - peak);
        float v = 0;

        if (d < 5)
            v = 50 / (d+1);

        HSVtoRGB(a, 100, v, &r, &g, &b);
        led_strip_set_pixel(strip, i, r, g, b);
    }
}

static void chasing_lights_2_tick(led_strip_handle_t strip, const arm_config *config, uint32_t counter) {
    float a = angle_normalize(config->angle + (counter/3));
    int n = (counter/5) % (config->num_leds * 2);

    if (n > config->num_leds)
        n -= 2 * (n - config->num_leds);

    for (int i = 0; i < config->num_leds; i++) {
        int r, g, b;
        float v = 0;

        if (i <= n)
            v = 50;

        HSVtoRGB(a, 100, v, &r, &g, &b);
        led_strip_set_pixel(strip, i, r, g, b);
    }
}

static void chasing_lights_3_tick(led_strip_handle_t strip, const arm_config *config, uint32_t counter) {
    float a = angle_normalize(config->angle + counter/3);
    int peak = counter % (4 * (config->num_leds + 50));
    bool outwards = true;

    for (int i = 0; i < config->num_leds; i++) {
        int r, g, b;
        int d = abs(i - peak);
        float v = 0;

        if (d < config->num_leds)
            v = (d / 10) * 10;

        HSVtoRGB(a, 100, v, &r, &g, &b);

        if (outwards)
            led_strip_set_pixel(strip, i, r, g, b);
        else
            led_strip_set_pixel(strip, config->num_leds - i - 1, r, g, b);
    }

    outwards = !outwards;
}

static void chasing_lights_4_tick(led_strip_handle_t strip, const arm_config *config, uint32_t counter) {
    float a = angle_normalize(counter/3);

    for (int i = 0; i < config->num_leds; i++) {
        float v = 0;
        int r, g, b;

        if ((i + config->num_leds - counter/40 - (config->angle / 90)) % 8 == 0)
            v = 100;

        HSVtoRGB(a, 100, v, &r, &g, &b);
        led_strip_set_pixel(strip, i, r, g, b);
    }
}

static void pulse_1_tick(led_strip_handle_t strip, const arm_config *config, uint32_t counter) {
    float a = angle_normalize(config->angle + counter/3);
    int v = counter % 400;

    if (v > 200)
        v = 0;
    else if (v > 100)
        v -= 2 * (v - 100);

    for (int i = 0; i < config->num_leds; i++) {
        int r, g, b;

        HSVtoRGB(a, 100, v, &r, &g, &b);
        led_strip_set_pixel(strip, i, r, g, b);
    }
}

static void sensor_test_tick(led_strip_handle_t strip, const arm_config *config, uint32_t counter) {
    bool on = gpio_get_level(config->hall_sensor_pin) == 0;
    
    for (int i = 0; i < config->num_leds; i++)
        led_strip_set_pixel(strip, i, 0, 0, 0);

    if (on)
        led_strip_set_pixel(strip, config->angle/90, 255, 0, 0);
}

static pattern_tick_func pattern_tick_funcs[] = {
    rainbow_tick,
    chasing_lights_1_tick,
    chasing_lights_2_tick,
    chasing_lights_3_tick,
    chasing_lights_4_tick,
    pulse_1_tick,
    // sensor_test_tick,
};

#define PATTERN_COUNT (sizeof(pattern_tick_funcs) / sizeof(pattern_tick_funcs[0]))

static uint32_t counter = 0;
static pattern_tick_func current_tick_func = NULL;

// Advance the pattern state machine and update the LED strips
void pattern_tick() {
    if (current_tick_func == NULL)
        pattern_next();

    for (int i = 0; i < MAX_ARMS; i++) {
        led_strip_handle_t strip = led_strip[i];
        arm_config *arm_config = &global_app_config->arm[i];

        if (strip != NULL) {
            current_tick_func(strip, arm_config, counter);
            led_strip_refresh(strip);
        }
    }

    counter++;
}

void pattern_next() {
    current_tick_func = pattern_tick_funcs[rand() % PATTERN_COUNT];
    counter = 0;
}
