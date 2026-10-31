#!/usr/bin/env python3
"""Stage authenticated N64 DAT inputs for the experimental OpenBW world.

SPDX-License-Identifier: GPL-3.0-or-later
This only stages tables; image/GRP and flat-map closure is not implemented.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_construction_sprites import load_sprites, EXPECTED_ROM_SHA1

TABLES = {
    'images.dat': ('00C/000', '6b6123e2cc6771d96f914fc9bb92d34da17cc3319dc02e2db0cd10b4f3d2efdc'),
    'units.dat': ('00C/001', '54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df'),
    'flingy.dat': ('00C/002', 'e0bfdff3b35ff417eee003a7d85fccce4787959dcf18d14995a75a4144ddf75a'),
    'sprites.dat': ('00C/003', '3e5ea29bf9937558d0a3a302cd85852c220cd126ec049c0da176c77739bb4a71'),
    'orders.dat': ('00C/004', '0b79cc64f1cd8b23c77419bdf1fb39fb91cde1359c1ad9cb73884d5cceb8f7c9'),
    'upgrades.dat': ('00C/006', '5c7a104f37eab018d15719066052c2db25ef046d9330770045d7e4977f98c12b'),
    'weapons.dat': ('00C/007', '764c6c1535a8ba5d3097c62c5755769ab9d1217e9ef4621e64720a8d2a22e730'),
    'techdata.dat': ('00C/008', '8ffb5f7eb6882a641f7812af1e47650c8f21f4651998be447375e8bc9608c66e'),
    'iscript.bin': ('012/002', 'f8e5f3d0fcca644470c37de65486e44b09eb30141eedccea3445afe7bc0f65bc'),
}

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--extractor', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    out = a.out.resolve()
    if not out.is_relative_to((ROOT / '.local').resolve()) or out.exists():
        p.error('output must be a new directory under this worktree .local')
    extractor = load_sprites(ROOT / 'tools/sc64_sprites.py').load_extractor(a.extractor)
    rom = extractor.load_rom(str(a.rom))
    if hashlib.sha1(rom).hexdigest() != EXPECTED_ROM_SHA1:
        p.error('ROM authentication failed')
    archive = extractor.BoltArchive(rom)
    entries = {}
    for entry in archive.entries():
        if entry.path in entries: p.error('duplicate archive entry')
        entries[entry.path] = entry
    result = {}
    for name, (path, digest) in TABLES.items():
        if path not in entries: p.error('missing archive entry ' + path)
        data = archive.read(entries[path])
        if hashlib.sha256(data).hexdigest() != digest: p.error('table authentication failed: ' + path)
        result[name] = data
    if len(result['orders.dat']) != 189 * 22:
        p.error('orders table size does not match the pinned 189-row schema')
    out.mkdir(parents=True, exist_ok=False)
    for name, data in result.items():
        with (out / name).open('xb') as f: f.write(data)
    with (out / 'receipt.json').open('x') as f:
        json.dump({'rom_sha1': EXPECTED_ROM_SHA1, 'tables': {name: {'path': path, 'sha256': digest, 'bytes': len(result[name])} for name, (path, digest) in TABLES.items()}}, f, indent=2)
        f.write('\n')

if __name__ == '__main__': main()
