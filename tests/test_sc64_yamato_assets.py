#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free Yamato export tests with synthetic DAT and GRP inputs."""
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_yamato_assets as yamato

class Sprites:
    @staticmethod
    def parse_grp(data):
        count=data[0]
        frame={'x':1,'y':0,'frame_width':2,'frame_height':1,'width':3,
               'alpha':bytes([0,1,1]), 'pixels':bytes([0,1,2])}
        return {'width':3,'height':1,'frames':[dict(frame) for _ in range(count)]}
    @staticmethod
    def crop_frame(frame): return frame['pixels'][1:3]

def fixture():
    dat=bytearray(999*10)
    for image,count,modifier in yamato.IMAGES:
        struct.pack_into('<I',dat,image*4,image-540)
        dat[999*8+image]=modifier
    paths={yamato.audit.grp_path(image-540):bytes([count]) for image,count,_ in yamato.IMAGES}
    paths['00C/000']=bytes(dat)
    return paths

class TestYamatoAssets(unittest.TestCase):
    def test_collect_dedup_and_render(self):
        paths=fixture(); palette={'gba_rgb555':[0]*256}
        with mock.patch.object(yamato.source,'_project_luts',return_value=([{'modifier':9,'rows':63}],bytes(63*256),{})):
            pixels,lut,rows=yamato.collect(paths.__getitem__,Sprites,bytes(range(256)),palette)
        self.assertEqual(len(rows),161);self.assertEqual(pixels,bytes([1,2]));self.assertEqual(len(lut),63*256)
        self.assertEqual(rows[0],(541,0,0,1,0,2,1,3,1,0));self.assertTrue(all(row[-1]==0 for row in rows))
        text=yamato.render(pixels,lut,rows);self.assertIn('#define SC64_YAMATO_FRAME_COUNT 161',text)
    def test_bad_modifier_frame_count_and_palette_fail(self):
        palette={'gba_rgb555':[0]*256}; paths=fixture()
        with self.assertRaises(ValueError):yamato.collect(paths.__getitem__,Sprites,b'',palette)
        altered=bytearray(paths['00C/000']);altered[999*8+541]=9;paths['00C/000']=bytes(altered)
        with self.assertRaisesRegex(ValueError,'modifier'):yamato.collect(paths.__getitem__,Sprites,bytes(range(256)),palette)
        paths=fixture();paths[yamato.audit.grp_path(1)]=bytes([67])
        with self.assertRaisesRegex(ValueError,'frame count'):yamato.collect(paths.__getitem__,Sprites,bytes(range(256)),palette)
    def test_public_source_output_rejected_before_read(self):
        out=yamato.ROOT / "tests" / "yamato-rejected-output"
        self.assertFalse(out.exists())
        with self.assertRaisesRegex(ValueError,"source tree"):
            yamato.generate(Path("missing-rom"),out,Path("missing"),Path("missing"))
        self.assertFalse(out.exists())
    def test_opaque_zero_remap_rejected(self):
        paths=fixture()
        with self.assertRaisesRegex(ValueError,"transparent"):
            yamato.collect(paths.__getitem__,Sprites,bytes(256),{"gba_rgb555":[0]*256})
    def test_rom_authentication_precedes_output(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);rom=root/'bad.rom';rom.write_bytes(b'wrong');out=root/'out'
            with self.assertRaisesRegex(ValueError,'SHA-1'):yamato.generate(rom,out,root/'missing',root/'missing')
            self.assertFalse(out.exists())
if __name__=='__main__':unittest.main()
