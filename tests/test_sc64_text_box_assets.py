#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of the text box importer: wrapping, row selection, output."""
import pathlib
import struct
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT))
from tools import sc64_text_box_assets as assets

ADVANCES = {ch: 6 for ch in range(32, 127)}


class Extractor:
    @staticmethod
    def _chk_string(table, string_id):
        return f"string {string_id}"


def action(opcode, string_id, unit=0, flags=0, location=1):
    raw = bytearray(assets.ACTION_SIZE)
    struct.pack_into("<II", raw, 0, location, string_id)
    struct.pack_into("<H", raw, 24, unit)
    raw[26], raw[28] = opcode, flags
    return bytes(raw)


def trigger(*actions):
    record = bytearray(assets.TRIGGER_SIZE)
    for slot, raw in actions:
        record[assets.ACTION_BASE + slot * assets.ACTION_SIZE:
               assets.ACTION_BASE + slot * assets.ACTION_SIZE + len(raw)] = raw
    return bytes(record)


class Wrap(unittest.TestCase):
    def test_lines_fit_and_pages_hold_four(self):
        text = " ".join(["word"] * 80)
        pages = assets.wrap_paragraphs(text, ADVANCES)
        self.assertGreater(len(pages), 1)
        for page in pages:
            lines = page.split("\n")
            self.assertLessEqual(len(lines), assets.TEXT_ROWS)
            for line in lines:
                self.assertLessEqual(len(line) * 6, assets.TEXT_PIXEL_WIDTH)
        self.assertEqual(" ".join(p.replace("\n", " ") for p in pages), text)

    def test_source_line_breaks_stay(self):
        self.assertEqual(assets.wrap_paragraphs("one\r\ntwo", ADVANCES), ["one\ntwo"])

    def test_blank_lines_do_not_make_empty_lines(self):
        self.assertEqual(assets.wrap_paragraphs("one\r\n\r\ntwo", ADVANCES), ["one\ntwo"])

    def test_overlong_word_is_rejected(self):
        with self.assertRaises(ValueError):
            assets.wrap_paragraphs("x" * 40, ADVANCES)

    def test_empty_text_is_rejected(self):
        with self.assertRaises(ValueError):
            assets.wrap_paragraphs(" \r\n ", ADVANCES)

    def test_control_bytes_are_dropped(self):
        self.assertEqual(assets.printable("a\x03b\tc\r\nd\x7f"), "ab c\r\nd")


class Rows(unittest.TestCase):
    def test_selection(self):
        payload = trigger(
            (0, action(assets.OP_TRANSMISSION, 5, unit=19)),
            (1, action(8, 6)),
            (2, action(assets.OP_DISPLAY_TEXT, 7, location=0)),
            (3, action(assets.OP_TRANSMISSION, 8, flags=assets.ACTION_DISABLED)),
            (4, action(assets.OP_TRANSMISSION, 9, location=0)),
            (5, action(assets.OP_DISPLAY_TEXT, 0)),
        ) + trigger((7, action(assets.OP_DISPLAY_TEXT, 10)))
        rows = assets.action_rows(payload, b"", Extractor)
        self.assertEqual([(r["trigger"], r["action"], r["string_id"]) for r in rows],
                         [(0, 0, 5), (0, 2, 7), (1, 7, 10)])
        self.assertEqual(rows[0]["unit"], 19)

    def test_bad_trigger_size(self):
        with self.assertRaises(ValueError):
            assets.action_rows(b"x", b"", Extractor)

    def test_unit_name_overrides_honour_use_default(self):
        settings = bytearray(228 + 228 * 13 + 228 * 2 + 228)
        settings[2] = 1
        struct.pack_into("<H", settings, 228 + 228 * 13 + 2 * 2, 30)
        struct.pack_into("<H", settings, 228 + 228 * 13 + 3 * 2, 31)
        names = assets.unit_name_overrides({b"UNIS": bytes(settings)}, Extractor)
        self.assertEqual(names, {3: "string 31"})
        self.assertEqual(assets.unit_name_overrides({}, Extractor), {})

    def test_unit_name_table(self):
        table = struct.pack("<H2H", 2, 6, 9) + b"ab\0cd\0"
        self.assertEqual(assets.table_string(table, 1), "cd")
        with self.assertRaises(ValueError):
            assets.table_string(table, 2)


class Header(unittest.TestCase):
    def test_missions_without_rows_stay_in_the_index(self):
        row = {"string_id": 5, "first_page": 0, "trigger": 1, "action": 2, "head": 0,
               "speaker": 0, "page_count": 1}
        text = assets.build_header([bytes(assets.HEAD_WIDTH * assets.HEAD_HEIGHT)], ["name"],
                                   ["page"], [[row], []])
        self.assertIn("{5, 0, 1, 2, 0, 0, 1},", text)
        self.assertIn("{0, 0},", text)
        self.assertIn("sc64_text_box_missions[2]", text)
        self.assertIn('"page"', text)


if __name__ == "__main__":
    unittest.main()
