#!/usr/bin/env python3
"""Standalone pygame window for visualizing whip LED commands.

Communicates via UDP socket on localhost.
"""

import pygame
import socket
import json

# Constants for the LED array
NUM_WHIPS = 24
LEDS_PER_WHIP = 120

# Display scaling
PIXELS_PER_LED = 2  # Each LED is 2 pixels tall
STRIP_HEIGHT = LEDS_PER_WHIP * PIXELS_PER_LED  # 240 pixels
STRIP_WIDTH = 5
STRIP_SPACING = 57  # Adjusted to make window ~2x wider
MARGIN = 20

# Communication
UDP_PORT = 19847


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
        for led in range(segment * 20, (segment + 1) * 20 - 3):
            if bit_set:
                whip_pixels[whip_index][led] = (255, 255, 255)  # White

        # LEDs 17-19 of segment: Red separator
        for led in range((segment + 1) * 20 - 3, (segment + 1) * 20):
            whip_pixels[whip_index][led] = (255, 0, 0)  # Red

        whip_num >>= 1


if __name__ == '__main__':
    main()
