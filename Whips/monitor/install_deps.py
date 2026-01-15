"""
PlatformIO pre-script to install Python dependencies for the visualizer.
Runs automatically before build in the visualizer environment.
"""
Import("env")

import subprocess
import sys

REQUIRED_PACKAGES = ["pygame", "Pillow"]

def install_python_deps(*args, **kwargs):
    for package in REQUIRED_PACKAGES:
        try:
            __import__(package if package != "Pillow" else "PIL")
        except ImportError:
            print(f"Installing {package}...")
            subprocess.check_call([sys.executable, "-m", "pip", "install", package])

# Run at script load time (before build)
install_python_deps()
