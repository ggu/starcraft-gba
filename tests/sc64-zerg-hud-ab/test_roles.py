# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from sc64_zerg_hud_pair import verify_build_roles
class Roles(unittest.TestCase):
    def test_swapped_profile_and_elf_are_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);files={n:root/n for n in ('z9-off-rom','z9-off-elf','z9-on-rom','z9-on-elf','z10-off-rom')}
            for n,p in files.items():p.write_bytes(n.encode())
            def entry(p):return {'path':str(p.resolve()),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
            record={'products':{'z9':{'off':{'rom':entry(files['z9-off-rom']),'elf':entry(files['z9-off-elf'])},
                                        'on':{'rom':entry(files['z9-on-rom']),'elf':entry(files['z9-on-elf'])}}}}
            args=[files[n] for n in ('z9-off-rom','z9-off-elf','z9-on-rom','z9-on-elf')]
            verify_build_roles(record,'z9',*args)
            for profile,values in [('z10',args),('z9',[args[2],args[1],args[0],args[3]]),
                                   ('z9',[args[0],args[3],args[2],args[1]]),
                                   ('z9',[files['z10-off-rom'],args[1],args[2],args[3]])]:
                with self.assertRaises(ValueError):verify_build_roles(record,profile,*values)
if __name__=='__main__':unittest.main()
