#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Decode and validate the bounded pre-instruction result trace."""
import argparse
import csv
from pathlib import Path
import struct
import sys

POINTS = {0x8008db2c: 'countdown', 0x8008db58: 'scan', 0x8008dbbc: 'owner',
          0x8008d9a8: 'publish', 0x801357a4: 'relink', 0x8008dbec: 'published',
          0x8008ce0c: 'reset'}
ROW = struct.Struct('>10I')
LIMIT = 1024 * ROW.size


def read_trace(path, begin, end):
    with path.open('rb') as data:
        raw = data.read(LIMIT + 1)
    if not raw or len(raw) > LIMIT or len(raw) % ROW.size:
        raise ValueError('invalid result trace length')
    rows = list(ROW.iter_unpack(raw))
    last = begin
    for step, pc, *_ in rows:
        if not last <= step < end or pc not in POINTS:
            raise ValueError('result trace outside event/window contract')
        last = step
    if not any(row[1] == 0x8008db2c for row in rows):
        raise ValueError('trace lacks a dispatcher decrement')
    return rows


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('begin', type=int)
    parser.add_argument('end', type=int)
    args = parser.parse_args()
    out = csv.writer(sys.stdout)
    out.writerow(('step', 'event', 'arg0', 'arg1', 'countdown', 'pending_0_to_3',
                  'pending_4_to_7', 'published_0_to_3', 'published_4_to_7', 'buildings_0_to_3'))
    for step, pc, arg0, arg1, countdown, *packed in read_trace(args.trace,args.begin,args.end):
        out.writerow((step, POINTS[pc], f'{arg0:08x}', f'{arg1:08x}', countdown,
                      *(f'{word:08x}' for word in packed)))
