# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'experiments' / 'ares-headless'))
from result_trace_decode import LIMIT, POINTS, ROW, inspect_cadence, read_trace

PCS = {event: pc for pc, event in POINTS.items()}


def row(event, step=4200, old=0, owner=0, value=0):
    return (step, PCS[event], owner, value, old, 0, 0, 0, 0, 0)


def scan(step):
    return [row('countdown', step), row('scan', step, 0xffff)]


class ResultTraceTests(unittest.TestCase):
    def test_ordinary_scan_and_reset_split_intervals(self):
        rows = scan(4200)
        rows += [row('owner', owner=i) for i in (0, 1, 2, 3)]
        rows += [row('publish', owner=0, value=2), row('relink', owner=0x802d9160, value=11), row('published')]
        rows += [row('countdown', 4201+i, old=n) for i, n in enumerate(range(30, 0, -1))]
        rows += scan(4231)
        rows += [row('countdown', 4232, old=30), row('reset', 4232, old=29), row('countdown', 4233, old=1)]
        rows += scan(4234)
        with tempfile.TemporaryDirectory() as d:
            trace = Path(d) / 'trace.bin'
            trace.write_bytes(b''.join(ROW.pack(*r) for r in rows))
            report = inspect_cadence(read_trace(trace, 4200, 4240))
        self.assertEqual([(i['kind'], i['decrements']) for i in report['intervals']], [('ordinary', 31), ('reset', 3)])
        self.assertEqual(report['scans'][0]['owners'], [0, 1, 2, 3])
        self.assertTrue(report['scans'][0]['relinks'][0]['during_result_pass'])
        self.assertEqual(report['scans'][0]['ordering_issues'], [])
        self.assertEqual(report['scans'][1]['ordering_issues'], [])  # no owner list, no result pass

    def test_missing_anchor_and_out_of_order_dispatch_are_visible(self):
        rows = [row('scan', old=0xffff), row('publish', owner=0, value=2), row('owner', owner=1)] + scan(4203)
        report = inspect_cadence(rows)
        self.assertFalse(report['scans'][0]['underflow_anchor'])
        self.assertEqual(report['intervals'][0]['kind'], 'unknown_anchor')
        self.assertIn('owner dispatch after result publication', report['scans'][0]['ordering_issues'])
        self.assertIn('missing result-pass return', report['scans'][0]['ordering_issues'])

    def test_publication_and_lack_of_return_cannot_hide_a_partial_pass(self):
        rows = scan(4200) + [row('publish', value=2)] + scan(4231)
        self.assertEqual(inspect_cadence(rows)['scans'][0]['ordering_issues'],
                         ['result publication without owner dispatch', 'missing result-pass return'])

    def test_malformed_and_unbounded_files_are_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            trace = Path(d) / 'trace.bin'
            for blob in (b'', b'\0' * (LIMIT + 1), ROW.pack(*row('countdown', 4240)),
                         ROW.pack(*row('countdown', 4205)) + ROW.pack(*row('countdown', 4201))):
                with self.subTest(length=len(blob)):
                    trace.write_bytes(blob)
                    with self.assertRaises(ValueError):
                        read_trace(trace, 4200, 4240)
            trace.write_bytes(ROW.pack(*row('countdown')))
            for begin, end in ((-1, 1), (4200, 4200), (4200, 4457)):
                with self.subTest(begin=begin, end=end), self.assertRaises(ValueError):
                    read_trace(trace, begin, end)


if __name__ == '__main__':
    unittest.main()
