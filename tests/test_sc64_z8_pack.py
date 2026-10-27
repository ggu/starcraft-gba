# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic fail-closed tests for the authored Z8 descriptor contract."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_z8_pack_test', ROOT / 'tools/sc64_z8_pack.py')
assert SPEC is not None and SPEC.loader is not None
PACK = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PACK
SPEC.loader.exec_module(PACK)


def synthetic_descriptor() -> dict:
    """Produce a shape-correct test descriptor without native source rows."""
    triggers = []
    actions = []
    conditions = []
    action_counts = [4] * 5 + [3] * 34  # 122 total
    condition_counts = [2] * 16 + [1] * 23  # 55 total
    for index in range(39):
        first_action = len(actions)
        first_condition = len(conditions)
        for row_index in range(action_counts[index]):
            actions.append({'trigger': index, 'index': row_index,
                            'location': 0, 'opcode': 1})
        for row_index in range(condition_counts[index]):
            conditions.append({'trigger': index, 'index': row_index,
                               'location': 0, 'opcode': 2})
        enabled = [0] * 27
        enabled[18] = 1
        triggers.append({
            'index': index,
            'first_action': first_action,
            'action_count': action_counts[index],
            'first_condition': first_condition,
            'condition_count': condition_counts[index],
            'execution_flags': 0,
            'initial_cursor': 0,
            'enabled_players': enabled,
        })

    units = []
    links = {192: 194, 194: 192, 195: 197, 197: 195}
    for index in range(198):
        linked_index = links.get(index)
        units.append({
            'record_index': index,
            'serial': 1000 + index,
            'x_source_px': 0, 'y_source_px': 0,
            'source_type': 134 if linked_index is not None else 37,
            'relation': 0, 'special': 0, 'valid': 0,
            'owner_raw': 0, 'hp_percent': 0, 'shield_percent': 0,
            'energy_percent': 0, 'resources': 0, 'hangar': 0, 'flags': 0,
            'unused_hex': '00000000',
            'linked_serial': 1000 + linked_index if linked_index is not None else 0,
            'linked_record_index': linked_index,
        })

    descriptor = {
        'schema': PACK.SCHEMA, 'mission': 'Z8', 'global_catalog_id': PACK.GLOBAL_ID,
        'source_map_path': PACK.MAP_PATH,
        'provenance': {
            'source_rom_sha1': PACK.EXPECTED_ROM_SHA1,
            'source_rom_bytes': PACK.EXPECTED_ROM_BYTES,
            'extractor_commit': PACK.EXTRACTOR_COMMIT,
            'source_chk_sha256': PACK.EXPECTED_CHK_SHA256,
            'source_chk_bytes': 1,
            'sections': [],
        },
        'dimensions': {'width': 128, 'height': 128},
        'counts': {
            'triggers': 39, 'actions': 122, 'conditions': 55, 'locations': 64,
            'unit_records': 198, 'preserve_actions': 13, 'create_actions': 16,
            'nydus_records': 4,
        },
        'histograms': {'action_opcode': {}, 'condition_opcode': {}, 'unit_raw_type': {}},
        'triggers': triggers,
        'conditions': conditions,
        'actions': actions,
        'locations': [{'index': i, 'left': 0, 'top': 0, 'right': 0, 'bottom': 0,
                       'name_id': 0, 'flags': 0} for i in range(1, 65)],
        'owners': [0] * 12, 'sides': [0] * 12, 'forces': [0] * 20,
        'properties': [[0] * 20 for _ in range(64)],
        'property_used': [0] * 64, 'puni': [0] * 5700,
        'unit_records': units,
        'ai_script_rows': [], 'ai_script_fourccs': ['MvTe', 'Ze8T', 'Zer8'],
        'nydus_pairs': [[192, 194], [195, 197]],
    }
    descriptor['descriptor_sha256'] = PACK._canonical_fingerprint(descriptor)
    return descriptor


class Z8PackContractTests(unittest.TestCase):
    def test_authored_source_identity_and_census_are_pinned(self):
        self.assertEqual(PACK.GLOBAL_ID, 19)
        self.assertEqual(PACK.MAP_PATH, '008/01B')
        self.assertEqual(PACK.EXPECTED_CHK_SHA256,
                         'ff6d0177a6a76fc68ef98f0962661857853ea11c50de9060b90ff934826cfd8e')
        self.assertEqual(PACK.EXPECTED_DESCRIPTOR_SHA256,
                         'f80b5f9e9419c324a6b86b9a11bbe3dab0ec545d20817fb0f26509e82c85a986')
        self.assertEqual(PACK.EXPECTED_COUNTS['preserve_actions'], 13)
        self.assertEqual(PACK.EXPECTED_COUNTS['nydus_records'], 4)
        self.assertEqual(sum(PACK.EXPECTED_ACTION_HISTOGRAM.values()), 122)
        self.assertEqual(sum(PACK.EXPECTED_CONDITION_HISTOGRAM.values()), 55)
        self.assertEqual(sum(PACK.EXPECTED_UNIT_HISTOGRAM.values()), 198)

    def test_valid_candidate_matches_expected_structure(self):
        expected = synthetic_descriptor()
        PACK.validate_candidate(copy.deepcopy(expected), expected)

    def test_metadata_hash_rows_and_counts_fail_closed(self):
        expected = synthetic_descriptor()
        for path, value in (
                (('provenance', 'source_chk_sha256'), '0' * 64),
                (('actions', 4, 'opcode'), 44),
                (('counts', 'actions'), 121)):
            with self.subTest(path=path):
                candidate = copy.deepcopy(expected)
                target = candidate
                for part in path[:-1]:
                    target = target[part]
                target[path[-1]] = value
                candidate['descriptor_sha256'] = PACK._canonical_fingerprint(candidate)
                with self.assertRaises(ValueError):
                    PACK.validate_candidate(candidate, expected)

    def test_bounds_and_nydus_links_fail_closed(self):
        expected = synthetic_descriptor()
        invalid_location = copy.deepcopy(expected)
        invalid_location['actions'][0]['location'] = 65
        invalid_location['descriptor_sha256'] = PACK._canonical_fingerprint(invalid_location)
        with self.assertRaisesRegex(ValueError, 'invalid location'):
            PACK.validate_structure(invalid_location)

        invalid_link = copy.deepcopy(expected)
        invalid_link['unit_records'][192]['linked_record_index'] = 193
        invalid_link['descriptor_sha256'] = PACK._canonical_fingerprint(invalid_link)
        with self.assertRaisesRegex(ValueError, 'invalid linked serial'):
            PACK.validate_structure(invalid_link)

    def test_row_content_tamper_fails_even_with_recomputed_fingerprint(self):
        expected = synthetic_descriptor()
        candidate = copy.deepcopy(expected)
        candidate['actions'][0]['group2'] = 12345
        candidate['descriptor_sha256'] = PACK._canonical_fingerprint(candidate)
        with self.assertRaisesRegex(ValueError, 'rows or provenance differ'):
            PACK.validate_candidate(candidate, expected)

        candidate = copy.deepcopy(expected)
        candidate['properties'][7][11] = 1
        candidate['descriptor_sha256'] = PACK._canonical_fingerprint(candidate)
        with self.assertRaisesRegex(ValueError, 'rows or provenance differ'):
            PACK.validate_candidate(candidate, expected)

    def test_non_object_unit_rows_raise_value_error(self):
        candidate = synthetic_descriptor()
        candidate['unit_records'][0] = ['malformed']
        candidate['descriptor_sha256'] = PACK._canonical_fingerprint(candidate)
        with self.assertRaisesRegex(ValueError, 'UNIT row 0 is not an object'):
            PACK.validate_structure(candidate)

    def test_duplicate_descriptor_json_keys_are_rejected(self):
        expected = synthetic_descriptor()
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'descriptor.json'
            path.write_text('{"schema": 1, "schema": 1}', encoding='utf-8')
            with mock.patch.object(PACK, 'build_descriptor', return_value=expected):
                with self.assertRaisesRegex(ValueError, 'duplicate key'):
                    PACK.validate_descriptor_file(path)

    def test_private_output_refuses_outside_existing_and_symlink_paths(self):
        outside = Path(tempfile.gettempdir()) / 'z8-pack-outside.json'
        with self.assertRaisesRegex(ValueError, 'under ignored .local/build'):
            PACK._private_output_path(outside)

        private_build = ROOT / '.local/build'
        private_build.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='z8-pack-test-', dir=private_build) as tmp:
            folder = Path(tmp)
            existing = folder / 'existing.json'
            existing.write_text('keep', encoding='ascii')
            with self.assertRaisesRegex(ValueError, 'overwrite'):
                PACK._private_output_path(existing)

            target = folder / 'target.json'
            target_link = folder / 'descriptor-link.json'
            target_link.symlink_to(target)
            with self.assertRaisesRegex(ValueError, 'symlink'):
                PACK._private_output_path(target_link)

            real_parent = folder / 'real-parent'
            real_parent.mkdir()
            parent_link = folder / 'parent-link'
            parent_link.symlink_to(real_parent, target_is_directory=True)
            with self.assertRaisesRegex(ValueError, 'symlink'):
                PACK._private_output_path(parent_link / 'descriptor.json')

    def test_exclusive_write_does_not_replace_racing_file(self):
        descriptor = synthetic_descriptor()
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp) / 'raced.json'
            target.write_text('preserve', encoding='ascii')
            with (mock.patch.object(PACK, '_private_output_path', return_value=target),
                  mock.patch.object(PACK, 'EXPECTED_DESCRIPTOR_SHA256',
                                    descriptor['descriptor_sha256'])):
                with self.assertRaisesRegex(ValueError, 'overwrite'):
                    PACK.write_private_descriptor(descriptor, target)
            self.assertEqual(target.read_text(encoding='ascii'), 'preserve')


if __name__ == '__main__':
    unittest.main()
