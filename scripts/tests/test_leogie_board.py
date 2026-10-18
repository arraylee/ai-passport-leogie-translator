import json
import re
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
BOARD = ROOT / 'main/boards/ai-passport-leogie'


class LeogieBoardTest(unittest.TestCase):
    def test_es8311_shared_clock_rates_match(self):
        config = (BOARD / 'config.h').read_text()
        rates = dict(re.findall(r'#define AUDIO_(INPUT|OUTPUT)_SAMPLE_RATE\s+(\d+)', config))
        self.assertEqual(rates['INPUT'], rates['OUTPUT'],
                         'ES8311 duplex driver aborts at boot when RX/TX rates differ')

    def test_unique_c3_build_and_safe_partitions(self):
        self.assertTrue((BOARD / 'config.json').is_file(), 'Leogie board missing')
        config = json.loads((BOARD / 'config.json').read_text())
        self.assertEqual(config['target'], 'esp32c3')
        self.assertEqual(config['type'], 'ai-passport-leogie')
        options = config['builds'][0]['sdkconfig_append']
        self.assertIn('CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y', options)
        self.assertIn('CONFIG_FLASH_NONE_ASSETS=y', options)
        rows = []
        for line in (ROOT / 'partitions/leogie.csv').read_text().splitlines():
            if line.strip() and not line.startswith('#'):
                fields = [x.strip() for x in line.split(',')]
                rows.append((fields[0], int(fields[3], 0), int(fields[4], 0)))
        self.assertIn(('legacy_cardid', 0x356000, 0x4000), rows)
        for name, start, size in rows:
            self.assertLessEqual(start + size, 0x800000)
            if name != 'legacy_cardid':
                self.assertTrue(start + size <= 0x356000 or start >= 0x35A000)
        for left, right in zip(sorted(rows, key=lambda r: r[1]), sorted(rows, key=lambda r: r[1])[1:]):
            self.assertLessEqual(left[1] + left[2], right[1])

    def test_no_efuse_writes_or_other_board_factory(self):
        self.assertTrue(BOARD.is_dir(), 'Leogie board missing')
        source = '\n'.join(p.read_text() for p in BOARD.glob('*.cc'))
        self.assertEqual(source.count('DECLARE_BOARD('), 1)
        self.assertNotIn('esp_efuse_write', source)


if __name__ == '__main__':
    unittest.main()
