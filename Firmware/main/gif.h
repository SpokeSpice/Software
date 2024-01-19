#pragma once

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "canvas.h"

void gif_init();
esp_err_t gif_load_file(const char *path);
esp_err_t gif_tick();
