#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarize sampled GBA gas timers; input stays private and is not copied.

JSONL rows: [frame, elapsed_cpu_cycles, [[slot,unused,owner,site,timer,carry],...]].
Only observed 59-to-positive-cargo intervals are counted. Time assumes the
GBA clock rate; it is emulated elapsed time, not host wall time.
"""
import argparse
import json
import statistics
from collections import Counter
from pathlib import Path


def summarize(rows):
    starts, intervals = {}, []
    peak = Counter()
    frames = 0
    for frame, cycles, workers in rows:
        frames += 1
        sites = Counter()
        for slot, _, owner, site, timer, carry in workers:
            if timer > 0 and carry == 0:
                sites[site] += 1
            if timer == 59 and carry == 0:
                starts[slot] = (frame, cycles, site)
            elif timer == 0 and carry > 0 and slot in starts:
                start, begin, initial_site = starts.pop(slot)
                if initial_site == site:
                    intervals.append((frame - start, (cycles - begin) / 16777216))
        for site, count in sites.items():
            peak[site] = max(peak[site], count)
    return {'frames': frames, 'intervals': len(intervals),
            'median_seconds': statistics.median(x[1] for x in intervals) if intervals else None,
            'frame_spans': sorted({x[0] for x in intervals}),
            'peak_countdowns': dict(peak)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture', type=Path)
    args = p.parse_args()
    with args.capture.open() as f:
        print(json.dumps(summarize(json.loads(line) for line in f if line.strip()), sort_keys=True))


if __name__ == '__main__':
    main()
