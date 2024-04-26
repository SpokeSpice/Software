#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_vfs.h"

#include <nsgif.h>

#include "canvas.h"
#include "gif.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static const char *TAG = "gif";

static nsgif_t *gif = NULL;
static uint8_t *gif_buffer = NULL;
static const nsgif_info_t *gif_info;
static uint32_t next_frame_time = 0;

static nsgif_bitmap_t *bitmap_create(int width, int height)
{
    return calloc(width * height, 4);
}

static void bitmap_destroy(nsgif_bitmap_t *bitmap)
{
    free(bitmap);
}

static uint8_t *bitmap_get_buffer(nsgif_bitmap_t *bitmap)
{
    return (uint8_t *)bitmap;
}

static void bitmap_set_opaque(void *bitmap, bool opaque) {}

static bool bitmap_test_opaque(void *bitmap)
{
    return false;
}

static void bitmap_mark_modified(void *bitmap) {}

static nsgif_bitmap_cb_vt bitmap_callbacks = {
    .create = bitmap_create,
    .destroy = bitmap_destroy,
    .get_buffer = bitmap_get_buffer,
    .set_opaque = bitmap_set_opaque,
    .test_opaque = bitmap_test_opaque,
    .modified = bitmap_mark_modified,
    .get_rowspan = NULL
};

static SemaphoreHandle_t mutex;

void gif_init()
{
    mutex = xSemaphoreCreateMutex();
}

static esp_err_t gif_load(const void *buf, size_t size)
{
    if (gif)
        nsgif_destroy(gif);

    gif = NULL;

    // ESP_LOG_BUFFER_HEX_LEVEL(TAG, buf, size, ESP_LOG_INFO);

    nsgif_error res = nsgif_create(&bitmap_callbacks, NSGIF_BITMAP_FMT_R8G8B8A8, &gif);
    if (res != NSGIF_OK) {
        ESP_LOGE(TAG, "Error creating GIF handler: %d (%s)", res, nsgif_strerror(res));
        return ESP_FAIL;
    }

    res = nsgif_data_scan(gif, size, buf);
    nsgif_data_complete(gif);

    if (res != NSGIF_OK) {
        nsgif_destroy(gif);
        gif = NULL;
        ESP_LOGE(TAG, "Error loading GIF: %d (%s)", res, nsgif_strerror(res));
        return ESP_FAIL;
    }

    gif_info = nsgif_get_info(gif);
    ESP_LOGI(TAG, "GIF frame_count=%lu, width=%lu, height=%lu, source size %d",
        gif_info->frame_count, gif_info->width, gif_info->height, size);

    next_frame_time = 0;

    return ESP_OK;
}

esp_err_t gif_load_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    struct stat st;
    esp_err_t err = fstat(fileno(f), &st);
    if (err != 0) {
        ESP_LOGE(TAG, "Failed to stat file");
        fclose(f);
        return ESP_FAIL;
    }

    size_t size = st.st_size;
    if (size == 0) {
        ESP_LOGE(TAG, "File is empty");
        fclose(f);
        return ESP_FAIL;
    }

    uint8_t *new_gif_buffer = malloc(size);
    if (new_gif_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        fclose(f);
        return ESP_FAIL;
    }

    size_t r = fread(new_gif_buffer, 1, size, f);
    if (size != r) {
        ESP_LOGE(TAG, "Failed to read file");
        fclose(f);
        return ESP_FAIL;
    }

    fclose(f);

    xSemaphoreTake(mutex, portMAX_DELAY);

    esp_err_t ret = gif_load(new_gif_buffer, size);
    free(gif_buffer);
    gif_buffer = new_gif_buffer;

    xSemaphoreGive(mutex);

    return ret;
}

esp_err_t gif_tick()
{
    xSemaphoreTake(mutex, portMAX_DELAY);

    if (!gif) {
        xSemaphoreGive(mutex);
        return ESP_OK;
    }

    uint32_t time_cs = esp_timer_get_time() / 10000;
    if (time_cs < next_frame_time) {
        xSemaphoreGive(mutex);
        return ESP_OK;
    }
    
    nsgif_rect_t frame_rect;
    uint32_t delay_cs;
    uint32_t frame;

    nsgif_error res = nsgif_frame_prepare(gif, &frame_rect, &delay_cs, &frame);
    // ESP_LOGI(TAG, "frame %lu, delay %lu, rect %lu %lu %lu %lu",
    //     frame, delay_cs, frame_rect.x0, frame_rect.y0, frame_rect.x1, frame_rect.y1);

    if (res != NSGIF_OK) {
        ESP_LOGW(TAG, "Error preparing GIF frame: %d (%s)", res, nsgif_strerror(res));
        xSemaphoreGive(mutex);
        return ESP_FAIL;
    }

    if (delay_cs == NSGIF_INFINITE) {
        if (gif_info->frame_count > 1) {
            ESP_LOGD(TAG, "GIF animation end, looping");
            nsgif_reset(gif);

            next_frame_time = time_cs;

            xSemaphoreGive(mutex);
            return ESP_OK;
        } else {
            next_frame_time = UINT32_MAX;
        }
    } else {
        next_frame_time = time_cs + delay_cs - 1;
    }

    nsgif_bitmap_t *bitmap;
    res = nsgif_frame_decode(gif, frame, &bitmap);
    if (res != NSGIF_OK) {
        ESP_LOGW(TAG, "Error decoding GIF frame: %d (%s)", res, nsgif_strerror(res));
        xSemaphoreGive(mutex);
        return ESP_FAIL;
    }

    uint32_t *frame_image = (uint32_t *)bitmap;

    for (size_t x = frame_rect.x0; x < MIN(CANVAS_WIDTH, frame_rect.x1); x++)
        for (size_t y = frame_rect.y0; y < MIN(CANVAS_HEIGHT, frame_rect.y1); y++) {
            size_t offs = y * gif_info->width + x;

            Pixel p = {
                .r = (frame_image[offs] >> 0)  & 0xff,
                .g = (frame_image[offs] >> 8)  & 0xff,
                .b = (frame_image[offs] >> 16) & 0xff
            };

            canvas_set_pixel(x, y, p);
        }

    // ESP_LOGI(TAG, "Rendered frame %lu  x %08lx", frame, frame_image[0]);
    // canvas_dump();

    xSemaphoreGive(mutex);

    return ESP_OK;
}