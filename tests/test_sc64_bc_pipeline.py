"""Asset-free checks for the canonical BC preparation contract."""
import importlib.machinery
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
loader = importlib.machinery.SourceFileLoader('bc_dev', str(ROOT / 'dev'))
spec = importlib.util.spec_from_loader(loader.name, loader)
dev = importlib.util.module_from_spec(spec)
loader.exec_module(dev)


class BcPipelineTests(unittest.TestCase):
    def test_readiness_requires_all_cover_markers_and_both_bodies(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(dev, 'ROOT', Path(tmp)):
            directory = Path(tmp) / dev.BC_DIR
            directory.mkdir(parents=True)
            for name, marker in dev.BC_MARKERS.items():
                (directory / name).write_text(f'#define {marker} 1\n')
            body = Path(tmp) / dev.BC_BODY_HEADER
            body.parent.mkdir(parents=True)
            body.write_text('#define SC64_BC_BODY_ART_FORMAT_VERSION 1u\n'
                            'sc64_race_sprite_science_facility_frame_0_pixels\n'
                            'sc64_race_sprite_physics_lab_frame_0_pixels\n')
            yamato = Path(tmp) / dev.BC_YAMATO_HEADER
            yamato.parent.mkdir(parents=True)
            yamato.write_text('SC64_YAMATO_FRAME_COUNT sc64_yamato_pixels sc64_yamato_light_lut')
            self.assertTrue(dev.bc_inputs_ready())
            (directory / 'sc64_races_generated.h').write_text('')
            self.assertFalse(dev.bc_inputs_ready())
            (directory / 'sc64_races_generated.h').write_text('#define SC64_RACES_COVER_DATA 1\n')
            body.write_text('#define SC64_BC_BODY_ART_FORMAT_VERSION 1u\n')
            self.assertFalse(dev.bc_inputs_ready())

    def test_prepare_writes_only_isolated_destinations(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(dev, 'ROOT', Path(tmp)), \
             patch.object(dev, 'ensure_extractor'), patch.object(dev, 'run') as run, \
             patch.object(dev, 'bc_inputs_ready', return_value=True):
            root = Path(tmp)
            rom = root / 'input.z64'
            rom.write_bytes(b'fixture')
            remap = root / '.local/sc64/backwater/generated/palette-remap.bin'
            remap.parent.mkdir(parents=True)
            remap.write_bytes(b'fixture')
            dev.prepare_bc(rom)
            calls = [call.args[0] for call in run.call_args_list]
            self.assertEqual(len(calls), 5)
            for command in calls[:3]:
                self.assertIn('--cover', command)
                self.assertTrue(str(root / dev.BC_DIR) in command[command.index('--out') + 1])
            self.assertEqual(calls[3][calls[3].index('--header') + 1], str(root / dev.BC_BODY_HEADER))
            self.assertEqual(remap.read_bytes(), b'fixture')

    def test_capability_bundle_has_nine_gates_and_isolated_overlay(self):
        text = (ROOT / 'Makefile').read_text()
        line = next(line for line in text.splitlines() if line.startswith('BC_FLAGS :='))
        self.assertEqual(line.count(' -DSC64_'), 9)
        self.assertIn('-DSC64_BATTLECRUISER_PRODUCTION_EXPERIMENT', line)
        self.assertIn('-I$(BC_DIR)', line)
        for profile in ('TERRAN_ROM_FLAGS', 'ZERG_ROM_FLAGS', 'PROTOSS_P7_CANONICAL_FLAGS', 'SHOWCASE_FLAGS'):
            line = next(line for line in text.splitlines() if line.startswith(profile + ' '))
            self.assertIn('$(BC_FLAGS)', line)
        self.assertIn('$(BC_P7_FACT_HEADER)', text)

class CanonicalLinkTests(unittest.TestCase):
    def test_expanded_construction_arrival_links_body_once_without_assets(self):
        import re
        import subprocess
        import tarfile
        with tempfile.TemporaryDirectory() as tmp:
            tar = subprocess.run(['git', 'archive', 'HEAD'], cwd=ROOT, check=True,
                                 capture_output=True).stdout
            import io
            with tarfile.open(fileobj=io.BytesIO(tar)) as archive:
                archive.extractall(tmp, filter='data')
            (Path(tmp) / 'Makefile').write_text((ROOT / 'Makefile').read_text())
            targets = [f'build/test-sc64-construction-arrival{suffix}-{profile}'
                       for profile in ('terran', 'zerg-z6', 'protoss-p7')
                       for suffix in ('', '-save')]
            held = []
            for _ in range(300):
                args = [v for name in held for v in ('-o', name)]
                result = subprocess.run(['make', '-n', *args, *targets], cwd=tmp,
                                        capture_output=True, text=True)
                if result.returncode == 0:
                    break
                match = re.search(r"No rule to make target [`']([^'`]+)'", result.stderr)
                self.assertIsNotNone(match, result.stderr[-2000:])
                self.assertNotIn(match.group(1), held)
                held.append(match.group(1))
            else:
                self.fail('make prerequisites did not converge')
            commands = [line for line in result.stdout.splitlines()
                        if line.startswith('clang ') and any(' -o ' + t in line for t in targets)]
            self.assertEqual(len(commands), len(targets), result.stdout[-2000:])
            for command in commands:
                self.assertEqual(command.split().count('starcraft/sc64_bc_body_art.c'), 1, command)

if __name__ == '__main__':
    unittest.main()
