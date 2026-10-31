#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare all generated private source rectangles with pinned OpenBW output."""
import argparse
import re
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--header', type=Path, required=True)
p.add_argument('--oracle', type=Path, required=True)
a = p.parse_args()
rows = [tuple(map(int, m.groups())) for line in a.header.read_text().splitlines()
        if (m := re.fullmatch(r'\s*\{(\d+),(\d+),(\d+),(\d+)\},', line))]
reference = {}
for line in a.oracle.read_text().splitlines():
    if m := re.fullmatch(r'(\d+) (\d+) (\d+) (\d+) (\d+)', line):
        i, *v = map(int, m.groups()); reference[i] = tuple(v)
assert len(rows) == len(reference) == 228
for i, row in enumerate(rows):
    assert row == reference[i], (i, row, reference[i])
print('228 generated source rectangles match the pinned OpenBW loader')
