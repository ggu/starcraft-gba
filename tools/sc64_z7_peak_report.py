#!/usr/bin/env python3
"""Summarize probe-only Z7D peak records, separate from replay qualification.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re

CPU_HZ = 16_777_216
COMPONENTS = ('orders', 'sight', 'mission', 'production', 'state', 'defenses',
              'research', 'remainder')
FIELDS = {
    'PEAK': ('call', 'tick', 'phase', 'before', 'frame', 'screen0', 'screen',
             'held', 'total', 'update', 'advancing', 'end'),
    'STAGES': ('call', 'tick', *COMPONENTS, 'end'),
    'STOP': ('calls', 'tick', 'end'),
}


def summarize(text: str) -> dict:
    pairs: dict[int, dict] = {}
    stop = None
    order = []
    for raw in text.splitlines():
        if 'Z7D ' not in raw:
            continue
        line = raw[raw.index('Z7D '):].strip()
        words = line.split()
        if len(words) < 3 or words[1] not in FIELDS:
            raise ValueError('unknown diagnostic record')
        kind = words[1]
        tokens = [re.fullmatch(r'(\w+)=(\d+)', token) for token in words[2:]]
        if any(token is None for token in tokens):
            raise ValueError('invalid diagnostic field')
        keys = tuple(token[1] for token in tokens)
        if keys != FIELDS[kind] or not line.endswith(' end=1'):
            raise ValueError('incomplete diagnostic record')
        values = {token[1]: int(token[2]) for token in tokens}
        if any(value > 0xffffffff for value in values.values()):
            raise ValueError('diagnostic value outside uint32')
        if kind == 'STOP':
            if stop is not None:
                raise ValueError('duplicate diagnostic stop')
            stop = values
            continue
        call = values['call']
        pair = pairs.setdefault(call, {})
        if kind in pair:
            raise ValueError('duplicate diagnostic observation')
        if kind == 'PEAK':
            if order and call <= order[-1]:
                raise ValueError('unordered diagnostic observations')
            order.append(call)
            if values['phase'] > 3 or values['advancing'] > 1 or values['update'] > values['total']:
                raise ValueError('invalid diagnostic context')
        elif 'PEAK' not in pair:
            raise ValueError('orphan diagnostic stages')
        pair[kind] = values
    rows = []
    for call in order:
        pair = pairs[call]
        if set(pair) != {'PEAK', 'STAGES'} or pair['PEAK']['tick'] != pair['STAGES']['tick']:
            raise ValueError('unpaired diagnostic observation')
        row = dict(pair['PEAK'])
        row['stages'] = {key: pair['STAGES'][key] for key in COMPONENTS}
        if row['phase'] == 2:
            context = 'HOME load'
        elif row['phase'] == 3:
            context = 'pause-menu save'
        elif row['screen0'] != 1:
            context = 'menu or mission transition'
        elif row['advancing']:
            context = 'active gameplay'
        else:
            context = 'non-advancing UI'
        row['context'] = context
        row['seconds'] = row['total'] / CPU_HZ
        rows.append(row)
    if not rows:
        raise ValueError('no complete peak observations')
    return {'scope': 'diagnostic maxima only; not a mission qualifier or unbiased timing baseline',
            'observations': rows, 'max_total': max(rows, key=lambda row: row['total']),
            'max_update': max(rows, key=lambda row: row['update']), 'stop': stop}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    try:
        result = summarize(args.log.read_text())
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
