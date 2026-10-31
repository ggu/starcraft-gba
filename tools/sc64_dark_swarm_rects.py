#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate private raw-unit source rectangles from authenticated units.dat."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UNITS_SHA256 = '54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df'
UNIT_COUNT = 228
# The pinned OpenBW units.dat loader reads 228 records in its dimensions
# column, each a little-endian four-int16 rectangle, at this byte offset.
DIMENSIONS_OFFSET = 12580


def generate(units_path, out):
    target = out.resolve()
    if target.is_relative_to(ROOT) and not target.is_relative_to(ROOT / '.local'):
        raise ValueError('output inside the source tree must be under .local')
    data = units_path.read_bytes()
    if hashlib.sha256(data).hexdigest() != UNITS_SHA256 or len(data) != 19876:
        raise ValueError('units.dat does not match the authenticated source table')
    rows = [struct.unpack_from('<hhhh', data, DIMENSIONS_OFFSET + i * 8)
            for i in range(UNIT_COUNT)]
    if any(any(v < 0 or v > 255 for v in row) for row in rows):
        raise ValueError('source dimensions exceed the compact representation')
    if [rows[i] for i in (0, 46, 52, 202)] != [
            (8, 9, 8, 10), (13, 12, 13, 12), (14, 14, 14, 14), (80, 80, 79, 79)]:
        raise ValueError('source dimension sentinels differ')
    lines = ['/* Generated from authenticated private source; do not commit. */',
             '#ifndef SC64_DARK_SWARM_RECTS_GENERATED_H',
             '#define SC64_DARK_SWARM_RECTS_GENERATED_H', '#include <stdint.h>',
             '#define SC64_DARK_SWARM_RECTS_FORMAT 1',
             '#define SC64_DARK_SWARM_RECTS_COUNT 228',
             'static const uint8_t sc64_dark_swarm_rects[228][4] = {']
    lines.extend('    {' + ','.join(map(str, row)) + '},' for row in rows)
    lines.extend(['};', '#endif', ''])
    out.mkdir(parents=True, exist_ok=True)
    header = out / 'dark_swarm_rects_generated.h'
    with header.open('x', encoding='ascii') as f:
        f.write('\n'.join(lines))
    return {'units_sha256': UNITS_SHA256, 'rows': len(rows),
            'header_sha256': hashlib.sha256(header.read_bytes()).hexdigest()}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--units-dat', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    print(json.dumps(generate(a.units_dat, a.out), indent=2))


if __name__ == '__main__':
    main()
