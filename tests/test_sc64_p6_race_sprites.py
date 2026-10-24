import subprocess
import tempfile
import unittest
from pathlib import Path
import sys

from tools.sc64_race_sprites import (
    P6_DOODAD_IDS, P6_UNIT_IDS, RACE_UNIT_IDS, T4_BUILDING_IDS,
    T4_UNIT_IDS, ZP_BUILDING_IDS, ZP_UNIT_IDS, Z5_DOODAD_IDS, write_header,
)


def frame(pixel):
    return {'pixels': bytes((pixel,)), 'alpha': bytes((1,)), 'width': 1,
            'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
            'frame_height': 1}


class P6RaceSpriteTests(unittest.TestCase):
    def test_p6_id_set_adds_only_three_actors_and_wall_trap(self):
        self.assertEqual(P6_UNIT_IDS, {
            'infested_terran_p6': 50,
            'hero_dark_templar_p6': 74,
            'zeratul_p6': 75,
        })
        self.assertEqual(P6_DOODAD_IDS, {'wall_missile_trap_p6': 210})
        self.assertNotIn(1, P6_UNIT_IDS.values())  # reuse base Ghost art
        self.assertNotIn(195, P6_DOODAD_IDS.values())  # reuse existing beacon
        self.assertNotIn(205, P6_DOODAD_IDS.values())  # reuse T4/Z5 door GRPs

    def test_p6_assets_are_gated_and_indexed_frames_fail_closed(self):
        base_units = [{'name': name, 'frame': frame(8)} for name in RACE_UNIT_IDS]
        t4_units = [{'name': name, 'frame': frame(9)} for name in T4_UNIT_IDS]
        t4_props = [{'name': name, 'frames': [frame(10), frame(11)]}
                    for name in T4_BUILDING_IDS]
        zp_units = [{'name': name, 'frame': frame(12)} for name in ZP_UNIT_IDS]
        zp_props = [{'name': name, 'frames': [frame(13)]}
                    for name in ZP_BUILDING_IDS]
        z5_props = [{'name': name, 'frames': [frame(14), frame(15)]}
                    for name in Z5_DOODAD_IDS]
        p6_units = [{'name': name, 'frame': frame(16)} for name in P6_UNIT_IDS]
        p6_props = [{'name': 'wall_missile_trap_p6',
                     'frames': [frame(20 + n) for n in range(7)]}]

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            header = root / 'p6.h'
            write_header(header, base_units, bytes(range(256)), t4_units,
                         t4_props, zp_selected=zp_units,
                         zp_building_selected=zp_props,
                         z5_doodad_selected=z5_props,
                         p6_selected=p6_units,
                         p6_doodad_selected=p6_props)
            text = header.read_text()
            self.assertIn('#ifdef SC64_P6_ACTORS', text)
            self.assertIn('#define SC64_P6_SPRITE_SET_COMPLETE 1', text)
            self.assertIn('case 50: return &sc64_race_sprite_infested_terran_p6;', text)
            self.assertIn('case 74: return &sc64_race_sprite_hero_dark_templar_p6;', text)
            self.assertIn('case 75: return &sc64_race_sprite_zeratul_p6;', text)
            self.assertIn('case 210:', text)
            self.assertIn('case 6: return &sc64_race_sprite_wall_missile_trap_p6_frame_6;', text)

            (root / 'p6.c').write_text('''#include <assert.h>
#include "p6.h"
int main(void) {
#ifdef SC64_P6_ACTORS
#ifndef SC64_P6_SPRITE_SET_COMPLETE
#error "complete P6 source-art set must advertise its marker"
#endif
    assert(sc64_race_unit_sprite(1));
    assert(sc64_race_unit_sprite(50));
    assert(sc64_race_unit_sprite(74));
    assert(sc64_race_unit_sprite(75));
    assert(sc64_race_unit_sprite(195));
    assert(sc64_race_building_sprite(196, 0));
    assert(sc64_race_building_sprite(205, 1));
    assert(sc64_race_building_sprite(203, 1));
    assert(sc64_race_building_sprite(213, 1));
    assert(sc64_race_building_sprite(210, 0));
    assert(sc64_race_building_sprite(210, 6));
    assert(!sc64_race_building_sprite(210, 7));
#else
    assert(!sc64_race_unit_sprite(50));
    assert(!sc64_race_unit_sprite(74));
    assert(!sc64_race_unit_sprite(75));
    assert(!sc64_race_building_sprite(210, 0));
#endif
    return 0;
}
''')
            flags = ['-DSC64_T4_ENABLED', '-DSC64_ZP_CAMPAIGN_ACTORS',
                     '-DSC64_Z5_DOODADS']
            for p6_enabled in (False, True):
                command = ['clang', '-std=c11', '-Wall', '-Wextra', '-Werror',
                           str(root / 'p6.c'), '-o', str(root / 'p6')]
                command += flags
                if p6_enabled:
                    command.append('-DSC64_P6_ACTORS')
                subprocess.run(command, check=True, capture_output=True)
                subprocess.run([str(root / 'p6')], check=True, capture_output=True)

    def test_partial_p6_selection_never_advertises_completeness(self):
        with tempfile.TemporaryDirectory() as temp:
            header = Path(temp) / 'partial.h'
            write_header(header, [], bytes(range(256)),
                         p6_selected=[{'name': 'infested_terran_p6',
                                       'frame': frame(7)}],
                         p6_doodad_selected=[{'name': 'wall_missile_trap_p6',
                                              'frames': [frame(8)]}])
            text = header.read_text()
        self.assertNotIn('SC64_P6_SPRITE_SET_COMPLETE', text)
        self.assertIn('case 50: return &sc64_race_sprite_infested_terran_p6;', text)
        self.assertIn('default: return (const Sc64SpriteFrame *)0;', text)

    def test_cli_exposes_opt_in_without_changing_base_only_writer(self):
        with tempfile.TemporaryDirectory() as temp:
            header = Path(temp) / 'base.h'
            write_header(header, [{'name': 'marine', 'frame': frame(1)}],
                         bytes(range(256)))
            text = header.read_text()
        self.assertNotIn('SC64_P6_ACTORS', text)
        help_result = subprocess.run(
            [sys.executable, str(Path(__file__).parents[1] / 'tools/sc64_race_sprites.py'), '--help'],
            check=True, capture_output=True, text=True)
        self.assertIn('--include-p6-actors', help_result.stdout)


if __name__ == '__main__':
    unittest.main()
