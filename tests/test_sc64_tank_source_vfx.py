"""Optional source-bank import regression; requires the authenticated local ROM and palette assets."""
from pathlib import Path
import json
import importlib.util
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXTRACTOR = ROOT/'references/sc64-maps/extract_sc64_maps.py'
TOOL = ROOT/'tools/sc64_tank_source_vfx.py'
if not TOOL.is_file(): TOOL = Path(__file__).resolve().parent/'sc64_tank_source_vfx.py'
ROM = ROOT/'.local/sc64/rom/StarCraft64-USA.z64'
PALETTE = ROOT/'.local/sc64/backwater/generated/palette.json'
REMAP = ROOT/'.local/sc64/backwater/generated/palette-remap.bin'

class TankSourceVfxImportTest(unittest.TestCase):
    @unittest.skipUnless(EXTRACTOR.is_file() and ROM.is_file() and PALETTE.is_file() and REMAP.is_file(), 'requires authenticated BOLT extractor, local StarCraft64 ROM and palette inputs')
    def test_python_generate_api(self):
        spec=importlib.util.spec_from_file_location('sc64_tank_source_vfx',TOOL)
        module=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory(prefix='sc64-tank-vfx-api-') as tmp:
            manifest=module.generate(ROM,REMAP,PALETTE,Path(tmp))
            self.assertEqual(manifest['rom_sha1'],'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b')
            self.assertEqual(len((Path(tmp)/'sc64_tank_source_vfx_generated.h').read_bytes())>0,True)

    @unittest.skipUnless(EXTRACTOR.is_file() and ROM.is_file() and PALETTE.is_file() and REMAP.is_file(), 'requires authenticated BOLT extractor, local StarCraft64 ROM and palette inputs')
    def test_imports_verified_frames_and_event_clock(self):
        with tempfile.TemporaryDirectory(prefix='sc64-tank-vfx-') as tmp:
            out=Path(tmp)
            subprocess.run([sys.executable,str(TOOL),'--rom',str(ROM),'--palette-json',str(PALETTE),
                            '--palette-remap',str(REMAP),'--out',str(out)],check=True)
            manifest=json.loads((out/'manifest.json').read_text())
            self.assertEqual(manifest['rom_sha1'],'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b')
            self.assertEqual({int(i):m['frame_count'] for i,m in manifest.items() if i.isdigit()}, {531:10,533:14,536:17,537:119})
            self.assertEqual(manifest['domissiledmg_events'],{'531':[2],'533':[3]})
            self.assertEqual(manifest['verified_weapon_chains']['tank']['image'],531)
            self.assertEqual(manifest['verified_weapon_chains']['siege']['image'],533)
            self.assertEqual({int(i):len(v) for i,v in manifest['descriptors'].items()}, {531:10,533:14,536:17,537:119})
            self.assertEqual(manifest['source_lookup']['binding'].split(':')[0],'INFERRED')
            self.assertTrue((out/'sc64_tank_source_vfx_generated.h').is_file())
            header=(out/'sc64_tank_source_vfx_generated.h').read_text()
            self.assertIn('#define SC64_TANK_SOURCE_VFX_VERSION 1',header)
            self.assertIn('sc64_tank_source_pixels[',header)
            raw=(out/'raw_modifier9_table0.bin').read_bytes()
            projected=(out/'projected_modifier9_lut.bin').read_bytes()
            remap=REMAP.read_bytes()
            palette=json.loads(PALETTE.read_text())
            source=palette['source_rgb555']; gba=palette['gba_rgb555']
            preimages={d:[] for d in range(256)}
            for source_index,dest_index in enumerate(remap): preimages[dest_index].append(source_index)
            def distance(a,b):
                return sum((((int(gba[a])>>shift)&31)-((int(source[b])>>shift)&31))**2 for shift in (0,5,10))
            reverse=[]
            missing=[]
            for dest in range(256):
                choices=preimages[dest]
                if not choices:
                    missing.append(dest)
                    choices=range(256)
                reverse.append(min(choices,key=lambda ix:(distance(dest,ix),ix)))
            self.assertGreater(len(missing),0)
            self.assertEqual(len(raw),63*256)
            self.assertEqual(len(projected),63*256)
            expected=bytes(remap[raw[(strength-1)*256+reverse[dest]]]
                           for strength in range(1,64) for dest in range(256))
            self.assertEqual(projected,expected,
                             'all 63 strength rows and 256 destinations must use the independently derived inverse palette')

if __name__=='__main__': unittest.main()
