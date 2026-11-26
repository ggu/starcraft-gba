# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of the S4 parity slice: tap.bin decoding, the layout's
proven-field gate, golden bundles, GBA trace parsing and every comparator
verdict, on synthetic data only (no ROM, capture or GBA build needed)."""
import contextlib
import hashlib
import io
import json
import shutil
import pathlib
import struct
import sys
import tempfile
import unittest
import uuid

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import parity_decode  # noqa: E402
sys.path.insert(0, str(ROOT / 'experiments' / 'ares-headless'))
import capture_report  # noqa: E402
import sc64_parity  # noqa: E402
import sc64_parity_metrics as metrics  # noqa: E402

HUMAN = 1


def row(x, y, owner=HUMAN, typ=metrics.MARINE):
    data = bytearray(312)
    struct.pack_into('>I', data, 0x0C, 0x80100000)  # sprite pointer: live
    struct.pack_into('>HH', data, 0x28, x, y)
    data[0x4C] = owner
    struct.pack_into('>H', data, 0x64, typ)
    return bytes(data)


def write_capture(run, frames, pause_ticks=(), command_step=100, tamper_tick=None, step_shift_tick=None):
    """A capture directory: tap.bin with one tick per two frontend steps, a
    manifest and a tape whose '# mark command' falls at command_step."""
    run.mkdir(parents=True)
    out = bytearray(b'SC64TAP\0' + struct.pack('<4I', 1, 48, 0xCC, 314))
    for tick, units in enumerate(frames, start=1):
        step = 2 * tick + (1 if step_shift_tick is not None and tick >= step_shift_tick else 0)
        glob = bytearray(0xCC)
        gathered = 8 * (tick // 20)
        struct.pack_into('>I', glob, 4 * HUMAN, 50 + gathered)
        struct.pack_into('>I', glob, 0x90 + 4 * HUMAN, gathered)
        if tick == tamper_tick:
            glob[0xC0] ^= 1
        pause = 1 if tick in pause_ticks else 0
        rows = b''.join(struct.pack('>H', i) + row(*u) for i, u in sorted(units.items()))
        for phase, flags, data in ((0, 2, b''), (1, 3, rows)):
            out += struct.pack('<12I', step + phase, tick, phase, len(units), 0, pause, 4, 1, 0, 0x300000, flags,
                               len(units) if phase else 0)
            out += glob + data
    (run / 'tap.bin').write_bytes(bytes(out))
    tape = f'tap tick rec\n{command_step} 0 0 0\n# mark command\n4 1 0 0\n100 0 0 0\n'.encode()
    (run / 'tape.txt').write_bytes(tape)
    (run / 'manifest.json').write_text(json.dumps({
        'ok': True, 'binary_sha256': 'b' * 64, 'tape_sha256': hashlib.sha256(tape).hexdigest(), 'rom_sha256': 'r' * 64,
        'mode': 'jit', 'flash_input_sha256': None, 'run_id': str(uuid.uuid4()),
        'outputs_sha256': {'tap.bin': hashlib.sha256(bytes(out)).hexdigest()}}))


def marine_frames(ticks=200, start=60, speed=4, settle=40):
    """Five Marines; from tick `start` each walks right at `speed` px/tick for `settle` ticks."""
    frames = []
    for tick in range(1, ticks + 1):
        moved = max(0, min(tick - start, settle)) * speed
        frames.append({i: (100 + 4 * i + moved, 200 + 10 * i) for i in range(5)} | {9: (900, 900, 4, 37)})
    return frames


class LayoutGate(unittest.TestCase):
    def test_non_proven_field_cannot_gate(self):
        layout = parity_decode.Layout()
        layout.unit_field('x', gating=True)
        with self.assertRaises(parity_decode.LayoutError):
            layout.unit_field('order', gating=True)
        layout.unit_field('order', gating=False)
        with self.assertRaises(parity_decode.LayoutError):
            layout.bank_array('gas', gating=True)
        self.assertEqual(layout.bank_array('minerals_gathered', gating=True)['status'], 'proven')

    def test_every_field_has_a_status_and_evidence(self):
        doc = parity_decode.Layout().doc
        fields = list(doc['unit_fields'].values()) + list(doc['globals']['banks']['arrays'].values())
        for field in fields:
            self.assertIn(field['status'], ('proven', 'candidate', 'unknown'))
            self.assertTrue(field.get('evidence'))


class Decoder(unittest.TestCase):
    def test_follow_step_records_unpaused_clock_and_chain(self):
        with tempfile.TemporaryDirectory() as temp:
            run = pathlib.Path(temp) / 'a'
            write_capture(run, marine_frames(30), pause_ticks={5, 6})
            cap = parity_decode.Capture(run)
            self.assertEqual(len(cap.ticks), 30)
            self.assertEqual(cap.faults, {'dual_differ': 0, 'bad_links': 0, 'missing_follow': 0, 'tick_skips': 0})
            series = cap.series()
            self.assertEqual(len(series['frames']), 28)  # two paused ticks take no source frame
            self.assertEqual([f['sf'] for f in series['frames']][:5], [0, 1, 2, 3, 4])
            self.assertEqual(series['frames'][0]['units'][0], (metrics.MARINE, HUMAN, 100, 200))

    def test_bundle_needs_identical_chains(self):
        with tempfile.TemporaryDirectory() as temp:
            a, b, c = (pathlib.Path(temp) / n for n in 'abc')
            write_capture(a, marine_frames(40))
            write_capture(b, marine_frames(40))
            write_capture(c, marine_frames(40), tamper_tick=17)
            A, B, C = (parity_decode.Capture(r) for r in (a, b, c))
            self.assertEqual(parity_decode.same_bundle(A, B), [])
            problems = parity_decode.same_bundle(A, C)
            self.assertTrue(any('tick 17' in p for p in problems), problems)

    def test_copies_edits_and_timing_metadata_break_a_bundle(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            write_capture(temp / 'a', marine_frames(40))
            shutil.copytree(temp / 'a', temp / 'copy')
            A, copy = parity_decode.Capture(temp / 'a'), parity_decode.Capture(temp / 'copy')
            self.assertTrue(any('same run id' in p for p in parity_decode.same_bundle(A, copy)))
            write_capture(temp / 'edited', marine_frames(40))
            (temp / 'edited' / 'tape.txt').write_text('500 0 0 0\n# mark command\n')
            self.assertTrue(any('tape.txt' in p for p in parity_decode.same_bundle(A, parity_decode.Capture(temp / 'edited'))))
            # Same rows and globals, different pause state or step: different chains.
            write_capture(temp / 'paused', marine_frames(40), pause_ticks={9})
            write_capture(temp / 'shifted', marine_frames(40), step_shift_tick=30)
            for name in ('paused', 'shifted'):
                problems = parity_decode.same_bundle(A, parity_decode.Capture(temp / name))
                self.assertTrue(any('hash chains diverge' in p for p in problems), (name, problems))


    def test_loaded_state_inputs_must_match(self):
        """Captures that load a state (S5) are one bundle only with the same state bytes."""
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            caps = {}
            for name, payload in (('a', b'state-one'), ('b', b'state-one'), ('c', b'state-two'), ('d', b'state-one')):
                write_capture(temp / name, marine_frames(40))
                (temp / name / 'input').mkdir()
                (temp / name / 'input' / 'lobby.state').write_bytes(payload)
                manifest = json.loads((temp / name / 'manifest.json').read_text())
                manifest['state_input_sha256'] = {'lobby.state': hashlib.sha256(b'state-one' if name == 'd' else payload).hexdigest()}
                (temp / name / 'manifest.json').write_text(json.dumps(manifest))
                caps[name] = parity_decode.Capture(temp / name)
            self.assertEqual(parity_decode.same_bundle(caps['a'], caps['b']), [])
            self.assertTrue(any('state_input_sha256' in p for p in parity_decode.same_bundle(caps['a'], caps['c'])))
            (temp / 'd' / 'input' / 'lobby.state').write_bytes(b'state-edited')
            caps['d'] = parity_decode.Capture(temp / 'd')
            self.assertTrue(any('state input lobby.state' in p for p in parity_decode.same_bundle(caps['a'], caps['d'])))
            # A manifest from before S5 has no state inputs and still pairs with an empty one.
            write_capture(temp / 'e', marine_frames(40))
            write_capture(temp / 'f', marine_frames(40))
            self.assertEqual(parity_decode.same_bundle(parity_decode.Capture(temp / 'e'), parity_decode.Capture(temp / 'f')), [])


class Golden(unittest.TestCase):
    def test_move_golden(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            for name in 'ab':
                write_capture(temp / name, marine_frames(), command_step=100)
            out, golden = parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home',
                                                     parity_decode.Layout())
            self.assertTrue(out.exists())
            values = {o['check']: o for o in golden['observations']}
            self.assertAlmostEqual(values['move.cruise_speed']['value'], 4.0)
            self.assertEqual(values['move.arrival_last']['value'], 39)
            self.assertEqual(values['move.start_latency']['value'], 12)  # last read at or before the mark: tick 49 (sf 48); first move at sf 60
            self.assertFalse(values['move.start_latency']['gating'])
            for obs in golden['observations']:
                self.assertEqual(obs['n_runs'], 2)
                self.assertEqual(obs['provenance'], 'n64-ram')
                self.assertTrue(set(obs['layout_fields']) <= {'unit.type', 'unit.owner', 'unit.x', 'unit.y', 'unit.sprite',
                                                              'clock.tick_counter', 'clock.pause_halfword'})
            self.assertEqual(golden['gba']['units'][0], [50, 100])

    def test_unproven_dependency_blocks_a_gating_golden(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            for name in 'ab':
                write_capture(temp / name, marine_frames())
            doc = json.loads(parity_decode.LAYOUT.read_text())
            doc['clock']['pause_halfword']['status'] = 'candidate'
            weak = temp / 'weak.json'
            weak.write_text(json.dumps(doc))
            with self.assertRaises(parity_decode.LayoutError):
                parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home', parity_decode.Layout(weak))
            # A golden judged under a layout that no longer proves a field cannot gate.
            _, golden = parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home',
                                                   parity_decode.Layout())
            rows = sc64_parity.judge('t1-move', golden, {'move.cruise_speed': 99.0}, {}, {}, parity_decode.Layout(weak))
            self.assertEqual(rows[0]['verdict'], 'REPORT')

    def test_one_capture_is_not_golden(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            write_capture(temp / 'a', marine_frames())
            with self.assertRaises(SystemExit):
                parity_decode.write_golden('t1-move', [temp / 'a'], temp / 'home', parity_decode.Layout())


class Comparator(unittest.TestCase):
    GOLDEN = {'observations': [
        {'check': 'x.a', 'subject': 's', 'value': 10.0, 'lo': 9.0, 'hi': 11.0, 'gating': True, 'n_runs': 2,
         'provenance': 'n64-ram', 'tolerance': '1'},
        {'check': 'x.b', 'subject': 's', 'value': 10.0, 'lo': 9.0, 'hi': 11.0, 'gating': False, 'n_runs': 2,
         'provenance': 'n64-ram', 'tolerance': '1'}]}

    def verdict(self, value, baseline=None):
        rows = sc64_parity.judge('demo', self.GOLDEN, {'x.a': value, 'x.b': 99.0}, {}, baseline or {})
        self.assertEqual(rows[1]['verdict'], 'REPORT')
        return rows[0]['verdict']

    def test_verdicts(self):
        self.assertEqual(self.verdict(10.5), 'PASS')
        self.assertEqual(self.verdict(20.0), 'REGRESSION')
        self.assertEqual(self.verdict(20.0, {'demo:x.a:s': {'gba': 20.0}}), 'KNOWN')
        self.assertEqual(self.verdict(20.5, {'demo:x.a:s': {'gba': 20.0}}), 'KNOWN')   # within half a band
        self.assertEqual(self.verdict(25.0, {'demo:x.a:s': {'gba': 20.0}}), 'REGRESSION')
        self.assertEqual(self.verdict(10.0, {'demo:x.a:s': {'gba': 20.0}}), 'IMPROVED')

    def test_exit_codes(self):
        skip = [{'id': 'scenarios', 'status': 'SKIP'}]
        self.assertEqual(sc64_parity.exit_code(skip, {}, private_inputs=False), 0)
        self.assertEqual(sc64_parity.exit_code(skip, {}, private_inputs=True), 1)
        ran = [{'id': 'demo', 'status': 'RUN'}]
        self.assertEqual(sc64_parity.exit_code(ran, {'KNOWN': 1, 'STALE': 1}, True), 0)
        self.assertEqual(sc64_parity.exit_code(ran, {'REGRESSION': 1}, True), 1)
        self.assertEqual(sc64_parity.exit_code(ran, {'REGRESSION': 1}, True, accept=True), 0)
        self.assertEqual(sc64_parity.exit_code(ran + [{'id': 'x', 'status': 'ERROR'}], {}, True), 1)

    def test_accept_refuses_a_missing_value(self):
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 'baseline.json'
            results = [{'id': 'demo', 'status': 'RUN', 'rows': [
                {'key': 'demo:x.a:s', 'gba': None, 'gating': True, 'verdict': 'REGRESSION'}]}]
            with self.assertRaises(ValueError):
                sc64_parity.write_baseline(path, results, {})
            self.assertFalse(path.exists())

    def test_income_counts_merged_credits_as_trips(self):
        curve = [0, 0, 8, 8, 24, 24, 32]  # 8, then two trips in one frame, then 8
        r = metrics.income_measure(curve, 6)
        self.assertEqual((r['trip_yield'], r['credits'], r['whole_trips'], r['first_delivery']), (8, 3, True, 2))

    def test_merged_credits_use_their_common_divisor(self):
        r = metrics.income_measure([0, 16, 16, 40], 3)  # two trips, then three, in single frames
        self.assertEqual((r['trip_yield'], r['whole_trips']), (8, True))

    def test_four_movers_are_not_the_five_marine_scenario(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            frames = marine_frames()
            for f in frames:
                f[4] = (116, 240)  # the fifth Marine stays put
            for name in 'ab':
                write_capture(temp / name, frames)
            with self.assertRaises(SystemExit):
                parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home', parity_decode.Layout())

    def test_gba_command_boundary(self):
        self.assertTrue(metrics.gba_boundary_ok(5))
        self.assertFalse(metrics.gba_boundary_ok(2))

    def test_baseline_rewrite_keeps_unrun_scenarios(self):
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 'baseline.json'
            old = {'other:x.a:s': {'key': 'other:x.a:s', 'gba': 1}, 'demo:gone:s': {'key': 'demo:gone:s', 'gba': 2}}
            results = [{'id': 'demo', 'status': 'RUN', 'rows': [
                {'key': 'demo:x.a:s', 'gba': 20.0, 'gating': True, 'verdict': 'REGRESSION'},
                {'key': 'demo:x.c:s', 'gba': 10.0, 'gating': True, 'verdict': 'PASS'}]}]
            sc64_parity.write_baseline(path, results, old)
            keys = [line['key'] for line in json.loads(path.read_text())['known']]
            self.assertEqual(keys, ['demo:x.a:s', 'other:x.a:s'])

    def test_gba_trace_series(self):
        with tempfile.TemporaryDirectory() as temp:
            trace = pathlib.Path(temp) / 't.trace'
            trace.write_text('#sc64-parity-trace 1\nH time=steps\n'
                             'S 1 0 1 1 1 0 0 0 0 -1 0\nU 3 1 0 0 1 50 60 40 40 0 -1 0 50 60 -1 0 0 -1 0 0 0 0 0\nP 0 50 0 8 18\n'
                             'S 2 1 2 2 1 0 0 0 0 -1 0\nU 3 1 0 0 1 51 60 40 40 0 -1 0 50 60 -1 0 0 -1 0 0 0 0 0\n'
                             'S 3 2 3 3 1 0 0 0 0 -1 0\nP 0 58 0 8 18\nZ overflow=0 write_errors=0\n')
            series = sc64_parity.gba_series(trace)
            self.assertEqual([f['sf'] for f in series['frames']], [0, 1, 2])
            self.assertEqual(series['frames'][1]['units'][3], (0, 1, 102, 120))
            self.assertEqual(series['frames'][2]['banks'][0], (58, None))
            trace.write_text(trace.read_text().replace('overflow=0', 'overflow=1'))
            with self.assertRaises(ValueError):
                sc64_parity.gba_series(trace)

    def test_tape_marks(self):
        with tempfile.TemporaryDirectory() as temp:
            tape = pathlib.Path(temp) / 'tape.txt'
            tape.write_text('tap tick rec\n700 0 0 0\nshot a\n10 2 0 0\n# mark command\n4 1 0 0\n')
            self.assertEqual(metrics.tape_commands(tape), {'command': 710})


class TapeMarks(unittest.TestCase):
    def test_marks_follow_executed_commands(self):
        """A mark after `load` (renumbered steps) and an `until` of unknown length lands where
        the next command actually began."""
        with tempfile.TemporaryDirectory() as temp:
            run = pathlib.Path(temp)
            (run / 'tape.txt').write_text('load a.state 5000\nwatch 800F0C20 2\n10 0 0 0\n'
                                          'until 800F0C20 2 eq 1 3000 0 0 0\n# mark box\n3 2 0 0\n20 0 0 0\n# mark end\n')
            (run / 'commands.csv').write_text('begin,steps,mask,x,y\n5000,10,0,0,0\n5010,917,0,0,0\n5927,3,2,0,0\n5930,20,0,0,0\n')
            self.assertEqual(metrics.tape_commands(run / 'tape.txt'), {'box': 5927, 'end': 5950})
            (run / 'commands.csv').write_text('begin,steps,mask,x,y\n5000,10,0,0,0\n5010,917,0,0,0\n5927,3,2,0,0\n')
            with self.assertRaises(SystemExit):   # one executed row missing
                metrics.tape_commands(run / 'tape.txt')
            (run / 'commands.csv').unlink()
            with self.assertRaises(SystemExit):
                metrics.tape_commands(run / 'tape.txt')
            # The frontend accepts a signed count, so the summing fallback must too.
            (run / 'tape.txt').write_text('+10 0 0 0\n# mark a\n5 1 0 0\n')
            self.assertEqual(metrics.tape_commands(run / 'tape.txt'), {'a': 10})

    def test_commands_log_must_match_the_manifest(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            write_capture(temp / 'a', marine_frames(40))
            (temp / 'a' / 'commands.csv').write_text('begin,steps,mask,x,y\n0,100,0,0,0\n')
            problems = parity_decode.Capture(temp / 'a').verified
            self.assertTrue(any('commands.csv' in p for p in problems), problems)
            # A log the manifest declares may not be deleted to fall back to inferred marks.
            manifest = json.loads((temp / 'a' / 'manifest.json').read_text())
            manifest['outputs_sha256']['commands.csv'] = hashlib.sha256((temp / 'a' / 'commands.csv').read_bytes()).hexdigest()
            (temp / 'a' / 'manifest.json').write_text(json.dumps(manifest))
            (temp / 'a' / 'commands.csv').unlink()
            problems = parity_decode.Capture(temp / 'a').verified
            self.assertTrue(any('recorded in the manifest but missing' in p for p in problems), problems)

    def test_tape_lines_follow_the_frontend(self):
        """LF-only lines, '#' comments (a CR inside one is not a quit), nothing after quit."""
        text = '# note\rquit\nload a.state\n10 0 0 0\nquit\nload b.state\n'
        self.assertEqual(list(metrics.frontend_lines(text)), [['load', 'a.state'], ['10', '0', '0', '0']])
        # File entry points read bytes, so a bare CR in a comment never becomes a line break.
        with tempfile.TemporaryDirectory() as temp:
            run = pathlib.Path(temp)
            (run / 'tape.txt').write_bytes(b'10 0 0 0\n# note\rquit\n# mark a\n5 1 0 0\n')
            self.assertEqual(metrics.tape_commands(run / 'tape.txt'), {'a': 10})

    def test_marks_stop_at_quit(self):
        with tempfile.TemporaryDirectory() as temp:
            run = pathlib.Path(temp)
            (run / 'tape.txt').write_text('10 0 0 0\n# mark a\n5 1 0 0\nquit\n7 0 0 0\n# mark late\n')
            (run / 'commands.csv').write_text('begin,steps,mask,x,y\n0,10,0,0,0\n10,5,1,0,0\n')
            self.assertEqual(metrics.tape_commands(run / 'tape.txt'), {'a': 10})

    def test_a_load_needs_a_recorded_digest(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            write_capture(temp / 'a', marine_frames(40))
            tape = (temp / 'a' / 'tape.txt')
            tape.write_bytes(b'load lobby.state 3900\n' + tape.read_bytes())
            manifest = json.loads((temp / 'a' / 'manifest.json').read_text())
            manifest['tape_sha256'] = hashlib.sha256(tape.read_bytes()).hexdigest()
            (temp / 'a' / 'manifest.json').write_text(json.dumps(manifest))
            problems = parity_decode.Capture(temp / 'a').verified
            self.assertTrue(any('records no digest' in p for p in problems), problems)


class CaptureReport(unittest.TestCase):
    """The spike report helpers never count missing evidence as agreement."""

    @staticmethod
    def run_dir(root, name, checkpoints, ticks=(), run_id=None, raw=b''):
        run = root / name
        run.mkdir()
        (run / 'checkpoints.csv').write_text('step,tick,sha256,cached_lines,dual_hits,dual_differ\n' +
                                             ''.join(f'{s},0,{h},0,0,0\n' for s, h, _ in checkpoints))
        (run / 'regions.csv').write_text(''.join(f'{s},' + ','.join(blocks) + '\n' for s, _, blocks in checkpoints if blocks))
        header = 'step,tick,phase,same,seconds,pause,selector,running,countdown,table,live,bad_links,asymmetric_links,' \
                 'dual_hits,dual_differ,raw_differs,rows_sha256,globals_sha256\n'
        (run / 'ticks.csv').write_text(header + ''.join(f'{t},{t},1,1,0,0,4,1,0,0,0,0,0,0,0,0,r{t},g{t}\n' for t in ticks))
        (run / 'ticks.raw').write_bytes(raw)
        CaptureReport.seal(run, run_id or str(uuid.uuid4()))
        return run

    @staticmethod
    def seal(run, run_id):
        """Record ticks.csv in the manifest, as capture.py does."""
        digest = hashlib.sha256((run / 'ticks.csv').read_bytes()).hexdigest()
        (run / 'manifest.json').write_text(json.dumps({'run_id': run_id, 'ok': True, 'outputs_sha256': {'ticks.csv': digest}}))

    def report(self, fn, *args):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            fn(*args)
        return out.getvalue()

    def test_compare_needs_every_region(self):
        full = ['0' * 16] * capture_report.REGION_BLOCKS
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            a = self.run_dir(temp, 'a', [('1000', 'x', full), ('2000', 'y', full)])
            b = self.run_dir(temp, 'b', [('1000', 'z', full[:5]), ('2000', 'w', [])])
            text = self.report(capture_report.compare, a, b, set())
            self.assertIn('state_regions_equal=0', text)
            self.assertIn('checkpoints_without_all_regions=2', text)

    def test_s8_pair_refuses_copies_and_partial_coverage(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            a = self.run_dir(temp, 'a', [], ticks=range(1, 6), run_id='same')
            b = self.run_dir(temp, 'b', [], ticks=range(1, 6), run_id='same')
            self.assertIn('no verdict', self.report(capture_report.s8_pair, a, b))
            c = self.run_dir(temp, 'c', [], ticks=range(1, 4))
            self.assertIn('coverage differs', self.report(capture_report.s8_pair, a, c))
            e = self.run_dir(temp, 'e', [], ticks=range(1, 6))
            with open(e / 'ticks.csv', 'a') as handle:
                handle.write('6,6,0,0,0,0,4,1,0,0,0,0,0,0,0,0,r6,g6\n')   # a change-step read only in e
            self.seal(e, 'e')
            self.assertIn('change-step records: 0/1 shared=0 only_in_one_run=1', self.report(capture_report.s8_pair, a, e))
            # A repeated tick: each follow-up belongs to the change before it.
            r1, r2 = self.run_dir(temp, 'r1', []), self.run_dir(temp, 'r2', [])
            header = (r1 / 'ticks.csv').read_text()
            row = lambda tick, phase, h: f'{tick},{tick},{phase},0,0,0,4,1,0,0,0,0,0,0,0,0,{h},g\n'
            (r1 / 'ticks.csv').write_text(header + row(5, 0, 'a') + row(5, 1, 'x') + row(5, 0, 'b'))
            (r2 / 'ticks.csv').write_text(header + row(5, 0, 'a') + row(5, 0, 'b') + row(5, 1, 'x'))
            self.seal(r1, 'r1'); self.seal(r2, 'r2')
            text = self.report(capture_report.s8_pair, r1, r2)
            self.assertIn('follow-up records: 1/1 shared=0 only_in_one_run=2', text)
            self.assertIn('coverage differs', text)
            # The same occurrences in a different order are not the same coverage.
            o1, o2 = self.run_dir(temp, 'o1', []), self.run_dir(temp, 'o2', [])
            (o1 / 'ticks.csv').write_text(header + row(5, 0, 'a') + row(6, 0, 'a') + row(5, 0, 'a'))
            (o2 / 'ticks.csv').write_text(header + row(5, 0, 'a') + row(5, 0, 'a') + row(6, 0, 'a'))
            self.seal(o1, 'o1'); self.seal(o2, 'o2')
            self.assertIn('coverage differs', self.report(capture_report.s8_pair, o1, o2))
            # Same change-step logs and no follow-up reads: not called a coverage difference.
            (o2 / 'ticks.csv').write_bytes((o1 / 'ticks.csv').read_bytes()); self.seal(o2, 'o2')
            text = self.report(capture_report.s8_pair, o1, o2)
            self.assertIn('a phase has no comparable unit rows', text)
            self.assertNotIn('coverage differs', text)
            # A unit table readable in one run only is a coverage difference, not a row mismatch.
            t1, t2 = self.run_dir(temp, 't1', []), self.run_dir(temp, 't2', [])
            line = lambda table: f'7,7,1,1,0,0,4,1,0,{table},3,0,0,0,0,0,r7,g7\n'
            (t1 / 'ticks.csv').write_text(header + line(3154736)); (t2 / 'ticks.csv').write_text(header + line(0))
            self.seal(t1, 't1'); self.seal(t2, 't2')
            text = self.report(capture_report.s8_pair, t1, t2)
            self.assertIn('table_in_one_run_only=1', text)
            self.assertIn('coverage differs', text)
            # A log copied over another run's, with that run's own manifest kept: no verdict.
            (o2 / 'ticks.csv').write_bytes((r1 / 'ticks.csv').read_bytes())
            self.assertIn('does not match its manifest', self.report(capture_report.s8_pair, o1, o2))
            (o2 / 'ticks.csv').unlink()
            self.assertIn('tick log is missing', self.report(capture_report.s8_pair, o1, o2))
            d = self.run_dir(temp, 'd', [], ticks=range(1, 6))
            (d / 'manifest.json').write_text(json.dumps({'outputs_sha256': json.loads((d / 'manifest.json').read_text())['outputs_sha256']}))
            self.assertIn('no verdict', self.report(capture_report.s8_pair, c, d))

    def test_watch_counts_only_value_changes(self):
        with tempfile.TemporaryDirectory() as temp:
            run = pathlib.Path(temp)
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n1,0,800d13e4,4,4\n9,0,800d13e4,4,4\n12,0,800d13e4,4,6\n')
            self.assertIn('changes=1 distinct=2', self.report(capture_report.watch_summary, run))

    def test_sweep_skips_an_empty_first_sample(self):
        def record(step, tick, rows):
            body = b''.join(struct.pack('>H', i) + r for i, r in enumerate(rows))
            return struct.pack('<4I', step, tick, 1, len(rows)) + body
        start = [row(10, 10, 0, 7), row(20, 20, 1, 41)]
        raw = record(1, 1, []) + record(2, 2, start) + record(3, 3, start + [row(30, 30, 0, 111)])
        with tempfile.TemporaryDirectory() as temp:
            run = self.run_dir(pathlib.Path(temp), 'a', [], raw=raw)
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\n')
            # Tick log: rows live from step 2; a stale seed at step 0 and the launch seed at step 2.
            header = (run / 'ticks.csv').read_text().split('\n')[0]
            readable = ''.join(f'{n},{n},1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' for n in range(2, 62))
            (run / 'ticks.csv').write_text(header + '\n1,1,1,1,0,0,4,1,0,0,0,0,0,0,0,0,r,g\n' + readable)
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n0,0,800eff20,4,111\n2,0,800eff20,4,222\n5,0,800eff20,4,333\n')
            text = self.report(capture_report.sweep, [run], 10, '800eff20')
            self.assertIn('seed=546', text)   # 0x222, not the stale 0x111 or the later 0x333
            self.assertIn('unit samples ending at tick 3', text)
            self.assertIn('through tick 3', text)
            # A pause in raw logging inside the window is missing samples, not a wide stride.
            gapped = record(4, 4, start) + record(5, 5, start) + record(60, 60, start) + record(61, 61, start)
            (run / 'ticks.raw').write_bytes(raw + gapped)
            self.assertIn('a missing unit sample at tick 6', self.report(capture_report.sweep, [run], 50, '800eff20'))
            # A gap that starts after the cutoff leaves a covered run eligible.
            self.assertNotIn('missing unit sample', self.report(capture_report.sweep, [run], 5, '800eff20'))
            # No --ticks: the gap check still runs through the run's own last sample.
            self.assertIn('a missing unit sample at tick 6', self.report(capture_report.sweep, [run], 1 << 30, '800eff20'))
            # A truncated record after a covered window: the run is reported, with its cutoff.
            (run / 'ticks.raw').write_bytes(raw + record(4, 4, start)[:-5])
            self.assertIn('a truncated record', self.report(capture_report.sweep, [run], 10, '800eff20'))
            (run / 'ticks.raw').write_bytes(raw + record(4, 4, start)[:9])   # a partial header
            self.assertIn('a truncated record', self.report(capture_report.sweep, [run], 10, '800eff20'))
            # Two launches under one tap: the tick counter restarts, so the run is refused.
            (run / 'ticks.raw').write_bytes(raw + record(4, 2, start) + record(5, 3, start))
            self.assertIn('a tick counter reset', self.report(capture_report.sweep, [run], 3, '800eff20'))
            # A sample whose read found no unit table is not coverage.
            (run / 'ticks.raw').write_bytes(raw + record(4, 4, start))
            csv_text = (run / 'ticks.csv').read_text()
            (run / 'ticks.csv').write_text(csv_text.replace('\n3,3,1,1,0,0,4,1,0,1,', '\n3,3,1,1,0,0,4,1,0,0,'))
            self.assertIn('no readable unit table at tick 3', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            # The tick log must reach an explicit cutoff, and a reset in the log refuses the run.
            (run / 'ticks.raw').write_bytes(raw + record(4, 4, start))
            self.assertIn('unit samples ending', self.report(capture_report.sweep, [run], 70, '800eff20'))
            reset_log = csv_text.split('\n')[0] + '\n' + ''.join(
                f'{st},{t},1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' for st, t in ((1, 1), (2, 2), (3, 3), (4, 0), (5, 1), (6, 2)))
            (run / 'ticks.csv').write_text(reset_log)
            self.assertIn('a tick counter reset to 0', self.report(capture_report.sweep, [run], 4, '800eff20'))
            # A reset after the cutoff still means two matches in one capture: refused too.
            self.assertIn('a tick counter reset to 0', self.report(capture_report.sweep, [run], 2, '800eff20'))
            # Empty records before the first live one, at the same tick or past the cutoff, are
            # outside the match and change nothing.
            (run / 'ticks.csv').write_text(csv_text)
            (run / 'ticks.raw').write_bytes(record(0, 90, []) + raw + record(4, 4, start))
            text = self.report(capture_report.sweep, [run], 3, '800eff20')
            self.assertNotIn('left out of the totals', text)
            self.assertIn('owner 0: 111@3', text)
            # A first live sample after the cutoff: that run is skipped, the next is still reported.
            late = self.run_dir(pathlib.Path(temp), 'late', [], raw=record(24, 24, start))
            (late / 'tape.txt').write_text('tap tick raw\n10 0 0 0\n')
            text = self.report(capture_report.sweep, [late, run], 3, '800eff20')
            self.assertIn('sweep late: no sample with live rows by tick 3', text)
            self.assertIn('sweep a: seed=', text)
            # --opening skips an empty pre-match record past the cutoff too.
            (run / 'ticks.raw').write_bytes(record(0, 90, []) + raw)
            self.assertIn('owner 0: 7@2 111@3', self.report(capture_report.opening, run, 3))
            # A second launch in the same capture is never merged into the opening.
            (run / 'ticks.raw').write_bytes(raw + record(9, 1, [row(5, 5, 2, 64)]))
            opened = self.report(capture_report.opening, run, 50)
            self.assertIn('restarts', opened)
            self.assertNotIn('owner 2', opened)
            # A reset between raw samples, visible only in the tick log, also stops the opening.
            (run / 'ticks.raw').write_bytes(raw + record(9, 24, [row(5, 5, 2, 64)]))
            hidden = csv_text.split('\n')[0] + '\n' + ''.join(
                f'{st},{t},1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' for st, t in ((1, 1), (2, 2), (3, 3), (5, 40), (6, 0), (9, 24)))
            (run / 'ticks.csv').write_text(hidden)
            opened = self.report(capture_report.opening, run, 50)
            self.assertIn('restarts', opened)
            self.assertNotIn('owner 2', opened)
            # A truncated record at the reset step does not discard the first match.
            (run / 'ticks.raw').write_bytes(raw + record(6, 0, start)[:-5])
            (run / 'ticks.csv').write_text(hidden)
            self.assertIn('owner 0: 7@2 111@3', self.report(capture_report.opening, run, 50))
            (run / 'ticks.csv').write_text(csv_text)
            # A tap turned off and on again could hide a relaunch: no report.
            (run / 'ticks.raw').write_bytes(raw)
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\ntap off\n10 0 0 0\ntap tick raw\n10 0 0 0\n')
            self.assertIn('no report', self.report(capture_report.opening, run, 50))
            # An initial or trailing `tap off` leaves no gap between active taps.
            (run / 'tape.txt').write_text('tap off\ntap tick raw\n10 0 0 0\ntap off\ntap off\n')
            self.assertNotIn('no report', self.report(capture_report.opening, run, 50))
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\n')
            # A header cut off after the step field, at the reset step, keeps the first match.
            (run / 'ticks.raw').write_bytes(raw + struct.pack('<I', 6))
            (run / 'ticks.csv').write_text(hidden)
            self.assertIn('owner 0: 7@2 111@3', self.report(capture_report.opening, run, 50))
            (run / 'ticks.csv').write_text(csv_text)
            (run / 'ticks.raw').write_bytes(raw)
            # A corrupt row count is refused before the body is read.
            (run / 'ticks.raw').write_bytes(struct.pack('<4I', 1, 1, 1, 100000))
            with self.assertRaises(ValueError):
                list(capture_report.raw_records(run / 'ticks.raw'))
            (run / 'ticks.raw').write_bytes(raw)
            # Reads before the match (an earlier tap reaching tick 300) cannot satisfy the cutoff.
            early = csv_text.split('\n')[0] + '\n0,300,1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' + ''.join(
                f'{st},{st},1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' for st in range(2, 6))
            (run / 'ticks.csv').write_text(early)
            self.assertIn('more than one match in the capture', self.report(capture_report.sweep, [run], 65, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            # Tap settings changed during the run: coverage cannot be judged.
            (run / 'ticks.raw').write_bytes(raw)
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\ntap tick raw 25\n10 0 0 0\n')
            self.assertIn('tap settings that change', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.raw').write_bytes(raw)
            # A missing raw file is reported and skipped, not fatal.
            (run / 'ticks.raw').unlink()
            self.assertIn('no ticks.raw', self.report(capture_report.sweep, [run], 10, '800eff20'))
            (run / 'ticks.raw').write_bytes(raw)
            # A half-width sample or a tape that turns watches off gives an unknown seed.
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n2,0,800eff20,2,1\n')
            self.assertIn('seed=None', self.report(capture_report.sweep, [run], 10, '800eff20'))
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n2,0,800eff20,4,222\n')
            (run / 'tape.txt').write_text('watch 800EFF20 4\nwatch off\n10 0 0 0\n')
            self.assertIn('seed=None', self.report(capture_report.sweep, [run], 10, '800eff20'))
            self.assertIn('owner 0: 111@3', text)
            self.assertNotIn('7@2', text)


if __name__ == '__main__':
    unittest.main()
