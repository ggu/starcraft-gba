#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Summarise capture spike runs (S1 hashes, S2 clocks, S3 tap records, and
checkpoint comparisons between runs with different tapes, such as a state
loaded in a fresh process against an uninterrupted run).

Reads private run directories written by capture.py and prints counts and
pass/fail lines only; it never copies guest bytes out of the run directory.
"""
import argparse
import collections
import csv
import pathlib
import re
import struct

ROW = 312
REGION_BLOCKS = 8 * 1024 * 1024 // 65536


def frontend_lines(text):
    """Tape lines as the frontend reads them (pass raw bytes decoded without newline
    translation, never read_text(), which turns a bare CR into a line break): split at LF only, comments and empty lines
    skipped (a line starting with '#'), tokens split at C whitespace, nothing after quit."""
    for line in text.split('\n'):
        if not line or line[0] == '#':
            continue
        words = [w for w in re.split('[ \t\n\v\f\r]+', line) if w]
        if not words:
            continue
        if words[0] == 'quit':
            return
        yield words


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
    same_inputs = all(manifests[0].get(k) == manifests[1].get(k) for k in keys) and \
        (manifests[0].get('state_input_sha256') or {}) == (manifests[1].get('state_input_sha256') or {})
    for run, manifest in zip((a, b), manifests):
        loads = [w[1] for w in frontend_lines((run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')) if len(w) >= 2 and w[0] == 'load'] \
            if (run / 'tape.txt').is_file() else []
        if any(name not in (manifest.get('state_input_sha256') or {}) for name in loads):
            print(f'S1 {run.name}: the tape loads a state with no recorded digest; no verdict')
            return
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


def regions_at(run):
    """{step: [64 KiB block FNV-1a strings]} from regions.csv ({} when the file is missing)."""
    out = {}
    if not (run / 'regions.csv').is_file():
        return out
    with open(run / 'regions.csv') as handle:
        for line in handle:
            parts = line.strip().split(',')
            out[parts[0]] = parts[1:]
    return out


def compare(a, b, exclude):
    """Checkpoint-by-checkpoint comparison of two runs at shared steps. Unlike S1 it
    does not require identical tapes, so it covers a loaded state against the run
    that wrote it. Blocks listed in `exclude` (64 KiB indexes) are left out of the
    game-state verdict."""
    left = {r['step']: r['sha256'] for r in read_csv(a / 'checkpoints.csv')}
    right = {r['step']: r['sha256'] for r in read_csv(b / 'checkpoints.csv')}
    shared = sorted(set(left) & set(right), key=int)
    if not shared:
        print('compare: no shared checkpoints')
        return
    ra, rb = regions_at(a), regions_at(b)
    full = [s for s in shared if left[s] == right[s]]
    blocks = collections.Counter()
    state_equal = missing_regions = 0
    for step in shared:
        if left[step] == right[step]:
            state_equal += 1
            continue
        left_blocks, right_blocks = ra.get(step, []), rb.get(step, [])
        complete = lambda blocks: len(blocks) == REGION_BLOCKS and all(
            len(h) == 16 and all(c in '0123456789abcdef' for c in h) for h in blocks)
        if not complete(left_blocks) or not complete(right_blocks):
            missing_regions += 1   # never counted as equal without every block on both sides
            continue
        differ = {i for i, (x, y) in enumerate(zip(left_blocks, right_blocks)) if x != y}
        blocks.update(differ)
        state_equal += not (differ - exclude)
    first = next((s for s in shared if left[s] != right[s]), None)
    print(f'compare checkpoints={len(shared)} steps={shared[0]}..{shared[-1]} full_view_equal={len(full)} '
          f'state_regions_equal={state_equal} (excluding {len(exclude)} blocks) first_full_differ={first}'
          + (f' checkpoints_without_all_regions={missing_regions}' if missing_regions else ''))
    if blocks:
        print('compare differing 64 KiB blocks (block: checkpoints):',
              ', '.join(f'0x{i * 65536:06x}:{n}' for i, n in sorted(blocks.items())))
    print(f'compare last step {shared[-1]} hashes', left[shared[-1]][:16], right[shared[-1]][:16])


def watch_summary(run):
    """Per watched field: number of changes and the distinct values seen."""
    rows = read_csv(run / 'watch.csv')
    fields = collections.defaultdict(list)
    for r in rows:
        fields[(r['address'], r['bytes'])].append((int(r['step']), r['value']))
    for (address, size), samples in sorted(fields.items()):
        # A field re-registered after `watch off` logs its value again; only a different value
        # from the previous sample is a change.
        changes = sum(1 for (_, a), (_, b) in zip(samples, samples[1:]) if a != b)
        values = collections.Counter(v for _, v in samples)
        print(f'watch {address}/{size}: changes={changes} distinct={len(values)} first={samples[0]} last={samples[-1]}')


def phase_records(run):
    """{phase: {(tick, occurrence): (rows_sha256, globals_sha256)}} from ticks.csv. Keying by
    occurrence keeps every record if the tick counter ever repeats (it restarts per mission)."""
    out, changes = collections.defaultdict(dict), collections.Counter()
    if not (run / 'ticks.csv').is_file():
        return None
    for r in read_csv(run / 'ticks.csv'):
        tick = int(r['tick'])
        if r['phase'] == '0':
            changes[tick] += 1
        # A follow-up read belongs to the tick change before it, so it takes that occurrence.
        occurrence = changes[tick] - 1
        # Rows only count as unit-row evidence when the table was readable (offset nonzero).
        rows = r['rows_sha256'] if r.get('table', '0') not in ('', '0') else None
        out[r['phase']].setdefault((tick, occurrence), (rows, r['globals_sha256']))
    return out


def s8_pair(a, b, legacy=False):
    """Same lobby, same Start step: compare per-tick unit records and globals (banks, RNG words)
    phase by phase: the change-step read (phase 0) and the follow-up read (phase 1)."""
    import json
    manifests = [json.loads((run / 'manifest.json').read_text()) if (run / 'manifest.json').is_file() else {} for run in (a, b)]
    ids = [m.get('run_id') for m in manifests]
    if not all(m.get('ok') for m in manifests):
        print('S8 pair: a capture did not finish cleanly (manifest ok is false or missing); the comparison is partial')
    if a.resolve() == b.resolve() or (ids[0] and ids[0] == ids[1]):
        print('S8 pair: the same run directory or run id appears twice; no verdict')
        return
    if not ids[0] or not ids[1]:
        if not legacy:
            print('S8 pair: a run id is missing; no verdict (--legacy accepts captures made before run ids)')
            return
        print('S8 pair: legacy manifests without run ids; independence rests on separate run directories')
    import hashlib
    for run, manifest in zip((a, b), manifests):
        recorded = (manifest.get('outputs_sha256') or {}).get('ticks.csv')
        log = run / 'ticks.csv'
        if log.is_file() and (not recorded or recorded != hashlib.sha256(log.read_bytes()).hexdigest()):
            print(f'S8 pair: {run.name}/ticks.csv does not match its manifest; no verdict')
            return
    left, right = phase_records(a), phase_records(b)
    if left is None or right is None:
        print('S8 pair: a tick log is missing; no verdict')
        return
    same_coverage, evidence = True, True
    for phase, name in (('0', 'change-step'), ('1', 'follow-up')):
        l, r = left.get(phase, {}), right.get(phase, {})
        shared = [k for k in l if k in r]            # capture order (dicts keep insertion order)
        only = len(set(l) ^ set(r))
        comparable = [k for k in shared if l[k][0] is not None and r[k][0] is not None]
        rows_differ = [k for k in comparable if l[k][0] != r[k][0]]
        no_table = sum(1 for k in shared if l[k][0] is None and r[k][0] is None)
        one_sided = sum(1 for k in shared if (l[k][0] is None) != (r[k][0] is None))
        globals_differ = [k for k in shared if l[k][1] != r[k][1]]
        same_coverage &= only == 0 and list(l) == list(r) and one_sided == 0
        evidence &= bool(comparable)
        print(f'S8 pair {name} records: {len(l)}/{len(r)} shared={len(shared)} only_in_one_run={only} '
              f'unit_rows_compared={len(comparable)} rows_differ={len(rows_differ)} no_table_both={no_table} '
              f'table_in_one_run_only={one_sided} globals_differ={len(globals_differ)} '
              f'first_rows_differ={rows_differ[0][0] if rows_differ else None} last_tick={shared[-1][0] if shared else None}')
    if not same_coverage:
        print('S8 pair: coverage differs between the runs; not a full-match comparison')
    elif not evidence:
        print('S8 pair: same coverage, but a phase has no comparable unit rows; not a full-match comparison')


def census(rows):
    """Counter of (owner, type, completed) over live rows."""
    out = collections.Counter()
    for body in rows.values():
        kind = struct.unpack_from('>H', body, 0x64)[0]
        out[(body[0x4C], kind, struct.unpack_from('>I', body, 0xD8)[0] >> 31)] += 1
    return out


def opening(run, ticks_limit):
    """Per owner, unit types in order of first appearance (sampled ticks), and the census at the last sample."""
    first = {}
    last = None
    # A restart of the tick counter (a second launch) may fall between raw samples, so its step
    # is taken from the unstrided tick log; nothing from that step on is used.
    log = read_csv(run / 'ticks.csv') if (run / 'ticks.csv').is_file() else []
    reset_step = None
    for previous, current in zip(log, log[1:]):
        if int(current['tick']) < int(previous['tick']):
            reset_step = int(current['step'])
            print('opening: the tick counter restarts (a second match); only the first match is used')
            break
    # With the tap off part of the time, a relaunch can hide between samples: decline then.
    tape = (run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape') if (run / 'tape.txt').is_file() else ''
    taps = [w for w in frontend_lines(tape) if w[0] == 'tap']
    active, was_active, gap = False, False, False   # an active tap, then off, then active again
    for w in taps:
        on = w[1:2] != ['off']
        gap |= on and was_active and not active
        was_active |= on
        active = on
    if gap:
        print('opening: the tape turns the tap off and on again; a relaunch could hide in the gap; no report')
        return
    for step, tick, phase, rows in raw_records(run / 'ticks.raw', before_step=reset_step):
        if last is None and not rows:
            continue   # records before the first live one are not part of the match
        if tick > ticks_limit:
            break
        if last is not None and tick < last[0]:
            print(f'opening: the tick counter restarts (a second match); stopping at tick {last[0]}')
            break
        last = (tick, rows)
        for body in rows.values():
            key = (body[0x4C], struct.unpack_from('>H', body, 0x64)[0])
            first.setdefault(key, tick)
    by_owner = collections.defaultdict(list)
    for (owner, kind), tick in sorted(first.items(), key=lambda item: (item[1], item[0])):
        by_owner[owner].append((tick, kind))
    for owner in sorted(by_owner):
        print(f'opening owner {owner}:', ' '.join(f'{kind}@{tick}' for tick, kind in by_owner[owner]))
    if last:
        counts = census(last[1])
        totals = collections.Counter()
        for (owner, kind, done), n in counts.items():
            totals[owner] += n
        print(f'census at tick {last[0]}:', dict(sorted(totals.items())))


WORKERS = {7: 'Terran', 41: 'Zerg', 64: 'Protoss'}


def sweep(runs, ticks_limit, seed_address):
    """Seed sweep: launch seed, owner -> race (by worker type), and per race the order
    in which new unit types first appear (initial units excluded) up to ticks_limit."""
    table = collections.defaultdict(dict)
    for run in runs:
        # The match's seed is the last value written at or before its first in-match record (the
        # first unstrided tick read with live unit rows), so a stale seed from an earlier launch is
        # never reported. Without that boundary the seed is unknown.
        first_record = None
        if (run / 'ticks.csv').is_file():
            first_record = next((int(r['step']) for r in read_csv(run / 'ticks.csv') if r['live'] not in ('', '0')), None)
        # Only full 4-byte samples count, and a tape that ever turns watches off could leave a
        # stale value standing, so its seed is reported as unknown.
        tape = (run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape') if (run / 'tape.txt').is_file() else ''
        watch_off = any(w[:2] == ['watch', 'off'] for w in frontend_lines(tape))
        seed = None
        if (run / 'watch.csv').is_file() and first_record is not None and not watch_off:
            for r in read_csv(run / 'watch.csv'):
                if r['address'] == seed_address and r['bytes'] == '4' and int(r['step']) <= first_record:
                    seed = int(r['value'], 16) or None
        if not (run / 'ticks.raw').is_file():
            print(f'sweep {run.name}: no ticks.raw (no unit samples); skipped')
            continue
        # Coverage can only be judged for a tape that sets raw sampling once and never changes the
        # tap afterwards; its stride is the K of that `tap ... raw [K]` line (default 1).
        tap_lines = [w for w in frontend_lines(tape) if w[0] == 'tap']
        raw_lines = [w for w in tap_lines if 'raw' in w]
        stride = 1
        if raw_lines:
            at = raw_lines[0].index('raw')
            if at + 1 < len(raw_lines[0]) and raw_lines[0][at + 1].isdigit():
                stride = int(raw_lines[0][at + 1])
        fixed_tap = len(raw_lines) == 1 and tap_lines[-1] is raw_lines[0]
        first, initial, owners = {}, set(), {}
        start = last = None
        raw_ticks, truncated = [], False   # raw sample ticks, read until one stride past the window
        raw_reads = []                     # (tick, step) of every raw record, for the readability check
        records = raw_records(run / 'ticks.raw')
        while True:
            try:
                step, tick, phase, rows = next(records)
            except StopIteration:
                break
            except (struct.error, ValueError):
                truncated = True
                break
            if start is None and not rows:
                continue   # records before the first live one are not part of the match
            if not raw_ticks or raw_ticks[-1] != tick:
                raw_ticks.append(tick)
            raw_reads.append((tick, step))
            if tick > ticks_limit:
                if start is not None and (tick >= ticks_limit + stride or ticks_limit == 1 << 30):
                    break
                if start is not None:
                    continue
            last = (tick, rows)
            for body in rows.values():
                owner, kind = body[0x4C], struct.unpack_from('>H', body, 0x64)[0]
                if start is None or tick == start:
                    initial.add((owner, kind))
                if kind in WORKERS:
                    owners.setdefault(owner, WORKERS[kind])
                first.setdefault((owner, kind), tick)
            if start is None:
                start, start_step = tick, step
        if start is None or start > ticks_limit:
            print(f'sweep {run.name}: no sample with live rows by tick {ticks_limit}; no starting roster')
            continue
        # Coverage comes from the unit samples themselves (raw logging may stop while ticks.csv
        # goes on): inside the window no gap may exceed the configured stride, and the samples
        # must reach the window's end.
        window = raw_ticks             # starts at the first live raw record
        inside = [t for t in window if t <= ticks_limit]
        end = ticks_limit if ticks_limit != 1 << 30 else inside[-1]
        # One match per capture: any restart of the tick counter anywhere in the tick log (a new
        # launch) refuses the run, so its seed, roster and coverage all belong to one match. Only
        # reads from the first live raw record's step on count toward coverage.
        full_log = read_csv(run / 'ticks.csv') if (run / 'ticks.csv').is_file() else []
        all_ticks = [int(r['tick']) for r in full_log]
        reset = next((b for a, b in zip(all_ticks, all_ticks[1:]) if b < a), None)
        if reset is None:
            reset = next((b for a, b in zip(window, window[1:]) if b <= a), None)
        log = [r for r in full_log if int(r['step']) >= start_step]
        log_ticks = [int(r['tick']) for r in log]
        # A sample is missing when the next one comes later than one stride, before the cutoff.
        gap = next((a + stride for a, b in zip(window, window[1:]) if b - a > stride and a + stride <= end), None)
        # Every raw record counts only if its own read (same step) found the unit table.
        readable = {int(r['step']): r.get('table', '0') not in ('', '0') for r in log}
        unreadable = next((t for t, st in raw_reads if start <= t <= end and not readable.get(st, False)), None)
        # Both the raw samples and the unstrided tick log must reach an explicit cutoff.
        reaches = ticks_limit == 1 << 30 or (window[-1] + stride > ticks_limit and max(log_ticks, default=-1) >= ticks_limit)
        short = (truncated or not fixed_tap or reset is not None or gap is not None or unreadable is not None
                 or not reaches)
        cutoff = min(end, inside[-1])
        if short:
            reason = ('a truncated record' if truncated else 'tap settings that change during the run' if not fixed_tap
                      else f'a tick counter reset to {reset} (more than one match in the capture)' if reset is not None
                      else f'a missing unit sample at tick {gap}' if gap is not None
                      else f'no readable unit table at tick {unreadable}' if unreadable is not None
                      else f'unit samples ending at tick {window[-1]}')
            print(f'sweep {run.name}: {reason} (configured stride {stride}); counted through tick {cutoff} only and '
                  'left out of the totals')
        # The baseline is trusted only if it is the first tapped tick with live rows.
        if (run / 'ticks.csv').is_file():
            first_live = next((int(r['tick']) for r in read_csv(run / 'ticks.csv') if r['live'] not in ('', '0')), None)
            if first_live != start:
                print(f'  warning: baseline sample is tick {start}, but rows were live from tick {first_live}; '
                      'units made in between count as starting units')
        built = collections.defaultdict(list)
        for (owner, kind), tick in sorted(first.items(), key=lambda item: (item[1], item[0][1])):
            if (owner, kind) not in initial and owner in owners:
                built[owner].append((tick, kind))
        if reset is not None:
            continue   # two matches: no per-run opening or census either
        human = [o for o in owners if not built[o]]
        print(f'sweep {run.name}: seed={seed} owners={dict(sorted(owners.items()))} '
              f'no new unit type through tick {cutoff}={human}')
        for owner, race in sorted(owners.items()):
            if built[owner]:
                order = ' '.join(f'{kind}@{tick}' for tick, kind in built[owner])
                if not short:
                    table[race][(str(run), owner)] = tuple(kind for _, kind in built[owner])
                print(f'  {race:8} owner {owner}: {order}')
        if last:
            counts = census(last[1])
            per = {o: sum(n for (owner, _, _), n in counts.items() if owner == o) for o in sorted(owners)}
            print(f'  census at tick {last[0]} (live rows by owner): {per}')
    for race, orders in sorted(table.items()):
        distinct = collections.Counter(orders.values())
        window = f'by tick {ticks_limit}' if ticks_limit != 1 << 30 else 'within each run\'s own last sample'
        print(f'sweep {race}: {len(distinct)} distinct opening orders over {len(orders)} owners with a new unit type {window}')


def raw_records(path, before_step=None):
    """Yield (step, tick, phase, rows{index: bytes}) from ticks.raw; with before_step, stop at the
    first record at or after that step without reading its body."""
    with open(path, 'rb') as handle:
        while True:
            head = handle.read(16)
            if not head:
                return
            # The step comes first, so a record at or after before_step ends the read even if
            # the rest of its header is cut off.
            if len(head) >= 4 and before_step is not None and struct.unpack_from('<I', head)[0] >= before_step:
                return
            if len(head) < 16:
                raise ValueError(f'{path}: truncated record header')
            step, tick, phase, live = struct.unpack('<4I', head)
            if live > 900:   # the unit table has 900 rows; a larger count is a corrupt header
                raise ValueError(f'{path}: record at step {step} claims {live} rows')
            body = handle.read(live * (2 + ROW))
            if len(body) < live * (2 + ROW):
                raise ValueError(f'{path}: truncated record at step {step}')
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
    # Pad commands answer "OK step wall cpu"; "until" answers "UNTIL..." and is not timed here.
    replies = [l.split() for l in open(run / 'bridge.log')
               if (l.startswith('OK ') and len(l.split()) == 4) or l.startswith(('UNTIL ', 'UNTILMISS '))]
    tape = (run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape') if (run / 'tape.txt').is_file() else ''
    if not tape or (len(replies) != len(rows) and not any(r[0] == 'UNTILMISS' for r in replies)):
        print(f'throughput: {len(replies)} timed replies for {len(rows)} commands; tape copy present={bool(tape)}')
        return
    import json
    mode, tap = json.loads((run / 'manifest.json').read_text()).get('mode', 'jit'), 'off'
    totals = collections.defaultdict(lambda: [0, 0, 0])
    index = 0
    for words in frontend_lines(tape):
        word = words[0]
        if word == 'until':
            if index >= len(replies) or replies[index][0] not in ('UNTIL', 'UNTILMISS'):
                print('throughput: replies do not follow the tape; no report')
                return
            if replies[index][0] == 'UNTILMISS':
                break   # the frontend exited at the miss
            index += 1
        elif word == 'mode':
            mode = words[1]
        elif word == 'tap':
            tap = words[1]
        elif re.fullmatch(r'[+-]?[0-9]+', word):
            if index >= len(replies) or replies[index][0] != 'OK':
                print('throughput: replies do not follow the tape; no report')
                return
            steps = int(word)
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
    parser.add_argument('--compare', nargs=2, type=pathlib.Path, metavar=('RUN_A', 'RUN_B'))
    parser.add_argument('--exclude-block', type=lambda t: int(t, 16) // 65536, action='append', default=[],
                        help='64 KiB block start (hex) left out of the game-state verdict, e.g. 190000')
    parser.add_argument('--watch', type=pathlib.Path)
    parser.add_argument('--s8', nargs=2, type=pathlib.Path, metavar=('RUN_A', 'RUN_B'))
    parser.add_argument('--legacy', action='store_true', help='--s8: accept captures made before capture.py wrote run ids')
    parser.add_argument('--opening', type=pathlib.Path)
    parser.add_argument('--ticks', type=int, default=1 << 30, help='last tick for --opening and --sweep')
    parser.add_argument('--sweep', nargs='+', type=pathlib.Path, metavar='RUN')
    parser.add_argument('--seed-address', default='800eff20', help='watch.csv address of the launch seed')
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
    if args.compare:
        compare(*args.compare, set(args.exclude_block))
    if args.watch:
        watch_summary(args.watch)
    if args.s8:
        s8_pair(*args.s8, legacy=args.legacy)
    if args.opening:
        opening(args.opening, args.ticks)
    if args.sweep:
        sweep(args.sweep, args.ticks, args.seed_address)


if __name__ == '__main__':
    main()
