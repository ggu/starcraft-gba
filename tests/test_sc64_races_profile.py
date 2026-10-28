"""Asset-free tests for race bank DWARF and compact entity observation."""
from __future__ import annotations

import struct
import unittest
from pathlib import Path
from unittest.mock import patch

from tools import sc64_races_profile as races


class FakeRsp:
    def __init__(self, data: bytes):
        self.data = data

    def read(self, address: int, size: int) -> bytes:
        result = self.data[address:address + size]
        if len(result) != size:
            raise AssertionError("fixture read exceeded its byte buffer")
        return result


class ShortRsp:
    def read(self, address: int, size: int) -> bytes:
        return bytes(size - 1)


def compact_unit_layout() -> tuple[int, races.DwarfOffsets]:
    offsets = {
        "hp": 0, "maxhp": 4, "shield": 12, "x": 16, "y": 18,
        "raw_type": 32, "raw_owner": 34, "owner": 50, "type": 51,
        "alive": 52, "selected": 53, "order": 54,
    }
    widths = {name: 4 for name in ("hp", "maxhp", "shield")}
    widths.update({name: 2 for name in ("x", "y", "raw_type", "raw_owner")})
    widths.update({name: 1 for name in ("owner", "type", "alive", "selected", "order")})
    signed = {name: name not in ("owner", "type", "alive", "selected", "order")
              for name in widths}
    return 56, races.DwarfOffsets(offsets, widths, signed)


def compact_building_layout() -> tuple[int, races.DwarfOffsets]:
    offsets = {"hp": 0, "maxhp": 4, "shield": 8, "x": 16, "y": 18,
               "raw_type": 20, "raw_owner": 22, "owner": 32, "type": 33,
               "alive": 34, "under_construction": 42}
    widths = {name: 4 for name in ("hp", "maxhp", "shield")}
    widths.update({name: 2 for name in ("x", "y", "raw_type", "raw_owner")})
    widths.update({name: 1 for name in ("owner", "type", "alive", "under_construction")})
    signed = {name: name not in ("owner", "type", "alive", "under_construction")
              for name in widths}
    return 48, races.DwarfOffsets(offsets, widths, signed)


def dwarf_fixture() -> str:
    return "\n".join([
        " <1><100>: Abbrev Number: 1 (DW_TAG_typedef)",
        "    <101> DW_AT_name : (indirect string, offset: 0x10): Building",
        "    <102> DW_AT_type : <0x110>",
        " <1><110>: Abbrev Number: 2 (DW_TAG_typedef)",
        "    <111> DW_AT_name : (indirect string, offset: 0x20): Sc64ZpCompactBuilding",
        "    <112> DW_AT_type : <0x120>",
        " <1><120>: Abbrev Number: 3 (DW_TAG_structure_type)",
        "    <121> DW_AT_byte_size : 48",
        " <2><130>: Abbrev Number: 4 (DW_TAG_member)",
        "    <131> DW_AT_name : hp",
        "    <132> DW_AT_type : <0x200>",
        "    <133> DW_AT_data_member_location : 0",
        " <2><140>: Abbrev Number: 4 (DW_TAG_member)",
        "    <141> DW_AT_name : x",
        "    <142> DW_AT_type : <0x210>",
        "    <143> DW_AT_data_member_location : 16",
        " <2><150>: Abbrev Number: 4 (DW_TAG_member)",
        "    <151> DW_AT_name : owner",
        "    <152> DW_AT_type : <0x220>",
        "    <153> DW_AT_data_member_location : 32",
        " <1><200>: Abbrev Number: 5 (DW_TAG_base_type)",
        "    <201> DW_AT_byte_size : 4",
        "    <202> DW_AT_encoding : 5 (signed)",
        " <1><210>: Abbrev Number: 5 (DW_TAG_base_type)",
        "    <211> DW_AT_byte_size : 2",
        "    <212> DW_AT_encoding : 5 (signed)",
        " <1><220>: Abbrev Number: 5 (DW_TAG_base_type)",
        "    <221> DW_AT_byte_size : 1",
        "    <222> DW_AT_encoding : 8 (unsigned char)",
    ])


class RaceProfileTests(unittest.TestCase):
    def test_typedef_alias_and_scalar_widths_resolve_from_dwarf(self) -> None:
        size, fields = races.parse_dwarf_struct(dwarf_fixture(), "Building", "fixture.elf")
        self.assertEqual(size, 48)
        self.assertEqual(dict(fields), {"hp": 0, "x": 16, "owner": 32})
        self.assertEqual(fields.widths, {"hp": 4, "x": 2, "owner": 1})
        self.assertEqual(fields.signed, {"hp": True, "x": True, "owner": False})

    def test_typedef_cycles_and_missing_names_fail_closed(self) -> None:
        cyclic = "\n".join([
            " <1><100>: Abbrev Number: 1 (DW_TAG_typedef)",
            "    <101> DW_AT_name : LoopA",
            "    <102> DW_AT_type : <0x110>",
            " <1><110>: Abbrev Number: 1 (DW_TAG_typedef)",
            "    <111> DW_AT_name : LoopB",
            "    <112> DW_AT_type : <0x100>",
        ])
        with self.assertRaisesRegex(RuntimeError, "no complete DWARF structure"):
            races.parse_dwarf_struct(cyclic, "LoopA")
        with self.assertRaisesRegex(RuntimeError, "no DWARF typedef"):
            races.parse_dwarf_struct(cyclic, "Missing")

    def test_frozen_compact_z6_bank_metadata_is_accepted_from_dwarf(self) -> None:
        def layout(_elf: Path, name: str):
            return compact_unit_layout() if name == "Unit" else compact_building_layout()

        with patch.object(races, "dwarf_struct_layout", side_effect=layout):
            bank = races.race_bank_profile(Path("z6.elf"), {"u": 240 * 56, "b": 160 * 48})
        self.assertEqual((bank["unit_capacity"], bank["building_capacity"]), (240, 160))
        self.assertEqual((bank["unit_stride_bytes"], bank["building_stride_bytes"]), (56, 48))
        self.assertEqual(bank["unit_field_widths"]["owner"], 1)
        self.assertEqual(bank["building_field_widths"]["x"], 2)

    def test_legacy_bank_shape_stays_supported(self) -> None:
        offsets = {"hp": 0, "owner": 4, "type": 8, "alive": 12, "x": 16, "y": 20,
                   "under_construction": 24}
        fields = races.DwarfOffsets(offsets, {key: 4 for key in offsets},
                                    {key: True for key in offsets})
        with patch.object(races, "dwarf_struct_layout",
                          side_effect=((88, fields), (100, fields))):
            bank = races.race_bank_profile(Path("legacy.elf"), {"u": 128 * 88, "b": 64 * 100})
        self.assertEqual((bank["unit_capacity"], bank["building_capacity"]), (128, 64))

    def test_legacy_reader_defaults_plain_offset_maps_to_int_fields(self) -> None:
        row = bytearray(24)
        struct.pack_into("<i", row, 0, 1)
        struct.pack_into("<i", row, 4, 0)
        struct.pack_into("<i", row, 8, 7)
        struct.pack_into("<i", row, 12, -41)
        struct.pack_into("<i", row, 16, 333)
        units = races.read_entities(FakeRsp(bytes(row)), 0, 1, 24,
                                    {"alive": 0, "owner": 4, "type": 8,
                                     "x": 12, "y": 16, "hp": 20})
        self.assertEqual(units["entities"][0]["x"], -41)
        self.assertEqual(units["entities"][0]["y"], 333)

    def test_compact_unit_and_building_fields_use_dwarf_widths(self) -> None:
        unit_stride, unit_fields = compact_unit_layout()
        unit = bytearray(unit_stride)
        struct.pack_into("<i", unit, 0, 123456)
        struct.pack_into("<h", unit, 16, -17)
        struct.pack_into("<h", unit, 18, 2047)
        struct.pack_into("<h", unit, 32, 44)
        unit[50:53] = bytes((0, 7, 1))
        observed_units = races.read_entities(FakeRsp(bytes(unit)), 0, 1, unit_stride,
                                             unit_fields)
        self.assertEqual(observed_units["alive"], 1)
        self.assertEqual(observed_units["entities"][0]["x"], -17)
        self.assertEqual(observed_units["entities"][0]["y"], 2047)
        self.assertEqual(observed_units["entities"][0]["raw_type"], 44)

        building_stride, building_fields = compact_building_layout()
        building = bytearray(building_stride)
        struct.pack_into("<i", building, 0, 90000)
        struct.pack_into("<h", building, 16, -30)
        struct.pack_into("<h", building, 18, 1001)
        struct.pack_into("<h", building, 20, 196)
        building[32:35] = bytes((0, 13, 1))
        building[42] = 0
        observed_buildings = races.read_entities(FakeRsp(bytes(building)), 0, 1,
                                                 building_stride, building_fields,
                                                 building=True)
        self.assertEqual(observed_buildings["alive"], 1)
        self.assertEqual(observed_buildings["entities"][0]["x"], -30)
        self.assertEqual(observed_buildings["entities"][0]["type"], 13)
        self.assertEqual(observed_buildings["completed_by_owner_type"], {"0": {"13": 1}})

    def test_entity_field_must_fit_stride_and_allowed_width(self) -> None:
        fields = races.DwarfOffsets({"alive": 0, "owner": 1, "type": 2, "x": 6, "y": 4},
                                    {"alive": 1, "owner": 1, "type": 1, "x": 4, "y": 2},
                                    {})
        with self.assertRaisesRegex(RuntimeError, "bounded ABI width"):
            races.read_entities(FakeRsp(bytes((1, 0, 0, 0, 0, 0, 0, 0))), 0, 1, 8, fields)

    def test_compact_dimensions_cannot_claim_wrong_abi(self) -> None:
        def wrong_building_layout(_elf: Path, name: str):
            if name == "Unit":
                return compact_unit_layout()
            return 100, compact_building_layout()[1]

        with patch.object(races, "dwarf_struct_layout", side_effect=wrong_building_layout):
            with self.assertRaisesRegex(RuntimeError, "Unexpected Races bank capacity"):
                races.race_bank_profile(Path("bad.elf"), {"u": 240 * 56, "b": 96 * 100})

    def test_compact_field_widths_must_match_exact_abi(self) -> None:
        unit_stride, unit_fields = compact_unit_layout()
        unit_fields.widths["owner"] = 4
        with patch.object(races, "dwarf_struct_layout",
                          side_effect=((unit_stride, unit_fields), compact_building_layout())):
            with self.assertRaisesRegex(RuntimeError, "unsupported or unbounded ABI width"):
                races.race_bank_profile(Path("bad-width.elf"), {"u": 240 * 56, "b": 160 * 48})

        legacy = races.DwarfOffsets(
            {"hp": 0, "owner": 4, "type": 8, "alive": 12, "x": 16, "y": 20,
             "under_construction": 24},
            {"hp": 4, "owner": 2, "type": 4, "alive": 4, "x": 4, "y": 4,
             "under_construction": 4},
            {"hp": True, "owner": True, "type": True, "alive": True,
             "x": True, "y": True, "under_construction": True})
        with patch.object(races, "dwarf_struct_layout",
                          side_effect=((88, legacy), (100, legacy))):
            with self.assertRaisesRegex(RuntimeError, "unsupported or unbounded ABI width"):
                races.race_bank_profile(Path("bad-legacy-width.elf"),
                                         {"u": 128 * 88, "b": 64 * 100})

    def test_short_debugger_bank_read_is_rejected(self) -> None:
        with self.assertRaisesRegex(RuntimeError, "truncated entity bank"):
            races.read_entities(ShortRsp(), 0, 1, 8,
                                {"alive": 0, "owner": 1, "type": 2,
                                 "x": 4, "y": 6})


class BriefingRouteTests(unittest.TestCase):
    def test_campaign_smoke_deploys_with_start_after_opening_dialogue(self) -> None:
        for profile in ('z1', 'z2', 'p1', 'p2', 't1', 't2'):
            for unified in (False, True):
                with self.subTest(profile=profile, unified=unified):
                    events, first_live = races.campaign_start_events(profile, unified)
                    self.assertEqual(events[-4][1], races.KEY_A)  # Open briefing.
                    self.assertEqual(events[-2][1], races.KEY_START)  # Explicit skip.
                    self.assertEqual(events[-1][1], 0)
                    self.assertEqual(first_live, events[-1][0] + 1)

    def test_custom_game_keeps_its_a_launch_control(self) -> None:
        events = races.smoke_events('skirmish', 2, 'zerg')
        self.assertEqual(events[-2][1], races.KEY_A)


if __name__ == "__main__":
    unittest.main()
