# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of the S4 parity slice: tap.bin decoding, the layout's
proven-field gate, golden bundles, GBA trace parsing and every comparator
verdict, on synthetic data only (no ROM, capture or GBA build needed)."""
import hashlib
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


if __name__ == '__main__':
    unittest.main()
