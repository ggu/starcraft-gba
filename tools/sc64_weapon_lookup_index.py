#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Derive an optional ROM lookup header from an existing source VFX bank.

No art is generated. The original bank is read-only. Outputs must be new files
unless --managed-update atomically replaces a previous output from that mode.
"""
import argparse
from pathlib import Path
import re
import subprocess
import os
import tempfile
import base64
import hashlib
import json


def make_index(source: str) -> str:
    def rows(name: str):
        match = re.search(r'\bsc64_weapon_source_' + name + r'\[(\d+)\]\s*=\s*\{(.*?)\n\};', source, re.S)
        if not match:
            raise ValueError(f'missing {name} table')
        result = [[int(v.strip(), 0) for v in row.split(',')]
                  for row in re.findall(r'\{([^{}]*)\}', match.group(2))]
        macro = {'images': 'IMAGE', 'animations': 'ANIMATION'}[name]
        counts = re.findall(r'^#define\s+SC64_WEAPON_SOURCE_' + macro + r'_COUNT\s+(\d+)\s*$', source, re.M)
        if len(counts) != 1 or int(counts[0]) == 0 or int(counts[0]) != len(result) or len(result) != int(match.group(1)):
            raise ValueError(f'unsupported {name} logical or physical count')
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
    patterns = [r'^static inline const Sc64WeaponSourceImage \*' + names[0] + r'\([^\n]*\)[ \t]*\{[^\n]*\}[ \t]*$',
                r'^static inline const Sc64WeaponSourceAnimation \*' + names[1] + r'\([^\n]*\)[ \t]*\{[^\n]*\}[ \t]*$']
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
        def replace(match):
            original = match.group(0)
            if original.count('{') != original.count('}'):
                raise ValueError('unbalanced lookup function')
            return replacement
        source, count = re.subn(pattern, replace, source, flags=re.M)
        if count != 1: raise ValueError('unsupported lookup function format')
    return source


MANAGED_MARKER = "/* sc64-weapon-lookup-index managed output v1 */\n"


def validate_output(source: Path, output: Path, managed: bool = False) -> None:
    # Check both the spelling and the resolved path, so an ignored symlink
    # cannot redirect generated private material into a tracked tree.
    source_resolved = source.resolve(strict=True)
    if output.is_symlink() or output.resolve() == source_resolved:
        raise FileExistsError(f'output aliases source or is a symlink: {output}')
    if output.exists() and (not managed or not output.is_file() or not output.read_text().startswith(MANAGED_MARKER)):
        raise FileExistsError(f'output already exists and is not managed: {output}')
    for path in (output.absolute(), output.resolve()):
        parent = path.parent
        while not parent.exists():
            parent = parent.parent
        result = subprocess.run(['git', '-C', str(parent), 'rev-parse', '--show-toplevel'],
                                text=True, capture_output=True)
        if result.returncode:
            continue
        root = Path(result.stdout.strip())
        try:
            relative = path.relative_to(root)
        except ValueError:
            continue
        ignored = subprocess.run(['git', '-C', str(root), 'check-ignore', '--no-index', '-q', '--', str(relative)], capture_output=True)
        if ignored.returncode != 0:
            raise ValueError(f'output inside a repository must be ignored: {output}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config-key', nargs=2, metavar=('MODE', 'SOURCE'),
                        help='print a read-only content key for Make invalidation')
    parser.add_argument('--managed-update', action='store_true', help='atomically replace a prior managed output')
    parser.add_argument('source', type=Path, nargs='?')
    parser.add_argument('output', type=Path, nargs='?')
    args = parser.parse_args()
    if args.config_key is not None:
        if args.source is not None or args.output is not None or args.managed_update:
            parser.error('--config-key cannot be combined with generation')
        mode, name = args.config_key
        if mode not in ('0', '1'):
            parser.error('lookup mode must be 0 or 1')
        source = Path(name)
        digest = hashlib.sha256(source.read_bytes()).hexdigest() if source.is_file() else None
        key = json.dumps([mode, str(source.resolve()), digest], separators=(',', ':')).encode()
        print(base64.urlsafe_b64encode(key).decode())
        return
    if args.source is None or args.output is None:
        parser.error('source and output are required for generation')
    validate_output(args.source, args.output, args.managed_update)
    derived = make_index(args.source.read_text())
    if not args.managed_update:
        with args.output.open('x') as out: out.write(derived)
        return
    # Build the complete candidate before replacing a prior managed output.
    data = MANAGED_MARKER + derived
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode='w', dir=args.output.parent, prefix='.lookup-', delete=False) as out:
            temporary = Path(out.name)
            out.write(data)
        validate_output(args.source, args.output, True)
        os.replace(temporary, args.output)
    finally:
        if temporary is not None and temporary.exists(): temporary.unlink()

if __name__ == '__main__': main()
