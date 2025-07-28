#!/usr/bin/env python3
"""
Upload files to LittleFS filesystem on Raspberry Pi Pico 2
This script creates a filesystem image and uploads it to the device.
"""

import os
import sys
import subprocess

def main():
    print("Creating LittleFS filesystem image...")
    
    # Check if data directory exists
    if not os.path.exists("data"):
        print("Error: data directory not found")
        sys.exit(1)
    
    # Create filesystem image
    try:
        # Use PlatformIO's filesystem upload
        result = subprocess.run(["pio", "run", "--target", "uploadfs"], 
                              capture_output=True, text=True)
        
        if result.returncode == 0:
            print("Filesystem uploaded successfully!")
            print(result.stdout)
        else:
            print("Filesystem upload failed:")
            print(result.stderr)
            
            # Try alternative method
            print("\nTrying alternative method...")
            result = subprocess.run(["pio", "run", "-t", "uploadfs"], 
                                  capture_output=True, text=True)
            
            if result.returncode == 0:
                print("Filesystem uploaded successfully with alternative method!")
                print(result.stdout)
            else:
                print("Alternative method also failed:")
                print(result.stderr)
                print("\nManual upload may be required.")
                print("Files to upload manually:")
                for root, dirs, files in os.walk("data"):
                    for file in files:
                        filepath = os.path.join(root, file)
                        print(f"  {filepath}")
    
    except FileNotFoundError:
        print("Error: PlatformIO not found. Please install PlatformIO.")
        sys.exit(1)

if __name__ == "__main__":
    main()