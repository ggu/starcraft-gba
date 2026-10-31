#!/usr/bin/env python3
"""Compare Q8 clear-waypoint motion with authenticated OpenBW logs."""
import argparse
import re
import subprocess
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument("probe")
p.add_argument("logs", nargs="+")
a = p.parse_args()
pattern = re.compile(r"shot_order=(\d+) state=(\d+) pos=\d+,\d+ exact=(\d+),(\d+) heading=(\d+) vel_heading=(\d+) desired=\d+ next=(\d+),(\d+)")
for name in a.logs:
    rows = [tuple(map(int, m.groups())) for line in Path(name).read_text().splitlines()
            if (m := pattern.search(line))]
    start = next(i for i, row in enumerate(rows) if row[:2] == (65, 2))
    end = next(i for i in range(start + 1, len(rows)) if rows[i][:2] == (65, 6))
    initial, expected = rows[start], rows[start + 1:end + 1]
    inputs = "".join(f"{row[6]} {row[7]}\n" for row in expected)
    result = subprocess.run([a.probe, str(initial[2]), str(initial[3]), str(initial[4])],
                            input=inputs, text=True, capture_output=True, check=True)
    actual = [tuple(map(int, line.split())) for line in result.stdout.splitlines()]
    assert actual == [row[2:6] for row in expected], name
    print(f"{name}: {len(actual)} source ticks match")
