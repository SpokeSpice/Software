from PIL import Image, ImageDraw, ImageSequence
import numpy as np
import math

def process_frame(frame, segments, vector_steps):
    center_x, center_y = frame.width / 2, frame.height / 2
    vector_length = min(frame.width, frame.height) / 2
    output_image = Image.new("RGB", (360, segments))
    draw = ImageDraw.Draw(output_image)

    for col in range(360):
        avg_colors = np.zeros((segments, 3), dtype=np.float64)
        counts = np.zeros(segments, dtype=int)

        # Averaging colors over specified vector steps for enhanced detail
        for step in range(vector_steps):
            angle_degrees = (col * vector_steps + step) / vector_steps * (360 / 360)
            angle_radians = math.radians(angle_degrees)

            for segment in range(segments):
                # Calculate end points based on the vector length and segment
                distance_ratio = (segment + 1) / segments
                end_x = center_x + math.cos(angle_radians) * distance_ratio * vector_length
                end_y = center_y + math.sin(angle_radians) * distance_ratio * vector_length

                x = int(end_x)
                y = int(end_y)

                if 0 <= x < frame.width and 0 <= y < frame.height:
                    pixel = frame.getpixel((x, y))
                    avg_colors[segment] += np.array(pixel)
                    counts[segment] += 1

        # Calculate and draw the average color for each segment
        for segment in range(segments):
            if counts[segment] > 0:
                color = tuple((avg_colors[segment] / counts[segment]).astype(np.uint8))
                draw.point((col, segment), fill=color)

    return output_image

def create_quantized_gif(input_path, output_path, segments=32, vector_steps=10):
    with Image.open(input_path) as input_gif:
        frames = []
        durations = []

        for frame in ImageSequence.Iterator(input_gif):
            frame_duration = frame.info.get('duration', 100)  # Default duration 100ms
            durations.append(frame_duration)
            processed_frame = process_frame(frame.convert("RGB"), segments, vector_steps)
            frames.append(processed_frame)

        # Save processed frames as a new GIF
        frames[0].save(output_path, save_all=True, append_images=frames[1:], loop=0, duration=durations, dispose=2)

# Usage: $0 input.gif output.gif
if __name__ == "__main__":
    # get cli parameters
    import sys
    if len(sys.argv) != 3:
        print("Usage: $0 input.gif output.rgif")
        sys.exit(1)
    input_gif = sys.argv[1]
    output_gif = sys.argv[2]

    create_quantized_gif(input_gif, output_gif)
