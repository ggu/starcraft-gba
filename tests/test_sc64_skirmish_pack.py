"""Synthetic, asset-free checks for bounded skirmish map packing helpers."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('sc64_skirmish_pack', ROOT / 'tools/sc64_skirmish_pack.py')
PACKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKER)


class SkirmishPackTests(unittest.TestCase):
    def test_flood_fill_rejects_disconnected_spawn_areas(self):
        width = height = 8
        blocked = [0] * (width * height)
        for y in range(height):
            blocked[4 + y * width] = 1
        component = PACKER.connected_component(blocked, width, height, 1 + width)
        self.assertNotIn(6 + 6 * width, component)

    def test_cover_bit_does_not_block_start_or_connectivity(self):
        width = height = 16
        starts = [(32, 32), (208, 32), (32, 208), (208, 208)]
        terrain = [2] * (width * height)
        self.assertEqual(len(PACKER.connected_component(terrain, width, height, 0)), 256)
        self.assertEqual(PACKER.select_starts(terrain, width, height, starts),
                         PACKER.select_starts([0] * (width * height), width, height, starts))
        terrain[1 + 2 * width] = 3
        with self.assertRaisesRegex(ValueError, 'clear 3x3'):
            PACKER.select_starts(terrain, width, height, starts)

    def test_start_selection_requires_clear_spawn_and_shared_access(self):
        width = height = 16
        blocked = [0] * (width * height)
        starts = [(32, 32), (208, 32), (32, 208), (208, 208), (128, 128)]
        selected = PACKER.select_starts(blocked, width, height, starts)
        self.assertEqual(len(selected), 4)
        self.assertEqual(selected[0], (32, 32))
        blocked[1 + 2 * width] = 1
        with self.assertRaisesRegex(ValueError, 'clear 3x3'):
            PACKER.select_starts(blocked, width, height, starts[:4])

    def test_isolated_walkable_regions_are_rejected(self):
        width = height = 16
        blocked = [0] * (width * height)
        for y in range(height):
            blocked[8 + y * width] = 1
        starts = [(32, 32), (208, 32), (32, 208), (208, 208)]
        with self.assertRaisesRegex(ValueError, 'not mutually connected'):
            PACKER.select_starts(blocked, width, height, starts)

    def test_minerals_are_bounded_and_report_omissions(self):
        starts = [(0, 0), (300, 0), (0, 300), (300, 300)]
        resources = [{'x_source_px': x * 2, 'y_source_px': y * 2,
                      'resources': 1500, 'source_type': 176}
                     for y in range(0, 400, 40) for x in range(0, 400, 40)]
        chosen, report = PACKER.choose_minerals(resources, starts)
        self.assertLessEqual(len(chosen), 32)
        self.assertEqual(report['packed_count'], len(chosen))
        self.assertEqual(report['omitted_count'], len(resources) - len(chosen))
        self.assertEqual(report['new_patches_selected_per_start'], [8, 8, 8, 8])
        self.assertEqual(len({(p['x'], p['y']) for p in chosen}), len(chosen))

    def test_sparse_minerals_are_fairly_distributed_across_starts(self):
        starts = [(0, 0), (300, 0), (0, 300), (300, 300)]
        resources = [{'x_source_px': x * 2, 'y_source_px': y * 2,
                      'resources': 1500, 'source_type': 176}
                     for y in range(0, 200, 40) for x in range(0, 200, 40)]
        chosen, report = PACKER.choose_minerals(resources, starts)
        self.assertEqual(len(chosen), 25)
        self.assertEqual(report['new_patches_selected_per_start'], [7, 6, 6, 6])
        self.assertEqual(report['omitted_count'], 0)

    def test_too_few_start_markers_fail_clearly(self):
        with self.assertRaisesRegex(ValueError, 'need 4'):
            PACKER.select_starts([0] * 64, 8, 8, [(32, 32)] * 3)

    def test_map_dimensions_are_limited_to_supported_64_or_96(self):
        PACKER.validate_dimensions('synthetic', 64, 96)
        with self.assertRaisesRegex(ValueError, 'outside the supported 64/96-cell'):
            PACKER.validate_dimensions('synthetic', 128, 128)
        with self.assertRaisesRegex(ValueError, 'outside the supported 64/96-cell'):
            PACKER.validate_dimensions('synthetic', 80, 64)


if __name__ == '__main__':
    unittest.main()
