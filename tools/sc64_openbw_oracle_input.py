#!/usr/bin/env python3
"""Stage one authenticated private IScript input for a local OpenBW oracle.

SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_construction_sprites import load_sprites, EXPECTED_ROM_SHA1, TABLE_HASHES

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--extractor', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    out = args.out.resolve()
    if not out.is_relative_to((ROOT / '.local').resolve()):
        parser.error('output must be under this worktree .local')
    if out == args.rom.resolve() or out == args.extractor.resolve():
        parser.error('output must not overwrite an input')
    base = load_sprites(ROOT / 'tools/sc64_sprites.py')
    extractor = base.load_extractor(args.extractor)
    rom = extractor.load_rom(str(args.rom))
    if hashlib.sha1(rom).hexdigest() != EXPECTED_ROM_SHA1:
        parser.error('ROM authentication failed')
    archive = extractor.BoltArchive(rom)
    matches = [entry for entry in archive.entries() if entry.path == '012/002']
    if len(matches) != 1:
        parser.error('IScript entry missing or ambiguous')
    data = archive.read(matches[0])
    digest = hashlib.sha256(data).hexdigest()
    if digest != TABLE_HASHES['012/002']:
        parser.error('IScript authentication failed')
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists():
        parser.error('output already exists; choose a new ignored path')
    with out.open('xb') as f:
        f.write(data)
    print('rom_sha1=' + EXPECTED_ROM_SHA1)
    print('iscript_sha256=' + digest)

if __name__ == '__main__':
    main()
