from platformio.public import DeviceMonitorFilterBase
import re
import subprocess
import socket
import json
import os

# Constants for the LED array
NUM_WHIPS = 24

# Communication
UDP_PORT = 19847


class Visualizer(DeviceMonitorFilterBase):
    NAME = "visualizer"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._buffer = ""
        self._sock = None
        self._display_process = None
        self._start_display()
        print("Visualizer filter is loaded")

    def _start_display(self):
        """Start the display in a separate process."""
        # Get the path to whip_display.py (same directory as this file)
        this_dir = os.path.dirname(os.path.abspath(__file__))
        display_script = os.path.join(this_dir, 'whip_display.py')

        # Launch the display as a separate process
        # Use the same Python that's running this script to ensure dependencies are available
        import sys
        self._display_process = subprocess.Popen(
            [sys.executable, display_script],
            stdout=subprocess.DEVNULL,
            stderr=None  # Show errors in terminal
        )

        # Set up UDP socket for sending commands
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def _send_command(self, cmd):
        """Send a command to the display via UDP."""
        try:
            data = json.dumps(cmd).encode('utf-8')
            self._sock.sendto(data, ('127.0.0.1', UDP_PORT))
        except Exception as e:
            print(f"Error sending command: {e}")

    def rx(self, text):
        self._buffer += text
        result = []

        while True:
            # Look for start of message with length prefix: D<length>{ or V<length>{
            match = re.search(r'([DV])(\d+)\{', self._buffer)
            if not match:
                break

            # Discard any data before the message start
            if match.start() > 0:
                self._buffer = self._buffer[match.start():]
                match = re.match(r'([DV])(\d+)\{', self._buffer)

            msg_type = match.group(1)
            length = int(match.group(2))
            content_start = match.end()  # Position right after the '{'

            # Check if we have enough bytes for the full message plus closing '}'
            if len(self._buffer) < content_start + length + 1:
                # Incomplete message, wait for more data
                break

            # Extract exactly 'length' bytes of content
            content = self._buffer[content_start:content_start + length]

            if msg_type == 'D':
                result.append(f"Debug: {content.rstrip()}")
            else:  # msg_type == 'V'
                viz_result = self.visualize(content)
                if viz_result:  # Skip empty strings
                    result.append(viz_result)

            # Remove processed message from buffer (content + closing '}')
            self._buffer = self._buffer[content_start + length + 1:]

        if result:
            return "\n".join(result) + "\n"
        return ""

    def visualize(self, content):
        if len(content) < 4:
            hex_bytes = ' '.join(f'{ord(c):02X}' for c in content)
            return f"Visualize: (too short) {hex_bytes}"

        # Skip checksum (bytes 0-1), get command (byte 2) and whip (byte 3)
        command = content[2]
        whip = ord(content[3])
        whip_str = 'ALL' if whip == 255 else str(whip)

        # Parse command-specific parameters (starting at byte 4)
        params = content[4:]

        if command == 'c':  # Set Whip Color - CRGB (3 bytes: R, G, B)
            if len(params) >= 3:
                r, g, b = ord(params[0]), ord(params[1]), ord(params[2])
                # Update the graphical display
                self._send_command({
                    'type': 'set_whip_color',
                    'whip': whip,
                    'r': r,
                    'g': g,
                    'b': b
                })
                return ""  # Shown in visualizer window
            return f"Visualize: Set Whip Color (Whip: {whip_str}, RGB: ?)"

        elif command == 'g':  # Show GIF Frame - uint32_t frame, uint16_t iGifNumber
            if len(params) >= 6:
                frame = ord(params[0]) | (ord(params[1]) << 8) | (ord(params[2]) << 16) | (ord(params[3]) << 24)
                gif_num = ord(params[4]) | (ord(params[5]) << 8)
                # Update the graphical display
                self._send_command({
                    'type': 'show_gif_frame',
                    'whip': whip,
                    'frame': frame,
                    'gif_number': gif_num
                })
                return ""  # Shown in visualizer window
            return f"Visualize: Show GIF Frame (Whip: {whip_str}, Frame: ?)"

        elif command == 'b':  # Set Brightness - uint8_t brightness
            if len(params) >= 1:
                brightness = ord(params[0])
                # Update the graphical display
                self._send_command({
                    'type': 'set_brightness',
                    'whip': whip,
                    'brightness': brightness
                })
                return ""  # Shown in visualizer window
            return f"Visualize: Set Brightness (Whip: {whip_str}, Brightness: ?)"

        elif command == 'i':  # Self Identify - no extra params
            # Update the graphical display
            self._send_command({
                'type': 'self_identify',
                'whip': whip
            })
            return ""  # Shown in visualizer window

        elif command == 'f':  # Flappy Bird game state
            if len(params) >= 19:
                # Parse the cmdFlappyState structure
                game_state = ord(params[0])
                bird_y = ord(params[1]) | (ord(params[2]) << 8)
                score = ord(params[3]) | (ord(params[4]) << 8)

                # Pipe 1 (int16_t X, uint16_t GapY)
                pipe1_x = ord(params[5]) | (ord(params[6]) << 8)
                if pipe1_x >= 0x8000:
                    pipe1_x -= 0x10000  # Convert to signed
                pipe1_gap_y = ord(params[7]) | (ord(params[8]) << 8)

                # Pipe 2
                pipe2_x = ord(params[9]) | (ord(params[10]) << 8)
                if pipe2_x >= 0x8000:
                    pipe2_x -= 0x10000
                pipe2_gap_y = ord(params[11]) | (ord(params[12]) << 8)

                # Pipe 3
                pipe3_x = ord(params[13]) | (ord(params[14]) << 8)
                if pipe3_x >= 0x8000:
                    pipe3_x -= 0x10000
                pipe3_gap_y = ord(params[15]) | (ord(params[16]) << 8)

                # Scroll X (int16_t)
                scroll_x = ord(params[17]) | (ord(params[18]) << 8)
                if scroll_x >= 0x8000:
                    scroll_x -= 0x10000

                # Send to visualizer
                self._send_command({
                    'type': 'flappy_state',
                    'game_state': game_state,
                    'bird_y': bird_y,
                    'score': score,
                    'pipe1_x': pipe1_x,
                    'pipe1_gap_y': pipe1_gap_y,
                    'pipe2_x': pipe2_x,
                    'pipe2_gap_y': pipe2_gap_y,
                    'pipe3_x': pipe3_x,
                    'pipe3_gap_y': pipe3_gap_y,
                    'scroll_x': scroll_x
                })
                return ""  # Shown in visualizer window
            return f"Visualize: Flappy State (incomplete data)"

        else:
            hex_bytes = ' '.join(f'{ord(c):02X}' for c in content)
            return f"Visualize: Unknown '{command}' (Whip: {whip_str}) {hex_bytes}"

    def tx(self, text):
        return text
