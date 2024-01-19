# SpokeSpice firmware

This is the firmware for the SpokeSpice project. It is written in C and is designed to run on an ESP32-S3.

It has the following features:

* Initialize the connected arms
* Install GPIO IRQ handlers for the magnetic field sensors on the arms
* Estimate the current frequency of the wheel
* Provide the current estimated wheel position at any time
* Initialize the SPIFFS file system
* When the wheel is spinning at a minimum speed, it will start to display rgif images and patterns on the arms
* When the wheel is spinning below the minimum speed, patterns will be displayed on the arms

## Compiling

The firmware can be compiled using the ESP-IDF framework. The ESP-IDF framework needs to be installed on your system. Please refer to the [ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) for more information.

The firmware can be compiled using the following commands:

```bash
idf.py build
```

## Images

Images are read from the SPIFFS filesystem. The playlist will iterate over all files in the filesystem and display them in order.

The images need to be preprocessed for the display so that the resulting format is a gif file with 32 rows (for the 32 LEDs) and
360 columns (for the 360 degrees of the wheel). Refer to the [convert](../convert/) directory for more information.

The firmware will then get the current wheel position and display the corresponding column of the image on the arms as fast as possible.

Images need to be placed in the `spiffs_data` directory and will be flashed to the SPIFFS filesystem through the `idf.py flash` command.

## Patterns

Patterns are code snippets that will be executed for each arm. The patterns are defined in `main/patterns.c` and can easily be extended.

## Configuration

The configuration is currently hard-coded in `main/app-config.c`.

## Contributing

Please file issues and pull requests on this GitHub repository.
Please also have a look at the [GitHub project](https://github.com/orgs/SpokeSpice/projects/1) for future ideas.

## License

This project is licensed under the GPLv2 License - see the [LICENSE](LICENSE) file for details.
