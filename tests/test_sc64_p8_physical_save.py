# SPDX-License-Identifier: GPL-3.0-or-later
"""Pure parser tests; all altered bytes below are in-process test records."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from sc64_races_profile import DwarfOffsets
import sc64_p8_physical_save as save


class P8NeutralSaveTest(unittest.TestCase):
    def setUp(self):
        self.layout = (16332, DwarfOffsets({'version': 0, 'state': 8},
                       {'version': 4}, {}, {'state': (32, 4, True)}))
        self.payload = bytearray(16292 + 16)
        struct.pack_into('<I', self.payload, 0, save.BASE_TAG)
        struct.pack_into('<I', self.payload, 8 + 2*4, 27)
        struct.pack_into('<I', self.payload, 8 + 7*4, 31676)
        struct.pack_into('<I', self.payload, 8 + 27*4, 0x42450000)
        struct.pack_into('<I', self.payload, 8 + 28*4, 0x42440000)

    def image(self, payload=None, *, tag=save.TRANSPORT_TAG, seq=1, fill=255):
        payload = bytes(self.payload if payload is None else payload)
        checksum = 2166136261
        for b in payload:
            checksum = ((checksum ^ b) * 16777619) & 0xffffffff
        image = bytearray([fill]) * save.SRAM_SIZE
        struct.pack_into('<4I', image, 0, tag, len(payload), checksum, seq)
        image[16:16+len(payload)] = payload
        return bytes(image)

    def test_neutral_and_medium_are_explicit(self):
        record = save.validate_neutral_journal(self.image(), self.layout, 31676)
        self.assertEqual((record['payload_bytes'], record['frame'], record['yamato_shots']), (16308,31676,0))
        host = self.image(fill=0)
        save.validate_neutral_journal(host, self.layout, 31676, medium='host')
        with self.assertRaisesRegex(ValueError, 'second SRAM'): save.validate_neutral_journal(host, self.layout, 31676)
        with self.assertRaisesRegex(ValueError, 'second SRAM'): save.validate_neutral_journal(self.image(), self.layout, 31676, medium='host')

    def test_unsupported_variable_sections_fail_closed(self):
        for delta in (20, 8, 26):
            with self.subTest(extra=delta), self.assertRaisesRegex(ValueError, 'envelope'):
                save.validate_neutral_journal(self.image(self.payload[:-16] + bytes(delta) + self.payload[-16:]), self.layout, 31676)
        payload = bytearray(self.payload)
        struct.pack_into('<I', payload, 8+28*4, 0x42440008)
        with self.assertRaisesRegex(ValueError, 'Yamato'): save.validate_neutral_journal(self.image(payload), self.layout, 31676)
        payload = bytearray(self.payload); payload[-1] = 1
        with self.assertRaisesRegex(ValueError, 'trailer'): save.validate_neutral_journal(self.image(payload), self.layout, 31676)

    def test_header_checksum_frame_and_markers(self):
        for image in (self.image(tag=save.BASE_TAG), self.image(seq=2)):
            with self.assertRaises(ValueError): save.validate_neutral_journal(image, self.layout, 31676)
        bad = bytearray(self.image()); bad[16] ^= 1
        with self.assertRaisesRegex(ValueError, 'checksum'): save.validate_neutral_journal(bad, self.layout, 31676)
        with self.assertRaisesRegex(ValueError, 'frame'): save.validate_neutral_journal(self.image(), self.layout, 31677)
        for off in (0, 8+27*4, 8+28*4):
            payload = bytearray(self.payload); payload[off+3] ^= 1
            with self.subTest(offset=off), self.assertRaises(ValueError): save.validate_neutral_journal(self.image(payload), self.layout, 31676)


if __name__ == '__main__': unittest.main()
