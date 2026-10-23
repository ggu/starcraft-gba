"""Asset-free checks for the T5 SRAM smoke menu-entry controls."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_t5_save_smoke", ROOT / "tools/sc64_t5_save_smoke.py")
assert SPEC and SPEC.loader
smoke = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(smoke)


class FakeRsp:
    def __init__(self):
        self.writes: list[tuple[int, bytes]] = []
        self.ticks: list[int] = []

    def write(self, address: int, data: bytes) -> None:
        self.writes.append((address, data))

    def tick(self, count: int = 1) -> None:
        self.ticks.append(count)

    def read(self, address: int, size: int) -> bytes:
        return self.memory[address][:size]


class T5SaveSmokeMenuTests(unittest.TestCase):
    def test_normal_menu_entry_uses_campaign_mission_briefing_deploy_sequence(self):
        rsp = FakeRsp()
        smoke.enter_t5_campaign_menu(rsp, 0x1234)
        self.assertEqual(smoke.MENU_ENTRY_BUTTONS, (1, 16, 16, 1, 1))
        self.assertEqual([data for _, data in rsp.writes], [
            b"\x01\x00", b"\0\0", b"\x10\x00", b"\0\0",
            b"\x10\x00", b"\0\0", b"\x01\x00", b"\0\0",
            b"\x01\x00", b"\0\0",
        ])
        self.assertTrue(all(address == 0x1234 for address, _ in rsp.writes))
        self.assertEqual(rsp.ticks, [2] * 10)

    def test_t6_menu_entry_selects_second_row_on_third_page(self):
        rsp = FakeRsp()
        smoke.enter_t5_campaign_menu(rsp, 0x1234, mission=6)
        self.assertEqual([data for _, data in rsp.writes], [
            b"\x01\x00", b"\0\0", b"\x10\x00", b"\0\0",
            b"\x10\x00", b"\0\0", b"\x80\x00", b"\0\0",
            b"\x01\x00", b"\0\0", b"\x01\x00", b"\0\0",
        ])
        self.assertEqual(rsp.ticks, [2] * 12)

    def test_late_terrans_use_correct_campaign_page_and_row(self):
        expected = {
            5: (1, 16, 16, 1, 1),
            6: (1, 16, 16, 128, 1, 1),
            7: (1, 16, 16, 16, 1, 1),
            8: (1, 16, 16, 16, 128, 1, 1),
            9: (1, 16, 16, 16, 16, 1, 1),
            10: (1, 16, 16, 16, 16, 128, 1, 1),
        }
        for mission, buttons in expected.items():
            with self.subTest(mission=mission):
                self.assertEqual(smoke.campaign_menu_buttons(mission), buttons)
                rsp = FakeRsp()
                smoke.enter_t5_campaign_menu(rsp, 0x1234, mission)
                self.assertEqual(len(rsp.ticks), 2 * len(buttons))
                self.assertEqual(rsp.ticks, [2] * (2 * len(buttons)))
        with self.assertRaises(ValueError):
            smoke.campaign_menu_buttons(11)

    @staticmethod
    def pack_compact(values):
        row = bytearray(38)
        bit = 0
        for name, width, bias in smoke.UNIT_COMPACT_FIELDS:
            value = values[name] + bias
            assert 0 <= value < (1 << width)
            for i in range(width):
                if (value >> i) & 1:
                    row[bit >> 3] |= 1 << (bit & 7)
                bit += 1
        assert bit == 303
        return bytes(row)

    def test_generation17_schema_and_compact_live_dead_row_fields(self):
        self.assertEqual(smoke.STORAGE_SCHEMAS[17], (96, 15312))
        self.assertEqual(smoke.STORAGE_UNIT_COUNTS[17], 192)
        self.assertEqual(smoke.STORAGE_MINERAL_COUNTS[17], 80)
        self.assertEqual(smoke.STORAGE_T6_STATE_BYTES[17], 1388)
        values = {name: 0 for name, _, _ in smoke.UNIT_COMPACT_FIELDS}
        values.update({"x": 1900, "y": 1024, "hp": -999, "maxhp": 65000,
                       "owner": 4, "type": 35, "alive": 0, "selected": 1,
                       "cd": 65535, "order": 9, "tx": 2047, "ty": 3,
                       "carry": -12345, "build": -100, "work_timer": 54321,
                       "armor": 255, "weapon": 65535, "target": 511,
                       "path_timer": 60000, "path_retry": 12000,
                       "path_x": 2020, "path_y": 2047, "shield": 131071,
                       "raw_type": 255, "raw_owner": -1, "flags": 0xA55A,
                       "shield_remainder": 1279})
        row = self.pack_compact(values)
        self.assertEqual(smoke.decode_compact_unit_row(row), values)
        self.assertEqual(smoke.decode_compact_unit_rows(bytes(4) + row + bytes(38), 2),
                         [values, {name: 0 for name, _, _ in smoke.UNIT_COMPACT_FIELDS}])
        with self.assertRaises(ValueError):
            smoke.decode_compact_unit_row(row[:-1])
        with self.assertRaises(ValueError):
            smoke.decode_compact_unit_row(row[:-1] + bytes([row[-1] | 0x80]))
        with self.assertRaises(ValueError):
            smoke.decode_compact_unit_rows(bytes(4) + row, 2)

    def test_generation18_uses_dwarf_payload_and_late_runtime_capacity(self):
        self.assertEqual(smoke.STORAGE_SCHEMAS[18], (96, None))
        self.assertEqual(smoke.STORAGE_UNIT_COUNTS[18], 192)
        self.assertEqual(smoke.STORAGE_MINERAL_COUNTS[18], 80)

    def test_late_state_payload_extracts_exact_union_member(self):
        payload = bytes(range(256)) * 8
        fields = {"terran_late": 137}
        self.assertEqual(smoke.late_save_state(payload, fields, 1892), payload[137:2029])
        with self.assertRaises(ValueError):
            smoke.late_save_state(payload[:1000], fields, 1892)

    def test_late_state_summary_decodes_all_trigger_clocks_links_and_goals(self):
        raw = bytearray(1892)
        trigger_fields = {"player": 0}
        player_fields = {"elapsed_ticks": 0, "fault": 139}
        for owner in range(8):
            base = owner * 144
            raw[base:base + 4] = (200 + owner).to_bytes(4, "little")
            raw[base + 139] = 0
        fields = {"triggers": 0, "flight_goals": 1456,
                  "emitter_unit": 1874, "emitter_carrier": 1876}
        raw[1874:1876] = (191).to_bytes(2, "little")
        raw[1876:1878] = (23).to_bytes(2, "little")
        raw[1456:1460] = (1 << 22 | 17 | (19 << 11)).to_bytes(4, "little")
        summary = smoke.late_state_summary(raw, fields, 1892,
                                           trigger_fields, 1184,
                                           player_fields, 144)
        self.assertEqual(summary["trigger_clocks"], list(range(200, 208)))
        self.assertEqual(summary["trigger_faults"], [0] * 8)
        self.assertEqual(summary["emitter_unit"], 191)
        self.assertEqual(summary["emitter_carrier"], 23)
        self.assertEqual(summary["active_flight_goals"], 1)
        self.assertEqual(summary["flight_goals"][0], 1 << 22 | 17 | (19 << 11))
        with self.assertRaises(ValueError):
            smoke.late_state_summary(raw[:-1], fields, 1892,
                                     trigger_fields, 1184,
                                     player_fields, 144)

    def test_compact_runtime_rows_preserve_every_named_field_including_dead(self):
        fields = {}
        cursor = 0
        for name, (size, _) in smoke.UNIT_RUNTIME_FIELDS.items():
            fields[name] = cursor
            cursor += size
        self.assertLessEqual(cursor, 56)
        stride = 56
        raw = bytearray(stride * 2)
        for slot in range(2):
            for i, (name, (size, signed)) in enumerate(smoke.UNIT_RUNTIME_FIELDS.items()):
                value = (-slot - 1 if signed else 0) if slot else (100 + i)
                start = slot * stride + fields[name]
                raw[start:start + size] = int(value).to_bytes(size, "little", signed=signed)
        rsp = FakeRsp()
        rsp.memory = {0x1000: bytes(raw)}
        rows = smoke.compact_live_unit_rows(rsp, 0x1000, fields, 2, stride)
        self.assertEqual(set(rows[0]), set(smoke.UNIT_RUNTIME_FIELDS))
        self.assertEqual(set(rows[1]), set(smoke.UNIT_RUNTIME_FIELDS))
        self.assertEqual(rows[0]["alive"], 106)
        self.assertEqual(rows[1]["hp"], -2)
        self.assertEqual(rows[1]["alive"], 0)


if __name__ == "__main__":
    unittest.main()
