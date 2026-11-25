#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarise capture spike runs (S1 hashes, S2 clocks, S3 tap records).

Reads private run directories written by capture.py and prints counts and
pass/fail lines only; it never copies guest bytes out of the run directory.
"""
import argparse
import collections
import csv
import pathlib
import struct

ROW = 312


def read_csv(path):
    with open(path, newline='') as handle:
        return list(csv.DictReader(handle))


def s1(a, b):
    import json
    manifests = []
    for run in (a, b):
        manifest = run / 'manifest.json'
        if not manifest.is_file() or not json.loads(manifest.read_text()).get('ok'):
            print(f'S1 {run.name}: run missing or not ok; no verdict')
            return
        manifests.append(json.loads(manifest.read_text()))
    keys = ('tape_sha256', 'flash_input_sha256', 'binary_sha256', 'mode', 'rom_sha256')
    same_inputs = all(manifests[0].get(k) == manifests[1].get(k) for k in keys)
    left = {r['step']: r['sha256'] for r in read_csv(a / 'checkpoints.csv')}
    right = {r['step']: r['sha256'] for r in read_csv(b / 'checkpoints.csv')}
    shared = sorted(set(left) & set(right), key=int)
    differ = [s for s in shared if left[s] != right[s]]
    print(f'S1 checkpoints compared={len(shared)} last={shared[-1] if shared else None} differ={len(differ)}')
    if differ:
        first = differ[0]
        print(f'S1 first divergent checkpoint step={first}')
        regions = {}
        for name, run in (('a', a), ('b', b)):
            with open(run / 'regions.csv') as handle:
                for line in handle:
                    parts = line.strip().split(',')
                    if parts[0] == first:
                        regions[name] = parts[1:]
        if len(regions) == 2:
            blocks = [i for i, (x, y) in enumerate(zip(regions['a'], regions['b'])) if x != y]
            print('S1 differing 64 KiB blocks at', ', '.join(f'0x{i * 65536:06x}' for i in blocks[:32]))
    elif not shared or set(left) != set(right):
        print(f'S1 incomplete: checkpoints only in one run: {len(set(left) ^ set(right))}')
    elif not same_inputs:
        print('S1 overlap equal, but tape, save, binary, ROM or mode differ: no determinism verdict')
    else:
        print('S1 PASS')


def raw_records(path):
    """Yield (step, tick, phase, rows{index: bytes}) from ticks.raw."""
    with open(path, 'rb') as handle:
        while True:
            head = handle.read(16)
            if len(head) < 16:
                return
            step, tick, phase, live = struct.unpack('<4I', head)
            body = handle.read(live * (2 + ROW))
            rows = {}
            for i in range(live):
                at = i * (2 + ROW)
                rows[struct.unpack_from('>H', body, at)[0]] = body[at + 2:at + 2 + ROW]
            yield step, tick, phase, rows


def s2(run, marine_type):
    ticks = [r for r in read_csv(run / 'ticks.csv') if r['phase'] == '0' and r['running'] == '1']
    if not ticks:
        print('S2 no running ticks')
        return
    paused = 0
    lead = []
    jumps = collections.Counter()
    selector = collections.Counter()
    previous = None
    for r in ticks:
        tick, seconds, pause = int(r['tick']), int(r['seconds']), int(r['pause'])
        if previous is not None:
            step = tick - previous[0]
            jumps[step] += 1
            # A tick is paused when the pause word is set at its change step.
            if pause != 0:
                paused += step
        unpaused = tick - paused
        lead.append(unpaused - 16 * seconds)
        selector[r['selector']] += 1
        previous = (tick, seconds)
    print(f'S2 ticks={len(ticks)} tick_steps={dict(jumps)} paused_ticks={paused} selector_counts={dict(selector)}')
    print(f'S2 unpaused-16*seconds range=[{min(lead)},{max(lead)}] width={max(lead) - min(lead)} (floor relation needs width<=15)')
    follow = {r['tick']: r for r in read_csv(run / 'ticks.csv') if r['phase'] == '1' and r['running'] == '1'}
    paused = 0
    lead_follow = []
    previous = None
    for r in ticks:
        f = follow.get(r['tick'])
        tick = int(r['tick'])
        if previous is not None and r['pause'] != '0':
            paused += tick - previous
        previous = tick
        if f is not None:
            lead_follow.append(tick - paused - 16 * int(f['seconds']))
    if lead_follow:
        print(f'S2 same check on follow-step reads ({len(lead_follow)}): range=[{min(lead_follow)},{max(lead_follow)}] '
              f'width={max(lead_follow) - min(lead_follow)}')
    edges = [(ticks[i]['step'], ticks[i - 1]['pause'], ticks[i]['pause'])
             for i in range(1, len(ticks)) if ticks[i]['pause'] != ticks[i - 1]['pause']]
    print('S2 pause edges (step, before, after):', edges[:12])
    raw = run / 'ticks.raw'
    if not raw.is_file():
        return
    # Build timer: the unfinished row of the given type with the largest timer at first sight.
    series = collections.defaultdict(list)
    pause_at = {int(r['tick']): int(r['pause']) for r in ticks}
    for step, tick, phase, rows in raw_records(raw):
        if phase:
            continue
        for index, body in rows.items():
            kind = struct.unpack_from('>H', body, 0x64)[0]
            status = struct.unpack_from('>I', body, 0xD8)[0]
            if kind == marine_type and not status >> 31:
                series[index].append((tick, struct.unpack_from('>H', body, 0xA8)[0], pause_at.get(tick)))
    for index, points in series.items():
        bad = 0
        frozen = moving = 0
        unpaused_steps = timer_steps = 0
        for (t0, v0, _), (t1, v1, p1) in zip(points, points[1:]):
            dt, dv = t1 - t0, v0 - v1
            if p1:
                frozen += dv == 0
                bad += dv != 0
            else:
                moving += 1
                unpaused_steps += dt
                timer_steps += dv
        print(f'S2 marine row {index}: samples={len(points)} timer {points[0][1]}->{points[-1][1]} '
              f'unpaused tick advance={unpaused_steps} timer advance={timer_steps} paused samples frozen={frozen} '
              f'paused samples moving={bad}')


def s3(run):
    ticks = read_csv(run / 'ticks.csv')
    change = [r for r in ticks if r['phase'] == '0']
    follow = [r for r in ticks if r['phase'] == '1']
    torn = [r for r in follow if r['same'] == '0']
    bad = sum(int(r['bad_links']) > 0 for r in ticks)
    asym = sum(int(r['asymmetric_links']) > 0 for r in ticks)
    dual = sum(int(r['dual_hits']) > 0 for r in ticks)
    phases = collections.Counter()
    phase_changes = collections.Counter()
    for r in ticks:
        if r['phase'] != '0':
            key = min(int(r['phase']), 4)
            phases[key] += 1
            phase_changes[key] += r['same'] == '0'
    print('S3 reads by phase (4 = 4 or more):', dict(sorted(phases.items())), 'changed without a tick change:', dict(sorted(phase_changes.items())))
    print(f'S3 reads={len(ticks)} tick_changes={len(change)} follow_reads={len(follow)} changed_on_follow={len(torn)} '
          f'({100.0 * len(torn) / max(1, len(follow)):.3f}%) bad_link_reads={bad} asymmetric_link_reads={asym} alias_conflict_reads={dual}')
    verify = run / 'verify.csv'
    if verify.is_file():
        rows = [r for r in read_csv(verify) if r['kind'] == 'branch']
        same = [r for r in read_csv(verify) if r['kind'] == 'same']
        if same:
            print(f"S3 same-mode round trips={len(same)} records_equal={sum(r['change_rows_equal'] == '1' for r in same)} "
                  f"full_view_equal={sum(r['view_equal'] == '1' for r in same)}")
        matched = 'tick_matched' if rows and 'tick_matched' in rows[0] else 'change_step_equal'
        ok = [r for r in rows if r[matched] == '1' and r['change_rows_equal'] == '1' and r['follow_rows_equal'] == '1']
        views = collections.Counter(r['view_equal'] for r in rows)
        skew = collections.Counter(r.get('skew', '?') for r in rows)
        same_step = [r for r in rows if r.get('skew') == '0']
        print(f'S3 branches={len(rows)} records_equal={len(ok)} skew_counts={dict(skew)} full_view_equal_counts={dict(views)}')
        if same_step:
            print(f"S3 zero-skew branches={len(same_step)} full_view_equal={sum(r['view_equal'] == '1' for r in same_step)}")
        differ = [int(r['view_differ_bytes']) for r in rows if r.get('skew') == '0' and r.get('view_differ_bytes') not in (None, '', '0')]
        if differ:
            print(f'S3 view differ bytes min={min(differ)} max={max(differ)}')
    raw = run / 'ticks.raw'
    if raw.is_file() and torn:
        # Which fields change between a tick-change read and the read one step later?
        offsets = collections.Counter()
        last = {}
        for step, tick, phase, rows in raw_records(raw):
            if phase == 1 and (tick in last):
                before = last[tick]
                for index, body in rows.items():
                    old = before.get(index)
                    if old is None:
                        offsets['row-added'] += 1
                        continue
                    for at in range(ROW):
                        if body[at] != old[at]:
                            offsets[at] += 1
                for index in before:
                    if index not in rows:
                        offsets['row-removed'] += 1
            if phase == 0:
                last = {tick: rows}
        print('S3 follow-step byte offsets changed (offset: count):', offsets.most_common(16))


def throughput(run):
    # Pair each run command with its tap mode and CPU mode from the tape order in bridge.log replies.
    rows = read_csv(run / 'commands.csv')
    replies = [l.split() for l in open(run / 'bridge.log') if l.startswith('OK ') and len(l.split()) == 4]
    tape = [l.strip() for l in open(run / 'tape.txt')] if (run / 'tape.txt').is_file() else []
    if len(replies) != len(rows) or not tape:
        print(f'throughput: {len(replies)} timed replies for {len(rows)} commands; tape copy present={bool(tape)}')
        return
    import json
    mode, tap = json.loads((run / 'manifest.json').read_text()).get('mode', 'jit'), 'off'
    totals = collections.defaultdict(lambda: [0, 0, 0])
    index = 0
    for line in tape:
        word = line.split()[0] if line else ''
        if word == 'quit':
            break
        if word == 'mode':
            mode = line.split()[1]
        elif word == 'tap':
            tap = line.split()[1]
        elif word[:1].isdigit():
            steps = int(line.split()[0])
            wall, cpu = int(replies[index][2]), int(replies[index][3])
            index += 1
            if steps >= 200:
                t = totals[(mode, tap)]
                t[0] += steps; t[1] += wall; t[2] += cpu
    for (mode, tap), (steps, wall, cpu) in sorted(totals.items()):
        print(f'throughput {mode} tap={tap}: steps={steps} cpu_steps_per_s={1000.0 * steps / max(1, cpu):.1f} '
              f'wall_steps_per_s={1000.0 * steps / max(1, wall):.1f}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--s1', nargs=2, type=pathlib.Path, metavar=('RUN_A', 'RUN_B'))
    parser.add_argument('--s2', type=pathlib.Path)
    parser.add_argument('--s3', type=pathlib.Path)
    parser.add_argument('--throughput', type=pathlib.Path)
    parser.add_argument('--marine-type', type=int, default=0)
    args = parser.parse_args()
    if args.s1:
        s1(*args.s1)
    if args.s2:
        s2(args.s2, args.marine_type)
    if args.s3:
        s3(args.s3)
    if args.throughput:
        throughput(args.throughput)


if __name__ == '__main__':
    main()
