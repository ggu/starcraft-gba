# SPDX-License-Identifier: GPL-3.0-or-later
"""Pure physical-input timeline checks; no emulator or guest writes."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_p8_physical_replay as p8


class TimelineTests(unittest.TestCase):
    def test_pause_save_and_fresh_load_alignment(self):
        cut = 32443
        original = [(32439,8),(32440,0),(32441,2),(32442,0),
                    (32443,8),(32444,0),(32445,400)]
        events = p8.resume_events(original,cut)
        self.assertEqual(events, [(0,512),(1,0),(2,400)])
        self.assertEqual(p8.resume_boundary(32445,cut),2)
        # play_to(end) applies events at indices strictly below end. Loading
        # replaces START at index 0; release at index 1 advances the world.
        frame = 31676
        for index in range(p8.resume_boundary(32445,cut)):
            held = dict(events).get(index,0)
            if index == 0:
                self.assertEqual(held,512)
            else:
                self.assertEqual(held,0)
                frame += 1
        self.assertEqual(frame,31677)

    def test_invalid_boundary(self):
        with self.assertRaises(ValueError): p8.resume_boundary(32443,32443)


class RejectionTests(unittest.TestCase):
    def test_invalid_plan_is_rejected_before_backend(self):
        from unittest.mock import patch
        with patch.object(p8.tape, 'load_trace', return_value={'final':{'won':0,'lost':0,'screen':3}, 'save':None}):
            with self.assertRaisesRegex(ValueError,'ordinary save'):
                p8.validate_plan(Path('unused'))

    def test_bad_save_sequence_is_rejected(self):
        from unittest.mock import patch
        plan={'final':{'won':1,'lost':0,'screen':3},'save':{'first_call':10,'resume_call':16},'transitions':[(10,8),(11,0),(12,1),(13,0),(14,8),(15,0)]}
        with patch.object(p8.tape,'load_trace',return_value=plan):
            with self.assertRaisesRegex(ValueError,'button sequence'):
                p8.validate_plan(Path('unused'))

    def test_provenance_rejected_before_start(self):
        import tempfile, json
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp); files=[base/str(i) for i in range(6)]
            for f in files: f.write_bytes(b'x')
            files[5].write_text(json.dumps({'provenance':{},'save_sha256':'wrong'}))
            plan={'stop':20,'checkpoints':{},'transitions':[],'save':{}}
            with patch.object(p8,'validate_plan',return_value=(plan,10)), patch.object(p8,'symbols',return_value={}), patch.object(p8,'symbol_sizes',return_value={}), patch.object(p8.races,'dwarf_struct_layout',return_value=None), patch.object(p8.physical,'start') as start:
                with self.assertRaisesRegex(ValueError,'provenance'):
                    p8.run(*files[:4],base/'out',resume=files[4],summary=files[5])
                start.assert_not_called()

    def test_mission_fault_and_terminal_fail_closed(self):
        import tempfile, json
        from unittest.mock import patch
        class Process:
            stdin=stdout=None
            def terminate(self): pass
            def wait(self,timeout): pass
        class Replay:
            def play_to(self,*args): pass
        baseline={'screen':3,'frame':20,'won':1,'lost':0,'race_mode':0,'race_completed':1<<27,'race_selected_mission':27,
                  'runners':[{'fault':0,'outcome':1,'done':1<<4},{'fault':0},{'fault':0}],
                  'campaign':{'mission':27,'phase':3,'objective_flags':1},'stasis_cells':[]}
        restored={'screen':1,'race_selected_mission':27,'world':{},'p8_persistent_sha256':'same'}
        plan={'stop':20,'checkpoints':{},'transitions':[],'save':{}}
        for mutation in ('mission','fault','objective','trigger','stasis'):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as tmp:
                base=Path(tmp); files=[base/str(i) for i in range(6)]
                for f in files: f.write_bytes(b'x')
                files[5].write_text(json.dumps({'provenance':{str(f.resolve()):p8.digest(f) for f in files[:4]},'save_sha256':p8.digest(files[4]),'snapshot':{'frame':10,'world':{},'p8_persistent_sha256':'same'}}))
                import copy
                final=copy.deepcopy(baseline)
                if mutation=='mission': final['race_selected_mission']=26
                if mutation=='fault': final['runners'][1]['fault']=1
                if mutation=='objective': final['campaign']['objective_flags']=0
                if mutation=='trigger': final['runners'][0]['done']=0
                if mutation=='stasis': final['stasis_cells']=[1]
                with patch.object(p8,'validate_plan',return_value=(plan,10)), patch.object(p8,'symbols',return_value={}), patch.object(p8,'symbol_sizes',return_value={}), patch.object(p8.races,'dwarf_struct_layout',return_value=None), patch.object(p8.save,'validate_neutral_journal'), patch.object(p8.physical,'start',return_value=(Process(),Replay(),None)), patch.object(p8.normal,'verify_restored'), patch.object(p8,'snapshot',side_effect=[restored,final]):
                    with self.assertRaises(RuntimeError):
                        p8.run(*files[:4],base/'out',resume=files[4],summary=files[5])

if __name__ == '__main__': unittest.main()
