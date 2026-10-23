"""Asset-free schema/gating checks for private T7-T10 import tooling."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def load_tool(name: str, filename: str):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'tools' / filename)
    module = importlib.util.module_from_spec(spec)
    assert spec and spec.loader
    spec.loader.exec_module(module)
    return module


SCENARIO = load_tool('late_scenario_pack', 'sc64_t4_scenario_pack.py')
AVAILABILITY = load_tool('late_availability', 'sc64_t5_availability.py')


def framed(tag: bytes, payload: bytes) -> bytes:
    return tag + struct.pack('<I', len(payload)) + payload


def make_records(root: Path, profile: str) -> tuple[Path, Path]:
    spec = SCENARIO.LATE_PROFILES[profile]
    map_path, location_count, trigger_count, doodad_count, property_count, cond_count, action_count = spec
    records_dir = root / profile / 'records'
    records_dir.mkdir(parents=True)
    locations = bytes(location_count * 20)
    triggers = bytearray(trigger_count * 2400)
    # Synthetic opcode rows establish slot/count retention without copying any
    # imported trigger operands into a tracked test fixture.
    for index in range(cond_count):
        trigger, slot = divmod(index, 16)
        triggers[trigger * 2400 + slot * 20 + 15] = 22
    for index in range(action_count):
        trigger, slot = divmod(index, 64)
        triggers[trigger * 2400 + 320 + slot * 32 + 26] = 1
    payloads = {
        b'MRGN': locations,
        b'TRIG': bytes(triggers),
        b'THG2': bytes(doodad_count * 10),
        b'FORC': bytes(20),
        b'OWNR': bytes(12),
        b'SIDE': bytes(12),
        b'UPRP': bytes(property_count * 20),
        b'UPUS': bytes(64),
        b'PUNI': bytes(5700),
    }
    required = SCENARIO.REQUIRED
    metadata = {}
    for tag, payload in payloads.items():
        filename = 'PUNI.bin' if tag == b'PUNI' else required[tag.decode('ascii')][0]
        data = framed(tag, payload)
        (records_dir / filename).write_bytes(data)
        metadata[tag.decode('ascii').strip()] = {
            'file': filename,
            'record_bytes': len(data),
            'payload_bytes': len(payload),
            'record_sha256': hashlib.sha256(data).hexdigest(),
            'payload_sha256': hashlib.sha256(payload).hexdigest(),
        }
    manifest = {
        'schema': 1,
        'source_map_path': map_path,
        'source_rom_sha1': SCENARIO.VERIFIED_ROM_SHA1,
        'source_chk_sha256': SCENARIO.LATE_CHK_SHA256[profile],
        'extractor': AVAILABILITY.EXPECTED_EXTRACTOR,
        'extractor_commit': AVAILABILITY.EXPECTED_EXTRACTOR_COMMIT,
        'records': metadata,
    }
    manifest_path = records_dir.parent / 'manifest.json'
    manifest_path.write_text(json.dumps(manifest), encoding='ascii')
    return records_dir, manifest_path


class TerranLateImportTests(unittest.TestCase):
    def test_all_late_profiles_retain_full_fixed_slots_and_emit_distinct_symbols(self):
        with tempfile.TemporaryDirectory(prefix='sc64-late-synthetic-') as td:
            root = Path(td)
            headers = []
            scenarios = []
            for profile, expected in SCENARIO.LATE_PROFILES.items():
                records_dir, manifest = make_records(root, profile)
                data = SCENARIO.parse_source(records_dir, manifest, profile=profile)
                self.assertEqual(len(data['locations']), expected[1])
                self.assertEqual(len(data['triggers']), expected[2])
                self.assertEqual(len(data['doodads']), expected[3])
                self.assertEqual(len(data['properties']), expected[4])
                self.assertEqual(data['active_condition_count'], expected[5])
                self.assertEqual(data['active_action_count'], expected[6])
                header = SCENARIO.emit_header(data, profile=profile)
                self.assertIn('defined(SC64_T4_RUNTIME) && defined(SC64_TERRAN_LATE_ENABLED)', header)
                self.assertIn(f'sc64_{profile}_scenario', header)
                self.assertNotIn('"', header.split('\n', 1)[0])
                headers.append(root / f'{profile}.h')
                headers[-1].write_text(header, encoding='ascii')
                scenarios.append(profile)

                puni = bytes(5700)
                availability = AVAILABILITY.render_header(puni, profile=profile)
                self.assertIn(f'sc64_{profile}_availability_bits', availability)
                self.assertIn('342u', availability)
                (root / f'{profile}_availability.h').write_text(availability, encoding='ascii')

            cc = shutil.which('clang') or shutil.which('cc')
            if not cc:
                self.skipTest('clang or cc is required for generated-header coexistence')
            includes = ''.join(f'#include "{path.name}"\n' for path in headers)
            includes += ''.join(f'#include "{p}_availability.h"\n' for p in scenarios)
            source = ('#include "sc64_types.h"\n' + includes +
                      'int main(void) { return '
                      'sc64_t7_scenario.trigger_count != 10 || '
                      'sc64_t8_scenario.trigger_count != 12 || '
                      'sc64_t9_scenario.trigger_count != 12 || '
                      'sc64_t10_scenario.trigger_count != 7; }\n')
            c_file = root / 'all.c'
            exe = root / 'all'
            c_file.write_text(source, encoding='ascii')
            subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-Wno-unused-const-variable', '-DSC64_T4_RUNTIME',
                            '-DSC64_TERRAN_LATE_ENABLED', '-I', str(ROOT / 'starcraft'),
                            str(c_file), '-o', str(exe)], check=True,
                           capture_output=True, text=True)
            subprocess.run([str(exe)], check=True, capture_output=True, text=True)

    def test_late_manifest_and_puni_are_bound_to_each_source_profile(self):
        with tempfile.TemporaryDirectory(prefix='sc64-late-manifest-') as td:
            root = Path(td)
            for profile in SCENARIO.LATE_PROFILES:
                records, manifest = make_records(root, profile)
                puni, loaded_manifest = AVAILABILITY.read_puni(
                    records, manifest, expected_map=AVAILABILITY.PROFILES[profile]['map'],
                    expected_puni_sha256=hashlib.sha256(bytes(5700)).hexdigest())
                self.assertEqual(len(puni), 5700)
                self.assertEqual(loaded_manifest['source_map_path'],
                                 AVAILABILITY.PROFILES[profile]['map'])
                doc = json.loads(manifest.read_text())
                doc['source_map_path'] = '008/00A'
                manifest.write_text(json.dumps(doc))
                with self.assertRaises(ValueError):
                    SCENARIO.parse_source(records, manifest, profile=profile)


if __name__ == '__main__':
    unittest.main()
