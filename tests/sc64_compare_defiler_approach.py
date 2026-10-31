#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare actual-game clear-cardinal trace with retained OpenBW world runs."""
import argparse
import re
from pathlib import Path

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--actual', type=Path, required=True)
p.add_argument('--oracle-dir', type=Path, required=True)
p.add_argument('--allow-se', action='store_true')
a = p.parse_args()
actual = {}
pattern = re.compile(r'case=(\d+) t=(\d+) x=(-?\d+) y=(-?\d+) heading=(\d+) phase=(\d+) blocked=(\d+) qx=(-?\d+) qy=(-?\d+)')
for line in a.actual.read_text().splitlines():
    if m := pattern.fullmatch(line):
        case, tick, x, y, heading, phase, blocked, qx, qy = map(int, m.groups())
        actual[case, tick] = (x, y, heading, phase, blocked, qx, qy)
source_pattern = re.compile(r'sample t=(\d+).*? state=(\d+).*?heading=(-?\d+) xy=(-?\d+),(-?\d+)')
names = ('east', 'south', 'west', 'north', 'diagonal') if a.allow_se else ('east', 'south', 'west', 'north')
for case, name in enumerate(names):
    reference = {}
    for line in (a.oracle_dir / (name + '.log')).read_text().splitlines():
        if m := source_pattern.match(line):
            tick, state, heading, x, y = map(int, m.groups())
            if tick <= 60:
                reference[tick] = (x, y, heading % 256, state)
    assert len(reference) == 60, (name, 'missing source samples')
    for tick, (x, y, heading, state) in reference.items():
        got = actual.get((case, tick))
        assert got is not None, (name, tick, 'missing actual sample')
        assert got[:3] == (x, y, heading) and not got[4], (name, tick, got, (x, y, heading))
    source_debit = next(tick for tick, row in reference.items() if row[3] == 2)
    actual_debit = next(tick for tick in range(1, 61) if actual[case, tick][3])
    assert actual_debit == source_debit, (name, actual_debit, source_debit)
if not a.allow_se:
    assert actual[4, 1][4], 'diagonal must fail closed without waypoint support'
print(f'{len(names)} actual clear approaches match pinned-source positions/headings and debit ticks')
