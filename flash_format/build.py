#!/usr/bin/env python3
import argparse
import subprocess
import os
import sys

# Configuration
FQBN = "adafruit:samd:adafruit_pygamer_m4"
PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_DIR = os.path.join(PROJECT_DIR, "bin")
SKETCH = os.path.join(PROJECT_DIR, "Software_Rasterizer.ino")

# Ensure bin/ exists
os.makedirs(BIN_DIR, exist_ok=True)

# Argument parsing
parser = argparse.ArgumentParser(description="Build and upload Arduino project using arduino-cli.")
group = parser.add_mutually_exclusive_group(required=True)
group.add_argument("--compile", action="store_true", help="Only compile the project.")
group.add_argument("--upload", action="store_true", help="Only upload the project (requires prior compilation).")
group.add_argument("--all", action="store_true", help="Compile and upload the project.")
parser.add_argument("--port", type=str, help="Serial port for upload (e.g. /dev/ttyACM0)")
args = parser.parse_args()

# Compile command
compile_cmd = [
    "arduino-cli", "compile",
    "-v",
    "--fqbn", FQBN,
    "--output-dir", BIN_DIR,
    PROJECT_DIR
]

# Find .bin file after compile
def find_bin_file():
    for f in os.listdir(BIN_DIR):
        if f.endswith(".bin"):
            return os.path.join(BIN_DIR, f)
    return None

# Upload command
def upload_cmd(port=None):
    bin_file = find_bin_file()
    if not bin_file:
        print("Error: Compiled binary not found in bin/. Run with --compile or --all first.")
        sys.exit(1)
    cmd = [
        "arduino-cli", "upload", "-v",
        "-p", port if port else "auto",
        "--fqbn", FQBN,
        "--input-file", bin_file,
        PROJECT_DIR
    ]
    return cmd

# Actions
if args.compile or args.all:
    print("Compiling...")
    result = subprocess.run(compile_cmd)
    if result.returncode != 0:
        print("Compilation failed.")
        sys.exit(result.returncode)
    print("Compilation successful. Binary stored in bin/.")

if args.upload or args.all:
    port = args.port
    print(f"Uploading{' to ' + port if port else ''}...")
    result = subprocess.run(upload_cmd(port))
    if result.returncode != 0:
        print("Upload failed.")
        sys.exit(result.returncode)
    print("Upload successful.")
