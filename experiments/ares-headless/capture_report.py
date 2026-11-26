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


def load_manifest(run):
    """manifest.json as a dict; {} when missing or malformed (it then verifies nothing). Nested
    maps of the wrong type are dropped, so they verify nothing either."""
    import json
    try:
        manifest = json.loads((run / 'manifest.json').read_text())
    except (OSError, ValueError):
        return {}
    if not isinstance(manifest, dict):
        return {}
    # A nested map of the wrong type is not the same as a missing one: the manifest is then
    # marked malformed, verifies nothing, and gives no verdict.
    for key in ('outputs_sha256', 'state_input_sha256', 'state_input_source', 'states_sha256'):
        if key in manifest and not isinstance(manifest[key], dict):
            manifest[key] = {}
            manifest['_malformed'] = key
    return manifest


def producer_ok(source):
    """A recorded state producer: a nonempty run id string and a 64-hex binary digest."""
    if not isinstance(source, dict):
        return False
    run_id, binary = source.get('run_id'), source.get('binary_sha256')
    return (isinstance(run_id, str) and bool(run_id) and isinstance(binary, str) and len(binary) == 64
            and all(c in '0123456789abcdef' for c in binary))


def verified_end(run, manifest):
    """completed_steps from status.log, only when the manifest records that file's digest and it
    matches; None otherwise."""
    return status_counter(run, manifest, 'completed_steps')


def status_counter(run, manifest, name):
    """A NAME=N counter from status.log, only when the manifest records that file's digest and it
    matches; None otherwise."""
    status = run / 'status.log'
    recorded = (manifest.get('outputs_sha256') or {}).get('status.log')
    if not recorded or not status.is_file() or file_sha256(status) != recorded:
        return None
    for word in status.read_text(errors='replace').split():
        if word.startswith(name + '=') and re.fullmatch(r'[0-9]{1,12}', word[len(name) + 1:]):
            return int(word[len(name) + 1:])
    return None


def clean(manifest):
    """A run finished cleanly only if its manifest says ok with the boolean true."""
    return manifest.get('ok') is True


def file_sha256(path):
    import hashlib
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def read_csv(path):
    """Rows of a CSV log; ValueError (not a traceback) for bad encoding or an oversized field."""
    try:
        with open(path, newline='') as handle:
            return list(csv.DictReader(handle))
    except (UnicodeDecodeError, csv.Error) as error:
        raise ValueError(f'{path.name} is not readable CSV ({error})') from None


def run_integrity(label, runs, logs, legacy=False):
    """Checks shared by the run-pair reports. Returns (manifests, partial) or None after
    printing why there is no verdict: the same directory or run id twice, a missing run id
    (unless legacy), a log that does not match its manifest digest, or a loaded state with no
    recorded digest. A run that did not finish cleanly only makes the result partial."""
    import hashlib
    import json
    if len({run.resolve() for run in runs}) != len(runs):
        print(f'{label}: the same run directory twice; no verdict')
        return None
    manifests = [load_manifest(run) for run in runs]
    legacy_gaps = False
    for run, manifest in zip(runs, manifests):
        if manifest.get('_malformed'):
            print(f'{label}: {run.name}/manifest.json has a malformed {manifest["_malformed"]}; no verdict')
            return None
    ids = [m.get('run_id') for m in manifests]
    if any(i is not None and not (isinstance(i, str) and i) for i in ids):
        print(f'{label}: a run id is not a nonempty string; no verdict')
        return None
    if any(ids) and len({i for i in ids if i}) != len([i for i in ids if i]):
        print(f'{label}: the same run id twice; no verdict')
        return None
    if not all(ids):
        if not legacy:
            print(f'{label}: a run id is missing; no verdict (--legacy accepts captures made before run ids)')
            return None
        print(f'{label}: legacy manifests without run ids; independence rests on separate run directories')
        legacy_gaps = True
    for run, manifest in zip(runs, manifests):
        recorded = manifest.get('outputs_sha256') or {}
        # Every log the report reads must exist and match its digest; so must any other log the
        # manifest lists, and the tape, since its `load` lines decide the state checks below.
        for name in sorted(set(logs) | set(recorded)):
            log = run / name
            if not log.is_file() or recorded.get(name) != file_sha256(log):
                print(f'{label}: {run.name}/{name} is missing or does not match its manifest; no verdict')
                return None
        tape_path = run / 'tape.txt'
        if not tape_path.is_file() or manifest.get('tape_sha256') != file_sha256(tape_path):
            print(f'{label}: {run.name}/tape.txt is missing or does not match its manifest; no verdict')
            return None
        tape = tape_path.read_bytes().decode('utf-8', 'surrogateescape')
        loads = [w[1] for w in frontend_lines(tape) if len(w) >= 2 and w[0] == 'load']
        states = manifest.get('state_input_sha256') or {}
        if any(name not in states for name in loads):
            print(f'{label}: {run.name} loads a state with no recorded digest; no verdict')
            return None
        for name in loads:
            staged = run / 'input' / name
            if not staged.is_file() or file_sha256(staged) != states[name]:
                print(f'{label}: {run.name}/input/{name} is missing or does not match its manifest; no verdict')
                return None
        # Positive provenance: the capture that wrote each loaded state (S5 manifests record it).
        sources = manifest.get('state_input_source') or {}
        unproven = [name for name in loads if not producer_ok(sources.get(name))]
        malformed = [name for name in unproven if name in sources]   # present (even null) but not valid
        if malformed:
            print(f'{label}: {run.name} has a malformed producer record for {", ".join(malformed)}; no verdict')
            return None
        if unproven:
            if not legacy:
                print(f'{label}: {run.name} loads {", ".join(unproven)} with no recorded producer; no verdict '
                      '(--legacy accepts captures made before capture.py recorded state sources)')
                return None
            print(f'{label}: {run.name} predates recorded state sources; the loaded state\'s producer is not checked')
            legacy_gaps = True
        if manifest.get('unrecorded_state'):
            print(f'{label}: {run.name} staged a state no capture recorded (--unrecorded-state); no verdict')
            return None
    partial = [run.name for run, m in zip(runs, manifests) if not clean(m)]
    if partial:
        print(f'{label}: {", ".join(partial)} did not finish cleanly (manifest ok is false or missing); the result is partial')
    return manifests, bool(partial), legacy_gaps


def s1(a, b, legacy=False):
    import json
    checked = run_integrity('S1', (a, b), ('checkpoints.csv', 'regions.csv'), legacy)
    if checked is None:
        return
    manifests, partial, legacy_gaps = checked
    if partial:
        print('S1: a run did not finish cleanly; no verdict')
        return
    hex64 = lambda v: isinstance(v, str) and len(v) == 64 and all(c in '0123456789abcdef' for c in v)
    for run, m in zip((a, b), manifests):
        if not (hex64(m.get('binary_sha256')) and hex64(m.get('rom_sha256')) and m.get('mode') in ('jit', 'interp')
                and 'flash_input_sha256' in m and (m['flash_input_sha256'] is None or hex64(m['flash_input_sha256']))):
            print(f'S1 {run.name}: the manifest lacks valid binary, ROM, mode or flash-input records; no verdict')
            return
    keys = ('tape_sha256', 'flash_input_sha256', 'binary_sha256', 'mode', 'rom_sha256')
    same_inputs = all(manifests[0].get(k) == manifests[1].get(k) for k in keys) and \
        (manifests[0].get('state_input_sha256') or {}) == (manifests[1].get('state_input_sha256') or {})
    for run in (a, b):
        # S1 is same-tape determinism from power-on: a `load` or `mode` line changes the machine
        # without a step, so a checkpoint could predate it. Such tapes belong to --compare.
        words = [w[0] for w in frontend_lines((run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape'))]
        if 'load' in words or 'mode' in words:
            print(f'S1 {run.name}: the tape uses load or mode; S1 covers power-on tapes only (use --compare); no verdict')
            return
    try:
        left, right = checkpoint_hashes(a), checkpoint_hashes(b)
    except ValueError as error:
        print(f'S1: {error}; no verdict')
        return
    shared = sorted(set(left) & set(right), key=checkpoint_order)
    conflicted = [k for k in shared if left[k] is None or right[k] is None]
    shared = [k for k in shared if k not in conflicted]
    differ = [s for s in shared if left[s] != right[s]]
    print(f'S1 checkpoints compared={len(shared)} last={shared[-1] if shared else None} differ={len(differ)}'
          + (f' conflicted_not_compared={len(conflicted)}' if conflicted else ''))
    if differ:
        first = differ[0]
        print(f'S1 first divergent checkpoint step={first}')
        regions = {}
        for name, run in (('a', a), ('b', b)):
            if first in regions_at(run):
                regions[name] = regions_at(run)[first]
        if len(regions) == 2:
            blocks = [i for i, (x, y) in enumerate(zip(regions['a'], regions['b'])) if x != y]
            print('S1 differing 64 KiB blocks at', ', '.join(f'0x{i * 65536:06x}' for i in blocks[:32]))
    elif not shared or set(left) != set(right):
        print(f'S1 incomplete: checkpoints only in one run: {len(set(left) ^ set(right))}')
    elif not same_inputs:
        print('S1 overlap equal, but tape, save, binary, ROM or mode differ: no determinism verdict')
    elif conflicted:
        print('S1 overlap equal, but some checkpoints had disagreeing cache aliases and were not compared: no PASS')
    elif legacy:
        print('S1 overlap equal, but --legacy was given: no PASS (run with full provenance for a PASS)')
    elif verified_end(a, manifests[0]) is None or verified_end(a, manifests[0]) != verified_end(b, manifests[1]):
        print(f'S1 overlap equal, but the runs\' verified end steps differ or are not recorded '
              f'({verified_end(a, manifests[0])} vs {verified_end(b, manifests[1])}): no PASS')
    elif any((tape_words(run) or [''])[-1] != 'hash' for run in (a, b)):
        print('S1 overlap equal, but a tape does not end with `hash`, so a command after the last checkpoint '
              '(a save, a step) is not observed: no PASS')
    elif step_of(shared[-1]) != verified_end(a, manifests[0]) or shared[-1] != max(left, key=checkpoint_order):
        print(f'S1 equal through step {shared[-1]}, but there is no shared checkpoint at the end step '
              f'{verified_end(a, manifests[0])}: no PASS')
    elif any(expected_checkpoints(run) != set(side) for run, side in ((a, left), (b, right))):
        print('S1 overlap equal, but the checkpoints logged are not exactly the ones the tape schedules (or the '
              'schedule cannot be derived, as with `until`): no PASS')
    else:
        print('S1 PASS')


def expected_checkpoints(run, allow_load=False, planned_end=None):
    """The checkpoint keys a power-on tape schedules ('STEP', 'STEP#2', ... as checkpoint_hashes
    numbers them): one after every step that is a multiple of `every N`, and one per `hash`.
    None when the schedule depends on the run (`until`) or a line does not parse."""
    tape = run / 'tape.txt'
    if not tape.is_file():
        return None
    completed, every, steps = 0, 0, []
    limit = 1 << 20   # more scheduled checkpoints than this: treat the schedule as unknown
    for words in frontend_lines(tape.read_bytes().decode('utf-8', 'surrogateescape')):
        if re.fullmatch(r'\+?[0-9]+', words[0]) and len(words[0].lstrip('+0')) <= 12:   # a step line (`>>` takes a leading sign)
            count = int(words[0].lstrip('+0') or '0')
            if every:
                due = range((completed // every + 1) * every, completed + count + 1, every)
                if len(steps) + len(due) > limit:
                    return None
                steps += due
            completed += count
        elif words[0] == 'every':
            # The frontend reads N with `>>`: a leading sign and trailing text are accepted.
            number = re.match(r'\+?[0-9]+', words[1]) if len(words) > 1 else None
            if number is None or len(number.group().lstrip('+0')) > 18:
                return None
            every = int(number.group().lstrip('+0') or '0')
        elif words[0] == 'hash':
            steps.append(completed)
            if len(steps) > limit:
                return None
        elif words[0] == 'load' and allow_load:
            if len(words) > 2 and not re.fullmatch(r'[0-9]{1,12}', words[2]):
                return None
            completed = int(words[2]) if len(words) > 2 else 0   # later steps are numbered from STEP
        elif words[0] == 'mode' and allow_load:
            continue
        elif words[0] in ('until', 'untilw', 'load', 'mode'):
            return None
    seen, keys = collections.Counter(), set()
    for step in steps:
        seen[step] += 1
        keys.add(str(step) if seen[step] == 1 else f'{step}#{seen[step]}')
    if planned_end is not None:
        planned_end.append(completed)
    return keys


def tape_words(run):
    """The first word of every command the frontend runs, in order (empty if no tape)."""
    tape = run / 'tape.txt'
    if not tape.is_file():
        return []
    return [w[0] for w in frontend_lines(tape.read_bytes().decode('utf-8', 'surrogateescape'))]


def checkpoint_hashes(run):
    """{step: sha256} from checkpoints.csv; a step checkpointed more than once (a `hash` at the
    same step as an `every` checkpoint) gets keys 'STEP', 'STEP#2', ... in capture order, so a
    later hash cannot hide an earlier one."""
    out, seen = {}, collections.Counter()
    for r in read_csv(run / 'checkpoints.csv'):
        step, digest = r.get('step'), r.get('sha256')
        if not (isinstance(step, str) and re.fullmatch(r'[0-9]{1,12}', step) and isinstance(digest, str)
                and re.fullmatch(r'[0-9a-f]{64}', digest) and re.fullmatch(r'[0-9]{1,12}', r.get('dual_differ') or '')):
            raise ValueError(f'{run.name}/checkpoints.csv has a malformed row')
        seen[step] += 1
        # A checkpoint whose cache aliases disagreed hashed one of them: it proves nothing (None).
        conflicted = r['dual_differ'] != '0'
        out[step if seen[step] == 1 else f'{step}#{seen[step]}'] = None if conflicted else digest
    return out


def step_of(key):
    return int(str(key).split('#')[0])


def checkpoint_order(key):
    """Numeric step, then numeric occurrence ('1000#10' after '1000#2')."""
    step, _, occurrence = str(key).partition('#')
    return int(step), int(occurrence or 1)


def regions_at(run):
    """{step: [64 KiB block FNV-1a strings]} from regions.csv ({} when the file is missing), keyed
    like checkpoint_hashes ('STEP', 'STEP#2', ...) since both are written per checkpoint."""
    out, seen = {}, collections.Counter()
    if not (run / 'regions.csv').is_file():
        return out
    with open(run / 'regions.csv', errors='replace') as handle:   # a bad byte fails the block check, not the report
        for line in handle:
            parts = line.strip().split(',')
            seen[parts[0]] += 1
            out[parts[0] if seen[parts[0]] == 1 else f'{parts[0]}#{seen[parts[0]]}'] = parts[1:]
    return out


def compare(a, b, exclude, legacy=False):
    """Checkpoint-by-checkpoint comparison of two runs at shared steps. Unlike S1 it
    does not require identical tapes, so it covers a loaded state against the run
    that wrote it. Blocks listed in `exclude` (64 KiB indexes) are left out of the
    game-state verdict. The runs must be two separate captures whose logs match their
    manifests; a run that ended early or did not finish cleanly is reported as such."""
    checked = run_integrity('compare', (a, b), ('checkpoints.csv', 'regions.csv'), legacy)
    if checked is None:
        return
    for run in (a, b):
        # A checkpoint taken before a `load` describes a machine the run never continued (the
        # frontend now refuses this; older captures may still have it).
        lines = list(frontend_lines((run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')))
        # `hash` and `dump` observe at once; a nonzero `every` before `load` is refused by the
        # frontend itself, so it never leaves a checkpoint before a load in a finished run.
        observes = lambda w: w[0] in ('hash', 'dump')
        last_load = max((i for i, w in enumerate(lines) if w[0] == 'load'), default=None)
        if last_load is not None and (sum(w[0] == 'load' for w in lines) > 1 or any(observes(w) for w in lines[:last_load])):
            print(f'compare: {run.name} checkpoints before a load, or loads more than once; no verdict')
            return
    try:
        left, right = checkpoint_hashes(a), checkpoint_hashes(b)
    except ValueError as error:
        print(f'compare: {error}; no verdict')
        return
    for run, side, manifest in ((a, left, checked[0][0]), (b, right, checked[0][1])):
        end = verified_end(run, manifest)
        beyond = [k for k in side if end is not None and step_of(k) > end]
        if beyond:
            print(f'compare: {run.name} logs {len(beyond)} checkpoints past its verified end step {end}; no verdict')
            return
    shared = sorted(set(left) & set(right), key=checkpoint_order)
    conflicted = [k for k in shared if left[k] is None or right[k] is None]
    if conflicted:
        print(f'compare: {len(conflicted)} checkpoints had disagreeing cache aliases (first at step '
              f'{step_of(conflicted[0])}); they are not compared, so the result is partial')
        shared = [k for k in shared if k not in conflicted]
    if not shared:
        print('compare: no shared checkpoints')
        return
    for run in (a, b):
        if 'mode' in [w[0] for w in frontend_lines((run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape'))]:
            print(f'compare: {run.name} switches CPU mode in the tape; a checkpoint taken before a switch at the '
                  'same step does not cover the state after it')
    # A run that loads a state starts later by design, so only the end is checked: a run whose
    # checkpoints stop before the other's is covered only up to its own last checkpoint.
    manifests = checked[0]
    def run_end(run, manifest, side):
        # The verified status.log gives the step the run reached; the last checkpoint otherwise.
        end = verified_end(run, manifest)
        if end is None:
            print(f'compare: {run} has no verified status.log; its end step is unknown, and its last checkpoint '
                  f'is at step {max(step_of(k) for k in side)}')
            return None
        return end
    ends = [(str(run), run_end(run, m, side)) for run, m, side in ((a, manifests[0], left), (b, manifests[1], right))]
    if None not in (ends[0][1], ends[1][1]) and ends[0][1] == ends[1][1] and ends[0][1] > step_of(shared[-1]):
        print(f'compare: both runs end at step {ends[0][1]}, after the last shared checkpoint at step '
              f'{step_of(shared[-1])}; steps after it are not compared')
    if None not in (ends[0][1], ends[1][1]) and ends[0][1] != ends[1][1]:
        short = min(ends, key=lambda e: e[1])
        print(f'compare: {short[0]} ends at step {short[1]}, the other run at step {max(e[1] for e in ends)}; '
              f'the result covers steps {shared[0]}..{shared[-1]} only')
    elif None in (ends[0][1], ends[1][1]):
        last = [(str(run), max(step_of(k) for k in side)) for run, side in ((a, left), (b, right))]
        if last[0][1] != last[1][1]:
            print(f'compare: last checkpoints differ ({last[0][1]} vs {last[1][1]}); '
                  f'the result covers steps {shared[0]}..{shared[-1]} only')
    lo, hi = step_of(shared[0]), step_of(shared[-1])
    unmatched = sorted(step_of(k) for k in set(left) ^ set(right) if lo <= step_of(k) <= hi)
    if unmatched:
        print(f'compare: {len(unmatched)} checkpoints between steps {lo} and {hi} exist in only one run '
              f'(first {unmatched[0]}); those steps are not compared')
    trailing = sorted(step_of(k) for k in set(left) ^ set(right) if step_of(k) > hi)
    if trailing:
        print(f'compare: {len(trailing)} checkpoints after step {hi} exist in only one run (from step {trailing[0]}); '
              'those steps are not compared')
    leading = sorted(step_of(k) for k in set(left) ^ set(right) if step_of(k) < lo)
    if leading:
        print(f'compare: {len(leading)} checkpoints before step {lo} exist in only one run (from step {leading[0]}); '
              'the result covers the shared span only')
    # Each log against its own tape's schedule: a checkpoint missing from both logs would
    # otherwise not show as a difference.
    for run, side in ((a, left), (b, right)):
        planned = []
        scheduled = expected_checkpoints(run, allow_load=True, planned_end=planned)
        end = verified_end(run, checked[0][(a, b).index(run)])
        if scheduled is not None and end is not None and end < planned[0]:
            print(f'compare: {run.name} stopped at step {end}, before its tape\'s last step {planned[0]}; '
                  'the result is partial')
        if scheduled is None:
            print(f'compare: {run.name}\'s checkpoint schedule cannot be derived from its tape (`until`); a checkpoint '
                  'missing from both logs would not show, so the result is partial')
        else:
            due = {k for k in scheduled if end is None or step_of(k) <= end}
            if set(side) - scheduled:
                print(f'compare: {run.name} logs {len(set(side) - scheduled)} checkpoints its tape does not schedule; '
                      'the result is partial')
            if due - set(side):
                print(f'compare: {run.name} logs {len(due & set(side))} of the {len(due)} checkpoints its tape schedules '
                      f'(first missing at step {min(step_of(k) for k in due - set(side))}); the result is partial')
    ra, rb = regions_at(a), regions_at(b)
    # Region rows are numbered per step like checkpoints; if a run has a different number of
    # region rows than checkpoint rows at a step, the numbering may pair the wrong rows.
    for run, side, regions in ((a, left, ra), (b, right, rb)):
        counts = collections.Counter(step_of(k) for k in side)
        region_counts = collections.Counter(step_of(k) for k in regions if re.fullmatch(r'[0-9]{1,12}(#[0-9]+)?', k))
        for key in [k for k in regions if re.fullmatch(r'[0-9]{1,12}(#[0-9]+)?', k)
                    and counts[step_of(k)] != region_counts[step_of(k)]]:
            del regions[key]
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
    for run in (a, b):
        if (tape_words(run) or [''])[-1] != 'hash':
            print(f'compare: {run.name}\'s tape does not end with `hash`; commands after its last checkpoint '
                  '(a save, a mode switch, steps) are not covered')
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
        # Rows only count as unit-row evidence when the table was readable (offset nonzero). A read
        # whose two cache aliases disagreed (dual_differ) hashed one of them: it proves nothing.
        rows = r['rows_sha256'] if r.get('table', '0') not in ('', '0') else None
        conflicted = r.get('dual_differ', '0') != '0'
        out[r['phase']].setdefault((tick, occurrence), (None if conflicted else rows,
                                                        None if conflicted else r['globals_sha256'], conflicted))
    return out


def s8_pair(a, b, legacy=False):
    """Same lobby, same Start step: compare per-tick unit records and globals (banks, RNG words)
    phase by phase: the change-step read (phase 0) and the follow-up read (phase 1)."""
    if not (a / 'ticks.csv').is_file() or not (b / 'ticks.csv').is_file():
        print('S8 pair: a tick log is missing; no verdict')
        return
    checked = run_integrity('S8 pair', (a, b), ('ticks.csv',), legacy)
    if checked is None:
        return
    for run, manifest in zip((a, b), checked[0]):
        # The tap must stay on to the run's end: a `tap off`, or a tick log that stops before the
        # verified end step, leaves steps the comparison never saw.
        tap_lines = [w for w in frontend_lines((run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape'))
                     if w[0] == 'tap']
        while tap_lines and tap_lines[0][1:2] == ['off']:
            tap_lines.pop(0)   # the tap starts off, so a leading `tap off` changes nothing
        if not tap_lines or any(w[1:2] != ['tick'] and w[1:2] != ['off'] for w in tap_lines):
            print(f'S8 pair: {run.name} has no `tap tick` line or uses another tap mode; this report covers `tap tick` '
                  'captures only; no verdict')
            return
        end = verified_end(run, manifest)
        try:
            log = read_csv(run / 'ticks.csv')
        except ValueError as error:
            print(f'S8 pair: {run.name}: {error}; no verdict')
            return
        if not all(r.get('phase') in ('0', '1') and
                   all(re.fullmatch(r'[0-9]{1,12}', r.get(k) or '') for k in ('step', 'tick', 'phase', 'live', 'table',
                                                                               'dual_differ'))
                   and all(re.fullmatch(r'[0-9a-f]{64}', r.get(k) or '') for k in ('rows_sha256', 'globals_sha256'))
                   for r in log):
            print(f'S8 pair: {run.name}/ticks.csv has unreadable rows; no verdict')
            return
        # Each change-step read needs its follow-up on the next step, unless the tick changed
        # again on that very step (then the next read is another change-step read).
        by_step = {int(r['step']): r for r in log}
        def follows(read):
            after = by_step.get(int(read['step']) + 1)
            if after is None:
                return False
            if after['phase'] == '1':
                return after['tick'] == read['tick']                  # its own follow-up
            return after['phase'] == '0' and int(after['tick']) == int(read['tick']) + 1   # the next tick
        # The frontend's own counters must account for every row: all tap reads, and one
        # change-step read per tick change.
        reads, changes_seen = status_counter(run, manifest, 'tap_reads'), status_counter(run, manifest, 'tick_changes')
        if reads is None or changes_seen is None:
            print(f'S8 pair: {run.name} has no verified tap_reads and tick_changes counters, so a read missing at the '
                  'end cannot be ruled out; the comparison is partial')
        elif reads != len(log) or changes_seen != sum(r['phase'] == '0' for r in log):
            print(f'S8 pair: {run.name}\'s tick log has {len(log)} reads ({sum(r["phase"] == "0" for r in log)} tick '
                  f'changes), but the frontend counted {reads} ({changes_seen}); the comparison is partial')
        steps_in_order = [int(r['step']) for r in log]
        if end is not None and any(st > end for st in steps_in_order):
            print(f'S8 pair: {run.name}/ticks.csv has reads past its verified end step {end}; no verdict')
            return
        if any(b <= a for a, b in zip(steps_in_order, steps_in_order[1:])):
            print(f'S8 pair: {run.name}/ticks.csv repeats or reorders a step; the comparison is partial')
        stray = next((int(r['tick']) for r in log if r['phase'] == '1' and not (
            by_step.get(int(r['step']) - 1) is not None and by_step[int(r['step']) - 1]['phase'] == '0'
            and by_step[int(r['step']) - 1]['tick'] == r['tick'])), None)
        if stray is not None:
            print(f'S8 pair: {run.name} has a follow-up read at tick {stray} that does not follow its change-step read; '
                  'the comparison is partial')
        missing = next((int(r['tick']) for r in log if r['phase'] == '0' and not follows(r)), None)
        if missing is not None:
            print(f'S8 pair: {run.name} has a change-step read at tick {missing} with no read on the next step; '
                  'the comparison is partial')
        # Every tick is seen: change-step reads advance the counter by exactly one (it restarts
        # only at a launch), and the log does not end on a change step whose follow-up never came.
        # After the first live read the unit table must stay readable: a read without it hides rows.
        # The match starts at the first change step past tick 0; reads before it may lack the table.
        in_match = next((i for i, r in enumerate(log) if r['phase'] == '0' and r['tick'] != '0'), len(log))
        lost = next((int(r['tick']) for r in log[in_match:] if r['table'] == '0'), None)
        if lost is not None:
            print(f'S8 pair: {run.name} lost the unit table at tick {lost} during the match; the comparison is partial')
        changes = [int(r['tick']) for r in log if r['phase'] == '0']
        skipped = next((b for a, b in zip(changes, changes[1:]) if b != a + 1 and b != 0), None)
        if skipped is not None:
            print(f'S8 pair: {run.name} skips a tick (before tick {skipped}); the comparison is partial')
        steps = {int(r['step']) for r in log if re.fullmatch(r'[0-9]{1,12}', r.get('step') or '')}
        if any(w[1:2] == ['off'] for w in tap_lines):
            print(f'S8 pair: {run.name} turns the tap off during the run; the comparison is partial')
        # A command that changes the machine without a step (a mode switch, a save under the JIT)
        # after the last step line is never observed by a tap read.
        commands = list(frontend_lines((run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')))
        last_steps = max((i for i, w in enumerate(commands) if re.fullmatch(r'\+?[0-9]+', w[0])), default=-1)
        if any(w[0] in ('mode', 'save', 'load') for w in commands[last_steps + 1:]) or any(
                w[0] in ('mode', 'save') for w in commands):
            print(f'S8 pair: {run.name}\'s tape switches mode, saves (which changes a JIT run) or loads after its last '
                  'step; a tick-tap read may not see the change, so the comparison is partial')
        # A quiet tail (steps after the last tick change) is normal, but not one longer than the
        # longest interval between tick changes seen in the run.
        # Cadence from the match only (from its first change step past tick 0): a long lobby wait
        # must not stretch the allowed tail. Fewer than two in-match changes: cadence unknown.
        # ...within the final match: after the last restart of the tick counter, if any.
        restart = max((i for i in range(1, len(log)) if int(log[i]['tick']) < int(log[i - 1]['tick'])), default=0)
        final_match = next((i for i, r in enumerate(log) if i >= restart and r['phase'] == '0' and r['tick'] != '0'),
                           len(log))
        change_steps = sorted(int(r['step']) for r in log[final_match:] if r['phase'] == '0')
        longest = max((b - a for a, b in zip(change_steps, change_steps[1:])), default=0)
        if end is None:
            print(f'S8 pair: {run.name} has no verified end step; the comparison is partial')
        elif len(change_steps) < 2:
            print(f'S8 pair: {run.name}\'s final match has fewer than two tick changes, so its cadence and tail are '
                  'unproven; the comparison is partial')
        elif end - change_steps[-1] >= longest:
            print(f'S8 pair: {run.name}\'s last tick change is at step {change_steps[-1]}, at least one tick interval '
                  f'({longest} steps) before its verified end step {end}; the comparison is partial')
        # ...and from the launch: a read of tick 0 before the first read with live rows.
        first_live = next((i for i, r in enumerate(log) if r.get('live') not in (None, '', '0')), None)
        if first_live is None or not any(r.get('tick') == '0' for r in log[:first_live]):
            print(f'S8 pair: {run.name} has no read of tick 0 before its first live read (the tap may have started '
                  'after the launch); the comparison is partial')
    ends = [verified_end(run, manifest) for run, manifest in zip((a, b), checked[0])]
    if None not in ends and ends[0] != ends[1]:
        print(f'S8 pair: the runs end at different verified steps ({ends[0]} vs {ends[1]}); the comparison covers '
              'unequal horizons and is partial')
    left, right = phase_records(a), phase_records(b)
    if left is None or right is None:
        print('S8 pair: a tick log is missing; no verdict')
        return
    same_coverage, evidence = True, True
    # Every phase is compared: 0 (change step), 1 (follow-up) and, with `tap step`, the later reads.
    phases = sorted(set(left) | set(right) | {'0', '1'}, key=int)
    names = {'0': 'change-step', '1': 'follow-up'}
    for phase, name in ((p, names.get(p, f'phase-{p}')) for p in phases):
        l, r = left.get(phase, {}), right.get(phase, {})
        shared = [k for k in l if k in r]            # capture order (dicts keep insertion order)
        only = len(set(l) ^ set(r))
        conflicted = [k for k in shared if l[k][2] or r[k][2]]
        if conflicted:
            same_coverage = False
            print(f'S8 pair {name}: {len(conflicted)} reads had disagreeing cache aliases (first at tick '
                  f'{conflicted[0][0]}); they are not compared, so the comparison is partial')
        comparable = [k for k in shared if l[k][0] is not None and r[k][0] is not None]
        rows_differ = [k for k in comparable if l[k][0] != r[k][0]]
        no_table = sum(1 for k in shared if l[k][0] is None and r[k][0] is None)
        one_sided = sum(1 for k in shared if (l[k][0] is None) != (r[k][0] is None))
        globals_differ = [k for k in shared if k not in conflicted and l[k][1] != r[k][1]]
        same_coverage &= only == 0 and list(l) == list(r) and one_sided == 0
        if phase in ('0', '1'):
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
    recorded = (load_manifest(run).get('outputs_sha256') or {}).get('ticks.csv')
    log_ok = (run / 'ticks.csv').is_file() and recorded == file_sha256(run / 'ticks.csv')
    if not log_ok:
        print('opening: ticks.csv is missing or does not match its manifest; a restart between raw samples '
              'cannot be ruled out, so this report may merge two matches')
    try:
        log = read_csv(run / 'ticks.csv') if log_ok else []
    except ValueError as error:
        print(f'opening: {error}; match boundaries are unverified; no report')
        return
    if not all(all(re.fullmatch(r'[0-9]{1,12}', r.get(k) or '') for k in ('step', 'tick', 'live')) for r in log):
        print('opening: ticks.csv has unreadable rows, so match boundaries are unverified; no report')
        return
    # Only after the first live read: a lobby tapped before launch may count down first.
    log = log[next((i for i, r in enumerate(log) if r['live'] not in ('', '0')), len(log)):]
    reset_step = None
    for previous, current in zip(log, log[1:]):
        if int(current['tick']) < int(previous['tick']):
            reset_step = int(current['step'])
            print('opening: the tick counter restarts (a second match); only the first match is used')
            break
    # With the tap off part of the time, a relaunch can hide between samples: decline then. The
    # tape is trusted for this only if it matches its manifest.
    tape_ok = (run / 'tape.txt').is_file() and load_manifest(run).get('tape_sha256') == file_sha256(run / 'tape.txt')
    if not tape_ok:
        print('opening: tape.txt is missing or does not match its manifest; tap gaps cannot be ruled out, so this '
              'report may combine matches')
    tape = (run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape') if tape_ok else ''
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
    if len({tuple(w) for w in taps if w[1:2] != ['off']}) > 1:
        print('opening: the tap settings change during the run (raw sampling may pause); no report')
        return
    if not clean(load_manifest(run)):
        print('opening: the capture did not finish cleanly; this report may stop short of the run\'s end')
    raw_matches_manifest(run, 'opening')
    for step, tick, phase, rows in readable_records(run / 'ticks.raw', 'opening', before_step=reset_step):
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


def sweep(runs, ticks_limit, seed_address, legacy=False):
    """Seed sweep: launch seed, owner -> race (by worker type), and per race the order
    in which new unit types first appear (initial units excluded) up to ticks_limit."""
    table, late = collections.defaultdict(dict), collections.defaultdict(set)
    seen_runs, directory_identities = set(), set()
    for run in runs:
        # One capture counts once: by run id, or by resolved directory for older captures.
        identity_manifest = load_manifest(run)
        run_id = identity_manifest.get('run_id')
        if 'run_id' in identity_manifest and not (isinstance(run_id, str) and run_id):
            print(f'sweep {run.name}: the run id is not a nonempty string; skipped')
            continue
        if run_id is None and not legacy:
            print(f'sweep {run.name}: no run id, so copies cannot be told apart; skipped (--legacy counts older '
                  'captures by directory)')
            continue
        identity = run_id or str(run.resolve())
        if not run_id:
            directory_identities.add(identity)
        # The tape and tick log decide coverage, so they are verified before anything is parsed.
        run_manifest = load_manifest(run)
        if run_manifest.get('_malformed'):
            print(f'sweep {run.name}: manifest.json has a malformed {run_manifest["_malformed"]}; skipped')
            continue
        outputs = run_manifest.get('outputs_sha256') or {}
        unverified_inputs = [name for name, recorded in (('tape.txt', run_manifest.get('tape_sha256')),
                                                         ('ticks.csv', outputs.get('ticks.csv')))
                             if not (run / name).is_file() or recorded != file_sha256(run / name)]
        if unverified_inputs:
            print(f'sweep {run.name}: {", ".join(unverified_inputs)} missing or not matching the manifest; skipped')
            continue
        # Every field the sweep uses is checked once, before any of it is used.
        try:
            tick_log = read_csv(run / 'ticks.csv')
            valid = all(isinstance(r.get(k), str) and re.fullmatch(r'[0-9]{1,12}', r[k]) for r in tick_log
                        for k in ('step', 'tick', 'live', 'table', 'phase', 'same', 'dual_differ'))
        except (csv.Error, UnicodeDecodeError, ValueError):
            valid = False
        if not valid:
            print(f'sweep {run.name}: ticks.csv has unreadable rows; skipped')
            continue
        # Without --ticks the cutoff is the last tick whose reads are complete: a log that ends on a
        # change read has not seen that tick's follow-up. It is fixed before any data is collected.
        limit = ticks_limit
        step_mode = any(w[0] == 'tap' and w[1:2] == ['step'] for w in frontend_lines(
            (run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')))
        if not step_mode and any(r['phase'] not in ('0', '1') for r in tick_log):
            print(f'sweep {run.name}: ticks.csv has a read phase that `tap tick` never writes; skipped')
            continue
        if ticks_limit == 1 << 30 and tick_log:
            # In `tap step` mode a tick can have reads past its follow-up, so it is complete only once
            # the next tick has begun.
            incomplete = tick_log[-1]['phase'] == '0' or step_mode
            limit = int(tick_log[-1]['tick']) - (1 if incomplete else 0)
        # The match's seed is the last value written at or before its first in-match record (the
        # first unstrided tick read with live unit rows), so a stale seed from an earlier launch is
        # never reported. Without that boundary the seed is unknown.
        first_record = next((int(r['step']) for r in tick_log if r['live'] != '0'), None)
        # Only full 4-byte samples count, and a tape that ever turns watches off could leave a
        # stale value standing, so its seed is reported as unknown.
        tape = (run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape') if (run / 'tape.txt').is_file() else ''
        watch_off = any(w[:2] == ['watch', 'off'] for w in frontend_lines(tape))
        seed = None
        watch_ok = (run / 'watch.csv').is_file() and outputs.get('watch.csv') == file_sha256(run / 'watch.csv')
        if (run / 'watch.csv').is_file() and not watch_ok:
            print(f'sweep {run.name}: watch.csv does not match the manifest; seed unknown')
        def physical(text):
            try:
                return int(text, 16) & 0x1FFFFFFF
            except ValueError:
                return None
        # The frontend reads the width with `>>`, which takes a leading number (sign allowed).
        width = lambda text: int(re.match(r'\+?[0-9]{1,12}', text).group()) if re.match(r'\+?[0-9]{1,12}', text) else None
        watched = any(w[0] == 'watch' and len(w) >= 3 and width(w[2]) == 4
                      and physical(w[1]) == physical(seed_address) for w in frontend_lines(tape))
        if watch_ok and watched and first_record is not None and not watch_off:
            try:
                for r in read_csv(run / 'watch.csv'):
                    if r['address'] == seed_address and r['bytes'] == '4' and int(r['step']) <= first_record:
                        seed = int(r['value'], 16) or None
            except (KeyError, ValueError, TypeError, UnicodeDecodeError, csv.Error):
                print(f'sweep {run.name}: watch.csv is not a readable watch log; seed unknown')
                seed = None
        if not (run / 'ticks.raw').is_file():
            print(f'sweep {run.name}: no ticks.raw (no unit samples); skipped')
            continue
        # Coverage can only be judged for a tape that sets raw sampling once and never changes the
        # tap afterwards; its stride is the K of that `tap ... raw [K]` line (default 1).
        tap_lines = [w for w in frontend_lines(tape) if w[0] == 'tap']
        while tap_lines and tap_lines[0][1:2] == ['off']:
            tap_lines.pop(0)   # the tap starts off, so a leading `tap off` changes nothing
        raw_lines = [w for w in tap_lines if 'raw' in w]
        stride = 1
        if raw_lines:
            at = raw_lines[0].index('raw')
            if at + 1 < len(raw_lines[0]) and re.fullmatch(r'[0-9]{1,9}', raw_lines[0][at + 1]):
                stride = int(raw_lines[0][at + 1])
        if not 1 <= stride <= 1000000:   # the frontend refuses such a tap line
            print(f'sweep {run.name}: the tap stride {stride} is outside 1..1000000; skipped')
            continue
        # One tap line only: an earlier tick-only tap could supply a pre-launch read while raw
        # sampling began after units were built.
        fixed_tap = len(tap_lines) == 1 and len(raw_lines) == 1
        tap_mode = tap_lines[0][1] if tap_lines and len(tap_lines[0]) > 1 else '?'   # tick or step
        # With a stride above 1 the samples fall on a grid; runs on different grids order types
        # differently, so the grid offset is part of the totals key.
        first, initial, owners = {}, set(), {}
        start = last = None
        # raw sample ticks, read until one stride past the window; a file that does not match its
        # manifest digest counts as truncated (a cut between records reads like a clean end)
        raw_ticks, truncated = [], not raw_matches_manifest(run, f'sweep {run.name}')
        unrecorded = bool(run_manifest.get('unrecorded_state'))
        # A loaded state counts only with a recorded digest and producer (or under --legacy).
        loaded = [w[1] for w in frontend_lines(tape) if w[0] == 'load' and len(w) >= 2]
        sources = run_manifest.get('state_input_source') or {}
        digests = run_manifest.get('state_input_sha256') or {}
        # The staged state must match a real recorded digest; --legacy covers only a missing producer.
        unverified_load = [name for name in loaded
                           if not (isinstance(digests.get(name), str) and re.fullmatch(r'[0-9a-f]{64}', digests[name])
                                   and (run / 'input' / name).is_file()
                                   and file_sha256(run / 'input' / name) == digests[name])]
        unproven_load = [name for name in loaded if name not in unverified_load and not producer_ok(sources.get(name))]
        # --legacy covers a producer record that is missing, never one that is present but malformed.
        malformed_source = [name for name in unproven_load if name in sources]   # present, even as null
        if malformed_source:
            unverified_load += malformed_source
            unproven_load = [name for name in unproven_load if name not in malformed_source]
        if unproven_load and legacy:
            print(f'sweep {run.name}: loads {", ".join(unproven_load)} with no recorded producer; counted under --legacy')
            unproven_load = []
        unproven_load += unverified_load
        failed = not clean(run_manifest)
        raw_reads = []                     # (tick, step) of every raw record, for the readability check
        raw_written = set()                # (step, tick, phase) of every record, empty pre-match ones too
        raw_live = {}                      # step: rows in that record
        previous_raw_step = None
        records = raw_records(run / 'ticks.raw')
        while True:
            try:
                step, tick, phase, rows = next(records)
            except StopIteration:
                break
            except (struct.error, ValueError):
                truncated = True
                break
            if previous_raw_step is not None and step <= previous_raw_step:
                truncated = True   # a repeated or out-of-order record: the file is not one capture's
                break
            previous_raw_step = step
            raw_written.add((step, tick, phase))
            if start is None and not rows:
                continue   # records before the first live one are not part of the match
            if not raw_ticks or raw_ticks[-1] != tick:
                raw_ticks.append(tick)
            raw_reads.append((tick, step, phase))
            raw_live[step] = len(rows)
            if tick > limit:
                if start is not None and (tick >= limit + stride or ticks_limit == 1 << 30):
                    break
                if start is not None:
                    continue
            last = (tick, rows)
            for body in rows.values():
                owner, kind = body[0x4C], struct.unpack_from('>H', body, 0x64)[0]
                if start is None:   # the first live sample alone; a later read of that tick is not the roster
                    initial.add((owner, kind))
                if kind in WORKERS:
                    owners.setdefault(owner, WORKERS[kind])
                first.setdefault((owner, kind), (tick, step))
            if start is None:
                start, start_step = tick, step
        if start is None or start > limit:
            print(f'sweep {run.name}: no sample with live rows by tick {limit}; no starting roster')
            continue
        # Coverage comes from the unit samples themselves (raw logging may stop while ticks.csv
        # goes on): inside the window no gap may exceed the configured stride, and the samples
        # must reach the window's end.
        window = raw_ticks             # starts at the first live raw record
        inside = [t for t in window if t <= limit]
        end = limit   # from --ticks, or the last tick the tick log shows complete
        # One match per capture: any restart of the tick counter after the first live read (a new
        # launch) refuses the run, so its seed, roster and coverage all belong to one match. A
        # lobby tapped before launch may count down first; that is not a second match. Only
        # reads from the first live raw record's step on count toward coverage.
        full_log = tick_log
        first_live_at = next((i for i, r in enumerate(full_log) if r['live'] not in ('', '0')), len(full_log))
        all_ticks = [int(r['tick']) for r in full_log[first_live_at:]]
        reset = next((b for a, b in zip(all_ticks, all_ticks[1:]) if b < a), None)
        if reset is None:
            reset = next((b for a, b in zip(window, window[1:]) if b <= a), None)
        log = [r for r in full_log if int(r['step']) >= start_step]
        log_ticks = [int(r['tick']) for r in log]
        # A sample is missing when the next one comes later than one stride, before the cutoff.
        gap = next((a + stride for a, b in zip(window, window[1:]) if b - a > stride and a + stride <= end), None)
        # The tick log itself must be complete through the cutoff before it can prove raw coverage:
        # in-match change reads advance by one tick, and each is followed on the next step by its
        # follow-up (same tick) or by the next tick's change read.
        by_step = {int(r['step']): r for r in full_log}
        def followed(read):
            after = by_step.get(int(read['step']) + 1)
            return after is not None and ((after['phase'] != '0' and after['tick'] == read['tick'])
                                          or (after['phase'] == '0' and int(after['tick']) == int(read['tick']) + 1))
        match_rows = [r for r in full_log[first_live_at:] if int(r['tick']) <= end]
        match_changes = [r for r in match_rows if r['phase'] == '0']
        first_tick = int(full_log[first_live_at]['tick']) if first_live_at < len(full_log) else start
        have = {int(r['tick']) for r in match_changes}
        # Every tick from the first live one through the cutoff needs its own change read.
        log_gap = next((t for t in range(first_tick, end + 1) if t not in have), None)
        if log_gap is None:
            log_gap = next((int(r['tick']) for r in match_changes if not followed(r)), None)
        if log_gap is None:
            # Every later read belongs to the latest change read: same tick, and in `tap step` mode
            # one step and one phase after the read before it.
            if not step_mode:
                at_step = {int(r['step']): r for r in match_rows}
                log_gap = next((int(r['tick']) for r in match_rows if r['phase'] == '1' and not (
                    int(r['step']) - 1 in at_step and at_step[int(r['step']) - 1]['phase'] == '0'
                    and at_step[int(r['step']) - 1]['tick'] == r['tick'])), None)
                steps_seen = [int(r['step']) for r in match_rows]
                if log_gap is None and any(y <= x for x, y in zip(steps_seen, steps_seen[1:])):
                    log_gap = next(int(r['tick']) for r, y in zip(match_rows, steps_seen[1:]) if y <= int(r['step']))
            if step_mode:
                # Through the next tick's first read, so the cutoff tick's own later phases count.
                through = full_log[first_live_at:first_live_at + len(match_rows) + 1]
                log_gap = next((int(y['tick']) for x, y in zip(through, through[1:])
                                if int(y['step']) != int(x['step']) + 1), None)
                if log_gap is None and len(through) == len(match_rows):
                    log_gap = end   # the log stops at the cutoff tick: its later reads are unseen
            previous = None
            for r in ([] if log_gap is not None else match_rows):
                if r['phase'] != '0' and (previous is None or r['tick'] != previous['tick'] or (
                        step_mode and (int(r['step']) != int(previous['step']) + 1
                                       or int(r['phase']) != int(previous['phase']) + 1))):
                    log_gap = int(r['tick'])
                    break
                previous = r
        if log_gap is not None and gap is None:
            gap = log_gap
        # The raw records the frontend must have written, derived from the tick log: the change
        # read of every `stride`th tick change, and each later read of that tick whose rows changed
        # (`same` = 0). Every one through the cutoff must be present.
        raw_present = raw_written
        expected, changes, sampled = [], 0, False
        for r in full_log:
            if r['phase'] == '0':
                changes += 1
                sampled = changes % stride == 0
                if sampled:
                    expected.append((int(r['tick']), int(r['step']), 0))
            elif sampled and r['same'] == '0':
                expected.append((int(r['tick']), int(r['step']), int(r['phase'])))
        # From the first live read of the tick log (not the first raw sample, which could itself be late).
        live_from = int(full_log[first_live_at]['step']) if first_live_at < len(full_log) else start_step
        missing_raw = next((t for t, st, ph in expected if st >= live_from and t <= end
                            and (st, t, ph) not in raw_present), None)
        if missing_raw is not None and gap is None:
            gap = missing_raw
        # ...and no record the stride would not have written: an off-grid sample could add a type.
        expected_keys = {(st, t, ph) for t, st, ph in expected}
        off_grid = next((t for t, st, ph in raw_reads if st >= live_from and t <= end
                         and (st, t, ph) not in expected_keys), None)
        # The frontend's own counters must account for every tick-log row; without --ticks the
        # cutoff comes from the log, so the counters are required then.
        reads = status_counter(run, run_manifest, 'tap_reads')
        tick_changes = status_counter(run, run_manifest, 'tick_changes')
        if reads is not None and tick_changes is not None:
            counters_bad = reads != len(full_log) or tick_changes != sum(r['phase'] == '0' for r in full_log)
        else:
            counters_bad = ticks_limit == 1 << 30
        run_end = verified_end(run, run_manifest)
        if run_end is not None and (any(int(r['step']) > run_end for r in full_log)
                                    or any(st > run_end for st, _, _ in raw_written)):
            counters_bad = True   # reads after the run's last step cannot be real
        # Every raw record counts only if its own read (same step) found the unit table.
        readable = {int(r['step']): r.get('table', '0') not in ('', '0') for r in log}
        unreadable = next((t for t, st, _ in raw_reads if start <= t <= end and not readable.get(st, False)), None)
        # ...and only if it is the read the tick log has at that step: same tick, phase and live count.
        log_at = {int(r['step']): r for r in log}
        disagree = next((t for t, st, ph in raw_reads if start <= t <= end and (
            st not in log_at or int(log_at[st]['tick']) != t or int(log_at[st]['phase']) != ph
            or int(log_at[st]['live']) != raw_live[st])), None)
        # A read whose cache aliases disagreed may have hashed the wrong copy: such a run is out.
        launch_reads = [r for r in full_log[:first_live_at] if r['tick'] == '0']   # every phase of the launch
        conflicted = next((int(r['tick']) for r in launch_reads + full_log[first_live_at:] if int(r['tick']) <= end
                           and r['dual_differ'] != '0'), None)
        # Both the raw samples and the unstrided tick log must reach an explicit cutoff.
        # The tick log must go past the cutoff tick, so the cutoff tick's own reads are complete.
        reaches = window[-1] + stride > limit and (ticks_limit == 1 << 30 or max(log_ticks, default=-1) > limit)
        # The tap must have seen the launch: a read with the tick counter at 0 before the first live
        # read. A read with no rows alone is not enough (an unreadable table also gives none), and
        # otherwise units built before the first read would count as starting units.
        launch_covered = first_live_at < len(full_log) and any(
            r['tick'] == '0' for r in full_log[:first_live_at])
        # Between launch and the first live read the game must not have advanced unseen: a read
        # past tick 0 with no readable table means units may have appeared before the roster.
        last_zero = max((i for i, r in enumerate(full_log[:first_live_at]) if r['tick'] == '0'), default=None)
        if launch_covered and any(r['tick'] != '0' for r in full_log[last_zero + 1:first_live_at]):
            launch_covered = False
        # ...and the first live read must come at tick 1: a jump from tick 0 to a later tick
        # leaves ticks whose units would count as starting units.
        wrong_first_tick = launch_covered and int(full_log[first_live_at]['tick']) != 1
        if wrong_first_tick:
            launch_covered = False
        short = (truncated or unrecorded or bool(unproven_load) or failed or conflicted is not None
                 or not launch_covered or not fixed_tap
                 or reset is not None or gap is not None or unreadable is not None or disagree is not None
                 or off_grid is not None or counters_bad or not reaches)
        cutoff = min(end, inside[-1])
        if short:
            reason = ('a truncated, unverified or out-of-order ticks.raw' if truncated
                      else 'a state no capture recorded (--unrecorded-state)' if unrecorded
                      else 'a loaded state that is not verified or has no recorded producer (--legacy admits older '
                           'captures whose staged state verifies)' if unproven_load
                      else 'a capture that did not finish cleanly' if failed
                      else f'a read with disagreeing cache aliases at tick {conflicted}' if conflicted is not None
                      else f'a first live read at tick {full_log[first_live_at]["tick"]}, not tick 1 (ticks in between '
                           'were not seen)' if wrong_first_tick
                      else 'no read of tick 0 before the first live one (the tap may have started after the launch)'
                      if not launch_covered
                      else 'tap settings that change during the run' if not fixed_tap
                      else f'a tick counter reset to {reset} (more than one match in the capture)' if reset is not None
                      else f'unit samples or tick reads ending before the cutoff (last sample at tick {window[-1]})'
                      if not reaches
                      else f'a missing unit sample at tick {gap}' if gap is not None
                      else f'no readable unit table at tick {unreadable}' if unreadable is not None
                      else f'a raw record at tick {disagree} that does not match its tick-log read (tick, phase or '
                           'live count)' if disagree is not None
                      else f'a raw record at tick {off_grid} that the stride would not have written' if off_grid is not None
                      else 'a tick log that status.log does not confirm (its read counters differ, it has reads past '
                           'the verified end step, or, without --ticks, there are no counters to confirm its end)')
            print(f'sweep {run.name}: {reason} (configured stride {stride}); counted through tick {cutoff} only and '
                  'left out of the totals')
        # The starting roster is exact only when the first raw sample is the first live read. With a
        # stride above 1 it can be up to stride - 1 ticks later; a unit type first made in that gap
        # then counts as starting. Such a run stays in the totals, which say how many are affected.
        late_baseline = None
        if first_live_at < len(full_log):
            first_live = int(full_log[first_live_at]['tick'])
            if first_live != start or int(full_log[first_live_at]['step']) != start_step:
                gap_rows = [r for r in full_log[first_live_at:] if int(r['step']) <= start_step]
                changed = len({r['live'] for r in gap_rows}) > 1
                late_baseline = (first_live, changed)
                print(f'  warning: starting roster is the tick-{start} sample, but rows were live from tick {first_live}'
                      f'{" and the live count changed in between" if changed else ""}; a unit type first made in '
                      'between counts as starting')
        built = collections.defaultdict(list)
        for (owner, kind), (tick, step) in sorted(first.items(), key=lambda item: (item[1], item[0][1])):
            if (owner, kind) not in initial and owner in owners:
                built[owner].append((tick, step, kind))
        if reset is not None:
            continue   # two matches: no per-run opening or census either
        human = [o for o in owners if not built[o]]
        print(f'sweep {run.name}: seed={seed} owners={dict(sorted(owners.items()))} '
              f'no new unit type through tick {cutoff}={human}')
        for owner, race in sorted(owners.items()):
            if built[owner]:
                # Types first seen in the same sample (the same read step) have no known order
                # between them, so each sample is one group: {a,b}@tick.
                groups = collections.OrderedDict()
                for tick, step, kind in built[owner]:
                    groups.setdefault(step, (tick, []))[1].append(kind)
                order = ' '.join((f'{kinds[0]}' if len(kinds) == 1 else '{' + ','.join(map(str, sorted(kinds))) + '}')
                                 + f'@{tick}' for tick, kinds in groups.values())
                offset = start % stride
                if not short and identity in seen_runs and owner == min(o for o in owners if built[o]):
                    print(f'sweep {run.name}: the same capture as an earlier counted argument (run id or '
                          'directory); not counted again')
                if not short and identity not in seen_runs:
                    # Totals are kept per stride: a coarser sample can merge what a finer one orders.
                    table[(race, stride, tap_mode, offset)][(identity, owner)] = tuple(frozenset(kinds) for _, kinds in groups.values())
                print(f'  {race:8} owner {owner}: {order}')
        if late_baseline and not short and identity not in seen_runs:
            for owner, race in owners.items():
                late[(race, stride, tap_mode, start % stride)].add(identity)   # the gap may have hidden its only new type
        if not short:
            seen_runs.add(identity)
        if last:
            counts = census(last[1])
            per = {o: sum(n for (owner, _, _), n in counts.items() if owner == o) for o in sorted(owners)}
            print(f'  census at tick {last[0]} (live rows by owner): {per}')
    # Totals are kept per raw stride and tap mode: a coarser schedule can merge what a finer one orders.
    for race, stride, tap_mode, offset in sorted(set(table) | set(late)):
        orders = table.get((race, stride, tap_mode, offset), {})
        distinct = collections.Counter(orders.values())
        window = f'by tick {ticks_limit}' if ticks_limit != 1 << 30 else 'within each run\'s own last sample'
        print(f'sweep {race}: {len(distinct)} distinct opening orders over {len(orders)} owners with a new unit type {window}'
              + f' (tap {tap_mode}, raw stride {stride}' + (f', samples at tick {offset} mod {stride}' if stride > 1 else '')
              + ('; types first seen in the same sample count as one unordered group)' if stride > 1 else ')'))
        legacy_ids = [i for i in directory_identities if any(key[0] == i for key in orders)]
        if legacy_ids:
            print(f'  note: {len(legacy_ids)} of these runs have no run id (--legacy) and are told apart by directory '
                  'only, so their independence is not verified')
        if late[(race, stride, tap_mode, offset)]:
            print(f'  warning: {len(late[(race, stride, tap_mode, offset)])} runs with this race have a starting roster taken after the first live tick '
                  '(a raw stride above 1 or an empty first sample); a unit type first made in that gap would count as starting and be missing from their orders')


def raw_matches_manifest(run, label):
    """A ticks.raw cut exactly between records reads like a clean end, so it is also checked
    against the digest its manifest records. Returns False (after a warning) on a mismatch."""
    recorded = (load_manifest(run).get('outputs_sha256') or {}).get('ticks.raw')
    raw = run / 'ticks.raw'
    if recorded is None:
        print(f'{label}: no manifest digest for ticks.raw; a cut between records cannot be detected, so this '
              'report may be partial')
        return False
    if not raw.is_file() or file_sha256(raw) != recorded:
        print(f'{label}: ticks.raw does not match its manifest (cut short or changed); this report is partial')
        return False
    return True


def readable_records(path, label, before_step=None):
    """raw_records for the single-run reports: a truncated or corrupt record ends the read with
    a warning that the report covers only the records before it, instead of a traceback."""
    if not path.is_file():
        print(f'{label}: no {path.name} (the tape has no `tap ... raw`); nothing to report')
        return
    records = raw_records(path, before_step)
    last_step = None
    while True:
        try:
            record = next(records)
        except StopIteration:
            return
        except (struct.error, ValueError) as error:
            print(f'{label}: {error}; this report covers only the records before it (last step {last_step})')
            return
        last_step = record[0]
        yield record


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
    raw_matches_manifest(run, 'S2')
    for step, tick, phase, rows in readable_records(raw, 'S2'):
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
        raw_matches_manifest(run, 'S3')
        for step, tick, phase, rows in readable_records(raw, 'S3'):
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
    # Pad commands answer "OK step wall cpu"; "until"/"untilw" answer "UNTIL..."/"UNTILW..." and are not timed here.
    replies = [l.split() for l in open(run / 'bridge.log')
               if (l.startswith('OK ') and len(l.split()) == 4) or l.startswith(('UNTIL ', 'UNTILMISS ', 'UNTILW ', 'UNTILWMISS ', 'PULSE '))]
    tape = (run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape') if (run / 'tape.txt').is_file() else ''
    if not tape or (len(replies) != len(rows) and not any(r[0] in ('UNTILMISS', 'UNTILWMISS') for r in replies)):
        print(f'throughput: {len(replies)} timed replies for {len(rows)} commands; tape copy present={bool(tape)}')
        return
    import json
    mode, tap = json.loads((run / 'manifest.json').read_text()).get('mode', 'jit'), 'off'
    totals = collections.defaultdict(lambda: [0, 0, 0])
    index = 0
    for words in frontend_lines(tape):
        word = words[0]
        if word in ('until', 'untilw', 'pulse'):
            if index >= len(replies) or replies[index][0] not in ('UNTIL', 'UNTILMISS', 'UNTILW', 'UNTILWMISS', 'PULSE'):
                print('throughput: replies do not follow the tape; no report')
                return
            if replies[index][0] in ('UNTILMISS', 'UNTILWMISS'):
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
    parser.add_argument('--legacy', action='store_true',
                        help='--s1, --compare, --s8, --sweep: accept captures made before capture.py wrote run ids '
                             'and state producers (never a PASS)')
    parser.add_argument('--opening', type=pathlib.Path)
    parser.add_argument('--ticks', type=int, default=1 << 30, help='last tick for --opening and --sweep')
    parser.add_argument('--sweep', nargs='+', type=pathlib.Path, metavar='RUN')
    parser.add_argument('--seed-address', default='800eff20', help='watch.csv address of the launch seed')
    parser.add_argument('--marine-type', type=int, default=0)
    args = parser.parse_args()
    if args.s1:
        s1(*args.s1, legacy=args.legacy)
    if args.s2:
        s2(args.s2, args.marine_type)
    if args.s3:
        s3(args.s3)
    if args.throughput:
        throughput(args.throughput)
    if args.compare:
        compare(*args.compare, set(args.exclude_block), legacy=args.legacy)
    if args.watch:
        watch_summary(args.watch)
    if args.s8:
        s8_pair(*args.s8, legacy=args.legacy)
    if args.opening:
        opening(args.opening, args.ticks)
    if args.sweep:
        sweep(args.sweep, args.ticks, args.seed_address, legacy=args.legacy)


if __name__ == '__main__':
    main()
