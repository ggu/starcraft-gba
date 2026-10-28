"""Unit raster packing and optional prepared-game regression checks."""
# SPDX-License-Identifier: GPL-3.0-or-later
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from tools.sc64_race_sprites import (
    RACE_UNIT_IDS,
    prepack_unit_sprite_words,
    write_header,
)


class UnitWordSpriteTests(unittest.TestCase):
    def test_prepack_has_four_phases_and_zero_color_in_transparent_lanes(self):
        phases = prepack_unit_sprite_words(3, 2, bytes((0x11, 0, 0x22, 0x33, 0x44, 0)))
        self.assertEqual(len(phases), 4)
        self.assertEqual(phases[0][0:2], (0x00220011, 0x00ff00ff))
        self.assertEqual(phases[1][0:2], (0x22001100, 0xff00ff00))
        self.assertEqual(phases[2][0:2], (0x00110000, 0x00ff0000))
        for phase in phases:
            for color, mask in zip(phase[0::2], phase[1::2]):
                for lane in range(4):
                    if not (mask & (0xff << (lane * 8))):
                        self.assertEqual(color & (0xff << (lane * 8)), 0)

    def test_prepack_rejects_bad_dimensions_or_pixel_count(self):
        for args in ((0, 1, b''), (1, 0, b''), (257, 1, bytes(257)),
                     (1, 1, b'')):
            with self.subTest(args=args), self.assertRaises(ValueError):
                prepack_unit_sprite_words(*args)

    def test_spawned_and_explicit_raw_units_take_packed_game_renderer_path(self):
        compiler = shutil.which('clang') or shutil.which('cc')
        if not compiler:
            self.skipTest('no host C compiler available')

        repo = Path(__file__).resolve().parents[1]
        generated_candidates = (
            repo / '.local/sc64/zp-stage/generated/sc64_race_sprites_generated.h',
            repo / '.local/sc64/races/generated/sc64_race_sprites_generated.h',
        )
        generated = next((path for path in generated_candidates
                          if path.is_file() and
                          'SC64_RACE_UNIT_WORD_SPRITE_METADATA 1' in path.read_text()), None)
        if generated is None:
            self.skipTest('private generated race sprite header with unit-word metadata is unavailable')
        briefing_header = repo / '.local/sc64/briefings/generated/sc64_briefing_generated.h'
        if not briefing_header.is_file():
            self.skipTest('private generated campaign briefing header is unavailable')

        defines = [
            '-DGBA_HOST', '-DGBA_SPRITE_WORDS', '-DGBA_SCENE_CLIP', '-DGBA_WINDOW_BANDS',
            '-DSC64_IMPORTED_MISSION', '-DSC64_HAS_TERRAIN_ART', '-DSC64_HAS_SPRITES',
            '-DSC64_AUDIO_IMPORTED', '-DSC64_RACES', '-DSC64_CONSTRUCTION_SOURCE_ART',
            '-DSC64_UNIFIED', '-DSC64_IMPORTED_SKIRMISH', '-DSC64_Z3_ENABLED',
            '-DSC64_CAMPAIGN_RACE=1', '-DSC64_ROUTE_PACK3', '-DSC64_VISIBILITY_SPANS',
            '-DSC64_Z3_WINDOWED_SCHEDULE', '-DSC64_NO_FRAME_CLEAR', '-DGBA_PRESENT_DEADLINE',
            '-DSC64_TARGET_OCCUPANCY', '-DSC64_UNARMED_QUERY_SKIP', '-DSC64_Z4_DATA_ENABLED',
            '-DSC64_Z4_ENABLED', '-DSC64_Z4_RUNTIME', '-DSC64_Z4_CAMPAIGN_ENTRY',
            '-DSC64_ZP_CAMPAIGN_ACTORS', '-DSC64_ZP_CAMPAIGN_STORAGE',
            '-DSC64_ZP_DEFENSE_TARGETS', '-DSC64_HUD_BUFFER_REUSE', '-DSC64_TERRAIN_SCROLL',
            '-DSC64_STATIC_KEY_CULL', '-DSC64_STATIC_DIRTY_ROWS',
            '-DSC64_ROUTE_FLOOD_SPLIT', '-DSC64_UNIT_WORD_SPRITES',
            '-DSC64_UNIT_WORD_SPRITE_DIAGNOSTICS',
        ]
        includes = [
            repo / 'common', repo / 'starcraft', generated.parent,
            repo / '.local/sc64/backwater/generated', repo / '.local/sc64/audio/generated',
            repo / '.local/sc64/briefings/generated',
            repo / '.local/sc64/construction/generated', repo / '.local/sc64/skirmish/generated',
            repo / '.local/sc64/zp-stage/generated', repo / '.local/sc64/races/generated',
            repo / '.local/sc64/z4/generated',
        ]
        with tempfile.TemporaryDirectory() as temp:
            binary = Path(temp) / 'spawn-game'
            command = [compiler, '-std=c11', '-O1', '-g', '-fno-strict-aliasing',
                       '-fsanitize=address,undefined', *defines]
            for include in includes:
                command.extend(['-I', str(include)])
            command.extend([str(repo / 'tests/sc64_unit_word_spawn_game.c'),
                            str(repo / 'common/gba.c'), '-o', str(binary)])
            compiled = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            executed = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(executed.returncode, 0, executed.stderr or executed.stdout)

    def test_generated_accessor_is_opt_in_raw_bounded_and_frame_checked(self):
        compiler = shutil.which('clang') or shutil.which('cc')
        if not compiler:
            self.skipTest('no host C compiler available')

        def frame(pixel):
            return {'pixels': bytes((pixel,)), 'alpha': bytes((1,)),
                    'width': 1, 'height': 1, 'x': 0, 'y': 0,
                    'frame_width': 1, 'frame_height': 1}

        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            header = root / 'sprites.h'
            units = [{'name': name, 'frame': frame(7)} for name in RACE_UNIT_IDS]
            t4_units = [{'name': 'civilian', 'frame': frame(9)}]
            write_header(header, units, bytes(range(256)), t4_selected=t4_units,
                         prepack_unit_words=True)
            generated = header.read_text()
            expected_raws = {raw for raw in RACE_UNIT_IDS.values() if raw < 106} | {15}
            self.assertIn('#define SC64_RACE_UNIT_WORD_SPRITE_METADATA 1', generated)
            self.assertEqual(generated.count('static const GbaSpriteWordFrame sc64_race_unit_words_'),
                             len(expected_raws))
            self.assertNotIn('sc64_race_unit_words_106 =', generated)
            self.assertIn('#ifdef SC64_UNIT_WORD_SPRITES', generated)
            self.assertIn('#ifdef SC64_T4_ENABLED\nstatic const u32 sc64_race_unit_words_15_p0', generated)

            source = root / 'check.c'
            source.write_text('''
#define SC64_UNIT_WORD_SPRITES 1
#define SC64_RACES 1
#define SC64_T4_ENABLED 1
#include "gba.h"
#include "sprites.h"
#include <assert.h>
int main(void) {
    const Sc64SpriteFrame *marine = sc64_race_unit_sprite(0);
    const GbaSpriteWordFrame *packed = sc64_race_unit_words_sprite(0, marine);
    assert(marine && packed && packed->width == 1 && packed->height == 1);
    assert(packed->phase_words[0][0] == 7 && packed->phase_words[0][1] == 255);
    assert(packed->phase_words[1][0] == 0x700 && packed->phase_words[1][1] == 0xff00);
    assert(!sc64_race_unit_words_sprite(0, sc64_race_unit_sprite(1)));
    assert(!sc64_race_unit_words_sprite(106, sc64_race_unit_sprite(106)));
    assert(sc64_race_unit_words_sprite(15, sc64_race_unit_sprite(15)));
    return 0;
}
''')
            binary = root / 'check'
            subprocess.run([compiler, '-std=c11', '-I', str(Path(__file__).parents[1] / 'common'),
                            '-I', str(root), str(source), '-o', str(binary)],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(binary)], check=True, capture_output=True, text=True)

            legacy = root / 'legacy.h'
            write_header(legacy, units, bytes(range(256)))
            legacy_text = legacy.read_text()
            self.assertNotIn('SC64_RACE_UNIT_WORD_SPRITE_METADATA', legacy_text)
            self.assertNotIn('sc64_race_unit_words_', legacy_text)


if __name__ == '__main__':
    unittest.main()
