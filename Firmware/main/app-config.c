#include <stdio.h>
#include <esp_types.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "pins.h"

#include "app-config.h"

app_config *global_app_config;

esp_err_t load_app_config() {
    global_app_config = malloc(sizeof(app_config));

    global_app_config->arm[0].hall_sensor_pin = PIN_HALL_SENSOR_0;
    global_app_config->arm[0].led_pin = PIN_LED_STRIP_0;
    global_app_config->arm[0].angle = 0;
    global_app_config->arm[0].num_leds = 32;

    global_app_config->arm[1].hall_sensor_pin = PIN_HALL_SENSOR_1;
    global_app_config->arm[1].led_pin = PIN_LED_STRIP_1;
    global_app_config->arm[1].angle = 90;
    global_app_config->arm[1].num_leds = 32;

    global_app_config->arm[2].hall_sensor_pin = PIN_HALL_SENSOR_2;
    global_app_config->arm[2].led_pin = PIN_LED_STRIP_2;
    global_app_config->arm[2].angle = 180;
    global_app_config->arm[2].num_leds = 32;

    global_app_config->arm[3].hall_sensor_pin = PIN_HALL_SENSOR_3;
    global_app_config->arm[3].led_pin = PIN_LED_STRIP_3;
    global_app_config->arm[3].angle = 270;
    global_app_config->arm[3].num_leds = 32;

    global_app_config->pattern_change_interval_seconds = 10;
    global_app_config->angle_offset = 120;

    ESP_LOGI("app-app_config", "Loaded app_config");

    return ESP_OK;
}
