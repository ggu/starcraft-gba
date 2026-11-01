"""New-contributor paths must work without an emulator or imported game data."""
import contextlib
import hashlib
import importlib.machinery
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
from tools import sc64_split_profile, sc64_split_smoke

ROOT = Path(__file__).resolve().parents[1]
UNIT_WORD_SPRITE_HEADER = (
    '#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n'
    '#define SC64_RACE_UNIT_WORD_SPRITE_METADATA 1\n'
    'static const u32 sc64_race_unit_words_0_p0[1] = {0};\n'
    'static const GbaSpriteWordFrame sc64_race_unit_words_0 = {1,1,{0,0,0,0}};\n'
    'static inline const GbaSpriteWordFrame *sc64_race_unit_words_sprite(unsigned short raw_type, const Sc64SpriteFrame *frame) {\n'
    '    switch (raw_type) { case 0: return &sc64_race_unit_words_0; }\n'
    '    return (const GbaSpriteWordFrame *)0;\n'
    '}\n'
)
FACING_SPRITE_HEADER = (
    '#define SC64_UNIT_FACING_BANK_VERSION 2\n'
    '#define SC64_UNIT_FACING_DIRECTIONAL_METADATA 1\n'
    '#define SC64_UNIT_FACING_WORD_METADATA 1\n'
    'static const unsigned char sc64_unit_facing_0_0_pixels[1] = {1};\n'
    'static const Sc64SpriteFrame sc64_unit_facing_0[17] = {{0}};\n'
    'static const u32 sc64_unit_facing_0_word_0_p0[1] = {1};\n'
    'static const GbaSpriteWordFrame sc64_unit_facing_0_word_0 = {0};\n'
    'static inline const Sc64SpriteFrame *sc64_unit_facing_frames(unsigned short raw_type) { return 0; }\n'
    'static inline const Sc64UnitFacingWordFrame *sc64_unit_facing_word_frame(unsigned short raw_type, unsigned char direction) { return 0; }\n'
)
ATTACK_POSE_HEADER = (
    '#define SC64_UNIT_ATTACK_POSE_BANK_VERSION 1\n'
    '#define SC64_UNIT_ATTACK_POSE_METADATA 1\n'
    'static const unsigned char sc64_unit_attack_pixels_0[1] = {1};\n'
    'static inline unsigned char sc64_unit_attack_source_release_tick(unsigned short raw_type, int target_air) { return 0; }\n'
    'static inline unsigned char sc64_unit_attack_source_duration(unsigned short raw_type, int target_air) { return 1; }\n'
    'static inline unsigned short sc64_unit_attack_source_full_duration(unsigned short raw_type, int target_air) { return 2; }\n'
    'static inline const Sc64SpriteFrame *sc64_unit_attack_source_frame(unsigned short raw_type, int target_air, unsigned char heading32, unsigned int post_release_tick) { return 0; }\n'
    'static inline const Sc64SpriteFrame *sc64_unit_attack_source_full_frame(unsigned short raw_type, int target_air, unsigned char heading32, unsigned int attack_tick) { return 0; }\n'
    '#define SC64_UNIT_ATTACK_POSE_PIXEL_BYTES 1u\n'
)
WEAPON_SOURCE_HEADER = (
    '#define SC64_WEAPON_SOURCE_BANK_VERSION 1\n'
    '#define SC64_WEAPON_SOURCE_SPROLUSELO_LO 1\n'
    '#define SC64_WEAPON_SOURCE_METADATA 1\n'
    '#define SC64_WEAPON_SOURCE_PROFILE_COUNT 1\n'
    '#define SC64_WEAPON_SOURCE_IMAGE_COUNT 1\n'
    '#define SC64_WEAPON_SOURCE_FRAME_COUNT 1\n'
    '#define SC64_WEAPON_SOURCE_ANIMATION_COUNT 1\n'
    '#define SC64_WEAPON_SOURCE_LUT_COUNT 1\n'
    'static const Sc64WeaponSourceAttackProfile sc64_weapon_source_profiles[1] = {{0}};\n'
    'static const Sc64WeaponSourceImage sc64_weapon_source_images[1] = {{0}};\n'
    'static const Sc64WeaponSourceFrame sc64_weapon_source_frames[1] = {{0}};\n'
    'static const Sc64WeaponSourceAnimation sc64_weapon_source_animations[1] = {{0}};\n'
    'static const Sc64WeaponSourceLut sc64_weapon_source_luts[1] = {{0}};\n'
    'static const uint8_t sc64_weapon_source_pixels[1] = {1};\n'
    'static const uint8_t sc64_weapon_source_lut_bytes[1] = {1};\n'
    'sc64_weapon_source_profile_lookup(\n'
    'sc64_weapon_source_image_lookup(\n'
    'sc64_weapon_source_frame_lookup(\n'
    'sc64_weapon_source_frame_pixels(\n'
    'sc64_weapon_source_animation_lookup(\n'
    'sc64_weapon_source_lut_for_image(\n'
)
BRIEFING_HEADER = (
    '#include "gba.h"\n'
    '#include "sc64_briefing.h"\n'
    '#define SC64_BRIEFING_BACKGROUND_WIDTH 240\n'
    '#define SC64_BRIEFING_BACKGROUND_HEIGHT 160\n'
    '#define SC64_BRIEFING_PORTRAIT_WIDTH 32\n'
    '#define SC64_BRIEFING_PORTRAIT_HEIGHT 32\n'
    '#define SC64_BRIEFING_PORTRAIT_COUNT 23\n'
    'static const Sc64BriefingPage sc64_briefing_pages_0[] = {\n'
    '    {"OBJECTIVES", "Destroy the enemy command center.", -1},\n'
    '};\n'
    'static const Sc64Briefing sc64_briefings[30] = {{0}};\n'
    'static inline const Sc64Briefing *sc64_briefing_get(int global_mission) {\n'
    '    return global_mission >= 0 && global_mission < 30 ? &sc64_briefings[global_mission] : 0;\n'
    '}\n'
)
DEATH_SOURCE_HEADER = (
    '#define SC64_DEATH_SOURCE_BANK_VERSION 2u\n'
    '#define SC64_DEATH_SOURCE_METADATA_VERSION 1u\n'
    '#define SC64_DEATH_SOURCE_ACTOR_COUNT 1u\n'
    '#define SC64_DEATH_SOURCE_PIXEL_BYTES 1u\n'
    '#define SC64_DEATH_SOURCE_WORD_METADATA 1u\n'
    '#define SC64_DEATH_SOURCE_WORD_BYTES 1u\n'
    'static const unsigned char sc64_death_image_1_frame_0_pixels[1] = {1};\n'
    'static const u32 sc64_death_image_1_frame_0_words_p0[1] = {1};\n'
    'static const u32 sc64_death_image_1_frame_0_words_p1[1] = {1};\n'
    'static const u32 sc64_death_image_1_frame_0_words_p2[1] = {1};\n'
    'static const u32 sc64_death_image_1_frame_0_words_p3[1] = {1};\n'
    'static const GbaSpriteWordFrame sc64_death_image_1_frame_0_words = {0};\n'
    'static const Sc64DeathSource sc64_death_raw_1 = {0};\n'
    'sc64_death_image_frame(unsigned short image_id, unsigned short frame_index)\n'
    'sc64_death_image_word_frame(unsigned short image_id, unsigned short frame_index)\n'
    'sc64_death_source(int raw_unit_id)\n'
)
TANK_VFX_HEADER = (
    '#define SC64_TANK_SOURCE_VFX_VERSION 1\n'
    'static const Sc64TankSourceFrame sc64_tank_frames_533[14] = {{0}};\n'
    'static const uint8_t sc64_tank_source_pixels[1] = {1};\n'
    'static const uint8_t sc64_tank_projected_lut[63*256] = {0};\n'
)
CONSTRUCTION_COLOR_HEADER = (
    '#define SC64_CONSTRUCTION_COLOR_MODIFIERS_VERSION 1\n'
    'static const unsigned char sc64_warp_211_strengths_0[1] = {32};\n'
    'static const unsigned char sc64_construction_blue_lookup[40*256] = {0};\n'
    'sc64_construction_modifier_frame(int image_id, int frame);\n'
    'sc64_construction_modifier_lookup(int image_id, int *rows);\n'
)

WALK_CYCLE_HEADER = (
    '#define SC64_UNIT_WALK_FACING_SHA256 "' + hashlib.sha256(FACING_SPRITE_HEADER.encode('ascii')).hexdigest() + '"\n'
    '#define SC64_UNIT_WALK_BANK_VERSION 1\n'
    'static const Sc64UnitWalkStep sc64_unit_walk_i54_steps[1] = {{0}};\n'
    'static const Sc64UnitWalkCycle sc64_unit_walk_i54 = {0};\n'
    'static inline const Sc64UnitWalkCycle *sc64_unit_walk_cycle(unsigned short raw_type) { return 0; }\n'
    '#define SC64_UNIT_WALK_HEADINGS 32u\n'
)


def emit_walk_header(root):
    path = root / dev.WALK_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(WALK_CYCLE_HEADER, encoding='ascii')


def emit_construction_header(root):
    path = root / dev.CONSTRUCTION_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(CONSTRUCTION_COLOR_HEADER, encoding='ascii')


def emit_facing_header(root):
    emit_construction_header(root)
    path = root / dev.FACING_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(FACING_SPRITE_HEADER, encoding='ascii')
    emit_walk_header(root)
    path = root / dev.TANK_VFX_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(TANK_VFX_HEADER, encoding='ascii')
    path = root / dev.ATTACK_POSE_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(ATTACK_POSE_HEADER, encoding='ascii')
    path = root / dev.WEAPON_SOURCE_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(WEAPON_SOURCE_HEADER, encoding='ascii')
    emit_death_source_header(root)


def emit_attack_pose_header(root):
    path = root / dev.ATTACK_POSE_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(ATTACK_POSE_HEADER, encoding='ascii')


def emit_weapon_source_header(root):
    path = root / dev.WEAPON_SOURCE_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(WEAPON_SOURCE_HEADER, encoding='ascii')


def emit_death_source_header(root):
    path = root / dev.DEATH_SOURCE_HEADER
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(DEATH_SOURCE_HEADER, encoding='ascii')


P4_SCENARIO_HEADER = (
    '#define SC64_P4_SCENARIO_GENERATED_H 1\n'
    '#define SC64_P4_CREATE_PREFIX_VERSION 1\n'
    'static const unsigned char sc64_p4_source_create_prefix[2] = {0,1};\n'
)
loader = importlib.machinery.SourceFileLoader('starcraft_dev', str(ROOT / 'dev'))
spec = importlib.util.spec_from_loader(loader.name, loader)
dev = importlib.util.module_from_spec(spec)
loader.exec_module(dev)
verify_loader = importlib.machinery.SourceFileLoader(
    'starcraft_verify_roms', str(ROOT / 'tools/verify_roms.py'))
verify_spec = importlib.util.spec_from_loader(verify_loader.name, verify_loader)
verify_roms = importlib.util.module_from_spec(verify_spec)
verify_loader.exec_module(verify_roms)


class OnboardingTests(unittest.TestCase):
    def test_construction_readiness_requires_glow_version_and_payload(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'construction.h'
            self.assertFalse(dev.construction_color_input_ready(header))
            for text in (
                '#define SC64_CONSTRUCTION_SPRITES_GENERATED_H\n',
                CONSTRUCTION_COLOR_HEADER.replace('MODIFIERS_VERSION 1', 'MODIFIERS_VERSION 10'),
                CONSTRUCTION_COLOR_HEADER.replace('sc64_construction_modifier_frame(', 'old_frame('),
                CONSTRUCTION_COLOR_HEADER.replace('211_strengths_0[1]', '211_strengths_0[0]'),
                CONSTRUCTION_COLOR_HEADER.replace('blue_lookup[40*256]', 'blue_lookup[0]'),
            ):
                header.write_text(text, encoding='ascii')
                self.assertFalse(dev.construction_color_input_ready(header))
            header.write_text(CONSTRUCTION_COLOR_HEADER, encoding='ascii')
            self.assertTrue(dev.construction_color_input_ready(header))

    def test_tank_effect_readiness_rejects_empty_or_stale_payload(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'tank.h'
            self.assertFalse(dev.tank_vfx_input_ready(header))
            for text in (
                TANK_VFX_HEADER.replace('VFX_VERSION 1', 'VFX_VERSION 10'),
                TANK_VFX_HEADER.replace('sc64_tank_frames_533[14]', 'sc64_tank_frames_533[1]'),
                TANK_VFX_HEADER.replace('sc64_tank_source_pixels[1]', 'sc64_tank_source_pixels[0]'),
                TANK_VFX_HEADER.replace('sc64_tank_projected_lut[63*256]', 'missing_lut[63*256]'),
            ):
                header.write_text(text, encoding='ascii')
                self.assertFalse(dev.tank_vfx_input_ready(header))
            header.write_text(TANK_VFX_HEADER, encoding='ascii')
            self.assertTrue(dev.tank_vfx_input_ready(header))

    def test_directional_readiness_rejects_legacy_markers_and_empty_bank(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'facing.h'
            invalid = (
                '#define SC64_UNIT_FACING_PIXEL_BYTES 1\n',
                FACING_SPRITE_HEADER.replace('BANK_VERSION 2', 'BANK_VERSION 20'),
                FACING_SPRITE_HEADER.replace('DIRECTIONAL_METADATA 1', 'DIRECTIONAL_METADATA 0'),
                FACING_SPRITE_HEADER.replace('WORD_METADATA 1', 'WORD_METADATA 0'),
                FACING_SPRITE_HEADER.replace('sc64_unit_facing_word_frame(', 'missing_accessor('),
                FACING_SPRITE_HEADER.replace('sc64_unit_facing_0_word_0 =', 'missing_frame ='),
                FACING_SPRITE_HEADER.replace('sc64_unit_facing_0_word_0_p0[1]', 'sc64_unit_facing_0_word_0_p0[0]'),
                FACING_SPRITE_HEADER.replace('sc64_unit_facing_0[17]', 'sc64_unit_facing_0[1]'),
                FACING_SPRITE_HEADER.replace('sc64_unit_facing_0_0_pixels[1]', 'sc64_unit_facing_0_0_pixels[0]'),
            )
            self.assertFalse(dev.facing_sprite_input_ready(header))
            for text in invalid:
                header.write_text(text, encoding='ascii')
                self.assertFalse(dev.facing_sprite_input_ready(header))
            header.write_text(FACING_SPRITE_HEADER, encoding='ascii')
            self.assertTrue(dev.facing_sprite_input_ready(header))

    def test_walk_cycle_readiness_requires_version_headings_and_a_cycle(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'walk.h'
            invalid = (
                WALK_CYCLE_HEADER.replace('BANK_VERSION 1', 'BANK_VERSION 10'),
                WALK_CYCLE_HEADER.replace('HEADINGS 32u', 'HEADINGS 17u'),
                WALK_CYCLE_HEADER.replace('sc64_unit_walk_cycle(', 'missing_accessor('),
                WALK_CYCLE_HEADER.replace('sc64_unit_walk_i54 =', 'missing_cycle ='),
            )
            self.assertFalse(dev.walk_cycle_input_ready(header))
            for text in invalid:
                header.write_text(text, encoding='ascii')
                self.assertFalse(dev.walk_cycle_input_ready(header))
            header.write_text(WALK_CYCLE_HEADER, encoding='ascii')
            self.assertTrue(dev.walk_cycle_input_ready(header))

    def test_attack_pose_readiness_requires_version_accessors_and_pixels(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'attack.h'
            self.assertFalse(dev.attack_pose_input_ready(header))
            invalid = (
                ATTACK_POSE_HEADER.replace('BANK_VERSION 1', 'BANK_VERSION 2'),
                ATTACK_POSE_HEADER.replace('POSE_METADATA 1', 'POSE_METADATA 0'),
                ATTACK_POSE_HEADER.replace('source_release_tick(', 'missing_release('),
                ATTACK_POSE_HEADER.replace('source_duration(', 'missing_duration('),
                ATTACK_POSE_HEADER.replace('source_frame(', 'missing_frame('),
                ATTACK_POSE_HEADER.replace('source_full_duration(', 'missing_full_duration('),
                ATTACK_POSE_HEADER.replace('source_full_frame(', 'missing_full_frame('),
                ATTACK_POSE_HEADER.replace('attack_pixels_0[1]', 'attack_pixels_0[0]'),
                ATTACK_POSE_HEADER.replace('POSE_PIXEL_BYTES 1u', 'POSE_PIXEL_BYTES 0u'),
            )
            for text in invalid:
                header.write_text(text, encoding='ascii')
                self.assertFalse(dev.attack_pose_input_ready(header))
            header.write_text(ATTACK_POSE_HEADER, encoding='ascii')
            self.assertTrue(dev.attack_pose_input_ready(header))

    def test_weapon_source_readiness_requires_selectors_frames_and_luts(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'weapon.h'
            self.assertFalse(dev.weapon_source_input_ready(header))
            invalid = (
                WEAPON_SOURCE_HEADER.replace('BANK_VERSION 1', 'BANK_VERSION 2'),
                WEAPON_SOURCE_HEADER.replace('SOURCE_METADATA 1', 'SOURCE_METADATA 0'),
                WEAPON_SOURCE_HEADER.replace('source_profile_lookup(', 'missing_profile('),
                WEAPON_SOURCE_HEADER.replace('source_image_lookup(', 'missing_image('),
                WEAPON_SOURCE_HEADER.replace('source_frame_lookup(', 'missing_frame('),
                WEAPON_SOURCE_HEADER.replace('source_frame_pixels(', 'missing_pixels('),
                WEAPON_SOURCE_HEADER.replace('source_animation_lookup(', 'missing_animation('),
                WEAPON_SOURCE_HEADER.replace('source_lut_for_image(', 'missing_lut('),
                WEAPON_SOURCE_HEADER.replace('source_pixels[1]', 'source_pixels[0]'),
                WEAPON_SOURCE_HEADER.replace('source_lut_bytes[1]', 'source_lut_bytes[0]'),
                WEAPON_SOURCE_HEADER.replace('PROFILE_COUNT 1', 'PROFILE_COUNT 0'),
                WEAPON_SOURCE_HEADER.replace('FRAME_COUNT 1', 'FRAME_COUNT 0'),
                WEAPON_SOURCE_HEADER.replace('LUT_COUNT 1', 'LUT_COUNT 0'),
            )
            for text in invalid:
                header.write_text(text, encoding='ascii')
                self.assertFalse(dev.weapon_source_input_ready(header))
            header.write_text(WEAPON_SOURCE_HEADER, encoding='ascii')
            self.assertTrue(dev.weapon_source_input_ready(header))

    @patch.object(dev, "bc_inputs_ready", return_value=True)
    def test_terran_build_requires_directional_bank_before_make(self, _bc_ready):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / '.local/sc64/terran-late/import-stage/sc64_race_sprites_generated.h'
            header.parent.mkdir(parents=True)
            header.write_text(UNIT_WORD_SPRITE_HEADER, encoding='ascii')
            with patch.object(dev, 'ROOT', root), patch.object(dev, 'run') as run:
                with self.assertRaisesRegex(RuntimeError, 'directional sprite bank'):
                    dev.build('terran')
                run.assert_not_called()
                emit_facing_header(root)
                dev.build('terran')
                run.assert_called_once_with(['make', 'terran'])
                run.reset_mock()
                (root / dev.TANK_VFX_HEADER).unlink()
                with self.assertRaisesRegex(RuntimeError, 'source Tank effects'):
                    dev.build('terran')
                run.assert_not_called()
                emit_facing_header(root)
                (root / dev.ATTACK_POSE_HEADER).unlink()
                with self.assertRaisesRegex(RuntimeError, 'source attack-pose data'):
                    dev.build('terran')
                run.assert_not_called()
                emit_facing_header(root)
                (root / dev.WEAPON_SOURCE_HEADER).unlink()
                with self.assertRaisesRegex(RuntimeError, 'source weapon VFX data'):
                    dev.build('terran')
                run.assert_not_called()
                emit_facing_header(root)
                (root / dev.CONSTRUCTION_HEADER).unlink()
                with self.assertRaisesRegex(RuntimeError, 'blue-glow modifier bank'):
                    dev.build('terran')
                run.assert_not_called()

    def test_z3_and_p3_profile_gates_are_required_and_distinct(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'sc64_races_generated.h'
            header.write_text('#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                              'sc64_race_z3_units\n#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n',
                              encoding='utf-8')
            self.assertTrue(dev.race_campaign_input_ready('zerg', header))
            self.assertFalse(dev.race_campaign_input_ready('protoss', header))
            header.write_text('#if defined(SC64_UNIFIED) && defined(SC64_P3_ENABLED)\n'
                              'sc64_race_p3_units\n#endif /* SC64_UNIFIED && SC64_P3_ENABLED */\n',
                              encoding='utf-8')
            self.assertTrue(dev.race_campaign_input_ready('protoss', header))
            header.write_text('sc64_race_z3_units\n', encoding='utf-8')
            self.assertFalse(dev.race_campaign_input_ready('zerg', header))

    def test_canonical_race_verifiers_use_current_mission_identities(self):
        self.assertEqual(verify_roms.TARGETS['--zerg'],
                         ('StarCraft-Zerg-Z7-Transport1-GBA', 'starcraft-zerg-z7', 'SCZ7', 'SC ZERG Z7'))
        self.assertEqual(verify_roms.TARGETS['--zerg-z6'],
                         ('StarCraft-Zerg-Z6-GBA', 'starcraft-zerg-z6', 'SCZ6', 'SC ZERG Z6'))
        self.assertEqual(verify_roms.TARGETS['--zerg-z7'],
                         ('StarCraft-Zerg-Z7-Transport1-GBA', 'starcraft-zerg-z7', 'SCZ7', 'SC ZERG Z7'))
        self.assertEqual(verify_roms.TARGETS['--zerg-z4'],
                         ('StarCraft-Zerg-Z4-GBA', 'starcraft-zerg-z4', 'SCZ4', 'SC ZERG Z4'))
        self.assertEqual(verify_roms.TARGETS['--zerg-z3'],
                         ('StarCraft-Zerg-GBA', 'starcraft-zerg', 'SCZ3', 'SC ZERG Z3'))
        self.assertEqual(verify_roms.TARGETS['--protoss'],
                         ('StarCraft-Protoss-P7-Transport1-GBA', 'starcraft-protoss-p7',
                          'SCP7', 'SC PROTOSS7'))
        self.assertEqual(verify_roms.TARGETS['--protoss-p6'],
                         ('StarCraft-Protoss-P6-GBA', 'starcraft-protoss-p6',
                          'SCP6', 'SC PROTOSS6'))
        self.assertEqual(verify_roms.TARGETS['--protoss-p5-shuttle'],
                         ('StarCraft-Protoss-P5-Shuttle-GBA', 'starcraft-protoss-p5-shuttle',
                          'SCPB', 'SC P5 CARGO'))
        self.assertEqual(verify_roms.TARGETS['--protoss-p4-storm'],
                         ('StarCraft-Protoss-P4-Storm-GBA', 'starcraft-protoss-p4-storm',
                          'SCP5', 'SC P4 STORM'))
        self.assertEqual(verify_roms.TARGETS['--protoss-p3'],
                         ('StarCraft-Protoss-GBA', 'starcraft-protoss', 'SCP3', 'SC PROTOSS3'))

    def test_dev_race_tests_include_full_campaign_targets(self):
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg'], 'test-zerg')
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg-z6'], 'test-zerg-z6')
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg-z7'], 'test-zerg-z7')
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg-z4'], 'test-zerg-z4')
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg-z3'], 'test-zerg-z3')
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss'], 'test-protoss')
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss-p7'], 'test-protoss')
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss-p6'], 'test-protoss-p6')
        self.assertEqual(dev.GAMES['protoss'], ('protoss', 'build/StarCraft-Protoss-P7-Transport1-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p7'], ('protoss-p7', 'build/StarCraft-Protoss-P7-Transport1-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p6'], ('protoss-p6', 'build/StarCraft-Protoss-P6-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p5-shuttle'],
                         ('protoss-p5-shuttle', 'build/StarCraft-Protoss-P5-Shuttle-GBA.gba'))
        self.assertEqual(dev.GAMES['zerg'], ('zerg', 'build/StarCraft-Zerg-Z7-Transport1-GBA.gba'))
        self.assertEqual(dev.GAMES['zerg-z6'], ('zerg-z6', 'build/StarCraft-Zerg-Z6-GBA.gba'))
        self.assertEqual(dev.GAMES['zerg-z7'], ('zerg-z7', 'build/StarCraft-Zerg-Z7-Transport1-GBA.gba'))
        self.assertEqual(dev.GAMES['zerg-z4'], ('zerg-z4', 'build/StarCraft-Zerg-Z4-GBA.gba'))
        self.assertEqual(dev.GAMES['zerg-z3'], ('zerg-z3', 'build/StarCraft-Zerg-GBA.gba'))
        self.assertEqual(dev.PLAY_SAVE_PROFILES,
                         {'terran': 'terran-transport1', 'zerg': 'zerg-z7-transport1', 'zerg-z6': 'zerg-z6', 'zerg-z7': 'zerg-z7-transport1',
                          'zerg-z6-probe': 'zerg-z6', 'zerg-z4': 'zerg-z4', 'zerg-z3': 'zerg',
                          'protoss': 'protoss-p7-transport1', 'protoss-p7': 'protoss-p7-transport1',
                          'protoss-p6': 'protoss-p6', 'protoss-p3': 'protoss',
                          'protoss-p4-storm': 'protoss-p4-storm',
                          'protoss-p5-shuttle': 'protoss-p5-shuttle'})
        self.assertEqual(sc64_split_smoke.PROFILE_BUILD_PATHS['zerg'],
                         ('StarCraft-Zerg-Z7-Transport1-GBA.gba', 'starcraft-zerg-z7.elf'))
        self.assertEqual(sc64_split_profile.PROFILE_BUILD_PATHS['zerg'],
                         ('StarCraft-Zerg-Z7-Transport1-GBA.gba', 'starcraft-zerg-z7.elf'))
        for routes in (sc64_split_profile.PROFILE_BUILD_PATHS, sc64_split_smoke.PROFILE_BUILD_PATHS):
            self.assertEqual(routes['protoss'],
                             ('StarCraft-Protoss-P7-Transport1-GBA.gba', 'starcraft-protoss-p7.elf'))
        self.assertEqual(sc64_split_smoke.PROFILE_BUILD_PATHS['protoss-p6'],
                         ('StarCraft-Protoss-P6-GBA.gba', 'starcraft-protoss-p6.elf'))

    def test_shared_zerg_fixtures_use_promoted_profile(self):
        makefile = (ROOT / 'Makefile').read_text(encoding='utf-8')
        promotion_makefile = (ROOT / 'tests/sc64-z7-promotion/Makefile').read_text(encoding='utf-8')
        self.assertIn('CANONICAL_ZERG_PROFILE_FLAGS = $(ZERG_Z7_CANONICAL_FLAGS)', promotion_makefile)
        self.assertIn('CANONICAL_ZERG_PROFILE_INPUTS = $(ZERG_Z7_CANONICAL_INPUTS)', promotion_makefile)
        for fixture in ('ORDER_MARKER_GAME_FIXTURE', 'IMPACT_EVENTS_GAME_FIXTURE',
                        'IMPACT_ARM_PROBE', 'UNIT_WALK_GAME_FIXTURE',
                        'SHARED_BUILDER_FIXTURE', 'SKIRMISH_CONTROLLER_PROFILE',
                        'SKIRMISH_AI_PROGRESS_PROFILE', 'DEPOT_APPROACH_GAME_FIXTURE',
                        'OWNER_LIST_FIXTURE', 'TERRAIN_TRANSFER_SCENE',
                        'CANONICAL_SKIRMISH_RESUME_PROFILE'):
            line = next(line for line in makefile.splitlines()
                        if f'call {fixture},zerg,' in line)
            self.assertIn('CANONICAL_ZERG_PROFILE_FLAGS', line, fixture)
            self.assertIn('CANONICAL_ZERG_PROFILE_INPUTS', line, fixture)
        tracer_makefile = (ROOT / 'tests/sc64-z7-promotion/tracer/Makefile').read_text(encoding='utf-8')
        self.assertIn('CANONICAL_TRACER_PROFILE_FIXTURE,zerg-z6,ZERG_ROM_FLAGS,ZERG_Z6_INPUTS', tracer_makefile)
        self.assertIn('build/test-sc64-tracer-profile-zerg: tests/sc64_z7_tracer_game.c', tracer_makefile)
        self.assertIn('include tests/sc64-z7-promotion/Makefile', makefile)
        self.assertIn('include tests/sc64-z7-promotion/tracer/Makefile', makefile)
        self.assertIn('test-sc64-canonical-tracer-profile: $(addprefix build/test-sc64-tracer-profile-,terran zerg protoss showcase)', tracer_makefile)
        self.assertIn('CONSTRUCTION_ARRIVAL_FLAGS_zerg-z6 = $(ZERG_ROM_FLAGS)', makefile)
        self.assertIn('BC_PROFILE_FIXTURE,$(case),zerg,ZERG_ROM_FLAGS,ZERG_Z6_INPUTS', makefile)

    def test_p4_storm_and_legacy_p3_have_explicit_build_and_save_routes(self):
        self.assertEqual(dev.GAMES['protoss-p3'],
                         ('protoss-p3', 'build/StarCraft-Protoss-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p4-storm'],
                         ('protoss-p4-storm', 'build/StarCraft-Protoss-P4-Storm-GBA.gba'))
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss-p4-storm'], 'test-protoss-p4-storm')
        makefile = (ROOT / 'Makefile').read_text(encoding='utf-8')
        p4_test = next(line for line in makefile.splitlines()
                       if line.startswith('test-protoss-p4-storm:'))
        self.assertIn('test-sc64-p4-runtime', p4_test)
        self.assertIn('test-sc64-p4-campaign', p4_test)
        self.assertIn('test-sc64-p4-occupancy-targets', p4_test)
        carried_rule = next(line for line in makefile.splitlines()
                            if line.startswith('build/test-sc64-protoss-carried-campaign:'))
        self.assertIn('$(PROTOSS_CANONICAL_INPUTS)', carried_rule)
        p5_rule = next(line for line in makefile.splitlines()
                       if line.startswith('build/starcraft-protoss-p5-shuttle.elf:'))
        self.assertIn('$(P5_INPUTS)', p5_rule)
        p5_recipe = makefile.splitlines()[makefile.splitlines().index(p5_rule) + 1]
        self.assertIn('$(PROTOSS_P5_CANONICAL_FLAGS)', p5_recipe)
        self.assertNotEqual(dev.GAMES['protoss'][1], dev.GAMES['protoss-p4-storm'][1])
        self.assertEqual(dev.PLAY_SAVE_PROFILES['protoss'], 'protoss-p7-transport1')
        self.assertEqual(dev.GAMES['protoss'], ('protoss', 'build/StarCraft-Protoss-P7-Transport1-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p5-shuttle'],
                         ('protoss-p5-shuttle', 'build/StarCraft-Protoss-P5-Shuttle-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p4-storm'], ('protoss-p4-storm', 'build/StarCraft-Protoss-P4-Storm-GBA.gba'))
        self.assertEqual(dev.PLAY_SAVE_PROFILES.get('protoss-p4-storm', 'protoss-p4-storm'),
                         'protoss-p4-storm')
        self.assertEqual(dev.PLAY_SAVE_PROFILES['protoss-p3'], 'protoss')

    def test_p4_prepare_authenticates_fresh_chapter_inputs_before_build(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rom = root / 'owned-rom.z64'
            rom.write_bytes(b'user supplied fixture')
            calls = []

            def emit(path, text):
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text, encoding='ascii')

            def fake_run(command, **_kwargs):
                calls.append(command)
                script = Path(command[1]).name
                local = root / '.local/sc64'
                if script == 'sc64_workspace.py':
                    emit(local / 'rom/StarCraft64-USA.z64', 'prepared source fixture')
                elif script == 'sc64_races_pack.py':
                    output = (Path(command[command.index('--out') + 1]) if '--out' in command
                              else local / 'races/generated/sc64_races_generated.h')
                    emit(output, ''.join(
                        f'#if defined(SC64_UNIFIED) && defined({gate})\n{payload}\n'
                        f'#endif /* SC64_UNIFIED && {gate} */\n'
                        for gate, payload in (dev.RACE_CAMPAIGN_GATES['protoss-p3'],
                                              dev.RACE_CAMPAIGN_GATES['protoss-p4-storm'])))
                elif script == 'sc64_race_sprites.py':
                    output = (Path(command[command.index('--header') + 1]) if '--header' in command
                              else local / 'races/generated/sc64_race_sprites_generated.h')
                    self.assertIn('--prepack-unit-words', command)
                    emit(output, UNIT_WORD_SPRITE_HEADER)
                elif script == 'sc64_construction_sprites.py':
                    emit_construction_header(root)
                elif script == 'sc64_unit_facing.py':
                    emit_facing_header(root)
                elif script == 'sc64_unit_walk_frames.py':
                    emit_walk_header(root)
                elif script in ('sc64_font_assets.py', 'sc64_icon_assets.py', 'sc64_chrome_assets.py'):
                    names = {'sc64_font_assets.py': 'sc64_font_generated.h',
                             'sc64_icon_assets.py': 'sc64_icons_generated.h',
                             'sc64_chrome_assets.py': 'sc64_chrome_generated.h'}
                    output = root / '.local/sc64/interface/generated' / names[script]
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('source-only interface fixture\n', encoding='ascii')
                elif script == 'sc64_briefing_assets.py':
                    output = local / 'briefings/generated/sc64_briefing_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(BRIEFING_HEADER, encoding='ascii')
                elif script == 'sc64_p4_pack.py':
                    emit(local / 'p4/manifest.json',
                         '{"schema":1,"mission":{"id":"P4","global_catalog_index":23,'
                         '"source_map_path":"008/021"}}')
                elif script == 'sc64_p4_scenario_pack.py':
                    emit(local / 'p4/generated/sc64_p4_scenario_generated.h',
                         P4_SCENARIO_HEADER)

            with patch.object(dev, 'ROOT', root), patch.object(dev, 'ensure_extractor'), \
                    patch.object(dev, 'run', side_effect=fake_run):
                dev.prepare(rom, 'protoss-p4-storm')
            self.assertTrue(all(dev.protoss_p4_input_ready(label, root / relative)
                                for label, relative, _ in dev.PROTOSS_P4_INPUTS))
            self.assertIn('sc64_skirmish_pack.py', [Path(call[1]).name for call in calls])
            self.assertIn('sc64_briefing_assets.py', [Path(call[1]).name for call in calls])
            # Existence is insufficient: stale maps, art, scenarios and source
            # identities must all be rejected before invoking Make.
            for label, relative, _ in dev.PROTOSS_P4_INPUTS:
                path = root / relative
                original = path.read_text()
                path.write_text('stale fixture\n', encoding='ascii')
                with self.subTest(label=label), patch.object(dev, 'ROOT', root), \
                        patch.object(dev, 'run', side_effect=AssertionError('Make must not run')):
                    for game in ('protoss', 'protoss-p4-storm'):
                        with self.assertRaisesRegex(RuntimeError, 'authenticated chapter inputs'):
                            dev.build(game)
                path.write_text(original, encoding='ascii')
            manifest = root / '.local/sc64/p4/manifest.json'
            manifest.write_text(
                '{"schema":1,"mission":{"id":"P4","global_catalog_index":22,'
                '"source_map_path":"008/021"}}', encoding='ascii')
            self.assertFalse(dev.protoss_p4_input_ready(
                'Protoss chapter 4 authenticated source', manifest))

    def test_p4_scenario_readiness_rejects_stale_prefix_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'sc64_p4_scenario_generated.h'
            label = 'Protoss chapter 4 scenario'
            header.write_text(P4_SCENARIO_HEADER, encoding='ascii')
            self.assertTrue(dev.protoss_p4_input_ready(label, header))
            stale_headers = (
                '#define SC64_P4_SCENARIO_GENERATED_H 1\n',
                P4_SCENARIO_HEADER.replace('PREFIX_VERSION 1', 'PREFIX_VERSION 0'),
                P4_SCENARIO_HEADER.replace('PREFIX_VERSION 1', 'PREFIX_VERSION 10'),
                P4_SCENARIO_HEADER.replace('sc64_p4_source_create_prefix', 'old_prefix'),
            )
            for text in stale_headers:
                with self.subTest(header=text):
                    header.write_text(text, encoding='ascii')
                    self.assertFalse(dev.protoss_p4_input_ready(label, header))

    def test_canonical_zerg_readiness_requires_z4_stage_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            emit_facing_header(root)
            markers = {
                '.local/sc64/rom/StarCraft64-USA.z64': 'source ROM fixture',
                '.local/sc64/backwater/generated/sc64_mission_generated.h': 'input',
                '.local/sc64/backwater/generated/sc64_assets_generated.h': 'input',
                '.local/sc64/backwater/generated/sc64_sprites_generated.h': 'input',
                '.local/sc64/races/generated/sc64_races_generated.h':
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\nsc64_race_z3_units\n#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */',
                '.local/sc64/races/generated/sc64_race_sprites_generated.h': 'input',
                '.local/sc64/races/generated/sc64_menu_generated.h': 'input',
                '.local/sc64/briefings/generated/sc64_briefing_generated.h': BRIEFING_HEADER,
                '.local/sc64/interface/generated/sc64_font_generated.h': 'interface fixture',
                '.local/sc64/interface/generated/sc64_icons_generated.h': 'interface fixture',
                '.local/sc64/interface/generated/sc64_chrome_generated.h': 'interface fixture',
                '.local/sc64/construction/generated/sc64_construction_sprites_generated.h': CONSTRUCTION_COLOR_HEADER,
                '.local/sc64/skirmish/generated/sc64_skirmish_generated.h': 'input',
                '.local/sc64/zp-stage/generated/sc64_races_generated.h':
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\nsc64_race_z3_units\n#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n'
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z4_DATA_ENABLED)\nsc64_race_z4_units\n#endif /* SC64_UNIFIED && SC64_Z4_DATA_ENABLED */',
                '.local/sc64/zp-stage/generated/sc64_race_sprites_generated.h':
                    UNIT_WORD_SPRITE_HEADER,
                '.local/sc64/z4/generated/sc64_z4_scenario_generated.h':
                    '#define SC64_Z4_SCENARIO_GENERATED_H 1',
                '.local/sc64/z4/manifest.json':
                    '{"schema":1,"mission":{"id":"Z4","global_catalog_index":15,"source_map_path":"008/017"}}',
                'build/StarCraft-Zerg-Z4-GBA.gba': 'rom',
                dev.DEATH_SOURCE_HEADER: DEATH_SOURCE_HEADER,
            }
            for relative, content in markers.items():
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding='ascii')
            sprite_header = root / '.local/sc64/zp-stage/generated/sc64_race_sprites_generated.h'
            sprite_header.write_text('#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n',
                                     encoding='ascii')
            output = io.StringIO()
            with patch.object(dev, 'ROOT', root), contextlib.redirect_stdout(output):
                self.assertEqual(dev.status('zerg-z4'), 1)
            self.assertIn('MISSING Zerg chapter 4 sprites', output.getvalue())
            self.assertIn('./dev sc64 prepare --rom PATH --profile zerg-z4', output.getvalue())
            sprite_header.write_text(UNIT_WORD_SPRITE_HEADER, encoding='ascii')
            output = io.StringIO()
            death_header = root / dev.DEATH_SOURCE_HEADER
            death_header.unlink()
            with patch.object(dev, 'ROOT', root), contextlib.redirect_stdout(output):
                self.assertEqual(dev.status('zerg'), 1)
            self.assertIn('MISSING Source death art', output.getvalue())
            death_header.write_text(DEATH_SOURCE_HEADER, encoding='ascii')
            output = io.StringIO()
            with patch.object(dev, 'ROOT', root), contextlib.redirect_stdout(output):
                self.assertEqual(dev.status('zerg-z4'), 0)
            self.assertIn('Zerg chapter 4 scenario:', output.getvalue())
            (root / '.local/sc64/z4/manifest.json').write_text(
                '{"schema":1,"mission":{"id":"Z4","global_catalog_index":14,"source_map_path":"008/017"}}',
                encoding='ascii')
            with patch.object(dev, 'ROOT', root), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(dev.status('zerg-z4'), 1)

    @patch.object(dev, "bc_inputs_ready", return_value=True)
    def test_canonical_zerg_requires_chapter_five_and_six_inputs(self, _bc_ready):
        z6_gate = ('#if defined(SC64_UNIFIED) && defined(SC64_Z6_DATA_ENABLED)\nsc64_race_z6_units\n'
                   '#endif /* SC64_UNIFIED && SC64_Z6_DATA_ENABLED */\n')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for label, relative, marker in dev.CANONICAL_ZERG_Z6_INPUTS:
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(f'#define {marker} 1\n', encoding='ascii')
            bank = root / '.local/sc64/zerg-z6/generated/sc64_race_sprites_generated.h'
            label = 'Zerg six-chapter sprites'
            # Markers without a packed payload, or without the Z5 props, fail.
            self.assertFalse(dev.canonical_zerg_z6_input_ready(label, bank))
            bank.write_text(UNIT_WORD_SPRITE_HEADER +
                            '#define SC64_Z6_BUILDING_SPRITE_SET_COMPLETE 1\n', encoding='ascii')
            self.assertFalse(dev.canonical_zerg_z6_input_ready(label, bank))
            bank.write_text(UNIT_WORD_SPRITE_HEADER +
                            '#define SC64_Z6_BUILDING_SPRITE_SET_COMPLETE 1\n'
                            '#define SC64_Z5_DOODAD_SPRITE_SET_COMPLETE 1\n', encoding='ascii')
            self.assertTrue(dev.canonical_zerg_z6_input_ready(label, bank))
            for label, relative, _ in dev.CANONICAL_ZERG_Z6_INPUTS:
                self.assertTrue(dev.canonical_zerg_z6_input_ready(label, root / relative))
            scenario = root / dev.CANONICAL_ZERG_Z6_INPUTS[3][1]
            scenario.write_text('stale', encoding='ascii')
            self.assertFalse(dev.canonical_zerg_z6_input_ready('Zerg chapter 6 scenario', scenario))
            # The build gate also requires the chapter 6 map payload.
            stage = root / '.local/sc64/zp-stage/generated'
            (stage / 'sc64_race_sprites_generated.h').write_text(UNIT_WORD_SPRITE_HEADER,
                                                                 encoding='ascii')
            scenario.write_text('#define SC64_Z6_SOURCE_GLOBAL_ID 17u\n', encoding='ascii')
            (stage / 'sc64_races_generated.h').write_text('no Z6 gate', encoding='ascii')
            with patch.object(dev, 'ROOT', root), patch.object(
                    dev, 'run', side_effect=AssertionError('make must not run')):
                with self.assertRaisesRegex(RuntimeError, 'SC64_Z6_DATA_ENABLED map gate'):
                    dev.build('zerg')
            (stage / 'sc64_races_generated.h').write_text(z6_gate, encoding='ascii')
            with patch.object(dev, 'ROOT', root), patch.object(
                    dev, 'run', side_effect=AssertionError('make must not run')):
                with self.assertRaisesRegex(RuntimeError, 'canonical Zerg Z7'):
                    dev.build('zerg')
            z7_gate = ('#if defined(SC64_UNIFIED) && defined(SC64_Z7_DATA_ENABLED)\nsc64_race_z7_units\n'
                       '#endif /* SC64_UNIFIED && SC64_Z7_DATA_ENABLED */\n')
            (stage / 'sc64_races_generated.h').write_text(z6_gate + z7_gate, encoding='ascii')
            scenario7 = root / dev.STAGED_CAMPAIGN_INPUTS['zerg-z7'][0][1]
            scenario7.parent.mkdir(parents=True, exist_ok=True)
            scenario7.write_text('#define SC64_Z7_SOURCE_GLOBAL_ID 18u\n', encoding='ascii')
            emit_facing_header(root)
            calls = []
            with patch.object(dev, 'ROOT', root), patch.object(
                    dev, 'run', side_effect=lambda command, **_: calls.append(command)):
                dev.build('zerg')
            self.assertEqual(calls, [['make', 'zerg']])

    def test_canonical_zerg_build_rejects_stale_sprite_header_before_make(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / '.local/sc64/zp-stage/generated/sc64_race_sprites_generated.h'
            header.parent.mkdir(parents=True)
            header.write_text('#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n',
                              encoding='ascii')
            with patch.object(dev, 'ROOT', root), patch.object(
                    dev, 'run', side_effect=AssertionError('make must not run')):
                with self.assertRaisesRegex(RuntimeError, 'prepacked unit-word sprite metadata'):
                    dev.build('zerg')

    @patch.object(dev, "bc_inputs_ready", return_value=True)
    def test_terran_build_requires_real_word_metadata_before_make(self, _bc_ready):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / '.local/sc64/terran-late/import-stage/sc64_race_sprites_generated.h'
            header.parent.mkdir(parents=True)
            for stale in ('ordinary indexed sprites',
                          '#define SC64_RACE_UNIT_WORD_SPRITE_METADATA 1\n',
                          UNIT_WORD_SPRITE_HEADER.replace('METADATA 1', 'METADATA 10')):
                header.write_text(stale, encoding='ascii')
                with self.subTest(stale=stale[:50]), patch.object(dev, 'ROOT', root), \
                        patch.object(dev, 'run', side_effect=AssertionError('Make must not run')):
                    with self.assertRaisesRegex(RuntimeError, 'prepacked unit-word sprite metadata'):
                        dev.build('terran')
            header.write_text(UNIT_WORD_SPRITE_HEADER, encoding='ascii')
            emit_facing_header(root)
            with patch.object(dev, 'ROOT', root), patch.object(dev, 'run') as run:
                dev.build('terran')
            run.assert_called_once_with(['make', 'terran'])

    def test_canonical_p5_requires_source_gate_and_prepacked_sprite_payload(self):
        label = 'Protoss chapter 5 sprites'
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'p5_sprites.h'
            header.write_text('#define SC64_Z6_SPRITE_SET_COMPLETE 1\n', encoding='ascii')
            self.assertFalse(dev.staged_campaign_input_ready('protoss-p5-shuttle', label, header))
            stale = UNIT_WORD_SPRITE_HEADER.replace(
                '#define SC64_RACE_UNIT_WORD_SPRITE_METADATA 1',
                '#define SC64_RACE_UNIT_WORD_SPRITE_METADATA 0')
            header.write_text('#define SC64_Z6_SPRITE_SET_COMPLETE 1\n' + stale, encoding='ascii')
            self.assertFalse(dev.staged_campaign_input_ready('protoss-p5-shuttle', label, header))
            header.write_text('#define SC64_Z6_SPRITE_SET_COMPLETE 1\n' + UNIT_WORD_SPRITE_HEADER,
                              encoding='ascii')
            self.assertTrue(dev.staged_campaign_input_ready('protoss-p5-shuttle', label, header))

    def test_canonical_p6_requires_complete_source_and_word_sprite_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for label, relative, marker in dev.STAGED_CAMPAIGN_INPUTS['protoss-p6']:
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                if label == 'Protoss chapter 5 sprites':
                    contents = '#define SC64_Z6_SPRITE_SET_COMPLETE 1\n' + UNIT_WORD_SPRITE_HEADER
                elif label == 'Protoss chapter 6 scenario':
                    contents = ('#define SC64_P6_SOURCE_GLOBAL_ID 25u\n'
                                'static const Sc64P6SourceAction sc64_p6_source_actions[1];\n')
                elif label == 'Protoss chapter 6 abilities':
                    contents = ('#define SC64_P6_ABILITIES_COMPLETE 1u\n'
                                '#define SC64_P6_ABILITIES_SOURCE_GLOBAL_ID 25u\n')
                elif label == 'Protoss chapter 6 doodads':
                    contents = ('#define SC64_P6_DOODAD_SOURCE_COUNT 27u\n'
                                'static const Sc64P6Doodad sc64_p6_doodads[27];\n')
                elif label == 'Protoss chapter 6 sprites':
                    contents = '#define SC64_P6_SPRITE_SET_COMPLETE 1\n' + UNIT_WORD_SPRITE_HEADER
                else:
                    contents = f'#define {marker} 1\n' if marker else 'maps\n'
                path.write_text(contents, encoding='ascii')
                self.assertTrue(dev.staged_campaign_input_ready('protoss-p6', label, path), label)

            p6_sprite = root / dev.STAGED_CAMPAIGN_INPUTS['protoss-p6'][-1][1]
            p6_sprite.write_text('#define SC64_P6_SPRITE_SET_COMPLETE 1\n', encoding='ascii')
            self.assertFalse(dev.staged_campaign_input_ready(
                'protoss-p6', 'Protoss chapter 6 sprites', p6_sprite))
            p6_sprite.write_text('#define SC64_P6_SPRITE_SET_COMPLETE 1\n' +
                                 UNIT_WORD_SPRITE_HEADER, encoding='ascii')
            self.assertTrue(dev.staged_campaign_input_ready(
                'protoss-p6', 'Protoss chapter 6 sprites', p6_sprite))

            scenario = root / dev.STAGED_CAMPAIGN_INPUTS['protoss-p6'][5][1]
            scenario.write_text('#define SC64_P6_SOURCE_GLOBAL_ID 24u\n'
                                'static const Sc64P6SourceAction sc64_p6_source_actions[1];\n',
                                encoding='ascii')
            self.assertFalse(dev.staged_campaign_input_ready(
                'protoss-p6', 'Protoss chapter 6 scenario', scenario))

    def test_staged_p6_preparation_uses_existing_private_packers_and_actor_bank(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rom = root / 'prepared-usa.z64'
            rom.write_bytes(b'fixture')
            calls = []
            with patch.object(dev, 'ROOT', root), patch.object(
                    dev, 'run', side_effect=lambda command, **_: calls.append(command)):
                dev.prepare_staged_campaign('protoss-p6', rom)
            scripts = [Path(command[1]).name for command in calls]
            self.assertEqual(scripts, [
                'sc64_zp_scenario_pack.py', 'sc64_p6_ability_pack.py',
                'sc64_p6_doodad_pack.py', 'sc64_race_sprites.py'])
            self.assertIn('--mission', calls[0])
            self.assertIn('P6', calls[0])
            self.assertIn('--header', calls[2])
            self.assertIn('--include-p6-actors', calls[3])
            self.assertIn('--prepack-unit-words', calls[3])
            self.assertEqual(calls[3][calls[3].index('--header') + 1],
                             str(root / '.local/sc64/protoss-p6/generated/sc64_race_sprites_generated.h'))

    def test_canonical_p6_make_flags_pin_validated_queries_against_probe_overrides(self):
        make_snippet = ('.PHONY: print-p6-flags\n'
                        'print-p6-flags:\n'
                        "\t@printf '%s\\n' '$(PROTOSS_P6_CANONICAL_FLAGS)'\n")
        overrides = {
            'P5_QUERY_FLAGS': '-DP5_QUERY_OVERRIDE_SENTINEL',
            'P6_QUERY_FLAGS': '-DP6_QUERY_OVERRIDE_SENTINEL',
            'P6_EXTRA_FLAGS': '-DP6_EXTRA_OVERRIDE_SENTINEL',
        }
        command = ['make', '-s', '-f', 'Makefile', '-f', '-', 'print-p6-flags']
        command.extend(f'{name}={value}' for name, value in overrides.items())
        result = subprocess.run(command, cwd=ROOT, input=make_snippet, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
        evaluated_flags = result.stdout.strip().split()
        evaluated = set(evaluated_flags)
        for flag in (
                '-DSC64_P5_TYPE_FIRST_COUNTS', '-DSC64_P5_PRUNE_STARTED',
                '-DSC64_P5_COUNT_SNAPSHOT', '-DSC64_P6_PRUNE_STARTED',
                '-DSC64_P6_PREVALIDATED_TICK', '-DSC64_P6_TRAP_CULL',
                '-DSC64_P6_COUNT_SNAPSHOT', '-DSC64_P6_BURROW_CULL',
                '-DSC64_P6_BURROW_GROUPED', '-DSC64_P6_RESCUE_GROUPED',
                '-DSC64_P6_STORM_POSTCHECK_LOCAL',
        ):
            with self.subTest(required_flag=flag):
                self.assertIn(flag, evaluated)
        for flag in overrides.values():
            with self.subTest(probe_override=flag):
                self.assertNotIn(flag, evaluated)

    def test_staged_z6_and_p5_profiles_have_explicit_build_and_test_routes(self):
        self.assertEqual(dev.GAMES['zerg-z6'],
                         ('zerg-z6', 'build/StarCraft-Zerg-Z6-GBA.gba'))
        self.assertEqual(dev.GAMES['zerg-z6-probe'],
                         ('zerg-z6-probe', 'build/StarCraft-Zerg-Z6-Probe-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p5-shuttle'],
                         ('protoss-p5-shuttle', 'build/StarCraft-Protoss-P5-Shuttle-GBA.gba'))
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg-z6-probe'], (
            'test-sc64-z6-runtime', 'test-sc64-z6-save', 'test-sc64-z6-profile',
            'test-sc64-z6-frame-coverage', 'test-sc64-z6-controller'))
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss-p5-shuttle'],
                         ('test-sc64-p5-shuttle-runtime', 'test-sc64-p5-campaign-run'))
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss'], 'test-protoss')
        makefile = (ROOT / 'Makefile').read_text(encoding='utf-8')
        self.assertIn(
            'ZERG_Z7_CANONICAL_FLAGS = -DSC64_Z7_VISIBILITY_ROW_CONTEXT '
            '-DSC64_Z7_VISIBILITY_PHASE_REJECT '
            '$(filter-out $(Z7_STEERING_OPTIONS),$(Z7_STEERING_FLAGS)) '
            '-DSC64_CONTACT_CACHE -DSC64_CONTACT_STOCK_ROM '
            '-DSC64_REACHABLE_FORMATION -DSC64_Z7_PRUNE_STARTED -DSC64_ROUTE_DECAY_FREQ',
            makefile)
        z8_flags = next(line for line in makefile.splitlines()
                        if line.startswith('ZERG_Z8_CANONICAL_FLAGS ='))
        self.assertIn('-DSC64_CONTACT_STEER_ROM', z8_flags)
        self.assertIn('ZERG_Z7_RELEASE_FLAGS = $(ZERG_Z7_CANONICAL_FLAGS) -DGBA_NO_DEBUG_INPUTS',
                      makefile)
        self.assertIn('ZERG_Z7_CANONICAL_FLAGS) $(3) $$< common/gba.c -o $$@', makefile)
        self.assertIn('$(CC) $(CFLAGS) $(ZERG_Z7_RELEASE_FLAGS)', makefile)
        self.assertIn('Z7_STEERING_FLAGS = $(Z7_FULL_RENDER_FLAGS) $(Z7_STEERING_OPTIONS)',
                      makefile)
        for fixture in (
            '$(eval $(call Z7_CANONICAL_FIXTURE,carried-z5,sc64_z5_campaign_run.c))',
            '$(eval $(call Z7_CANONICAL_FIXTURE,carried-z6,sc64_z6_campaign_run.c))',
            '$(eval $(call Z7_CANONICAL_FIXTURE,z6-controller-policy,sc64_z6_controller_policy.c))',
        ):
            self.assertIn(fixture, makefile)
        canonical_z7_target = next(
            line for line in makefile.splitlines() if line.startswith('test-zerg-z7:'))
        self.assertIn('test-sc64-z7-canonical-carried-z5', canonical_z7_target)
        self.assertIn('test-sc64-z7-canonical-carried-z6', canonical_z7_target)
        self.assertNotIn('test-sc64-z7-carried-z5', canonical_z7_target)
        self.assertNotIn('test-sc64-z7-carried-z6', canonical_z7_target)
        self.assertIn('Z7_EGG_PROBE_FLAGS := $(Z7_PROBE_FLAGS) -DSC64_CONSTRUCTION_HP -DSC64_DIRECT_ATTACK -DSC64_Z7_EGG_PRODUCTION',
                      makefile)
        self.assertIn('Z7_FULL_RENDER_FLAGS =', makefile)
        self.assertIn('build/StarCraft-Zerg-Z7-Transport1-GBA.gba', makefile)
        self.assertIn("'SC ZERG Z7' SCZ7", makefile)
        self.assertIn('test-sc64-z7-canonical-skirmish', makefile)
        self.assertIn('test-zerg: test-zerg-z7', makefile)
        self.assertIn('test-zerg-z6: build/test-zerg build/test-sc64-zerg-promoted-profile', makefile)
        self.assertIn('PROTOSS_P5_CANONICAL_FLAGS = $(P4_CREATE_PREFIX_FLAGS) $(P5_SHUTTLE_FLAGS)', makefile)
        self.assertIn('test-protoss: ', makefile)
        canonical_test = next(line for line in makefile.splitlines() if line.startswith('test-protoss:'))
        self.assertIn('test-sc64-p5-canonical', canonical_test)
        self.assertIn('build/test-sc64-p5-canonical-campaign-run', makefile)

    def test_z7_candidate_has_separate_generation_and_scenario_route(self):
        self.assertIn('zerg-z7', dev.SC64_PROFILES)
        self.assertEqual(dev.PLAY_SAVE_PROFILES['zerg-z7'], 'zerg-z7-transport1')
        self.assertEqual(dev.RACE_CAMPAIGN_GATES['zerg-z7'],
                         ('SC64_Z7_DATA_ENABLED', 'sc64_race_z7_units'))
        self.assertEqual(dev.STAGED_CAMPAIGN_INPUTS['zerg-z7'], (
            ('Zerg chapter 7 scenario',
             '.local/sc64/zp-stage/generated/scenarios/sc64_z7_scenario_generated.h',
             'SC64_Z7_SOURCE_GLOBAL_ID'),
        ))

    def test_staged_campaign_preparation_uses_existing_private_packers(self):
        for profile, expected in (
            ('zerg-z6', {
                'sc64_z4_pack.py', 'sc64_z4_scenario_pack.py',
                'sc64_zp_scenario_pack.py:Z5', 'sc64_zp_scenario_pack.py:Z6',
                'sc64_z5_doodad_pack.py', 'sc64_z5_ability_pack.py',
                'sc64_z6_transport_pack.py', 'sc64_z6_ability_pack.py',
                'sc64_race_sprites.py:z5', 'sc64_race_sprites.py:z6',
            }),
            ('protoss-p5-shuttle', {
                'sc64_p4_pack.py', 'sc64_p4_scenario_pack.py',
                'sc64_zp_scenario_pack.py:P5', 'sc64_race_sprites.py:z6',
            }),
        ):
            calls = []
            private_rom = Path('/private/StarCraft64-USA.z64')
            with self.subTest(profile=profile), patch.object(dev, 'run',
                                                              side_effect=lambda cmd, **_kw: calls.append(cmd)):
                dev.prepare_staged_campaign(profile, private_rom)
            observed = set()
            for command in calls:
                name = Path(command[1]).name
                mission = command[command.index('--mission') + 1] if '--mission' in command else None
                if name == 'sc64_race_sprites.py':
                    bank = 'z6' if '--include-z6-actors' in command else 'z5'
                    observed.add(f'{name}:{bank}')
                else:
                    observed.add(f'{name}:{mission}' if mission else name)
            self.assertEqual(observed, expected)
            p5_sprite_commands = [command for command in calls if Path(command[1]).name == 'sc64_race_sprites.py' and '--include-z6-actors' in command]
            if profile == 'protoss-p5-shuttle':
                self.assertTrue(all('--prepack-unit-words' in command for command in p5_sprite_commands))
            self.assertTrue(all('/private/StarCraft64-USA.z64' not in command
                                for command in calls if Path(command[1]).name not in
                                ('sc64_z4_pack.py', 'sc64_p4_pack.py', 'sc64_race_sprites.py')))

    def test_staged_make_inputs_are_produced_from_empty_campaign_dirs(self):
        expected = {
            'zerg-z6': {
                '.local/sc64/zp-stage/generated/sc64_races_generated.h',
                '.local/sc64/zp-stage/generated/sc64_race_sprites_generated.h',
                '.local/sc64/z4/generated/sc64_z4_scenario_generated.h',
                '.local/sc64/z5/generated/sc64_race_sprites_generated.h',
                '.local/sc64/zp-stage/generated/scenarios/sc64_z5_scenario_generated.h',
                '.local/sc64/zp-stage/generated/scenarios/sc64_z5_doodads_generated.h',
                '.local/sc64/zp-stage/generated/scenarios/sc64_z5_abilities_generated.h',
                '.local/sc64/zp-stage/generated/scenarios/sc64_z6_scenario_generated.h',
                '.local/sc64/zp-stage/generated/scenarios/sc64_z6_transport_generated.h',
                '.local/sc64/zp-stage/generated/scenarios/sc64_z6_abilities_generated.h',
                '.local/sc64/z6/generated/sc64_race_sprites_generated.h',
            },
            'protoss-p5-shuttle': {
                '.local/sc64/zp-stage/generated/sc64_races_generated.h',
                '.local/sc64/zp-stage/generated/sc64_race_sprites_generated.h',
                '.local/sc64/p4/generated/sc64_p4_scenario_generated.h',
                '.local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h',
                '.local/sc64/p5/generated/sc64_race_sprites_generated.h',
            },
        }
        private_rom = Path('/private/StarCraft64-USA.z64')
        for profile, required in expected.items():
            with self.subTest(profile=profile), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                # Inputs produced by the shared race import phase.
                stage = root / '.local/sc64/zp-stage/generated'
                stage.mkdir(parents=True)
                gate, payload = dev.RACE_CAMPAIGN_GATES[profile]
                (stage / 'sc64_races_generated.h').write_text(
                    f'#if defined(SC64_UNIFIED) && defined({gate})\n{payload}\n'
                    f'#endif /* SC64_UNIFIED && {gate} */\n', encoding='ascii')
                (stage / 'sc64_race_sprites_generated.h').write_text(
                    '#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n', encoding='ascii')
                markers_by_path = {}
                for _, relative, marker in dev.STAGED_CAMPAIGN_INPUTS[profile]:
                    if marker:
                        markers_by_path.setdefault(relative, set()).add(marker)
                calls = []

                def fake_pack(command, **_kwargs):
                    calls.append(command)
                    script = Path(command[1]).name
                    output = None
                    mission = command[command.index('--mission') + 1] if '--mission' in command else None
                    if '--header' in command:
                        output = Path(command[command.index('--header') + 1])
                    elif script == 'sc64_z4_scenario_pack.py':
                        output = root / '.local/sc64/z4/generated/sc64_z4_scenario_generated.h'
                    elif script == 'sc64_p4_scenario_pack.py':
                        output = root / '.local/sc64/p4/generated/sc64_p4_scenario_generated.h'
                    elif script == 'sc64_zp_scenario_pack.py':
                        output = root / f'.local/sc64/zp-stage/generated/scenarios/sc64_{mission.lower()}_scenario_generated.h'
                    elif script == 'sc64_z5_doodad_pack.py':
                        output = root / '.local/sc64/zp-stage/generated/scenarios/sc64_z5_doodads_generated.h'
                    elif script == 'sc64_z5_ability_pack.py':
                        output = root / '.local/sc64/zp-stage/generated/scenarios/sc64_z5_abilities_generated.h'
                    elif script == 'sc64_z6_transport_pack.py':
                        output = root / '.local/sc64/zp-stage/generated/scenarios/sc64_z6_transport_generated.h'
                    elif script == 'sc64_z6_ability_pack.py':
                        output = root / '.local/sc64/zp-stage/generated/scenarios/sc64_z6_abilities_generated.h'
                    if output is not None:
                        output.parent.mkdir(parents=True, exist_ok=True)
                        marker_set = markers_by_path.get(str(output.relative_to(root)), set())
                        content = '\n'.join(f'#define {marker} 1' for marker in marker_set) + '\n'
                        if relative.endswith('/p5/generated/sc64_race_sprites_generated.h'):
                            content += UNIT_WORD_SPRITE_HEADER
                        output.write_text(content, encoding='ascii')

                with patch.object(dev, 'ROOT', root), patch.object(dev, 'run', side_effect=fake_pack):
                    dev.prepare_staged_campaign(profile, private_rom)

                listed = {relative for _, relative, _ in dev.STAGED_CAMPAIGN_INPUTS[profile]}
                self.assertEqual(listed, required)
                for relative in required:
                    path = root / relative
                    self.assertTrue(path.is_file(), f'preparation omitted Make input {relative}')
                    label = next(label for label, item, _ in dev.STAGED_CAMPAIGN_INPUTS[profile]
                                 if item == relative)
                    self.assertTrue(dev.staged_campaign_input_ready(profile, label, path), relative)

    def test_full_staged_prepare_emits_all_base_and_chapter_make_inputs(self):
        base_inputs = {
            '.local/sc64/backwater/generated/sc64_mission_generated.h',
            '.local/sc64/backwater/generated/sc64_assets_generated.h',
            '.local/sc64/backwater/generated/sc64_sprites_generated.h',
            '.local/sc64/races/generated/sc64_races_generated.h',
            '.local/sc64/races/generated/sc64_race_sprites_generated.h',
            '.local/sc64/races/generated/sc64_menu_generated.h',
            '.local/sc64/briefings/generated/sc64_briefing_generated.h',
            '.local/sc64/interface/generated/sc64_font_generated.h',
            '.local/sc64/interface/generated/sc64_icons_generated.h',
            '.local/sc64/interface/generated/sc64_chrome_generated.h',
            '.local/sc64/skirmish/generated/sc64_skirmish_generated.h',
        }
        for profile in ('zerg-z6', 'zerg-z7', 'protoss-p5-shuttle'):
            with self.subTest(profile=profile), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                rom = root / 'user-rom.z64'
                rom.write_bytes(b'owned source fixture')
                calls = []

                def fake_run(command, **_kwargs):
                    calls.append(command)
                    script = Path(command[1]).name
                    if script == 'sc64_workspace.py':
                        for relative in (
                            '.local/sc64/rom/StarCraft64-USA.z64',
                            '.local/sc64/backwater/generated/sc64_mission_generated.h',
                            '.local/sc64/backwater/generated/sc64_assets_generated.h',
                            '.local/sc64/backwater/generated/sc64_sprites_generated.h',
                        ):
                            path = root / relative
                            path.parent.mkdir(parents=True, exist_ok=True)
                            path.write_text('prepared\n', encoding='ascii')
                    elif script == 'sc64_races_pack.py':
                        output = (Path(command[command.index('--out') + 1]) if '--out' in command
                                  else root / '.local/sc64/races/generated/sc64_races_generated.h')
                        output.parent.mkdir(parents=True, exist_ok=True)
                        if output.parent == root / '.local/sc64/zp-stage/generated':
                            gate_profiles = (('zerg', 'zerg-z4', 'zerg-z6', 'zerg-z7')
                                             if profile.startswith('zerg') else (profile,))
                            text = ''.join(
                                f'#if defined(SC64_UNIFIED) && defined({dev.RACE_CAMPAIGN_GATES[item][0]})\n'
                                f'{dev.RACE_CAMPAIGN_GATES[item][1]}\n'
                                f'#endif /* SC64_UNIFIED && {dev.RACE_CAMPAIGN_GATES[item][0]} */\n'
                                for item in gate_profiles)
                            output.write_text(text, encoding='ascii')
                        elif profile in ('zerg-z6', 'zerg-z7'):
                            gate, payload = dev.RACE_CAMPAIGN_GATES['zerg']
                            output.write_text(f'#if defined(SC64_UNIFIED) && defined({gate})\n'
                                              f'{payload}\n#endif /* SC64_UNIFIED && {gate} */\n',
                                              encoding='ascii')
                        else:
                            output.write_text('race map payload\n', encoding='ascii')
                    elif script == 'sc64_race_sprites.py':
                        if '--header' in command:
                            output = Path(command[command.index('--header') + 1])
                        else:
                            output = root / '.local/sc64/races/generated/sc64_race_sprites_generated.h'
                        output.parent.mkdir(parents=True, exist_ok=True)
                        markers = set()
                        if '--include-zp-campaign-actors' in command:
                            markers.add('SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE')
                        if '--include-z5-actors' in command:
                            markers.add('SC64_Z5_SPRITE_SET_COMPLETE')
                        if '--include-z5-doodads' in command:
                            markers.add('SC64_Z5_DOODAD_SPRITE_SET_COMPLETE')
                        if '--include-z6-actors' in command:
                            markers.add('SC64_Z6_SPRITE_SET_COMPLETE')
                        if '--include-z6-buildings' in command:
                            markers.add('SC64_Z6_BUILDING_SPRITE_SET_COMPLETE')
                        content = '\n'.join(f'#define {marker} 1' for marker in markers) + '\n'
                        if '--prepack-unit-words' in command:
                            content += UNIT_WORD_SPRITE_HEADER
                        output.write_text(content, encoding='ascii')
                    elif script == 'sc64_construction_sprites.py':
                        emit_construction_header(root)
                    elif script == 'sc64_unit_facing.py':
                        emit_facing_header(root)
                    elif script == 'sc64_unit_walk_frames.py':
                        emit_walk_header(root)
                    elif script == 'sc64_unit_attack_sprites.py':
                        emit_attack_pose_header(root)
                    elif script == 'sc64_weapon_source_sprites.py':
                        emit_weapon_source_header(root)
                    elif script == 'sc64_death_sprites.py':
                        emit_death_source_header(root)
                    elif script == 'sc64_menu_assets.py':
                        output = root / '.local/sc64/races/generated/sc64_menu_generated.h'
                        output.parent.mkdir(parents=True, exist_ok=True)
                        output.write_text('menu\n', encoding='ascii')
                    elif script in ('sc64_font_assets.py', 'sc64_icon_assets.py', 'sc64_chrome_assets.py'):
                        names = {'sc64_font_assets.py': 'sc64_font_generated.h',
                                 'sc64_icon_assets.py': 'sc64_icons_generated.h',
                                 'sc64_chrome_assets.py': 'sc64_chrome_generated.h'}
                        output = root / '.local/sc64/interface/generated' / names[script]
                        output.parent.mkdir(parents=True, exist_ok=True)
                        output.write_text('source-only interface fixture\n', encoding='ascii')
                    elif script == 'sc64_briefing_assets.py':
                        output = root / '.local/sc64/briefings/generated/sc64_briefing_generated.h'
                        output.parent.mkdir(parents=True, exist_ok=True)
                        output.write_text(BRIEFING_HEADER, encoding='ascii')
                    elif script == 'sc64_skirmish_pack.py':
                        output = root / '.local/sc64/skirmish/generated/sc64_skirmish_generated.h'
                        output.parent.mkdir(parents=True, exist_ok=True)
                        output.write_text('skirmish\n', encoding='ascii')
                    elif script in ('sc64_z4_scenario_pack.py', 'sc64_p4_scenario_pack.py'):
                        race = 'z4' if 'z4' in script else 'p4'
                        path = root / f'.local/sc64/{race}/generated/sc64_{race}_scenario_generated.h'
                        path.parent.mkdir(parents=True, exist_ok=True)
                        guard = 'SC64_Z4_SCENARIO_GENERATED_H' if race == 'z4' else 'SC64_P4_SCENARIO_GENERATED_H'
                        path.write_text(f'#define {guard} 1\n', encoding='ascii')
                    else:
                        mission = command[command.index('--mission') + 1] if '--mission' in command else None
                        outputs = {
                            'Z5': '.local/sc64/zp-stage/generated/scenarios/sc64_z5_scenario_generated.h',
                            'Z6': '.local/sc64/zp-stage/generated/scenarios/sc64_z6_scenario_generated.h',
                            'Z7': '.local/sc64/zp-stage/generated/scenarios/sc64_z7_scenario_generated.h',
                            'P5': '.local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h',
                        }
                        relative = outputs.get(mission)
                        if script == 'sc64_z5_doodad_pack.py':
                            relative = '.local/sc64/zp-stage/generated/scenarios/sc64_z5_doodads_generated.h'
                        elif script == 'sc64_z5_ability_pack.py':
                            relative = '.local/sc64/zp-stage/generated/scenarios/sc64_z5_abilities_generated.h'
                        elif script == 'sc64_z6_transport_pack.py':
                            relative = '.local/sc64/zp-stage/generated/scenarios/sc64_z6_transport_generated.h'
                        elif script == 'sc64_z6_ability_pack.py':
                            relative = '.local/sc64/zp-stage/generated/scenarios/sc64_z6_abilities_generated.h'
                        if relative:
                            path = root / relative
                            path.parent.mkdir(parents=True, exist_ok=True)
                            marker_sets = [dev.STAGED_CAMPAIGN_INPUTS[profile]]
                            if profile == 'zerg-z7':
                                marker_sets.append(dev.STAGED_CAMPAIGN_INPUTS['zerg-z6'])
                            marker = next((marker for items in marker_sets for _, item, marker in items
                                           if item == relative and marker), 'numeric payload')
                            path.write_text(f'#define {marker} 1\n', encoding='ascii')

                with patch.object(dev, 'ROOT', root), patch.object(dev, 'ensure_extractor'), \
                        patch.object(dev, 'run', side_effect=fake_run):
                    dev.prepare(rom, profile)

                # Preparing a later staged profile rewrites the same shared
                # bank as canonical Zerg; it must not erase the packed data.
                shared = root / '.local/sc64/zp-stage/generated/sc64_race_sprites_generated.h'
                self.assertTrue(dev.canonical_zerg_input_ready('Zerg chapter 4 sprites', shared))
                base = root / '.local/sc64/races/generated/sc64_race_sprites_generated.h'
                self.assertIn('SC64_RACE_UNIT_WORD_SPRITE_METADATA 1', base.read_text())

                produced = set()
                for _, relative, _ in dev.STAGED_CAMPAIGN_INPUTS[profile]:
                    produced.add(relative)
                produced |= base_inputs
                self.assertIn('sc64_briefing_assets.py', [Path(call[1]).name for call in calls])
                if profile in ('zerg-z6', 'zerg-z7'):
                    produced.add('.local/sc64/zp-stage/generated/scenarios/sc64_z5_doodads_generated.h')
                    produced |= {relative for _, relative, _ in dev.CANONICAL_ZERG_Z6_INPUTS}
                if profile == 'zerg-z7':
                    produced.add('.local/sc64/zp-stage/generated/scenarios/sc64_z7_scenario_generated.h')
                for relative in produced:
                    self.assertTrue((root / relative).is_file(),
                                    f'{profile} fresh prepare omitted Make input {relative}')

    def test_staged_profile_status_checks_gates_and_required_headers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for profile in ('zerg-z6', 'protoss-p5-shuttle'):
                with self.subTest(profile=profile):
                    for _, relative, marker in dev.STAGED_CAMPAIGN_INPUTS[profile]:
                        path = root / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_text((marker + '\n') if marker else 'base input\n',
                                        encoding='ascii')
                    for relative in (
                        '.local/sc64/races/generated/sc64_races_generated.h',
                        '.local/sc64/zp-stage/generated/sc64_races_generated.h',
                    ):
                        path = root / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        gate, payload = dev.RACE_CAMPAIGN_GATES[profile]
                        path.write_text(f'#if defined(SC64_UNIFIED) && defined({gate})\n'
                                        f'{payload}\n#endif /* SC64_UNIFIED && {gate} */\n',
                                        encoding='ascii')
                    for relative in (
                        '.local/sc64/rom/StarCraft64-USA.z64',
                        '.local/sc64/backwater/generated/sc64_mission_generated.h',
                        '.local/sc64/backwater/generated/sc64_assets_generated.h',
                        '.local/sc64/backwater/generated/sc64_sprites_generated.h',
                        '.local/sc64/briefings/generated/sc64_briefing_generated.h',
                        '.local/sc64/interface/generated/sc64_font_generated.h',
                        '.local/sc64/interface/generated/sc64_icons_generated.h',
                        '.local/sc64/interface/generated/sc64_chrome_generated.h',
                        '.local/sc64/skirmish/generated/sc64_skirmish_generated.h',
                    ):
                        path = root / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_text(BRIEFING_HEADER if 'briefing' in relative else 'input\n',
                                        encoding='ascii')
                    games = dict(dev.GAMES, **{profile: (profile, 'build/staged.gba')})
                    output = io.StringIO()
                    with patch.object(dev, 'ROOT', root), patch.object(dev, 'GAMES', games), \
                            contextlib.redirect_stdout(output):
                        self.assertEqual(dev.status(profile), 1)  # staged data is ready; ROM is not built
                    self.assertNotIn('MISSING Zerg chapter 6 scenario', output.getvalue())
                    self.assertNotIn('MISSING Protoss chapter 5 scenario', output.getvalue())
                    self.assertIn('MISSING Built ROM', output.getvalue())

                    # A malformed compile gate must make readiness fail even if the file exists.
                    gate_path = root / '.local/sc64/zp-stage/generated/sc64_races_generated.h'
                    gate_path.write_text('descriptor without data guard\n', encoding='ascii')
                    with patch.object(dev, 'ROOT', root), patch.object(dev, 'GAMES', games), \
                            contextlib.redirect_stdout(io.StringIO()):
                        self.assertEqual(dev.status(profile), 1)

    def test_race_profile_status_requires_its_compile_gated_map_data(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            race_header = root / '.local/sc64/races/generated/sc64_races_generated.h'
            race_header.parent.mkdir(parents=True)
            race_header.write_text('#if defined(SC64_UNIFIED) && defined(SC64_P3_ENABLED)\n'
                                    'sc64_race_p3_units\n'
                                    '#endif /* SC64_UNIFIED && SC64_P3_ENABLED */\n',
                                   encoding='utf-8')
            output = io.StringIO()
            games = dict(dev.GAMES, zerg=('zerg', 'build/zerg.gba'))
            with patch.object(dev, 'ROOT', root), patch.object(dev, 'GAMES', games), \
                 contextlib.redirect_stdout(output):
                status = dev.status('zerg')
            self.assertEqual(status, 1)
            self.assertIn('MISSING Zerg chapter 3 gated map data', output.getvalue())

    def test_canonical_zerg_map_requires_both_z3_and_z4_payload_guards(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'sc64_races_generated.h'
            label = 'Zerg chapter 4 maps'
            header.write_text(
                '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                'sc64_race_z3_units\n'
                '#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n', encoding='ascii')
            self.assertFalse(dev.canonical_zerg_input_ready(label, header))

            header.write_text(
                '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                'sc64_race_z3_units\n'
                '#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n'
                '#if defined(SC64_UNIFIED)\n'
                'sc64_race_z4_units\n'
                '#endif /* SC64_UNIFIED && SC64_Z4_DATA_ENABLED */\n', encoding='ascii')
            self.assertFalse(dev.canonical_zerg_input_ready(label, header))

            header.write_text(
                '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                'sc64_race_z3_units\n'
                '#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n'
                '#if defined(SC64_UNIFIED) && defined(SC64_Z4_DATA_ENABLED)\n'
                'sc64_race_z4_units\n'
                '#endif /* SC64_UNIFIED && SC64_Z4_DATA_ENABLED */\n', encoding='ascii')
            self.assertTrue(dev.canonical_zerg_input_ready(label, header))

    def test_canonical_zerg_sprite_readiness_requires_word_metadata_and_payload(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'sc64_race_sprites_generated.h'
            label = 'Zerg chapter 4 sprites'
            header.write_text(
                '#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n', encoding='ascii')
            self.assertFalse(dev.canonical_zerg_input_ready(label, header))
            header.write_text(
                '#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n'
                'static inline const GbaSpriteWordFrame *sc64_race_unit_words_sprite(void) { }\n',
                encoding='ascii')
            self.assertFalse(dev.canonical_zerg_input_ready(label, header))
            header.write_text(UNIT_WORD_SPRITE_HEADER.replace(
                '#define SC64_RACE_UNIT_WORD_SPRITE_METADATA 1\n',
                '#define SC64_RACE_UNIT_WORD_SPRITE_METADATA 0\n'), encoding='ascii')
            self.assertFalse(dev.canonical_zerg_input_ready(label, header))
            header.write_text(UNIT_WORD_SPRITE_HEADER, encoding='ascii')
            self.assertTrue(dev.canonical_zerg_input_ready(label, header))

    def test_death_source_readiness_requires_versioned_nonempty_bank(self):
        with tempfile.TemporaryDirectory() as directory:
            header = Path(directory) / 'sc64_death_sprites_generated.h'
            self.assertFalse(dev.death_source_input_ready(header))
            header.write_text(DEATH_SOURCE_HEADER.replace(
                '#define SC64_DEATH_SOURCE_METADATA_VERSION 1u\n', ''), encoding='ascii')
            self.assertFalse(dev.death_source_input_ready(header))
            header.write_text(DEATH_SOURCE_HEADER.replace(
                '#define SC64_DEATH_SOURCE_PIXEL_BYTES 1u',
                '#define SC64_DEATH_SOURCE_PIXEL_BYTES 0u'), encoding='ascii')
            self.assertFalse(dev.death_source_input_ready(header))
            header.write_text(DEATH_SOURCE_HEADER.replace(
                '#define SC64_DEATH_SOURCE_ACTOR_COUNT 1u',
                '#define SC64_DEATH_SOURCE_ACTOR_COUNT 0u'), encoding='ascii')
            self.assertFalse(dev.death_source_input_ready(header))
            header.write_text(DEATH_SOURCE_HEADER.replace(
                '#define SC64_DEATH_SOURCE_WORD_METADATA 1u\n', ''), encoding='ascii')
            self.assertFalse(dev.death_source_input_ready(header))
            header.write_text(DEATH_SOURCE_HEADER.replace(
                '#define SC64_DEATH_SOURCE_WORD_BYTES 1u',
                '#define SC64_DEATH_SOURCE_WORD_BYTES 0u'), encoding='ascii')
            self.assertFalse(dev.death_source_input_ready(header))
            header.write_text(DEATH_SOURCE_HEADER.replace(
                'sc64_death_image_word_frame(', 'sc64_death_image_frame('), encoding='ascii')
            self.assertFalse(dev.death_source_input_ready(header))
            for phase in range(4):
                array = f'sc64_death_image_1_frame_0_words_p{phase}[1]'
                header.write_text(DEATH_SOURCE_HEADER.replace(
                    array, array.replace('[1]', '[0]')), encoding='ascii')
                self.assertFalse(dev.death_source_input_ready(header),
                                 f'empty packed phase p{phase} passed readiness')
                header.write_text(DEATH_SOURCE_HEADER.replace(
                    f'static const u32 sc64_death_image_1_frame_0_words_p{phase}[1] = {{1}};\n',
                    ''), encoding='ascii')
                self.assertFalse(dev.death_source_input_ready(header),
                                 f'missing packed phase p{phase} passed readiness')
            header.write_text(DEATH_SOURCE_HEADER, encoding='ascii')
            self.assertTrue(dev.death_source_input_ready(header))

    @patch.object(dev, "bc_inputs_ready", return_value=True)
    def test_canonical_race_build_requires_current_death_bank(self, _bc_ready):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / '.local/sc64/terran-late/import-stage').mkdir(parents=True)
            (root / '.local/sc64/terran-late/import-stage/sc64_race_sprites_generated.h').write_text(
                UNIT_WORD_SPRITE_HEADER, encoding='ascii')
            for relative, content in (
                (dev.FACING_HEADER, FACING_SPRITE_HEADER),
                (dev.WALK_HEADER, WALK_CYCLE_HEADER),
                (dev.TANK_VFX_HEADER, TANK_VFX_HEADER),
                (dev.ATTACK_POSE_HEADER, ATTACK_POSE_HEADER),
                (dev.WEAPON_SOURCE_HEADER, WEAPON_SOURCE_HEADER),
                (dev.CONSTRUCTION_HEADER, CONSTRUCTION_COLOR_HEADER),
            ):
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding='ascii')
            death = root / dev.DEATH_SOURCE_HEADER
            death.parent.mkdir(parents=True, exist_ok=True)
            death.write_text(DEATH_SOURCE_HEADER.replace(
                '#define SC64_DEATH_SOURCE_WORD_BYTES 1u',
                '#define SC64_DEATH_SOURCE_WORD_BYTES 0u'), encoding='ascii')
            with patch.object(dev, 'ROOT', root), patch.object(
                    dev, 'run', side_effect=AssertionError('Make must not run')):
                with self.assertRaisesRegex(RuntimeError, 'source death-art data'):
                    dev.build('terran')
            death.write_text(DEATH_SOURCE_HEADER, encoding='ascii')
            with patch.object(dev, 'ROOT', root), patch.object(dev, 'run') as run:
                dev.build('terran')
            run.assert_called_once_with(['make', 'terran'])

    def test_canonical_zerg_status_rejects_stale_z3_only_stage_map(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            inputs = {
                '.local/sc64/rom/StarCraft64-USA.z64': 'ROM',
                '.local/sc64/backwater/generated/sc64_mission_generated.h': 'input',
                '.local/sc64/backwater/generated/sc64_assets_generated.h': 'input',
                '.local/sc64/backwater/generated/sc64_sprites_generated.h': 'input',
                '.local/sc64/races/generated/sc64_races_generated.h':
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                    'sc64_race_z3_units\n'
                    '#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n',
                '.local/sc64/races/generated/sc64_race_sprites_generated.h': 'sprites',
                '.local/sc64/races/generated/sc64_menu_generated.h': 'menu',
                '.local/sc64/briefings/generated/sc64_briefing_generated.h': BRIEFING_HEADER,
                '.local/sc64/interface/generated/sc64_font_generated.h': 'interface fixture',
                '.local/sc64/interface/generated/sc64_icons_generated.h': 'interface fixture',
                '.local/sc64/interface/generated/sc64_chrome_generated.h': 'interface fixture',
                '.local/sc64/construction/generated/sc64_construction_sprites_generated.h': CONSTRUCTION_COLOR_HEADER,
                '.local/sc64/skirmish/generated/sc64_skirmish_generated.h': 'skirmish',
                '.local/sc64/zp-stage/generated/sc64_races_generated.h':
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                    'sc64_race_z3_units\n'
                    '#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n',
                '.local/sc64/zp-stage/generated/sc64_race_sprites_generated.h':
                    '#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n',
                '.local/sc64/z4/generated/sc64_z4_scenario_generated.h':
                    '#define SC64_Z4_SCENARIO_GENERATED_H 1\n',
                '.local/sc64/z4/manifest.json':
                    '{"schema":1,"mission":{"id":"Z4","global_catalog_index":15,'
                    '"source_map_path":"008/017"}}',
                'build/StarCraft-Zerg-Z4-GBA.gba': 'ROM',
            }
            for relative, content in inputs.items():
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding='ascii')

            output = io.StringIO()
            with patch.object(dev, 'ROOT', root), contextlib.redirect_stdout(output):
                self.assertEqual(dev.status('zerg'), 1)
            self.assertIn('MISSING Zerg chapter 4 maps', output.getvalue())
            self.assertIn('READY   Zerg chapter 4 authenticated source', output.getvalue())

    def test_canonical_zerg_prepare_rejects_z3_only_stage_map(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rom = root / 'owned-rom.z64'
            rom.write_bytes(b'user supplied ROM fixture')

            def fake_run(command, *, cwd=dev.ROOT):
                script = Path(command[1]).name
                local = root / '.local/sc64'
                if script == 'sc64_workspace.py':
                    for relative in (
                        '.local/sc64/rom/StarCraft64-USA.z64',
                        '.local/sc64/backwater/generated/sc64_mission_generated.h',
                        '.local/sc64/backwater/generated/sc64_assets_generated.h',
                        '.local/sc64/backwater/generated/sc64_sprites_generated.h',
                    ):
                        path = root / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_text('prepared\n', encoding='ascii')
                elif script == 'sc64_races_pack.py':
                    output = (Path(command[command.index('--out') + 1]) if '--out' in command
                              else local / 'races/generated/sc64_races_generated.h')
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(
                        '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                        'sc64_race_z3_units\n'
                        '#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n', encoding='ascii')
                elif script == 'sc64_race_sprites.py' and '--header' in command:
                    output = Path(command[command.index('--header') + 1])
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n',
                                      encoding='ascii')
                elif script == 'sc64_construction_sprites.py':
                    emit_construction_header(root)
                elif script in ('sc64_font_assets.py', 'sc64_icon_assets.py', 'sc64_chrome_assets.py'):
                    names = {'sc64_font_assets.py': 'sc64_font_generated.h',
                             'sc64_icon_assets.py': 'sc64_icons_generated.h',
                             'sc64_chrome_assets.py': 'sc64_chrome_generated.h'}
                    output = root / '.local/sc64/interface/generated' / names[script]
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('source-only interface fixture\n', encoding='ascii')
                elif script == 'sc64_briefing_assets.py':
                    output = local / 'briefings/generated/sc64_briefing_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(BRIEFING_HEADER, encoding='ascii')
                elif script == 'sc64_unit_facing.py':
                    emit_facing_header(root)
                elif script == 'sc64_unit_walk_frames.py':
                    emit_walk_header(root)
                elif script == 'sc64_unit_attack_sprites.py':
                    emit_attack_pose_header(root)
                elif script == 'sc64_weapon_source_sprites.py':
                    emit_weapon_source_header(root)
                elif script == 'sc64_death_sprites.py':
                    emit_death_source_header(root)
                elif script == 'sc64_z4_pack.py':
                    output = local / 'z4/manifest.json'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(
                        '{"schema":1,"mission":{"id":"Z4","global_catalog_index":15,'
                        '"source_map_path":"008/017"}}', encoding='ascii')
                elif script == 'sc64_z4_scenario_pack.py':
                    output = local / 'z4/generated/sc64_z4_scenario_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('#define SC64_Z4_SCENARIO_GENERATED_H 1\n',
                                      encoding='ascii')

            with patch.object(dev, 'ROOT', root), patch.object(dev, 'ensure_extractor'), \
                 patch.object(dev, 'run', side_effect=fake_run):
                with self.assertRaisesRegex(RuntimeError, 'SC64_Z4_DATA_ENABLED map gate'):
                    dev.prepare(rom, 'zerg')

    def test_race_prepare_fails_if_generated_map_gate_is_wrong(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rom = root / 'user-rom.z64'
            rom.write_bytes(b'user ROM fixture')

            def fake_run(command, *, cwd=dev.ROOT):
                script = Path(command[1]).name
                local = root / '.local/sc64'
                if script == 'sc64_workspace.py':
                    (local / 'rom').mkdir(parents=True, exist_ok=True)
                    (local / 'rom/StarCraft64-USA.z64').write_bytes(b'prepared ROM')
                    base = local / 'backwater/generated'
                    base.mkdir(parents=True, exist_ok=True)
                    for name in ('sc64_mission_generated.h', 'sc64_assets_generated.h',
                                 'sc64_sprites_generated.h'):
                        (base / name).write_text('mock\n', encoding='utf-8')
                elif script == 'sc64_races_pack.py':
                    path = local / 'races/generated/sc64_races_generated.h'
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text('#if defined(SC64_UNIFIED) && defined(SC64_P3_ENABLED)\n'
                                    'sc64_race_p3_units\n'
                                    '#endif /* SC64_UNIFIED && SC64_P3_ENABLED */\n',
                                    encoding='utf-8')

            with patch.object(dev, 'ROOT', root), patch.object(dev, 'ensure_extractor'), \
                 patch.object(dev, 'run', side_effect=fake_run):
                with self.assertRaisesRegex(RuntimeError, 'SC64_Z3_ENABLED map gate'):
                    dev.prepare(rom, 'zerg')

    def test_canonical_zerg_prepare_generates_private_z4_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rom = root / 'owned-rom.z64'
            rom.write_bytes(b'user supplied ROM fixture')
            calls = []

            def fake_run(command, *, cwd=dev.ROOT):
                calls.append(command)
                script = Path(command[1]).name
                local = root / '.local/sc64'
                if script == 'sc64_workspace.py':
                    (local / 'rom').mkdir(parents=True, exist_ok=True)
                    (local / 'rom/StarCraft64-USA.z64').write_bytes(b'prepared ROM')
                elif script == 'sc64_races_pack.py':
                    output = (Path(command[command.index('--out') + 1]) if '--out' in command
                              else local / 'races/generated/sc64_races_generated.h')
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(
                        '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                        'sc64_race_z3_units\n#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n' +
                        ('#if defined(SC64_UNIFIED) && defined(SC64_Z4_DATA_ENABLED)\n'
                         'sc64_race_z4_units\n'
                         '#endif /* SC64_UNIFIED && SC64_Z4_DATA_ENABLED */\n'
                         if output.parent == local / 'zp-stage/generated' else ''),
                        encoding='ascii')
                elif script == 'sc64_race_sprites.py':
                    if '--header' in command:
                        output = Path(command[command.index('--header') + 1])
                    else:
                        output = local / 'races/generated/sc64_race_sprites_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(UNIT_WORD_SPRITE_HEADER if '--prepack-unit-words' in command
                                      else '#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n',
                                      encoding='ascii')
                elif script == 'sc64_construction_sprites.py':
                    emit_construction_header(root)
                elif script in ('sc64_font_assets.py', 'sc64_icon_assets.py', 'sc64_chrome_assets.py'):
                    names = {'sc64_font_assets.py': 'sc64_font_generated.h',
                             'sc64_icon_assets.py': 'sc64_icons_generated.h',
                             'sc64_chrome_assets.py': 'sc64_chrome_generated.h'}
                    output = root / '.local/sc64/interface/generated' / names[script]
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('source-only interface fixture\n', encoding='ascii')
                elif script == 'sc64_briefing_assets.py':
                    output = local / 'briefings/generated/sc64_briefing_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(BRIEFING_HEADER, encoding='ascii')
                elif script == 'sc64_unit_facing.py':
                    emit_facing_header(root)
                elif script == 'sc64_unit_walk_frames.py':
                    emit_walk_header(root)
                elif script == 'sc64_unit_attack_sprites.py':
                    emit_attack_pose_header(root)
                elif script == 'sc64_weapon_source_sprites.py':
                    emit_weapon_source_header(root)
                elif script == 'sc64_death_sprites.py':
                    emit_death_source_header(root)
                elif script == 'sc64_z4_pack.py':
                    output = local / 'z4/manifest.json'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(
                        '{"schema":1,"mission":{"id":"Z4","global_catalog_index":15,'
                        '"source_map_path":"008/017"}}', encoding='ascii')
                elif script == 'sc64_z4_scenario_pack.py':
                    output = local / 'z4/generated/sc64_z4_scenario_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('#define SC64_Z4_SCENARIO_GENERATED_H 1\n', encoding='ascii')

            with patch.object(dev, 'ROOT', root), patch.object(dev, 'ensure_extractor'), \
                 patch.object(dev, 'run', side_effect=fake_run):
                dev.prepare(rom, 'zerg-z4')

            scripts = [Path(call[1]).name for call in calls]
            for required in ('sc64_races_pack.py', 'sc64_race_sprites.py',
                             'sc64_z4_pack.py', 'sc64_z4_scenario_pack.py',
                             'sc64_briefing_assets.py'):
                self.assertIn(required, scripts)
            self.assertTrue(all(dev.canonical_zerg_input_ready(label, root / relative)
                                for label, relative, _ in dev.CANONICAL_ZERG_INPUTS))
            canonical_sprite_calls = [call for call in calls
                                      if Path(call[1]).name == 'sc64_race_sprites.py'
                                      and '--header' in call
                                      and str(root / '.local/sc64/zp-stage/generated/'
                                              'sc64_race_sprites_generated.h') in call]
            self.assertEqual(len(canonical_sprite_calls), 1)
            self.assertIn('--prepack-unit-words', canonical_sprite_calls[0])

    def test_canonical_zerg_prepare_adds_chapter_five_to_seven_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rom = root / 'owned-rom.z64'
            rom.write_bytes(b'user supplied ROM fixture')
            calls = []

            def fake_run(command, *, cwd=dev.ROOT):
                calls.append(command)
                script = Path(command[1]).name
                local = root / '.local/sc64'
                if script == 'sc64_workspace.py':
                    (local / 'rom').mkdir(parents=True, exist_ok=True)
                    (local / 'rom/StarCraft64-USA.z64').write_bytes(b'prepared ROM')
                elif script == 'sc64_races_pack.py':
                    output = (Path(command[command.index('--out') + 1]) if '--out' in command
                              else local / 'races/generated/sc64_races_generated.h')
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(
                        '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
                        'sc64_race_z3_units\n#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n' +
                        ('#if defined(SC64_UNIFIED) && defined(SC64_Z4_DATA_ENABLED)\n'
                         'sc64_race_z4_units\n'
                         '#endif /* SC64_UNIFIED && SC64_Z4_DATA_ENABLED */\n'
                         if output.parent == local / 'zp-stage/generated' else ''),
                        encoding='ascii')
                elif script == 'sc64_race_sprites.py':
                    if '--header' in command:
                        output = Path(command[command.index('--header') + 1])
                    else:
                        output = local / 'races/generated/sc64_race_sprites_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(UNIT_WORD_SPRITE_HEADER if '--prepack-unit-words' in command
                                      else '#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1\n',
                                      encoding='ascii')
                elif script == 'sc64_construction_sprites.py':
                    emit_construction_header(root)
                elif script in ('sc64_font_assets.py', 'sc64_icon_assets.py', 'sc64_chrome_assets.py'):
                    names = {'sc64_font_assets.py': 'sc64_font_generated.h',
                             'sc64_icon_assets.py': 'sc64_icons_generated.h',
                             'sc64_chrome_assets.py': 'sc64_chrome_generated.h'}
                    output = root / '.local/sc64/interface/generated' / names[script]
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('source-only interface fixture\n', encoding='ascii')
                elif script == 'sc64_briefing_assets.py':
                    output = local / 'briefings/generated/sc64_briefing_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(BRIEFING_HEADER, encoding='ascii')
                elif script == 'sc64_unit_facing.py':
                    emit_facing_header(root)
                elif script == 'sc64_unit_walk_frames.py':
                    emit_walk_header(root)
                elif script == 'sc64_z4_pack.py':
                    output = local / 'z4/manifest.json'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(
                        '{"schema":1,"mission":{"id":"Z4","global_catalog_index":15,'
                        '"source_map_path":"008/017"}}', encoding='ascii')
                elif script == 'sc64_z4_scenario_pack.py':
                    output = local / 'z4/generated/sc64_z4_scenario_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('#define SC64_Z4_SCENARIO_GENERATED_H 1\n', encoding='ascii')

            staged = []

            def fake_staged(profile, private_rom):
                staged.append(profile)
                local = root / '.local/sc64'
                (local / 'zp-stage/generated/sc64_races_generated.h').write_text(
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\nsc64_race_z3_units\n'
                    '#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n'
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z4_DATA_ENABLED)\nsc64_race_z4_units\n'
                    '#endif /* SC64_UNIFIED && SC64_Z4_DATA_ENABLED */\n'
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z6_DATA_ENABLED)\nsc64_race_z6_units\n'
                    '#endif /* SC64_UNIFIED && SC64_Z6_DATA_ENABLED */\n'
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z7_DATA_ENABLED)\nsc64_race_z7_units\n'
                    '#endif /* SC64_UNIFIED && SC64_Z7_DATA_ENABLED */\n', encoding='ascii')
                for label, relative, marker in dev.CANONICAL_ZERG_Z6_INPUTS[:-1]:
                    path = root / relative
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_text(f'#define {marker} 1\n', encoding='ascii')
                if profile == 'zerg-z7':
                    for label, relative, marker in dev.STAGED_CAMPAIGN_INPUTS['zerg-z7']:
                        path = root / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_text(f'#define {marker} 18u\n', encoding='ascii')

            def fake_bank_run(command, *, cwd=dev.ROOT):
                fake_run(command, cwd=cwd)
                if '--include-z6-buildings' in command:
                    output = Path(command[command.index('--header') + 1])
                    output.write_text(UNIT_WORD_SPRITE_HEADER +
                                      '#define SC64_Z6_BUILDING_SPRITE_SET_COMPLETE 1\n'
                                      '#define SC64_Z5_DOODAD_SPRITE_SET_COMPLETE 1\n',
                                      encoding='ascii')

            with patch.object(dev, 'ROOT', root), patch.object(dev, 'ensure_extractor'), \
                 patch.object(dev, 'run', side_effect=fake_bank_run), \
                 patch.object(dev, 'prepare_staged_campaign', side_effect=fake_staged):
                dev.prepare(rom, 'zerg')
            self.assertEqual(staged, ['zerg-z6', 'zerg-z7'])
            bank_calls = [call for call in calls if '--include-z6-buildings' in call]
            self.assertEqual(len(bank_calls), 1)
            self.assertIn('--prepack-unit-words', bank_calls[0])
            self.assertIn(str(root / '.local/sc64/zerg-z6/generated/sc64_race_sprites_generated.h'),
                          bank_calls[0])
            self.assertTrue(all(dev.canonical_zerg_z6_input_ready(label, root / relative)
                                for label, relative, _ in dev.CANONICAL_ZERG_Z6_INPUTS))

            scripts = [Path(call[1]).name for call in calls]
            for required in ('sc64_races_pack.py', 'sc64_race_sprites.py',
                             'sc64_z4_pack.py', 'sc64_z4_scenario_pack.py',
                             'sc64_briefing_assets.py',
                             'sc64_unit_attack_sprites.py',
                             'sc64_weapon_source_sprites.py',
                             'sc64_death_sprites.py'):
                self.assertIn(required, scripts)
            pose_calls = [call for call in calls
                          if Path(call[1]).name == 'sc64_unit_attack_sprites.py']
            self.assertEqual(len(pose_calls), 1)
            self.assertIn('--rom', pose_calls[0])
            self.assertIn('--remap', pose_calls[0])
            self.assertIn(str(root / dev.ATTACK_POSE_HEADER), pose_calls[0])
            self.assertTrue(dev.attack_pose_input_ready(root / dev.ATTACK_POSE_HEADER))
            weapon_calls = [call for call in calls
                            if Path(call[1]).name == 'sc64_weapon_source_sprites.py']
            self.assertEqual(len(weapon_calls), 1)
            self.assertIn('--rom', weapon_calls[0])
            self.assertIn('--out', weapon_calls[0])
            self.assertIn(str((root / dev.WEAPON_SOURCE_HEADER).parent), weapon_calls[0])
            self.assertTrue(dev.weapon_source_input_ready(root / dev.WEAPON_SOURCE_HEADER))
            death_calls = [call for call in calls
                           if Path(call[1]).name == 'sc64_death_sprites.py']
            self.assertEqual(len(death_calls), 1)
            self.assertIn('--rom', death_calls[0])
            self.assertIn('--palette-remap', death_calls[0])
            self.assertIn(str(root / '.local/sc64/backwater/generated/palette-remap.bin'), death_calls[0])
            self.assertIn('--header', death_calls[0])
            self.assertIn(str(root / dev.DEATH_SOURCE_HEADER), death_calls[0])
            self.assertIn('--out', death_calls[0])
            self.assertIn(str((root / dev.DEATH_SOURCE_HEADER).parent), death_calls[0])
            self.assertTrue(dev.death_source_input_ready(root / dev.DEATH_SOURCE_HEADER))
            self.assertTrue(all(dev.canonical_zerg_input_ready(label, root / relative)
                                for label, relative, _ in dev.CANONICAL_ZERG_INPUTS))
            canonical_sprite_calls = [call for call in calls
                                      if Path(call[1]).name == 'sc64_race_sprites.py'
                                      and '--header' in call
                                      and str(root / '.local/sc64/zp-stage/generated/'
                                              'sc64_race_sprites_generated.h') in call]
            self.assertEqual(len(canonical_sprite_calls), 1)
            self.assertIn('--prepack-unit-words', canonical_sprite_calls[0])

    def check_doctor(self, *, require_emulator=False, emulator=None, missing=None):
        output = io.StringIO()
        def which(command):
            return None if command == missing else '/test-tools/' + command
        with patch.object(dev.shutil, 'which', side_effect=which), \
             patch.object(dev, 'locate_mgba', return_value=emulator), \
             contextlib.redirect_stdout(output):
            status = dev.doctor(require_emulator)
        return status, output.getvalue()

    def test_default_doctor_accepts_missing_emulator(self):
        status, output = self.check_doctor()
        self.assertEqual(status, 0)
        self.assertIn('OPTIONAL mGBA', output)

    def test_explicit_emulator_check_fails_without_emulator(self):
        status, output = self.check_doctor(require_emulator=True)
        self.assertEqual(status, 1)
        self.assertIn('MISSING mGBA', output)

    def test_missing_required_tool_still_fails(self):
        prefix = os.environ.get('PREFIX', 'arm-none-eabi-')
        status, output = self.check_doctor(missing=prefix + 'size')
        self.assertEqual(status, 1)
        self.assertIn('MISSING ARM size', output)

    def test_bad_optional_emulator_configuration_does_not_block_build_tools(self):
        output = io.StringIO()
        with patch.object(dev.shutil, 'which', return_value='/test-tools/available'), \
             patch.object(dev, 'locate_mgba', side_effect=RuntimeError('invalid MGBA path')), \
             contextlib.redirect_stdout(output):
            self.assertEqual(dev.doctor(), 0)
        self.assertIn('OPTIONAL mGBA: invalid MGBA path', output.getvalue())

    def test_emulator_cli_help_requires_no_emulator_or_private_data(self):
        environment = dict(os.environ, MGBA=str(ROOT / 'build/nonexistent-emulator'))
        scripts = ('emulator_smoke.py', 'sc64_campaign_save_smoke.py',
                   'sc64_menu_smoke.py', 'sc64_profile.py', 'sc64_races_profile.py',
                   'sc64_replay.py', 'sc64_selection_smoke.py', 'sc64_unified_smoke.py')
        for name in scripts:
            with self.subTest(script=name):
                result = subprocess.run([sys.executable, str(ROOT / 'tools' / name), '--help'],
                                        cwd=ROOT, env=environment, capture_output=True, text=True,
                                        timeout=10)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn('usage:', result.stdout)

    def test_full_terran_status_reports_missing_fresh_layout_dependencies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = io.StringIO()
            games = dict(dev.GAMES, terran=('terran-full', 'build/StarCraft-Terran-Transport1-GBA.gba'))
            with patch.object(dev, 'ROOT', root), patch.object(dev, 'GAMES', games), \
                 contextlib.redirect_stdout(output):
                status = dev.status('terran')
            self.assertEqual(status, 1)
            text = output.getvalue()
            for dependency in (
                'Terran T5 scenario', 'Terran T5 availability',
                'Terran T6 scenario', 'Terran T6 availability',
                'Full Terran maps', 'Full Terran sprites',
                'Terran T7 scenario', 'Terran T10 availability',
                'Directional unit sprites', 'Tank firing effects',
                'Unit attack poses', 'Source weapon effects', 'Built ROM',
                'Campaign briefings and portraits',
            ):
                with self.subTest(dependency=dependency):
                    self.assertIn(f'MISSING {dependency}', text)

    def test_full_terran_prepare_populates_all_status_inputs_from_fresh_layout(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rom = root / 'user-rom.z64'
            rom.write_bytes(b'local user-supplied ROM fixture')
            games = dict(dev.GAMES, terran=('terran-full', 'build/StarCraft-Terran-Transport1-GBA.gba'))
            calls = []

            def touch(path):
                path = Path(path)
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('mock generated input\n')

            def fake_run(command, *, cwd=dev.ROOT):
                calls.append(command)
                script = Path(command[1]).name if len(command) > 1 else ''
                local = root / '.local/sc64'
                race = local / 'races/generated'
                if script == 'sc64_workspace.py':
                    (local / 'rom').mkdir(parents=True, exist_ok=True)
                    (local / 'rom/StarCraft64-USA.z64').write_bytes(b'prepared user ROM')
                    base = local / 'backwater/generated'
                    for name in ('sc64_mission_generated.h', 'sc64_assets_generated.h',
                                 'sc64_sprites_generated.h'):
                        touch(base / name)
                elif script == 'sc64_races_pack.py':
                    for name in ('sc64_races_generated.h', 'sc64_t4_scenario_generated.h',
                                 'sc64_t5_scenario_generated.h', 'sc64_t5_availability_generated.h',
                                 'sc64_t6_scenario_generated.h', 'sc64_t6_availability_generated.h'):
                        touch(race / name)
                    for mission in range(7, 11):
                        touch(race / f'sc64_t{mission}_scenario_generated.h')
                        touch(race / f'sc64_t{mission}_availability_generated.h')
                elif script == 'sc64_t4_scenario_pack.py':
                    profile = command[command.index('--profile') + 1] if '--profile' in command else 't4'
                    touch(race / f'sc64_{profile}_scenario_generated.h')
                elif script == 'sc64_t5_availability.py':
                    profile = command[command.index('--profile') + 1]
                    touch(race / f'sc64_{profile}_availability_generated.h')
                elif script == 'sc64_race_sprites.py':
                    if '--header' in command:
                        header = Path(command[command.index('--header') + 1])
                        touch(Path(command[command.index('--out') + 1]) / 'manifest.json')
                    else:
                        header = race / 'sc64_race_sprites_generated.h'
                    touch(header)
                    if '--prepack-unit-words' in command:
                        header.write_text(UNIT_WORD_SPRITE_HEADER, encoding='ascii')
                elif script == 'sc64_construction_sprites.py':
                    emit_construction_header(root)
                elif script == 'sc64_unit_facing.py':
                    emit_facing_header(root)
                elif script == 'sc64_unit_walk_frames.py':
                    emit_walk_header(root)
                elif script == 'sc64_unit_attack_sprites.py':
                    emit_attack_pose_header(root)
                elif script == 'sc64_weapon_source_sprites.py':
                    emit_weapon_source_header(root)
                elif script == 'sc64_death_sprites.py':
                    emit_death_source_header(root)
                elif script == 'sc64_menu_assets.py':
                    touch(race / 'sc64_menu_generated.h')
                elif script in ('sc64_font_assets.py', 'sc64_icon_assets.py', 'sc64_chrome_assets.py'):
                    names = {'sc64_font_assets.py': 'sc64_font_generated.h',
                             'sc64_icon_assets.py': 'sc64_icons_generated.h',
                             'sc64_chrome_assets.py': 'sc64_chrome_generated.h'}
                    output = root / '.local/sc64/interface/generated' / names[script]
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text('source-only interface fixture\n', encoding='ascii')
                elif script == 'sc64_briefing_assets.py':
                    output = local / 'briefings/generated/sc64_briefing_generated.h'
                    output.parent.mkdir(parents=True, exist_ok=True)
                    output.write_text(BRIEFING_HEADER, encoding='ascii')
                elif script == 'sc64_skirmish_pack.py':
                    touch(local / 'skirmish/generated/sc64_skirmish_generated.h')

            output = io.StringIO()
            with patch.object(dev, 'ROOT', root), patch.object(dev, 'GAMES', games), \
                 patch.object(dev, 'ensure_extractor'), patch.object(dev, 'run', side_effect=fake_run), \
                 contextlib.redirect_stdout(output):
                dev.prepare(rom, 'terran')
                status_output = io.StringIO()
                with contextlib.redirect_stdout(status_output):
                    self.assertEqual(dev.status('terran'), 1)  # data ready; no ROM build yet

            self.assertIn("Prepared local StarCraft 64 data for profile 'terran'", output.getvalue())
            for name in (
                'sc64_t5_scenario_generated.h', 'sc64_t5_availability_generated.h',
                'sc64_t6_scenario_generated.h', 'sc64_t6_availability_generated.h',
            ):
                with self.subTest(name=name):
                    self.assertTrue((root / '.local/sc64/races/generated' / name).is_file())
            stage = root / '.local/sc64/terran-late/import-stage'
            self.assertTrue((stage / 'sc64_races_generated.h').is_file())
            self.assertTrue((stage / 'sc64_race_sprites_generated.h').is_file())
            self.assertTrue((stage / 'sprites/manifest.json').is_file())
            for mission in range(7, 11):
                self.assertTrue((stage / f'sc64_t{mission}_scenario_generated.h').is_file())
                self.assertTrue((stage / f'sc64_t{mission}_availability_generated.h').is_file())
            ready_output = status_output.getvalue()
            self.assertNotIn('MISSING Terran T5', ready_output)
            self.assertNotIn('MISSING Terran T6', ready_output)
            self.assertNotIn('MISSING Full Terran', ready_output)
            self.assertIn('MISSING Built ROM', ready_output)
            commands = [Path(command[1]).name for command in calls]
            self.assertIn('sc64_races_pack.py', commands)
            self.assertIn('sc64_briefing_assets.py', commands)
            self.assertIn('sc64_t4_scenario_pack.py', commands)
            self.assertIn('sc64_t5_availability.py', commands)
            t4_profile_calls = [command for command in calls
                                if Path(command[1]).name == 'sc64_t4_scenario_pack.py']
            self.assertIn(['--profile', 't5'], [command[-2:] for command in t4_profile_calls])
            self.assertIn(['--profile', 't6'], [command[-2:] for command in t4_profile_calls])
            availability_profiles = [command[command.index('--profile') + 1] for command in calls
                                     if Path(command[1]).name == 'sc64_t5_availability.py']
            self.assertEqual(availability_profiles, ['t5', 't6'])
            sprite_calls = [command for command in calls
                            if Path(command[1]).name == 'sc64_race_sprites.py']
            self.assertTrue(any('--include-t6' in command and '--include-late-terran' in command
                                for command in sprite_calls))


if __name__ == '__main__':
    unittest.main()
