# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_icon_assets", ROOT / "tools/sc64_icon_assets.py")
icons = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(icons)


def synthetic_grp() -> bytes:
    # Three by two frame: transparent + literal pixels on row 0; repeated run
    # on row 1. Frame data offsets are relative to the line-table beginning.
    header = bytes((1, 0, 3, 0, 2, 0))
    data_offset = len(header) + 8
    frame_header = bytes((0, 0, 3, 2)) + data_offset.to_bytes(4, "little")
    rows = bytes((4, 0, 8, 0)) + bytes((0x81, 2, 1, 2, 0x43, 3))
    return header + frame_header + rows


class Sc64IconAssetTests(unittest.TestCase):
    def test_grp_crop_transparency_and_world_palette_remap(self):
        grp = icons.decode_grp(synthetic_grp())
        frame = grp["frames"][0]
        self.assertEqual((frame["w"], frame["h"]), (3, 2))
        self.assertEqual(frame["alpha"], bytes((0, 1, 1, 1, 1, 1)))
        self.assertEqual(frame["pixels"], bytes((0, 1, 2, 3, 3, 3)))
        remap = bytes(32 + i % 224 for i in range(256))
        self.assertEqual(icons.crop_frame(frame, remap), bytes((0, 33, 34, 35, 35, 35)))
        fitted = icons.fit_icon(frame, remap)
        self.assertEqual((fitted["w"], fitted["h"]), (3, 2))
        self.assertEqual(fitted["pixels"], icons.crop_frame(frame, remap))

    def test_fit_and_resize_are_bounded_nearest_neighbor(self):
        remap = bytes(32 + i % 224 for i in range(256))
        pixels = bytes((1, 2, 3, 4, 5, 6, 7, 8))
        frame = {"w": 4, "h": 2, "pixels": pixels, "alpha": bytes((1,) * 8)}
        fitted = icons.fit_icon(frame, remap, bound=2)
        self.assertEqual((fitted["w"], fitted["h"]), (2, 1))
        self.assertEqual(fitted["pixels"], bytes((33, 35)))
        self.assertEqual(icons.scale_indexed(bytes((1, 2, 3, 4)), 2, 2, 4, 4),
                         bytes((1, 1, 2, 2, 1, 1, 2, 2,
                                3, 3, 4, 4, 3, 3, 4, 4)))
        with self.assertRaises(ValueError):
            icons.scale_indexed(b"123", 2, 2, 1, 1)

    def test_header_checks_icon_count_indices_and_encodes_stable_ids(self):
        frames = [{"w": 1, "h": 1, "pixels": bytes((0,))}
                  for _ in range(icons.ICON_COUNT)]
        header = icons.build_header(frames)
        self.assertIn("SC64_ICON_UNIT_COUNT = 228", header)
        self.assertIn("SC64_ICON_MOVE = 228", header)
        self.assertIn("SC64_ICON_STORM = 243", header)
        self.assertIn("SC64_ICON_COUNT = 244", header)
        self.assertIn("Sc64IconFrame sc64_icons[SC64_ICON_COUNT]", header)
        with self.assertRaises(ValueError):
            icons.build_header(frames[:-1])
        invalid = list(frames)
        invalid[2] = {"w": 1, "h": 1, "pixels": bytes((31,))}
        with self.assertRaises(ValueError):
            icons.build_header(invalid)

    @unittest.skipUnless((ROOT / ".local/sc64/rom/StarCraft64-USA.z64").is_file()
                         and (ROOT / ".local/sc64/backwater/generated/palette-remap.bin").is_file()
                         and (ROOT / "references/sc64-maps/.git").exists(),
                         "private ROM, world remap or pinned decoder is unavailable")
    def test_verified_atlas_frames_fit_and_remap_to_world_indices(self):
        with tempfile.TemporaryDirectory(prefix="sc64-icons-") as temp:
            out = Path(temp)
            report = icons.generate(
                ROOT / ".local/sc64/rom/StarCraft64-USA.z64",
                ROOT / "references/sc64-maps",
                ROOT / ".local/sc64/backwater/generated/palette-remap.bin",
                out / "sc64_icons_generated.h", out / "report.json")
            self.assertEqual(report["icons"], 244)
            self.assertEqual(report["grp_sha1"], "e64e5ed3f69fb537556f381ac900372cf38ffb9b")
            self.assertEqual(len(report["entries"]), 244)
            self.assertTrue(all(0 < row["w"] <= 20 and 0 < row["h"] <= 20
                                for row in report["entries"]))
            generated = (out / "sc64_icons_generated.h").read_text()
            self.assertIn("SC64_HAS_ICON_ASSETS 1", generated)
            array = generated.split("sc64_icon_pixels", 1)[1].split("{", 1)[1].split("};", 1)[0]
            pixels = [int(value) for value in __import__("re").findall(r"\d+", array)]
            self.assertTrue(any(value == 0 for value in pixels))
            self.assertTrue(all(value == 0 or 32 <= value <= 255 for value in pixels))


if __name__ == "__main__":
    unittest.main()
