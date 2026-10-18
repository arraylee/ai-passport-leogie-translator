import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]

class EmbeddedFontTest(unittest.TestCase):
    def test_all_gb2312_han_and_ui_are_embedded_at_original_quality(self):
        m = json.loads((ROOT/'assets/leogie/font-manifest.json').read_text())
        self.assertEqual(m.get('storage'), 'embedded-app')
        self.assertEqual((m['size'], m['bpp'], m['line_height']), (20, 4, 29))
        points = set(m['codepoints'])
        han = set()
        for a in range(0xb0, 0xf8):
            for b in range(0xa1, 0xff):
                try: han.add(ord(bytes([a,b]).decode('gb2312')))
                except UnicodeDecodeError: pass
        self.assertEqual(len(han), 6763)
        self.assertTrue(han <= points)
        source = (ROOT/'main/boards/ai-passport-leogie/leogie_display.cc').read_text()
        ui = ''.join(re.findall(r'"([^"\n]*)"', source))
        self.assertTrue({ord(c) for c in ui if ord(c)>127} <= points)
        font = (ROOT/'main/boards/ai-passport-leogie/leogie_font20.c').read_text()
        self.assertIn('.glyph_bitmap = glyph_bitmap', font)
        self.assertNotIn('.glyph_bitmap = NULL', font)
        self.assertNotIn('esp_partition_mmap', source)
        self.assertLess(m['bitmap_bytes'], 1200000)
        self.assertIn('LEOGIE_FONT_LICENSE_BEGIN', font)
        self.assertIn('SIL OPEN FONT LICENSE Version 1.1', font)
        self.assertIn('.user_data = (void*)leogie_font_license', font)
