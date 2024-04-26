use gif::{Encoder, Frame};
use std::fs::File;
use std::cmp;
use std::io;
use std::io::Write;

const OUTPUT_WIDTH: u16 = 360;
const OUTPUT_HEIGHT: u16 = 32;
const DEG_OVERSAMPLING: u16 = 10;

// The center offset is the number of LEDs that are missing in the center of the output.
const CENTER_OFFSET: u16 = 3;
const OUTPUT_STEPS: u16 = OUTPUT_HEIGHT + CENTER_OFFSET;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let input_path = std::env::args().nth(1).expect("no input file given");
    let output_path = std::env::args().nth(2).expect("no output file given");

    let input = File::open(input_path).unwrap();
    let mut options = gif::DecodeOptions::new();
    options.set_color_output(gif::ColorOutput::RGBA);
    let mut decoder = options.read_info(input).unwrap();

    let palette = decoder.palette().unwrap();

    let mut output = File::create(output_path).unwrap();
    let mut encoder = Encoder::new(&mut output, OUTPUT_WIDTH, OUTPUT_HEIGHT, palette).unwrap();

    let mut count = 0;

    while let Some(frame) = decoder.read_next_frame().unwrap() {
        count += 1;

        print!("Processing frame {} ...\r", count);
        io::stdout().flush().unwrap();

        let new_frame = process_frame(&frame)?;
        encoder.write_frame(&new_frame).unwrap();
    }

    print!("Processed {} frame(s)        \n", count);

    Ok(())
}

fn process_frame(frame: &gif::Frame) -> Result<Frame<'static>, gif::EncodingError> {
    let mut output_buffer = vec![0; OUTPUT_WIDTH as usize * OUTPUT_HEIGHT as usize * 4];

    let (center_x, center_y) = (frame.width / 2, frame.height / 2);
    let vector_length = cmp::min(center_x, center_y);

    for deg in 0..OUTPUT_WIDTH {
        let mut r = 0;
        let mut g = 0;
        let mut b = 0;
        let mut count = 0;
        let mut current_output_y = 0;

        // Iterate over all pixels along the way of the vector and average their colors.
        // Oversample by the factor of DEG_OVERSAMPLING to get a smoother output.
        for i in 0..vector_length {
            for sub in 0..DEG_OVERSAMPLING {
                let a = deg as f32 + (sub as f32 / DEG_OVERSAMPLING as f32);
                let x: u32 = (center_x as f32 + i as f32 * a.to_radians().cos()).round() as u32;
                let y: u32 = (center_y as f32 + i as f32 * a.to_radians().sin()).round() as u32;
   
                r += frame.buffer[(y * (frame.width as u32) + x) as usize * 4] as u32;
                g += frame.buffer[(y * (frame.width as u32) + x) as usize * 4 + 1] as u32;
                b += frame.buffer[(y * (frame.width as u32) + x) as usize * 4 + 2] as u32;
                count += 1;
            }

            // If we have reached a step that falls into the next rasterized output line,
            // average the colors and write them to the output buffer.
            if i as f32 > (vector_length as f32 / OUTPUT_STEPS as f32) * current_output_y as f32 {
                current_output_y += 1;

                if current_output_y <= CENTER_OFFSET {
                    continue;
                }

                let index = ((current_output_y - CENTER_OFFSET - 1) * OUTPUT_WIDTH + deg) as usize * 4;

                output_buffer[index + 0] = (r / count) as u8;
                output_buffer[index + 1] = (g / count) as u8;
                output_buffer[index + 2] = (b / count) as u8;
                output_buffer[index + 3] = 255;

                r = 0;
                g = 0;
                b = 0;
                count = 0;
            }
        }
    }

    let mut new_frame = gif::Frame::from_rgba(OUTPUT_WIDTH, OUTPUT_HEIGHT, &mut output_buffer);

    // // Preserve other frame properties
    new_frame.delay = frame.delay;
    new_frame.dispose = frame.dispose;
    new_frame.transparent = frame.transparent;

    Ok(new_frame)
}
