# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic fail-closed tests for the Z9/Z10 descriptor and table tools."""
from __future__ import annotations

import copy
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))

import sc64_z8_pack  # noqa: E402
import sc64_zerg_late_pack as late  # noqa: E402
import sc64_zerg_late_program_pack as program  # noqa: E402


def synthetic_descriptor(mission: str = 'Z9') -> dict:
    """A shape-correct descriptor with authored rows, not native source data."""
    spec = late.CONTRACTS[mission]
    counts = spec['counts']
    triggers, actions, conditions = [], [], []
    n = counts['trigger_records']
    action_split = [counts['actions'] // n] * n
    action_split[0] += counts['actions'] - sum(action_split)
    condition_split = [counts['conditions'] // n] * n
    condition_split[0] += counts['conditions'] - sum(condition_split)
    for index in range(n):
        first_action, first_condition = len(actions), len(conditions)
        for row in range(action_split[index]):
            actions.append({'trigger': index, 'index': row, 'location': 0, 'opcode': 1,
                            'string_id': 0, 'wav_id': 0, 'time': 0, 'group1': 0,
                            'group2': 0, 'unit': 0, 'modifier': 0, 'flags': 0})
        for row in range(condition_split[index]):
            conditions.append({'trigger': index, 'index': row, 'location': 0, 'opcode': 2,
                               'player': 13, 'amount': 0, 'unit': 229, 'mask': 0,
                               'comparison': 1, 'resource': 0, 'flags': 16})
        enabled = [0] * 27
        enabled[18] = 1
        triggers.append({'index': index, 'first_action': first_action,
                         'action_count': action_split[index],
                         'first_condition': first_condition,
                         'condition_count': condition_split[index],
                         'execution_flags': 0, 'initial_cursor': 0,
                         'enabled_players': enabled})
    units = [{
        'record_index': i, 'serial': 1000 + i, 'x_source_px': 0, 'y_source_px': 0,
        'source_type': 37, 'relation': 0, 'special': 0, 'valid': 0, 'owner_raw': 0,
        'hp_percent': 0, 'shield_percent': 0, 'energy_percent': 0, 'resources': 0,
        'hangar': 0, 'flags': 0, 'unused_hex': '00000000', 'linked_serial': 0,
        'linked_record_index': None,
    } for i in range(counts['unit_records'])]
    width, height = spec['dimensions']
    descriptor = {
        'schema': late.SCHEMA, 'mission': mission,
        'global_catalog_id': spec['global_id'], 'source_map_path': spec['map_path'],
        'provenance': {}, 'dimensions': {'width': width, 'height': height},
        'counts': late._descriptor_counts(spec),
        'triggers': triggers, 'conditions': conditions, 'actions': actions,
        'locations': [{'index': i + 1, 'left': 0, 'top': 0, 'right': 32, 'bottom': 32,
                       'flags': 0} for i in range(counts['locations'])],
        'owners': [3, 0, 5, 6, 5, 0, 7, 5, 0, 0, 0, 0], 'sides': [0] * 12,
        'forces': [3, 3, 3, 0, 2, 3, 3, 1] + [0] * 12,
        'properties': [[0] * 20 for _ in range(64)], 'property_used': [0] * 64,
        'puni': [0] * 5700, 'unit_records': units, 'ai_script_fourccs': spec['ai_fourccs'],
    }
    descriptor['descriptor_sha256'] = sc64_z8_pack._canonical_fingerprint(descriptor)
    return descriptor


def refingerprint(descriptor: dict) -> dict:
    descriptor['descriptor_sha256'] = sc64_z8_pack._canonical_fingerprint(descriptor)
    return descriptor


class LatePackStructureTest(unittest.TestCase):
    def test_synthetic_descriptors_are_structurally_valid(self):
        for mission in late.CONTRACTS:
            late.validate_structure(synthetic_descriptor(mission))

    def test_rejects_unknown_mission_and_wrong_dimensions(self):
        d = synthetic_descriptor('Z9')
        d['mission'] = 'Z8'
        with self.assertRaises(ValueError):
            late.validate_structure(refingerprint(d))
        d = synthetic_descriptor('Z10')
        d['dimensions'] = {'width': 128, 'height': 128}
        with self.assertRaises(ValueError):
            late.validate_structure(refingerprint(d))

    def test_rejects_broken_rows(self):
        for mutate in (
            lambda d: d['triggers'][1].update(first_action=0),
            lambda d: d['actions'][0].update(opcode=0),
            lambda d: d['conditions'][0].update(location=65),
            lambda d: d['locations'][0].update(right=5000),
            lambda d: d['unit_records'][3].update(linked_serial=1001),
            lambda d: d['unit_records'][3].update(serial=1000),
        ):
            d = synthetic_descriptor('Z9')
            mutate(d)
            with self.assertRaises(ValueError):
                late.validate_structure(refingerprint(d))

    def test_rejects_stale_fingerprint(self):
        d = synthetic_descriptor('Z9')
        d['owners'][0] = 5
        with self.assertRaises(ValueError):
            late.validate_structure(d)

    def test_unpinned_descriptor_is_not_authenticated(self):
        d = synthetic_descriptor('Z10')
        with self.assertRaises(ValueError):
            late.require_authenticated(d)
        with self.assertRaises(ValueError):
            program.render(d)

    def test_private_output_is_mission_scoped(self):
        with self.assertRaises(ValueError):
            late._private_output_path('Z9', ROOT / '.local/build/z10-source/d.json')
        with self.assertRaises(ValueError):
            late._private_output_path('Z10', Path(tempfile.gettempdir()) / 'z10-x/d.json')


class LateProgramPackTest(unittest.TestCase):
    def render(self, d):
        spec = copy.deepcopy(late.CONTRACTS)
        spec[d['mission']]['descriptor_sha256'] = d['descriptor_sha256']
        with mock.patch.object(late, 'CONTRACTS', spec):
            return program.render(d)

    def test_renders_mission_prefixed_tables(self):
        text = self.render(synthetic_descriptor('Z9'))
        self.assertIn('#define SC64_Z9_PROGRAM_TRIGGER_COUNT 14u', text)
        self.assertIn('#define SC64_Z9_PROGRAM_ACTIVE_MASK 156u', text)
        self.assertIn('static const u8 sc64_z9_program_forces[8] = {4u,4u,4u,1u,3u,4u,4u,2u};', text)
        self.assertIn('Do not commit or distribute', text)
        text = self.render(synthetic_descriptor('Z10'))
        self.assertIn('#define SC64_Z10_PROGRAM_MAP_PIXEL_WIDTH 3072u', text)
        self.assertIn('sc64_z10_program_permissions', text)


if __name__ == '__main__':
    unittest.main()
