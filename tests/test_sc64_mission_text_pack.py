# SPDX-License-Identifier: GPL-3.0-or-later
"""Mission text packing rules, on synthetic rows; no ROM or game text."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_mission_text_pack', ROOT / 'tools/sc64_mission_text_pack.py')
pack = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(pack)


def action(index, opcode, text='', unit=0, flags=0, string_id=1):
    return {'index': index, 'opcode': opcode, 'text': text, 'unit_type': unit,
            'flags': flags, 'string_id': string_id}


class MissionTextPackTests(unittest.TestCase):
    def test_rows_keep_source_breaks_and_fit_each_page(self):
        advances = {ch: 6 for ch in range(32, 127)}
        text = 'HEADER:\r\n- one line\r\n' + ' '.join(['word'] * 40)
        pages = pack.wrap_pages(text, advances).split(pack.PAGE_BREAK)
        self.assertEqual(pages[0].split(pack.ROW_BREAK)[:2], ['HEADER:', '- one line'])
        for page in pages:
            rows = page.split(pack.ROW_BREAK)
            self.assertLessEqual(len(rows), pack.TEXT_ROWS)
            for row in rows:
                self.assertLessEqual(len(row) * 6, pack.TEXT_PIXEL_WIDTH)
        joined = ' '.join(' '.join(pages).replace(pack.ROW_BREAK, ' ').split())
        self.assertEqual(joined, ' '.join(text.split()))

    def test_only_enabled_text_rows_are_presented(self):
        rows = pack.select_rows([{'record': 3, 'actions': [
            action(0, 4),
            action(1, pack.TRANSMISSION, 'spoken', 19, string_id=7),
            action(2, pack.DISPLAY_TEXT, 'shown', string_id=8),
            action(3, pack.DISPLAY_TEXT, 'disabled comment', flags=pack.ACTION_DISABLED),
        ]}])
        self.assertEqual([(r['trigger'], r['action'], r['opcode'], r['string_id']) for r in rows],
                         [(3, 1, pack.TRANSMISSION, 7), (3, 2, pack.DISPLAY_TEXT, 8)])
        with self.assertRaisesRegex(ValueError, 'empty row'):
            pack.select_rows([{'record': 1, 'actions': [action(0, pack.DISPLAY_TEXT, '  ')]}])

    def test_speakers_map_to_real_portraits_or_the_unit_icon(self):
        self.assertEqual(len(pack.SPEAKERS), 26)
        for unit, (name, portrait) in pack.SPEAKERS.items():
            self.assertTrue(name)
            self.assertLess(unit, 256)
            if portrait is not None:
                self.assertIn(portrait, range(pack.PORTRAIT_COUNT))
        self.assertEqual(pack.speaker_of(pack.DISPLAY_TEXT, 19), pack.ADVISOR_UNIT)
        self.assertEqual(pack.speaker_of(pack.TRANSMISSION, 19), 19)
        with self.assertRaisesRegex(ValueError, 'no mapping'):
            pack.speaker_of(pack.TRANSMISSION, 1)

    def test_header_shares_repeated_text_and_counts_missions(self):
        row = {'trigger': 1, 'action': 2, 'opcode': pack.TRANSMISSION, 'string_id': 5, 'unit': 19,
               'text': 'same words'}
        other = dict(row, action=3, unit=20)
        header, report = pack.build_header([[row, other], [], [dict(row, string_id=9)]])
        self.assertEqual(report['rows'], 3)
        self.assertEqual(report['distinct_texts'], 1)
        self.assertEqual(report['rows_by_mission'], [2, 0, 1])
        self.assertEqual(report['max_rows_in_one_trigger'], 2)
        self.assertIn('sc64_mission_text_first[SC64_MISSION_TEXT_MISSION_COUNT + 1]', header)
        self.assertIn('0, 2, 2, 3', header)
        self.assertEqual(header.count('same words'), 1)

    def test_portrait_colours_map_to_the_nearest_world_colour(self):
        white, red, blue = 0x7FFF, 0x001F, 0x7C00
        world = [red, blue, white] + [0] * 221
        table = pack.portrait_lut([0x7BDE, 0x0019, 0x6800] + [0] * 221, world)
        self.assertEqual(table[:3], [34, 32, 33])
        self.assertTrue(all(32 <= entry <= 255 for entry in table))
        header, _ = pack.build_header([[]], lut=table)
        self.assertIn('sc64_mission_text_portrait_lut[224]', header)
        with self.assertRaisesRegex(ValueError, 'one entry per'):
            pack.build_header([[]], lut=[32])

    def test_page_break_reaches_c_as_an_escape(self):
        long_row = {'trigger': 1, 'action': 2, 'opcode': pack.DISPLAY_TEXT, 'string_id': 5, 'unit': 0,
                    'text': ' '.join(['word'] * 200)}
        header, _ = pack.build_header([[long_row]])
        self.assertIn('"\\f"', header)
        self.assertNotIn('\f', header)
        self.assertNotIn('\v', header)


if __name__ == '__main__':
    unittest.main()
