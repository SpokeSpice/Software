# Radial GIF Creator

This tool processes an input GIF, transforming it into a radial version where each frame is represented as 32 rows (for 32 LEDs) and
360 columns (for 360 degrees). The tool creates a radial representation of the input GIF so that the firmware can display it on the LED strip.

The GIFs can be animated. The tool will process each frame individually will preserve the frame durations of the input GIF in the output.

## Features

- **Configurable Output Resolution**: Customize the number of segments (rows) in the output image.
- **Vector Steps Averaging**: Configure the number of steps for averaging, enhancing detail.
- **Frame Duration Preservation**: The output GIF retains the same frame durations as the input GIF.

## Requirements

- Rust

## Installation

Ensure you have Rust installed on your system. You can download Rust from [here](https://www.rust-lang.org/tools/install).

Ensure you have Python installed on your system. You can download Python from [here](https://www.python.org/downloads/).

Next, build the binary:
```bash
cargo build
```

# Usage

To use the tool, run the following command in your terminal:

```bash
cargo run input.gif output.gif
```

Replace `input.gif` with the path to your input GIF and `output.gif` with the path to the output GIF you want to create.

For bulk operation there is a Makefile that can be used to process all GIFs in a directory. To use it, run the following command in your terminal:

```bash
make all
```

Make sure that the `INPUT_DIR` and `OUTPUT_DIR` variables in the Makefile are set to the correct directories.

## License

This project is licensed under the GPLv2 License - see the [LICENSE](LICENSE) file for details.
