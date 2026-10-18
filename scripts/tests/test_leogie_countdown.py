from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]

class CountdownTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which('c++'), 'C++ compiler required')
    def test_deadline_bounds_cancel_and_restart(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / 'countdown')
            subprocess.run(['c++', '-std=c++17', '-I', str(ROOT / 'main'),
                            str(ROOT / 'tests/leogie_countdown_test.cc'), '-o', binary], check=True)
            subprocess.run([binary], check=True, timeout=10)

    @unittest.skipUnless(shutil.which('c++'), 'C++ compiler required')
    def test_real_alarm_fits_nonblocking_audio_queue(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / 'alarm')
            subprocess.run(['c++', '-std=c++17', '-I', str(ROOT / 'main'),
                '-I', str(ROOT / 'tests/timer/stubs'),
                str(ROOT / 'tests/leogie_alarm_test.cc'),
                str(ROOT / 'main/audio/demuxer/ogg_demuxer.cc'),
                str(ROOT / 'main/boards/ai-passport-leogie/leogie_alarm.cc'),
                '-o', binary], check=True)
            subprocess.run([binary], check=True, timeout=10)

    @unittest.skipUnless(shutil.which('c++'), 'C++ compiler required')
    def test_rapid_releases_and_long_press_suppression(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / 'keys')
            subprocess.run(['c++', '-std=c++17', '-I', str(ROOT / 'main'),
                            str(ROOT / 'tests/leogie_timer_keys_test.cc'), '-o', binary], check=True)
            subprocess.run([binary], check=True, timeout=10)
