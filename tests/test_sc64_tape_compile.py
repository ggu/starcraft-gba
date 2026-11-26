# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of the parity tape compiler (tools/sc64_tape_compile.py)
and of the unit-class lab measurements (tools/sc64_parity_metrics.py), on
synthetic scenarios and series only: no ROM, capture or GBA build needed."""
import json
import math
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
        # Nothing selected before the drag, so a failed drag never leaves a stale group.
        empty = out.index('until 800C0C00 4 eq 0 1 0 0 0')
        self.assertEqual(out[empty + 1], f'{tc.PRESS_HOLD} {tc.BUTTONS["a"]} 0 0')   # right before A goes down
        legacy = lines(scenario([{'op': 'box', 'from': [150, 100], 'to': [225, 182]}], legacy_selection=True))
        self.assertNotIn('until 800C0C00 4 eq 0 1 0 0 0', legacy)

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


def as_gba(series, removals=()):
    """A walk() series as a GBA trace gives it: each source frame's last engine step (5 steps to
    4 frames) and the trace's removal records."""
    series['source'], series['removals'] = 'gba', list(removals)
    for frame in series['frames']:
        frame['step'] = (5 * frame['sf'] + 4) // 4
    return series


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
        # owner, x, y, hp, shields, max hp, engine type, cartridge type, under construction, live owner
        self.assertEqual(series['frames'][0]['buildings'][2], (1, 600, 600, 1500, 0, 1500, 0, -1, 0, 1))
        self.assertEqual(series['frames'][0]['live'], {3: 1})

    def test_gba_series_reads_weapon_hits_and_capacities(self):
        trace = '\n'.join([
            '#sc64-parity-trace 1', 'H schema=sc64-parity-trace/1',
            'H max_units=173 max_buildings=64 max_minerals=8 owners=5 cell_cap=4',
            'S 1 0 1 1 1 0 0 0 -1 -1 0', 'D 180 10 3 1',
            'Z overflow=0 write_errors=0', ''])
        import sc64_parity
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 't.trace'
            path.write_text(trace)
            series = sc64_parity.gba_series(path)
        # step, target (max_units + building slot 7), damage, source engine type, source owner
        self.assertEqual((series['max_units'], series['hits']), (173, [(1, 180, 10, 3, 1)]))

    def test_gba_series_keeps_each_removal_with_its_last_row(self):
        # A unit and a building removed one step after their rows: kind, step, slot, owner, x, y,
        # hp at removal, max hp, last row's hp, cartridge type, engine type, last row's step.
        trace = '\n'.join([
            '#sc64-parity-trace 1', 'H schema=sc64-parity-trace/1',
            'S 1 0 1 1 1 0 0 0 -1 -1 0', '+U 3 7 1',
            'U 3 7 -1 1 -1 100 50 80 80 60 -1 0 100 50 -1 0 0 -1 0 0 0 0 0',
            'B 2 0 -1 1 -1 300 300 1500 1500 0 0 0 0 1 1 0 -1 -1 -1 -1 0 0',
            'S 2 1 2 2 1 0 0 0 -1 -1 0', '-U 3 7 1 -3', '-B 2 0 1 0',
            'Z overflow=0 write_errors=0', ''])
        import sc64_parity
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 't.trace'
            path.write_text(trace)
            series = sc64_parity.gba_series(path)
        self.assertEqual(series['removals'], [('U', 2, 3, 1, 200, 100, -3, None, 80.0, -1, 7, None, None),
                                              ('B', 2, 2, 1, 600, 600, 0, 1500, 1500, -1, 0, 1, 0)])


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

    def test_a_settled_short_return_leg_cannot_zero_the_cruise_band(self):
        # The first leg cruises at 4 px/frame; the return leg moves two frames and settles. It
        # reads cruise 0 (no whole chord fits), so it must not set the band's lower bound to 0,
        # which would let an arbitrarily slow GBA pass lab.cruise_speed.
        pos, x = {}, 100.0
        for sf in range(0, 200):
            if 10 <= sf < 50:
                x = min(260, x + 4)
            if sf in (104, 105):
                x -= 2
            pos[sf] = (round(x), 50)
        m, err = metrics.lab_move_measure(walk(pos), 1, [9, 99], [(260, 50), (256, 50)])
        self.assertIsNone(err)
        self.assertEqual(m['legs'][1]['cruise'], 0.0)
        self.assertEqual(m['cruise_left_out'], [1])
        self.assertAlmostEqual(m['cruise_min'], m['cruise'])
        self.assertGreater(m['cruise_min'], 3.5)
        obs = {o['check']: o for o in metrics.lab_move_observations(m, 'Marine')}
        self.assertGreater(obs['lab.cruise_speed']['lo'], 3.0)
        self.assertIn('leg 1 left out', obs['lab.cruise_speed']['tolerance'])
        # A first leg that never holds a cruise is a measurement error, not a zero band.
        still = {sf: (100 + (2 if sf >= 12 else 0) + (2 if sf >= 13 else 0), 50) for sf in range(0, 100)}
        m, err = metrics.lab_move_measure(walk(still), 1, [9], [(104, 50)])
        self.assertIsNone(m)
        self.assertIn('leg 0', err)

    @staticmethod
    def ramp_leg(speed, ramp, start=10, length=240, grid=None, phase=0.0, diagonal=True):
        """One diagonal leg: still until start, then the speed rises linearly over ramp frames to
        cruise; grid: positions held on a GBA-like grid of that many N64 px per axis (a diagonal
        step is then 2.83 N64 px, as an Overlord's), with a sub-step phase."""
        pos, d = {}, 0.0
        for sf in range(0, start + length + 40):
            k = sf - start
            if 0 <= k < length:
                d += speed * min(1.0, (k + 1) / ramp) if ramp else speed
            a = d / math.sqrt(2) if diagonal else d
            shown = a if grid is None else grid * math.floor((a + phase) / grid)
            pos[sf] = (round(100 + shown), round(50 + (shown if diagonal else 0)))
        return pos

    def accel(self, pos, chord=None, floor_px=None):
        end = pos[max(pos)]
        m, err = metrics.lab_move_measure(walk(pos), 1, [9], [end], chord, floor_px)
        self.assertIsNone(err)
        return m['accel'], m['accel_chord']

    def reference(self, speed, ramp, diagonal=True):
        """The N64 side: reading, chord and floor, as the golden records them."""
        pos = self.ramp_leg(speed, ramp, diagonal=diagonal)
        m, err = metrics.lab_move_measure(walk(pos), 1, [9], [pos[max(pos)]])
        self.assertIsNone(err)
        return m['accel'], m['accel_chord'], m['accel_floor']

    def test_slow_grid_mover_reads_the_same_at_every_start_phase(self):
        # An Overlord-like mover (0.84 N64 px a frame, no acceleration) on the GBA's 2-px grid: its
        # reading must not depend on where the first grid step falls. A 4-frame chord holds one
        # grid step or two by phase alone; the chord now spans two steps at cruise.
        n64, chord = self.accel(self.ramp_leg(0.84, 0))
        self.assertEqual(chord, metrics.lab_accel_chord(0.84))
        self.assertGreaterEqual(chord * 0.84, metrics.LAB_ACCEL_GRID_STEPS * metrics.LAB_GBA_GRID)
        readings = {self.accel(self.ramp_leg(0.84, 0, grid=2, phase=p / 4), chord)[0] for p in range(8)}
        self.assertTrue(all(abs(r - n64) <= metrics.MOVE_TIME_TOLERANCE for r in readings), (n64, readings))
        short = {self.accel(self.ramp_leg(0.84, 0, grid=2, phase=p / 4), metrics.LAB_ACCEL_CHORD)[0]
                 for p in range(8)}
        self.assertGreater(max(short) - min(short), metrics.MOVE_TIME_TOLERANCE)   # why the chord grew
        # A fast mover keeps the shortest chord: its readings are unchanged.
        self.assertEqual(metrics.lab_accel_chord(4.0), metrics.LAB_ACCEL_CHORD)

    def test_constant_speed_reads_the_same_on_the_grid_at_any_speed(self):
        # Straight constant-speed moves, axial and diagonal, slow and fast: the grid reading
        # stays within the band of the N64's at every phase (the cutoff allows the whole-step
        # rounding: an axial 0.90 px/frame mover covers 4 px in its 5-frame chord).
        for speed in (0.5, 0.84, 0.9, 1.3, 2.0, 4.0, 6.5):
            for diagonal in (False, True):
                n64, chord, floor_px = self.reference(speed, 0, diagonal)
                for p in range(8):
                    gba, _ = self.accel(self.ramp_leg(speed, 0, grid=2, phase=p / 4, diagonal=diagonal),
                                        chord, floor_px)
                    self.assertLessEqual(abs(gba - n64), metrics.MOVE_TIME_TOLERANCE,
                                         (speed, diagonal, p, n64, gba, chord))

    def test_a_mover_that_takes_twice_as_long_to_cruise_still_fails(self):
        # The longer chord must not blind the check: a GBA track reaching cruise in twice the
        # N64's frames reads outside the +-2 frame band at every grid phase, for a fast mover
        # and a slow one. (A slow mover's reading carries about one grid step of noise once it
        # accelerates, so its ramp here is long enough to be resolved: 20 frames, then 40.)
        for speed, ramp in ((4.0, 6), (0.84, 20)):
            n64, chord, floor_px = self.reference(speed, ramp)
            for p in range(8):
                gba, _ = self.accel(self.ramp_leg(speed, 2 * ramp, grid=2, phase=p / 4), chord, floor_px)
                self.assertGreater(gba - n64, metrics.MOVE_TIME_TOLERANCE, (speed, p, n64, gba, chord))

    def test_a_slower_gba_cruise_cannot_lower_the_cutoff(self):
        # 0.84 px/frame: the N64 reaches cruise in 6 frames, the GBA in 24, and the GBA's own
        # cruise estimate comes out a little low (0.80). The floor is the N64's, so the slower
        # ramp fails at every phase; with the GBA's own floor it passed at one phase. (A slow
        # mover resolves about one grid step, 3-4 frames: 6 against 12 frames is not resolved.)
        n64, chord, floor_px = self.reference(0.84, 6)
        for p in range(8):
            pos = self.ramp_leg(0.80, 24, grid=2, phase=p / 4)
            gba, _ = self.accel(pos, chord, floor_px)
            self.assertGreater(gba - n64, metrics.MOVE_TIME_TOLERANCE, (p, n64, gba, chord, floor_px))

    def test_chord_covers_two_steps_after_rounding(self):
        for cruise in (4 / 161, 4 / 7, 0.5, 0.84, 0.9, 1.0, 1.3):
            chord = metrics.lab_accel_chord(cruise)
            self.assertGreaterEqual(metrics.lab_accel_floor(cruise, chord),
                                    metrics.LAB_ACCEL_GRID_STEPS * metrics.LAB_GBA_GRID, cruise)

    def test_gba_side_uses_the_goldens_chord(self):
        pos = self.ramp_leg(0.84, 0, grid=2)
        report = {'subject': 1, 'lab': [[5, 9, 16, -1, pos[max(pos)][0] // 2, pos[max(pos)][1] // 2]]}
        golden = {'gba': {'cmds': [[5, 0, 0, 0]], 'accel_chord': 5, 'accel_floor': 4.0}, 'decoder_revision': 2}
        result, err = metrics.lab_move_gba({})(walk(pos), golden, report)
        self.assertIsNone(err)
        self.assertEqual(result['measured']['accel_chord'], 5)
        legacy = {'gba': {'cmds': [[5, 0, 0, 0]]}}   # revision 1: the shortest chord
        result, err = metrics.lab_move_gba({})(walk(pos), legacy, report)
        self.assertEqual(result['measured']['accel_chord'], metrics.LAB_ACCEL_CHORD)
        for bad in (None, 0, 2, 4.5, '5', 10 ** 400):
            golden['gba']['accel_chord'] = bad
            result, err = metrics.lab_move_gba({})(walk(pos), golden, report)
            self.assertIn('acceleration chord', err)
        golden['gba']['accel_chord'] = 5
        for bad in (None, 0, 2.0, 3.0, float('inf'), float('nan'), '4', 10 ** 400):
            golden['gba']['accel_floor'] = bad
            result, err = metrics.lab_move_gba({})(walk(pos), golden, report)
            self.assertIn('acceleration floor', err, bad)
        # Revision 1 falls back only when both keys are absent, not when one is null.
        for gba in ({'accel_chord': None}, {'accel_floor': None}, {'accel_chord': None, 'accel_floor': None}):
            result, err = metrics.lab_move_gba({})(walk(pos), {'gba': dict(gba, cmds=[[5, 0, 0, 0]])}, report)
            self.assertIn('acceleration', err, gba)
        for revision in (None, '2', 0, -1):
            result, err = metrics.lab_move_gba({})(walk(pos), {'gba': {'cmds': [[5, 0, 0, 0]]},
                                                               'decoder_revision': revision}, report)
            self.assertIn('decoder revision', err)

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
        # The building and the Drone took hits between the Zergling's, within reach of the
        # subject: it may have fired at them, so no cooldown interval spans them.
        self.assertEqual(m['gaps'], [])
        self.assertEqual(list(m['victims']), ['2'])
        far = walk(pos, hp={1: marine, 2: ling, 3: bunker, 4: drone},
                   others=[(2, metrics.ZERGLING, 4, 150, 100), (3, 125, 4, 900, 100),
                           (4, metrics.DRONE, 4, 900, 140)])
        self.assertEqual(metrics.lab_attack_measure(far, 1, 5, metrics.ZERGLING)[0]['gaps'], [15, 15])
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

    def test_a_second_attacker_on_the_sample_before_the_hit(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35, 50) if t <= sf]) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        # A second Marine stands in reach on frame 34 only: it may have fired the hit seen at 35.
        series['frames'][34]['units'][5] = (metrics.MARINE, 1, 200, 100)
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual((m['hits'], m['ambiguous_hits'], m['gaps']), (2, 1, []))
        # A GBA building of the human in reach of the victim is a possible attacker too.
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        for frame in series['frames']:
            frame['buildings'] = {7: (1, 220, 100)}
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual((m['hits'], m['ambiguous_hits']), (0, 3))

    def test_inconclusive_gba_combat_is_not_absent(self):
        golden = {'observations': [{'check': c, 'value': 6, 'lo': 6, 'hi': 6, 'gating': True}
                                   for c in ('lab.damage', 'lab.cooldown', 'lab.counter_damage')]}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 - 5 * len([t for t in (25, 40) if t <= sf]) for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35, 50) if t <= sf]) for sf in range(0, 80)}
        ling2 = {sf: 35.0 for sf in range(0, 80)}
        # A second Marine in reach of the Zergling, and a second Zergling in reach of the subject.
        series = walk(pos, hp={1: marine, 2: ling, 3: ling2, 5: {sf: 40.0 for sf in range(0, 80)}},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (3, metrics.ZERGLING, 4, 160, 120),
                              (5, metrics.MARINE, 1, 250, 100)])
        result, err = metrics.lab_attack_gba({'foe_type': metrics.ZERGLING})(series, golden, {'subject': 1, 'lab': [[5, 4, 1, -1]]})
        self.assertIsNone(err)
        # The fight happened: no ABSENT a no-fight baseline would accept, no value at all instead.
        self.assertEqual(result['measured']['inconclusive'], ['lab.cooldown', 'lab.counter_damage', 'lab.damage'])
        self.assertEqual(result['measured']['absent'], [])
        for check in ('lab.damage', 'lab.cooldown', 'lab.counter_damage'):
            self.assertNotIn(check, result['values'])

    def test_an_attack_golden_without_a_gating_check_fails(self):
        import sc64_parity
        golden = {'observations': [metrics.observation('lab.first_hit', 'Marine on Zergling', 40, 38, 42, 'report',
                                                       False, [], provenance='n64-ram', n_runs=2)], 'decoder_revision': 2}
        rows = sc64_parity.judge('lab-marine-zergling', golden, {'lab.first_hit': 40}, {}, {})
        self.assertEqual([r['verdict'] for r in rows], ['REGRESSION'])
        golden['observations'].append(metrics.observation('lab.damage', 'Marine on Zergling', 6, 6, 6, 'exact', True, [],
                                                          provenance='n64-ram', n_runs=2))
        rows = sc64_parity.judge('lab-marine-zergling', golden, {'lab.first_hit': 40, 'lab.damage': 6}, {}, {})
        self.assertEqual(sorted(r['verdict'] for r in rows), ['PASS', 'REPORT'])

    def test_a_fight_without_a_usable_value_is_judged_afresh(self):
        import sc64_parity
        obs = [metrics.observation('lab.cooldown', 'Marine', 15, 14, 16, '+-1', True, [], provenance='n64-ram', n_runs=2),
               metrics.observation('lab.survival', 'Marine', 50, 46, 54, '+-4', True, [], provenance='n64-ram', n_runs=2)]
        golden = {'observations': obs}
        baseline = {f'lab-x:{o["check"]}:{o["subject"]}': {'gba': metrics.ABSENT} for o in obs}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35) if t <= sf]) for sf in range(0, 80)}
        drone = {sf: 40.0 - 6 * len([t for t in (28,) if t <= sf]) for sf in range(0, 80)}
        report = {'subject': 1, 'lab': [[5, 4, 1, -1]]}
        spec = {'foe_type': metrics.ZERGLING}
        # Two clean hits split by a nearby Drone's drop: no interval, no ambiguous hit, a fight.
        fought = walk(pos, hp={1: marine, 2: ling, 4: drone},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (4, metrics.DRONE, 4, 160, 100)])
        result, _ = metrics.lab_attack_gba(spec)(fought, golden, report)
        self.assertEqual(result['measured']['ambiguous_hits'], 0)
        self.assertEqual(result['measured']['inconclusive'], ['lab.cooldown'])
        verdicts = {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, baseline)}
        self.assertEqual(verdicts['lab.cooldown'], 'REGRESSION')   # never the no-fight KNOWN
        self.assertEqual(verdicts['lab.survival'], 'KNOWN')        # it did not die: a real absence
        # The control: no fight at all keeps the no-fight baseline.
        calm = walk(pos, hp={1: marine, 2: {sf: 35.0 for sf in range(0, 80)}},
                    others=[(2, metrics.ZERGLING, 4, 600, 100)])
        result, _ = metrics.lab_attack_gba(spec)(calm, golden, report)
        verdicts = {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, baseline)}
        self.assertEqual(set(verdicts.values()), {'KNOWN'})

    def test_damage_off_the_foe_near_the_subject_is_a_fight(self):
        import sc64_parity
        obs = [metrics.observation(c, 'Marine', v, v - 1, v + 1, '+-1', True, [], provenance='n64-ram', n_runs=2)
               for c, v in (('lab.cooldown', 15), ('lab.damage', 6), ('lab.survival', 50))]
        golden = {'observations': obs}
        baseline = {f'lab-x:{o["check"]}:{o["subject"]}': {'gba': metrics.ABSENT} for o in obs}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        calm_ling = {sf: 35.0 for sf in range(0, 80)}
        report = {'subject': 1, 'lab': [[5, 4, 1, -1]]}
        spec = {'foe_type': metrics.ZERGLING}

        def verdicts(series):
            result, _ = metrics.lab_attack_gba(spec)(series, golden, report)
            return {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, baseline)}
        fought = {'lab.cooldown': 'REGRESSION', 'lab.damage': 'REGRESSION', 'lab.survival': 'KNOWN'}
        calm = {'lab.cooldown': 'KNOWN', 'lab.damage': 'KNOWN', 'lab.survival': 'KNOWN'}

        def with_drone(drone, x=160, removals=()):
            return as_gba(walk(pos, hp={1: marine, 2: calm_ling, 4: drone},
                               others=[(2, metrics.ZERGLING, 4, 900, 100), (4, metrics.DRONE, 4, x, 100)]), removals)
        # The Zergling is far and untouched and the Marine takes nothing, but in its reach a Drone
        # is wounded, killed (on the N64 its row keeps 0 hit points), or removed with 0 on the GBA.
        self.assertEqual(verdicts(with_drone({sf: 40.0 - 6 * (sf >= 28) for sf in range(0, 80)})), fought)
        self.assertEqual(verdicts(with_drone({sf: 40.0 if sf < 28 else 0.0 for sf in range(0, 80)})), fought)
        gone = {sf: 40.0 for sf in range(0, 28)}
        self.assertEqual(verdicts(with_drone(gone, removals=[('U', 35, 4, 4, 160, 100, -2, None, 40)])), fought)
        # The checks read the OFF_TARGET marker: a baseline holding it accepts the run, one
        # holding no fight or a measurement does not, and a fight with the foe never reads it.
        wounded = with_drone({sf: 40.0 - 6 * (sf >= 28) for sf in range(0, 80)})
        result, _ = metrics.lab_attack_gba(spec)(wounded, golden, report)
        self.assertEqual(result['measured']['off_target'], ['lab.cooldown', 'lab.damage'])
        self.assertEqual((result['values']['lab.damage'], result['values']['lab.survival']),
                         (metrics.OFF_TARGET, metrics.ABSENT))
        marked = dict(baseline)
        for check in ('lab.cooldown', 'lab.damage'):
            marked[f'lab-x:{check}:Marine'] = {'gba': metrics.OFF_TARGET}
        rows = {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, marked)}
        self.assertEqual(set(rows.values()), {'KNOWN'})
        measured = {k: {'gba': 9} for k in marked}
        rows = {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, measured)}
        self.assertEqual(rows['lab.damage'], 'REGRESSION')
        calm_run, _ = metrics.lab_attack_gba(spec)(with_drone({sf: 40.0 for sf in range(0, 80)}), golden, report)
        rows = {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, calm_run['values'], {}, marked)}
        self.assertEqual(rows['lab.damage'], 'REGRESSION')   # no fight where the baseline fought
        ling = {sf: 35.0 - 6 * (sf >= 20) for sf in range(0, 80)}   # one clean foe hit, then the Drone
        mixed = walk(pos, hp={1: marine, 2: ling, 4: {sf: 40.0 - 6 * (sf >= 28) for sf in range(0, 80)}},
                     others=[(2, metrics.ZERGLING, 4, 150, 100), (4, metrics.DRONE, 4, 160, 100)])
        result, _ = metrics.lab_attack_gba(spec)(mixed, golden, report)
        self.assertEqual((result['measured']['off_target'], result['measured']['inconclusive']), ([], ['lab.cooldown']))
        # A unit born and killed inside one source frame shows only in the removal record.
        self.assertEqual(verdicts(with_drone({sf: 40.0 for sf in range(0, 80)}, x=900,
                                             removals=[('U', 35, 9, 4, 170, 100, 0, None, 40)])), fought)
        # The controls: a Drone wounded out of reach, a row that goes on in place as another unit
        # (an Egg hatching: no removal at 0 hit points), a hatch removal with hit points left, and
        # a neutral or friendly removal keep the no-fight baseline.
        self.assertEqual(verdicts(with_drone({sf: 40.0 - 6 * (sf >= 28) for sf in range(0, 80)}, x=700)), calm)
        hatch = with_drone({sf: 40.0 for sf in range(0, 80)})
        for frame in hatch['frames']:
            if frame['sf'] >= 28:
                frame['units'][4] = (metrics.ZERGLING, 4, 160, 100)
        self.assertEqual(verdicts(hatch), calm)
        self.assertEqual(verdicts(with_drone(gone, removals=[('U', 35, 4, 4, 160, 100, 40, None, 40)])), calm)
        for owner in (11, 1):
            self.assertEqual(verdicts(with_drone({sf: 40.0 for sf in range(0, 80)}, x=900,
                                                 removals=[('B', 35, 7, owner, 200, 100, 0, 500, 500)])), calm)

        def with_building(rows, removals=()):
            series = walk(pos, hp={1: marine, 2: calm_ling}, others=[(2, metrics.ZERGLING, 4, 900, 100)])
            for frame in series['frames']:
                b = rows(frame['sf'])
                frame['buildings'] = {7: b} if b else {}
            return as_gba(series, removals)
        # A GBA building in reach that loses hit points or shields, or is destroyed (also when it
        # was born and destroyed inside one source frame),
        self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, 500 - 6 * (sf >= 28), 0, 1500))), fought)
        self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, 500, 100 - 6 * (sf >= 28), 500))), fought)
        self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, 500, 0, 500) if sf < 28 else None,
                                                [('B', 35, 7, 4, 200, 100, 0, 500, 500)])), fought)
        self.assertEqual(verdicts(with_building(lambda sf: None, [('B', 35, 7, 4, 200, 100, 0, 500, 500)])), fought)
        # and a burning Barracks (a third of its hit points or less) hit harder than it burns.
        self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, 400 - 6 * (sf >= 28), 0, 1500, 3, 111, 0))), fought)
        # The controls: the same damage out of reach or to a friendly building, a burning
        # building's 1 hp a sample, and a burning building that burns out.
        self.assertEqual(verdicts(with_building(lambda sf: (4, 700, 100, 500 - 6 * (sf >= 28), 0, 1500))), calm)
        self.assertEqual(verdicts(with_building(lambda sf: (1, 200, 100, 500 - 6 * (sf >= 28), 0, 1500))), calm)
        self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, 400 - (sf >= 28), 0, 1500, 3, 111, 0))), calm)
        burnout = ('B', 35, 7, 4, 200, 100, 0, 1500, 1, 111, 3, 34, 0)
        self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, 1, 0, 1500, 3, 111, 0) if sf < 28 else None,
                                                [burnout])), calm)
        # Burn stops at exactly 0, so overkill is a shot; and only a completed Terran building
        # burns (a Hatchery, an unfinished Barracks, one of unknown kind: kills).
        for record in (burnout[:6] + (-5,) + burnout[7:], burnout[:9] + (131,) + burnout[10:],
                       burnout[:12] + (1,), burnout[:9] + (-1,) + burnout[10:]):
            self.assertEqual(verdicts(with_building(lambda sf: None, [record])), fought)
        # The runner's engine-type map names a row's kind when the row carries no cartridge id.
        series = with_building(lambda sf: None, [burnout[:9] + (-1,) + burnout[10:]])
        result, _ = metrics.lab_attack_gba(spec)(series, golden, dict(report, building_raw=[0, 0, 0, 111]))
        self.assertEqual(result['measured']['combat'], False)
        # A slot whose type changes between samples (a replacement inside one engine step) holds
        # two buildings: no drop.
        self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, 500, 0, 1500, 3, 111, 0) if sf < 28
                                                else (4, 200, 100, 200, 0, 1000, 4, 112, 0))), calm)
        # A slot's earlier occupant killed in a step that shares the foe's birth frame is not the
        # foe's removal: the foe, born in the next step, leaves with hit points.
        series = as_gba(walk(pos, hp={1: marine, 2: {sf: 35.0 for sf in range(16, 17)}},
                             others=[(2, metrics.ZERGLING, 4, 900, 100)]),
                        [('U', 20, 2, 4, 900, 100, -3, None, 40, metrics.DRONE, 0, None, None),
                         ('U', 22, 2, 4, 900, 100, 35, None, 35, metrics.ZERGLING, 0, None, None)])
        self.assertEqual(verdicts(series), calm)
        # A foe killed while its slot is refilled in the same step: the successor leaving with hit
        # points in the next step (the same source frame) does not stand for the foe's end.
        for refill in ([('+U', 20, 2, 0, 4)], []):
            series = as_gba(walk(pos, hp={1: marine, 2: {sf: 35.0 for sf in range(0, 16)}},
                                 others=[(2, metrics.ZERGLING, 4, 900, 100)]),
                            [('U', 21, 2, 4, 900, 100, 40, None, 40, metrics.DRONE, 0, None, None)])
            series['events'] = refill
            self.assertEqual(verdicts(series), fought)
            result, _ = metrics.lab_attack_gba(spec)(series, golden, report)
            self.assertTrue(result['measured']['foe_combat'])
        # A foe removed with hit points left (a mission script) was not killed.
        series = as_gba(walk(pos, hp={1: marine, 2: {sf: 35.0 for sf in range(0, 28)}},
                             others=[(2, metrics.ZERGLING, 4, 900, 100)]),
                        [('U', 35, 2, 4, 900, 100, 35, None, 35, metrics.ZERGLING, 0, None, None)])
        self.assertEqual(verdicts(series), calm)
        # Burn explains a death only from what it could take since the building's last row: a
        # shot that kills a building at 5 hp, or a burning one at 300, is a kill.
        for last_hp, max_hp in ((5, 500), (300, 1500)):
            self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, last_hp, 0, max_hp) if sf < 28 else None,
                                                    [('B', 35, 7, 4, 200, 100, -5, max_hp, last_hp, -1, 0, 34)])), fought)
        # A slot whose building left with hit points and was replaced by a weaker one between two
        # samples holds two buildings: no drop.
        self.assertEqual(verdicts(with_building(lambda sf: (4, 200, 100, 500 if sf < 28 else 200, 0, 1500),
                                                [('B', 35, 7, 4, 200, 100, 500, 1500, 500, -1, 0, 30)])), calm)
        # A foe born and killed inside one source frame is a foe kill, by its row's id or the
        # runner's engine-type map: a fight with the foe, never an off-target one.
        for record, raw_map in ((('U', 35, 9, 4, 170, 100, 0, None, 35, metrics.ZERGLING, 3, None), None),
                                (('U', 35, 9, 4, 170, 100, 0, None, 35, -1, 3, None), [0, 0, 0, metrics.ZERGLING])):
            series = with_drone({sf: 40.0 - 6 * (sf >= 28) for sf in range(0, 80)}, removals=[record])
            result, _ = metrics.lab_attack_gba(spec)(series, golden, dict(report, unit_raw=raw_map))
            self.assertEqual(result['measured']['off_target'], [])
            self.assertEqual(result['measured']['inconclusive'], ['lab.cooldown', 'lab.damage'])

    def test_an_early_end_or_a_lost_subject_is_not_an_absence(self):
        import sc64_parity
        obs = [metrics.observation(c, 'Marine', v, v - 1, v + 1, '+-1', True, [], provenance='n64-ram', n_runs=2)
               for c, v in (('lab.damage', 6), ('lab.survival', 50))]
        golden = {'observations': obs, 'gba': {'steps': 100}}
        baseline = {f'lab-x:{o["check"]}:{o["subject"]}': {'gba': metrics.ABSENT} for o in obs}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        series = walk(pos, hp={1: {sf: 40.0 for sf in range(0, 80)}, 2: {sf: 35.0 for sf in range(0, 80)}},
                      others=[(2, metrics.ZERGLING, 4, 900, 100)])

        def verdicts(report):
            result, _ = metrics.lab_attack_gba({'foe_type': metrics.ZERGLING})(series, golden, report)
            return {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, baseline)}
        # The run stopped at step 60 of 100 (the mission left play): nothing is established.
        self.assertEqual(verdicts({'subject': 1, 'lab': [[5, 4, 1, -1]], 'steps': 60}),
                         {'lab.damage': 'REGRESSION', 'lab.survival': 'REGRESSION'})
        # The measure found no subject at all.
        self.assertEqual(verdicts({'subject': 3, 'lab': [[5, 4, 1, -1]], 'steps': 100}),
                         {'lab.damage': 'REGRESSION', 'lab.survival': 'REGRESSION'})
        # The complete run keeps the no-fight baseline.
        self.assertEqual(verdicts({'subject': 1, 'lab': [[5, 4, 1, -1]], 'steps': 100}),
                         {'lab.damage': 'KNOWN', 'lab.survival': 'KNOWN'})

    def test_off_target_damage_needs_the_subject_as_sole_attacker(self):
        import sc64_parity
        obs = [metrics.observation(c, 'Marine', v, v - 1, v + 1, '+-1', True, [], provenance='n64-ram', n_runs=2)
               for c, v in (('lab.cooldown', 15), ('lab.damage', 6), ('lab.survival', 50))]
        golden = {'observations': obs}
        off = {f'lab-x:{o["check"]}:{o["subject"]}': {'gba': metrics.OFF_TARGET} for o in obs}
        off['lab-x:lab.survival:Marine'] = {'gba': metrics.ABSENT}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        report = {'subject': 1, 'lab': [[5, 4, 1, -1]]}
        spec = {'foe_type': metrics.ZERGLING}

        def run(series):
            result, _ = metrics.lab_attack_gba(spec)(series, golden, report)
            rows = {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, off)}
            return result, rows
        wounded = {sf: 40.0 - 6 * (sf >= 28) for sf in range(0, 80)}
        for rival in ((), ((5, metrics.MARINE, 1, 220, 100),)):
            series = as_gba(walk(pos, hp={1: marine, 2: {sf: 35.0 for sf in range(0, 80)}, 4: wounded,
                                          **({5: marine} if rival else {})},
                                 others=[(2, metrics.ZERGLING, 4, 900, 100), (4, metrics.DRONE, 4, 160, 100)] + list(rival)))
            result, rows = run(series)
            if not rival:   # the subject alone in reach of the Drone: an off-target fight
                self.assertEqual(result['measured']['off_target'], ['lab.cooldown', 'lab.damage'])
                self.assertEqual(set(rows.values()), {'KNOWN'})
            else:           # another Marine in reach of it too: nobody can say who fought
                self.assertEqual(result['measured']['off_target'], [])
                self.assertEqual(result['measured']['inconclusive'], ['lab.cooldown', 'lab.damage'])
                self.assertEqual((rows['lab.damage'], rows['lab.survival']), ('REGRESSION', 'KNOWN'))
        # The same for a GBA building: a second human unit in reach of it leaves the drop unattributed.
        series = as_gba(walk(pos, hp={1: marine, 2: {sf: 35.0 for sf in range(0, 80)}, 5: marine},
                             others=[(2, metrics.ZERGLING, 4, 900, 100), (5, metrics.MARINE, 1, 300, 100)]))
        for frame in series['frames']:
            frame['buildings'] = {7: (4, 200, 100, 500 - 6 * (frame['sf'] >= 28), 0, 1500, 3, 154, 0)}
        result, _ = run(series)
        self.assertEqual(result['measured']['inconclusive'], ['lab.cooldown', 'lab.damage'])

    def test_weapon_hits_in_the_trace_override_the_burn_allowance(self):
        import sc64_parity
        obs = [metrics.observation(c, 'Marine', v, v - 1, v + 1, '+-1', True, [], provenance='n64-ram', n_runs=2)
               for c, v in (('lab.cooldown', 15), ('lab.damage', 6))]
        golden = {'observations': obs}
        baseline = {f'lab-x:{o["check"]}:{o["subject"]}': {'gba': metrics.ABSENT} for o in obs}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        report = {'subject': 1, 'lab': [[5, 4, 1, -1]]}

        def verdicts(rows, removals=(), hits=()):
            series = as_gba(walk(pos, hp={1: {sf: 40.0 for sf in range(0, 80)}, 2: {sf: 35.0 for sf in range(0, 80)}},
                                 others=[(2, metrics.ZERGLING, 4, 900, 100)]), removals)
            for frame in series['frames']:
                b = rows(frame['sf'])
                frame['buildings'] = {7: b} if b else {}
            series['hits'], series['max_units'] = list(hits), 100
            result, _ = metrics.lab_attack_gba({'foe_type': metrics.ZERGLING})(series, golden, report)
            return set(r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, baseline))
        burning = lambda sf: (4, 200, 100, 400 - (sf >= 28), 0, 1500, 3, 111, 0)   # a burning Barracks, -1 hp
        self.assertEqual(verdicts(burning), {'KNOWN'})                              # burn alone
        self.assertEqual(verdicts(burning, hits=[(35, 107, 6, 0, 1)]), {'REGRESSION'})   # a weapon hit at step 35
        self.assertEqual(verdicts(burning, hits=[(35, 108, 6, 0, 1)]), {'KNOWN'})        # on another building
        burnout = ('B', 35, 7, 4, 200, 100, 0, 1500, 1, 111, 3, 34, 0)
        last = lambda sf: (4, 200, 100, 1, 0, 1500, 3, 111, 0) if sf < 28 else None
        self.assertEqual(verdicts(last, [burnout]), {'KNOWN'})
        self.assertEqual(verdicts(last, [burnout], hits=[(35, 107, 6, 0, 1)]), {'REGRESSION'})

    def test_an_unconfirmed_gba_death_leaves_survival_unmeasured(self):
        import sc64_parity
        obs = [metrics.observation('lab.survival', 'Marine', 50, 46, 54, '+-4', True, [], provenance='n64-ram', n_runs=2)]
        golden = {'observations': obs}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        report = {'subject': 1, 'lab': [[5, 4, 1, -1]]}
        # The subject is last sampled at 29: an owner change and a live removal in one source
        # frame leave a removal record of another owner; or no removal record at all.
        for removals, lost in (([('U', 38, 1, 2, 100, 100, 40, None, 40, metrics.MARINE, 0, None, None)], True),
                               ([], True),
                               ([('U', 38, 1, 1, 100, 100, 0, None, 40, metrics.MARINE, 0, None, None)], False)):
            series = as_gba(walk(pos, hp={1: {sf: 40.0 for sf in range(0, 30)}, 2: {sf: 35.0 for sf in range(0, 80)}},
                                 others=[(2, metrics.ZERGLING, 4, 900, 100)]), removals)
            result, _ = metrics.lab_attack_gba({'foe_type': metrics.ZERGLING})(series, golden, report)
            self.assertEqual(result['measured']['subject_lost'], lost)
            if lost:
                self.assertIsNone(result['measured']['death'])
                self.assertEqual(result['measured']['inconclusive'], ['lab.survival'])

    def test_hits_in_one_frame_are_not_timed_apart(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        a = {sf: 35.0 - 6 * len([t for t in (20, 35) if t <= sf]) for sf in range(0, 80)}
        b = {sf: 35.0 - 6 * (sf >= 20) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: a, 3: b},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (3, metrics.ZERGLING, 4, 140, 110)])
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual((m['gaps'], m['range_first'], m['first_hit']), ([15], None, 15))

    def test_only_an_enemy_building_removal_splits_the_timing(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35) if t <= sf]) for sf in range(0, 80)}
        for owner, gaps in ((1, [15]), (4, [])):   # the human's own building, then an enemy's
            series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
            series['events'] = [('-B', 35, 7, 0, 0)]
            series['removals'] = [('B', 35, 7, owner, 900, 900, 5, 500, 5, -1, 0, 34, 0)]
            self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], gaps)

    def test_live_owners_and_damage_records(self):
        import sc64_parity
        obs = [metrics.observation(c, 'Marine', v, v - 1, v + 1, '+-1', True, [], provenance='n64-ram', n_runs=2)
               for c, v in (('lab.cooldown', 15), ('lab.damage', 6))]
        golden = {'observations': obs}
        report = {'subject': 1, 'lab': [[5, 4, 1, -1]]}
        spec = {'foe_type': metrics.ZERGLING}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        calm_ling = {sf: 35.0 for sf in range(0, 80)}
        wounded = {sf: 40.0 - 6 * (sf >= 28) for sf in range(0, 80)}

        def measured(series):
            return metrics.lab_attack_gba(spec)(series, golden, report)[0]['measured']
        # A rescued unit keeps its cartridge owner (4) but fights for the human (live owner 0):
        # in reach of the wounded Drone it is a second attacker, so the drop is unattributed.
        series = as_gba(walk(pos, hp={1: marine, 2: calm_ling, 4: wounded, 5: marine},
                             others=[(2, metrics.ZERGLING, 4, 900, 100), (4, metrics.DRONE, 4, 160, 100),
                                     (5, metrics.MARINE, 4, 220, 100)]))
        for frame in series['frames']:
            frame['live'] = {1: 0, 2: 3, 4: 3, 5: 0}
        m = measured(series)
        self.assertEqual((m['off_target'], m['inconclusive']), ([], ['lab.cooldown', 'lab.damage']))
        # Weapon-hit records count whatever the sampled hit points show: on an enemy building in
        # reach (its hit points offset by repair), on the foe, or on the subject.
        def quiet(hits_, building=False):
            series = as_gba(walk(pos, hp={1: marine, 2: calm_ling}, others=[(2, metrics.ZERGLING, 4, 900, 100)]))
            for frame in series['frames']:
                frame['buildings'] = {7: (4, 200, 100, 500, 0, 1500, 3, 154, 0, 3)} if building else {}
            series['hits'], series['max_units'] = hits_, 100
            return measured(series)
        self.assertEqual(quiet([], building=True)['combat'], False)
        m = quiet([(35, 107, 6, 0, 0)], building=True)
        self.assertEqual((m['combat'], m['foe_combat'], m['off_target']), (True, False, ['lab.cooldown', 'lab.damage']))
        m = quiet([(35, 2, 6, 0, 0)])
        self.assertEqual((m['foe_combat'], m['inconclusive']), (True, ['lab.cooldown', 'lab.damage']))
        m = quiet([(35, 1, 5, 3, 3)])
        self.assertEqual((m['foe_combat'], m['inconclusive']), (True, ['lab.cooldown', 'lab.damage']))

    def test_weapon_hit_records_on_the_foe_and_killing_hits(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35, 50) if t <= sf]) for sf in range(0, 80)}

        def measure(hits_, removals=(), drone=None):
            hp = {1: marine, 2: ling, **({4: drone} if drone else {})}
            others = [(2, metrics.ZERGLING, 4, 150, 100)] + ([(4, metrics.DRONE, 4, 160, 100)] if drone else [])
            series = as_gba(walk(pos, hp=hp, others=others), removals)
            series['hits'], series['max_units'] = hits_, 100
            return metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]
        steps = {sf: (5 * sf + 4) // 4 for sf in (20, 27, 35, 50)}
        matched = [(steps[sf], 2, 6, 0, 0) for sf in (20, 35, 50)]
        m = measure(matched)
        self.assertEqual((m['gaps'], m['range_first'] is not None), ([15, 15], True))
        # A hit on the foe at 27 that no sampled drop shows may be a shot of the subject's: the
        # interval across it goes, and so does the first hit's range when it comes first.
        m = measure(matched + [(steps[27], 2, 6, 0, 0)])
        self.assertEqual(m['gaps'], [15])
        m = measure(matched + [(steps[20] - 8, 2, 6, 0, 0)])
        self.assertEqual((m['range_first'], m['first_hit']), (None, None))
        # A hit in the pre-command frame is not evidence of a fight after the command.
        quiet = as_gba(walk(pos, hp={1: marine, 2: {sf: 35.0 for sf in range(0, 80)}},
                            others=[(2, metrics.ZERGLING, 4, 900, 100)]))
        quiet['hits'], quiet['max_units'] = [((5 * 5 + 4) // 4, 1, 5, 3, 3)], 100
        self.assertFalse(metrics.lab_attack_measure(quiet, 1, 5, metrics.ZERGLING)[0]['combat'])
        # A killing hit on a Drone in reach is credited through its removal record (the Drone's
        # last position), not left ambiguous because the Drone is gone from the next sample.
        gone = {sf: 40.0 for sf in range(0, 28)}
        series = as_gba(walk(pos, hp={1: marine, 2: {sf: 35.0 for sf in range(0, 80)}, 4: gone},
                             others=[(2, metrics.ZERGLING, 4, 900, 100), (4, metrics.DRONE, 4, 160, 100)]),
                        [('U', 35, 4, 4, 160, 100, -2, None, 40, metrics.DRONE, 0, None, None)])
        series['hits'], series['max_units'] = [(35, 4, 6, 0, 0)], 100
        m = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]
        self.assertEqual((m['combat'], m['foe_combat'], m['nearby_ambiguous']), (True, False, False))

    def test_hit_records_that_samples_cannot_resolve(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        calm_ling = {sf: 35.0 for sf in range(0, 80)}
        # A shot on a Drone in reach, which construction then consumes in the same step (removed
        # with hit points left): the shot is still a fight.
        drone = {sf: 40.0 for sf in range(0, 28)}
        series = as_gba(walk(pos, hp={1: marine, 2: calm_ling, 4: drone},
                             others=[(2, metrics.ZERGLING, 4, 900, 100), (4, metrics.DRONE, 4, 160, 100)]),
                        [('U', 35, 4, 4, 160, 100, 34, None, 40, metrics.DRONE, 0, None, None)])
        series['hits'], series['max_units'] = [(35, 4, 6, 0, 0)], 100
        self.assertTrue(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['combat'])
        # A friendly slot reused by a unit born at step 35 and hit at 35, before any sample of it:
        # the hit is unresolved, never discarded as a hit on the friendly unit.
        series = as_gba(walk(pos, hp={1: marine, 2: calm_ling, 6: {sf: 40.0 for sf in range(0, 28)}},
                             others=[(2, metrics.ZERGLING, 4, 900, 100), (6, metrics.MARINE, 1, 600, 100)]))
        series['events'] = [('+U', 35, 6, 0, 4)]
        series['hits'], series['max_units'] = [(35, 6, 6, 0, 0)], 100
        m = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]
        self.assertEqual((m['combat'], m['nearby_ambiguous']), (True, True))
        # Two hit records on the foe in one frame whose sum looks like one hit: no damage value
        # from that drop, and no interval touching it.
        ling = {sf: 35.0 - 6 * len([t for t in (20, 36, 52) if t <= sf]) for sf in range(0, 80)}
        series = as_gba(walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)]))
        # Source frame 36 holds engine steps 45 and 46 (5 steps to 4 frames).
        series['hits'] = [(26, 2, 6, 0, 0), (45, 2, 3, 0, 0), (46, 2, 3, 0, 0), (66, 2, 6, 0, 0)]
        series['max_units'] = 100
        m = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]
        self.assertEqual((m['damage_values'], m['gaps']), ([6], []))
        # Two hits on a Drone in the step its slot is refilled by a friendly unit (a type and
        # owner change record in that step): their target is unknown, and they split the timing.
        # (the Drone in the subject's reach, out of the Zergling's)
        series = as_gba(walk(pos, hp={1: marine, 2: ling, 4: {sf: 40.0 for sf in range(0, 80)}},
                             others=[(2, metrics.ZERGLING, 4, 150, 100), (4, metrics.DRONE, 4, -150, 100)]))
        for frame in series['frames']:
            if frame['sf'] >= 28:
                frame['units'][4] = (metrics.DRONE, 1, -150, 100)
        series['events'] = [('OU', 35, 4, 4, 1)]
        series['hits'] = [(26, 2, 6, 0, 0), (35, 4, 6, 0, 0), (35, 4, 6, 0, 0), (66, 2, 6, 0, 0)]
        series['max_units'] = 100
        m = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]
        self.assertEqual((m['nearby_ambiguous'], m['gaps']), (True, [16]))
        # Two records on a distant ally between the subject's hits split nothing.
        series = as_gba(walk(pos, hp={1: marine, 2: ling, 6: {sf: 40.0 for sf in range(0, 80)}},
                             others=[(2, metrics.ZERGLING, 4, 150, 100), (6, metrics.MARINE, 1, 900, 100)]))
        series['hits'] = [(26, 2, 6, 0, 0), (45, 6, 3, 3, 3), (46, 6, 3, 3, 3), (66, 2, 6, 0, 0)]
        series['max_units'] = 100
        m = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]
        self.assertEqual(m['gaps'], [16, 16])
        # A friendly slot refilled by a foe inside one engine step (an owner change record, no
        # birth), hit in the next step of the same source frame before any sample of the foe:
        # unresolved, never discarded as a hit on the friendly unit.
        series = as_gba(walk(pos, hp={1: marine, 2: calm_ling, 6: {sf: 40.0 for sf in range(0, 36)}},
                             others=[(2, metrics.ZERGLING, 4, 900, 100), (6, metrics.MARINE, 1, 600, 100)]))
        series['events'] = [('OU', 45, 6, 1, 4)]
        series['hits'], series['max_units'] = [(46, 6, 6, 0, 0)], 100
        m = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]
        self.assertEqual((m['combat'], m['nearby_ambiguous']), (True, True))

    def test_neutral_removals_and_interval_endpoints(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35) if t <= sf]) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        series['events'] = [('-B', 35, 7, 0, 0)]   # a neutral mineral field's removal: no barrier
        series['removals'] = [('B', 35, 7, 11, 900, 900, 0, 500, 5, -1, 0, 34, 0)]
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [15])
        # A drop on a Drone in reach in the second hit's own frame may hide a shot: no interval.
        drone = {sf: 40.0 - 6 * (sf >= 35) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling, 4: drone},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (4, metrics.DRONE, 4, 160, 100)])
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [])

    def test_a_lost_or_shielded_subject(self):
        import sc64_parity
        obs = [metrics.observation(c, 'Marine', v, v - 1, v + 1, '+-1', True, [], provenance='n64-ram', n_runs=2)
               for c, v in (('lab.damage', 6), ('lab.survival', 50))]
        golden = {'observations': obs}
        baseline = {f'lab-x:{o["check"]}:{o["subject"]}': {'gba': metrics.ABSENT} for o in obs}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        report = {'subject': 1, 'lab': [[5, 4, 1, -1]]}
        spec = {'foe_type': metrics.ZERGLING}

        def run(series):
            result, _ = metrics.lab_attack_gba(spec)(series, golden, report)
            rows = {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, baseline)}
            return result, rows
        # The subject's row vanishes at 30. Its removal record with hit points left is a lost
        # subject, not a death: survival is unmeasured. At 0 it is a death.
        for hp_left, lost in ((40, True), (0, False)):
            series = as_gba(walk(pos, hp={1: {sf: 40.0 for sf in range(0, 30)}, 2: {sf: 35.0 for sf in range(0, 80)}},
                                 others=[(2, metrics.ZERGLING, 4, 900, 100)]),
                            [('U', 38, 1, 1, 100, 100, hp_left, None, 40, metrics.MARINE, 0, None)])
            result, rows = run(series)
            self.assertEqual(result['measured']['subject_lost'], lost)
            if lost:
                self.assertIsNone(result['measured']['survival'])
                self.assertEqual(rows, {'lab.damage': 'REGRESSION', 'lab.survival': 'REGRESSION'})
            else:
                self.assertEqual(result['measured']['death'], 26)   # from the command frame, 4
        # A hit on the subject's shields alone is a hit taken: a fight, never an absence.
        series = walk(pos, hp={1: {sf: 40.0 for sf in range(0, 80)}, 2: {sf: 35.0 for sf in range(0, 80)}},
                      others=[(2, metrics.ZERGLING, 4, 900, 100)])
        for frame in series['frames']:
            frame['shield'] = {1: 80.0 - 5 * (frame['sf'] >= 30)}
        result, rows = run(series)
        self.assertEqual(result['measured']['inconclusive'], ['lab.damage'])
        self.assertEqual(rows['lab.damage'], 'REGRESSION')

    def test_a_changed_subject_leaves_its_checks_unmeasured(self):
        import sc64_parity
        obs = [metrics.observation(c, 'Marine', v, v - 1, v + 1, '+-1', True, [], provenance='n64-ram', n_runs=2)
               for c, v in (('lab.damage', 6), ('lab.survival', 50))]
        golden = {'observations': obs}
        baseline = {f'lab-x:{o["check"]}:{o["subject"]}': {'gba': metrics.ABSENT} for o in obs}
        pos = {sf: (100, 100) for sf in range(0, 80)}
        # No damage anywhere; the subject's row changes owner in place at 30 (a Mind Control).
        series = walk(pos, hp={1: {sf: 40.0 for sf in range(0, 80)}, 2: {sf: 35.0 for sf in range(0, 80)}},
                      others=[(2, metrics.ZERGLING, 4, 900, 100)])
        for frame in series['frames']:
            if frame['sf'] >= 30:
                frame['units'][1] = (metrics.MARINE, 2, 100, 100)
        result, _ = metrics.lab_attack_gba({'foe_type': metrics.ZERGLING})(series, golden, {'subject': 1, 'lab': [[5, 4, 1, -1]]})
        self.assertTrue(result['measured']['subject_changed'])
        self.assertEqual(result['measured']['inconclusive'], ['lab.damage', 'lab.survival'])
        verdicts = {r['check']: r['verdict'] for r in sc64_parity.judge('lab-x', golden, result['values'], {}, baseline)}
        self.assertEqual(verdicts, {'lab.damage': 'REGRESSION', 'lab.survival': 'REGRESSION'})

    def test_spawned_enemies_and_buildings_split_the_timing(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35) if t <= sf]) for sf in range(0, 80)}
        egg = {sf: (5.0 if sf < 30 else 0.0) for sf in range(25, 80)}   # born at 25, killed at 30
        series = walk(pos, hp={1: marine, 2: ling, 3: egg},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (3, metrics.DRONE, 4, 160, 100)])
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [])
        # A GBA enemy building destroyed between the two hits.
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        for frame in series['frames']:
            frame['buildings'] = {7: (4, 600, 100)} if frame['sf'] < 28 else {}
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [])

    def test_an_unresolved_first_hit_withholds_the_range(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35, 50) if t <= sf]) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        series['frames'][20]['units'][5] = (metrics.MARINE, 1, 200, 100)   # a rival at the first hit only
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual((m['hits'], m['range_first'], m['first_hit']), (2, None, None))

    def test_an_unresolved_first_hit_still_sizes_the_replay(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35, 50) if t <= sf]) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        series['frames'][20]['units'][5] = (metrics.MARINE, 1, 200, 100)
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual(m['first_landed'], 30)
        checks = [o['check'] for o in metrics.lab_attack_observations(m, 'Marine', 'Zergling')]
        self.assertIn('lab.damage', checks)
        self.assertNotIn('lab.range_first', checks)
        self.assertNotIn('lab.first_hit', checks)

    def test_a_hatched_or_reused_row_is_a_new_unit(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        # Row 2 is an Egg until 30, then the Zergling that hatched from it; the Marine hits it at 40 and 55.
        ling = {sf: (200.0 if sf < 30 else 35.0 - 6 * len([t for t in (40, 55) if t <= sf])) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, 36, 4, 150, 100)])
        for frame in series['frames']:
            if frame['sf'] >= 30:
                frame['units'][2] = (metrics.ZERGLING, 4, 150, 100)
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual((m['hits'], m['gaps']), (2, [15]))   # the hatch itself is no hit
        self.assertEqual(list(m['victims']), ['2.1'])

    def test_a_hatch_is_no_kill_and_same_frame_drops_sort(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35) if t <= sf]) for sf in range(0, 80)}
        egg = {sf: 200.0 for sf in range(0, 80)}   # an unrelated Egg hatches at 28
        series = walk(pos, hp={1: marine, 2: ling, 3: egg},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (3, 36, 4, 900, 100)])
        for frame in series['frames']:
            if frame['sf'] >= 28:
                frame['units'][3] = (metrics.ZERGLING, 4, 900, 100)
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [15])
        # Two credited drops on one frame, one on a later lifetime of its row: the hits still sort.
        ling2 = {sf: (30.0 if sf < 10 else 35.0 - 6 * len([t for t in (20,) if t <= sf])) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling, 4: ling2},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (4, metrics.DRONE, 4, 160, 100)])
        for frame in series['frames']:
            if frame['sf'] >= 10:
                frame['units'][4] = (metrics.ZERGLING, 4, 160, 100)
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual(sorted(m['victims']), ['2', '4.1'])

    def test_a_gba_type_change_is_a_removal(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35) if t <= sf]) for sf in range(0, 80)}
        other = {sf: 100.0 for sf in range(0, 80)}   # a Zealot whose slot a Probe takes at 28
        series = walk(pos, hp={1: marine, 2: ling, 3: other},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (3, 65, 4, 900, 100)])
        for frame in series['frames']:
            if frame['sf'] >= 28:
                frame['units'][3] = (64, 4, 900, 100)
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [15])   # N64: a morph
        series['source'] = 'gba'   # the GBA can kill and reuse a slot in one step: a removal
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [])
        # Zerg-profile rows carry no raw type (-1): the engine type alone shows the replacement.
        for frame in series['frames']:
            frame['units'][3] = (-1, 4, 900, 100)
            frame['etype'] = {3: 7 if frame['sf'] < 28 else 9}
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [])

    def test_a_replaced_subject_is_not_the_subject(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: (40.0 if sf < 30 else 60.0) for sf in range(0, 80)}   # row 1 becomes an SCV at 30
        series = walk(pos, hp={1: marine}, others=[])
        for frame in series['frames']:
            if frame['sf'] >= 30:
                frame['units'][1] = (metrics.SCV, 1, 100, 100)
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        # Its lifetime ended at 30 without a death: never 60 hit points, and no survival either.
        self.assertEqual((m['death'], m['survival'], m['subject_changed']), (None, None, True))
        # A GBA birth record in the slot means the subject was removed first: that is a death.
        series['events'] = [('+U', 38, 1, 0, 0)]   # GBA step 38 = source frame 30
        m, _ = metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)
        self.assertEqual((m['death'], m['subject_changed']), (25, False))

    def test_a_gba_building_removal_event_splits_the_timing(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 for sf in range(0, 80)}
        ling = {sf: 35.0 - 6 * len([t for t in (20, 35) if t <= sf]) for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 150, 100)])
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [15])
        series['events'] = [('-B', 35, 7, 0, 0), ('+B', 35, 7, 0, 0)]   # GBA step 35 = source frame 28
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [])

    def test_damage_taken_across_a_type_change(self):
        pos = {sf: (100, 100) for sf in range(0, 80)}
        marine = {sf: 40.0 - 5 * len([t for t in (20, 30) if t <= sf]) for sf in range(0, 80)}
        ling = {sf: 35.0 for sf in range(0, 80)}
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 130, 100)])
        series['frames'][19]['units'][2] = (metrics.DRONE, 4, 130, 100)   # the row was a Drone before the hit
        self.assertFalse(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['taken_single'])

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
        # A kill of another enemy type splits the timing the same way.
        series = walk(pos, hp={1: marine, 2: ling, 3: other},
                      others=[(2, metrics.ZERGLING, 4, 150, 100), (3, metrics.DRONE, 4, 160, 100)])
        self.assertEqual(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['gaps'], [15])

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
        # The only enemy in reach is a building, or a unit of another type: neither stands
        # for the Zergling's damage.
        for others in ([(2, metrics.ZERGLING, 4, 900, 100), (3, 125, 4, 130, 100)],
                       [(2, metrics.ZERGLING, 4, 900, 100), (3, metrics.DRONE, 4, 130, 100)]):
            series = walk(pos, hp={1: marine, 2: ling, 3: {sf: 300.0 for sf in range(0, 80)}}, others=others)
            self.assertFalse(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['taken_single'])
        series = walk(pos, hp={1: marine, 2: ling}, others=[(2, metrics.ZERGLING, 4, 900, 100)])
        for frame in series['frames']:   # a lone GBA building
            frame['buildings'] = {7: (4, 200, 100)}
        self.assertFalse(metrics.lab_attack_measure(series, 1, 5, metrics.ZERGLING)[0]['taken_single'])

    def test_n64_order_must_match_the_replay(self):
        class Cap:
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}}
                     for s, t, o in ((100, 50, 3), (103, 51, 3), (106, 52, 14), (109, 53, 14))]

            @staticmethod
            def unit(row, name):
                return row if name == 'order' else 0   # type and owner stay the same
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

        class Unread(Cap):   # tick 51 began before the press and was never read: its order is unknown
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}} for s, t, o in ((100, 50, 3), (106, 52, 6))]
            change_steps = {50: 99, 51: 102, 52: 105}
        ok, seen = metrics.n64_order_after(Unread, 9, 103, metrics.ORDER_MOVE)
        self.assertIsNone(ok)
        self.assertIn('the order at the press was not read', seen)

        class HeldRead(Cap):   # tick 51 began at the press step and was read one step later,
            # with the button held: Move there is the press's order, not the state before it
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}} for s, t, o in ((100, 50, 2), (102, 51, 6), (105, 52, 14))]
            change_reads = [(99, 50), (101, 51), (104, 52)]
        self.assertEqual(metrics.n64_order_after(HeldRead, 9, 101, (metrics.ORDER_MOVE, metrics.ORDER_ATTACK_MOVE)), (6, [6]))
        self.assertIsNone(metrics.n64_order_after(HeldRead, 9, 101, metrics.ORDER_ATTACK_MOVE)[0])

        class PressSkip(Cap):   # tick 51 began at the press step and ended inside it, unread: it
            # could have held the press's first order, so a later Move is no evidence
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}} for s, t, o in ((100, 50, 3), (103, 52, 6))]
            change_reads = [(99, 50), (101, 51), (102, 52)]
        ok, seen = metrics.n64_order_after(PressSkip, 9, 101, metrics.ORDER_MOVE)
        self.assertIsNone(ok)
        self.assertIn('the order at the press was not read', seen)

        class Wrapped(Cap):   # the counter wraps between the read pre-press tick and the press
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}} for s, t, o in ((100, 0xFFFFFFFF, 3), (103, 0, 3), (106, 1, 6))]
            change_steps = {0xFFFFFFFF: 99, 0: 102, 1: 105}
        self.assertEqual(metrics.n64_order_after(Wrapped, 9, 103, metrics.ORDER_MOVE)[0], 6)

        class WrappedUnread(Cap):   # ... and the tick that began at the wrap was never read
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}} for s, t, o in ((100, 0xFFFFFFFF, 3), (106, 1, 6))]
            change_steps = {0xFFFFFFFF: 99, 0: 102, 1: 105}
        self.assertIsNone(metrics.n64_order_after(WrappedUnread, 9, 103, metrics.ORDER_MOVE)[0])

        class Restarted(Cap):   # tick 51 repeats after a counter restart: the read at the press stands
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}} for s, t, o in ((100, 50, 3), (103, 51, 3), (106, 52, 6))]
            change_steps = {50: 99, 51: 200, 52: 105}
            change_reads = [(99, 50), (102, 51), (105, 52), (200, 51)]
        self.assertEqual(metrics.n64_order_after(Restarted, 9, 103, metrics.ORDER_MOVE)[0], 6)

        class Hidden(Cap):   # tick 52 after the press was never read: it could have held another order
            ticks = [{'step': s, 'tick': t, 'rows': {9: o}} for s, t, o in ((100, 50, 3), (103, 51, 3), (109, 53, 6))]
            change_reads = [(99, 50), (102, 51), (105, 52), (108, 53)]
        ok, seen = metrics.n64_order_after(Hidden, 9, 103, metrics.ORDER_MOVE)
        self.assertIsNone(ok)
        self.assertIn('a tick between the press and this read was not read', seen)

        class Replaced(Cap):   # the row is gone for a tick, then a replacement shows Move
            ticks = [{'step': s, 'tick': t, 'rows': r} for s, t, r in
                     ((100, 50, {9: 3}), (103, 51, {9: 3}), (106, 52, {}), (109, 53, {9: 6}))]
        ok, seen = metrics.n64_order_after(Replaced, 9, 103, metrics.ORDER_MOVE)
        self.assertIsNone(ok)
        self.assertIn('the subject row was gone or replaced before its order changed', seen)

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
