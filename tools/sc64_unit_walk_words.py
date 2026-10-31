#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Derive an optional bounded walk-word bank from prepared walk/facing headers."""
import argparse
import pathlib
import hashlib
import base64
import json
import os
import tempfile
import re
from sc64_unit_facing import pack_word_phases
from sc64_weapon_lookup_index import validate_output

# Hot ground actors only; every other actor continues to use its original pose.
DEFAULT_UNITS = (0, 32, 37, 38, 41, 65)
MANAGED_MARKER = "/* Managed private walk-word sidecars v1; do not commit. */\n"


def derive(walk: str, facing: str, units=DEFAULT_UNITS) -> str:
    pixel_re = r'static const unsigned char (\w+)\[(\d+)\] = \{(.*?)\n\};'
    pixels = {}
    for name, count, body in re.findall(pixel_re, facing + '\n' + walk, re.S):
        tokens = body.split(',')
        if tokens and not tokens[-1].strip(): tokens.pop()
        if name in pixels or len(tokens) != int(count):
            raise ValueError('duplicate or malformed pixel array: ' + name)
        values = []
        for token in tokens:
            value = token.strip()
            if not re.fullmatch(r'(?:0[xX][0-9a-fA-F]+|[0-9]+)[uU]?', value):
                raise ValueError('invalid pixel token: ' + value)
            number = int(value.rstrip('uU'), 0)
            if number > 255: raise ValueError('pixel out of byte range')
            values.append(number)
        pixels[name] = bytes(values)
    def unique(rows, label):
        result = {}
        for name, value in rows:
            if name in result: raise ValueError('duplicate ' + label + ': ' + str(name))
            result[name] = value
        return result
    poses = unique(re.findall(r'static const Sc64SpriteFrame (\w+)\[32\] = \{(.*?)\n\};', walk, re.S), 'pose')
    cycle_rows = re.findall(r'static const Sc64UnitWalkStep (\w+)_steps\[(\d+)\] = \{(.*?)\n\};', walk, re.S)
    cycles = unique(((name,(int(count),body)) for name,count,body in cycle_rows), 'cycle')
    raw_rows = [(int(raw),name) for raw,name in re.findall(r'case (\d+): return &(sc64_unit_walk_i\d+);', walk)]
    raw_map = unique(raw_rows, 'raw ID')
    for name, (count, body) in cycles.items():
        matches = re.findall(r'\{\d+u,(\w+)\}', body)
        residue = re.sub(r'\{\d+u,\w+\}', '', body)
        if len(matches) != count or not matches or residue.strip(' ,\t\r\n'):
            raise ValueError('malformed or empty cycle: ' + name)
        if any(pose not in poses for pose in matches): raise ValueError('cycle references missing pose: ' + name)
    if any(cycle not in cycles for cycle in raw_map.values()): raise ValueError('raw ID references missing cycle')
    if len(set(units)) != len(units) or any(raw not in raw_map for raw in units):
        raise ValueError('selected raw IDs must be unique and present')
    selected = {}
    for raw in units:
        count, body = cycles[raw_map[raw]]
        steps = re.findall(r'\{\d+u,(\w+)\}', body)
        residue = re.sub(r'\{\d+u,\w+\}', '', body)
        if len(steps) != count or not steps or residue.strip(' ,\t\r\n'):
            raise ValueError('malformed or empty cycle')
        selected[raw] = steps
    lines = ['/* Private derived data; do not commit. */', '#ifndef SC64_WALK_WORDS_GENERATED_H', '#define SC64_WALK_WORDS_GENERATED_H']
    emitted, pose_words = {}, {}
    for pose in dict.fromkeys(p for steps in selected.values() for p in steps):
        entries = re.findall(r'\{(-?\d+),(-?\d+),(\d+),(\d+),(\d+),(\d+),(\w+)\}', poses[pose])
        residue = re.sub(r'\{(-?\d+),(-?\d+),(\d+),(\d+),(\d+),(\d+),(\w+)\}', '', poses[pose])
        if len(entries) != 32 or residue.strip(' ,\t\r\n'):
            raise ValueError('pose must contain exactly 32 frames: ' + pose)
        symbols = []
        for _, _, width, height, _, _, pixel in entries:
            width, height = int(width), int(height)
            data = pixels.get(pixel)
            if not data or len(data) != width * height: raise ValueError('invalid frame ' + pixel)
            key = (width, height, data)
            if key not in emitted:
                symbol = 'sc64_walk_word_%d' % len(emitted); emitted[key] = symbol
                phases = pack_word_phases(width, height, data)
                for phase, words in enumerate(phases):
                    lines.append('static const u32 %s_p%d[] = {%s};' %
                                 (symbol, phase, ','.join('0x%08xu' % w for w in words)))
                lines.append('static const GbaSpriteWordFrame %s = {%d,%d,{%s}};' %
                             (symbol,width,height,','.join('%s_p%d' % (symbol,p) for p in range(4))))
            symbols.append('&' + emitted[key])
        table = pose + '_word_table'; pose_words[pose] = table
        lines.append('static const GbaSpriteWordFrame *const %s[32] = {%s};' % (table, ','.join(symbols)))
    for raw, steps in selected.items():
        lines.append('static const GbaSpriteWordFrame *const *const sc64_walk_raw_%d_words[] = {%s};' %
                     (raw, ','.join(pose_words[p] for p in steps)))
    lines += ['static inline const GbaSpriteWordFrame *sc64_walk_word_frame(unsigned raw, unsigned step, unsigned dir) {',
              '    if (dir >= 32) return 0;', '    switch (raw) {']
    for raw, steps in selected.items():
        lines.append('    case %d: return step < %d ? sc64_walk_raw_%d_words[step][dir] : 0;' % (raw,len(steps),raw))
    return '\n'.join(lines + ['    default: return 0;', '    }', '}', '#endif', ''])


def config_key(mode, walk, facing):
    if mode not in ('0', '1'): raise ValueError('mode must be 0 or 1')
    paths = [walk, facing, pathlib.Path(__file__), pathlib.Path(__file__).with_name('sc64_unit_facing.py')]
    items = [(str(p.resolve()), hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None) for p in paths]
    return base64.urlsafe_b64encode(json.dumps([mode, DEFAULT_UNITS, items], separators=(',', ':')).encode()).decode()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--managed-update', action='store_true')
    parser.add_argument('--config-key', choices=('0','1'))
    parser.add_argument('walk', type=pathlib.Path); parser.add_argument('facing', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path, nargs='?'); args=parser.parse_args()
    if args.config_key is not None:
        if args.output or args.managed_update: parser.error('config key does not write output')
        print(config_key(args.config_key,args.walk,args.facing)); return
    if args.output is None: parser.error('output is required')
    # Reuse repository/alias checks and require an ownership marker before replacement.
    if args.output.is_symlink() or args.output.resolve() in (args.facing.resolve(strict=True),args.walk.resolve(strict=True)):
        raise ValueError('output aliases facing source or is a symlink')
    if args.output.exists():
        if not args.managed_update or not args.output.is_file() or not args.output.read_text().startswith(MANAGED_MARKER):
            raise FileExistsError('output exists and is not managed')
        # Validate the destination's ignored-path rules with an absent sibling.
        validate_output(args.walk,args.output.with_name(args.output.name+'.validation-probe'))
    else: validate_output(args.walk,args.output)
    data=derive(args.walk.read_text(), args.facing.read_text())
    if not args.managed_update:
        with args.output.open('x') as out: out.write(data)
        return
    tmp=None
    try:
        with tempfile.NamedTemporaryFile(mode='w',dir=args.output.parent,prefix='.walk-words-',delete=False) as f:
            tmp=pathlib.Path(f.name);f.write(MANAGED_MARKER+data)
        if args.output.is_symlink() or (args.output.exists() and not args.output.read_text().startswith(MANAGED_MARKER)):
            raise FileExistsError('destination changed during generation')
        os.replace(tmp,args.output)
    finally:
        if tmp is not None and tmp.exists(): tmp.unlink()
if __name__=='__main__': main()
