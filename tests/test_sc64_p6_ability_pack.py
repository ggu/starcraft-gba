"""Asset-free tests for authenticated P6 ability fact extraction."""
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
    'sc64_p6_ability_pack_test', ROOT / 'tools/sc64_p6_ability_pack.py')
assert SPEC is not None and SPEC.loader is not None
PACK = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PACK
SPEC.loader.exec_module(PACK)


def unit_row(raw=79, owner=2, valid=0x0008, energy=100, flags=0):
    row = bytearray(36)
    struct.pack_into('<H', row, 8, raw)
    struct.pack_into('<H', row, 14, valid)
    row[16] = owner
    row[19] = energy
    struct.pack_into('<H', row, 26, flags)
    return bytes(row)


def ptec(*, use_global=1, player=(1, 1), global_row=(1, 1)):
    data = bytearray(PACK.PTEC_BYTES)
    index = 2 * 24 + 19
    data[PACK.PLAYER_AVAILABLE + index] = player[0]
    data[PACK.PLAYER_RESEARCHED + index] = player[1]
    data[PACK.PLAYER_USE_GLOBAL + index] = use_global
    data[PACK.GLOBAL_AVAILABLE + 19] = global_row[0]
    data[PACK.GLOBAL_RESEARCHED + 19] = global_row[1]
    return bytes(data)


def scenario(digest='a' * 64):
    return {'mission': 'P6', 'global_catalog_id': 25,
            'source_map_path': '008/023', 'width': 128, 'height': 128,
            'source_chk_sha256': digest}


def frame(tag, data):
    return tag + struct.pack('<I', len(data)) + data


class FakePacker:
    def __init__(self, value): self.value = dict(value)
    def parse_source(self, source_root, mission):
        assert mission == 'P6'
        return dict(self.value)
    @staticmethod
    def parse_chk_sections(data):
        sections = {}
        offset = 0
        while offset < len(data):
            tag = data[offset:offset + 4]
            size = struct.unpack_from('<I', data, offset + 4)[0]
            sections[tag] = data[offset + 8:offset + 8 + size]
            offset += 8 + size
        return sections


class P6AbilityPackTests(unittest.TestCase):
    def sections(self, hero=None, tech=None):
        return {b'UNIT': unit_row() if hero is None else hero,
                b'PTEC': ptec() if tech is None else tech}

    def test_ptec_uses_owner_two_and_validated_player_or_global_cells(self):
        source = ptec(use_global=1, player=(0, 0), global_row=(1, 1))
        self.assertEqual((PACK.ptec_value(source, 2, 19)['available'],
                          PACK.ptec_value(source, 2, 19)['researched']), (1, 1))
        source = ptec(use_global=0, player=(1, 1), global_row=(0, 0))
        self.assertEqual((PACK.ptec_value(source, 2, 19)['available'],
                          PACK.ptec_value(source, 2, 19)['researched']), (1, 1))
        for owner, tech in ((12, 19), (2, 24)):
            with self.assertRaises(ValueError): PACK.ptec_value(ptec(), owner, tech)
        with self.assertRaisesRegex(ValueError, 'exactly 912'):
            PACK.ptec_value(ptec()[:-1], 2, 19)
        for offset in (PACK.PLAYER_AVAILABLE + 67, PACK.PLAYER_RESEARCHED + 67,
                       PACK.PLAYER_USE_GLOBAL + 67, PACK.GLOBAL_AVAILABLE + 19,
                       PACK.GLOBAL_RESEARCHED + 19):
            bad = bytearray(ptec()); bad[offset] = 2
            with self.assertRaisesRegex(ValueError, 'boolean'):
                PACK.ptec_value(bytes(bad), 2, 19)

    def test_exact_raw79_identity_energy_and_flags(self):
        self.assertEqual(PACK.decode_tassadar(unit_row())['owner'], 2)
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            PACK.decode_tassadar(unit_row(79, 2) + unit_row(79, 2))
        with self.assertRaisesRegex(ValueError, 'owner2'):
            PACK.decode_tassadar(unit_row(owner=3))
        with self.assertRaisesRegex(ValueError, 'energy property'):
            PACK.decode_tassadar(unit_row(valid=0))
        with self.assertRaisesRegex(ValueError, 'percentage'):
            PACK.decode_tassadar(unit_row(energy=99))
        with self.assertRaisesRegex(ValueError, 'initially cloaked'):
            PACK.decode_tassadar(unit_row(flags=4))
        with self.assertRaisesRegex(ValueError, 'complete 36-byte'):
            PACK.decode_tassadar(unit_row()[:-1])

    def test_p6_identity_and_researched_storm_are_required(self):
        derived = PACK.derive_facts(self.sections(), scenario())
        self.assertEqual((derived['source_owner'], derived['storm_tech_id']), (2, 19))
        self.assertEqual((derived['storm_available'], derived['storm_researched']), (1, 1))
        self.assertEqual((derived['hero_initial_energy_percent'], derived['hero_initial_cloaked']), (100, 0))
        header = PACK.emit_header({'mission': 'P6', **derived})
        self.assertIn('SC64_P6_ABILITIES_PSIONIC_STORM_AVAILABLE 1u', header)
        self.assertIn('SC64_P6_ABILITIES_PSIONIC_STORM_RESEARCHED 1u', header)
        self.assertTrue(header.endswith('SC64_P6_ABILITIES_COMPLETE 1u\n#endif\n'))
        with self.assertRaisesRegex(ValueError, 'identity'):
            PACK.derive_facts(self.sections(), {**scenario(), 'global_catalog_id': 24})
        with self.assertRaisesRegex(ValueError, 'does not authorize'):
            PACK.derive_facts(self.sections(tech=ptec(global_row=(1, 0))), scenario())
        with self.assertRaisesRegex(ValueError, 'does not authorize'):
            PACK.derive_facts(self.sections(tech=ptec(global_row=(0, 1))), scenario())

    def test_authenticated_source_is_reread_and_hash_compared(self):
        chk = frame(b'UNIT', unit_row()) + frame(b'PTEC', ptec())
        digest = hashlib.sha256(chk).hexdigest()
        fake = FakePacker(scenario(digest))
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp) / 'p6'; folder.mkdir()
            (folder / 'source.chk').write_bytes(chk)
            with mock.patch.object(PACK, '_load_packer', return_value=fake):
                result = PACK.parse_source(Path(tmp))
            self.assertEqual(result['source_chk_sha256'], digest)
            with mock.patch.object(PACK, '_load_packer', return_value=FakePacker(scenario('0' * 64))):
                with self.assertRaisesRegex(ValueError, 'changed between'):
                    PACK.parse_source(Path(tmp))


if __name__ == '__main__':
    unittest.main()
