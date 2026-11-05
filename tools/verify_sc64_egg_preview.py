#!/usr/bin/env python3
"""Verify an opt-in custom Egg ROM and the normal GBA RAM reserve bounds."""
import argparse
from pathlib import Path
import subprocess
import verify_roms

PROFILES = {
    'terran': verify_roms.TARGETS['--terran'],
    'zerg': verify_roms.TARGETS['--zerg-z9'],
    'z7': verify_roms.TARGETS['--zerg-z7'],
    'protoss': verify_roms.TARGETS['--protoss-p10'],
    'showcase': ('StarCraft-Showcase-Transport1-GBA', 'starcraft-showcase', 'SCSH', 'SC SHOWCASE'),
}

def ram_ends(text):
    ends = {}
    for line in text.splitlines():
        words = line.split()
        if len(words) >= 3 and words[-1] in ('__bss_end', '__iwram_end'):
            if words[-1] in ends:
                raise ValueError('duplicate RAM end symbol')
            ends[words[-1]] = int(words[0], 16)
    if set(ends) != {'__bss_end', '__iwram_end'}:
        raise ValueError('missing RAM end symbol')
    if ends['__bss_end'] >= 0x0203e000 or ends['__iwram_end'] >= 0x03005000:
        raise ValueError('GBA RAM reserve exhausted')
    return ends

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('profile', choices=PROFILES)
    args = parser.parse_args()
    verify_roms.ROOT = args.root.resolve()
    name, elf, code, title = PROFILES[args.profile]
    result = verify_roms.verify(name, elf, code, title)
    symbols = subprocess.check_output([verify_roms.executable('nm'), '-n',
                                       str(verify_roms.ROOT / 'build' / (elf + '.elf'))], text=True)
    ends = ram_ends(symbols)
    print(f"{args.profile}: {result['bytes']} bytes SHA-256 {result['sha256']}; "
          f"EWRAM end {ends['__bss_end']:#010x}; IWRAM end {ends['__iwram_end']:#010x}")

if __name__ == '__main__':
    main()
