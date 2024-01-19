#include "freertos/FreeRTOS.h"
#include <math.h>
#include <string.h>

#include "app-config.h"
#include "esp_log.h"
#include "led_strip.h"
#include "canvas.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

static const char *TAG = "canvas";

Pixel *canvas;

void canvas_init()
{
    canvas = malloc(sizeof(Pixel) * CANVAS_WIDTH * CANVAS_HEIGHT);
    canvas_clear();
}

static inline int canvas_index(int x, int y)
{
    return y * CANVAS_WIDTH + x;
}

void canvas_clear()
{
    ESP_LOGI(TAG, "Clearing canvas");
    memset(canvas, 0, sizeof(Pixel) * CANVAS_WIDTH * CANVAS_HEIGHT);
}

void canvas_dump()
{
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, canvas, 1024, ESP_LOG_INFO);
}

void canvas_set_pixel(int x, int y, Pixel p)
{
    if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT)
        return;

    int pixelIndex = canvas_index(x, y);

    canvas[pixelIndex].r = p.r;
    canvas[pixelIndex].g = p.g;
    canvas[pixelIndex].b = p.b;
}

void canvas_render(led_strip_handle_t led_strip, const arm_config *config, int angle)
{
    int x = (config->angle - angle + CANVAS_WIDTH) % CANVAS_WIDTH;

    for (int y = 0; y < config->num_leds; y++) {
        int pixel_index = canvas_index(x, y);
        led_strip_set_pixel(led_strip, y, canvas[pixel_index].r, canvas[pixel_index].g, canvas[pixel_index].b);
    }

    led_strip_refresh(led_strip);
}