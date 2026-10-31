# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_z7_normal_save as save
import sc64_z7_normal_replay as replay


class SaveTest(unittest.TestCase):
    def plan(self):
        return {'stop': 20, 'transitions': [(1, 1), (4, 0), (11, 4), (12, 0)],
                'observations': {10: {'screen': 1, 'won': 0, 'lost': 0, 'fault': 0}}}

    def test_cut_must_be_neutral_play_observation(self):
        plan = self.plan()
        save.validate_cut(plan, 10)
        for cut in (0, 4, 20, None):
            with self.assertRaises(ValueError): save.validate_cut(plan, cut)
        plan['transitions'].insert(2, (9, 2))
        with self.assertRaisesRegex(ValueError, 'neutral'): save.validate_cut(plan, 10)
        plan = self.plan(); plan['observations'][10]['screen'] = 2
        with self.assertRaisesRegex(ValueError, 'play'): save.validate_cut(plan, 10)

    def test_scheduler_save_sequence_and_boundary_transition(self):
        # Simulate the scheduler's held/pressed semantics and pause behavior.
        events = save.save_inputs(10) + [(14, 4), (15, 0)]
        held = previous = 0; screen = 1; actions = []
        for tick in range(10, 16):
            held = dict(events).get(tick, held); pressed = held & ~previous
            if screen == 1 and pressed & save.START: screen = 2; actions.append((tick, 'pause'))
            elif screen == 2 and pressed & save.B: actions.append((tick, 'save'))
            previous = held
        self.assertEqual(actions, [(10, 'pause'), (12, 'save')])
        self.assertEqual(save.resume_inputs(self.plan()['transitions'], 10),
                         [(0, 512), (1, 0), (2, 4), (3, 0)])

    def test_held_reestablish_and_immediate_override(self):
        self.assertEqual(save.resume_inputs([(1, 4), (15, 0)], 10),
                         [(0, 512), (1, 0), (2, 4), (6, 0)])
        self.assertEqual(save.resume_inputs([(1, 4), (11, 0)], 10),
                         [(0, 512), (1, 0), (2, 0)])

    def test_restored_and_tamper_fail_closed(self):
        world = {'frame': 10, 'screen': 2, 'fingerprint': 'a'*64}
        saved = {'cut': 10, 'mission': 18, 'world': world}
        restored = copy.deepcopy(saved); restored['world']['screen'] = 1
        save.verify_restored(saved, restored, 10, 18)
        for field, value in [('frame', 11), ('fingerprint', 'b'*64), ('screen', 2)]:
            changed = copy.deepcopy(restored); changed['world'][field] = value
            with self.subTest(field=field), self.assertRaises(RuntimeError):
                save.verify_restored(saved, changed, 10, 18)
        changed = copy.deepcopy(restored); changed['mission'] = 17
        with self.assertRaises(RuntimeError): save.verify_restored(saved, changed, 10, 18)
        with tempfile.TemporaryDirectory() as d:
            paths = [Path(d)/n for n in ('trace','rom','elf','runner','save','summary')]
            for path in paths: path.write_bytes(b'original')
            before = replay.input_hashes(paths)
            for path in paths:
                path.write_bytes(b'tampered')
                with self.subTest(path=path), self.assertRaises(RuntimeError): replay.require_unchanged(paths, before)
                path.write_bytes(b'original')


if __name__ == '__main__': unittest.main()
