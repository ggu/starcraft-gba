"""New-contributor paths must work without an emulator or imported game data."""
import contextlib
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
                         ('StarCraft-Zerg-Z4-GBA', 'starcraft-zerg-z4', 'SCZ4', 'SC ZERG Z4'))
        self.assertEqual(verify_roms.TARGETS['--zerg-z4'], verify_roms.TARGETS['--zerg'])
        self.assertEqual(verify_roms.TARGETS['--zerg-z3'],
                         ('StarCraft-Zerg-GBA', 'starcraft-zerg', 'SCZ3', 'SC ZERG Z3'))
        self.assertEqual(verify_roms.TARGETS['--protoss'],
                         ('StarCraft-Protoss-P4-Storm-GBA', 'starcraft-protoss-p4-storm',
                          'SCP5', 'SC P4 STORM'))
        self.assertEqual(verify_roms.TARGETS['--protoss-p4-storm'], verify_roms.TARGETS['--protoss'])
        self.assertEqual(verify_roms.TARGETS['--protoss-p3'],
                         ('StarCraft-Protoss-GBA', 'starcraft-protoss', 'SCP3', 'SC PROTOSS3'))

    def test_dev_race_tests_include_full_campaign_targets(self):
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg'], 'test-zerg')
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg-z3'], 'test-zerg-z3')
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss'], 'test-protoss')
        self.assertEqual(dev.GAMES['zerg'], ('zerg', 'build/StarCraft-Zerg-Z4-GBA.gba'))
        self.assertEqual(dev.GAMES['zerg-z3'], ('zerg-z3', 'build/StarCraft-Zerg-GBA.gba'))
        self.assertEqual(dev.PLAY_SAVE_PROFILES,
                         {'zerg': 'zerg-z4', 'zerg-z3': 'zerg',
                          'protoss': 'protoss-p4-storm', 'protoss-p3': 'protoss'})
        self.assertEqual(sc64_split_smoke.PROFILE_BUILD_PATHS['zerg'],
                         ('StarCraft-Zerg-Z4-GBA.gba', 'starcraft-zerg-z4.elf'))
        self.assertEqual(sc64_split_profile.PROFILE_BUILD_PATHS['zerg'],
                         ('StarCraft-Zerg-Z4-GBA.gba', 'starcraft-zerg-z4.elf'))
        for routes in (sc64_split_profile.PROFILE_BUILD_PATHS, sc64_split_smoke.PROFILE_BUILD_PATHS):
            self.assertEqual(routes['protoss'],
                             ('StarCraft-Protoss-P4-Storm-GBA.gba', 'starcraft-protoss-p4-storm.elf'))

    def test_p4_storm_and_legacy_p3_have_explicit_build_and_save_routes(self):
        self.assertEqual(dev.GAMES['protoss-p3'],
                         ('protoss-p3', 'build/StarCraft-Protoss-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p4-storm'],
                         ('protoss-p4-storm', 'build/StarCraft-Protoss-P4-Storm-GBA.gba'))
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss-p4-storm'], 'test-protoss')
        self.assertEqual(dev.GAMES['protoss'][1], dev.GAMES['protoss-p4-storm'][1])
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
                elif script == 'sc64_p4_pack.py':
                    emit(local / 'p4/manifest.json',
                         '{"schema":1,"mission":{"id":"P4","global_catalog_index":23,'
                         '"source_map_path":"008/021"}}')
                elif script == 'sc64_p4_scenario_pack.py':
                    emit(local / 'p4/generated/sc64_p4_scenario_generated.h',
                         '#define SC64_P4_SCENARIO_GENERATED_H 1\n')

            with patch.object(dev, 'ROOT', root), patch.object(dev, 'ensure_extractor'), \
                    patch.object(dev, 'run', side_effect=fake_run):
                dev.prepare(rom, 'protoss')
            self.assertTrue(all(dev.protoss_p4_input_ready(label, root / relative)
                                for label, relative, _ in dev.PROTOSS_P4_INPUTS))
            self.assertIn('sc64_skirmish_pack.py', [Path(call[1]).name for call in calls])
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

    def test_canonical_zerg_readiness_requires_z4_stage_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            markers = {
                '.local/sc64/rom/StarCraft64-USA.z64': 'source ROM fixture',
                '.local/sc64/backwater/generated/sc64_mission_generated.h': 'input',
                '.local/sc64/backwater/generated/sc64_assets_generated.h': 'input',
                '.local/sc64/backwater/generated/sc64_sprites_generated.h': 'input',
                '.local/sc64/races/generated/sc64_races_generated.h':
                    '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\nsc64_race_z3_units\n#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */',
                '.local/sc64/races/generated/sc64_race_sprites_generated.h': 'input',
                '.local/sc64/races/generated/sc64_menu_generated.h': 'input',
                '.local/sc64/construction/generated/sc64_construction_sprites_generated.h': 'input',
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
                self.assertEqual(dev.status('zerg'), 1)
            self.assertIn('MISSING Zerg chapter 4 sprites', output.getvalue())
            self.assertIn('./dev sc64 prepare --rom PATH --profile zerg', output.getvalue())
            sprite_header.write_text(UNIT_WORD_SPRITE_HEADER, encoding='ascii')
            output = io.StringIO()
            with patch.object(dev, 'ROOT', root), contextlib.redirect_stdout(output):
                self.assertEqual(dev.status('zerg'), 0)
            self.assertIn('Zerg chapter 4 scenario:', output.getvalue())
            (root / '.local/sc64/z4/manifest.json').write_text(
                '{"schema":1,"mission":{"id":"Z4","global_catalog_index":14,"source_map_path":"008/017"}}',
                encoding='ascii')
            with patch.object(dev, 'ROOT', root), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(dev.status('zerg'), 1)

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

    def test_terran_build_requires_real_word_metadata_before_make(self):
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
            with patch.object(dev, 'ROOT', root), patch.object(dev, 'run') as run:
                dev.build('terran')
            run.assert_called_once_with(['make', 'terran'])

    def test_staged_z6_and_p5_profiles_have_explicit_build_and_test_routes(self):
        self.assertEqual(dev.GAMES['zerg-z6'],
                         ('zerg-z6-probe', 'build/StarCraft-Zerg-Z6-Probe-GBA.gba'))
        self.assertEqual(dev.GAMES['protoss-p5-shuttle'],
                         ('protoss-p5-shuttle', 'build/StarCraft-Protoss-P5-Shuttle-GBA.gba'))
        self.assertEqual(dev.SC64_TEST_TARGETS['zerg-z6'], (
            'test-sc64-z6-runtime', 'test-sc64-z6-save', 'test-sc64-z6-profile',
            'test-sc64-z6-frame-coverage', 'test-sc64-z6-controller'))
        self.assertEqual(dev.SC64_TEST_TARGETS['protoss-p5-shuttle'],
                         ('test-sc64-p5-shuttle-runtime', 'test-sc64-p5-campaign-run'))

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
                        output.write_text('\n'.join(f'#define {marker} 1' for marker in marker_set) + '\n',
                                          encoding='ascii')

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
            '.local/sc64/skirmish/generated/sc64_skirmish_generated.h',
        }
        for profile in ('zerg-z6', 'protoss-p5-shuttle'):
            with self.subTest(profile=profile), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                rom = root / 'user-rom.z64'
                rom.write_bytes(b'owned source fixture')

                def fake_run(command, **_kwargs):
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
                            gate, payload = dev.RACE_CAMPAIGN_GATES[profile]
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
                    elif script == 'sc64_menu_assets.py':
                        output = root / '.local/sc64/races/generated/sc64_menu_generated.h'
                        output.parent.mkdir(parents=True, exist_ok=True)
                        output.write_text('menu\n', encoding='ascii')
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
                            marker = next((marker for _, item, marker in dev.STAGED_CAMPAIGN_INPUTS[profile]
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
                if profile == 'zerg-z6':
                    produced.add('.local/sc64/zp-stage/generated/scenarios/sc64_z5_doodads_generated.h')
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
                        '.local/sc64/skirmish/generated/sc64_skirmish_generated.h',
                    ):
                        path = root / relative
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_text('input\n', encoding='ascii')
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
                '.local/sc64/construction/generated/sc64_construction_sprites_generated.h': 'art',
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
                dev.prepare(rom, 'zerg')

            scripts = [Path(call[1]).name for call in calls]
            for required in ('sc64_races_pack.py', 'sc64_race_sprites.py',
                             'sc64_z4_pack.py', 'sc64_z4_scenario_pack.py'):
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
            games = dict(dev.GAMES, terran=('terran-full', 'build/StarCraft-Terran-GBA.gba'))
            with patch.object(dev, 'ROOT', root), patch.object(dev, 'GAMES', games), \
                 contextlib.redirect_stdout(output):
                status = dev.status('terran')
            self.assertEqual(status, 1)
            text = output.getvalue()
            for dependency in (
                'Terran T5 scenario', 'Terran T5 availability',
                'Terran T6 scenario', 'Terran T6 availability',
                'Full Terran maps', 'Full Terran sprites',
                'Terran T7 scenario', 'Terran T10 availability', 'Built ROM',
            ):
                with self.subTest(dependency=dependency):
                    self.assertIn(f'MISSING {dependency}', text)

    def test_full_terran_prepare_populates_all_status_inputs_from_fresh_layout(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rom = root / 'user-rom.z64'
            rom.write_bytes(b'local user-supplied ROM fixture')
            games = dict(dev.GAMES, terran=('terran-full', 'build/StarCraft-Terran-GBA.gba'))
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
                elif script == 'sc64_menu_assets.py':
                    touch(race / 'sc64_menu_generated.h')
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
