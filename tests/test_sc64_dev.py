"""New-contributor paths must work without an emulator or imported game data."""
import contextlib
import importlib.machinery
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import sys
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


if __name__ == '__main__':
    unittest.main()
