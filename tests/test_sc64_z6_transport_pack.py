"""Asset-free checks for authenticated Z6 Ventral Sacs source facts."""
from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_z6_transport_pack_test', ROOT / 'tools/sc64_z6_transport_pack.py')
assert SPEC and SPEC.loader
PACK = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PACK
SPEC.loader.exec_module(PACK)


def scenario(digest='a' * 64):
    return {'mission': 'Z6', 'global_catalog_id': 17, 'source_map_path': '008/019',
            'width': 128, 'height': 128, 'source_chk_sha256': digest}


def upgs_fixture(*, use_default=1, values=(200, 0, 200, 0, 2400, 0)):
    data = bytearray(PACK.UPGS_BYTES)
    data[PACK.UPGRADE_ID] = use_default
    off = PACK.UPGRADE_COUNT
    for value in values:
        struct.pack_into('<H', data, off + PACK.UPGRADE_ID * 2, value)
        off += PACK.UPGRADE_COUNT * 2
    return bytes(data)


def upgr_fixture(*, slot=3, max_level=1, current_level=0, use_global=1,
                 global_max=1, global_current=0):
    n = PACK.UPGRADE_COUNT
    data = bytearray(PACK.UPGR_BYTES)
    data[slot * n + PACK.UPGRADE_ID] = max_level
    data[n * 12 + slot * n + PACK.UPGRADE_ID] = current_level
    base = n * 24
    data[base + PACK.UPGRADE_ID] = global_max
    data[base + n + PACK.UPGRADE_ID] = global_current
    data[base + n * 2 + slot * n + PACK.UPGRADE_ID] = use_global
    return bytes(data)


def puni_fixture(*, values=None):
    data = bytearray(PACK.PUNI_BYTES)
    values = values or {42: 1, 131: 1, 132: 1, 133: 1}
    for raw, allow in values.items():
        pi = PACK.SOURCE_OWNER * 228 + raw
        data[pi] = allow
        data[12 * 228 + raw] = allow
        data[13 * 228 + pi] = 1
    return bytes(data)


def unit_row(raw, owner=3):
    row = bytearray(PACK.UNIT_BYTES)
    struct.pack_into('<H', row, 8, raw)
    row[16] = owner
    return bytes(row)


def section_fixture(*, unit=None, upgs=None, upgr=None, puni=None, owners=None):
    rows = [unit_row(42) for _ in range(3)] + [unit_row(131)]
    if unit is not None:
        rows = [unit]
    return {b'UNIT': b''.join(rows), b'UPGS': upgs or upgs_fixture(),
            b'UPGR': upgr or upgr_fixture(), b'PUNI': puni or puni_fixture(),
            b'OWNR': owners or bytes([0, 5, 5, 6, 0, 0, 0, 0])}


def frame(tag, payload):
    return tag + struct.pack('<I', len(payload)) + payload


class FakeScenarioPacker:
    def __init__(self, expected): self.expected = dict(expected)
    def parse_source(self, source_root, mission):
        assert mission == 'Z6'
        return dict(self.expected)
    @staticmethod
    def parse_chk_sections(data):
        result, pos = {}, 0
        while pos < len(data):
            tag = data[pos:pos + 4]
            size = struct.unpack_from('<I', data, pos + 4)[0]
            result[tag] = data[pos + 8:pos + 8 + size]
            pos += 8 + size
        return result


class Z6TransportPackTests(unittest.TestCase):
    @unittest.skipUnless((PACK.DEFAULT_SOURCE_ROOT / 'z6' / 'source.chk').is_file(),
                         'requires privately staged authenticated Z6 source')
    def test_real_authenticated_source(self):
        facts = PACK.verify_staged(PACK.DEFAULT_SOURCE_ROOT)
        self.assertEqual((facts['global_catalog_id'], facts['source_map_path']), (17, '008/019'))
        self.assertEqual((facts['upgrade_id'], facts['source_max_level'],
                          facts['source_initial_level']), (24, 1, 0))
        self.assertEqual((facts['mineral_cost'], facts['gas_cost'],
                          facts['time_cost_source_frames']), (200, 200, 2400))
        self.assertEqual((facts['source_overlord_allowed'], facts['source_lair_allowed'],
                          facts['source_hive_allowed']), (1, 1, 1))
        self.assertEqual((facts['starting_overlord_count'], facts['starting_hatchery_count'],
                          facts['starting_lair_count'], facts['starting_hive_count']), (3, 1, 0, 0))

    def test_upgs_array_layout_and_default_costs(self):
        decoded = PACK.decode_upgs(upgs_fixture())
        self.assertEqual((decoded['uses_default_settings'], decoded['mineral_cost'],
                          decoded['gas_cost'], decoded['time_cost']), (1, 200, 200, 2400))
        data = bytearray(upgs_fixture()); data[PACK.UPGRADE_ID] = 2
        with self.assertRaisesRegex(ValueError, 'boolean'):
            PACK.decode_upgs(bytes(data))
        with self.assertRaisesRegex(ValueError, 'exactly 598'):
            PACK.decode_upgs(bytes(PACK.UPGS_BYTES - 1))
        with self.assertRaisesRegex(ValueError, '0..45'):
            PACK.decode_upgs(bytes(PACK.UPGS_BYTES), 46)

    def test_upgr_resolves_per_player_global_default(self):
        row = PACK.decode_upgr(upgr_fixture())
        self.assertEqual((row['uses_global_default'], row['max_level'], row['current_level']),
                         (1, 1, 0))
        override = PACK.decode_upgr(upgr_fixture(max_level=1, current_level=1,
                                                  use_global=0, global_current=0))
        self.assertEqual((override['max_level'], override['current_level']), (1, 1))
        data = bytearray(upgr_fixture())
        data[PACK.UPGRADE_COUNT * 26 + 3 * PACK.UPGRADE_COUNT + 24] = 2
        with self.assertRaisesRegex(ValueError, 'boolean'):
            PACK.decode_upgr(bytes(data))

    def test_puni_resolution_and_source_owner_unit_census(self):
        self.assertEqual(PACK.decode_puni(puni_fixture(), unit_raw=42)['available'], 1)
        self.assertEqual(PACK.decode_puni(puni_fixture(), unit_raw=132)['available'], 1)
        self.assertEqual(PACK.source_unit_census(
            unit_row(42) * 3 + unit_row(131))['owner3_raw42_count'], 3)
        data = bytearray(puni_fixture()); data[3 * 228 + 42] = 3
        with self.assertRaisesRegex(ValueError, 'boolean'):
            PACK.decode_puni(bytes(data))

    def test_derivation_fails_closed_for_source_drift(self):
        s = section_fixture()
        self.assertEqual(PACK.derive_facts(s, scenario())['starting_overlord_count'], 3)
        with self.assertRaisesRegex(ValueError, 'cost/default'):
            PACK.derive_facts(section_fixture(upgs=upgs_fixture(values=(199,0,200,0,2400,0))), scenario())
        with self.assertRaisesRegex(ValueError, 'start at level0'):
            PACK.derive_facts(section_fixture(upgr=upgr_fixture(current_level=1, use_global=0)), scenario())
        with self.assertRaisesRegex(ValueError, 'PUNI'):
            PACK.derive_facts(section_fixture(puni=puni_fixture(values={42:0,131:1,132:1,133:1})), scenario())
        with self.assertRaisesRegex(ValueError, 'census changed'):
            PACK.derive_facts(section_fixture(unit=unit_row(42)), scenario())
        with self.assertRaisesRegex(ValueError, 'human slot'):
            PACK.derive_facts(section_fixture(owners=bytes(8)), scenario())
        wrong = scenario(); wrong['global_catalog_id'] = 18
        with self.assertRaisesRegex(ValueError, 'identity'):
            PACK.derive_facts(s, wrong)

    def test_emitter_is_numeric_and_parse_authenticates_reread(self):
        facts = PACK.derive_facts(section_fixture(), scenario())
        header = PACK.emit_header({'mission': 'Z6', 'global_catalog_id': 17, **facts})
        self.assertIn('SC64_Z6_TRANSPORT_UPGRADE_ID 24u', header)
        self.assertIn('SC64_Z6_TRANSPORT_MINERAL_COST 200u', header)
        self.assertNotIn('original source map', header.lower())
        sections = section_fixture()
        chk = b''.join(frame(tag, value) for tag, value in sections.items())
        digest = hashlib.sha256(chk).hexdigest()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); (root / 'z6').mkdir()
            (root / 'z6' / 'source.chk').write_bytes(chk)
            with mock.patch.object(PACK, '_load_scenario_packer',
                                   return_value=FakeScenarioPacker(scenario(digest))):
                self.assertEqual(PACK.parse_source(root)['source_chk_sha256'], digest)
            with mock.patch.object(PACK, '_load_scenario_packer',
                                   return_value=FakeScenarioPacker(scenario('0' * 64))):
                with self.assertRaisesRegex(ValueError, 'changed between'):
                    PACK.parse_source(root)


if __name__ == '__main__':
    unittest.main()
