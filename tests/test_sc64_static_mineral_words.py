"""Source-only fixture for the derived static mineral bank."""
# SPDX-License-Identifier: GPL-3.0-or-later

import pathlib
import re
import os
import subprocess
import tempfile
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
    def test_prepared_bank_tracks_same_path_with_old_timestamp(self):
        root = pathlib.Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as directory:
            work = pathlib.Path(directory)
            (work / 'tools').symlink_to(root / 'tools', target_is_directory=True)
            (work / 'build').mkdir()
            source = work / 'input.h'
            source.write_text(source_bank())
            (work / 'Makefile').write_text(
                f'PYTHON := {sys.executable}\n'
                'SC64_STATIC_MINERAL_WORDS_SOURCE := input.h\n'
                'build:\n\t@mkdir -p build\n'
                f'include {root}/mk/static-mineral-words/Makefile\n')
            def build():
                subprocess.run(['make', '-s', 'sc64-static-mineral-words'],
                               cwd=work, check=True, capture_output=True, text=True)
            build()
            output = work / 'build/generated/sc64_static_mineral_words_generated.h'
            first = output.read_bytes()
            source.write_text(source_bank().replace('0,1,0,', '0,9,0,', 1))
            os.utime(source, (1, 1))
            build()
            second = output.read_bytes()
            self.assertNotEqual(first, second)
            timestamp = output.stat().st_mtime_ns
            build()
            self.assertEqual(timestamp, output.stat().st_mtime_ns)
            alternate = work / 'alternate.h'
            alternate.write_text(source_bank())
            os.utime(alternate, (1, 1))
            subprocess.run(['make', '-s', 'sc64-static-mineral-words',
                            'SC64_STATIC_MINERAL_WORDS_SOURCE=alternate.h'],
                           cwd=work, check=True, capture_output=True, text=True)
            self.assertEqual(first, output.read_bytes())

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
