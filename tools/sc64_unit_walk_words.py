#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Derive an optional bounded walk-word bank from prepared walk/facing headers."""
import argparse
import pathlib
import re
from sc64_unit_facing import pack_word_phases
from sc64_weapon_lookup_index import validate_output

# Hot ground actors only; every other actor continues to use its original pose.
DEFAULT_UNITS = (0, 32, 37, 38, 41, 65)


def derive(walk: str, facing: str, units=DEFAULT_UNITS) -> str:
    pixel_re = r'static const unsigned char (\w+)\[(\d+)\] = \{(.*?)\n\};'
    pixels = {}
    for name, count, body in re.findall(pixel_re, facing + '\n' + walk, re.S):
        values = re.findall(r'0x([0-9a-fA-F]{2})', body)
        if name in pixels or len(values) != int(count):
            raise ValueError('duplicate or malformed pixel array: ' + name)
        pixels[name] = bytes(int(x, 16) for x in values)
    poses = dict(re.findall(r'static const Sc64SpriteFrame (\w+)\[32\] = \{(.*?)\n\};', walk, re.S))
    cycles = dict(re.findall(r'static const Sc64UnitWalkStep (\w+)_steps\[\d+\] = \{(.*?)\n\};', walk, re.S))
    raw_map = {int(raw): name for raw, name in re.findall(r'case (\d+): return &(sc64_unit_walk_i\d+);', walk)}
    if len(set(units)) != len(units) or any(raw not in raw_map for raw in units):
        raise ValueError('selected raw IDs must be unique and present')
    selected = {}
    for raw in units:
        steps = re.findall(r'\{\d+u,(\w+)\}', cycles[raw_map[raw]])
        if not steps: raise ValueError('empty cycle')
        selected[raw] = steps
    lines = ['/* Private derived data; do not commit. */', '#ifndef SC64_WALK_WORDS_GENERATED_H', '#define SC64_WALK_WORDS_GENERATED_H']
    emitted, pose_words = {}, {}
    for pose in dict.fromkeys(p for steps in selected.values() for p in steps):
        entries = re.findall(r'\{(-?\d+),(-?\d+),(\d+),(\d+),(\d+),(\d+),(\w+)\}', poses[pose])
        if len(entries) != 32: raise ValueError('pose must contain 32 frames: ' + pose)
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


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('walk', type=pathlib.Path); parser.add_argument('facing', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path); args=parser.parse_args()
    validate_output(args.walk,args.output)
    if args.output.resolve() == args.facing.resolve(strict=True): raise ValueError('output aliases facing source')
    data=derive(args.walk.read_text(), args.facing.read_text())
    with args.output.open('x') as out: out.write(data)
if __name__=='__main__': main()
