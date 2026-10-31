import tempfile
import subprocess
import re
import unittest
from pathlib import Path
import sys

from tools.sc64_race_sprites import (
    MISSION_RAW_TYPES, RACE_UNIT_IDS, T4_EXPECTED_VISIBLE_RAW_TYPES,
    T4_INVISIBLE_RAW_TYPES, T4_UNIT_IDS, asset_manifest_sha256,
    named_asset_manifest_sha256, write_header,
    T4_BUILDING_IDS, T6_UNIT_IDS, LATE_UNIT_IDS, LATE_BUILDING_IDS,
    ZP_UNIT_IDS, ZP_BUILDING_IDS, Z5_UNIT_IDS, Z5_BUILDING_IDS, Z6_UNIT_IDS,
    Z5_DOODAD_IDS, Z6_BUILDING_IDS,
    P7_UNIT_IDS, P7_BUILDING_IDS, P7_SHARED_UNIT_IDS, Z8_BUILDING_IDS,
    P6_UNIT_IDS, P6_DOODAD_IDS,
    image_init_frame,
    compose_init_layers, partition_init_layers,
    animation_frame_timeline,
)


class RaceSpritePackTests(unittest.TestCase):
    def test_import_roster_covers_supported_modes_and_buildings(self):
        self.assertTrue(
            {13, 16, 30, 34}.issubset(set(RACE_UNIT_IDS.values())),
            'Spider Mine, Kerrigan, Siege Mode, and Medic need source art')
        self.assertTrue(
            {110, 113}.issubset(set(RACE_UNIT_IDS.values())),
            'Refinery and Factory need source art')

    def test_z8_nydus_source_frames_are_isolated_and_bound_by_raw_id(self):
        def frame(pixel):
            return {'pixels': bytes((pixel,)), 'alpha': bytes((1,)), 'width': 1,
                    'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                    'frame_height': 1}
        self.assertEqual(Z8_BUILDING_IDS, {'nydus_canal_z8': 134})
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            legacy, disabled, enabled = (root / 'legacy.h', root / 'disabled.h', root / 'enabled.h')
            base_units = [{'name': name, 'frame': frame(3)} for name in RACE_UNIT_IDS]
            write_header(legacy, base_units, bytes(range(256)))
            write_header(disabled, base_units, bytes(range(256)), z8_building_selected=None)
            self.assertEqual(legacy.read_bytes(), disabled.read_bytes())
            write_header(enabled, base_units, bytes(range(256)),
                         z8_building_selected=[{'name': 'nydus_canal_z8',
                                                'frames': [frame(11+i) for i in range(5)]}])
            (root / 'check.c').write_text('''#include <assert.h>
#include "enabled.h"
int main(void) {
#ifdef SC64_Z8_RUNTIME
    assert(SC64_Z8_BUILDING_SPRITE_SET_COMPLETE == 1);
    for (unsigned i = 0; i < 5; ++i) {
        const Sc64SpriteFrame *s = sc64_race_building_sprite(134, i);
        assert(s && s->pixels[0] == 11 + i);
    }
    assert(!sc64_race_building_sprite(134, 5));
#else
    assert(!sc64_race_building_sprite(134, 0));
#endif
    return 0;
}
''')
            for flags in (['-DSC64_RACES'], ['-DSC64_RACES', '-DSC64_Z8_RUNTIME']):
                subprocess.run(['clang', '-std=c11', '-Werror', *flags,
                                str(root / 'check.c'), '-o', str(root / 'check')],
                               check=True, capture_output=True)
                subprocess.run([str(root / 'check')], check=True, capture_output=True)

    def test_factory_is_emitted_in_ordinary_sprite_lookup(self):
        frame = {'pixels': bytes((9,)), 'alpha': bytes((1,)), 'width': 1,
                 'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                 'frame_height': 1}
        with tempfile.TemporaryDirectory() as temp:
            header = Path(temp) / 'sprites.h'
            write_header(header, [{'name': name, 'frame': frame}
                                  for name in RACE_UNIT_IDS], bytes(range(256)))
            text = header.read_text()
            self.assertIn('sc64_race_sprite_factory_pixels', text)
            self.assertIn('case 113: return &sc64_race_sprite_factory;', text)

    def test_shadow_layer_is_deferred_without_expanding_body(self):
        body = {'pixels': bytes([7]), 'alpha': bytes([1]), 'width': 1, 'height': 1,
                'x': 0, 'y': 0, 'frame_width': 1, 'frame_height': 1}
        modifiers = bytes([0, 10, 1])
        specs = [(1, 0, 42, True), (2, 1, 0, False)]
        opaque, deferred = partition_init_layers(specs, modifiers)
        self.assertEqual(opaque, [(2, 1, 0, False)])
        self.assertEqual(deferred, [{'image_id': 1, 'modifier': 10,
                                     'x': 0, 'y': 42, 'underlay': True}])
        body_only, _ = partition_init_layers(specs[:1], modifiers)
        result = compose_init_layers(body, [(body, x, y, under)
                                            for _, x, y, under in body_only]) if body_only else body
        self.assertEqual((result['width'], result['height'], result['x'], result['y'],
                          result['frame_width'], result['frame_height'], result['pixels']),
                         (1, 1, 0, 0, 1, 1, bytes([7])))
        overlay = dict(body, pixels=bytes([9]))
        result = compose_init_layers(body, [(overlay, x, y, under)
                                            for _, x, y, under in opaque])
        self.assertEqual((result['width'], result['height']), (3, 1))
        self.assertEqual(result['pixels'], bytes([0, 7, 9]))
        with self.assertRaises(ValueError):
            partition_init_layers([(3, 0, 0, True)], modifiers)

    def test_unit_init_frame_comes_from_authored_iscript_playfram(self):
        # SCPE record with Init at byte 16: playfram 2, then end.
        payload = bytearray(32)
        payload[16:20] = b'SCPE'
        payload[20:24] = (0).to_bytes(4, 'little')
        payload[24:26] = (28).to_bytes(2, 'little')
        payload[26:28] = (0).to_bytes(2, 'little')
        payload[28:32] = bytes((0, 2, 0, 22))
        self.assertEqual(image_init_frame(3, (0, 0, 0, 42), bytes(payload), {42: 16}), 2)

    def test_photon_attack_timeline_tracks_wait_plus_one_source_updates(self):
        payload = bytearray(56)
        payload[:33] = bytes((0,2,0, 5,2, 0,1,0, 5,2, 0,3,0, 5,2,
                              5,1, 38, 42, 48, 0,1,0, 5,2, 0,2,0,
                              5,2, 7,40,0))
        payload[40:45] = bytes((0,0,0, 7,45))
        payload[45] = 0  # The idle target is used only as a jump boundary.
        payload[48:56] = bytes((5,1, 38,42,48, 0,1,0,))[:8]
        payload[53:56] = bytes((5,2, 0,))
        # Replace the compact repeat tail with explicit instructions and goto.
        payload[48:59] = bytes((5,1, 38,42,48, 0,1,0, 5,2, 0))
        payload[59:64] = bytes((2,0, 5,2, 7))
        payload[64:66] = bytes((40,0))
        self.assertEqual(animation_frame_timeline(bytes(payload), 0, 45), [
            {'frame': 2, 'duration': 3}, {'frame': 1, 'duration': 3},
            {'frame': 3, 'duration': 5}, {'frame': 1, 'duration': 3},
            {'frame': 2, 'duration': 3}, {'frame': 0, 'duration': 0},
        ])
        self.assertEqual(animation_frame_timeline(bytes(payload), 48, 45), [
            {'frame': 0, 'duration': 2}, {'frame': 1, 'duration': 3},
            {'frame': 2, 'duration': 3}, {'frame': 0, 'duration': 0},
        ])

    def test_source_init_layers_compose_under_and_over_primary_pixels(self):
        def frame(pixels, alpha, width=2, height=2):
            return {'pixels': bytes(pixels), 'alpha': bytes(alpha),
                    'width': width, 'height': height, 'x': 0, 'y': 0,
                    'frame_width': width, 'frame_height': height}
        main = frame((1, 0, 0, 0), (1, 0, 0, 0))
        underlay = frame((2, 2, 2, 2), (1, 1, 1, 1))
        overlay = frame((3, 0, 0, 0), (1, 0, 0, 0))
        composed = compose_init_layers(main, [(underlay, 0, 0, True),
                                                (overlay, 0, 0, False)])
        self.assertEqual(composed['pixels'], bytes((3, 2, 2, 2)))
        self.assertEqual(composed['alpha'], bytes((1, 1, 1, 1)))

    def test_photon_cannon_frame_bank_is_available_to_race_renderer(self):
        def frame(pixel):
            return {'pixels': bytes((pixel,)), 'alpha': bytes((1,)), 'width': 1,
                    'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                    'frame_height': 1}
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root / 'sprites.h'
            base_units = [{'name': name, 'frame': frame(20)} for name in RACE_UNIT_IDS]
            write_header(path, base_units, bytes(range(256)),
                         race_building_selected=[{'name': 'photon_cannon',
                                                  'frames': [frame(21), frame(22)],
                                                  'animations': {
                                                      'gnd_attack_init': [
                                                          {'frame': 2, 'duration': 3},
                                                          {'frame': 1, 'duration': 3},
                                                          {'frame': 3, 'duration': 5},
                                                          {'frame': 1, 'duration': 3},
                                                          {'frame': 2, 'duration': 3},
                                                          {'frame': 0, 'duration': 0}],
                                                      'gnd_attack_repeat': [
                                                          {'frame': 0, 'duration': 2},
                                                          {'frame': 1, 'duration': 3},
                                                          {'frame': 2, 'duration': 3},
                                                          {'frame': 0, 'duration': 0}]}}])
            (root / 'check.c').write_text('''#include <assert.h>
#include "sprites.h"
int main(void) {
    assert(sc64_race_building_sprite(162, 0));
    assert(sc64_race_building_sprite(162, 1));
    assert(!sc64_race_building_sprite(162, 2));
    assert(sc64_race_building_sprite(162, 1)->pixels[0] == 22);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(0) == 2);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(2) == 2);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(3) == 1);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(5) == 1);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(6) == 3);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(10) == 3);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(11) == 1);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(14) == 2);
    assert(sc64_race_photon_cannon_gnd_attack_init_frame(17) == 0);
    assert(sc64_race_photon_cannon_gnd_attack_repeat_frame(0) == 0);
    assert(sc64_race_photon_cannon_gnd_attack_repeat_frame(2) == 1);
    assert(sc64_race_photon_cannon_gnd_attack_repeat_frame(5) == 2);
    assert(sc64_race_photon_cannon_gnd_attack_repeat_frame(8) == 0);
    assert(SC64_PHOTON_SOURCE_FRAMES == 18u);
    assert(sc64_photon_cannon_attack_frame(16) == 2);
    assert(sc64_photon_cannon_attack_frame(17) == 0);
    assert(sc64_photon_cannon_attack_frame(400) == 0);
    return 0;
}
''')
            subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-DSC64_RACES', str(root / 'check.c'), '-o', str(root / 'check')],
                           check=True, capture_output=True)
            subprocess.run([str(root / 'check')], check=True, capture_output=True)

    def test_p7_sprite_set_gates_arbiter_and_all_structure_frames(self):
        def frame(pixel):
            return {'pixels': bytes((pixel,)), 'alpha': bytes((1,)), 'width': 1,
                    'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                    'frame_height': 1}
        base_units = [{'name': name, 'frame': frame(9)} for name in RACE_UNIT_IDS]
        p7_units = [{'name': name, 'frame': frame(20)} for name in P7_UNIT_IDS]
        p7_buildings = [{'name': name, 'frames': [frame(21), frame(22)]}
                        for name in P7_BUILDING_IDS]
        late_buildings = [{'name': name, 'frames': [frame(31), frame(32)]}
                          for name in LATE_BUILDING_IDS]
        z6_buildings = [{'name': name, 'frames': [frame(41), frame(42)]}
                        for name in Z6_BUILDING_IDS]
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root / 'p7.h'
            write_header(path, base_units, bytes(range(256)),
                         late_building_selected=late_buildings,
                         z6_building_selected=z6_buildings,
                         p7_selected=p7_units,
                         p7_building_selected=p7_buildings)
            text = path.read_text()
            self.assertIn('#define SC64_P7_SPRITE_SET_COMPLETE 1', text)
            self.assertIn('case 71: return &sc64_race_sprite_arbiter_p7;', text)
            self.assertEqual(P7_BUILDING_IDS['robotics_support_bay_p7'], 171)
            self.assertNotIn('robotics_facility_p7', P7_BUILDING_IDS)
            for raw in (68, 71, 72, 84):
                self.assertIn(f'case {raw}:', text)
            for raw in P7_BUILDING_IDS.values():
                self.assertIn(f'case {raw}:', text)
            self.assertEqual(set(P7_SHARED_UNIT_IDS.values()), {68, 72, 84})
            (root / 'check.c').write_text('''#include <assert.h>
#include "p7.h"
int main(void) {
#ifdef SC64_P7_ACTORS
#ifndef SC64_P7_SPRITE_SET_COMPLETE
#error "complete P7 sprite set must advertise its own marker"
#endif
    assert(sc64_race_unit_sprite(68));
    assert(sc64_race_unit_sprite(71));
    assert(sc64_race_unit_sprite(72));
    assert(sc64_race_unit_sprite(84));
    assert(sc64_race_building_sprite(159, 1));
    assert(sc64_race_building_sprite(163, 1));
    assert(sc64_race_building_sprite(165, 1));
    assert(sc64_race_building_sprite(169, 1));
    assert(sc64_race_building_sprite(170, 1));
    assert(sc64_race_building_sprite(171, 1));
    assert(sc64_race_building_sprite(172, 1));
    assert(sc64_race_unit_sprite(71)->pixels[0] == 20);
    assert(sc64_race_building_sprite(171, 1)->pixels[0] == 22);
    assert(sc64_race_building_sprite(172, 1)->pixels[0] == 22);
    assert(!sc64_race_building_sprite(171, 2));
#else
    assert(!sc64_race_unit_sprite(71));
#endif
    return 0;
}
''')
            for defines in ([], ['-DSC64_P7_ACTORS'],
                            ['-DSC64_P7_ACTORS', '-DSC64_TERRAN_LATE_ENABLED',
                             '-DSC64_Z6_RUNTIME']):
                subprocess.run(['clang', '-std=c11', '-Werror', str(root / 'check.c'),
                                '-o', str(root / 'check'), *defines],
                               check=True, capture_output=True)
                subprocess.run([str(root / 'check')], check=True, capture_output=True)
            partial = root / 'partial.h'
            write_header(partial, base_units, bytes(range(256)),
                         p7_selected=p7_units,
                         p7_building_selected=p7_buildings[:-1])
            self.assertNotIn('SC64_P7_SPRITE_SET_COMPLETE', partial.read_text())
            missing_shared = root / 'missing-shared.h'
            no_shared_units = [item for item in base_units
                               if item['name'] not in P7_SHARED_UNIT_IDS]
            write_header(missing_shared, no_shared_units, bytes(range(256)),
                         p7_selected=p7_units,
                         p7_building_selected=p7_buildings)
            self.assertNotIn('SC64_P7_SPRITE_SET_COMPLETE',
                             missing_shared.read_text())
            no_p7 = root / 'no-p7.h'
            write_header(no_p7, base_units, bytes(range(256)),
                         late_building_selected=late_buildings,
                         z6_building_selected=z6_buildings)
            no_p7_text = no_p7.read_text()
            self.assertNotIn('!SC64_P7_ACTORS', no_p7_text)
            self.assertNotIn('SC64_P7_ACTORS', no_p7_text)
            legacy = root / 'legacy.h'
            explicit_off = root / 'explicit-off.h'
            write_header(legacy, base_units, bytes(range(256)))
            write_header(explicit_off, base_units, bytes(range(256)),
                         p7_selected=[], p7_building_selected=[])
            self.assertEqual(legacy.read_bytes(), explicit_off.read_bytes())
        help_result = subprocess.run(
            [sys.executable, str(Path(__file__).parents[1] / 'tools/sc64_race_sprites.py'), '--help'],
            check=True, capture_output=True, text=True)
        self.assertIn('--include-p7-actors', help_result.stdout)

    def test_campaign_raw_types_have_real_asset_mappings(self):
        self.assertEqual(set(MISSION_RAW_TYPES) - set(RACE_UNIT_IDS.values()), set())
        self.assertEqual(RACE_UNIT_IDS['drone'], 41)
        self.assertEqual(RACE_UNIT_IDS['probe'], 64)
        self.assertEqual(RACE_UNIT_IDS['zealot'], 65)
        self.assertEqual(RACE_UNIT_IDS['hydralisk'], 38)
        self.assertEqual(RACE_UNIT_IDS['photon_cannon'], 162)

    def test_all_runtime_raw_actor_and_building_ids_have_source_art(self):
        rules = (Path(__file__).resolve().parents[1] / 'starcraft/race_rules.h').read_text()

        def numeric_raw_ids(function_signature, next_signature, include_roster=False):
            start = rules.index(f'static inline int {function_signature}\n{{')
            end = rules.index(f'static inline int {next_signature}\n{{', start)
            body = rules[start:end]
            if not include_roster:
                # The identity-only extension is off in every normal ROM.
                # Retain the all-gates art audit for shipped actor paths while
                # checking its explicit missing-art set separately below.
                lines, depth = [], 0
                for line in body.splitlines():
                    if re.match(r'\s*#\s*(if|ifdef|ifndef)\b', line):
                        if depth or 'SC64_SKIRMISH_ROSTER' in line:
                            depth += 1
                            continue
                    elif re.match(r'\s*#\s*endif\b', line) and depth:
                        depth -= 1
                        continue
                    if not depth:
                        lines.append(line)
                self.assertEqual(depth, 0)
                body = '\n'.join(lines)
            return {int(value) for value in re.findall(r'return (\d+);', body)}

        runtime_units = numeric_raw_ids('sc64_race_raw_unit(int type)',
                                        'sc64_race_raw_building(int type)')
        unit_assets = set(RACE_UNIT_IDS.values()) | set(T4_UNIT_IDS.values())
        unit_assets |= set(T6_UNIT_IDS.values()) | set(LATE_UNIT_IDS.values())
        unit_assets |= set(ZP_UNIT_IDS.values()) | set(Z5_UNIT_IDS.values())
        unit_assets |= set(Z6_UNIT_IDS.values()) | set(P6_UNIT_IDS.values())
        unit_assets |= set(P7_UNIT_IDS.values())
        p8_adapter = (Path(__file__).resolve().parents[1] /
                      'starcraft/sc64_p8_sprite_adapter.h').read_text()
        self.assertIn('#ifdef SC64_P8_DATA_ENABLED', p8_adapter)
        self.assertIn('raw_type == 28u) raw_type = 12u', p8_adapter)
        unit_assets.add(28)  # P8 raw28 uses the verified shared raw12 descriptor.
        self.assertEqual(runtime_units - unit_assets, {101},
                         'raw 101 is intentionally source-invisible')
        extended_units = numeric_raw_ids('sc64_race_raw_unit(int type)',
                                        'sc64_race_raw_building(int type)', True)
        self.assertEqual(extended_units - unit_assets - {101}, {58, 60, 61, 62, 63},
                         'identity-only roster art gaps must stay explicit; '
                         'these are not enabled in normal ROMs')

        runtime_buildings = numeric_raw_ids('sc64_race_raw_building(int type)',
                                            'sc64_race_unit_from_raw(int raw)')
        building_assets = set(RACE_UNIT_IDS.values()) | set(T4_BUILDING_IDS.values())
        building_assets |= set(LATE_BUILDING_IDS.values()) | set(ZP_BUILDING_IDS.values())
        building_assets |= set(Z5_BUILDING_IDS.values()) | set(Z5_DOODAD_IDS.values())
        building_assets |= set(Z6_BUILDING_IDS.values()) | set(P6_DOODAD_IDS.values())
        building_assets |= set(P7_BUILDING_IDS.values())
        self.assertEqual(runtime_buildings - building_assets, {134},
                         'raw 134 belongs to the default-off Nydus runtime, '
                         'whose source art is not imported yet')
        extended_buildings = numeric_raw_ids('sc64_race_raw_building(int type)',
                                            'sc64_race_unit_from_raw(int raw)', True)
        self.assertEqual(extended_buildings - building_assets, {107, 134},
                         'Comsat and Nydus source art remain unprepared')

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
            base = [{'name': name, 'frame': frame} for name in RACE_UNIT_IDS]
            write_header(path, base, bytes(range(256)), t4)
            text = path.read_text()
        self.assertIn('#ifdef SC64_T4_ENABLED', text)
        self.assertIn('case 15: return &sc64_race_sprite_civilian;', text)
        self.assertIn('case 20: return &sc64_race_sprite_raynor_raw20;', text)
        self.assertIn('case 19: return &sc64_race_sprite_raynor_vulture_t6;', text)
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

    def test_z5_props_have_separate_complete_gate_and_keep_all_frames(self):
        def frame(pixel):
            return {'pixels': bytes((pixel,)), 'alpha': bytes((1,)), 'width': 1,
                    'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                    'frame_height': 1}
        props = [{'name': name, 'frames': [frame(9), frame(10)]}
                 for name in Z5_DOODAD_IDS]
        t4 = [{'name': name, 'frames': [frame(7)]} for name in T4_BUILDING_IDS]
        base_units = [{'name': name, 'frame': frame(8)} for name in RACE_UNIT_IDS]
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sprites.h'
            write_header(path, base_units, bytes(range(256)), t4_building_selected=t4,
                         z5_doodad_selected=props)
            text = path.read_text()
        self.assertIn('#ifdef SC64_Z5_DOODADS', text)
        self.assertIn('#define SC64_Z5_DOODAD_SPRITE_SET_COMPLETE 1', text)
        self.assertIn('case 213:', text)
        self.assertIn('sc64_race_sprite_right_wall_flame_trap_z5_frame_1_pixels[1]', text)
        self.assertIn('#ifndef SC64_Z5_DOODADS', text)
        # In the combined T4/Z5 build, the Z5 raw mapping owns the duplicate
        # prop type; the explicit guard prevents duplicate switch labels.
        segment = text[text.index('sc64_race_building_sprite'):]
        self.assertIn('case 203:', segment)
        self.assertIn('case 212:', segment)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'sprites.h').write_text(text)
            (root / 'fixture.c').write_text('''#include <assert.h>
#include "sprites.h"
#ifndef SC64_Z5_DOODAD_SPRITE_SET_COMPLETE
#error "Z5 prop set must be explicitly complete"
#endif
int main(void) {
    assert(sc64_race_building_sprite(203, 1));
    assert(sc64_race_building_sprite(213, 1));
    assert(sc64_race_building_sprite(212, 0));
    assert(!sc64_race_building_sprite(203, 2));
    return 0;
}
''')
            subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-DSC64_T4_ENABLED', '-DSC64_Z5_DOODADS',
                            str(root / 'fixture.c'), '-o', str(root / 'fixture')],
                           check=True, capture_output=True)
            subprocess.run([str(root / 'fixture')], check=True)

    def test_asset_manifest_hash_ignores_nonidentity_fields(self):
        asset = {'name': 'x', 'unit_id': 15, 'flingy_id': 1, 'sprite_id': 2,
                 'image_id': 3, 'grp_filename_index': 4, 'bolt_path': '00D/001',
                 'sha1': 'a' * 40, 'canvas': [1, 2], 'selected_frame': 0,
                 'frame_rect': [0, 0, 1, 2], 'opaque_pixels': 2,
                 'preview': 'one.png'}
        before = asset_manifest_sha256([asset])
        asset['preview'] = 'renamed.png'
        self.assertEqual(before, asset_manifest_sha256([asset]))

    def test_named_group_hashes_ignore_later_groups_but_track_members(self):
        def asset(name, digest):
            return {'name': name, 'unit_id': len(name), 'flingy_id': 1,
                    'sprite_id': 2, 'image_id': 3, 'grp_filename_index': 4,
                    'bolt_path': f'00D/{len(name):03d}', 'sha1': digest * 40,
                    'canvas': [8, 8], 'selected_frame': 0,
                    'frame_rect': [0, 0, 8, 8], 'opaque_pixels': 32}

        zp_names = set(ZP_UNIT_IDS) | set(ZP_BUILDING_IDS)
        z5_names = set(Z5_UNIT_IDS) | set(Z5_BUILDING_IDS)
        doodad_names = set(Z5_DOODAD_IDS)
        z6_names = set(Z6_UNIT_IDS)
        early = [asset(name, 'a') for name in sorted(zp_names)]
        early += [asset(name, 'b') for name in sorted(z5_names)]
        early_hash = named_asset_manifest_sha256(early, zp_names)
        z5_hash = named_asset_manifest_sha256(early, z5_names)
        doodad_hash = named_asset_manifest_sha256(early, doodad_names)
        z6_hash = named_asset_manifest_sha256(early, z6_names)

        later = early + [asset(name, 'c') for name in sorted(doodad_names | z6_names)]
        self.assertEqual(early_hash, named_asset_manifest_sha256(later, zp_names))
        self.assertEqual(z5_hash, named_asset_manifest_sha256(later, z5_names))
        self.assertNotEqual(doodad_hash,
                            named_asset_manifest_sha256(later, doodad_names))
        self.assertNotEqual(z6_hash, named_asset_manifest_sha256(later, z6_names))

        changed_zp = list(later)
        first_zp_name = next(iter(zp_names))
        changed_zp[0] = asset(first_zp_name, 'd')
        self.assertNotEqual(early_hash,
                            named_asset_manifest_sha256(changed_zp, zp_names))

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
    assert(sc64_race_unit_sprite(115));
#else
    assert(!ship);
#endif
    assert(sc64_race_unit_sprite(19)); /* Raw-19 hero sprite is unconditional. */
    assert(sc64_race_unit_sprite(20)); /* Base enum Raynor alias is unconditional. */
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
        zp_buildings = [{'name': name, 'frames': [frame, frame]}
                        for name in ZP_BUILDING_IDS]
        base_units = [{'name': name, 'frame': frame} for name in RACE_UNIT_IDS]
        t4_units = [{'name': name, 'frame': frame} for name in T4_UNIT_IDS]
        t4_buildings = [{'name': name, 'frames': [frame, frame]}
                         for name in T4_BUILDING_IDS]
        t6_units = [{'name': name, 'frame': frame} for name in T6_UNIT_IDS]
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            full = root / 'late.h'
            write_header(full, base_units, bytes(range(256)), t4_units,
                         t4_buildings, t6_units, late_units, late_buildings,
                         zp_building_selected=zp_buildings)
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
                            '-DSC64_ZP_CAMPAIGN_ACTORS',
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

            zp_units = [{'name': name, 'frame': frame} for name in ZP_UNIT_IDS]
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
        self.assertIn('--include-z6-buildings', help_result.stdout)

    def test_z6_guardian_sprite_set_is_separately_complete_and_gated(self):
        frame = {'pixels': bytes((11,)), 'alpha': bytes((1,)), 'width': 1,
                 'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                 'frame_height': 1}
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root / 'z6.h'
            base_units = [{'name': name, 'frame': frame} for name in RACE_UNIT_IDS]
            write_header(path, base_units, bytes(range(256)),
                         z6_selected=[{'name': name, 'frame': frame}
                                      for name in Z6_UNIT_IDS])
            text = path.read_text()
            (root / 'check.c').write_text('''#include <assert.h>
#include "z6.h"
int main(void) {
#ifdef SC64_Z6_ACTORS
#ifndef SC64_Z6_SPRITE_SET_COMPLETE
#error "complete Guardian sprite set must carry its own marker"
#endif
    assert(sc64_z6_actor_sprite(44));
    assert(!sc64_z6_actor_sprite(43));
#endif
    return 0;
}
''')
            for enabled in (False, True):
                command = ['clang', '-std=c11', '-Werror', str(root / 'check.c'),
                           '-o', str(root / 'check')]
                if enabled:
                    command.append('-DSC64_Z6_ACTORS')
                subprocess.run(command, check=True, capture_output=True)
                subprocess.run([str(root / 'check')], check=True, capture_output=True)
            self.assertIn('#ifdef SC64_Z6_ACTORS', text)
            self.assertIn('#define SC64_Z6_SPRITE_SET_COMPLETE 1', text)
            self.assertIn('case 44: return &sc64_race_sprite_guardian_z6;', text)
            partial = root / 'partial.h'
            write_header(partial, base_units, bytes(range(256)), z6_selected=[])
            self.assertNotIn('SC64_Z6_SPRITE_SET_COMPLETE', partial.read_text())

    def test_z6_building_framebanks_are_independently_complete_and_runtime_gated(self):
        frames = [
            {'pixels': bytes((11 + i,)), 'alpha': bytes((1,)), 'width': 1,
             'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
             'frame_height': 1}
            for i in range(3)
        ]
        assets = [{'name': name, 'frames': frames} for name in Z6_BUILDING_IDS]
        late_aliases = [{'name': name, 'frames': frames}
                        for name in ('robotics_support_bay', 'shield_battery')]
        base_units = [{'name': name, 'frame': frames[0]} for name in RACE_UNIT_IDS]
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            full = root / 'z6_buildings.h'
            write_header(full, base_units, bytes(range(256)),
                         late_building_selected=late_aliases,
                         z6_building_selected=assets)
            text = full.read_text()
            self.assertIn('#ifdef SC64_Z6_RUNTIME\n#define SC64_Z6_BUILDING_SPRITE_SET_COMPLETE 1', text)
            self.assertIn('sc64_race_sprite_robotics_support_bay_z6_frame_2_pixels[1]', text)
            self.assertIn('sc64_race_sprite_shield_battery_z6_frame_2_pixels[1]', text)
            self.assertIn('case 171:', text)
            self.assertIn('case 172:', text)
            self.assertIn('case 2: return &sc64_race_sprite_robotics_support_bay_z6_frame_2;', text)
            self.assertIn('case 2: return &sc64_race_sprite_shield_battery_z6_frame_2;', text)
            (root / 'check.c').write_text('''#include <assert.h>
#include "z6_buildings.h"
int main(void) {
#ifdef SC64_Z6_RUNTIME
#ifndef SC64_Z6_BUILDING_SPRITE_SET_COMPLETE
#error "complete Z6 building set must advertise its own marker"
#endif
    assert(sc64_race_building_sprite(171, 0));
    assert(sc64_race_building_sprite(171, 2));
    assert(!sc64_race_building_sprite(171, 3));
    assert(sc64_race_building_sprite(172, 1));
#else
#ifdef SC64_Z6_BUILDING_SPRITE_SET_COMPLETE
#error "Z6 building marker must stay behind the runtime gate"
#endif
#endif
    return 0;
}
''')
            for defines in ([], ['-DSC64_Z6_RUNTIME'],
                            ['-DSC64_Z6_RUNTIME', '-DSC64_TERRAN_LATE_ENABLED']):
                command = ['clang', '-std=c11', '-Werror', str(root / 'check.c'),
                           '-o', str(root / 'check'), *defines]
                subprocess.run(command, check=True, capture_output=True)
                subprocess.run([str(root / 'check')], check=True, capture_output=True)
            partial = root / 'partial.h'
            write_header(partial, base_units, bytes(range(256)),
                         z6_building_selected=assets[:1])
            self.assertNotIn('SC64_Z6_BUILDING_SPRITE_SET_COMPLETE', partial.read_text())



if __name__ == '__main__':
    unittest.main()
