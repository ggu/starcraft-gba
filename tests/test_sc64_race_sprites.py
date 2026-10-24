import tempfile
import subprocess
import unittest
from pathlib import Path
import sys

from tools.sc64_race_sprites import (
    MISSION_RAW_TYPES, RACE_UNIT_IDS, T4_EXPECTED_VISIBLE_RAW_TYPES,
    T4_INVISIBLE_RAW_TYPES, T4_UNIT_IDS, asset_manifest_sha256, write_header,
    T4_BUILDING_IDS, T6_UNIT_IDS, LATE_UNIT_IDS, LATE_BUILDING_IDS,
    Z5_UNIT_IDS, Z5_BUILDING_IDS,
)


class RaceSpritePackTests(unittest.TestCase):
    def test_campaign_raw_types_have_real_asset_mappings(self):
        self.assertEqual(set(MISSION_RAW_TYPES) - set(RACE_UNIT_IDS.values()), set())
        self.assertEqual(RACE_UNIT_IDS['drone'], 41)
        self.assertEqual(RACE_UNIT_IDS['probe'], 64)
        self.assertEqual(RACE_UNIT_IDS['zealot'], 65)
        self.assertEqual(RACE_UNIT_IDS['hydralisk'], 38)
        self.assertEqual(RACE_UNIT_IDS['photon_cannon'], 162)

    def test_header_remaps_opaque_indices_and_keeps_transparency_zero(self):
        frame = {'pixels': bytes((0, 9, 0, 7)), 'alpha': bytes((0, 1, 0, 1)),
                 'width': 2, 'height': 2, 'x': 0, 'y': 0,
                 'frame_width': 2, 'frame_height': 2}
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sprites.h'
            remap = bytes((i + 32) & 255 for i in range(256))
            write_header(path, [{'name': 'test', 'frame': frame}], remap)
            text = path.read_text()
        self.assertIn('sc64_race_sprite_test_pixels[4]', text)
        self.assertIn('0x00,0x29,0x00,0x27', text)
        self.assertIn('{0,0,2,2,2,2,sc64_race_sprite_test_pixels}', text)

    def test_t4_assets_and_invisible_revealer_are_profile_gated(self):
        frame = {'pixels': bytes((0, 9, 0, 7)), 'alpha': bytes((0, 1, 0, 1)),
                 'width': 2, 'height': 2, 'x': 0, 'y': 0,
                 'frame_width': 2, 'frame_height': 2}
        t4 = [{'name': name, 'frame': frame} for name in T4_UNIT_IDS]
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sprites.h'
            write_header(path, [], bytes(range(256)), t4)
            text = path.read_text()
        self.assertIn('#ifdef SC64_T4_ENABLED', text)
        self.assertIn('case 15: return &sc64_race_sprite_civilian;', text)
        self.assertIn('case 20: return &sc64_race_sprite_raynor_raw20;', text)
        self.assertIn('case 89: return &sc64_race_sprite_rhynadon;', text)
        self.assertIn('case 95: return &sc64_race_sprite_ragnasaur;', text)
        self.assertIn('case 195: return &sc64_race_sprite_terran_beacon;', text)
        self.assertIn('case 218: return &sc64_race_sprite_data_disk;', text)
        self.assertNotIn('case 101: return &sc64_race_sprite_', text)
        self.assertIn('case 101: return 2; /* source-invisible map revealer */', text)
        self.assertIn('default: return sc64_race_unit_sprite(raw_type) ? 1 : 0;', text)
        self.assertEqual(T4_INVISIBLE_RAW_TYPES, (101,))
        resolved = set(RACE_UNIT_IDS.values()) | set(T4_UNIT_IDS.values())
        self.assertEqual(set(T4_EXPECTED_VISIBLE_RAW_TYPES) - resolved, set())

    def test_t4_prop_grps_retain_all_source_frames_without_state_guessing(self):
        def frame(pixel):
            return {'pixels': bytes((pixel,)), 'alpha': bytes((1,)), 'width': 1,
                    'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                    'frame_height': 1}
        props = [{'name': name, 'frames': [frame(9), frame(10)]}
                 for name in T4_BUILDING_IDS]
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sprites.h'
            write_header(path, [], bytes(range(256)),
                         [{'name': 'civilian', 'frame': frame(8)}], props)
            text = path.read_text()
        self.assertIn('#ifdef SC64_T4_ENABLED', text)
        self.assertIn('sc64_race_sprite_upper_door_frame_0_pixels[1]', text)
        self.assertIn('sc64_race_sprite_upper_door_frame_1_pixels[1]', text)
        self.assertIn('case 205:', text)
        self.assertIn('case 0: return &sc64_race_sprite_upper_door_frame_0;', text)
        self.assertIn('case 1: return &sc64_race_sprite_upper_door_frame_1;', text)
        self.assertIn('static inline const Sc64SpriteFrame *sc64_race_building_sprite', text)
        self.assertEqual(set(T4_BUILDING_IDS.values()), {203,205,206,207,208,209,211,212})
        # The packer exposes indexed frames and does not equate an index to a
        # semantic door state such as "closed" or "open".
        self.assertNotIn('door_is_open', text)
        self.assertNotIn('frame_index == 1 ? 1 : 0', text)

    def test_asset_manifest_hash_ignores_nonidentity_fields(self):
        asset = {'name': 'x', 'unit_id': 15, 'flingy_id': 1, 'sprite_id': 2,
                 'image_id': 3, 'grp_filename_index': 4, 'bolt_path': '00D/001',
                 'sha1': 'a' * 40, 'canvas': [1, 2], 'selected_frame': 0,
                 'frame_rect': [0, 0, 1, 2], 'opaque_pixels': 2,
                 'preview': 'one.png'}
        before = asset_manifest_sha256([asset])
        asset['preview'] = 'renamed.png'
        self.assertEqual(before, asset_manifest_sha256([asset]))

    def test_base_header_has_no_t4_cases_or_visibility_api(self):
        frame = {'pixels': bytes((0,)), 'alpha': bytes((1,)), 'width': 1,
                 'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                 'frame_height': 1}
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sprites.h'
            write_header(path, [{'name': 'marine', 'frame': frame}], bytes(range(256)))
            text = path.read_text()
        self.assertNotIn('SC64_T4_ENABLED', text)
        self.assertNotIn('sc64_race_unit_sprite_mode', text)
        self.assertNotIn('case 218:', text)
        self.assertNotIn('SC64_Z5_ACTORS', text)

    def test_crashed_norad_lookup_is_available_only_in_t6_profile(self):
        frame = {'pixels': bytes((9,)), 'alpha': bytes((1,)), 'width': 1,
                 'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                 'frame_height': 1}
        selected = [{'name': name, 'frame': frame} for name in RACE_UNIT_IDS]
        t6 = [{'name': name, 'frame': frame} for name in T6_UNIT_IDS]
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            write_header(root / 'sprites.h', selected, bytes(range(256)),
                         t6_selected=t6)
            (root / 'check.c').write_text('''#include <assert.h>
#include "sprites.h"
int main(void) {
    const Sc64SpriteFrame *ship = sc64_race_unit_sprite(126);
#ifdef SC64_T6_ENABLED
#ifndef SC64_T6_SPRITE_SET_COMPLETE
#error "complete T6 preparation must advertise its sprite set"
#endif
    assert(ship && ship->w == 1 && ship->pixels[0] == 9);
    assert(sc64_race_unit_sprite(19));
    assert(sc64_race_unit_sprite(115));
#else
    assert(!ship);
#endif
    assert(sc64_race_unit_sprite(138)); /* shared Queen's Nest art */
    assert(!sc64_race_unit_sprite(29)); /* flying Norad hero is distinct */
    return 0;
}
''')
            for enabled in (False, True):
                command = ['clang', '-std=c11', str(root / 'check.c'),
                           '-o', str(root / 'check')]
                if enabled:
                    command.append('-DSC64_T6_ENABLED')
                subprocess.run(command, check=True, capture_output=True)
                subprocess.run([str(root / 'check')], check=True, capture_output=True)

    def test_late_terran_sprite_set_is_complete_and_profile_gated(self):
        frame = {'pixels': bytes((9,)), 'alpha': bytes((1,)), 'width': 1,
                 'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                 'frame_height': 1}
        late_units = [{'name': name, 'frame': frame} for name in LATE_UNIT_IDS]
        late_buildings = [{'name': name, 'frames': [frame, frame]}
                          for name in LATE_BUILDING_IDS]
        base_units = [{'name': name, 'frame': frame} for name in RACE_UNIT_IDS]
        t4_units = [{'name': name, 'frame': frame} for name in T4_UNIT_IDS]
        t4_buildings = [{'name': name, 'frames': [frame, frame]}
                         for name in T4_BUILDING_IDS]
        t6_units = [{'name': name, 'frame': frame}
                    for name in ('crashed_norad_ii', 'raynor_vulture_t6',
                                 'control_tower_t6')]
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            full = root / 'late.h'
            write_header(full, base_units, bytes(range(256)), t4_units,
                         t4_buildings, t6_units, late_units, late_buildings)
            text = full.read_text()
            (root / 'check.c').write_text('''#include <assert.h>
#include "late.h"
int main(void) {
#ifndef SC64_TERRAN_LATE_SPRITE_SET_COMPLETE
#error "late sprite preparation marker missing"
#endif
    assert(sc64_race_unit_sprite(29));
    assert(sc64_race_unit_sprite(217));
    assert(sc64_race_building_sprite(108, 1));
    assert(sc64_race_building_sprite(127, 0));
    assert(sc64_race_building_sprite(172, 1));
    assert(!sc64_race_building_sprite(108, 2));
    return 0;
}
''')
            subprocess.run(['clang', '-std=c11', '-DSC64_T4_ENABLED',
                            '-DSC64_T6_ENABLED', '-DSC64_TERRAN_LATE_ENABLED',
                            str(root / 'check.c'), '-o', str(root / 'check')],
                           check=True, capture_output=True)
            subprocess.run([str(root / 'check')], check=True, capture_output=True)
            self.assertIn('#ifdef SC64_TERRAN_LATE_ENABLED', text)
            self.assertIn('case 29: return &sc64_race_sprite_norad_ii;', text)
            self.assertIn('case 217: return &sc64_race_sprite_psi_emitter;', text)
            self.assertIn('case 127:', text)
            self.assertIn('case 172:', text)
            partial = root / 'partial.h'
            write_header(partial, base_units, bytes(range(256)), t4_units,
                         t4_buildings, t6_units, late_units[:1], late_buildings)
            self.assertNotIn('SC64_TERRAN_LATE_SPRITE_SET_COMPLETE',
                             partial.read_text())

    def test_z5_sprite_set_has_independent_gate_and_complete_marker(self):
        frame = {'pixels': bytes((9,)), 'alpha': bytes((1,)), 'width': 1,
                 'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                 'frame_height': 1}
        base_units = [{'name': name, 'frame': frame} for name in RACE_UNIT_IDS]
        z5_units = [{'name': name, 'frame': frame} for name in Z5_UNIT_IDS]
        z5_buildings = [{'name': name, 'frames': [frame, frame]}
                        for name in Z5_BUILDING_IDS]
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            full = root / 'z5.h'
            write_header(full, base_units, bytes(range(256)),
                         z5_selected=z5_units,
                         z5_building_selected=z5_buildings)
            text = full.read_text()
            (root / 'check.c').write_text('''#include <assert.h>
#include "z5.h"
int main(void) {
#ifdef SC64_Z5_ACTORS
#ifndef SC64_Z5_SPRITE_SET_COMPLETE
#error "complete Z5 set must advertise its own completion marker"
#endif
    assert(sc64_z5_actor_sprite(90));
    assert(sc64_z5_actor_sprite(218));
    assert(sc64_race_building_sprite(195, 1));
    assert(!sc64_race_building_sprite(195, 2));
#endif
    return 0;
}
''')
            for enabled in (False, True):
                command = ['clang', '-std=c11', str(root / 'check.c'),
                           '-o', str(root / 'check')]
                if enabled:
                    command.append('-DSC64_Z5_ACTORS')
                subprocess.run(command, check=True, capture_output=True)
                subprocess.run([str(root / 'check')], check=True, capture_output=True)
            self.assertIn('#ifdef SC64_Z5_ACTORS', text)
            self.assertIn('#define SC64_Z5_SPRITE_SET_COMPLETE 1', text)
            self.assertIn('sc64_z5_actor_sprite(unsigned short raw_type)', text)
            self.assertIn('case 90: return &sc64_race_sprite_bengalaas_z5;', text)
            self.assertIn('case 195:', text)
            partial = root / 'partial.h'
            write_header(partial, base_units, bytes(range(256)),
                         z5_selected=z5_units[:-1],
                         z5_building_selected=z5_buildings)
            self.assertNotIn('SC64_Z5_SPRITE_SET_COMPLETE', partial.read_text())
            partial_building = root / 'partial-building.h'
            write_header(partial_building, base_units, bytes(range(256)),
                         z5_selected=z5_units,
                         z5_building_selected=z5_buildings[:-1])
            self.assertNotIn('SC64_Z5_SPRITE_SET_COMPLETE',
                             partial_building.read_text())

            zp_units = [{'name': name, 'frame': frame} for name in
                        ('infested_kerrigan', 'raynor_vulture_zp', 'tassadar_hero_zp')]
            zp_buildings = [{'name': name, 'frames': [frame]}
                            for name in ('control_tower_zp', 'machine_shop_zp',
                                         'armory_zp', 'queens_nest_zp',
                                         'ultralisk_cavern_zp', 'protoss_beacon_zp')]
            both = root / 'both.h'
            write_header(both, base_units, bytes(range(256)),
                         zp_selected=zp_units, zp_building_selected=zp_buildings,
                         z5_selected=z5_units, z5_building_selected=z5_buildings)
            both_text = both.read_text()
            self.assertIn('SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE', both_text)
            self.assertIn('SC64_Z5_SPRITE_SET_COMPLETE', both_text)
        help_result = subprocess.run(
            [sys.executable, str(Path(__file__).parents[1] / 'tools/sc64_race_sprites.py'), '--help'],
            check=True, capture_output=True, text=True)
        self.assertIn('--include-z5-actors', help_result.stdout)
        self.assertIn('--include-zp-campaign-actors', help_result.stdout)



if __name__ == '__main__':
    unittest.main()
