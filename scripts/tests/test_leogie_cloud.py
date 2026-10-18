import json
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]

class CloudConfigurationTest(unittest.TestCase):
    def test_release_uses_official_cloud_and_has_role_instructions(self):
        config=json.loads((ROOT/'main/boards/ai-passport-leogie/config.json').read_text())
        self.assertIn('CONFIG_OTA_URL="https://api.tenclass.net/xiaozhi/ota/"',config['builds'][0]['sdkconfig_append'])
        prompt=(ROOT/'docs/leogie-role.txt').read_text()
        for required in ('中文','英文','A1','A2','只输出','不要回答','数字','否定'):
            self.assertIn(required,prompt)

class CloudReceiveTest(unittest.TestCase):
    @unittest.skipUnless((ROOT/'managed_components/espressif__cjson/cJSON/cJSON.c').is_file(),
                         'Run ESP-IDF dependency resolution to provide cJSON')
    def test_production_receive_callbacks_without_custom_ack(self):
        import subprocess
        import tempfile
        cjson=ROOT/'managed_components/espressif__cjson/cJSON'
        with tempfile.TemporaryDirectory() as folder:
            obj=str(Path(folder)/'json.o'); binary=str(Path(folder)/'receive-test')
            subprocess.run(['cc','-I',str(cjson),'-c',str(cjson/'cJSON.c'),'-o',obj],check=True)
            subprocess.run(['c++','-std=c++17','-I',str(ROOT/'tests/translator/stubs'),
                            '-I',str(ROOT/'main'),'-I',str(cjson),
                            str(ROOT/'tests/translator/cloud_receive_test.cc'),
                            str(ROOT/'main/translator/leogie_application.cc'),obj,'-o',binary],check=True)
            subprocess.run([binary],check=True,timeout=10)
