idf_component_register(SRCS "canvas.c" "pattern.c" "app-config.c" "hall-tracker.c" "main.c" "gif.c" "playlist.c" "wifi.c"
                       INCLUDE_DIRS ".")
set(CMAKE_CXX_STANDARD 17)
spiffs_create_partition_image(storage ../spiffs_data FLASH_IN_PROJECT)