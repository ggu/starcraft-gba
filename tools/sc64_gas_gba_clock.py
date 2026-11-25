#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarize sampled GBA gas timers; input stays private and is not copied.

JSONL rows: [frame, elapsed_cpu_cycles, [[slot,unit_kind,owner,site,timer,carry],...]].
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
    peak_progress = Counter()
    parallel_frames = 0
    last_workers = {}
    frames = 0
    previous_frame = previous_cycles = None
    for frame, cycles, workers in rows:
        if previous_frame is not None:
            if frame <= previous_frame or cycles <= previous_cycles:
                raise ValueError("non-monotone frame or cycle count")
            if frame != previous_frame + 1:
                starts.clear()
                last_workers.clear()
        previous_frame, previous_cycles = frame, cycles
        if len({w[0] for w in workers}) != len(workers):
            raise ValueError("duplicate worker slot")
        frames += 1
        sites = Counter()
        advancing = Counter()
        current_workers = {}
        for slot, kind, owner, site, timer, carry in workers:
            worker = (slot, kind, owner, site)
            current_workers[worker] = (timer, carry)
            if worker in last_workers:
                old_timer, old_carry = last_workers[worker]
                starting = old_timer == 0 and timer == 59 and carry == 0
                counting = timer > 0 and old_timer == timer + 1 and carry == 0
                collecting = old_timer == 1 and timer == 0 and carry > 0
                if old_carry == 0 and (starting or counting or collecting):
                    advancing[site] += 1
            if timer > 0 and carry == 0:
                sites[site] += 1
            identity = (kind, owner, site)
            previous = starts.get(slot)
            if previous is not None:
                start, begin, prior_identity, prior_timer = previous
                valid = (identity == prior_identity and
                         ((carry == 0 and timer == prior_timer - 1 and timer > 0) or
                          (carry > 0 and timer == 0 and prior_timer == 1)))
                if valid and carry > 0:
                    intervals.append((frame - start, (cycles - begin) / 16777216))
                    del starts[slot]
                elif valid:
                    starts[slot] = (start, begin, identity, timer)
                else:
                    del starts[slot]
            if timer == 59 and carry == 0:
                starts[slot] = (frame, cycles, identity, timer)
        for site, count in advancing.items():
            peak_progress[site] = max(peak_progress[site], count)
        parallel_frames += len(advancing) >= 2
        last_workers = current_workers
        for slot in set(starts) - {w[0] for w in workers}:
            del starts[slot]
        for site, count in sites.items():
            peak[site] = max(peak[site], count)
    return {'frames': frames, 'intervals': len(intervals),
            'median_seconds': statistics.median(x[1] for x in intervals) if intervals else None,
            'frame_spans': sorted({x[0] for x in intervals}),
            'peak_countdowns': dict(peak), 'peak_advancing': dict(peak_progress),
            'parallel_site_frames': parallel_frames}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture', type=Path)
    args = p.parse_args()
    with args.capture.open() as f:
        print(json.dumps(summarize(json.loads(line) for line in f if line.strip()), sort_keys=True))


if __name__ == '__main__':
    main()
