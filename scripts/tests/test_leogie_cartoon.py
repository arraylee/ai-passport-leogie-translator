import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CartoonAssetsTest(unittest.TestCase):
    def test_flash_native_sprites_preserve_alpha_and_budget(self):
        path = ROOT / 'assets/leogie/cartoon/manifest.json'
        self.assertTrue(path.exists(), 'Cartoon sprite pack has not been generated')
        manifest = json.loads(path.read_text())
        self.assertEqual(manifest['poses'], ['idle', 'listening', 'thinking', 'speaking'])
        total = 0
        for size, width, height in [('large', 180, 160), ('small', 120, 106)]:
            for pose in manifest['poses']:
                data = (path.parent / f'{pose}-{size}.i8').read_bytes()
                self.assertEqual(len(data), 1024 + width * height)
                alphas = data[3:1024:4]
                self.assertIn(0, alphas)
                self.assertGreaterEqual(max(alphas), 250)
                total += len(data)
        self.assertLess(total, 180 * 1024)
