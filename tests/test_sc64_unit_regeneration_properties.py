"""Source property extraction and compact flags, independent of game assets."""
import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest

from tools.sc64_unit_spec_reference import unit_flag_properties
from tools.sc64_stock_profiles_generate import emit
from tools.test_sc64_stock_profiles import check


class RegenerationProperties(unittest.TestCase):
    def test_source_flags_use_exact_tokens_and_preserve_building(self):
        source = '''static const unsigned unitFlags[5] = {
            0, RegeneratesHP | OrganicUnit,
            Building | RegeneratesHP, Building,
            FlyingBuilding /* RegeneratesHP */ | RegeneratesHPExtra
        };'''
        self.assertEqual(unit_flag_properties(source, 5), [
            {'is_building': False, 'regenerates_hp': False},
            {'is_building': False, 'regenerates_hp': True},
            {'is_building': True, 'regenerates_hp': True},
            {'is_building': True, 'regenerates_hp': False},
            {'is_building': False, 'regenerates_hp': False},
        ])
        with self.assertRaisesRegex(ValueError, 'expected 6 rows'):
            unit_flag_properties(source, 6)

    def test_old_facts_cannot_silently_clear_regeneration(self):
        with tempfile.TemporaryDirectory() as temp:
            source, output = Path(temp) / 'facts.json', Path(temp) / 'out.h'
            source.write_text(json.dumps({'units': [{'is_building': False}], 'weapons': []}))
            with self.assertRaisesRegex(ValueError, 're-extract the pinned reference'):
                emit(source, output)
            self.assertFalse(output.exists())
            # The table verifier must reject stale metadata before spending
            # time compiling its C probes.
            source.write_text(json.dumps({'units': [{'id': 0, 'is_building': False}],
                                          'weapons': []}))
            with self.assertRaisesRegex(ValueError, 're-extract the pinned reference'):
                check(Path(temp), source)

    def test_emit_all_four_property_combinations(self):
        units = []
        for ident, (building, regen) in enumerate(((False, False), (True, False),
                                                   (False, True), (True, True))):
            units.append(dict(id=ident, name='fixture', is_building=building,
                regenerates_hp=regen, top_speed=0, ground_weapon={'id': 130},
                air_weapon={'id': 130}, ground_weapon_hits=0, air_weapon_hits=0,
                size={'id': 0}, armor=0, mineral_cost=0, gas_cost=0, build_time=1,
                supply_required=0, supply_provided=0, hp=1, shields=0))
        with tempfile.TemporaryDirectory() as temp:
            source, output = Path(temp) / 'facts.json', Path(temp) / 'out.h'
            source.write_text(json.dumps({'units': units, 'weapons': []}))
            with contextlib.redirect_stdout(io.StringIO()):
                emit(source, output)
            text = output.read_text()
            for ident, flag in enumerate((0, 1, 2, 3)):
                self.assertIn(f'[{ident}] = {{ {flag}, 130, 130,', text)


if __name__ == '__main__':
    unittest.main()
