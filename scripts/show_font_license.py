#!/usr/bin/env python3
"""Read the bundled font license directly from a standalone application/web BIN."""
import argparse
from pathlib import Path


def extract_license(data):
    start = b'LEOGIE_FONT_LICENSE_BEGIN\n'
    end = b'\nLEOGIE_FONT_LICENSE_END\n'
    a = data.find(start)
    b = data.find(end, a + len(start)) if a >= 0 else -1
    if a < 0 or b < 0 or b-a > 32768:
        raise ValueError('No complete bundled font license found')
    return data[a+len(start):b].decode('utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('image', type=Path)
    args = parser.parse_args()
    print(extract_license(args.image.read_bytes()))
