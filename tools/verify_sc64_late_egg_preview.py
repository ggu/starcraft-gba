#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify isolated opt-in late-campaign Egg products and normal RAM reserves."""
import argparse
from pathlib import Path
import subprocess
import verify_roms
from verify_sc64_egg_preview import ram_ends

PROFILES = {'zerg': '--zerg-z9', 'z10': '--zerg-z10'}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('profile', choices=PROFILES)
    args = parser.parse_args()
    verify_roms.ROOT = args.root.resolve()
    name, elf, code, title = verify_roms.TARGETS[PROFILES[args.profile]]
    result = verify_roms.verify(name, elf, code, title)
    symbols = subprocess.check_output([verify_roms.executable('nm'), '-n',
        str(verify_roms.ROOT / 'build' / (elf + '.elf'))], text=True)
    ends = ram_ends(symbols)
    print(f"{args.profile}: {result['bytes']} bytes SHA-256 {result['sha256']}; "
          f"EWRAM end {ends['__bss_end']:#010x}; IWRAM end {ends['__iwram_end']:#010x}")

if __name__ == '__main__':
    main()
