from pathlib import Path
import json
import unittest

ROOT = Path(__file__).resolve().parents[2]


class AssetsTest(unittest.TestCase):
    def test_native_sprites_and_font_cover_all_product_copy(self):
        path = ROOT / 'assets/leogie/manifest.json'
        self.assertTrue(path.exists(), 'Character assets have not been generated')
        manifest = json.loads(path.read_text())
        self.assertEqual(manifest['frame_size'], [72, 78])
        self.assertGreaterEqual(len(manifest['frames']), 8)
        self.assertLessEqual(len(manifest['frames']), 16)
        for frame in manifest['frames']:
            self.assertEqual((ROOT / frame['path']).stat().st_size, 72 * 78 * 2)
        font = json.loads((ROOT / 'assets/leogie/font-manifest.json').read_text())
        self.assertEqual(font['bpp'], 4)
        self.assertEqual(font['size'], 20)
        self.assertGreater(len(font['codepoints']), 6500)
        self.assertTrue(set(map(ord, '按一下开始翻译再按结束我在听正在播报网络已断开请重试没有听清中文英文音量配网')) <= set(font['codepoints']))


if __name__ == '__main__': unittest.main()
