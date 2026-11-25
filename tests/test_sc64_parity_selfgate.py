# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks for tools/sc64_parity_selfgate.py's parsing helpers."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
SPEC = importlib.util.spec_from_file_location("sc64_parity_selfgate", ROOT / "tools" / "sc64_parity_selfgate.py")
selfgate = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(selfgate)


class ParitySelfgateTest(unittest.TestCase):
    def test_profiles_need_a_known_scenario(self):
        self.assertEqual(selfgate.parse_profiles(["terran:t1", "zerg-z10:skirmish"]),
                         [("terran", "t1"), ("zerg-z10", "skirmish")])
        with self.assertRaises(SystemExit):
            selfgate.parse_profiles(["terran"])
        with self.assertRaises(SystemExit):
            selfgate.parse_profiles(["terran:t2"])
        with self.assertRaises(SystemExit):
            selfgate.parse_profiles(["../x:t1"])

    def test_dependency_file_lists_each_path_once(self):
        with tempfile.TemporaryDirectory() as scratch:
            dep = Path(scratch) / "run.d"
            dep.write_text("build/parity/x: tests/sc64_parity_run.c \\\n"
                           "  starcraft/game.c .local/sc64/a/generated/h.h \\\n"
                           "  starcraft/game.c\n")
            paths = selfgate.dependency_paths(dep)
        names = [str(p.relative_to(ROOT)) for p in paths]
        self.assertEqual(names, ["tests/sc64_parity_run.c", "starcraft/game.c",
                                 ".local/sc64/a/generated/h.h"])

    def test_record_shapes(self):
        self.assertTrue(selfgate.record_ok(b"S 1 0 6 1 1 0 0 0 1 -1 0"))
        self.assertFalse(selfgate.record_ok(b"S 1"))
        self.assertFalse(selfgate.record_ok(b"D"))
        self.assertTrue(selfgate.record_ok(b"V 3 0"))
        self.assertFalse(selfgate.record_ok(b"V 3 x"))
        self.assertTrue(selfgate.record_ok(b"FV 4 4 ff00"))
        self.assertFalse(selfgate.record_ok(b"FV 128 96 ff00"))
        self.assertTrue(selfgate.record_ok(b"I move 0 slot=2 x=1 y=2"))

    def test_rom_flags_keep_preprocessor_options(self):
        line = ("arm-none-eabi-gcc -mcpu=arm7tdmi -O2 -DA=1 '-DB=\"x y\"' -Iinc -isystem/s -imacros m.h "
                "-include h.h -Wall game.c -o build/x.elf")
        self.assertEqual(selfgate.rom_flags(line, "build/x.elf"),
                         "-DA=1 '-DB=\"x y\"' -Iinc -isystem/s -imacros m.h -include h.h")

    def test_make_escapes(self):
        self.assertEqual(selfgate.make_tokens(r"a\ b.h c$$d.h  e\#f.h"), ["a b.h", "c$d.h", "e#f.h"])


if __name__ == "__main__":
    unittest.main()
