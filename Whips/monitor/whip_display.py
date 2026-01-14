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
STRIP_HEIGHT = 240  # pixels (2x LED count for visibility)
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
    whip_colors = [(0, 0, 0)] * NUM_WHIPS  # Start all black
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
                data, addr = sock.recvfrom(1024)
                cmd = json.loads(data.decode('utf-8'))

                if cmd.get('type') == 'set_whip_color':
                    whip = cmd['whip']
                    r, g, b = cmd['r'], cmd['g'], cmd['b']
                    if whip == 255:  # ALL whips
                        for i in range(NUM_WHIPS):
                            whip_colors[i] = (r, g, b)
                    elif 0 <= whip < NUM_WHIPS:
                        whip_colors[whip] = (r, g, b)

                elif cmd.get('type') == 'set_brightness':
                    whip = cmd['whip']
                    brightness = cmd['brightness']
                    if whip == 255:  # ALL whips
                        for i in range(NUM_WHIPS):
                            whip_brightness[i] = brightness
                    elif 0 <= whip < NUM_WHIPS:
                        whip_brightness[whip] = brightness
            except BlockingIOError:
                break
            except Exception as e:
                print(f"Error processing command: {e}")
                break

        # Draw the display
        screen.fill((0, 0, 0))  # Black background

        for i in range(NUM_WHIPS):
            x = MARGIN + i * (STRIP_WIDTH + STRIP_SPACING)
            r, g, b = whip_colors[i]
            brightness = whip_brightness[i]

            # Apply brightness to color
            display_color = (
                r * brightness // 255,
                g * brightness // 255,
                b * brightness // 255
            )

            # Draw the whip rectangle
            pygame.draw.rect(screen, display_color, (x, MARGIN, STRIP_WIDTH, STRIP_HEIGHT))

            # Draw outline
            pygame.draw.rect(screen, (51, 51, 51), (x, MARGIN, STRIP_WIDTH, STRIP_HEIGHT), 1)

        pygame.display.flip()
        clock.tick(60)  # 60 FPS

    sock.close()
    pygame.quit()


if __name__ == '__main__':
    main()
