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

ROOT = Path(__file__).resolve().parents[1]
loader = importlib.machinery.SourceFileLoader('starcraft_dev', str(ROOT / 'dev'))
spec = importlib.util.spec_from_loader(loader.name, loader)
dev = importlib.util.module_from_spec(spec)
loader.exec_module(dev)


class OnboardingTests(unittest.TestCase):
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
