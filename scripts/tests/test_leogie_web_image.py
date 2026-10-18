import importlib.util
from pathlib import Path
import struct
import hashlib
import unittest
from scripts.tests.test_leogie_release import image

ROOT = Path(__file__).resolve().parents[2]

def partitions():
    rows = [('nvs',1,2,0x9000,0x6000),('phy_init',1,1,0xf000,0x1000),
            ('factory',0,0,0x10000,0x340000),('legacy_cardid',1,2,0x356000,0x4000),
            ('leogie_font',1,0x40,0x360000,0x4a0000)]
    data = b''.join(struct.pack('<2sBBII16sI',b'\xaa\x50',t,s,o,n,name.encode(),0) for name,t,s,o,n in rows)
    data += b'\xeb\xeb'+b'\xff'*14+hashlib.md5(data).digest()
    return data.ljust(3072,b'\xff')

class WebImageTest(unittest.TestCase):
    def setUp(self):
        spec=importlib.util.spec_from_file_location('package_leogie',ROOT/'scripts/package_leogie.py')
        self.m=importlib.util.module_from_spec(spec);spec.loader.exec_module(self.m)

    def test_merge_contains_exact_images_and_stops_before_identity(self):
        boot,part,app=image(),partitions(),image(b'font-and-program')
        web=self.m.merge_web_image(boot,part,app)
        self.assertEqual(web[:len(boot)],boot)
        self.assertEqual(web[0x8000:0x8c00],part)
        self.assertEqual(web[0x10000:],app)
        self.m.verify_web_image(web)
        self.assertLessEqual((len(web)+4095)//4096*4096,0x350000)

    def test_corrupt_image_partition_padding_and_crossing_are_rejected(self):
        web=self.m.merge_web_image(image(),partitions(),image())
        bad=[]
        for offset in [0,35,0x8002,0x8500,0x9000,0x10023]:
            data=bytearray(web);data[offset]^=1;bad.append(bytes(data))
        bad += [web[:-1],web+b'\xff',web.ljust(0x356001,b'\xff'),b'']
        for raw in bad:
            with self.subTest(length=len(raw)),self.assertRaises(ValueError):self.m.verify_web_image(raw)
        with self.assertRaises(ValueError):self.m.merge_web_image(image(),partitions(),image(b'x'*0x340000))
