from platformio.public import DeviceMonitorFilterBase

class Visualizer(DeviceMonitorFilterBase):
    NAME = "visualizer"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._buffer = ""
        print("Visualizer filter is loaded")

    def rx(self, text):
        self._buffer += text
        result = []

        while True:
            # Look for start of message
            start = self._buffer.find("D{")
            if start == -1:
                break

            # Look for closing brace after the opening
            end = self._buffer.find("}", start + 2)
            if end == -1:
                # Incomplete message, wait for more data
                break

            # Extract the content between braces
            content = self._buffer[start + 2:end].rstrip("\r\n")
            result.append(f"Debug: {content}")

            # Remove processed message from buffer
            self._buffer = self._buffer[end + 1:]

        if result:
            return "\n".join(result) + "\n"
        return ""

    def tx(self, text):
        print(f"Sent: {text}\n")
        return text
