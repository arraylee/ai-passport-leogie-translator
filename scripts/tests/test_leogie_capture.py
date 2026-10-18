from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class CaptureCancellationTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which('c++'), 'C++ compiler required')
    def test_inflight_encoder_and_microphone_read_cannot_cross_restart(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory)/'capture-test')
            subprocess.run(['c++', '-std=c++17', '-pthread', '-I', str(ROOT/'main'),
                            str(ROOT/'tests/leogie_capture_test.cc'), '-o', binary], check=True)
            subprocess.run([binary], check=True, timeout=10)
