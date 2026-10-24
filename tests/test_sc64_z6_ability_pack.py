"""Asset-free validation of authenticated Z6 ability fact extraction."""
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
SPEC = importlib.util.spec_from_file_location('sc64_z6_ability_pack_test', ROOT / 'tools/sc64_z6_ability_pack.py')
assert SPEC and SPEC.loader
PACK = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PACK
SPEC.loader.exec_module(PACK)


def hero_row(owner=3, valid=0x000B, energy=100, flags=0, raw=51):
    return struct.pack('<I6H4BIHHII', 1, 0, 0, raw, 0, 0, valid, owner,
                       100, 0, energy, 0, 0, flags, 0, 0)


def ptec_fixture(*, use_global=1, player=None, global_values=None):
    data = bytearray(PACK.PTEC_BYTES)
    player = player or {10:(1,0), 16:(0,0), 17:(1,0), 19:(1,0)}
    global_values = global_values or {10:(1,0), 16:(0,0), 17:(1,0), 19:(1,0)}
    for tech, (available, researched) in player.items():
        ix = 3 * 24 + tech
        data[PACK.PTEC_PLAYER_AVAILABLE + ix] = available
        data[PACK.PTEC_PLAYER_RESEARCHED + ix] = researched
        data[PACK.PTEC_PLAYER_USE_GLOBAL + ix] = use_global
    for tech, (available, researched) in global_values.items():
        data[PACK.PTEC_GLOBAL_AVAILABLE + tech] = available
        data[PACK.PTEC_GLOBAL_RESEARCHED + tech] = researched
    return bytes(data)


def sections(*, unit=None, ptec=None):
    return {b'UNIT': hero_row() if unit is None else unit,
            b'PTEC': ptec_fixture() if ptec is None else ptec}


def scenario(digest='a' * 64):
    return {'mission':'Z6', 'global_catalog_id':17, 'source_map_path':'008/019',
            'width':128, 'height':128, 'source_chk_sha256':digest}


def frame(tag, payload):
    return tag + struct.pack('<I', len(payload)) + payload


class FakeScenarioPacker:
    def __init__(self, expected): self.expected = dict(expected)
    def parse_source(self, root, mission):
        assert mission == 'Z6'
        return dict(self.expected)
    @staticmethod
    def parse_chk_sections(data):
        result, off = {}, 0
        while off < len(data):
            tag = data[off:off+4]
            size = struct.unpack_from('<I', data, off+4)[0]
            result[tag] = data[off+8:off+8+size]
            off += 8 + size
        return result


class Z6AbilityPackTests(unittest.TestCase):
    @unittest.skipUnless((PACK.DEFAULT_SOURCE_ROOT / 'z6' / 'source.chk').is_file(),
                         'requires privately prepared authenticated Z6 source')
    def test_real_authenticated_source_facts(self):
        result = PACK.verify_staged(PACK.DEFAULT_SOURCE_ROOT)
        self.assertEqual((result['global_catalog_id'], result['source_map_path']), (17,'008/019'))
        self.assertEqual((result['cloak_available'],result['cloak_researched']), (1,0))
        self.assertEqual((result['consume_available'],result['consume_researched']), (0,0))
        self.assertEqual((result['ensnare_available'],result['ensnare_researched']), (1,0))
        self.assertEqual((result['storm_available'],result['storm_researched']), (1,0))
        self.assertEqual((result['hero_initial_energy_percent'],result['hero_initial_cloaked']), (100,0))
        self.assertEqual((result['observer_raw_type'],result['observer_sight_tiles']), (84,9))
        self.assertEqual((result['photon_cannon_raw_type'],result['photon_cannon_sight_tiles']), (162,11))

    def test_ptec_global_default_and_player_override(self):
        data = ptec_fixture()
        self.assertEqual(PACK.ptec_value(data,3,10)['available'],1)
        override = ptec_fixture(use_global=0, player={10:(0,1),16:(0,0),17:(1,1),19:(1,1)})
        self.assertEqual((PACK.ptec_value(override,3,10)['available'], PACK.ptec_value(override,3,10)['researched']), (0,1))
        self.assertEqual((PACK.ptec_value(override,3,19)['available'], PACK.ptec_value(override,3,19)['researched']), (1,1))

    def test_ptec_rejects_bad_bounds_and_non_boolean_cells(self):
        data = ptec_fixture()
        with self.assertRaisesRegex(ValueError,'exactly 912'):
            PACK.ptec_value(data[:-1],3,10)
        with self.assertRaisesRegex(ValueError,'owner'):
            PACK.ptec_value(data,12,10)
        with self.assertRaisesRegex(ValueError,'tech'):
            PACK.ptec_value(data,3,24)
        for off in (PACK.PTEC_PLAYER_AVAILABLE+3*24+10,
                    PACK.PTEC_PLAYER_RESEARCHED+3*24+10,
                    PACK.PTEC_PLAYER_USE_GLOBAL+3*24+10,
                    PACK.PTEC_GLOBAL_AVAILABLE+10,
                    PACK.PTEC_GLOBAL_RESEARCHED+10):
            bad=bytearray(data); bad[off]=2
            with self.assertRaisesRegex(ValueError,'boolean'):
                PACK.ptec_value(bytes(bad),3,10)

    def test_hero_owner_energy_validity_and_initial_cloak_are_strict(self):
        row=PACK.decode_hero(hero_row())
        self.assertEqual((row['raw_type'],row['owner'],row['energy_percent']), (51,3,100))
        for args, pattern in (((2,0xB,100,0),'owner3'), ((3,3,100,0),'energy property'),
                              ((3,0xB,99,0),'percentage'), ((3,0xB,100,4),'initially cloaked')):
            with self.assertRaisesRegex(ValueError,pattern):
                PACK.decode_hero(hero_row(args[0],args[1],args[2],args[3]))
        with self.assertRaisesRegex(ValueError,'exactly one'):
            PACK.decode_hero(hero_row()+hero_row())

    def test_derivation_rejects_wrong_availability_and_map_identity(self):
        with self.assertRaisesRegex(ValueError,'must not enable Consume'):
            PACK.derive_facts(sections(ptec=ptec_fixture(global_values={10:(1,0),16:(1,0),17:(1,0),19:(1,0)})),scenario())
        with self.assertRaisesRegex(ValueError,'Personnel Cloaking available'):
            PACK.derive_facts(sections(ptec=ptec_fixture(global_values={10:(0,0),16:(0,0),17:(1,0),19:(1,0)})),scenario())
        wrong=scenario(); wrong['global_catalog_id']=18
        with self.assertRaisesRegex(ValueError,'identity'):
            PACK.derive_facts(sections(),wrong)

    def test_header_contains_only_numeric_audited_facts(self):
        facts=PACK.derive_facts(sections(),scenario())
        header=PACK.emit_header({'mission':'Z6',**facts})
        self.assertIn('SC64_Z6_ABILITIES_PERSONNEL_CLOAK_AVAILABLE 1u',header)
        self.assertIn('SC64_Z6_ABILITIES_CONSUME_AVAILABLE 0u',header)
        self.assertIn('SC64_Z6_ABILITIES_PSIONIC_STORM_AVAILABLE 1u',header)
        self.assertIn('SC64_Z6_ABILITIES_PSIONIC_STORM_HERO_RESEARCH_EXEMPTION 1u',header)
        self.assertIn('SC64_Z6_ABILITIES_OBSERVER_RAW_TYPE 84u',header)
        self.assertTrue(header.rstrip().endswith('SC64_Z6_ABILITIES_COMPLETE 1u\n#endif'))
        self.assertNotIn('dialogue',header.lower())

    def test_source_parse_authenticates_reread_hash(self):
        chunks=sections()
        chk=b''.join(frame(tag,payload) for tag,payload in chunks.items())
        digest=hashlib.sha256(chk).hexdigest()
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); (root/'z6').mkdir(); (root/'z6/source.chk').write_bytes(chk)
            with mock.patch.object(PACK,'_load_scenario_packer',return_value=FakeScenarioPacker(scenario(digest))):
                self.assertEqual(PACK.parse_source(root)['source_chk_sha256'],digest)
            with mock.patch.object(PACK,'_load_scenario_packer',return_value=FakeScenarioPacker(scenario('0'*64))):
                with self.assertRaisesRegex(ValueError,'changed between'):
                    PACK.parse_source(root)


if __name__ == '__main__':
    unittest.main()
