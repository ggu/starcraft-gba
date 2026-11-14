#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Decode and validate the bounded pre-instruction result trace."""
import argparse
import csv
import json
from pathlib import Path
import struct
import sys

POINTS = {0x8008db2c: 'countdown', 0x8008db58: 'scan', 0x8008dbbc: 'owner',
          0x8008d9a8: 'publish', 0x801357a4: 'relink', 0x8008dbec: 'published',
          0x8008ce0c: 'reset'}
ROW = struct.Struct('>10I')
LIMIT = 1024 * ROW.size
MAX_STEPS = 256


def read_trace(path, begin, end):
    if not 0 <= begin < end or end - begin > MAX_STEPS:
        raise ValueError('invalid result trace window')
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


def inspect_cadence(rows):
    """Classify full scan pairs; tolerate a window opening/closing mid-scan."""
    scans, intervals = [], []
    values, resets = [], []
    previous = None
    for idx, (step, pc, arg0, arg1, countdown, *_) in enumerate(rows):
        event = POINTS[pc]
        if event == 'countdown':
            values.append(countdown)
        elif event == 'reset':
            resets.append({'row': idx, 'step': step, 'old_countdown': countdown})
        elif event == 'scan':
            anchor = (idx > 0 and rows[idx - 1][1] == 0x8008db2c and
                      rows[idx - 1][0] == step and rows[idx - 1][4] == 0 and
                      countdown == 0xffff)
            scan = {'row': idx, 'step': step, 'underflow_anchor': anchor,
                    'owners': [], 'writes': [], 'relinks': [], 'returned': False,
                    'ordering_issues': []}
            if previous is not None:
                # The following scan proves the previous one ended. A scan
                # without a live list can skip the result pass entirely.
                if (previous['owners'] or previous['writes']) and not previous['returned']:
                    previous['ordering_issues'].append('missing result-pass return')
                kind = 'unknown_anchor'
                if anchor and previous['underflow_anchor']:
                    if resets:
                        # At each reset helper store the next decrement
                        # starts at 1; more resets may restart that sequence.
                        last = resets[-1]['row']
                        tail = [r[4] for r in rows[last + 1:idx] if r[1] == 0x8008db2c]
                        kind = 'reset' if tail == [1, 0] else 'unexpected_reset_tail'
                    else:
                        kind = 'ordinary' if values == list(range(30, -1, -1)) else 'unexpected_countdown'
                intervals.append({'from_scan_row': previous['row'], 'to_scan_row': idx,
                                  'kind': kind, 'decrements': len(values), 'resets': resets})
            previous = scan
            scans.append(scan)
            values, resets = [], []
        elif previous is not None:
            issues = previous['ordering_issues']
            if event == 'owner':
                if previous['writes'] or previous['returned']:
                    issues.append('owner dispatch after result publication')
                if arg0 >= 8 or arg0 in previous['owners']:
                    issues.append('invalid or repeated owner dispatch')
                previous['owners'].append(arg0)
            elif event == 'publish':
                if not previous['owners']:
                    issues.append('result publication without owner dispatch')
                if previous['returned']:
                    issues.append('result publication after pass returned')
                previous['writes'].append({'row': idx, 'owner': arg0, 'result': arg1})
            elif event == 'relink':
                # Other game mechanics also change owners. Keep those rows
                # visible without treating them as defeat evidence.
                in_pass = bool(previous['writes']) and not previous['returned']
                previous['relinks'].append({'row': idx, 'actor': arg0, 'owner': arg1,
                                            'during_result_pass': in_pass})
            elif event == 'published':
                if previous['returned']:
                    issues.append('repeated result-pass return')
                previous['returned'] = True
    return {'scans': scans, 'intervals': intervals,
            'trailing_decrements': len(values), 'trailing_resets': resets}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('begin', type=int)
    parser.add_argument('end', type=int)
    parser.add_argument('--cadence', action='store_true', help='report scan anchors and intervals as JSON')
    args = parser.parse_args()
    rows = read_trace(args.trace, args.begin, args.end)
    if args.cadence:
        print(json.dumps(inspect_cadence(rows), indent=2))
        sys.exit(0)
    out = csv.writer(sys.stdout)
    out.writerow(('step', 'event', 'arg0', 'arg1', 'countdown', 'pending_0_to_3',
                  'pending_4_to_7', 'published_0_to_3', 'published_4_to_7', 'buildings_0_to_3'))
    for step, pc, arg0, arg1, countdown, *packed in rows:
        out.writerow((step, POINTS[pc], f'{arg0:08x}', f'{arg1:08x}', countdown,
                      *(f'{word:08x}' for word in packed)))
