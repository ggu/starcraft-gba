#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Derive an optional ROM lookup header from an existing source VFX bank.

No art is generated. The original bank is read-only and the output must be a
new file; use SC64_WEAPON_SOURCE_BANK_HEADER to select it for an experiment.
"""
import argparse
from pathlib import Path
import re


def make_index(source: str) -> str:
    def rows(name: str):
        match = re.search(r'\bsc64_weapon_source_' + name + r'\[(\d+)\]\s*=\s*\{(.*?)\n\};', source, re.S)
        if not match:
            raise ValueError(f'missing {name} table')
        result = [[int(v.strip(), 0) for v in row.split(',')]
                  for row in re.findall(r'\{([^{}]*)\}', match.group(2))]
        if len(result) != int(match.group(1)) or not result:
            raise ValueError(f'invalid {name} row count')
        return result

    images, anims = rows('images'), rows('animations')
    image_ids = [r[0] for r in images]
    animation_keys = [(r[0], r[1]) for r in anims]
    if image_ids != sorted(set(image_ids)) or animation_keys != sorted(set(animation_keys)):
        raise ValueError('tables must have unique sorted keys')
    maxid = max(image_ids[-1], animation_keys[-1][0])
    if image_ids[0] < 0 or animation_keys[0][0] < 0 or any(a < 0 for _, a in animation_keys) or maxid > 4095 or len(images) > 255 or len(anims) > 65535 or any(a > 255 for _, a in animation_keys):
        raise ValueError('bank exceeds bounded index representation')
    image_index = [0] * (maxid + 1)
    starts = [0] * (maxid + 1)
    counts = [0] * (maxid + 1)
    for i, image in enumerate(image_ids): image_index[image] = i + 1
    for i, (image, _) in enumerate(animation_keys):
        if not counts[image]: starts[image] = i
        counts[image] += 1
        if counts[image] > 255: raise ValueError('animation span too large')
    def array(name, typ, data):
        return f'static const {typ} {name}[] = {{' + ','.join(map(str, data)) + '};\n'
    tables = (array('sc64_vfx_image_index', 'uint8_t', image_index) +
              array('sc64_vfx_anim_start', 'uint16_t', starts) +
              array('sc64_vfx_anim_count', 'uint8_t', counts))
    names = ('sc64_weapon_source_image_lookup', 'sc64_weapon_source_animation_lookup')
    patterns = [r'^static inline const Sc64WeaponSourceImage \*' + names[0] + r'\([^\n]*$',
                r'^static inline const Sc64WeaponSourceAnimation \*' + names[1] + r'\([^\n]*$']
    replacements = [tables + f'''static inline const Sc64WeaponSourceImage *{names[0]}(uint16_t id) {{
    if (id > {maxid}u) return 0;
    unsigned i = sc64_vfx_image_index[id];
    return i ? &sc64_weapon_source_images[i-1] : 0;
}}''', f'''static inline const Sc64WeaponSourceAnimation *{names[1]}(uint16_t image_id, uint8_t animation_id) {{
    if (image_id > {maxid}u) return 0;
    unsigned lo=sc64_vfx_anim_start[image_id], end=lo+sc64_vfx_anim_count[image_id], hi=end;
    while (lo<hi) {{ unsigned m=lo+(hi-lo)/2; if (sc64_weapon_source_animations[m].animation_id<animation_id) lo=m+1; else hi=m; }}
    return lo<end && sc64_weapon_source_animations[lo].animation_id==animation_id ? &sc64_weapon_source_animations[lo] : 0;
}}''']
    for pattern, replacement in zip(patterns, replacements):
        source, count = re.subn(pattern, lambda _: replacement, source, flags=re.M)
        if count != 1: raise ValueError('unsupported lookup function format')
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    derived = make_index(args.source.read_text())
    with args.output.open('x') as out: out.write(derived)

if __name__ == '__main__': main()
