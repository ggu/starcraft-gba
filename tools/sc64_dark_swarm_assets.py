#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Export authenticated Dark Swarm image 337 to a private generated header."""
import argparse
import hashlib
import json
from pathlib import Path
import sc64_yamato_assets as base


def generate(rom_path, out, remap_path, palette_path):
    target = out.resolve()
    if target.is_relative_to(base.ROOT) and not target.is_relative_to(base.ROOT / '.local'):
        raise ValueError('output inside the source tree must be under .local')
    rom = rom_path.read_bytes()
    if hashlib.sha1(rom).hexdigest() != base.ROM_SHA1:
        raise ValueError('ROM SHA-1 does not match authenticated StarCraft 64 USA')
    get, sprites = base.source._source_bolt(rom)
    base.IMAGES = ((337, 10, 9),)
    pixels, lut, rows = base.collect(get, sprites, remap_path.read_bytes(),
                                     json.loads(palette_path.read_text()))
    text = base.render(pixels, lut, rows).replace('YAMATO', 'DARK_SWARM').replace('Yamato', 'DarkSwarm').replace('yamato', 'dark_swarm')
    out.mkdir(parents=True, exist_ok=True)
    header = out / 'dark_swarm_source_generated.h'
    header.write_text(text, encoding='ascii')
    return {'frames': len(rows), 'pixel_bytes': len(pixels), 'lut_bytes': len(lut),
            'header_sha256': hashlib.sha256(header.read_bytes()).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('rom', 'out', 'palette-remap', 'palette-json'):
        parser.add_argument('--' + name, type=Path, required=True)
    a = parser.parse_args()
    print(json.dumps(generate(a.rom, a.out, a.palette_remap, a.palette_json), indent=2))


if __name__ == '__main__':
    main()
