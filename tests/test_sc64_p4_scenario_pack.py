"""Asset-free structure and rejection tests for numeric P4 trigger staging."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import types
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_p4_scenario_pack_test', ROOT / 'tools/sc64_p4_scenario_pack.py')
assert SPEC is not None and SPEC.loader is not None
PACK = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PACK
SPEC.loader.exec_module(PACK)


def frame(tag: bytes, payload: bytes) -> bytes:
    return tag + len(payload).to_bytes(4, 'little') + payload


def make_trigger_payload() -> bytes:
    payload = bytearray(PACK.TRIGGER_COUNT * 2400)
    for trigger in range(PACK.TRIGGER_COUNT):
        base = trigger * 2400
        for index in range(PACK.CONDITION_COUNTS[trigger]):
            struct.pack_into('<IIIHBBBBH', payload, base + index * 20,
                             index + 1, 13, 0x100 + trigger, 37, 0, 1, 0, 0, 0)
        for index in range(PACK.ACTION_COUNTS[trigger]):
            opcode = 11 if trigger == 1 and index == 0 else 2
            struct.pack_into('<IIIIIIHBBB3s', payload,
                             base + 320 + index * 32,
                             index + 1, 1000 + index, 2000 + index,
                             3000 + index, 13, 0, 37, opcode, 0, 16, b'\0\0\0')
        payload[base + 2372 + 18] = 1
    return bytes(payload)


def staged_fixture(root: Path) -> bytes:
    unit = bytearray(PACK.UNIT_BYTES)
    for index, raw in ((0, 67), (1, 79)):
        struct.pack_into('<H', unit, index * 36 + PACK.UNIT_ID_OFFSET, raw)
        struct.pack_into('<H', unit, index * 36 + PACK.UNIT_STATE_FLAGS_OFFSET,
                         PACK.UNIT_ENERGY_VALID)
        unit[index * 36 + 19] = 100
    ptec = bytearray(PACK.PTEC_BYTES)
    player_tech = PACK.P4_HUMAN_SLOT * PACK.PTEC_PLAYER_BYTES + PACK.PSIONIC_STORM_TECH_ID
    ptec[PACK.PTEC_GLOBAL_AVAILABLE_OFFSET + PACK.PSIONIC_STORM_TECH_ID] = 1
    ptec[PACK.PTEC_GLOBAL_RESEARCHED_OFFSET + PACK.PSIONIC_STORM_TECH_ID] = 1
    ptec[PACK.PTEC_USE_DEFAULT_OFFSET + player_tech] = 1
    records = {
        b'TRIG': make_trigger_payload(),
        b'MRGN': b''.join(struct.pack('<IIIIHH', i, i + 1, i + 2, i + 3,
                                      i + 4, i + 5) for i in range(64)),
        b'OWNR': bytes(range(12)),
        b'SIDE': bytes(range(12, 24)),
        b'FORC': bytes(range(20)),
        b'UPRP': bytes((i * 7) & 0xff for i in range(64 * 20)),
        b'UPUS': bytes(i % 2 for i in range(64)),
        b'PTEC': bytes(ptec),
        b'UNIT': bytes(unit),
    }
    chk = b''.join(frame(tag, value) for tag, value in records.items())
    (root / 'records').mkdir(parents=True)
    for tag, value in records.items():
        name = tag.decode('ascii')
        (root / 'records' / f'{name}.bin').write_bytes(frame(tag, value))
    (root / 'source.chk').write_bytes(chk)
    return chk


class P4ScenarioPackTests(unittest.TestCase):
    def test_parses_all_13_records_and_preserves_numeric_metadata(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            chk = staged_fixture(source)
            verifier = types.SimpleNamespace(verify_staged=lambda _path: {})
            with (mock.patch.object(PACK, '_load_pack', return_value=verifier),
                  mock.patch.object(PACK, 'CHK_SHA256', hashlib.sha256(chk).hexdigest())):
                data = PACK.parse_source(source)
                verified = PACK.verify_staged(source)
            self.assertEqual(len(data['triggers']), 13)
            self.assertEqual(len(data['actions']), 153)
            self.assertEqual(len(data['conditions']), 16)
            self.assertEqual([t['first_action'] for t in data['triggers']],
                             [0, 1, 7, 13, 29, 37, 62, 71, 116, 122, 124, 145, 148])
            self.assertEqual([t['first_condition'] for t in data['triggers']],
                             [0, 1, 3, 4, 6, 8, 9, 10, 11, 12, 13, 14, 15])
            self.assertEqual(data['actions'][1]['string_id'], 1000)
            self.assertEqual(data['actions'][1]['wav_id'], 2000)
            self.assertEqual(data['actions'][1]['time'], 3000)
            self.assertEqual(data['actions'][1]['group1'], 13)
            self.assertEqual(data['locations'][63]['flags'], 68)
            self.assertEqual(data['owners'], list(range(12)))
            self.assertEqual(data['sides'], list(range(12, 24)))
            self.assertEqual(data['forces'], list(range(20)))
            self.assertEqual(data['properties'][63], list((i * 7) & 0xff
                                                            for i in range(1260, 1280)))
            self.assertEqual(data['property_used'], [i % 2 for i in range(64)])
            self.assertEqual(data['actor_creation']['total_instances_per_execution'], 1)
            self.assertEqual(data['psionic_storm'], {
                'slot': 7, 'tech_id': 19, 'availability_ptec': 0,
                'researched_ptec': 0, 'global_availability_ptec': 1,
                'global_researched_ptec': 1, 'use_default_ptec': 1,
                'effective_available': 1, 'effective_researched': 1,
                'energy_percent_by_raw_type': {'67': 100, '79': 100}})
            self.assertEqual(data['actor_creation']['instances_per_execution_by_trigger_and_raw_unit'],
                             {'1': {'37': 1}})
            self.assertEqual(verified['created_instances_per_execution'], 1)

    def test_opcode_11_quantity_helper_uses_known_zero_default(self):
        self.assertEqual(PACK.create_instance_count({'opcode': 11, 'modifier': 0}), 1)
        self.assertEqual(PACK.create_instance_count({'opcode': 11, 'modifier': 2}), 2)
        with self.assertRaisesRegex(ValueError, 'expected Create Unit'):
            PACK.create_instance_count({'opcode': 2, 'modifier': 0})
        with self.assertRaisesRegex(ValueError, 'outside its byte field'):
            PACK.create_instance_count({'opcode': 11, 'modifier': 256})

    def test_header_contains_complete_numeric_tables_without_dialogue(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            chk = staged_fixture(source)
            verifier = types.SimpleNamespace(verify_staged=lambda _path: {})
            with (mock.patch.object(PACK, '_load_pack', return_value=verifier),
                  mock.patch.object(PACK, 'CHK_SHA256', hashlib.sha256(chk).hexdigest())):
                data = PACK.parse_source(source)
            header = PACK.emit_header(data)
            self.assertIn('sc64_p4_source_actions[153]', header)
            self.assertIn('sc64_p4_source_conditions[16]', header)
            self.assertIn('sc64_p4_source_triggers[13]', header)
            self.assertIn('sc64_p4_source_locations[64]', header)
            self.assertIn('sc64_p4_source_properties[64][20]', header)
            self.assertIn('sc64_p4_source_forces[20]', header)
            self.assertIn('sc64_p4_source_property_used[64]', header)
            self.assertIn('sc64_p4_source_storm_slot7_researched_ptec', header)
            self.assertIn('sc64_p4_source_energy_percent_67', header)
            self.assertIn('sc64_p4_source_energy_percent_79', header)
            self.assertNotIn('Tassadar', header)
            self.assertNotIn('private fixture strings', header)
            self.assertNotIn('wav bytes', header)

    def test_effective_ptec_honors_default_and_explicit_player_flags(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            chk = staged_fixture(source)
            verifier = types.SimpleNamespace(verify_staged=lambda _path: {})
            with (mock.patch.object(PACK, '_load_pack', return_value=verifier),
                  mock.patch.object(PACK, 'CHK_SHA256', hashlib.sha256(chk).hexdigest())):
                defaulted = PACK.parse_source(source)['psionic_storm']
                self.assertEqual((defaulted['effective_available'],
                                  defaulted['effective_researched']), (1, 1))

            sections = PACK._parse_sections(chk)
            ptec = bytearray(sections[b'PTEC'])
            player_tech = PACK.P4_HUMAN_SLOT * PACK.PTEC_PLAYER_BYTES + PACK.PSIONIC_STORM_TECH_ID
            ptec[PACK.PTEC_USE_DEFAULT_OFFSET + player_tech] = 0
            ptec[PACK.PTEC_AVAILABLE_OFFSET + player_tech] = 1
            ptec[PACK.PTEC_RESEARCHED_OFFSET + player_tech] = 1
            sections[b'PTEC'] = bytes(ptec)
            changed = b''.join(tag + len(value).to_bytes(4, 'little') + value
                               for tag, value in sections.items())
            (source / 'source.chk').write_bytes(changed)
            with (mock.patch.object(PACK, '_load_pack', return_value=verifier),
                  mock.patch.object(PACK, 'CHK_SHA256', hashlib.sha256(changed).hexdigest())):
                explicit = PACK.parse_source(source)['psionic_storm']
                self.assertEqual((explicit['effective_available'],
                                  explicit['effective_researched']), (1, 1))

            ptec[PACK.PTEC_RESEARCHED_OFFSET + player_tech] = 0
            sections[b'PTEC'] = bytes(ptec)
            changed = b''.join(tag + len(value).to_bytes(4, 'little') + value
                               for tag, value in sections.items())
            (source / 'source.chk').write_bytes(changed)
            with (mock.patch.object(PACK, '_load_pack', return_value=verifier),
                  mock.patch.object(PACK, 'CHK_SHA256', hashlib.sha256(changed).hexdigest())):
                with self.assertRaisesRegex(ValueError, 'does not effectively mark.*researched'):
                    PACK.parse_source(source)

    def test_rejects_missing_source_caster_energy_and_validity_flag(self):

        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            chk = staged_fixture(source)
            sections = PACK._parse_sections(chk)
            unit = bytearray(sections[b'UNIT'])
            struct.pack_into('<H', unit, 36 + PACK.UNIT_ID_OFFSET, 42)
            sections[b'UNIT'] = bytes(unit)
            changed = b''.join(
                tag + len(value).to_bytes(4, 'little') + value
                for tag, value in sections.items())
            (source / 'source.chk').write_bytes(changed)
            verifier = types.SimpleNamespace(verify_staged=lambda _path: {})
            with (mock.patch.object(PACK, '_load_pack', return_value=verifier),
                  mock.patch.object(PACK, 'CHK_SHA256', hashlib.sha256(changed).hexdigest())):
                with self.assertRaisesRegex(ValueError, 'missing a High Templar or Tassadar'):
                    PACK.parse_source(source)

        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            chk = staged_fixture(source)
            sections = PACK._parse_sections(chk)
            unit = bytearray(sections[b'UNIT'])
            struct.pack_into('<H', unit, PACK.UNIT_STATE_FLAGS_OFFSET, 0)
            sections[b'UNIT'] = bytes(unit)
            changed = b''.join(tag + len(value).to_bytes(4, 'little') + value
                               for tag, value in sections.items())
            (source / 'source.chk').write_bytes(changed)
            verifier = types.SimpleNamespace(verify_staged=lambda _path: {})
            with (mock.patch.object(PACK, '_load_pack', return_value=verifier),
                  mock.patch.object(PACK, 'CHK_SHA256', hashlib.sha256(changed).hexdigest())):
                with self.assertRaisesRegex(ValueError, 'lacks the UNIT energy-valid flag'):
                    PACK.parse_source(source)

    def test_rejects_action_holes_invalid_locations_and_record_tampering(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            chk = staged_fixture(source)
            verifier = types.SimpleNamespace(verify_staged=lambda _path: {})
            with mock.patch.object(PACK, '_load_pack', return_value=verifier):
                trig = bytearray(_unframe_record(source / 'records/TRIG.bin', b'TRIG'))
                # Trigger 1 has active action 2 after a now-empty action 1.
                trig[2400 + 320 + 32:2400 + 320 + 64] = bytes(32)
                replace_trigger(source, chk, bytes(trig))
                changed_chk = (source / 'source.chk').read_bytes()
                with mock.patch.object(PACK, 'CHK_SHA256',
                                       hashlib.sha256(changed_chk).hexdigest()):
                    with self.assertRaisesRegex(ValueError, 'noncontiguous active actions'):
                        PACK.parse_source(source)

        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp)
            chk = staged_fixture(source)
            verifier = types.SimpleNamespace(verify_staged=lambda _path: {})
            with mock.patch.object(PACK, '_load_pack', return_value=verifier):
                # Keep staged records consistent with CHK, but reference an
                # out-of-range location in an active action.
                trig = bytearray(_unframe_record(source / 'records/TRIG.bin', b'TRIG'))
                struct.pack_into('<I', trig, 320, 65)
                replace_trigger(source, chk, bytes(trig))
                changed_chk = (source / 'source.chk').read_bytes()
                with mock.patch.object(PACK, 'CHK_SHA256',
                                       hashlib.sha256(changed_chk).hexdigest()):
                    with self.assertRaisesRegex(ValueError, 'invalid location'):
                        PACK.parse_source(source)


def _unframe_record(path: Path, tag: bytes) -> bytes:
    data = path.read_bytes()
    assert data[:4] == tag
    return data[8:]


def replace_trigger(root: Path, chk: bytes, trig: bytes) -> None:
    # Rebuild exactly the synthetic section sequence with only TRIG changed.
    sections = PACK._parse_sections(chk)
    sections[b'TRIG'] = trig
    changed = b''.join(frame(tag, payload) for tag, payload in sections.items())
    (root / 'source.chk').write_bytes(changed)
    (root / 'records/TRIG.bin').write_bytes(frame(b'TRIG', trig))


if __name__ == '__main__':
    unittest.main()
