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
import capture  # noqa: E402
import sc64_parity  # noqa: E402
import sc64_parity_metrics as metrics  # noqa: E402

HUMAN = 1


def H(label):
    """A checkpoint hash for a test label: equal labels give equal hashes."""
    return hashlib.sha256(label.encode()).hexdigest()


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
            layout.unit_field('shields', gating=True)
        layout.unit_field('shields', gating=False)
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
            # A loaded state needs a recorded producer (run id and binary) for golden evidence.
            (temp / 'a' / 'tape.txt').write_bytes(b'load lobby.state 3900\n' + (temp / 'a' / 'tape.txt').read_bytes())
            manifest = json.loads((temp / 'a' / 'manifest.json').read_text())
            manifest['tape_sha256'] = hashlib.sha256((temp / 'a' / 'tape.txt').read_bytes()).hexdigest()
            for bad in ({'run_id': None, 'binary_sha256': 'c' * 64}, {'run_id': True, 'binary_sha256': True},
                        {'run_id': 'w', 'binary_sha256': 'not-a-digest'}, None, 'x'):
                manifest['state_input_source'] = {'lobby.state': bad}
                (temp / 'a' / 'manifest.json').write_text(json.dumps(manifest))
                problems = parity_decode.Capture(temp / 'a').verified
                self.assertTrue(any('no recorded producer' in p for p in problems), (bad, problems))
            # A run that bypassed the producer check is never part of a bundle.
            manifest = json.loads((temp / 'b' / 'manifest.json').read_text())
            manifest['unrecorded_state'] = True
            (temp / 'b' / 'manifest.json').write_text(json.dumps(manifest))
            problems = parity_decode.same_bundle(caps['a'], parity_decode.Capture(temp / 'b'))
            self.assertTrue(any('--unrecorded-state' in p for p in problems), problems)
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
            # It fails rather than quietly reporting, unless recording work allows stale goldens.
            rows = sc64_parity.judge('t1-move', golden, {'move.cruise_speed': 99.0}, {}, {}, parity_decode.Layout(weak))
            self.assertEqual(rows[0]['verdict'], 'STALE-GOLDEN')
            self.assertIn('no longer proves clock.pause_halfword', rows[0]['why'])
            rows = sc64_parity.judge('t1-move', golden, {'move.cruise_speed': 99.0}, {}, {}, parity_decode.Layout(weak),
                                     allow_stale=True)
            self.assertEqual(rows[0]['verdict'], 'REPORT')

    def test_layout_edits_keep_or_stale_a_golden_per_field(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            for name in 'ab':
                write_capture(temp / name, marine_frames())
            _, golden = parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home',
                                                   parity_decode.Layout())
            self.assertTrue(all(len(o['layout_fields_sha256']) == 64 for o in golden['observations']))
            regression = {'move.cruise_speed': 99.0}

            def judged(doc):
                path = temp / f'layout-{len(list(temp.glob("layout-*")))}.json'
                path.write_text(json.dumps(doc))
                return {r['check']: r for r in sc64_parity.judge('t1-move', golden, regression, {}, {},
                                                                 parity_decode.Layout(path))}
            doc = json.loads(parity_decode.LAYOUT.read_text())
            # Additive: a new field, a reworded evidence line and a promotion elsewhere.
            doc['unit_fields']['brand_new'] = {'offset': '0x99', 'type': 'u8', 'status': 'candidate', 'evidence': 'x'}
            doc['unit_fields']['x']['evidence'] = 'reworded'
            doc['unit_fields']['shields']['status'] = 'proven'
            rows = judged(doc)
            self.assertEqual(rows['move.cruise_speed']['verdict'], 'REGRESSION')   # still gating, still caught
            # A field the golden reads moves: every observation reading it is stale and fails.
            doc['unit_fields']['x']['offset'] = '0x2C'
            rows = judged(doc)
            self.assertEqual(rows['move.cruise_speed']['verdict'], 'STALE-GOLDEN')
            self.assertIn('parity_decode.py golden', rows['move.cruise_speed']['why'])
            self.assertIn('STALE-GOLDEN', sc64_parity.FAILING)
            path = temp / 'moved.json'
            path.write_text(json.dumps(doc))
            loose = sc64_parity.judge('t1-move', golden, regression, {}, {}, parity_decode.Layout(path), allow_stale=True)
            self.assertEqual({r['verdict'] for r in loose}, {'REPORT'})
            # A golden from before per-field digests falls back to the whole layout.
            legacy = json.loads(json.dumps(golden))
            for obs in legacy['observations']:
                del obs['layout_fields_sha256']
            rows = sc64_parity.judge('t1-move', legacy, regression, {}, {}, parity_decode.Layout(path))
            self.assertEqual(rows[0]['verdict'], 'STALE-GOLDEN')
            rows = sc64_parity.judge('t1-move', legacy, regression, {}, {}, parity_decode.Layout())
            self.assertEqual(rows[0]['verdict'], 'REGRESSION')

    def test_reads_join_the_digest_and_recorded_addresses_are_fixed(self):
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            for name in 'ab':
                write_capture(temp / name, marine_frames())
            _, golden = parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home',
                                                   parity_decode.Layout())
            reads = golden['observations'][0]['layout_reads']
            self.assertTrue({'unit.x', 'unit.y', 'unit.type'} <= set(reads))
            doc = json.loads(parity_decode.LAYOUT.read_text())
            doc['clock']['pause_halfword']['address'] = '0x800F0C24'
            moved = temp / 'moved.json'
            moved.write_text(json.dumps(doc))
            with self.assertRaises(parity_decode.LayoutError):   # tap.bin recorded the old address
                parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home2', parity_decode.Layout(moved))
            rows = sc64_parity.judge('t1-move', golden, {'move.cruise_speed': 4.0}, {}, {}, parity_decode.Layout(moved))
            self.assertEqual(rows[0]['verdict'], 'STALE-GOLDEN')
            # Same addresses, another width or liveness rule: tap.bin still holds the old reads.
            for edit in (lambda d: d['clock']['pause_halfword'].update(type='u32'),
                         lambda d: d['unit_table'].update(live_rule='rows with nonzero hit points'),
                         lambda d: d['clock'].update(record_rule='the change-step read')):
                doc = json.loads(parity_decode.LAYOUT.read_text())
                edit(doc)
                other = temp / 'other.json'
                other.write_text(json.dumps(doc))
                with self.assertRaises(parity_decode.LayoutError):
                    parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home3', parity_decode.Layout(other))
            # Hex spelling does not matter.
            doc = json.loads(parity_decode.LAYOUT.read_text())
            doc['clock']['pause_halfword']['address'] = '0x800f0c20'
            lower = temp / 'lower.json'
            lower.write_text(json.dumps(doc))
            parity_decode.write_golden('t1-move', [temp / 'a', temp / 'b'], temp / 'home4', parity_decode.Layout(lower))
            # A per-field golden from before layout_reads needs the whole layout to match.
            partial = json.loads(json.dumps(golden))
            for obs in partial['observations']:
                del obs['layout_reads']
            doc = json.loads(parity_decode.LAYOUT.read_text())
            doc['unit_fields']['brand_new'] = {'offset': '0x99', 'type': 'u8', 'status': 'candidate', 'evidence': 'x'}
            added = temp / 'added.json'
            added.write_text(json.dumps(doc))
            rows = sc64_parity.judge('t1-move', partial, {'move.cruise_speed': 4.0}, {}, {}, parity_decode.Layout(added))
            self.assertEqual(rows[0]['verdict'], 'STALE-GOLDEN')

    def test_pool_reads_need_the_current_chain_format(self):
        layout = parity_decode.Layout()
        obs = {'check': 'x.a', 'subject': 's', 'value': 1, 'lo': 0, 'hi': 2, 'gating': True, 'n_runs': 2,
               'provenance': 'n64-ram', 'tolerance': '1', 'layout_fields': ['pool.fog_tiles'],
               'layout_reads': ['pool.fog_tiles'], 'layout_proven': ['pool.fog_tiles'],
               'layout_fields_sha256': layout.fields_digest(['pool.fog_tiles'])}
        old = {'bundle': {}, 'observations': [obs]}
        new = {'bundle': {'chain_format': 2}, 'observations': [obs]}
        self.assertEqual(sc64_parity.judge('demo', old, {'x.a': 1}, {}, {}, layout)[0]['verdict'], 'STALE-GOLDEN')
        self.assertEqual(sc64_parity.judge('demo', new, {'x.a': 1}, {}, {}, layout)[0]['verdict'], 'PASS')
        legacy = dict(obs)
        for key in ('layout_reads', 'layout_proven', 'layout_fields_sha256'):
            del legacy[key]
        whole = {'layout_sha256': layout.sha256, 'bundle': {}, 'observations': [legacy]}
        self.assertEqual(sc64_parity.judge('demo', whole, {'x.a': 1}, {}, {}, layout)[0]['verdict'], 'STALE-GOLDEN')
        # A golden from an older decoder revision of its scenario is stale.
        self.assertEqual(metrics.SCENARIOS['t3-opening']['decoder_revision'], 2)
        older = {'bundle': {'chain_format': 2}, 'decoder_revision': 1, 'observations': [obs]}
        self.assertEqual(sc64_parity.judge('t3-opening', older, {'x.a': 1}, {}, {}, layout)[0]['verdict'], 'STALE-GOLDEN')

    def test_staged_tag_reaches_each_row(self):
        golden = {'tags': ['n64-staged'], 'observations': [
            {'check': 'x.a', 'subject': 's', 'value': 1, 'lo': 0, 'hi': 2, 'gating': True, 'n_runs': 2,
             'provenance': 'n64-ram', 'tolerance': '1'}]}
        row = sc64_parity.judge('demo', golden, {'x.a': 1}, {}, {})[0]
        self.assertEqual((row['provenance'], row['tags'], row['verdict']), ('n64-ram n64-staged', ['n64-staged'], 'PASS'))

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

    def test_one_sided_report_rows(self):
        golden = {'observations': [
            {'check': 'x.n64only', 'subject': 's', 'value': 5, 'lo': 5, 'hi': 5, 'gating': False, 'n_runs': 2,
             'provenance': 'n64-ram', 'tolerance': 'report'},
            {'check': 'x.gbaonly', 'subject': 's', 'value': None, 'lo': None, 'hi': None, 'gating': False, 'n_runs': 2,
             'provenance': 'n64-ram', 'tolerance': 'report'},
            {'check': 'x.neither', 'subject': 's', 'value': None, 'lo': None, 'hi': None, 'gating': False, 'n_runs': 2,
             'provenance': 'n64-ram', 'tolerance': 'report'}]}
        rows = {r['check']: r for r in sc64_parity.judge('demo', golden, {'x.n64only': None, 'x.gbaonly': 7}, {}, {})}
        self.assertEqual({r['verdict'] for r in rows.values()}, {'REPORT'})
        self.assertIn('GBA only', rows['x.gbaonly']['why'])
        self.assertIn('not measured on the GBA', rows['x.n64only']['why'])
        self.assertIn('neither', rows['x.neither']['why'])
        lines, counts = sc64_parity.scorecard([{'id': 'demo', 'status': 'RUN', 'rows': list(rows.values())}], pathlib.Path('.'), [])
        self.assertEqual(counts, {'REPORT': 3})

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


class StateSource(unittest.TestCase):
    def test_only_clean_producers_supply_states(self):
        with tempfile.TemporaryDirectory() as temp:
            run = pathlib.Path(temp)
            state = run / 'a.state'
            state.write_bytes(b'state')
            digest = hashlib.sha256(b'state').hexdigest()
            plain = b'10 0 0 0\nsave a.state\nquit\n'
            loads = b'load in.state 0\n10 0 0 0\nsave a.state\nquit\n'
            saved = {'ok': True, 'states_sha256': {'a.state': digest}, 'run_id': 'p', 'binary_sha256': 'd' * 64}
            source = {'in.state': {'run_id': 'r', 'binary_sha256': 'c' * 64}}
            cases = (
                (saved, plain, True, True),
                (saved, plain, False, False),                       # no tape digest
                ({**saved, 'ok': 'false'}, plain, True, False),
                ({**saved, 'states_sha256': ['a.state']}, plain, True, False),
                ({**saved, 'states_sha256': {'a.state': 'f' * 64}}, plain, True, False),
                ({**saved, 'run_id': None}, plain, True, False),           # no producer identity
                ({**saved, 'binary_sha256': None}, plain, True, False),
                ({**saved, 'state_input_sha256': {'in.state': None}, 'state_input_source': source}, loads, True, False),
                (None, plain, True, False),
                ({**saved, 'unrecorded_state': True}, plain, True, False),
                ({**saved, 'state_input_sha256': {'in.state': 'f' * 64}}, loads, True, False),
                (saved, loads, True, False),                        # loaded, but no records
                ({**saved, 'state_input_sha256': {'in.state': 'f' * 64}, 'state_input_source': source}, loads, True, True),
            )
            for manifest, tape, digest_tape, good in cases:
                (run / 'tape.txt').write_bytes(tape)
                if isinstance(manifest, dict) and digest_tape:
                    manifest = {**manifest, 'tape_sha256': hashlib.sha256(tape).hexdigest()}
                (run / 'manifest.json').write_text(json.dumps(manifest))
                self.assertEqual(capture.state_source(state, digest) is not None, good, manifest)
            # A producer from before run ids only with the explicit legacy switch.
            manifest = {**saved, 'run_id': None, 'tape_sha256': hashlib.sha256(plain).hexdigest()}
            (run / 'tape.txt').write_bytes(plain)
            (run / 'manifest.json').write_text(json.dumps(manifest))
            self.assertIsNone(capture.state_source(state, digest))
            self.assertIsNotNone(capture.state_source(state, digest, legacy_producer=True))
            for bad in ('', True, 5):   # present but malformed: the legacy switch does not cover it
                (run / 'manifest.json').write_text(json.dumps({**manifest, 'run_id': bad}))
                self.assertIsNone(capture.state_source(state, digest, legacy_producer=True))


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
                                             ''.join(f'{s},0,{H(h)},0,0,0\n' for s, h, _ in checkpoints))
        (run / 'regions.csv').write_text(''.join(f'{s},' + ','.join(blocks) + '\n' for s, _, blocks in checkpoints if blocks))
        header = 'step,tick,phase,same,seconds,pause,selector,running,countdown,table,live,bad_links,asymmetric_links,' \
                 'dual_hits,dual_differ,raw_differs,rows_sha256,globals_sha256\n'
        (run / 'ticks.csv').write_text(header + ''.join(f'{t},{t},1,1,0,0,4,1,0,0,0,0,0,0,0,0,{H("r%d" % t)},{H("g%d" % t)}\n'
                                                        for t in ticks))
        (run / 'ticks.raw').write_bytes(raw)
        (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\n')
        CaptureReport.seal(run, run_id or str(uuid.uuid4()))
        return run

    @staticmethod
    def seal(run, run_id, ok=True, **extra):
        """Record the logs in the manifest, as capture.py does."""
        digests = {name: hashlib.sha256((run / name).read_bytes()).hexdigest()
                   for name in ('ticks.csv', 'checkpoints.csv', 'regions.csv', 'ticks.raw', 'watch.csv', 'status.log')
                   if (run / name).is_file()}
        tape = hashlib.sha256((run / 'tape.txt').read_bytes()).hexdigest() if (run / 'tape.txt').is_file() else None
        inputs = {'binary_sha256': 'b' * 64, 'rom_sha256': 'a' * 64, 'mode': 'jit', 'flash_input_sha256': None}
        (run / 'manifest.json').write_text(json.dumps({'run_id': run_id, 'ok': ok, 'outputs_sha256': digests,
                                                       'tape_sha256': tape, **inputs, **extra}))

    def put_raw(self, run, data):
        """Rewrite ticks.raw and record its digest, as a capture would."""
        (run / 'ticks.raw').write_bytes(data)
        self.seal(run, 'sweep')

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
            # Two checkpoints at one step but one region row: the row could pair with either, so neither counts.
            c = self.run_dir(temp, 'c', [('1000', 'x', []), ('1000', 'y', full)])
            d = self.run_dir(temp, 'd', [('1000', 'z', full), ('1000', 'w', full)])
            text = self.report(capture_report.compare, c, d, set())
            self.assertIn('state_regions_equal=0', text)
            # --opening on a capture tapped without `raw` says so instead of failing.
            (c / 'ticks.raw').unlink(); self.seal(c, 'c')
            self.assertIn('no ticks.raw', self.report(capture_report.opening, c, 10))
            # A checkpoint the tape schedules that neither log has is reported, not passed over.
            for run in (a, b):
                (run / 'tape.txt').write_text('1000 0 0 0\nhash\n1000 0 0 0\nhash\n500 0 0 0\nhash\n')
                self.seal(run, run.name)
            self.assertIn('logs 2 of the 3 checkpoints its tape schedules (first missing at step 2500)',
                          self.report(capture_report.compare, a, b, set()))
            # Both runs stopping at the same step before the tape's end is partial too.
            for run in (a, b):
                (run / 'status.log').write_text('frontend=capture completed_steps=2000\n')
                self.seal(run, run.name)
            self.assertIn("stopped at step 2000, before its tape's last step 2500", self.report(capture_report.compare, a, b, set()))
            # A checkpoint past the run's verified end step: no verdict.
            for run in (a, b):
                (run / 'status.log').write_text('frontend=capture completed_steps=1500\n')
                self.seal(run, run.name)
            self.assertIn('past its verified end step 1500; no verdict', self.report(capture_report.compare, a, b, set()))
            self.assertEqual(capture_report.expected_checkpoints(a) if (a / 'tape.txt').write_text(
                'every +500\n+1000 0 0 0\n') else None, {'500', '1000'})
            self.assertEqual(capture_report.expected_checkpoints(a) if (a / 'tape.txt').write_text(
                'every 0000000000000500\n1000 0 0 0\n') else None, {'500', '1000'})
            # A checkpoint the tape never schedules is reported too.
            for run in (a, b):
                (run / 'tape.txt').write_text('1000 0 0 0\n'); (run / 'status.log').unlink(); self.seal(run, run.name)
            self.assertIn('checkpoints its tape does not schedule', self.report(capture_report.compare, a, b, set()))

    def test_s1_under_legacy_is_never_a_pass(self):
        full = ['0' * 16] * capture_report.REGION_BLOCKS
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            a = self.run_dir(temp, 'a', [('1000', 'h', full)])
            b = self.run_dir(temp, 'b', [('1000', 'h', full)])
            for run in (a, b):
                manifest = json.loads((run / 'manifest.json').read_text())
                del manifest['run_id']
                (run / 'manifest.json').write_text(json.dumps(manifest))
            text = self.report(lambda: capture_report.s1(a, b, legacy=True))
            self.assertIn('S1 checkpoints compared=1', text)
            self.assertNotIn('S1 PASS', text)
            self.assertIn('no PASS', text)
            # Even with run ids recorded, --legacy never yields a PASS.
            for run in (a, b):
                (run / 'status.log').write_text('frontend=capture completed_steps=1000\n')
                (run / 'tape.txt').write_text('1000 0 0 0\nhash\nquit\n')
            self.seal(a, 'a'); self.seal(b, 'b')
            self.assertNotIn('S1 PASS', self.report(lambda: capture_report.s1(a, b, legacy=True)))
            self.assertIn('S1 PASS', self.report(lambda: capture_report.s1(a, b)))
            # A checkpoint the tape schedules but neither log has: no PASS.
            for run in (a, b):
                (run / 'tape.txt').write_text('500 0 0 0\nhash\n500 0 0 0\nhash\nquit\n')
                self.seal(run, run.name)
            self.assertIn('not exactly the ones the tape schedules', self.report(lambda: capture_report.s1(a, b)))
            # `every` checkpoints count too: every 500 over 1000 steps schedules 500 and 1000, plus the hash.
            self.assertEqual(capture_report.expected_checkpoints(a) if (a / 'tape.txt').write_text(
                'every 500\n1000 0 0 0\nhash\n') else None, {'500', '1000', '1000#2'})
            for run in (a, b):
                (run / 'tape.txt').write_text('1000 0 0 0\nhash\nquit\n')
                self.seal(run, run.name)
            # Equal verified ends, but no checkpoint at the end: agreement is only through the last one.
            for run in (a, b):
                (run / 'status.log').write_text('frontend=capture completed_steps=1500\n')
                self.seal(run, run.name)
            self.assertIn('no shared checkpoint at the end step 1500', self.report(lambda: capture_report.s1(a, b)))
            for run in (a, b):
                (run / 'status.log').write_text('frontend=capture completed_steps=1000\n')
                self.seal(run, run.name)
            # A repeated checkpoint at one step is compared in order: an earlier mismatch shows.
            for run, first in ((a, 'p'), (b, 'q')):
                (run / 'checkpoints.csv').write_text('step,tick,sha256,cached_lines,dual_hits,dual_differ\n'
                                                     f'1000,0,{H(first)},0,0,0\n1000,0,{H("h")},0,0,0\n')
                (run / 'regions.csv').write_text(f'1000,{",".join(full)}\n1000,{",".join(full)}\n')
                self.seal(run, run.name)
            self.assertNotIn('S1 PASS', self.report(lambda: capture_report.s1(a, b)))
            # A checkpoint whose cache aliases disagreed is not compared, and never yields a PASS.
            for run in (a, b):
                (run / 'checkpoints.csv').write_text(f'step,tick,sha256,cached_lines,dual_hits,dual_differ\n1000,0,{H("h")},0,1,1\n')
                self.seal(run, run.name)
            self.assertIn('conflicted_not_compared=1', self.report(lambda: capture_report.s1(a, b)))
            self.assertNotIn('S1 PASS', self.report(lambda: capture_report.s1(a, b)))
            for run in (a, b):
                (run / 'checkpoints.csv').write_text(f'step,tick,sha256,cached_lines,dual_hits,dual_differ\n1000,0,{H("h")},0,0,0\n')
                self.seal(run, run.name)
            # A checkpoint row with no conflict count is malformed: no verdict.
            (a / 'checkpoints.csv').write_text(f'step,tick,sha256,cached_lines,dual_hits,dual_differ\n1000,0,{H("h")}\n')
            self.seal(a, 'a')
            self.assertIn('malformed row', self.report(lambda: capture_report.s1(a, b)))
            # A checkpoint row without a hash withholds the verdict.
            (a / 'checkpoints.csv').write_text('step,tick,sha256,cached_lines,dual_hits,dual_differ\n1000,0\n')
            self.seal(a, 'a')
            self.assertIn('malformed row', self.report(lambda: capture_report.s1(a, b)))
            # Occurrences sort numerically: the second differs before the tenth.
            rows = ''.join(f'1000,0,{H("same")},0,0,0\n' for _ in range(10))
            (a / 'checkpoints.csv').write_text('step,tick,sha256,cached_lines,dual_hits,dual_differ\n' + rows)
            self.assertEqual(sorted(['1000#10', '1000#2', '1000'], key=capture_report.checkpoint_order), ['1000', '1000#2', '1000#10'])
            # A tape with load or mode is not a power-on tape: S1 refuses it.
            (a / 'tape.txt').write_text('hash\nmode interp\nquit\n')
            self.seal(a, 'a')
            self.assertIn('S1 covers power-on tapes only', self.report(lambda: capture_report.s1(a, b)))
            for run in (a, b):
                (run / 'tape.txt').write_text('1000 0 0 0\nhash\nquit\n')
                (run / 'checkpoints.csv').write_text(f'step,tick,sha256,cached_lines,dual_hits,dual_differ\n1000,0,{H("h")},0,0,0\n')
                (run / 'regions.csv').write_text(f'1000,{",".join(full)}\n')
                self.seal(run, run.name)
            # Matching checkpoints beyond the verified end: no PASS.
            for run in (a, b):
                (run / 'checkpoints.csv').write_text(f'step,tick,sha256,cached_lines,dual_hits,dual_differ\n'
                                                     f'1000,0,{H("h")},0,0,0\n2000,0,{H("k")},0,0,0\n')
                (run / 'regions.csv').write_text(f'1000,{",".join(full)}\n2000,{",".join(full)}\n')
                self.seal(run, run.name)
            self.assertNotIn('S1 PASS', self.report(lambda: capture_report.s1(a, b)))
            for run in (a, b):
                (run / 'checkpoints.csv').write_text(f'step,tick,sha256,cached_lines,dual_hits,dual_differ\n1000,0,{H("h")},0,0,0\n')
                (run / 'regions.csv').write_text(f'1000,{",".join(full)}\n')
                self.seal(run, run.name)
            # Different verified end steps after the last checkpoint: no PASS.
            (b / 'status.log').write_text('frontend=capture completed_steps=1007\n')
            self.seal(b, 'b')
            self.assertIn('end steps differ', self.report(lambda: capture_report.s1(a, b)))
            # A save after the last hash: the tape no longer ends with a checkpoint.
            (b / 'status.log').write_text('frontend=capture completed_steps=1000\n')
            for run in (a, b):
                (run / 'tape.txt').write_text('1000 0 0 0\nhash\nsave x.state\nquit\n')
                self.seal(run, run.name)
            self.assertIn('does not end with `hash`', self.report(lambda: capture_report.s1(a, b)))
            for run in (a, b):
                (run / 'tape.txt').write_text('1000 0 0 0\nhash\nquit\n')
            (b / 'status.log').write_text('frontend=capture completed_steps=1000\n')
            self.seal(b, 'b')
            # Malformed manifests verify nothing and never crash: null, or ok as a string.
            (a / 'manifest.json').write_text('null')
            self.assertIn('no verdict', self.report(lambda: capture_report.s1(a, b)))
            self.seal(a, 'a', ok='true')
            self.assertIn('did not finish cleanly', self.report(lambda: capture_report.s1(a, b)))
            # A malformed state map or run id refuses the verdict.
            self.seal(a, 'a', state_input_sha256=['x'])
            self.assertIn('malformed state_input_sha256', self.report(lambda: capture_report.s1(a, b)))
            self.seal(a, 7)
            self.assertIn('not a nonempty string', self.report(lambda: capture_report.s1(a, b)))
            # Missing input metadata, or a map set to null, never yields a PASS.
            self.seal(a, 'a', binary_sha256=None)
            self.assertIn('lacks valid binary', self.report(lambda: capture_report.s1(a, b)))
            self.seal(a, 'a', state_input_sha256=None)
            self.assertIn('malformed state_input_sha256', self.report(lambda: capture_report.s1(a, b)))

    def test_compare_reports_coverage_gaps(self):
        full = ['0' * 16] * capture_report.REGION_BLOCKS
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            (temp / 'x').mkdir(); (temp / 'y').mkdir()
            a = self.run_dir(temp / 'x', 'run', [('1000', 'h', full), ('2000', 'h', full), ('3000', 'h', full)])
            b = self.run_dir(temp / 'y', 'run', [('1000', 'h', full), ('3000', 'h', full)])
            text = self.report(lambda: capture_report.compare(a, b, set()))
            self.assertIn('1 checkpoints between steps 1000 and 3000 exist in only one run', text)
            c = self.run_dir(temp / 'y', 'short', [('1000', 'h', full)])
            self.assertIn('last checkpoints differ (3000 vs 1000)', self.report(lambda: capture_report.compare(a, c, set())))
            # The same last checkpoint, but the verified status.log shows one run stopped earlier.
            d = self.run_dir(temp / 'y', 'stopped', [('1000', 'h', full), ('2000', 'h', full), ('3000', 'h', full)])
            for run, steps in ((a, 3400), (d, 3100)):
                (run / 'status.log').write_text(f'frontend=capture completed_steps={steps}\n')
                manifest = json.loads((run / 'manifest.json').read_text())
                manifest['outputs_sha256']['status.log'] = hashlib.sha256((run / 'status.log').read_bytes()).hexdigest()
                (run / 'manifest.json').write_text(json.dumps(manifest))
            self.assertIn(f'{d} ends at step 3100', self.report(lambda: capture_report.compare(a, d, set())))
            # A status.log the manifest does not record is ignored, and the fallback is labelled.
            e = self.run_dir(temp / 'y', 'unrecorded', [('1000', 'h', full), ('3000', 'h', full)])
            (e / 'status.log').write_text('frontend=capture completed_steps=3400\n')
            text = self.report(lambda: capture_report.compare(a, e, set()))
            self.assertIn('has no verified status.log', text)
            # Equal verified ends past the last shared checkpoint: the uncompared tail is reported.
            for run in (f_ := self.run_dir(temp / 'y', 'tail1', [('1000', 'h', full)]),
                        g_ := self.run_dir(temp / 'y', 'tail2', [('1000', 'h', full)])):
                (run / 'status.log').write_text('frontend=capture completed_steps=1500\n')
                self.seal(run, run.name)
            self.assertIn('steps after it are not compared', self.report(lambda: capture_report.compare(f_, g_, set())))
            # Same end, but a checkpoint after the last shared one exists in only one run.
            f = self.run_dir(temp / 'y', 'trail', [('1000', 'h', full), ('2000', 'x', full)])
            g = self.run_dir(temp / 'y', 'trail2', [('1000', 'h', full)])
            self.assertIn('after step 1000 exist in only one run', self.report(lambda: capture_report.compare(f, g, set())))

    def test_compare_refuses_a_checkpoint_before_a_load(self):
        full = ['0' * 16] * capture_report.REGION_BLOCKS
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            a = self.run_dir(temp, 'a', [('0', 'h', full)])
            b = self.run_dir(temp, 'b', [('0', 'h', full)])
            (b / 'input').mkdir()
            (b / 'input' / 'x.state').write_bytes(b'x')
            provenance = {'state_input_sha256': {'x.state': hashlib.sha256(b'x').hexdigest()},
                          'state_input_source': {'x.state': {'run_id': 'w', 'binary_sha256': 'c' * 64}}}
            (b / 'tape.txt').write_text('hash\nload x.state 0\nhash\nquit\n')
            self.seal(b, 'b', **provenance)
            self.assertIn('checkpoints before a load', self.report(lambda: capture_report.compare(a, b, set())))
            (b / 'tape.txt').write_text('load x.state 0\nhash\nload x.state 0\nhash\nquit\n')
            self.seal(b, 'b', **provenance)
            self.assertIn('loads more than once', self.report(lambda: capture_report.compare(a, b, set())))
            for zero in ('0', '00', '+0'):   # parsed like the frontend: no checkpoints
                (b / 'tape.txt').write_text(f'every {zero}\nload x.state 0\nhash\nquit\n')
                self.seal(b, 'b', **provenance)
                self.assertNotIn('no verdict', self.report(lambda: capture_report.compare(a, b, set())))
            # (A nonzero `every` before `load` never finishes: the frontend's load refuses it.)

    def test_compare_warns_about_mode_switches(self):
        full = ['0' * 16] * capture_report.REGION_BLOCKS
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            a = self.run_dir(temp, 'a', [('1000', 'h', full)])
            b = self.run_dir(temp, 'b', [('1000', 'h', full)])
            (b / 'tape.txt').write_text('10 0 0 0\nhash\nmode interp\nquit\n')
            self.seal(b, 'b')
            self.assertIn('switches CPU mode', self.report(lambda: capture_report.compare(a, b, set())))

    def test_compare_checks_the_runs_themselves(self):
        full = ['0' * 16] * capture_report.REGION_BLOCKS
        with tempfile.TemporaryDirectory() as temp:
            temp = pathlib.Path(temp)
            a = self.run_dir(temp, 'a', [('1000', 'x', full), ('2000', 'y', full)])
            b = self.run_dir(temp, 'b', [('1000', 'x', full)])
            compare = lambda x, y, legacy=False: self.report(lambda: capture_report.compare(x, y, set(), legacy=legacy))
            self.assertIn('same run directory twice', compare(a, a))
            text = compare(a, b)
            self.assertIn('last checkpoints differ (2000 vs 1000)', text)   # an early end is never read as agreement
            self.seal(b, 'b', ok=False)
            self.assertIn('did not finish cleanly', compare(a, b))
            self.seal(a, 'a'); self.seal(b, 'a')
            self.assertIn('same run id twice', compare(a, b))
            self.seal(b, 'b')
            (b / 'checkpoints.csv').write_text((a / 'checkpoints.csv').read_text())
            self.assertIn('does not match its manifest', compare(a, b))
            self.seal(b, 'b')
            (b / 'tape.txt').write_text('load lobby.state 900\n10 0 0 0\n')
            self.assertIn('tape.txt is missing or does not match', compare(a, b))   # edited after sealing
            self.seal(b, 'b')
            self.assertIn('loads a state with no recorded digest', compare(a, b))
            (b / 'input').mkdir()
            (b / 'input' / 'lobby.state').write_bytes(b'state')
            digest = {'lobby.state': hashlib.sha256(b'state').hexdigest()}
            self.seal(b, 'b', state_input_sha256=digest)
            self.assertIn('no recorded producer', compare(a, b))
            self.assertIn('predates recorded state sources', compare(a, b, legacy=True))
            # Under --legacy a present but malformed producer record still refuses the verdict.
            self.seal(b, 'b', state_input_sha256=digest, state_input_source={'lobby.state': None})
            self.assertIn('malformed producer record', compare(a, b, legacy=True))
            source = {'lobby.state': {'run_id': 'w', 'binary_sha256': 'c' * 64}}
            self.seal(b, 'b', state_input_sha256=digest, state_input_source=source, unrecorded_state=True)
            self.assertIn('--unrecorded-state', compare(a, b))
            self.seal(b, 'b', state_input_sha256=digest, state_input_source=source)
            self.assertIn('compare checkpoints=', compare(a, b))   # verified provenance: a verdict
            (b / 'regions.csv').unlink()
            self.assertIn('regions.csv is missing', compare(a, b))
            (b / 'manifest.json').write_text('{}')
            self.assertIn('run id is missing', compare(a, b))
            self.assertIn('legacy manifests', compare(a, b, legacy=True))

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
                handle.write(f'6,6,0,0,0,0,4,1,0,0,0,0,0,0,0,0,{H("r6")},{H("g6")}\n')   # a change-step read only in e
            self.seal(e, 'e')
            self.assertIn('change-step records: 0/1 shared=0 only_in_one_run=1', self.report(capture_report.s8_pair, a, e))
            # A repeated tick: each follow-up belongs to the change before it.
            r1, r2 = self.run_dir(temp, 'r1', []), self.run_dir(temp, 'r2', [])
            header = (r1 / 'ticks.csv').read_text()
            row = lambda tick, phase, h: f'{tick},{tick},{phase},0,0,0,4,1,0,0,0,0,0,0,0,0,{H(h)},{H("g")}\n'
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
            line = lambda table: f'7,7,1,1,0,0,4,1,0,{table},3,0,0,0,0,0,{H("r7")},{H("g7")}\n'
            (t1 / 'ticks.csv').write_text(header + line(3154736)); (t2 / 'ticks.csv').write_text(header + line(0))
            self.seal(t1, 't1'); self.seal(t2, 't2')
            text = self.report(capture_report.s8_pair, t1, t2)
            self.assertIn('table_in_one_run_only=1', text)
            self.assertIn('coverage differs', text)
            # A tape that turns the tap off (or a run without a verified end) is a partial comparison.
            (o1 / 'tape.txt').write_text('tap tick\n10 0 0 0\ntap off\n10 1 0 0\n'); self.seal(o1, 'o1')
            self.assertIn('has no verified end step', self.report(capture_report.s8_pair, o1, o2))
            (o1 / 'tape.txt').write_text('tap tick\n10 0 0 0\nsave x.state\n10 1 0 0\n'); self.seal(o1, 'o1')
            self.assertIn('saves (which changes a JIT run)', self.report(capture_report.s8_pair, o1, o2))
            # Readable change-step rows but no follow-up reads in either run: still partial.
            n1, n2 = self.run_dir(temp, 'n1', []), self.run_dir(temp, 'n2', [])
            readable = lambda tick: f'{tick},{tick},0,0,0,0,4,1,0,7,1,0,0,0,0,0,{H("r%d" % tick)},{H("g")}\n'
            for run in (n1, n2):
                (run / 'ticks.csv').write_text(header + row(0, 0, 'a') + readable(5) + readable(6))
                self.seal(run, run.name)
            text = self.report(capture_report.s8_pair, n1, n2)
            self.assertIn('follow-up records: 0/0', text)
            self.assertIn('no comparable unit rows', text)
            # Equal logs but different verified ends: unequal horizons, partial.
            for run, steps in ((n1, 7), (n2, 9)):
                (run / 'status.log').write_text(f'frontend=capture completed_steps={steps}\n')
                self.seal(run, run.name)
            self.assertIn('different verified steps (7 vs 9)', self.report(capture_report.s8_pair, n1, n2))
            self.assertIn('no verified tap_reads and tick_changes counters', self.report(capture_report.s8_pair, n1, n2))
            for run in (n1, n2):
                (run / 'status.log').write_text('frontend=capture completed_steps=9\ntap_reads=4 tick_changes=3\n')
                self.seal(run, run.name)
            self.assertIn('has 3 reads (3 tick changes), but the frontend counted 4 (3)', self.report(capture_report.s8_pair, n1, n2))
            (n1 / 'status.log').write_text('frontend=capture completed_steps=5\ntap_reads=3 tick_changes=3\n'); self.seal(n1, 'n1')
            self.assertIn('reads past its verified end step 5; no verdict', self.report(capture_report.s8_pair, n1, n2))
            # A digest-matching log that is not valid UTF-8: no verdict, no traceback.
            (n1 / 'ticks.csv').write_bytes(b'step,tick\n\xff\xfe,1\n'); self.seal(n1, 'n1')
            self.assertIn('not readable CSV', self.report(capture_report.s8_pair, n1, n2))
            # A missing interior follow-up is a gap; a tick change on the very next step is not.
            stepped = lambda st, tick, phase: f'{st},{tick},{phase},0,0,0,4,1,0,0,0,0,0,0,0,0,{H("x")},{H("g")}\n'
            m1, m2 = self.run_dir(temp, 'm1', []), self.run_dir(temp, 'm2', [])
            for run, rows in ((m1, [(0, 0, 0), (1, 0, 1), (2, 1, 0), (3, 2, 0), (4, 2, 1)]),
                              (m2, [(0, 0, 0), (1, 0, 1), (2, 1, 0), (5, 2, 0), (6, 2, 1)])):
                (run / 'ticks.csv').write_text(header + ''.join(stepped(*r) for r in rows))
                self.seal(run, run.name)
            gaps = self.report(capture_report.s8_pair, m1, m2)
            self.assertNotIn('m1 has a change-step read', gaps)   # tick changes on the very next step
            self.assertIn('m2 has a change-step read at tick 1 with no read on the next step', gaps)
            # A next-step follow-up from another tick, or a table lost mid-match: partial.
            for run, rows in ((m1, [(0, 0, 0), (1, 0, 1), (2, 1, 0), (3, 2, 1)]), (m2, [(0, 0, 0), (1, 0, 1), (2, 1, 0), (3, 2, 1)])):
                (run / 'ticks.csv').write_text(header + ''.join(stepped(*r) for r in rows))
                self.seal(run, run.name)
            self.assertIn('has a change-step read at tick 1 with no read on the next step', self.report(capture_report.s8_pair, m1, m2))
            # A duplicate follow-up (a second phase-1 read of tick 2) is not silently dropped.
            for run in (m1, m2):
                (run / 'ticks.csv').write_text(header + ''.join(stepped(*r) for r in
                                                                ((0, 0, 0), (1, 0, 1), (2, 1, 0), (3, 1, 1), (4, 2, 0), (5, 2, 1), (6, 2, 1))))
                self.seal(run, run.name)
            self.assertIn('follow-up read at tick 2 that does not follow its change-step read', self.report(capture_report.s8_pair, m1, m2))
            lost_rows = header + f'0,0,0,0,0,0,4,1,0,0,0,0,0,0,0,0,{H("x")},{H("g")}\n' + ''.join(
                f'{st},{t},{ph},0,0,0,4,1,0,{tb},2,0,0,0,0,0,{H("x")},{H("g")}\n'
                for st, t, ph, tb in ((1, 1, 0, 7), (2, 1, 1, 7), (3, 2, 0, 0), (4, 2, 1, 0), (5, 3, 0, 7), (6, 3, 1, 7)))
            for run in (m1, m2):
                (run / 'ticks.csv').write_text(lost_rows)
                self.seal(run, run.name)
            self.assertIn('lost the unit table at tick 2', self.report(capture_report.s8_pair, m1, m2))
            # A row cut right after the phase field is unreadable.
            (m1 / 'ticks.csv').write_text(header + '0,0,0\n'); self.seal(m1, 'm1')
            self.assertIn('unreadable rows; no verdict', self.report(capture_report.s8_pair, m1, m2))
            # The same skipped tick in both runs is still a gap.
            k1, k2 = self.run_dir(temp, 'k1', []), self.run_dir(temp, 'k2', [])
            for run in (k1, k2):
                (run / 'ticks.csv').write_text(header + row(0, 0, 'a') + row(0, 1, 'a') + row(1, 0, 'a') + row(1, 1, 'a')
                                               + row(3, 0, 'a') + row(3, 1, 'a'))
                self.seal(run, run.name)
            self.assertIn('skips a tick (before tick 3)', self.report(capture_report.s8_pair, k1, k2))
            # `tap step` captures get no verdict from this report.
            for run in (m1, m2):
                (run / 'tape.txt').write_text('tap step\n10 0 0 0\n'); self.seal(run, run.name)
            self.assertIn('covers `tap tick` captures only', self.report(capture_report.s8_pair, m1, m2))
            # Reads whose cache aliases disagreed prove nothing: excluded, and the result is partial.
            d1, d2 = self.run_dir(temp, 'd1', []), self.run_dir(temp, 'd2', [])
            for run in (d1, d2):
                (run / 'ticks.csv').write_text(header + row(0, 0, 'a') + row(0, 1, 'a')
                                               + f'1,1,0,0,0,0,4,1,0,0,0,0,0,1,1,0,{H("x")},{H("g")}\n' + row(1, 1, 'a'))
                self.seal(run, run.name)
            self.assertIn('disagreeing cache aliases', self.report(capture_report.s8_pair, d1, d2))
            # A phase a `tap tick` capture cannot produce refuses the verdict.
            p1, p2 = self.run_dir(temp, 'p1', []), self.run_dir(temp, 'p2', [])
            for run, last in ((p1, 'x'), (p2, 'y')):
                (run / 'ticks.csv').write_text(header + row(0, 0, 'a') + row(5, 0, 'a') + row(5, 1, 'a') + row(5, 2, last))
                self.seal(run, run.name)
            self.assertIn('unreadable rows; no verdict', self.report(capture_report.s8_pair, p1, p2))   # tap tick has no phase 2
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

    def test_sweep_needs_every_expected_raw_record(self):
        """The tick log says which raw records the frontend wrote; a missing one keeps the run out."""
        def record(step, tick, phase, rows):
            body = b''.join(struct.pack('>H', i) + r for i, r in enumerate(rows))
            return struct.pack('<4I', step, tick, phase, len(rows)) + body
        start = [row(10, 10, 0, 7), row(20, 20, 1, 41)]
        built = start + [row(30, 30, 0, 111)]
        line = lambda st, t, ph, same, live: f'{st},{t},{ph},{same},0,0,4,1,0,{1 if live else 0},{live},0,0,0,0,0,r,g\n'
        log = ('step,tick,phase,same,seconds,pause,selector,running,countdown,table,live,bad_links,asymmetric_links,'
               'dual_hits,dual_differ,raw_differs,rows_sha256,globals_sha256\n'
               + line(1, 0, 0, 0, 0) + line(2, 0, 1, 1, 0) + line(3, 1, 0, 0, 2) + line(4, 1, 1, 1, 2)
               + line(5, 2, 0, 0, 2) + line(6, 2, 1, 0, 3) + line(7, 3, 0, 0, 3) + line(8, 3, 1, 1, 3) + line(9, 4, 0, 0, 3)
               + line(10, 4, 1, 1, 3))
        complete = (record(1, 0, 0, []) + record(3, 1, 0, start) + record(5, 2, 0, start) + record(6, 2, 1, built)
                    + record(7, 3, 0, built) + record(9, 4, 0, built))
        with tempfile.TemporaryDirectory() as temp:
            run = self.run_dir(pathlib.Path(temp), 'a', [], raw=complete)
            (run / 'ticks.csv').write_text(log)
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\n')
            self.seal(run, 'run-a')
            self.assertNotIn('left out of the totals', self.report(capture_report.sweep, [run], 3, '800eff20'))
            # Without --ticks, a log ending on a change read: that tick contributes nothing.
            (run / 'ticks.csv').write_text(log + line(11, 5, 0, 0, 4))
            self.put_raw(run, complete + record(11, 5, 0, built + [row(40, 40, 0, 112)]))
            self.seal(run, 'run-a')
            self.assertNotIn('112@', self.report(capture_report.sweep, [run], 1 << 30, '800eff20'))
            (run / 'ticks.csv').write_text(log)
            self.put_raw(run, complete)
            self.seal(run, 'run-a')
            # Without --ticks, a raw log that stops before the last complete tick: out of the totals.
            self.put_raw(run, complete.replace(record(9, 4, 0, built), b''))
            self.seal(run, 'run-a')
            self.assertIn('left out of the totals', self.report(capture_report.sweep, [run], 1 << 30, '800eff20'))
            self.put_raw(run, complete)
            self.seal(run, 'run-a')
            # A conflict on the launch (tick-0) read also keeps the run out.
            (run / 'ticks.csv').write_text(log.replace('\n1,0,0,0,0,0,4,1,0,0,0,0,0,0,0,0,r,g', '\n1,0,0,0,0,0,4,1,0,0,0,0,0,1,1,0,r,g'))
            self.seal(run, 'run-a')
            self.assertIn('disagreeing cache aliases at tick 0', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(log)
            self.seal(run, 'run-a')
            # A read with disagreeing cache aliases keeps the run out.
            (run / 'ticks.csv').write_text(log.replace('\n5,2,0,0,0,0,4,1,0,1,2,0,0,0,0,0,r,g', '\n5,2,0,0,0,0,4,1,0,1,2,0,0,1,1,0,r,g'))
            self.seal(run, 'run-a')
            self.assertIn('disagreeing cache aliases at tick 2', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(log)
            self.seal(run, 'run-a')
            # The first live record missing: the check starts at the tick log's first live read.
            self.put_raw(run, complete.replace(record(3, 1, 0, start), b''))
            self.seal(run, 'run-a')
            self.assertIn('left out of the totals', self.report(capture_report.sweep, [run], 3, '800eff20'))
            self.put_raw(run, complete)
            self.seal(run, 'run-a')
            # Drop the follow-up record at tick 2 whose rows changed: that sample is missing.
            self.put_raw(run, complete.replace(record(6, 2, 1, built), b''))
            self.seal(run, 'run-a')
            self.assertIn('a missing unit sample at tick 2', self.report(capture_report.sweep, [run], 3, '800eff20'))

    def test_sweep_skips_an_empty_first_sample(self):
        def record(step, tick, rows):
            body = b''.join(struct.pack('>H', i) + r for i, r in enumerate(rows))
            return struct.pack('<4I', step, tick, 0, len(rows)) + body   # change reads (phase 0)
        start = [row(10, 10, 0, 7), row(20, 20, 1, 41)]
        raw = record(1, 1, []) + record(2, 2, start) + record(3, 3, start + [row(30, 30, 0, 111)])
        with tempfile.TemporaryDirectory() as temp:
            run = self.run_dir(pathlib.Path(temp), 'a', [], raw=raw)
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            # Tick log: the launch read at step 0, then one change read per tick with rows live from
            # tick 1 (step 1); a stale seed before the launch and the launch seed at step 1.
            header = (run / 'ticks.csv').read_text().split('\n')[0]
            # Live counts as the raw records have them: three rows at tick 3, two elsewhere.
            readable = ''.join(f'{n},{n},0,1,0,0,4,1,0,1,{3 if n == 3 else 2},0,0,0,0,0,r,g\n' for n in range(1, 62))
            (run / 'ticks.csv').write_text(header + '\n0,0,0,1,0,0,4,1,0,0,0,0,0,0,0,0,r,g\n' + readable)
            self.seal(run, 'sweep')
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n0,0,800eff20,4,111\n1,1,800eff20,4,222\n5,5,800eff20,4,333\n')
            self.seal(run, 'sweep')
            text = self.report(capture_report.sweep, [run], 10, '800eff20')
            self.assertIn('seed=546', text)   # 0x222, not the stale 0x111 or the later 0x333
            self.assertIn('ending before the cutoff (last sample at tick 3)', text)
            self.assertIn('through tick 3', text)
            # A pause in raw logging inside the window is missing samples, not a wide stride.
            gapped = record(4, 4, start) + record(5, 5, start) + record(60, 60, start) + record(61, 61, start)
            self.put_raw(run, raw + gapped)
            self.assertIn('a missing unit sample at tick 6', self.report(capture_report.sweep, [run], 50, '800eff20'))
            # A gap that starts after the cutoff leaves a covered run eligible.
            self.assertNotIn('missing unit sample', self.report(capture_report.sweep, [run], 5, '800eff20'))
            # No --ticks: the gap check still runs through the run's own last sample.
            self.assertIn('a missing unit sample at tick 6', self.report(capture_report.sweep, [run], 1 << 30, '800eff20'))
            # A truncated record after a covered window: the run is reported, with its cutoff.
            self.put_raw(run, raw + record(4, 4, start)[:-5])
            self.assertIn('a truncated, unverified or out-of-order ticks.raw', self.report(capture_report.sweep, [run], 10, '800eff20'))
            (run / 'ticks.raw').write_bytes(raw + record(4, 4, start)[:9])   # a partial header
            self.assertIn('a truncated, unverified or out-of-order ticks.raw', self.report(capture_report.sweep, [run], 10, '800eff20'))
            # Two launches under one tap: the tick counter restarts, so the run is refused.
            self.put_raw(run, raw + record(4, 2, start) + record(5, 3, start))
            self.assertIn('a tick counter reset', self.report(capture_report.sweep, [run], 3, '800eff20'))
            # A sample whose read found no unit table is not coverage.
            self.put_raw(run, raw + record(4, 4, start))
            csv_text = (run / 'ticks.csv').read_text()
            (run / 'ticks.csv').write_text(csv_text.replace('\n3,3,0,1,0,0,4,1,0,1,', '\n3,3,0,1,0,0,4,1,0,0,'))
            self.seal(run, 'sweep')
            self.assertIn('no readable unit table at tick 3', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A tick with no change read of its own (a follow-up read in its place) is a gap.
            (run / 'ticks.csv').write_text(csv_text.replace('\n3,3,0,1,0,0,4,1,0,1,', '\n3,2,1,1,0,0,4,1,0,1,'))
            self.seal(run, 'sweep')
            self.assertIn('a missing unit sample at tick 3', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # Under `tap tick` a follow-up must come right after its own change read.
            (run / 'ticks.csv').write_text(csv_text.replace('\n2,2,0,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n',
                                                            '\n2,2,0,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n2,2,1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n'))
            self.seal(run, 'sweep')
            self.assertIn('left out of the totals', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A raw record whose row count differs from its tick-log read is not counted.
            (run / 'ticks.csv').write_text(csv_text.replace('\n2,2,0,1,0,0,4,1,0,1,2,', '\n2,2,0,1,0,0,4,1,0,1,5,'))
            self.seal(run, 'sweep')
            self.assertIn('does not match its tick-log read', self.report(capture_report.sweep, [run], 3, '800eff20'))
            # --opening refuses a tick log it cannot parse instead of failing.
            (run / 'ticks.csv').write_text(csv_text + '62,6')
            self.seal(run, 'sweep')
            self.assertIn('match boundaries are unverified', self.report(capture_report.opening, run, 3))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A digest-matching log that is not UTF-8: --opening and --sweep report it and go on.
            (run / 'ticks.csv').write_bytes(csv_text.encode() + b'\xff\n')
            self.seal(run, 'sweep')
            self.assertIn('not readable CSV', self.report(capture_report.opening, run, 3))
            self.assertIn('ticks.csv has unreadable rows; skipped', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            (run / 'ticks.csv').write_text(csv_text.replace('\n2,2,0,1,', '\n2,2,2,1,'))
            self.seal(run, 'sweep')
            self.assertIn('never writes; skipped', self.report(capture_report.sweep, [run], 3, '800eff20'))
            # A repeated raw record (same step twice) and a stride the frontend refuses: out, batch goes on.
            (run / 'ticks.csv').write_text(csv_text)
            self.put_raw(run, raw + record(3, 3, start + [row(30, 30, 0, 111)]) + record(4, 4, start))
            self.assertIn('out-of-order ticks.raw', self.report(capture_report.sweep, [run], 3, '800eff20'))
            self.put_raw(run, raw + record(4, 4, start))
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw 0\n10 0 0 0\n')
            self.seal(run, 'sweep')
            self.assertIn('outside 1..1000000; skipped', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            self.put_raw(run, raw)
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # Frontend counters that do not match the tick log keep the run out of the totals.
            (run / 'status.log').write_text('frontend=capture completed_steps=62\ntap_reads=5 tick_changes=5\n')
            self.seal(run, 'sweep')
            self.put_raw(run, raw + record(4, 4, start))
            self.assertIn('a tick log that status.log does not confirm', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'status.log').unlink()
            self.seal(run, 'sweep')
            self.put_raw(run, raw)
            # The tick log must reach an explicit cutoff, and a reset in the log refuses the run.
            self.put_raw(run, raw + record(4, 4, start))
            self.assertIn('ending before the cutoff', self.report(capture_report.sweep, [run], 70, '800eff20'))
            reset_log = csv_text.split('\n')[0] + '\n0,0,1,1,0,0,4,1,0,0,0,0,0,0,0,0,r,g\n' + ''.join(
                f'{st},{t},1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' for st, t in ((1, 1), (2, 2), (3, 3), (4, 0), (5, 1), (6, 2)))
            (run / 'ticks.csv').write_text(reset_log)
            self.seal(run, 'sweep')
            self.assertIn('a tick counter reset to 0', self.report(capture_report.sweep, [run], 4, '800eff20'))
            # A reset after the cutoff still means two matches in one capture: refused too.
            self.assertIn('a tick counter reset to 0', self.report(capture_report.sweep, [run], 2, '800eff20'))
            # Empty records before the first live one, at the same tick or past the cutoff, are
            # outside the match and change nothing.
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            self.put_raw(run, record(0, 90, []) + raw + record(4, 4, start))
            text = self.report(capture_report.sweep, [run], 3, '800eff20')
            self.assertNotIn('left out of the totals', text)
            self.assertIn('owner 0: 111@3', text)
            # A first live sample after the cutoff: that run is skipped, the next is still reported.
            late = self.run_dir(pathlib.Path(temp), 'late', [], raw=record(24, 24, start))
            (late / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(late, 'late')
            text = self.report(capture_report.sweep, [late, run], 3, '800eff20')
            self.assertIn('sweep late: no sample with live rows by tick 3', text)
            self.assertIn('sweep a: seed=', text)
            # --opening skips an empty pre-match record past the cutoff too.
            self.put_raw(run, record(0, 90, []) + raw)
            self.assertIn('owner 0: 7@2 111@3', self.report(capture_report.opening, run, 3))
            # A second launch in the same capture is never merged into the opening.
            self.put_raw(run, raw + record(9, 1, [row(5, 5, 2, 64)]))
            opened = self.report(capture_report.opening, run, 50)
            self.assertIn('restarts', opened)
            self.assertNotIn('owner 2', opened)
            # A reset between raw samples, visible only in the tick log, also stops the opening.
            self.put_raw(run, raw + record(9, 24, [row(5, 5, 2, 64)]))
            hidden = csv_text.split('\n')[0] + '\n' + ''.join(
                f'{st},{t},1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' for st, t in ((1, 1), (2, 2), (3, 3), (5, 40), (6, 0), (9, 24)))
            (run / 'ticks.csv').write_text(hidden)
            self.seal(run, 'sweep')
            opened = self.report(capture_report.opening, run, 50)
            self.assertIn('restarts', opened)
            self.assertNotIn('owner 2', opened)
            # A truncated record at the reset step does not discard the first match.
            self.put_raw(run, raw + record(6, 0, start)[:-5])
            (run / 'ticks.csv').write_text(hidden)
            self.seal(run, 'sweep')
            self.assertIn('owner 0: 7@2 111@3', self.report(capture_report.opening, run, 50))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A tap turned off and on again could hide a relaunch: no report.
            self.put_raw(run, raw)
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\ntap off\n10 0 0 0\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            self.assertIn('no report', self.report(capture_report.opening, run, 50))
            # An initial or trailing `tap off` leaves no gap between active taps.
            (run / 'tape.txt').write_text('tap off\ntap tick raw\n10 0 0 0\ntap off\ntap off\n')
            self.seal(run, 'sweep')
            self.assertNotIn('no report', self.report(capture_report.opening, run, 50))
            # Raw sampling paused by a tap line without `raw`: no opening.
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\ntap tick\n10 0 0 0\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            self.assertIn('tap settings change during the run', self.report(capture_report.opening, run, 50))
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            # A header cut off after the step field, at the reset step, keeps the first match.
            self.put_raw(run, raw + struct.pack('<I', 6))
            (run / 'ticks.csv').write_text(hidden)
            self.seal(run, 'sweep')
            self.assertIn('owner 0: 7@2 111@3', self.report(capture_report.opening, run, 50))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            self.put_raw(run, raw)
            # Single-run reports survive a truncated tail and say the report is partial.
            self.put_raw(run, raw + record(4, 4, start)[:-5])
            self.assertIn('covers only the records before it', self.report(capture_report.opening, run, 50))
            self.put_raw(run, raw)
            # A lobby counter that runs down before the first live read is not a second match.
            lobby = csv_text.split('\n')[0] + '\n0,500,0,1,0,0,4,1,0,0,0,0,0,0,0,0,r,g\n' + ''.join(
                f'{st},{st},0,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' for st in range(1, 62))
            (run / 'ticks.csv').write_text(lobby)
            self.seal(run, 'sweep')
            self.assertNotIn('restarts', self.report(capture_report.opening, run, 50))
            self.assertNotIn('more than one match', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A corrupt row count is refused before the body is read.
            self.put_raw(run, struct.pack('<4I', 1, 1, 1, 100000))
            with self.assertRaises(ValueError):
                list(capture_report.raw_records(run / 'ticks.raw'))
            self.put_raw(run, raw)
            # Reads before the match (an earlier tap reaching tick 300) cannot satisfy the cutoff.
            early = csv_text.split('\n')[0] + '\n0,0,1,1,0,0,4,1,0,0,0,0,0,0,0,0,r,g\n0,300,0,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' + ''.join(
                f'{st},{st},0,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n' for st in range(2, 6))
            (run / 'ticks.csv').write_text(early)
            self.seal(run, 'sweep')
            self.assertIn('left out of the totals', self.report(capture_report.sweep, [run], 65, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # Tap settings changed during the run: coverage cannot be judged.
            self.put_raw(run, raw)
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\ntap tick raw 25\n10 0 0 0\n')
            self.seal(run, 'sweep')
            self.assertIn('tap settings that change', self.report(capture_report.sweep, [run], 3, '800eff20'))
            self.put_raw(run, raw)
            # No raw digest, or a state no capture recorded: out of the totals.
            self.put_raw(run, raw + record(4, 4, start))
            manifest = json.loads((run / 'manifest.json').read_text())
            del manifest['outputs_sha256']['ticks.raw']
            (run / 'manifest.json').write_text(json.dumps(manifest))
            self.assertIn('unverified or out-of-order ticks.raw', self.report(capture_report.sweep, [run], 3, '800eff20'))
            self.put_raw(run, raw + record(4, 4, start))
            manifest = json.loads((run / 'manifest.json').read_text())
            manifest['unrecorded_state'] = True
            (run / 'manifest.json').write_text(json.dumps(manifest))
            self.assertIn('--unrecorded-state', self.report(capture_report.sweep, [run], 3, '800eff20'))
            self.put_raw(run, raw)
            # A failed capture, an edited tick log, or a tap started after the launch: out of the totals.
            self.seal(run, 'sweep', ok=False)
            self.assertIn('did not finish cleanly', self.report(capture_report.sweep, [run], 3, '800eff20'))
            self.seal(run, 'sweep')
            (run / 'ticks.csv').write_text((run / 'ticks.csv').read_text() + '99,99,1,1,0,0,4,1,0,1,2,0,0,0,0,0,r,g\n')
            self.assertIn('ticks.csv missing or not matching the manifest', self.report(capture_report.sweep, [run], 3, '800eff20'))
            saved = (run / 'ticks.csv').read_text()
            (run / 'ticks.csv').write_text('\n'.join(l for l in saved.split('\n') if not l.startswith('0,0,')))
            self.seal(run, 'sweep')
            self.assertIn('the tap may have started after the launch', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A tick-only tap before launch and raw sampling later: the tap lines differ, so out.
            (run / 'tape.txt').write_text('tap tick\n10 0 0 0\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            self.assertIn('tap settings that change', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            # An edited watch log: the seed is unknown, the run still counts.
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n2,0,800eff20,4,999\n')
            edited = self.report(capture_report.sweep, [run], 3, '800eff20')
            self.assertIn('seed unknown', edited)
            self.assertIn('seed=None', edited)
            self.seal(run, 'sweep')
            # A corrupt tick log is skipped before it is parsed, and the batch goes on.
            (run / 'ticks.csv').write_text('step,tick\nnot,a,number\n')
            batch = self.report(capture_report.sweep, [run, late], 3, '800eff20')
            self.assertIn('sweep a: ticks.csv missing or not matching the manifest; skipped', batch)
            self.assertIn('sweep late:', batch)
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A mid-match tap whose first read found no table (zero rows, tick not 0): out of the totals.
            (run / 'ticks.csv').write_text(csv_text.replace('\n0,0,0,1,0,0,4,1,0,0,', '\n0,1,0,1,0,0,4,1,0,0,'))
            self.seal(run, 'sweep')
            self.assertIn('no read of tick 0', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A jump from tick 0 straight to a later first live tick: units in between are unseen.
            (run / 'ticks.csv').write_text(csv_text.replace('\n1,1,0,1,0,0,4,1,0,1,', '\n1,10,0,1,0,0,4,1,0,1,'))
            self.seal(run, 'sweep')
            self.assertIn('a first live read at tick 10, not tick 1', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A first live read at tick 0 or 2 (not 1): out of the totals.
            for tick in ('0', '2'):
                (run / 'ticks.csv').write_text(csv_text.replace('\n1,1,0,1,0,0,4,1,0,1,', f'\n1,{tick},0,1,0,0,4,1,0,1,'))
                self.seal(run, 'sweep')
                self.assertIn('left out of the totals', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # An explicit null run id is refused; only an absent one falls back to the directory.
            self.seal(run, None)
            self.assertIn('run id is not a nonempty string; skipped', self.report(capture_report.sweep, [run], 3, '800eff20'))
            self.seal(run, 'sweep')
            # A run id that is not a string skips the run, and the batch goes on.
            self.seal(run, ['not', 'a', 'string'])
            weird = self.report(capture_report.sweep, [run, late], 3, '800eff20')
            self.assertIn('run id is not a nonempty string; skipped', weird)
            self.assertIn('sweep late:', weird)
            self.seal(run, 'sweep')
            # Tick 0 seen, but the game then advanced with no readable table before the first live read.
            (run / 'ticks.csv').write_text(csv_text.replace('\n1,1,0,1,0,0,4,1,0,1,2,', '\n1,1,0,1,0,0,4,1,0,0,0,'))
            self.seal(run, 'sweep')
            self.assertIn('no read of tick 0', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A watch declared with width 04 is the same four-byte watch.
            (run / 'tape.txt').write_text('watch 800EFF20 04\ntap tick raw\n10 0 0 0\n')
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n1,1,800eff20,4,222\n')
            self.seal(run, 'sweep')
            self.assertIn('seed=546', self.report(capture_report.sweep, [run], 3, '800eff20'))
            # --opening does not trust an edited tape to rule out tap gaps.
            (run / 'tape.txt').write_text('tap tick raw\n10 0 0 0\n')
            self.assertIn('tap gaps cannot be ruled out', self.report(capture_report.opening, run, 3))
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            # Two samples within one tick (change step, then follow-up) keep their order.
            two = raw + record(4, 4, start + [row(31, 31, 0, 112)]) + struct.pack('<4I', 5, 4, 1, 4) + b''.join(
                struct.pack('>H', i) + r for i, r in enumerate(start + [row(31, 31, 0, 112), row(32, 32, 0, 113)]))
            self.put_raw(run, two)
            self.assertIn('112@4 113@4', self.report(capture_report.sweep, [run], 4, '800eff20'))
            # A watch width with trailing text still reads as 4, as in the frontend.
            (run / 'tape.txt').write_text('watch 800EFF20 4#seed\ntap tick raw\n10 0 0 0\n')
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n1,1,800eff20,4,222\n')
            self.seal(run, 'sweep')
            self.assertIn('seed=546', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            # A nonnumeric step on the first live row, a non-ASCII digit, or a row without a table: skipped.
            for bad in (csv_text.replace('\n1,1,0,1,', '\nx,1,0,1,'), csv_text.replace('\n1,1,0,1,', '\n\u00b2,1,0,1,'),
                        csv_text + '9,9\n'):
                (run / 'ticks.csv').write_text(bad)
                self.seal(run, 'sweep')
                bad_batch = self.report(capture_report.sweep, [run, late], 3, '800eff20')
                self.assertIn('ticks.csv has unreadable rows; skipped', bad_batch)
                self.assertIn('sweep late:', bad_batch)
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # Unreadable tick rows (a partial final row) skip the run, and the batch goes on.
            (run / 'ticks.csv').write_text(csv_text + '7')
            self.seal(run, 'sweep')
            rows_batch = self.report(capture_report.sweep, [run, late], 3, '800eff20')
            self.assertIn('ticks.csv has unreadable rows; skipped', rows_batch)
            self.assertIn('sweep late:', rows_batch)
            (run / 'ticks.csv').write_text(csv_text)
            self.put_raw(run, raw)
            # Two types first seen in the same sample are one unordered group.
            self.put_raw(run, raw + record(4, 4, start + [row(31, 31, 0, 112), row(32, 32, 0, 113)]))
            self.assertIn('{112,113}@4', self.report(capture_report.sweep, [run], 4, '800eff20'))
            self.put_raw(run, raw)
            # A null provenance map: malformed, skipped.
            manifest = json.loads((run / 'manifest.json').read_text())
            manifest['state_input_source'] = None
            (run / 'manifest.json').write_text(json.dumps(manifest))
            self.assertIn('malformed state_input_source; skipped', self.report(capture_report.sweep, [run], 3, '800eff20'))
            self.seal(run, 'sweep')
            # A width written +4 is the frontend's four-byte watch too.
            (run / 'tape.txt').write_text('watch 800EFF20 +4\ntap tick raw\n10 0 0 0\n')
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n1,1,800eff20,4,222\n')
            self.seal(run, 'sweep')
            self.assertIn('seed=546', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            # A binary file named watch.csv (a dump), digest-matching: seed unknown, batch goes on.
            (run / 'watch.csv').write_bytes(bytes(range(256)) * 4)
            self.seal(run, 'sweep')
            binary = self.report(capture_report.sweep, [run, late], 3, '800eff20')
            self.assertIn('seed=None', binary)
            self.assertIn('sweep late:', binary)
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n1,1,800eff20,4,222\n')
            self.seal(run, 'sweep')
            # --opening warns when the tick log cannot be trusted to show restarts.
            (run / 'ticks.csv').write_text(csv_text + '\n')
            self.assertIn('may merge two matches', self.report(capture_report.opening, run, 3))
            (run / 'ticks.csv').write_text(csv_text)
            self.seal(run, 'sweep')
            # A loaded state without a recorded producer stays out of the totals unless --legacy.
            (run / 'tape.txt').write_text('load lobby.state 0\nwatch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            (run / 'input').mkdir(exist_ok=True)
            (run / 'input' / 'lobby.state').write_bytes(b'lobby')
            lobby = hashlib.sha256(b'lobby').hexdigest()
            for bad in (None, 'f' * 64):   # a null digest, or one the staged file does not match
                self.seal(run, 'sweep', state_input_sha256={'lobby.state': bad})
                self.assertIn('not verified', self.report(lambda: capture_report.sweep([run], 3, '800eff20', legacy=True)))
            self.seal(run, 'sweep', state_input_sha256={'lobby.state': lobby})
            self.assertIn('has no recorded producer', self.report(capture_report.sweep, [run], 3, '800eff20'))
            # An explicit null producer record is present, so --legacy does not waive it.
            self.seal(run, 'sweep', state_input_sha256={'lobby.state': lobby}, state_input_source={'lobby.state': None})
            self.assertIn('left out of the totals', self.report(lambda: capture_report.sweep([run], 3, '800eff20', legacy=True)))
            # A malformed producer record is not a missing one: --legacy does not admit it.
            self.seal(run, 'sweep', state_input_sha256={'lobby.state': lobby},
                      state_input_source={'lobby.state': {'run_id': 'r', 'binary_sha256': 'nope'}})
            self.assertIn('left out of the totals', self.report(lambda: capture_report.sweep([run], 3, '800eff20', legacy=True)))
            self.seal(run, 'sweep', state_input_sha256={'lobby.state': lobby})
            legacy = self.report(lambda: capture_report.sweep([run], 3, '800eff20', legacy=True))
            self.assertIn('counted under --legacy', legacy)
            self.assertNotIn('left out of the totals', legacy)
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            # The same capture twice (a symlink, or a copy with its run id): counted once.
            (pathlib.Path(temp) / 'alias').symlink_to(run)
            twice = self.report(capture_report.sweep, [run, pathlib.Path(temp) / 'alias'], 3, '800eff20')
            self.assertIn('the same capture as an earlier counted argument', twice)
            # A leading `tap off` changes nothing: the run still counts.
            (run / 'tape.txt').write_text('tap off\nwatch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            self.assertNotIn('left out of the totals', self.report(capture_report.sweep, [run], 3, '800eff20'))
            (run / 'tape.txt').write_text('watch 800EFF20 4\ntap tick raw\n10 0 0 0\n')
            self.seal(run, 'sweep')
            # A missing raw file is reported and skipped, not fatal.
            (run / 'ticks.raw').unlink()
            self.assertIn('no ticks.raw', self.report(capture_report.sweep, [run], 10, '800eff20'))
            self.put_raw(run, raw)
            # A half-width sample or a tape that turns watches off gives an unknown seed.
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n1,1,800eff20,2,1\n')
            self.seal(run, 'sweep')
            self.assertIn('seed=None', self.report(capture_report.sweep, [run], 10, '800eff20'))
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n1,1,800eff20,4,222\n')
            self.seal(run, 'sweep')
            (run / 'tape.txt').write_text('watch 800EFF20 4\nwatch off\n10 0 0 0\n')
            self.seal(run, 'sweep')
            self.assertIn('seed=None', self.report(capture_report.sweep, [run], 10, '800eff20'))
            self.assertIn('owner 0: 111@3', text)
            self.assertNotIn('7@2', text)


if __name__ == '__main__':
    unittest.main()
