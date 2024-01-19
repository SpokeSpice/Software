#pragma once

#include "led_strip.h"
#include "app-config.h"

#define CANVAS_WIDTH 360
#define CANVAS_HEIGHT 32

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

extern Pixel *canvas;

void canvas_init();

void canvas_clear();
void canvas_set_pixel(int x, int y, Pixel p);
void canvas_dump();

void canvas_render(led_strip_handle_t led_strip, const arm_config *config, int angle);
