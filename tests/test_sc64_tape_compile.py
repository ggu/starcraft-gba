# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of the parity tape compiler (tools/sc64_tape_compile.py)
and of the unit-class lab measurements (tools/sc64_parity_metrics.py), on
synthetic scenarios and series only: no ROM, capture or GBA build needed."""
import json
import pathlib
import re
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import sc64_tape_compile as tc  # noqa: E402
import sc64_parity_metrics as metrics  # noqa: E402

PAD = re.compile(r'^-?[0-9]+ [0-9]+ -?[0-9]+ -?[0-9]+$')
UNTIL = re.compile(r'^until [0-9A-F]{8} [124] (eq|ne|ge|lt|changed|set|clear) [0-9a-f]+ [0-9]+ [0-9]+ -?[0-9]+ -?[0-9]+'
                   r'( [0-9]+ [0-9]+)?$')
UNTILW = re.compile(r'^untilw [xy] (eq|ne|ge|lt) -?[0-9]+ [0-9]+ [0-9]+ -?[0-9]+ -?[0-9]+$')


def scenario(program, **extra):
    doc = {'format': tc.FORMAT, 'id': 'test', 'start': {'load': 'lab.state', 'step': 100},
           'camera': [-32, -32], 'map': [2048, 2048], 'program': program}
    doc.update(extra)
    return doc


def lines(doc):
    return [line for line in tc.compile_scenario(doc).split('\n') if line]


class SimulatedTape:
    """A tiny model of the frontend for one axis pair: cursor and camera move under the
    stick with lag and coarse pan steps; runs `untilw` lines the way capture.cpp does."""

    def __init__(self, camera=(-32, -32), cursor=(320, 240), world=(2048, 2048), lag=2):
        self.camera = list(camera)
        self.cursor = list(cursor)
        self.world = world
        self.lag = lag
        self.queue = [(0, 0, 0)] * lag
        self.steps = 0

    def step(self, mask, x, y):
        self.queue.append((mask, x, y))
        mask, x, y = self.queue.pop(0)
        self.steps += 1
        for axis, v in ((0, x), (1, y)):
            if not v:
                continue
            if mask & tc.BUTTONS['z']:
                pan = (40, 48)[axis] * (1 if abs(v) > 12000 else 0.5)
                limit = self.world[axis] - tc.SCREEN[axis] + 32
                self.camera[axis] = int(max(-32, min(limit, self.camera[axis] + (pan if v > 0 else -pan))))
            else:
                speed = 5.4 if abs(v) > 12000 else 1.9
                self.cursor[axis] = max(0, min(tc.SCREEN[axis] - 1, self.cursor[axis] + (speed if v > 0 else -speed)))

    def world_point(self, axis):
        return self.camera[axis] + int(self.cursor[axis])

    def run(self, text):
        for line in text.split('\n'):
            words = line.split()
            if not words or words[0].startswith('#') or words[0] in ('load', 'watch', 'tap', 'quit', 'save', 'shot'):
                continue
            if words[0] == 'untilw':
                axis, op, value, limit, mask, x, y = words[1:]
                axis = 0 if axis == 'x' else 1
                met = False
                for _ in range(int(limit)):
                    self.step(int(mask), int(x), int(y))
                    w = self.world_point(axis)
                    met = {'eq': w == int(value), 'ne': w != int(value), 'ge': w >= int(value),
                           'lt': w < int(value)}[op]
                    if met:
                        break
                if not met:
                    return False
            elif words[0] == 'until':
                address, width, op, value, limit, mask, x, y = words[1:9]
                address = int(address, 16)
                fields = {tc.CURSOR_X: lambda: int(self.cursor[0]), tc.CURSOR_Y: lambda: int(self.cursor[1])}
                if address not in fields:
                    self.step(int(mask), int(x), int(y))
                    continue
                for _ in range(int(limit)):
                    self.step(int(mask), int(x), int(y))
                    v = fields[address]()
                    if {'ge': v >= int(value, 16), 'lt': v < int(value, 16)}[op]:
                        break
            elif PAD.match(line):
                steps, mask, x, y = (int(w) for w in words)
                for _ in range(steps):
                    self.step(mask, x, y)
        return True


class CompileTest(unittest.TestCase):
    def test_every_line_is_a_frontend_command(self):
        doc = scenario([{'op': 'boxes', 'count': 2}, {'op': 'guard'}, {'op': 'tap'},
                        {'op': 'select', 'at': [310, 140], 'row': '80300210'},
                        {'op': 'order', 'kind': 'move', 'at': [520, 330], 'mark': 'command'},
                        {'op': 'wait', 'steps': 2500}, {'op': 'order', 'kind': 'attack', 'at': [600, 1200]},
                        {'op': 'order', 'kind': 'stop', 'mark': 'halt'}, {'op': 'save', 'name': 'x.state'}])
        out = lines(doc)
        self.assertEqual(out[0], '# sc64_tape_compile sc64-tape-scenario/1 scenario test')
        self.assertEqual(out[1], 'load lab.state 100')
        self.assertEqual(out[-1], 'quit')
        for line in out[1:-1]:
            ok = (line.startswith(('load ', 'watch ', '# mark ', 'save ')) or line == 'tap tick rec' or
                  PAD.match(line) or UNTIL.match(line) or UNTILW.match(line))
            self.assertTrue(ok, line)
        # A wait longer than 1000 steps is split into pad lines of at most 1000.
        self.assertIn('1000 0 0 0', out)
        self.assertIn('500 0 0 0', out)

    def test_marks_land_right_before_the_issuing_press(self):
        out = lines(scenario([{'op': 'select', 'at': [310, 140], 'row': '80300210'},
                              {'op': 'order', 'kind': 'move', 'at': [520, 330], 'mark': 'command'},
                              {'op': 'order', 'kind': 'hold', 'mark': 'hold'}]))
        i = out.index('# mark command')
        self.assertEqual(out[i + 1], f'{tc.PRESS_HOLD} {tc.BUTTONS["c_right"]} 0 0')
        # The guard (no box open) and the position checks come before the mark, never between it and the press.
        self.assertTrue(out[i - 1].startswith('until 800F0C20 2 eq 0 1 '))
        j = out.index('# mark hold')
        self.assertEqual(out[j + 1], f'{tc.PRESS_HOLD} {tc.BUTTONS["c_left"]} 0 0')
        attack = lines(scenario([{'op': 'order', 'kind': 'attack', 'at': [300, 300], 'mark': 'command'}]))
        k = attack.index('# mark command')
        self.assertEqual(attack[k + 1], f'{tc.PRESS_HOLD} {tc.BUTTONS["b"]} 0 0')

    def test_marks_resolve_through_tape_commands(self):
        text = tc.compile_scenario(scenario([{'op': 'wait', 'steps': 10},
                                             {'op': 'order', 'kind': 'stop', 'mark': 'command'}],
                                            start={'boot': 't1'}, camera=None))
        with tempfile.TemporaryDirectory() as temp:
            tape = pathlib.Path(temp) / 'tape.txt'
            tape.write_text(text)
            # Without commands.csv the until lines (the guard) make marks unresolvable.
            with self.assertRaises(SystemExit):
                metrics.tape_commands(tape)
            # With the executed log each step command, `until` and `untilw` included, is one row.
            rows = []
            step = 0
            for words in metrics.frontend_lines(text):
                if metrics.STEP_COUNT.fullmatch(words[0]):
                    rows.append((step, int(words[0])))
                    step += int(words[0])
                elif words[0] in ('until', 'untilw'):
                    rows.append((step, 3))
                    step += 3
            (pathlib.Path(temp) / 'commands.csv').write_text(
                'begin,steps,mask,x,y\n' + ''.join(f'{b},{n},0,0,0\n' for b, n in rows))
            marks = metrics.tape_commands(tape)
            boot = sum(n for n, _ in tc.BOOT_ROUTES['t1'])
            self.assertEqual(marks, {'command': boot + 10 + 3})

    def test_cursor_reaches_points_on_and_off_screen(self):
        for target in ([310, 140], [520, 330], [600, 1200], [1900, 1800], [330, 150], [1200, 300]):
            text = tc.compile_scenario(scenario([{'op': 'cursor', 'at': target}]))
            sim = SimulatedTape()
            self.assertTrue(sim.run(text), f'{target}: a position check missed')
            for axis in (0, 1):
                self.assertLessEqual(abs(sim.world_point(axis) - target[axis]), tc.REACH, target)

    def test_pan_never_waits_on_an_unreachable_point(self):
        # A target near the top-left edge pans from the far side.
        out = lines(scenario([{'op': 'cursor', 'at': [300, 1500]}, {'op': 'cursor', 'at': [330, 260]}]))
        pans = [line for line in out if line.startswith('untilw') and ' 1024 ' in line]
        self.assertTrue(pans)
        for line in pans:
            words = line.split()
            axis, op, value = words[1], words[2], int(words[3])
            lo = tc.CAMERA_MIN + tc.CENTRE[0 if axis == 'x' else 1]
            if op == 'lt':
                self.assertGreater(value, lo, line)

    def test_boot_routes(self):
        t1 = lines(scenario([], start={'boot': 't1'}, camera=None))
        self.assertNotIn('load', ' '.join(t1))
        bb = lines(scenario([], start={'boot': {'lobby': 'blood-bath', 'human': 'zerg', 'computer': 'terran'}},
                            camera=None))
        rights = [line for line in bb if line == f'10 {tc.BUTTONS["right"]} 0 0']
        # Scenario tab, 11 maps, 3 human races, 1 computer race.
        self.assertEqual(len(rights), 1 + 11 + 3 + 1)
        with self.assertRaises(tc.ScenarioError):
            lines(scenario([], start={'boot': {'lobby': 'blood-bath', 'human': 'elf', 'computer': 'terran'}}))

    def test_refusals(self):
        bad = [
            ({'op': 'order', 'kind': 'dance', 'at': [1, 1]}, 'unknown order'),
            ({'op': 'order', 'kind': 'stop', 'at': [1, 1]}, 'takes no target'),
            ({'op': 'select', 'at': [1, -5]}, 'outside'),
            ({'op': 'select', 'at': [1.5, 2]}, 'integer'),
            ({'op': 'save', 'name': '../x.state'}, 'NAME.state'),
            ({'op': 'save', 'name': '.state'}, 'NAME.state'),
            ({'op': 'mark', 'name': 'a b'}, 'bad mark'),
            ({'op': 'until', 'field': '800AFDCD:2', 'cmp': 'eq', 'value': 1}, 'bad field'),
            ({'op': 'until', 'field': 'pause', 'cmp': 'near', 'value': 1}, 'cmp'),
            ({'op': 'press', 'button': 'x'}, 'unknown button'),
            ({'op': 'wait', 'steps': 0}, 'outside'),
            ({'op': 'teleport'}, 'unknown op'),
        ]
        for step, message in bad:
            with self.assertRaisesRegex(tc.ScenarioError, message, msg=str(step)):
                lines(scenario([step]))
        more = [
            ({'op': 'until', 'field': 'pause', 'cmp': 'eq', 'value': 65536}, 'outside'),
            ({'op': 'until', 'field': 'pause', 'cmp': 'eq', 'value': 1, 'stick': []}, 'stick'),
            ({'op': 'hold', 'steps': 2, 'stick': [1]}, 'stick'),
            ({'op': 'wait', 'steps': 10, 'dismiss': True}, 'pulse'),
            ({'op': 'poke', 'address': '800D13C4', 'data': '0C'}, 'poke'),
            ({'op': 'select', 'at': [310, 140], 'row': '80300211'}, 'bad field'),
        ]
        for step, message in more:
            with self.assertRaisesRegex(tc.ScenarioError, message, msg=str(step)):
                lines(scenario([step]))
        with self.assertRaisesRegex(tc.ScenarioError, 'id must be'):
            lines(scenario([], id='a\nquit'))
        with self.assertRaisesRegex(tc.ScenarioError, 'poke'):
            lines(scenario([], start={'boot': {'episode': 'terran', 'mission': 3}}, camera=None))
        with self.assertRaisesRegex(tc.ScenarioError, 'frontend_commands'):
            lines(scenario([], frontend_commands=['teleport']))
        with self.assertRaisesRegex(tc.ScenarioError, 'no command after'):
            lines(scenario([{'op': 'mark', 'name': 'late'}]))
        with self.assertRaisesRegex(tc.ScenarioError, 'mark b follows mark a'):
            lines(scenario([{'op': 'mark', 'name': 'a'}, {'op': 'mark', 'name': 'b'}, {'op': 'wait', 'steps': 1}]))
        with self.assertRaisesRegex(tc.ScenarioError, 'starting camera'):
            lines(scenario([{'op': 'cursor', 'at': [5, 5]}], camera=None))
        with self.assertRaisesRegex(tc.ScenarioError, 'not a'):
            tc.compile_scenario({'format': 'other'})

    def test_declared_frontend_commands_and_exact_selection(self):
        out = lines(scenario([{'op': 'wait', 'steps': 10, 'dismiss': True},
                              {'op': 'select', 'at': [310, 140], 'row': '80300210'}],
                             frontend_commands=['pulse']))
        self.assertIn('pulse 800F0C20 2 ne 0 10 2 3 60', out)
        self.assertIn('until 800C0C00 4 eq 80300210 30 0 0 0', out)
        staged = lines(scenario([], start={'boot': {'episode': 'zerg', 'mission': 2}}, camera=None,
                                frontend_commands=['poke']))
        self.assertIn('poke 800D13C4 0C0A0A', staged)
        self.assertTrue(any(line.startswith('# n64-staged') for line in staged))

    def test_pan_leaves_a_safe_axis_alone(self):
        # Y is already inside the safe window: only X pans, and the point is reached.
        doc = scenario([{'op': 'cursor', 'at': [1000, 368]}], camera=[-32, 96], map=[2048, 736])
        text = tc.compile_scenario(doc)
        self.assertFalse(any(line.startswith('untilw y') and ' 1024 ' in line for line in text.split('\n')))
        sim = SimulatedTape(camera=(-32, 96), world=(2048, 736))
        self.assertTrue(sim.run(text))
        self.assertEqual(sim.camera[1], 96)
        # Near the top-left corner the pan runs into the edge, pinning the camera exactly.
        text = tc.compile_scenario(scenario([{'op': 'cursor', 'at': [900, 900]},
                                             {'op': 'box', 'from': [150, 100], 'to': [225, 182], 'count': 5}]))
        sim = SimulatedTape()
        self.assertTrue(sim.run(text))
        self.assertEqual(sim.camera, [-32, -32])

    def test_selection_must_be_named(self):
        with self.assertRaisesRegex(tc.ScenarioError, 'select needs "row"'):
            lines(scenario([{'op': 'select', 'at': [310, 140]}]))
        with self.assertRaisesRegex(tc.ScenarioError, 'box needs "count"'):
            lines(scenario([{'op': 'box', 'from': [150, 100], 'to': [225, 182]}]))
        # Scenarios written before the rule keep their tapes: something selected is enough.
        legacy = lines(scenario([{'op': 'select', 'at': [310, 140]}], legacy_selection=True))
        self.assertIn('until 800C0C00 4 ne 0 30 0 0 0', legacy)
        out = lines(scenario([{'op': 'box', 'from': [150, 100], 'to': [225, 182], 'count': 5}]))
        self.assertIn('until 800C0C10 4 ne 0 30 0 0 0', out)   # slot 4 holds the fifth unit
        self.assertIn('until 800C0C14 4 eq 0 1 0 0 0', out)    # and slot 5 is empty

    def test_mark_names_are_unique(self):
        with self.assertRaisesRegex(tc.ScenarioError, 'used twice'):
            lines(scenario([{'op': 'order', 'kind': 'stop', 'mark': 'command'},
                            {'op': 'order', 'kind': 'hold', 'mark': 'command'}]))
        with tempfile.TemporaryDirectory() as temp:
            tape = pathlib.Path(temp) / 'tape.txt'
            tape.write_text('# mark command\n5 0 0 0\n# mark command\n5 0 0 0\n')
            with self.assertRaises(SystemExit):
                metrics.tape_commands(tape)

    def test_cli_never_overwrites(self):
        with tempfile.TemporaryDirectory() as temp:
            src = pathlib.Path(temp) / 's.json'
            src.write_text(json.dumps(scenario([{'op': 'wait', 'steps': 5}])))
            out = pathlib.Path(temp) / 't.txt'
            self.assertEqual(tc.main([str(src), '-o', str(out)]), 0)
            text = out.read_text()
            self.assertIn('# scenario sha256 ', text)
            self.assertEqual(tc.main([str(src), '-o', str(out)]), 2)
            self.assertEqual(out.read_text(), text)


def walk(points_by_sf, uid=1, typ=metrics.MARINE, owner=1, hp=None, others=()):
    frames = []
    for sf in sorted(points_by_sf):
        x, y = points_by_sf[sf]
        units = {uid: (typ, owner, x, y)}
        units.update({o[0]: o[1:] for o in others})
        frame = {'sf': sf, 'step': sf * 3, 'units': units, 'banks': {}}
        if hp is not None:
            frame['hp'] = {k: v[sf] for k, v in hp.items() if sf in v}
            for k in list(frame['units']):
                if k not in frame['hp']:
                    del frame['units'][k]
        frames.append(frame)
    return {'source': 'n64', 'frames': frames}


class LabRunnerArgsTest(unittest.TestCase):
    def test_lab_arguments_reach_only_lab_runners(self):
        import sc64_parity
        golden = {'gba': {'steps': 400, 'at_step': 5, 'mission': 6, 'subject': [548, 1402],
                          'cmds': [[5, 0, 640, 1450], [259, 16, -90, 3]]}}
        _, cmd = sc64_parity.gba_command('lab-vulture-move', golden, pathlib.Path('x.trace'))
        self.assertEqual(cmd[cmd.index('--scenario') + 1], 'lab-mission')
        self.assertEqual(cmd[cmd.index('--mission') + 1], '6')
        self.assertEqual(cmd[cmd.index('--subject') + 1], '548,1402')
        self.assertEqual([cmd[i + 1] for i, w in enumerate(cmd) if w == '--cmd'], ['5,0,640,1450', '259,16,-90,3'])
        # A golden cannot hand lab arguments to another runner.
        _, cmd = sc64_parity.gba_command('bb-train', golden, pathlib.Path('x.trace'))
        self.assertNotIn('--mission', cmd)
        self.assertNotIn('--cmd', cmd)

    def test_gba_series_reads_hit_points_shields_and_building_rows(self):
        trace = '\n'.join([
            '#sc64-parity-trace 1', 'H schema=sc64-parity-trace/1',
            'S 1 0 1 1 1 0 0 0 -1 -1 0', '+U 3 7 1',
            'U 3 7 -1 1 -1 100 50 80 80 60 -1 0 100 50 -1 0 0 -1 0 0 0 0 0',
            'B 2 0 -1 1 -1 300 300 1500 1500 0 0 0 0 1 1 0 -1 -1 -1 -1 0 0',
            'S 2 1 2 2 1 0 0 0 -1 -1 0',
            'U 3 7 -1 1 -1 101 50 80 80 50 -1 0 100 50 -1 0 0 -1 0 0 0 0 0',
            'Z overflow=0 write_errors=0', ''])
        import sc64_parity
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 't.trace'
            path.write_text(trace)
            series = sc64_parity.gba_series(path)
        self.assertEqual([f['shield'][3] for f in series['frames']], [60.0, 50.0])
        self.assertEqual(series['frames'][0]['hp'][3], 80.0)
        self.assertEqual(series['frames'][1]['units'][3][2:], (202, 100))
        self.assertIn(('+U', 1, 3, 7, 1), series['events'])
        self.assertIn(('B', 1, 2, 0, 1, 0, 1, 1, 0), series['events'])


class LabMeasureTest(unittest.TestCase):
    def test_move_legs_turn_and_acceleration(self):
        pos, x = {}, 100.0
        speeds = [0, 1, 2, 3, 4] + [4] * 40
        for sf in range(0, 200):
            if 10 <= sf < 10 + len(speeds) and x < 260:
                x = min(260, x + speeds[sf - 10])
            if sf >= 100:
                k = sf - 104   # second leg: starts 4 frames later than the first (a turn)
                if k >= 0 and x > 100:
                    x = max(100, x - min(4, k + 1))
            pos[sf] = (round(x), 50)
        series = walk(pos)
        m, err = metrics.lab_move_measure(series, 1, [9, 99], [(260, 50), (100, 50)])
        self.assertIsNone(err)
        self.assertAlmostEqual(m['cruise'], 4.0, places=1)
        self.assertEqual(m['latency'], 2)
        self.assertEqual(m['turn_cost'], 3)
        self.assertGreaterEqual(m['accel'], 3)
        obs = {o['check']: o for o in metrics.lab_move_observations(m, 'Marine')}
        self.assertTrue(obs['lab.turn_cost']['gating'])
        self.assertFalse(obs['lab.start_latency']['gating'])

    def test_move_refuses_an_unsettled_leg(self):
        pos = {sf: (100 + 4 * sf, 50) for sf in range(0, 60)}
        m, err = metrics.lab_move_measure(walk(pos), 1, [0], [(400, 50)])
        self.assertIsNone(m)
        self.assertIn('still moving', err)

    def test_move_refuses_a_leg_that_stops_away_from_its_order(self):
        # A mirrored route: the unit moves 160 px the other way and stops there; it is no arrival.
        pos = {sf: (max(100 - 4 * max(0, sf - 10), -60) + 200, 50) for sf in range(0, 120)}
        m, err = metrics.lab_move_measure(walk(pos), 1, [9], [(460, 50)])
        self.assertIsNone(m)
        self.assertIn('from its order point', err)
        m, err = metrics.lab_move_measure(walk(pos), 1, [9], [(150, 60)])   # within one tile: fine
        self.assertIsNone(err)

    def test_gba_arrival_uses_the_issued_point(self):
        # A relative order past the map edge: the runner clamps it to x=8 (16 N64 px) and says so.
        pos = {sf: (max(16, 200 - 4 * max(0, sf - 6)), 100) for sf in range(0, 120)}
        golden = {'gba': {'cmds': [[5, 16, -112, 0]]}}
        report = {'subject': 1, 'subject_start': [100, 50], 'lab': [[5, 3, 16, -1, 8, 50]]}
        result, err = metrics.lab_move_gba({})(walk(pos), golden, report)
        self.assertIsNone(err)
        self.assertEqual(result['measured']['legs'][0]['target_miss'], 0.0)
        result, err = metrics.lab_move_gba({})(walk(pos), golden, {'subject': 1, 'lab': [[5, 3, 16, -1]]})
        self.assertIn('did not report where', err)

    def test_gba_steps_keep_offsets(self):
        steps = metrics.lab_gba_steps([40, 140, 151])
        self.assertEqual(steps[0], 5)
        base = (5 - 1) * 4 // 5
        for t, s in zip([40, 140, 151], steps):
            self.assertIn((s - 1) * 4 // 5 - base - (t - 40), (0, 1))
            self.assertTrue(metrics.gba_boundary_ok(s))
        # Every offset is reachable within one frame.
        for delta in range(1, 400):
            s = metrics.lab_gba_steps([0, delta])[1]
            self.assertIn((s - 1) * 4 // 5 - base - delta, (0, 1))

    def test_attack_hits_cooldown_range_and_death(self):
        pos = {sf: (100, 100) for sf in range(0, 120)}
        marine = {sf: 40.0 for sf in range(0, 120)}
        ling = {sf: 35.0 for sf in range(0, 120)}
        for k, sf in enumerate(range(20, 120, 15)):     # Marine hits every 15 frames for 6
            for later in range(sf, 120):
                ling[later] = max(0.0, 35.0 - 6 * (k + 1) + 0.0156 * (later - sf))   # Zerg regeneration
        for k, sf in enumerate(range(30, 120, 8)):      # Zergling hits for 5 every 8 frames
            for later in range(sf, 120):
                marine[later] = 40.0 - 5 * (k + 1)
        dead = next(sf for sf in sorted(marine) if marine[sf] <= 0)
        marine = {sf: v for sf, v in marine.items() if sf <= dead}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 228, 100)])
        m, err = metrics.lab_attack_measure(series, 1, 5)
        self.assertIsNone(err)
        self.assertEqual(m['damage_values'], [6])
        self.assertEqual(m['cooldown'], 15)
        self.assertEqual(m['range_first'], 128.0)
        self.assertEqual(m['counter_values'], [5])
        self.assertEqual(m['survival'], dead - 30)
        self.assertEqual(m['first_hit'], 15)

    def test_attack_counts_only_the_named_foe(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35, 50) if t <= sf]) for sf in range(0, 80)}
        bunker = {sf: 350.0 - 6 * len([t for t in (25, 40) if t <= sf]) for sf in range(0, 80)}   # a building row
        drone = {sf: 40.0 - 6 * len([t for t in (30,) if t <= sf]) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling, 3: bunker, 4: drone},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (3, 125, 4, 140, 100),
                              (4, metrics.DRONE, 4, 160, 100)])
        m, err = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertIsNone(err)
        self.assertEqual(m['hits'], 3)
        self.assertEqual(m['gaps'], [15, 15])
        self.assertEqual(list(m['victims']), ['2'])
        # Without the foe filter the building and the Drone would pass as hits on the foe.
        m, _ = metrics.lab_attack_measure(series, 1, 5)
        self.assertEqual(m['hits'], 6)

    def test_hits_need_a_single_attacker_in_reach(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35, 50) if t <= sf]) for sf in range(0, 80)}
        friend = {sf: 40.0 for sf in range(0, 80)}
        # A second human Marine 100 px from the Zergling: every hit is ambiguous, none is credited.
        series = walk(pos, hp={1: marine, 2: ling, 5: friend},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (5, metrics.MARINE, 1, 250, 100)])
        m, err = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual((m['hits'], m['ambiguous_hits']), (0, 3))
        # Outside reach it is no rival.
        series = walk(pos, hp={1: marine, 2: ling, 5: friend},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (5, metrics.MARINE, 1, 150 + 400, 100)])
        m, err = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual((m['hits'], m['ambiguous_hits']), (3, 0))

    def test_gap_and_ambiguous_hits_never_set_timing(self):
        frames = [sf for sf in range(0, 80) if sf != 34]   # frame 34 was never sampled
        pos = {sf: (100, 100) for sf in frames}
        marine = {sf: 40.0 for sf in frames}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35, 50) if t <= sf]) for sf in frames}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        # The hit first seen at 35 follows a gap: ambiguous, and no interval spans it.
        self.assertEqual((m['hits'], m['ambiguous_hits'], m['gaps']), (2, 1, []))

    def test_kills_split_cooldown_intervals(self):
        pos = {sf: (100, 100) for sf in range(0, 90)}
        marine = {sf: 40.0 for sf in range(0, 90)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 50, 65) if t <= sf]) for sf in range(0, 90)}
        other = {sf: (5.0 if sf < 35 else 0.0) for sf in range(0, 90)}   # a second Zergling killed at 35
        series = walk(pos, hp={1: marine, 2: ling, 3: other},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (3, metrics.ZERGLING, 4, 160, 100)])
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual(m['gaps'], [15])   # 20 -> 50 spans the kill at 35; 50 -> 65 stands

    def test_enemy_buildings_count_for_damage_taken(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 - 5 * len([t for t in (20, 30) if t <= sf]) for sf in range(0, 80)}
        ling = {sf: 35.0 for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 130, 100)])
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertTrue(m['taken_single'])
        for frame in series['frames']:   # a GBA enemy building (e.g. a Sunken Colony) in reach
            frame['buildings'] = {7: (4, 200, 100)}
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertFalse(m['taken_single'])

    def test_n64_order_must_match_the_replay(self):
        class Cap:
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}}
                     for s, t, o in ((100, 50, 3), (103, 51, 3), (106, 52, 14), (109, 53, 14))]

            @staticmethod
            def unit(row, name):
                return row
        self.assertEqual(metrics.n64_order_after(Cap, 9, 101, metrics.ORDER_ATTACK_MOVE)[0], 14)
        self.assertEqual(metrics.n64_order_after(Cap, 9, 101, (metrics.ORDER_MOVE, metrics.ORDER_ATTACK_MOVE))[0], 14)
        ok, seen = metrics.n64_order_after(Cap, 9, 101, metrics.ORDER_MOVE)
        self.assertIsNone(ok)
        self.assertEqual(seen, [3, 14])

        class Already:   # Move before the press and after it: no evidence the press did anything
            ticks = [{'step': s, 'tick': t, 'rows': {9: 6}} for s, t in ((100, 50), (103, 51), (106, 52))]
            unit = Cap.unit
        self.assertIsNone(metrics.n64_order_after(Already, 9, 101, metrics.ORDER_MOVE)[0])

        class Late:   # the change comes 20 ticks after the press: outside the window
            ticks = [{'step': 100, 'tick': 50, 'rows': {9: 3}}, {'step': 140, 'tick': 70, 'rows': {9: 6}}]
            unit = Cap.unit
        self.assertIsNone(metrics.n64_order_after(Late, 9, 101, metrics.ORDER_MOVE)[0])

    def test_exact_damage_needs_every_hit(self):
        golden = {'observations': [{'check': 'lab.damage', 'value': 6, 'lo': 6, 'hi': 6, 'gating': True},
                                   {'check': 'lab.range_first', 'value': 50, 'lo': 42, 'hi': 58, 'gating': True}]}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        drops = {20: 4, 35: 6, 50: 8}   # median 6, but two hits are wrong
        ling = {sf: 35.0 - sum(d for t, d in drops.items() if t <= sf) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        spec = {'foe_type': metrics.ZERGLING}
        result, err = metrics.lab_attack_gba(spec)(series, golden, {'subject': 1, 'lab': [[5, 4, 1, -1]]})
        self.assertIsNone(err)
        self.assertEqual(result['measured']['damage'], 6)
        self.assertIn(result['values']['lab.damage'], (4, 8))   # the worst hit, never the median

    def test_cancelled_tail_keeps_the_head_start(self):
        # Two SCVs queued at step 10; the second is cancelled at step 100 (count 2 -> 1, same head,
        # progress still rising); the first completes at 384.
        ev = [('B', 10, 1, 0, 3, 0, 2, 1, 0), ('B', 100, 1, 0, 3, 0, 1, 91, 0),
              ('B', 384, 1, 0, 3, 0, 0, 0, -1), ('+U', 384, 7, 0, 3)]
        table = metrics.train_gba({'events': ev}, 0, [7], [106])
        self.assertEqual([d for _, d, _ in table[7]], [(384 - 10 + 1) * 4 / 5])

    def test_training_births_go_to_the_producer_that_completed(self):
        # Two Command Centers (slots 1, 2) of owner 3 train SCVs (engine 0); the newer one finishes first.
        ev = [('B', 10, 1, 0, 3, 0, 1, 1, 0), ('B', 20, 2, 0, 3, 0, 1, 1, 0),
              ('B', 300, 2, 0, 3, 0, 0, 0, -1), ('+U', 300, 7, 0, 3),
              ('B', 400, 1, 0, 3, 0, 0, 0, -1), ('+U', 400, 8, 0, 3)]
        table = metrics.train_gba({'events': ev}, 0, [7], [106])
        self.assertEqual(sorted(d for _, d, _ in table[7]), [(300 - 20 + 1) * 4 / 5, (400 - 10 + 1) * 4 / 5])
        # Two producers completing the same type in one step: the birth is ambiguous, not guessed.
        ev = [('B', 10, 1, 0, 3, 0, 1, 1, 0), ('B', 20, 2, 0, 3, 0, 1, 1, 0),
              ('B', 300, 1, 0, 3, 0, 0, 0, -1), ('B', 300, 2, 0, 3, 0, 0, 0, -1), ('+U', 300, 7, 0, 3)]
        ambiguous = []
        table = metrics.train_gba({'events': ev}, 0, [7], [106], ambiguous)
        self.assertEqual(table, {})
        self.assertEqual(len(ambiguous), 1)

    def test_clipped_shield_drop_and_one_shot_death(self):
        pos = {sf: (100, 100) for sf in range(0, 60)}
        zealot_shield = {sf: 80.0 if sf < 20 else 70.0 if sf < 35 else 0.0 for sf in range(0, 60)}  # 70 -> 0 is clipped
        zealot_hp = {sf: 80.0 for sf in range(0, 60)}
        marine = {sf: 40.0 for sf in range(0, 50)}   # removed at 50 with no visible hit (GBA one-shot)
        series = walk(pos, hp={1: marine, 2: zealot_hp}, others=[(2, 65, 4, 160, 100)])
        for frame in series['frames']:
            frame['shield'] = {2: zealot_shield[frame['sf']]}
        m, err = metrics.lab_attack_measure(series, 1, 5)
        self.assertIsNone(err)
        self.assertEqual(m['shield_values'], [10])     # the 70 -> 0 drop shows only what was left
        self.assertEqual(m['shield_hits'], 2)          # but still counts for timing
        self.assertEqual(m['survival'], 0)             # killed by a hit the trace never shows


if __name__ == '__main__':
    unittest.main()
