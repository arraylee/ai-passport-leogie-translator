#!/usr/bin/env python3
"""Independent CLI entrypoint for the safe Leogie raw image at address zero."""
from pathlib import Path
import argparse
import hashlib
try:
    from package_leogie import verify_web_image
except ImportError:
    from verify_release import verify_web_image

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('image', type=Path)
    args = parser.parse_args()
    data = args.image.read_bytes()
    verify_web_image(data)
    print(f'Verified {len(data)} bytes; erase end 0x{(len(data)+4095)//4096*4096:x}; SHA256 {hashlib.sha256(data).hexdigest()}')
