#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <dirent.h>
#include <string.h>

#include "gif.h"
#include "pins.h"
#include "playlist.h"
#include "hall-tracker.h"

static const char *TAG = "Playlist";

static DIR *dir;
static int count = 0;
static const char *base_dir;

static int ls_dir()
{
    struct dirent *entry;
    int count = 0;

    ESP_LOGI(TAG, "Playlist contents:");

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".rgif") == NULL)
            continue;

        ESP_LOGI(TAG, "%s", entry->d_name);
        count++;
    }

    ESP_LOGI(TAG, "----------------------------");
    rewinddir(dir);

    return count;
}

void playlist_next()
{
    struct dirent *entry = readdir(dir);
    char path[PATH_MAX];

    while (true) {
        if (entry == NULL) {
            rewinddir(dir);
            entry = readdir(dir);
        }

        if (strstr(entry->d_name, ".rgif") == NULL)
            continue;

        snprintf(path, PATH_MAX, "%s/%s", base_dir, entry->d_name);
        ESP_LOGI(TAG, "Loading %s", path);

        esp_err_t ret = gif_load_file(path);
        if (ret == ESP_OK)
            return;

        ESP_LOGE(TAG, "Failed to load %s", path);
    }
}

int playlist_count()
{
    return count;
}

void playlist_init(const char *path)
{
    dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory %s", path);
        return;
    }

    count = ls_dir();
    base_dir = path;
}
