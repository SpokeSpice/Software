#pragma once

#include "freertos/FreeRTOS.h"

#define MAX_ARMS 4

typedef struct {
    int hall_sensor_pin;
    int led_pin;
    int num_leds;
    int angle;
} arm_config;

typedef struct {
    arm_config arm[MAX_ARMS];
    int pattern_change_interval_seconds;
    int angle_offset;
} app_config;

extern app_config *global_app_config;

esp_err_t load_app_config();