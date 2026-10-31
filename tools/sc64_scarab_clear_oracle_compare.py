#!/usr/bin/env python3
"""Compare clear-motion fixture output to retained pinned OpenBW traces."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("fixture")
parser.add_argument("far")
parser.add_argument("south")
args = parser.parse_args()
rows = {}
for line in Path(args.fixture).read_text().splitlines():
    heading, age, x, y = map(int, line.split())
    rows[heading, age] = (x, y)
for heading, path in ((64, args.far), (128, args.south)):
    observed = []
    for line in Path(path).read_text().splitlines():
        match = re.search(r"shot_order=65 state=(\d+) pos=(\d+),(\d+)", line)
        if match:
            observed.append(tuple(map(int, match.groups())))
    start = next(i for i, row in enumerate(observed) if row[0] == 2)
    for age in range(4):
        assert rows[heading, age] == observed[start + age][1:]
print("aligned far and south source-frame positions match")
