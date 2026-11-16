"""Source-only fixture for the derived static mineral bank."""
# SPDX-License-Identifier: GPL-3.0-or-later

import pathlib
import re
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "tools"))
from sc64_static_mineral_words import SOURCES, emit, frame_data


def source_bank() -> str:
    out = []
    for i, name in enumerate(SOURCES, 1):
        out += [
            f"static const u8 {name}_pixels[6] = {{\n0,{i},0,\n"
            f"{i + 10},0,{i + 20},\n}};",
            f"static const Sc64SpriteFrame {name} = "
            f"{{ -1, 2, 3, 2, 5, 6, {name}_pixels }};",
        ]
    return "\n".join(out)


class MineralWordBankTests(unittest.TestCase):
    def test_preserves_frame_identity_and_all_phases(self):
        source = source_bank()
        generated = emit(source)
        for i, name in enumerate(SOURCES, 1):
            assert f"frame == &{name}) return &sc64_static_mineral_words_{i};" in generated
            for phase in range(4):
                bank = re.search(
                    rf"sc64_static_mineral_words_{i}_p{phase}\[\] = "
                    r"\{(.*?)\};", generated, re.S)
                self.assertIsNotNone(bank)
                words = [int(v, 16) for v in re.findall(r"0x([0-9a-f]+)u", bank[1])]
                groups = (phase + 3 + 3) // 4
                self.assertEqual(len(words), 2 * 2 * groups)
                for y, source_row in enumerate(((0, i, 0), (i + 10, 0, i + 20))):
                    expected = [0] * phase + list(source_row)
                    expected += [0] * (groups * 4 - len(expected))
                    start = y * groups * 2
                    colors = b"".join(w.to_bytes(4, "little")
                                      for w in words[start:start + 2 * groups:2])
                    mask = b"".join(w.to_bytes(4, "little")
                                    for w in words[start + 1:start + 2 * groups:2])
                    self.assertEqual(list(colors), expected)
                    self.assertEqual(list(mask), [255 if v else 0 for v in expected])

    def test_rejects_ambiguous_or_truncated_source(self):
        source = source_bank()
        name = SOURCES[0]
        with self.assertRaisesRegex(ValueError, "duplicate source frame"):
            frame_data(source + "\n" + source, name)
        with self.assertRaisesRegex(ValueError, "wrong source pixel count"):
            frame_data(source.replace(f"{name}_pixels[6]", f"{name}_pixels[5]"), name)
        with self.assertRaisesRegex(ValueError, "invalid source frame dimensions"):
            frame_data(source.replace("{ -1, 2, 3, 2", "{ -1, 2, 257, 2"), name)


if __name__ == "__main__":
    unittest.main()
