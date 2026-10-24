"""Asset-free validation for private Z5 ability-fact extraction."""
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
    'sc64_z5_ability_pack_test', ROOT / 'tools/sc64_z5_ability_pack.py')
assert SPEC is not None and SPEC.loader is not None
PACK = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PACK
SPEC.loader.exec_module(PACK)


def hero_row(*, owner=3, valid_properties=0x000B, energy_percent=100,
             flags=0, raw_type=51):
    return struct.pack('<I6H4BIHHII', 1, 3393, 230, raw_type, 0, 0,
                       valid_properties, owner, 100, 0, energy_percent,
                       0, 0, flags, 0, 0)


def trap_payload(*, disable_203=3, disable_209=6):
    rows = []
    index = 0
    for raw_type, count, disabled in (
        (203, 9, disable_203), (209, 10, disable_209),
        (211, 5, 0), (213, 1, 0),
        (205, 2, 0), (206, 4, 0), (207, 4, 0), (208, 6, 0),
    ):
        for item in range(count):
            flags = 0x8000 if item < disabled else 0
            rows.append(struct.pack('<HHHBBH', raw_type, 100 + index,
                                    200 + index, 6, 0, flags))
            index += 1
    for item in range(30):
        rows.append(struct.pack('<HHHBBH', 127, 100 + index, 200 + index,
                                0, 0, 0x1000))
        index += 1
    return b''.join(rows)


def ptec_fixture(*, use_global=1, player_cloak=(1, 0), player_storm=(1, 0),
                 global_cloak=(1, 0), global_storm=(0, 0)):
    data = bytearray(PACK.PTEC_BYTES)
    owner = PACK.HERO_SOURCE_OWNER

    def set_player(tech, available, researched):
        index = owner * PACK.PTEC_TECH_COUNT + tech
        data[PACK.PTEC_PLAYER_AVAILABLE + index] = available
        data[PACK.PTEC_PLAYER_RESEARCHED + index] = researched
        data[PACK.PTEC_PLAYER_USE_GLOBAL + index] = use_global

    def set_global(tech, available, researched):
        data[PACK.PTEC_GLOBAL_AVAILABLE + tech] = available
        data[PACK.PTEC_GLOBAL_RESEARCHED + tech] = researched

    set_player(PACK.PTEC_CLOAK_ID, *player_cloak)
    set_player(PACK.PTEC_STORM_ID, *player_storm)
    set_global(PACK.PTEC_CLOAK_ID, *global_cloak)
    set_global(PACK.PTEC_STORM_ID, *global_storm)
    return bytes(data)


def sections_fixture(*, units=None, ptec=None, thg2=None):
    return {
        b'UNIT': hero_row() if units is None else units,
        b'PTEC': ptec_fixture() if ptec is None else ptec,
        b'THG2': trap_payload() if thg2 is None else thg2,
    }


def scenario_fixture(digest='a' * 64):
    return {'mission': 'Z5', 'global_catalog_id': 16,
            'source_map_path': '008/018', 'width': 128, 'height': 128,
            'source_chk_sha256': digest}


def frame(tag, payload):
    return tag + struct.pack('<I', len(payload)) + payload


class FakeScenarioPacker:
    def __init__(self, expected_scenario):
        self.expected_scenario = expected_scenario
        self.missions = []

    def parse_source(self, source_root, mission):
        self.missions.append(mission)
        return dict(self.expected_scenario)

    @staticmethod
    def parse_chk_sections(chk):
        sections = {}
        offset = 0
        while offset < len(chk):
            tag = chk[offset:offset + 4]
            size = struct.unpack_from('<I', chk, offset + 4)[0]
            sections[tag] = chk[offset + 8:offset + 8 + size]
            offset += 8 + size
        return sections


class Z5AbilityPackTests(unittest.TestCase):
    def test_ptec_global_default_and_explicit_player_override(self):
        payload = ptec_fixture()
        cloak = PACK.ptec_value(payload, 3, 10)
        storm = PACK.ptec_value(payload, 3, 19)
        self.assertEqual((cloak['available'], cloak['researched']), (1, 0))
        self.assertEqual((storm['available'], storm['researched']), (0, 0))

        override = ptec_fixture(use_global=0, player_cloak=(0, 1),
                                global_cloak=(1, 0), player_storm=(1, 1),
                                global_storm=(0, 0))
        self.assertEqual(PACK.ptec_value(override, 3, 10)['available'], 0)
        self.assertEqual(PACK.ptec_value(override, 3, 10)['researched'], 1)
        self.assertEqual(PACK.ptec_value(override, 3, 19)['available'], 1)
        self.assertEqual(PACK.ptec_value(override, 3, 19)['researched'], 1)

    def test_ptec_bounds_length_and_target_booleans_are_strict(self):
        payload = ptec_fixture()
        with self.assertRaisesRegex(ValueError, 'exactly 912'):
            PACK.ptec_value(payload[:-1], 3, 10)
        with self.assertRaisesRegex(ValueError, 'owner'):
            PACK.ptec_value(payload, 12, 10)
        with self.assertRaisesRegex(ValueError, 'tech'):
            PACK.ptec_value(payload, 3, 24)
        offsets = (
            PACK.PTEC_PLAYER_AVAILABLE + 3 * 24 + 10,
            PACK.PTEC_PLAYER_RESEARCHED + 3 * 24 + 10,
            PACK.PTEC_PLAYER_USE_GLOBAL + 3 * 24 + 10,
            PACK.PTEC_GLOBAL_AVAILABLE + 10,
            PACK.PTEC_GLOBAL_RESEARCHED + 10,
        )
        for offset in offsets:
            with self.subTest(offset=offset):
                bad = bytearray(payload)
                bad[offset] = 2
                with self.assertRaisesRegex(ValueError, 'boolean'):
                    PACK.ptec_value(bytes(bad), 3, 10)

    def test_exact_hero_validity_energy_and_initial_cloak_fields(self):
        row = PACK.decode_hero(hero_row())
        self.assertEqual((row['raw_type'], row['owner']), (51, 3))
        self.assertTrue(row['valid_properties'] & 0x8)
        self.assertEqual(row['energy_percent'], 100)
        self.assertEqual(row['source_flags'], 0)
        with self.assertRaisesRegex(ValueError, 'energy property'):
            PACK.decode_hero(hero_row(valid_properties=0x3))
        with self.assertRaisesRegex(ValueError, 'percentage'):
            PACK.decode_hero(hero_row(energy_percent=99))
        with self.assertRaisesRegex(ValueError, 'initially cloaked'):
            PACK.decode_hero(hero_row(flags=0x4))
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            PACK.decode_hero(hero_row() + hero_row())
        with self.assertRaisesRegex(ValueError, 'owner3'):
            PACK.decode_hero(hero_row(owner=2))
        with self.assertRaisesRegex(ValueError, 'complete 36-byte'):
            PACK.decode_hero(hero_row()[:-1])

    def test_thg2_detector_traps_are_distinct_from_doors(self):
        facts = PACK.detector_census(trap_payload())
        self.assertEqual(facts['thg2_rows'], 71)
        self.assertEqual(facts['unit_backed_rows'], 41)
        self.assertEqual(facts['detector_traps'], 25)
        self.assertEqual(facts['detector_traps_initially_enabled'], 16)
        self.assertEqual(facts['non_detector_doors'], 16)
        self.assertEqual((facts['floor_missile_traps'], facts['floor_gun_traps'],
                          facts['wall_flame_traps'], facts['right_wall_flame_traps']),
                         (9, 10, 5, 1))
        with self.assertRaisesRegex(ValueError, 'enabled detector count'):
            PACK.detector_census(trap_payload(disable_203=4))

    def test_fact_derivation_and_header_marker_follow_full_validation(self):
        facts = PACK.derive_facts(sections_fixture(), scenario_fixture())
        self.assertEqual(facts['cloak_available'], 1)
        self.assertEqual(facts['cloak_researched'], 0)
        self.assertEqual(facts['storm_available'], 0)
        self.assertEqual(facts['storm_researched'], 0)
        header = PACK.emit_header({'mission': 'Z5', **facts})
        self.assertIn('SC64_Z5_ABILITIES_PERSONNEL_CLOAK_AVAILABLE 1u', header)
        self.assertIn('SC64_Z5_ABILITIES_PSIONIC_STORM_AVAILABLE 0u', header)
        self.assertIn('SC64_Z5_ABILITIES_DETECTOR_TRAP_COUNT 25u', header)
        self.assertIn('SC64_Z5_ABILITIES_DETECTOR_TRAP_INITIAL_ACTIVE_COUNT 16u', header)
        self.assertIn('SC64_Z5_ABILITIES_NON_DETECTOR_DOOR_COUNT 16u', header)
        self.assertTrue(header.rstrip().endswith('SC64_Z5_ABILITIES_COMPLETE 1u\n#endif'))
        self.assertNotIn('dialogue', header.lower())
        self.assertNotIn('source text', header.lower())

    def test_derivation_rejects_wrong_ptec_or_source_identity(self):
        with self.assertRaisesRegex(ValueError, 'does not enable'):
            PACK.derive_facts(sections_fixture(ptec=ptec_fixture(global_cloak=(0, 0))),
                              scenario_fixture())
        with self.assertRaisesRegex(ValueError, 'unexpectedly enables'):
            PACK.derive_facts(sections_fixture(ptec=ptec_fixture(global_storm=(1, 0))),
                              scenario_fixture())
        bad_scenario = scenario_fixture()
        bad_scenario['global_catalog_id'] = 17
        with self.assertRaisesRegex(ValueError, 'identity'):
            PACK.derive_facts(sections_fixture(), bad_scenario)

    def test_parse_source_authenticates_then_rereads_same_chk_hash(self):
        sections = sections_fixture()
        chk = b''.join(frame(tag, payload) for tag, payload in sections.items())
        digest = hashlib.sha256(chk).hexdigest()
        fake = FakeScenarioPacker(scenario_fixture(digest))
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / 'z5'
            source.mkdir()
            (source / 'source.chk').write_bytes(chk)
            with mock.patch.object(PACK, '_scenario_packer', return_value=fake):
                facts = PACK.parse_source(Path(tmp))
            self.assertEqual(fake.missions, ['Z5'])
            self.assertEqual(facts['source_chk_sha256'], digest)
            self.assertEqual(facts['detector_traps_initially_enabled'], 16)

            # The shared parser authenticated one hash; any changed reread is rejected.
            stale = FakeScenarioPacker(scenario_fixture('0' * 64))
            with mock.patch.object(PACK, '_scenario_packer', return_value=stale):
                with self.assertRaisesRegex(ValueError, 'changed between'):
                    PACK.parse_source(Path(tmp))


if __name__ == '__main__':
    unittest.main()
