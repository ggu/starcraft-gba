#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check sampled gas contention. Input is JSONL [field,tick,[[worker,order,site],...]].

Order 82 means waiting and 83 means service. IDs are opaque. A worker must be
observed waiting before a grant can prove its place. Simultaneous first samples
are ties; no ordering within a tie is inferred. The first sample may be mid-job.
"""
import argparse
import json
from collections import defaultdict
from pathlib import Path


def analyze(rows, first_tick=None, last_tick=None):
    if first_tick is not None and last_tick is not None and first_tick > last_tick:
        raise ValueError("invalid tick window")
    waiting = {}  # (worker, site) -> first observed field
    prior_active = set()
    grants = []
    peak = 0
    previous_field = previous_tick = -1
    for field, tick, actors in rows:
        if field <= previous_field or tick < previous_tick:
            raise ValueError("unordered observation")
        previous_field, previous_tick = field, tick
        if len({worker for worker, _, _ in actors}) != len(actors):
            raise ValueError(f"duplicate worker at field {field}")
        current = {(w, site) for w, order, site in actors if order == 82}
        active = {(w, site) for w, order, site in actors if order == 83}
        sites = defaultdict(int)
        for _, site in active:
            sites[site] += 1
        in_window = (first_tick is None or tick >= first_tick) and (last_tick is None or tick <= last_tick)
        if in_window and any(n > 1 for n in sites.values()):
            raise ValueError(f'multiple active workers at field {field}')
        for key in current:
            waiting.setdefault(key, field)
        for worker, site in active - prior_active:
            key = worker, site
            if first_tick is not None and tick < first_tick:
                continue
            if last_tick is not None and tick > last_tick:
                continue
            if key not in waiting:
                raise ValueError(f'grant without observed wait at field {field}')
            candidates = [waiting[k] for k in current if k[1] == site]
            oldest = min(candidates + [waiting[key]])
            if waiting[key] != oldest:
                raise ValueError(f'older waiter bypassed at field {field}')
            grants.append(tick)
        if in_window:
            peak = max(peak, max((sum(1 for _, s in current if s == site)
                                  for site in {s for _, s in current}), default=0))
        # Service ends this wait. A direct return to 82 is a new arrival.
        waiting = {k: v for k, v in waiting.items() if k in current}
        prior_active = active
    return {'grants': grants, 'peak_waiters': peak}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture', type=Path)
    p.add_argument('--first-tick', type=int)
    p.add_argument('--last-tick', type=int)
    args = p.parse_args()
    with args.capture.open() as f:
        print(json.dumps(analyze((json.loads(line) for line in f if line.strip()),
                                 args.first_tick, args.last_tick)))


if __name__ == '__main__':
    main()
