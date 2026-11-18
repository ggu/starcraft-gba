# SPDX-License-Identifier: GPL-3.0-or-later
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_z2_normal_replay import p10_campaign_journal


class Layout(dict):
    widths = {"version": 4}
    arrays = {"state": (32, 4, True)}


def fixture():
    payload = bytearray(152)
    struct.pack_into("<I", payload, 4, 0x7243505F)
    struct.pack_into("<I", payload, 16, 2)
    struct.pack_into("<I", payload, 36, 500)
    checksum = 2166136261
    for byte in payload:
        checksum = ((checksum ^ byte) * 16777619) & 0xffffffff
    data = bytearray(b"\xff" * 32768)
    struct.pack_into("<4I", data, 0, 0x7643505F, len(payload), checksum, 1)
    data[16:16 + len(payload)] = payload
    return data


class CampaignJournalTest(unittest.TestCase):
    def test_binds_fresh_journal_to_the_actual_live_cut(self):
        data = fixture()
        layout = (160, Layout(version=4, state=8))
        row = p10_campaign_journal(data, layout, 2, 500)
        self.assertEqual((row['sequence'], row['bytes'], row['frame']), (1, 152, 500))
        with self.assertRaisesRegex(ValueError, "identity"):
            p10_campaign_journal(data, layout, 3, 500)
        with self.assertRaisesRegex(ValueError, "identity"):
            p10_campaign_journal(data, layout, 2, 501)

    def test_rejects_altered_payload_and_extra_bank(self):
        data = fixture()
        layout = (160, Layout(version=4, state=8))
        data[16] ^= 1
        with self.assertRaisesRegex(ValueError, "checksum"):
            p10_campaign_journal(data, layout, 2, 500)
        data = fixture()
        data[16384] = 0
        with self.assertRaisesRegex(ValueError, "one fresh"):
            p10_campaign_journal(data, layout, 2, 500)
