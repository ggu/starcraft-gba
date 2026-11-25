#!/usr/bin/env python3
"""Authenticate and inspect the USA N64 resource pickup/deposit code.

Read-only: the ROM is never copied or written. No extracted code is emitted.
The known region digests pin the instructions described in the accompanying
native gas audit; no third-party disassembler or local emulator is required.
SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse
import hashlib
import json
from pathlib import Path

ROM_SHA256 = 'e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
# File offsets refer to the normalized big-endian USA ROM. Region hashes bind
# the audited instruction sequences without storing or exporting their bytes.
REGIONS = {
    'resource_pickup_dispatch': (0xf6f9c, 444, '5f699d01046af17310a2196658a943e581d13531bba3d723f6c531fdcaa9aca7'),
    'cargo_assignment': (0xfa830, 156, 'd121fe9ec7d9917118ed6587ea6d3c57c3c1400b564c12d6fb66e4824153faa2'),
    'cargo_deposit': (0xfaa64, 248, '8e6f4d0c999764814ad86728db87635a0ad8975eb6c828b3a1033dbb58c186fe'),
    'conditional_bonus': (0x4ddb8, 128, '3c52f5eadec3755a6aa7609a6ae97cebdb5d90e28e56e5553b3951f884376889'),
}


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def verify_regions(rom, regions=REGIONS):
    """Reject truncated or changed regions, returning digests only."""
    digests = {}
    for name, (offset, size, expected) in regions.items():
        data = rom[offset:offset + size]
        if len(data) != size or sha256(data) != expected:
            raise ValueError('instruction region mismatch: ' + name)
        digests[name] = expected
    return digests


def audit(rom):
    if sha256(rom) != ROM_SHA256:
        raise ValueError('ROM is not the authenticated normalized USA image')
    regions = verify_regions(rom)
    return {
        'rom_sha256': ROM_SHA256,
        'region_sha256': regions,
        'resource_type_ids': {'refinery': 110, 'extractor': 149, 'assimilator': 157},
        'reserve': {'actor_offset': 204, 'width_bits': 16, 'unsigned': True},
        'base_gas_pickup': {'at_least_8': 8, 'less_than_8': 2,
                            'below_8_reserve_after': 0},
        'worker_cargo_offset': 203,
        'owner_gas_bank_address': '0x800b1d74',
        'evidence': 'authenticated ROM static analysis; no live depleted trip',
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    args = parser.parse_args()
    try:
        result = audit(args.rom.read_bytes())
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
