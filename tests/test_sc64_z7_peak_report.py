# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free diagnostic context and incomplete-output checks."""
import importlib.util
from pathlib import Path
import unittest

SPEC = importlib.util.spec_from_file_location('z7_peak', Path(__file__).resolve().parents[1] / 'tools/sc64_z7_peak_report.py')
peak = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(peak)


def record(call=0, phase=2, advancing=0):
    a = f'Z7D PEAK call={call} tick=136058 phase={phase} before=0 frame=134787 screen0=0 screen=1 held=512 total=158000000 update=157000000 advancing={advancing} end=1'
    b = f'Z7D STAGES call={call} tick=136058 detailed=0 orders=0 sight=0 mission=0 production=0 state=0 defenses=0 research=0 remainder=0 end=1'
    return a + '\n' + b


class PeakReportTests(unittest.TestCase):
    def test_home_load_is_not_gameplay(self):
        result = peak.summarize('[WARN] ' + record() + '\nZ7D STOP calls=1 tick=136059 end=1')
        self.assertEqual(result['max_total']['context'], 'HOME load')
        self.assertEqual(result['stop']['calls'], 1)
        self.assertFalse(result['max_total']['detailed_stages'])

    def test_gameplay_context_and_standard_log_are_distinct(self):
        text = record(phase=1, advancing=1).replace('screen0=0', 'screen0=1')
        result = peak.summarize('Z7 DONE cursor=4\n' + text)
        self.assertEqual(result['max_update']['context'], 'active gameplay')

    def test_truncated_or_missing_partner_rejected(self):
        for text in (record().replace(' end=1', '', 1), record().splitlines()[0],
                     record().splitlines()[1], record().replace('update=157000000', 'update=99999999999')):
            with self.subTest(text=text), self.assertRaises(ValueError):
                peak.summarize(text)

    def test_final_advancing_tick_is_a_result_transition(self):
        # Real winning calls still advance the simulation before autosaving
        # and publishing the result; advancing=1 alone is not active play.
        text = record(phase=1, advancing=1).replace('screen0=0 screen=1', 'screen0=1 screen=3')
        self.assertEqual(peak.summarize(text)['max_total']['context'], 'mission result transition')

    def test_explicit_pause_save_context_takes_precedence(self):
        text = record(phase=3).replace('screen0=0 screen=1', 'screen0=1 screen=2')
        self.assertEqual(peak.summarize(text)['max_total']['context'], 'pause-menu save')

    def test_duplicate_and_mismatched_ticks_rejected(self):
        for text in (record() + '\n' + record(), record().replace('STAGES call=0 tick=136058', 'STAGES call=0 tick=136059')):
            with self.subTest(text=text), self.assertRaises(ValueError):
                peak.summarize(text)


if __name__ == '__main__':
    unittest.main()
