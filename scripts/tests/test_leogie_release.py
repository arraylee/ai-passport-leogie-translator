import hashlib
import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]


def image(payload=b'\x01\x02\x03\x04'):
    header = bytearray(24)
    header[0:4] = bytes([0xE9, 1, 2, 0x3F])
    header[12:14] = struct.pack('<H', 5)
    header[23] = 1
    raw = header + struct.pack('<II', 0x3c000000, len(payload)) + payload
    checksum = 0xEF
    for byte in payload: checksum ^= byte
    raw += b'\0' * (15 - len(raw) % 16) + bytes([checksum])
    return bytes(raw) + hashlib.sha256(raw).digest()


class ReleaseTests(unittest.TestCase):
    def module(self):
        path = ROOT/'scripts/package_leogie.py'
        self.assertTrue(path.exists(), 'Release verifier missing')
        spec = importlib.util.spec_from_file_location('package_leogie', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_image_integrity_chip_and_truncation(self):
        m = self.module()
        m.verify_esp_image(image())
        for bad in [b'', image()[:-1], image()[:35] + b'\xff' + image()[36:],
                    image()[:12] + b'\x09\0' + image()[14:]]:
            with self.assertRaises(ValueError): m.verify_esp_image(bad)

    def test_protected_sectors_flash_capacity_overlap_and_empty_rejected(self):
        m = self.module()
        m.verify_ranges([(0, 24000), (0x8000, 3072), (0x10000, 0x330000), (0x360000, 4439696)])
        for ranges in [[(0, 0x360000)], [(0x356000, 1)], [(0x355FFF, 2)],
                       [(0x359FFF, 1)], [(0x800000, 1)], [(0, 0)],
                       [(0, 0x8001), (0x8000, 100)], [(1, 20)]]:
            with self.assertRaises(ValueError): m.verify_ranges(ranges)

    def test_current_font_image_matches_manifest_and_partition(self):
        import json
        m = self.module()
        data = (ROOT/'assets/leogie/font-bitmap.bin').read_bytes()
        manifest = json.loads((ROOT/'assets/leogie/font-manifest.json').read_text())
        m.verify_font(data, manifest)
        with self.assertRaises(ValueError): m.verify_font(data[:-1], manifest)
        corrupt = bytes([data[0] ^ 1]) + data[1:]
        with self.assertRaises(ValueError): m.verify_font(corrupt, manifest)


if __name__ == '__main__': unittest.main()
