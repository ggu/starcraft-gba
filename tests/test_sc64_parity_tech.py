# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of the tech-tree production labs (tools/sc64_parity_tech.py) on synthetic
captures and traces: no ROM, capture or GBA build needed."""
import pathlib
import struct
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import parity_decode as pd  # noqa: E402
import sc64_parity_metrics as metrics  # noqa: E402
import sc64_parity  # noqa: E402
import sc64_parity_tech as tech  # noqa: E402

LAYOUT = pd.Layout()
UNIT = LAYOUT.doc['unit_fields']
BANKS = LAYOUT.doc['globals']['banks']['arrays']
HUMAN, COMPUTER = 3, 1


def row(typ, owner, x, y, done, queue=()):
    data = bytearray(312)
    put = lambda name, fmt, v: struct.pack_into(fmt, data, int(UNIT[name]['offset'], 16), v)
    put('type', '>H', typ)
    put('owner', '>B', owner)
    put('x', '>H', x)
    put('y', '>H', y)
    put('status', '>I', (1 << 31) if done else 0)
    slots = list(queue) + [tech.QUEUE_EMPTY] * (tech.QUEUE_SLOTS - len(queue))
    struct.pack_into('>5H', data, tech.QUEUE_OFFSET, *slots)
    return bytes(data)


def globals_bytes(banks):
    data = bytearray(0xC0)
    for name, owner, value in banks:
        struct.pack_into('>I', data, int(BANKS[name]['offset'], 16) + 4 * owner, value)
    return bytes(data)


def record(sf, rows, minerals, gathered=50, gas=10000, gas_gathered=0):
    return {'table': True, 'paused': False, 'sf': sf, 'step': sf * 3, 'rows': rows,
            'globals': globals_bytes([('minerals', HUMAN, minerals), ('minerals_gathered', HUMAN, gathered),
                                      ('gas', HUMAN, gas), ('gas_gathered', HUMAN, gas_gathered),
                                      ('minerals', COMPUTER, 0)])}


def capture(records):
    cap = object.__new__(pd.Capture)
    cap.layout = LAYOUT
    cap.ticks = records
    return cap


class N64Test(unittest.TestCase):
    def setUp(self):
        start = {0: row(106, HUMAN, 1760, 240, True), 1: row(7, HUMAN, 1700, 200, True),
                 2: row(41, COMPUTER, 100, 100, True), 21: row(188, 11, 1984, 352, True)}
        frames = []
        # Frame 0: the edited bank. Frame 5: a Barracks is placed (150 debited). Frame 8: the
        # geyser's row becomes a Refinery (100 debited; 8 minerals gathered the same tick).
        # Frame 10: an SCV enters the Command Center's queue (50) and its row is born.
        frames.append(record(0, dict(start), 10000))
        rows = {**start, **{74: row(111, HUMAN, 1664, 464, False)}}
        frames.append(record(5, rows, 9850))
        rows = {**rows, **{21: row(110, HUMAN, 1984, 352, False)}}
        frames.append(record(8, rows, 9758, gathered=58))
        rows = {**rows, **{0: row(106, HUMAN, 1760, 240, True, queue=[7]), 75: row(7, HUMAN, 1760, 300, False)}}
        frames.append(record(10, rows, 9708, gathered=58))
        rows = {**rows, **{75: row(7, HUMAN, 1760, 300, True), 0: row(106, HUMAN, 1760, 240, True)}}
        frames.append(record(311, rows, 9708, gathered=58))
        rows = {**rows, **{21: row(110, HUMAN, 1984, 352, True)}}
        frames.append(record(610, rows, 9708, gathered=58))
        rows = {**rows, **{74: row(111, HUMAN, 1664, 464, True)}}
        frames.append(record(1207, rows, 9708, gathered=58))
        self.cap = capture(frames)

    def test_products_queue_entries_and_corrected_debits(self):
        m = tech.n64_production(self.cap)
        self.assertEqual(m['human'], HUMAN)
        got = {(p['raw'], p['born'], p['done']) for p in m['products']}
        # The Refinery that took the geyser's row is a product; the starting rows are not.
        self.assertEqual(got, {(111, 5, 1207), (110, 8, 610), (7, 10, 311)})
        self.assertEqual([(q['raw'], q['sf'], q['x'], q['y']) for q in m['queued']], [(7, 10, 1760, 240)])
        # The Refinery's debit is corrected for the 8 minerals gathered in the same tick.
        self.assertEqual([d[:2] for d in m['debits']], [(5, 150), (8, 100), (10, 50)])

    def test_debits_are_found_by_minerals_alone(self):
        start = {1: row(7, HUMAN, 1700, 200, True)}
        cap = capture([record(0, start, 10000), record(1, start, 10000, gas=9900), record(2, start, 9950, gas=9900)])
        # A gas-only change is no debit; the mineral debit reads the gas change of its own tick (none).
        self.assertEqual(tech.n64_production(cap)['debits'], [(2, 50, 0)])

    def test_one_human_with_the_edited_bank(self):
        cap = capture([record(0, {1: row(7, HUMAN, 1, 1, True)}, 400)])
        with self.assertRaisesRegex(SystemExit, 'edited bank'):
            tech.n64_production(cap)

    def test_add_on_parent(self):
        m = {'products': [{'raw': 113, 'born': 0, 'done': 10, 'x': 1664, 'y': 688},
                          {'raw': 116, 'born': 0, 'done': 10, 'x': 1856, 'y': 688}],
             'start_buildings': [{'raw': 106, 'born': 0, 'done': 0, 'x': 1760, 'y': 240}]}
        # The Machine Shop sits at its Factory's centre plus (96, 16); the Science Facility next door is nearer.
        self.assertEqual(tech.nearest_building(m, {'raw': 120, 'born': 20, 'x': 1760, 'y': 704}), (1664, 688))
        self.assertEqual(tech.nearest_building(m, {'raw': 107, 'born': 20, 'x': 1856, 'y': 256}), (1760, 240))


TRACE = """#sc64-parity-trace 1
S 1 0 0
+B 0 0 0
B 0 0 -1 0 -1 880 120 1500 1500 0 0 0 0 0 0 -1 -1 -1 -1 -1 3 0
P 0 10000 10000 8 20
S 100 80 0
+B 5 2 0
B 5 2 -1 0 -1 832 232 100 1000 0 1 1 0 0 0 -1 -1 -1 -1 -1 3 0
P 0 9850 10000 8 20
S 1600 1280 0
B 5 2 -1 0 -1 832 232 1000 1000 0 0 0 0 0 0 -1 -1 -1 -1 -1 3 0
S 1700 1360 0
B 5 2 -1 0 -1 832 232 1000 1000 0 0 0 0 1 1 4 -1 -1 -1 -1 3 0
P 0 9800 10000 8 20
S 2150 1720 0
B 5 2 -1 0 -1 832 232 1000 1000 0 0 0 0 0 0 -1 -1 -1 -1 -1 3 0
+U 9 4 0
U 9 4 -1 0 -1 840 250
Z overflow=0 write_errors=0
"""


class GbaTest(unittest.TestCase):
    def test_durations_and_debits_from_a_trace(self):
        with tempfile.TemporaryDirectory() as tmp:
            trace = pathlib.Path(tmp) / 't.trace'
            trace.write_text(TRACE)
            (pathlib.Path(tmp) / 't.trace.gather').write_text('# gather\n')
            unit_raw = [-1, -1, -1, -1, 0]          # engine unit 4 is a Marine
            building_raw = [106, -1, 111]           # engine building 2 is a Barracks
            g = tech.read_gba(str(trace), str(trace) + '.gather', 0, unit_raw, building_raw)
        self.assertEqual(g['done'], [{'raw': 111, 'born': 100, 'done': 1600}])
        # The Marine's training runs from its producer's progress 1 to the step that producer
        # completes its head and a Marine is born.
        self.assertEqual(g['train'], [{'raw': 0, 'start': 1700, 'born': 2150}])
        self.assertEqual(g['debits'], {100: (150, 0), 1700: (50, 0)})

    def test_a_birth_two_producers_could_have_made_is_left_out(self):
        two = TRACE.replace('S 2150 1720 0\n', 'S 2150 1720 0\nB 6 2 -1 0 -1 900 232 1000 1000 0 0 0 0 0 0 '
                            '-1 -1 -1 -1 -1 3 0\n').replace(
            'S 1700 1360 0\n', 'S 1700 1360 0\nB 6 2 -1 0 -1 900 232 1000 1000 0 0 0 0 1 1 4 -1 -1 -1 -1 3 0\n')
        with tempfile.TemporaryDirectory() as tmp:
            trace = pathlib.Path(tmp) / 't.trace'
            trace.write_text(two)
            (pathlib.Path(tmp) / 't.trace.gather').write_text('# gather\n')
            g = tech.read_gba(str(trace), str(trace) + '.gather', 0, [-1, -1, -1, -1, 0], [106, -1, 111])
            self.assertEqual(g['train'], [])
            # The gather log is required: no income correction is no measurement.
            (pathlib.Path(tmp) / 't.trace.gather').unlink()
            with self.assertRaises(FileNotFoundError):
                tech.read_gba(str(trace), str(trace) + '.gather', 0, [0], [106])

    def test_a_debit_is_claimed_by_one_command_only(self):
        debits = {100: (50, 0), 200: (75, 0)}
        one = [{'kind': 2, 'raw': 0, 'state': 1, 'issued': 100}, {'kind': 2, 'raw': 2, 'state': 1, 'issued': 200}]
        self.assertEqual(tech.gba_costs(one, debits), {0: (50, 0), 2: (75, 0)})
        same = [{'kind': 2, 'raw': 0, 'state': 1, 'issued': 100}, {'kind': 2, 'raw': 32, 'state': 1, 'issued': 101}]
        self.assertEqual(tech.gba_costs(same, debits), {})
        # The bank edit (kind 0) and commands not taken claim nothing.
        quiet = [{'kind': 0, 'raw': 0, 'state': 1, 'issued': 100}, {'kind': 2, 'raw': 0, 'state': 3, 'issued': 0}]
        self.assertEqual(tech.gba_costs(quiet, debits), {})

    def test_mark_names(self):
        self.assertEqual(tech.mark_product('supply_depot2'), 109)
        self.assertEqual(tech.mark_product('scv'), 7)
        self.assertEqual(tech.mark_product('siege_tank'), 5)
        self.assertIsNone(tech.mark_product('command'))

    def test_absent_and_refused_products_are_markers(self):
        golden = {'observations': [
            {'check': 'tech.train_time', 'subject': 'Valkyrie', 'gating': True},
            {'check': 'tech.train_time', 'subject': 'Science Vessel', 'gating': True},
            {'check': 'tech.cost', 'subject': 'Marine', 'gating': True}]}
        report = {'match': {'owners': [{'owner': 0, 'human': 1}]}, 'unit_raw': [0], 'building_raw': [106],
                  'tech': [{'step': 5, 'kind': 2, 'raw': 58, 'state': 2, 'issued': 0, 'slot': -1},
                           {'step': 6, 'kind': 2, 'raw': 9, 'state': 3, 'issued': 0, 'slot': -1},
                           {'step': 7, 'kind': 2, 'raw': 0, 'state': 1, 'issued': 7, 'slot': 0}]}
        with tempfile.TemporaryDirectory() as tmp:
            trace = pathlib.Path(tmp) / 't.trace'
            trace.write_text('#sc64-parity-trace 1\nS 7 5 0\nP 0 100 0 0 0\nS 8 6 0\nP 0 50 0 0 0\n')
            (pathlib.Path(tmp) / 't.trace.gather').write_text('# gather\n')
            series = tech.gba_series(str(trace), report)
            out, err = tech.make_measure_gba('tech-terran')(series, golden, report)
        self.assertIsNone(err)
        values = out['values']
        self.assertEqual(values[('tech.train_time', 'Valkyrie')], metrics.ABSENT)
        self.assertEqual(values[('tech.train_time', 'Science Vessel')], metrics.REFUSED)
        self.assertEqual(values[('tech.cost', 'Marine')], 50)

    def test_refused_is_a_marker_in_the_judge(self):
        obs = {'check': 'tech.train_time', 'subject': 'Science Vessel', 'value': 1201, 'lo': 1199, 'hi': 1203,
               'gating': True, 'tolerance': '+-2', 'provenance': 'n64-staged', 'layout_fields': [], 'n_runs': 2}
        golden = {'observations': [obs], 'gba': {}}
        key = 'tech-terran:tech.train_time:Science Vessel'
        verdict = lambda value, base: sc64_parity.judge('tech-terran', golden, {('tech.train_time', 'Science Vessel'): value},
                                                        {}, base)[0]['verdict']
        refused = {key: {'gba': metrics.REFUSED}}
        self.assertEqual(verdict(metrics.REFUSED, refused), 'KNOWN')            # the same marker
        self.assertEqual(verdict(metrics.REFUSED, {}), 'REGRESSION')            # a new refusal
        self.assertEqual(verdict(metrics.ABSENT, refused), 'REGRESSION')        # refused, now absent
        self.assertEqual(verdict(metrics.REFUSED, {key: {'gba': 1100}}), 'REGRESSION')   # measured before
        self.assertEqual(verdict(1200.0, refused), 'IMPROVED')                  # made, inside the band
        self.assertEqual(sc64_parity.fmt(metrics.REFUSED), 'refused')

    def test_runner_arguments(self):
        params = {'seed': 123, 'starve': [10, 900], 'tech_cmds': [[2, 0, 0, 10000, 10000], [364, 1, 109, 824, 176]]}
        args = tech.gba_args('tech-terran', params, '/tmp/x.trace')
        self.assertEqual(args[:4], ['--human', 't', '--bot', 'z'])
        self.assertIn('--starve', args)
        self.assertEqual(args[-4:], ['--tcmd', '2,0,0,10000,10000', '--tcmd', '364,1,109,824,176'])
        self.assertIn('tech-terran', metrics.SCENARIOS)
        self.assertIn(metrics.REFUSED, metrics.MARKERS)


if __name__ == '__main__':
    unittest.main()
