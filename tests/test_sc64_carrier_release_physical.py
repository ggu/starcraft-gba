# SPDX-License-Identifier: GPL-3.0-or-later
"""Fail-closed checks for the Carrier ARM gate's evidence parser."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_carrier_release_physical as gate


TAG = 0x764D5442


def record(tag, sequence, data):
    checksum = 2166136261
    for value in data:
        checksum = ((checksum ^ value) * 16777619) & 0xffffffff
    return (struct.pack("<4I", tag, len(data), checksum, sequence) + data).ljust(gate.SLOT, b"\xff")


class TestCarrierReleasePhysical(unittest.TestCase):
    def setUp(self):
        self.data = bytes(range(256)) * 40
        self.old = record(TAG, 1, self.data)
        self.erased = b"\xff" * gate.SLOT
        self.first = self.old + self.erased

    def test_immediate_payload_and_predecessor(self):
        verified = gate.verify_journal(self.first, self.old + record(TAG, 2, self.data), TAG)
        self.assertEqual(verified["payload_bytes"], len(self.data))

    def test_reject_valid_but_wrong_generation_sequence_or_payload(self):
        for changed in (record(0x76435442, 2, self.data), record(TAG, 3, self.data),
                        record(TAG, 2, b"x" + self.data[1:])):
            with self.subTest(header=changed[:16]), self.assertRaises(gate.ContinuationError):
                gate.verify_journal(self.first, self.old + changed, TAG)

    def test_reject_checksum_corruption_and_non_erased_other_bank(self):
        new = record(TAG, 2, self.data)
        with self.assertRaisesRegex(gate.ContinuationError, "checksum"):
            gate.verify_journal(self.first, self.old + new[:16] + b"x" + new[17:], TAG)
        with self.assertRaisesRegex(gate.ContinuationError, "other bank"):
            gate.verify_journal(self.old + new, self.old + new, TAG)

    def test_predecessor_padding_is_preserved(self):
        new = record(TAG, 2, self.data)
        with self.assertRaisesRegex(gate.ContinuationError, "predecessor"):
            gate.verify_journal(self.first, self.old[:-1] + b"\x00" + new, TAG)

    def test_showcase_has_no_campaign_cartridge_case(self):
        with self.assertRaises(gate.ContinuationError):
            gate.plan_case("showcase", "campaign")


if __name__ == "__main__":
    unittest.main()
