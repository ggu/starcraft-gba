# SPDX-License-Identifier: GPL-3.0-or-later
"""Release replay must use buttons without requiring or writing scheduler RAM."""
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_z7_arm_replay as z7
import sc64_z7_normal_replay as replay


class PhysicalDriverTest(unittest.TestCase):
    def exercise(self, stop=None):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            files={name:root/name for name in ('trace','rom','elf','runner')}
            for path in files.values(): path.write_bytes(b'fixture')
            final={key:0 for key in z7.SCALARS}
            final.update({key:[0]*8 for key in z7.LISTS})
            final.update(kind='won',host_tick=2,screen=3,won=1,outcome=1,
                         started=254,switch10=1)
            files['trace'].write_text(json.dumps({'schema':1,'profile':'zerg-z7',
                'mission':18,'events':[
                    {'kind':'input','host_tick':1,'held':1,'pressed':1},
                    {'kind':'input','host_tick':2,'held':0,'pressed':0},final]}))
            class Buttons:
                completed=0
                held=0
                def read(self,address,length):
                    return (18 if address==3 else 0).to_bytes(length,'little')
                def write(self,*args):
                    raise AssertionError('physical replay wrote guest memory')
                def play_page(self,page):
                    for tick,held in page['events']:
                        if tick<page['end']:self.held=held
                    self.completed=page['end']
            buttons=Buttons()
            proc=SimpleNamespace(stdin=None,stdout=None,terminate=lambda:None,
                                 wait=lambda timeout:None)
            start=lambda *args,**kwargs:(proc,buttons,None)
            with patch.dict(sys.modules,{'sc64_headless_physical':SimpleNamespace(start=start)}), \
                 patch.object(replay,'symbols',return_value={'screen':1,'frame':2,'race_selected_mission':3}), \
                 patch.object(replay,'symbol_sizes',return_value={}), \
                 patch.object(replay,'make_layout',return_value={}), \
                 patch.object(replay,'snapshot',return_value=final), \
                 patch.object(replay.scheduler,'schedule_mode',side_effect=AssertionError('release has no scheduler')), \
                 patch.object(replay.headless,'start',side_effect=AssertionError('wrong backend')):
                result=replay.run(**files,output=root/'out',stop=stop,physical=True)
            # This is a transport-routing mock, not emulator qualification.
            self.assertEqual(result['transport'],'physical-buttons')
            self.assertEqual(json.loads((root/'out/result.json').read_text()),result)
            return result,buttons

    def test_release_without_scheduler_uses_only_button_transport(self):
        result,buttons=self.exercise()
        self.assertEqual((result['status'],buttons.completed,buttons.held),('won',2,0))

    def test_prefix_does_not_apply_next_update_key(self):
        result,buttons=self.exercise(stop=1)
        self.assertEqual((result['status'],buttons.completed,buttons.held),('prefix',1,1))
        self.assertEqual(result['checkpoint_count'],0)

    def test_timing_requires_complete_matching_update_histogram(self):
        before={'updates':12,'cycles':100,'peak':1,'buckets':[0]*8}
        after={'updates':42,'cycles':16777316,'peak':800000,'buckets':[0,30,0,0,0,0,0,0]}
        result=replay.timing_delta(before,after,30)
        self.assertEqual(result['effective_main_loop_hz'],30)
        self.assertEqual(result['updates'],30)
        self.assertIn('not physical-device FPS',result['scope'])
        for field in ('updates','cycles','buckets'):
            bad=dict(after)
            bad[field]=([0]*8 if field=='buckets' else before[field])
            with self.subTest(field=field),self.assertRaises(RuntimeError):
                replay.timing_delta(before,bad,30)
        self.assertIsNone(replay.timing_delta(None,after,30))


if __name__=='__main__':unittest.main()
