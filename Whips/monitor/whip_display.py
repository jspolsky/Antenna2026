#!/usr/bin/env python3
"""Standalone pygame window for visualizing whip LED commands.

Communicates via UDP socket on localhost.
"""

import pygame
import socket
import json
import os
import ctypes
import platform
from PIL import Image

# Constants for the LED array
NUM_WHIPS = 24
LEDS_PER_WHIP = 110  # Matches NUM_LEDS in platformio.ini

# Display scaling
PIXELS_PER_LED = 2  # Each LED is 2 pixels tall
STRIP_HEIGHT = LEDS_PER_WHIP * PIXELS_PER_LED  # 220 pixels
STRIP_WIDTH = 5
STRIP_SPACING = 57  # Adjusted to make window ~2x wider
MARGIN = 20

# Communication
UDP_PORT = 19847

# GIF directory (relative to this script)
# Whips-Art is a sibling of Whips, so go up two levels from monitor/
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
GIF_DIR = os.path.join(SCRIPT_DIR, '..', '..', 'Whips-Art', 'Hex', 'sdflash')

# Shared library directory
LIB_DIR = os.path.join(SCRIPT_DIR, 'build')


class FlappyRenderer:
    """Wrapper for the FlappyRender shared library."""

    def __init__(self):
        self._lib = None
        self._buffer = None
        self._load_library()

    def _load_library(self):
        """Load the shared library."""
        if platform.system() == 'Darwin':
            lib_name = 'libFlappyRender.dylib'
        else:
            lib_name = 'libFlappyRender.so'

        lib_path = os.path.join(LIB_DIR, lib_name)

        if not os.path.exists(lib_path):
            print(f"FlappyRender library not found at: {lib_path}")
            print("Run 'make' in the monitor/ directory to build it.")
            return

        try:
            self._lib = ctypes.CDLL(lib_path)

            # Set up the function signature
            # void renderFlappyState(
            #     uint8_t gameState, uint16_t birdY, uint16_t score,
            #     int16_t pipe1X, uint16_t pipe1GapY,
            #     int16_t pipe2X, uint16_t pipe2GapY,
            #     int16_t pipe3X, uint16_t pipe3GapY,
            #     int16_t scrollX, uint8_t* rgbBuffer
            # );
            self._lib.renderFlappyState.argtypes = [
                ctypes.c_uint8,   # gameState
                ctypes.c_uint16,  # birdY
                ctypes.c_uint16,  # score
                ctypes.c_int16,   # pipe1X
                ctypes.c_uint16,  # pipe1GapY
                ctypes.c_int16,   # pipe2X
                ctypes.c_uint16,  # pipe2GapY
                ctypes.c_int16,   # pipe3X
                ctypes.c_uint16,  # pipe3GapY
                ctypes.c_int16,   # scrollX
                ctypes.POINTER(ctypes.c_uint8)  # rgbBuffer
            ]
            self._lib.renderFlappyState.restype = None

            # Pre-allocate the buffer (24 whips * 110 LEDs * 3 bytes per LED)
            self._buffer = (ctypes.c_uint8 * (NUM_WHIPS * LEDS_PER_WHIP * 3))()

            print("FlappyRender library loaded successfully")

        except Exception as e:
            print(f"Error loading FlappyRender library: {e}")
            self._lib = None

    def render(self, game_state, bird_y, score,
               pipe1_x, pipe1_gap_y, pipe2_x, pipe2_gap_y,
               pipe3_x, pipe3_gap_y, scroll_x):
        """Render the game state and return the RGB buffer as nested lists."""
        if self._lib is None:
            return None

        # Call the C++ function
        self._lib.renderFlappyState(
            ctypes.c_uint8(game_state),
            ctypes.c_uint16(bird_y),
            ctypes.c_uint16(score),
            ctypes.c_int16(pipe1_x),
            ctypes.c_uint16(pipe1_gap_y),
            ctypes.c_int16(pipe2_x),
            ctypes.c_uint16(pipe2_gap_y),
            ctypes.c_int16(pipe3_x),
            ctypes.c_uint16(pipe3_gap_y),
            ctypes.c_int16(scroll_x),
            self._buffer
        )

        # Convert buffer to nested lists: [whip][led] = (r, g, b)
        result = []
        for whip in range(NUM_WHIPS):
            whip_leds = []
            for led in range(LEDS_PER_WHIP):
                idx = (whip * LEDS_PER_WHIP + led) * 3
                r = self._buffer[idx]
                g = self._buffer[idx + 1]
                b = self._buffer[idx + 2]
                whip_leds.append((r, g, b))
            result.append(whip_leds)

        return result

    def is_available(self):
        """Check if the library is loaded and available."""
        return self._lib is not None


class GifCache:
    """Cache for loaded and decoded GIF frames."""

    def __init__(self):
        self._cache = {}  # gif_number -> list of frames, each frame is [whip][led] = (r,g,b)

    def get_frame(self, gif_number, frame_index):
        """Get a specific frame from a GIF, loading if necessary."""
        if gif_number not in self._cache:
            self._load_gif(gif_number)

        if gif_number not in self._cache:
            return None  # Failed to load

        frames = self._cache[gif_number]
        if not frames:
            return None

        # Mod frame index by number of frames (like the C++ code does)
        return frames[frame_index % len(frames)]

    def _load_gif(self, gif_number):
        """Load and decode a GIF file."""
        filename = os.path.join(GIF_DIR, f'{gif_number:03d}.gif')

        if not os.path.exists(filename):
            print(f"GIF file not found: {filename}")
            self._cache[gif_number] = []
            return

        try:
            img = Image.open(filename)
            frames = []

            for frame_idx in range(img.n_frames):
                img.seek(frame_idx)
                # Convert to RGB (GIFs might be palette-based)
                rgb_frame = img.convert('RGB')

                # Extract pixel data: frame_data[whip][led] = (r, g, b)
                frame_data = []
                for whip in range(NUM_WHIPS):
                    whip_leds = []
                    for led in range(LEDS_PER_WHIP):
                        if led < rgb_frame.width and whip < rgb_frame.height:
                            pixel = rgb_frame.getpixel((led, whip))
                            whip_leds.append(pixel)
                        else:
                            whip_leds.append((0, 0, 0))
                    frame_data.append(whip_leds)

                frames.append(frame_data)

            self._cache[gif_number] = frames
            print(f"Loaded GIF {gif_number}: {len(frames)} frames")

        except Exception as e:
            print(f"Error loading GIF {gif_number}: {e}")
            self._cache[gif_number] = []


def main():
    """Main display loop."""
    pygame.init()

    # Calculate window size
    width = MARGIN * 2 + NUM_WHIPS * STRIP_WIDTH + (NUM_WHIPS - 1) * STRIP_SPACING
    height = MARGIN * 2 + STRIP_HEIGHT

    screen = pygame.display.set_mode((width, height))
    pygame.display.set_caption("Whip Visualizer")

    # Set up UDP socket for receiving commands
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('127.0.0.1', UDP_PORT))
    sock.setblocking(False)

    clock = pygame.time.Clock()

    # Per-pixel colors for each whip: whip_pixels[whip][led] = (r, g, b)
    whip_pixels = [[(0, 0, 0) for _ in range(LEDS_PER_WHIP)] for _ in range(NUM_WHIPS)]
    whip_brightness = [255] * NUM_WHIPS  # Start at full brightness
    gif_cache = GifCache()
    flappy_renderer = FlappyRenderer()
    running = True

    while running:
        # Handle pygame events
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break

        # Process any pending UDP commands
        while True:
            try:
                data, addr = sock.recvfrom(4096)
                cmd = json.loads(data.decode('utf-8'))

                if cmd.get('type') == 'set_whip_color':
                    whip = cmd['whip']
                    r, g, b = cmd['r'], cmd['g'], cmd['b']
                    color = (r, g, b)
                    if whip == 255:  # ALL whips
                        for w in range(NUM_WHIPS):
                            for led in range(LEDS_PER_WHIP):
                                whip_pixels[w][led] = color
                    elif 0 <= whip < NUM_WHIPS:
                        for led in range(LEDS_PER_WHIP):
                            whip_pixels[whip][led] = color

                elif cmd.get('type') == 'set_brightness':
                    whip = cmd['whip']
                    brightness = cmd['brightness']
                    if whip == 255:  # ALL whips
                        for i in range(NUM_WHIPS):
                            whip_brightness[i] = brightness
                    elif 0 <= whip < NUM_WHIPS:
                        whip_brightness[whip] = brightness

                elif cmd.get('type') == 'self_identify':
                    whip = cmd['whip']
                    if whip == 255:  # ALL whips
                        for w in range(NUM_WHIPS):
                            set_identify_pattern(whip_pixels, w)
                    elif 0 <= whip < NUM_WHIPS:
                        set_identify_pattern(whip_pixels, whip)

                elif cmd.get('type') == 'show_gif_frame':
                    gif_number = cmd['gif_number']
                    frame = cmd['frame']
                    whip = cmd['whip']

                    frame_data = gif_cache.get_frame(gif_number, frame)
                    if frame_data:
                        if whip == 255:  # ALL whips
                            for w in range(NUM_WHIPS):
                                for led in range(LEDS_PER_WHIP):
                                    whip_pixels[w][led] = frame_data[w][led]
                        elif 0 <= whip < NUM_WHIPS:
                            for led in range(LEDS_PER_WHIP):
                                whip_pixels[whip][led] = frame_data[whip][led]

                elif cmd.get('type') == 'flappy_state':
                    if flappy_renderer.is_available():
                        frame_data = flappy_renderer.render(
                            cmd['game_state'],
                            cmd['bird_y'],
                            cmd['score'],
                            cmd['pipe1_x'],
                            cmd['pipe1_gap_y'],
                            cmd['pipe2_x'],
                            cmd['pipe2_gap_y'],
                            cmd['pipe3_x'],
                            cmd['pipe3_gap_y'],
                            cmd['scroll_x']
                        )
                        if frame_data:
                            for w in range(NUM_WHIPS):
                                for led in range(LEDS_PER_WHIP):
                                    whip_pixels[w][led] = frame_data[w][led]

            except BlockingIOError:
                break
            except Exception as e:
                print(f"Error processing command: {e}")
                break

        # Draw the display
        screen.fill((0, 0, 0))  # Black background

        for w in range(NUM_WHIPS):
            x = MARGIN + w * (STRIP_WIDTH + STRIP_SPACING)
            brightness = whip_brightness[w]

            for led in range(LEDS_PER_WHIP):
                r, g, b = whip_pixels[w][led]

                # Apply brightness to color
                display_color = (
                    r * brightness // 255,
                    g * brightness // 255,
                    b * brightness // 255
                )

                # Draw this LED (2 pixels tall) - pixel 0 at bottom, 119 at top
                y = MARGIN + (LEDS_PER_WHIP - 1 - led) * PIXELS_PER_LED
                pygame.draw.rect(screen, display_color, (x, y, STRIP_WIDTH, PIXELS_PER_LED))

            # Draw outline around entire strip
            pygame.draw.rect(screen, (51, 51, 51), (x, MARGIN, STRIP_WIDTH, STRIP_HEIGHT), 1)

        pygame.display.flip()
        clock.tick(60)  # 60 FPS

    sock.close()
    pygame.quit()


def set_identify_pattern(whip_pixels, whip_index):
    """Set the self-identify pattern for a whip, showing its number in binary."""
    whip_num = whip_index

    # Start with all black
    for led in range(LEDS_PER_WHIP):
        whip_pixels[whip_index][led] = (0, 0, 0)

    # 5 segments of 20 LEDs each
    for segment in range(5):
        bit_set = whip_num & 0x01

        # LEDs 0-16 of segment: White if bit set, Black otherwise
        for led in range(segment * 20, min((segment + 1) * 20 - 3, LEDS_PER_WHIP)):
            if bit_set:
                whip_pixels[whip_index][led] = (255, 255, 255)  # White

        # LEDs 17-19 of segment: Red separator
        for led in range((segment + 1) * 20 - 3, min((segment + 1) * 20, LEDS_PER_WHIP)):
            whip_pixels[whip_index][led] = (255, 0, 0)  # Red

        whip_num >>= 1


if __name__ == '__main__':
    main()
