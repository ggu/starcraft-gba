import unittest

from tools.sc64_sprites import parse_grp, crop_frame, write_c_header


def grp_one_row(width, runs):
    """Build a one-frame GRP, where runs are already-encoded row bytes."""
    data_off = 14
    row_data = bytes(runs)
    return ((1).to_bytes(2, "little") + width.to_bytes(2, "little")
            + (1).to_bytes(2, "little")
            + bytes((0, 0, width, 1)) + data_off.to_bytes(4, "little")
            + (2).to_bytes(2, "little") + row_data)


class ParseGrpTests(unittest.TestCase):
    def test_literal_row_and_crop(self):
        grp = parse_grp(grp_one_row(3, [3, 10, 20, 30]))
        frame = grp["frames"][0]
        self.assertEqual((grp["width"], grp["height"]), (3, 1))
        self.assertEqual(frame["pixels"], bytes((10, 20, 30)))
        self.assertEqual(frame["alpha"], bytes((1, 1, 1)))
        self.assertEqual(crop_frame(frame), bytes((10, 20, 30)))

    def test_transparent_repeat_and_literal_runs(self):
        # transparent 2, repeat color 9 twice, then literal color 7
        frame = parse_grp(grp_one_row(5, [0x82, 0x42, 9, 1, 7]))["frames"][0]
        self.assertEqual(frame["pixels"], bytes((0, 0, 9, 9, 7)))
        self.assertEqual(frame["alpha"], bytes((0, 0, 1, 1, 1)))

    def test_rejects_run_past_frame(self):
        with self.assertRaisesRegex(ValueError, "exceeds frame width"):
            parse_grp(grp_one_row(2, [3, 1, 2, 3]))

    def test_header_applies_remap_but_keeps_transparency_zero(self):
        frame = parse_grp(grp_one_row(5, [0x82, 0x42, 9, 1, 7]))["frames"][0]
        remap = bytes((i + 32) & 0xFF for i in range(256))
        import tempfile
        from pathlib import Path
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "sprites.h"
            write_c_header(path, [{"name": "test", "frame": frame}], remap)
            text = path.read_text()
        self.assertIn("{ 0, 0, 5, 1, 5, 1, sc64_sprite_test_pixels }", text)
        self.assertIn("0x00, 0x00, 0x29, 0x29, 0x27", text)


if __name__ == "__main__":
    unittest.main()
