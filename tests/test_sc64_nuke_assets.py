# SPDX-License-Identifier: GPL-3.0-or-later
"""Check the nuclear source frame format without copyrighted input."""
import unittest

from tools.sc64_nuke_assets import pack_frame


def unpack_frame(data: bytes, width: int, height: int) -> bytes:
    output = bytearray()
    cursor = 0
    for _ in range(height):
        row_remaining = width
        while row_remaining:
            token = data[cursor]
            cursor += 1
            op, count = divmod(token, 64)
            if not 1 <= count <= row_remaining or op == 3:
                raise ValueError("invalid span")
            if op == 2:
                output.extend(bytes(count))
            elif op == 1:
                pixel = data[cursor]
                if not pixel:
                    raise ValueError("transparent repeat")
                output.extend(bytes([pixel]) * count)
                cursor += 1
            else:
                pixels = data[cursor:cursor + count]
                if len(pixels) != count or 0 in pixels:
                    raise ValueError("invalid literal")
                output.extend(pixels)
                cursor += count
            row_remaining -= count
    if cursor != len(data):
        raise ValueError("trailing frame data")
    return bytes(output)


class NukeFrameFormat(unittest.TestCase):
    def test_known_tokens(self):
        self.assertEqual(pack_frame(bytes([0, 0, 0, 4, 4, 4, 7, 9]), 8, 1),
                         bytes([0x83, 0x43, 4, 2, 7, 9]))

    def test_round_trip_max_spans_and_rows(self):
        row1 = bytes(64) + bytes([255]) * 62
        row2 = bytes(range(1, 127))
        row3 = bytes([63]) * 126
        row4 = bytes(126)
        pixels = row1 + row2 + row3 + row4
        packed = pack_frame(pixels, 126, 4)
        self.assertEqual(unpack_frame(packed, 126, 4), pixels)
        self.assertLess(len(packed), 150)

    def test_reject_invalid_rectangle(self):
        for pixels, width, height in [(b"", 0, 0), (b"", -1, 2), (b"\0", 2, 1)]:
            with self.subTest(width=width, height=height), self.assertRaises(ValueError):
                pack_frame(pixels, width, height)
