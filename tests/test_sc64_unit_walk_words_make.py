# SPDX-License-Identifier: GPL-3.0-or-later
"""Mode, source and generator changes must invalidate managed sidecars."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]

class WalkWordsMakeTest(unittest.TestCase):
    def test_rapid_switch_and_same_tick_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory); (root/'Makefile').write_bytes((ROOT/'Makefile').read_bytes())
            (root/'tools').mkdir()
            for source in (ROOT/'tools').iterdir():
                if source.is_file():
                    dest=root/'tools'/source.name
                    if source.name=='sc64_unit_walk_words.py':shutil.copyfile(source,dest)
                    else:dest.symlink_to(source)
            (root/'bank').mkdir()
            facing=root/'bank/facing.h'; walk=root/'bank/walk.h'
            facing.write_text('static const unsigned char pixels[2] = {\n0x01,0x00,\n};\n')
            frame='{1,2,2,1,4,4,pixels}'
            walk.write_text('static const Sc64SpriteFrame pose[32] = {\n'+','.join([frame]*32)+'\n};\n'
                            'static const Sc64UnitWalkStep sc64_unit_walk_i1_steps[1] = {\n{0u,pose}\n};\n'+
                            ''.join('case %d: return &sc64_unit_walk_i1;\n'%n for n in (0,32,37,38,41,65)))
            (root/'fixture.mk').write_text('''check: build/walk-consumer
build/walk-consumer: $(CANONICAL_LOOKUP_INPUTS)
\t@printf 'rebuilt\\n' >> build/rebuilds
\t@touch $@
''')
            def run(mode):
                args=['make','-f','Makefile','-f','fixture.mk','CANONICAL_LOOKUP_INDEX=0',
                      'CANONICAL_WALK_WORDS='+str(mode),'UNIT_WALK_HEADER=bank/walk.h',
                      'UNIT_FACING_HEADER=bank/facing.h','check']
                subprocess.run(args,cwd=root,check=True,capture_output=True)
                return len((root/'build/rebuilds').read_text().splitlines())
            self.assertEqual(run(0),1);self.assertEqual(run(0),1)
            self.assertEqual(run(1),2);self.assertEqual(run(1),2)
            output=root/'.local/walk-words-managed/sc64_walk_words_generated.h'
            before=output.read_bytes();facing.write_text(facing.read_text().replace('0x01','0x02'))
            self.assertEqual(run(1),3);self.assertNotEqual(before,output.read_bytes())
            walk.write_text(walk.read_text()+'\n/* changed */\n')
            self.assertEqual(run(1),4)
            tool=root/'tools/sc64_unit_walk_words.py';tool.write_text(tool.read_text()+'\n')
            self.assertEqual(run(1),5);self.assertEqual(run(0),6);self.assertEqual(run(1),7)
            self.assertEqual(run(1),7)
            retained=output.read_bytes(); facing.write_text('bad input')
            with self.assertRaises(subprocess.CalledProcessError):run(1)
            self.assertEqual(output.read_bytes(),retained)
            facing.write_text('static const unsigned char pixels[2] = {\n0x02,0x00,\n};\n')
            output.write_text('unmanaged')
            with self.assertRaises(subprocess.CalledProcessError):run(1)
            self.assertEqual(output.read_text(),'unmanaged')

if __name__=='__main__':unittest.main()
