#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Scenario measurements shared by the N64 golden writer and the GBA comparator.

Both sides are reduced to the same normalised series before anything is
measured, so one function computes each number for both engines:

  {'source': 'n64'|'gba',
   'frames': [{'sf': source frame, 'units': {id: (sc_type, owner, x, y)},
               'banks': {owner: (minerals, minerals_gathered or None)}}, ...]}

Positions are N64 map pixels (GBA pixels times two). Unit types are the
cartridge's unit ids (the GBA trace's raw_type; -1 on profiles whose trace has
no raw types, such as zerg-z10, so a type filter there matches nothing). Time is source frames: N64
unpaused ticks counted from the first in-mission tick, GBA simulation steps
* 4 / 5.

Each scenario in SCENARIOS names its GBA runner arguments, how the N64
capture is measured (measure_n64) and how a GBA trace is measured
(measure_gba). An observation is {check, subject, value, lo, hi, gating,
tolerance, layout_fields}; the band [lo, hi] is the N64 value widened by the
tolerance written next to it, each derived from the S4 data
(docs/PARITY-HARNESS.md).
"""
import collections
import functools
import math
import pathlib
import re
import statistics

MARINE, SCV, DRONE, ZERGLING = 0, 7, 41, 37
MINERAL_TYPES = {176, 177, 178}
TRIP_MINERALS = 8
CHORD = 8  # source frames per chord in the cruise-speed measure


# ---- tape ------------------------------------------------------------------

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


# The frontend reads a pad line's count with `>>` into a long long, so a sign is allowed.
STEP_COUNT = re.compile(r'[+-]?[0-9]+')


def tape_commands(path):
    """Step numbers of '# mark NAME' comments in a capture tape (the frontend skips comments).

    A mark falls where the next step-advancing command (a pad line, `until` or `pulse`) began. When
    the run's commands.csv is beside the tape, those begin steps are the executed ones, which
    covers `load NAME STEP` renumbering and `until`/`untilw` waits of any length. Without it the pad
    steps are summed, which is only valid for tapes with neither command."""
    tape = pathlib.Path(path)
    # Read as the frontend does: lines split at LF, tokens at C whitespace, a line starting with
    # '#' is a comment (marks are '# mark NAME'), nothing after `quit` runs.
    lines = []
    for line in tape.read_bytes().decode('utf-8', 'surrogateescape').split('\n'):
        words = [w for w in re.split('[ \t\n\v\f\r]+', line) if w]
        if line[:1] == '#':
            lines.append(('mark', words[2]) if len(words) >= 3 and words[:2] == ['#', 'mark'] else ('comment', None))
        elif words:
            if words[0] == 'quit':
                break
            lines.append(('command', words[0]))
    counts = collections.Counter(word for kind, word in lines if kind == 'mark')
    duplicates = sorted(name for name, n in counts.items() if n > 1)
    if duplicates:
        raise SystemExit(f'{tape}: mark names used more than once: {", ".join(duplicates)}')
    steps = lambda word: STEP_COUNT.fullmatch(word) is not None
    executed = tape.parent / 'commands.csv'
    if executed.is_file():
        rows = executed.read_text().splitlines()[1:]
        begins = [(int(r.split(',')[0]), int(r.split(',')[1])) for r in rows if r]
        marks, index = {}, 0
        for kind, word in lines:
            if kind == 'mark':
                if index < len(begins):
                    marks[word] = begins[index][0]
                elif begins:
                    marks[word] = begins[-1][0] + begins[-1][1]
                else:
                    raise SystemExit(f'{tape}: mark {word} has no executed command')
            elif kind == 'command' and (steps(word) or word in ('until', 'untilw', 'pulse')):
                index += 1
        if index != len(begins):
            raise SystemExit(f'{tape}: {index} step commands but {len(begins)} rows in commands.csv')
        return marks
    marks, step = {}, 0
    for kind, word in lines:
        if kind == 'command' and word in ('load', 'until', 'untilw', 'pulse'):
            raise SystemExit(f'{tape}: marks after load or until need the run\'s commands.csv')
        if kind == 'mark':
            marks[word] = step
        elif kind == 'command' and steps(word):
            step += int(word)
    return marks


def n64_command_frame(series, mark):
    """The last source frame read before the press that starts after frontend step
    `mark`: its follow-up read was taken at or before the mark, so no step with
    the button held had run yet."""
    t0 = None
    for frame in series['frames']:
        if frame['step'] <= mark:
            t0 = frame['sf']
    # A tick that ran inside one frontend step before the mark (no follow-up read) still counts.
    for step, sf in series.get('skipped', []):
        if step <= mark and (t0 is None or sf > t0):
            t0 = sf
    if t0 is None:
        raise SystemExit('the capture has no in-mission frame before the command mark')
    return t0


def gba_boundary_ok(at_step):
    """The GBA pre-command step must own its source frame, or collapsing steps into
    frames would put a post-command state at the command frame."""
    before = at_step - 1
    return before >= 1 and (before - 1) * 4 // 5 != before * 4 // 5 != at_step * 4 // 5


# ---- generic measurements --------------------------------------------------

def track(series, uid, since):
    """[(sf, x, y)] for one unit from source frame `since` on (last sample per frame)."""
    out = {}
    for frame in series['frames']:
        if frame['sf'] < since:
            continue
        unit = frame['units'].get(uid)
        if unit is not None:
            out[frame['sf']] = (unit[2], unit[3])
    return [(sf, *out[sf]) for sf in sorted(out)]


def movement(points):
    """Start, arrival (first frame after which it never moves again), cruise speed and path length."""
    if len(points) < 3:
        return None
    steps = []
    for (s0, x0, y0), (s1, x1, y1) in zip(points, points[1:]):
        steps.append((s1, math.hypot(x1 - x0, y1 - y0) / max(1, s1 - s0)))
    moving = [sf for sf, d in steps if d > 0]
    if not moving:
        return None
    start, arrive = moving[0], moving[-1]
    path = sum(math.hypot(b[1] - a[1], b[2] - a[2]) for a, b in zip(points, points[1:]))
    # Cruise: the middle half of the moving interval, so acceleration at the
    # start and the slow-down near the goal stay out.
    lo = start + (arrive - start) // 4
    hi = arrive - (arrive - start) // 4
    window = [d for sf, d in steps if lo <= sf <= hi]
    # Speed along the path from CHORD-frame chords: long enough to average out
    # the GBA's 2-px position grain, short enough to follow a bend.
    span = [p for p in points if lo - 1 <= p[0] <= hi]
    chords, frames = 0.0, 0
    for k in range(0, len(span) - CHORD, CHORD):  # k + CHORD stays inside span
        a, b = span[k], span[k + CHORD]
        chords += math.hypot(b[1] - a[1], b[2] - a[2])
        frames += b[0] - a[0]
    cruise = chords / frames if frames else 0.0
    return {'start': start, 'arrive': arrive, 'cruise': cruise, 'cruise_median_step': statistics.median(window) if window else 0.0,
            'path': path, 'end': (points[-1][1], points[-1][2]), 'begin': (points[0][1], points[0][2]),
            'settled_frames': points[-1][0] - arrive}


def spread(points):
    cx = sum(p[0] for p in points) / len(points)
    cy = sum(p[1] for p in points) / len(points)
    rms = math.sqrt(sum((p[0] - cx) ** 2 + (p[1] - cy) ** 2 for p in points) / len(points))
    return rms, (cx, cy), (max(p[0] for p in points) - min(p[0] for p in points),
                          max(p[1] for p in points) - min(p[1] for p in points))


def observation(check, subject, value, lo, hi, tolerance, gating, fields, **extra):
    obs = {'check': check, 'subject': subject, 'value': round(value, 4) if isinstance(value, float) else value,
           'lo': round(lo, 4) if isinstance(lo, float) else lo, 'hi': round(hi, 4) if isinstance(hi, float) else hi,
           'tolerance': tolerance, 'gating': gating, 'layout_fields': fields}
    obs.update(extra)
    return obs


# ---- T1 five-Marine move ---------------------------------------------------

MOVE_FIELDS = ['unit.type', 'unit.owner', 'unit.x', 'unit.y', 'unit.sprite', 'clock.tick_counter', 'clock.pause_halfword']
# Tolerances (source frames or N64 pixels), from the S4 captures and the two
# engines' quantisation; docs/PARITY-HARNESS.md gives the derivation.
MOVE_TIME_TOLERANCE = 2      # +-1 tick on each end of an interval, +-1 GBA step rounding is inside it
MOVE_SPEED_TOLERANCE = 0.10  # relative: GBA 2-px position grain over a ~25-frame cruise window, plus speed rounding
MOVE_SPREAD_TOLERANCE = 4.0  # N64 px: two GBA pixels per axis


def move_measure(series, movers, t0, goal=None):
    """goal: the point the remaining-distance curve measures to (default: the final centroid)."""
    per = {}
    for uid in movers:
        m = movement(track(series, uid, t0))
        if m is None:
            return None, f'unit {uid} never moved after frame {t0}'
        per[uid] = m
    ends = [m['end'] for m in per.values()]
    begins = [m['begin'] for m in per.values()]
    rms_end, centroid_end, box_end = spread(ends)
    rms_begin, centroid_begin, _ = spread(begins)
    start = min(m['start'] for m in per.values())
    result = {
        't0': t0, 'start': start, 'latency': start - t0,
        'arrival_first': min(m['arrive'] for m in per.values()) - start,
        'arrival_last': max(m['arrive'] for m in per.values()) - start,
        'cruise': statistics.median(m['cruise'] for m in per.values()),
        'cruise_min': min(m['cruise'] for m in per.values()),
        'cruise_max': max(m['cruise'] for m in per.values()),
        'spread_end': rms_end, 'spread_begin': rms_begin, 'box_end': box_end,
        'centroid_end': centroid_end, 'centroid_begin': centroid_begin,
        'min_settled': min(m['settled_frames'] for m in per.values()),
        'units': {str(k): {kk: (list(vv) if isinstance(vv, tuple) else vv) for kk, vv in v.items()} for k, v in per.items()},
    }
    # Remaining distance of the group centroid to the goal, per frame from the start.
    gx, gy = goal if goal is not None else centroid_end
    curve = []
    tracks = {uid: dict((p[0], (p[1], p[2])) for p in track(series, uid, start)) for uid in movers}
    last = {uid: None for uid in movers}
    for sf in range(start, start + result['arrival_last'] + 1):
        for uid in movers:
            if sf in tracks[uid]:
                last[uid] = tracks[uid][sf]
        if any(v is None for v in last.values()):
            continue
        cx = sum(v[0] for v in last.values()) / len(movers)
        cy = sum(v[1] for v in last.values()) / len(movers)
        curve.append(round(math.hypot(cx - gx, cy - gy), 2))
    result['remaining_curve'] = curve
    return result, None


def move_observations(m):
    t = MOVE_TIME_TOLERANCE
    obs = [
        observation('move.cruise_speed', 'Marine', m['cruise'],
                    min(m['cruise_min'], m['cruise']) * (1 - MOVE_SPEED_TOLERANCE),
                    max(m['cruise_max'], m['cruise']) * (1 + MOVE_SPEED_TOLERANCE),
                    f'N64 per-unit range widened by {MOVE_SPEED_TOLERANCE:.0%}', True, MOVE_FIELDS, unit='N64 px per source frame'),
        observation('move.arrival_first', 'Marine x5', m['arrival_first'], m['arrival_first'] - t, m['arrival_first'] + t,
                    f'+-{t} source frames', True, MOVE_FIELDS, unit='source frames from the group starting to move'),
        observation('move.arrival_last', 'Marine x5', m['arrival_last'], m['arrival_last'] - t, m['arrival_last'] + t,
                    f'+-{t} source frames', True, MOVE_FIELDS, unit='source frames from the group starting to move'),
        observation('move.group_spread', 'Marine x5', m['spread_end'], m['spread_end'] - MOVE_SPREAD_TOLERANCE,
                    m['spread_end'] + MOVE_SPREAD_TOLERANCE, f'+-{MOVE_SPREAD_TOLERANCE} N64 px', True, MOVE_FIELDS,
                    unit='RMS distance of final positions from their centroid, N64 px'),
        observation('move.start_latency', 'Marine x5', m['latency'], m['latency'] - t, m['latency'] + t,
                    'report only: N64 command tick comes from the tape step, not a RAM field', False, MOVE_FIELDS,
                    unit='source frames from the command to the first movement'),
    ]
    return obs


def move_measure_n64(cap, marks):
    series = cap.series(gating=True)
    t0 = n64_command_frame(series, marks['command'])
    first = series['frames'][0]['units']
    marines = {uid: u for uid, u in first.items() if u[0] == MARINE}
    owners = {u[1] for u in marines.values()}
    if len(owners) != 1:
        raise SystemExit(f't1-move: Marines belong to {sorted(owners)}; expected one human owner')
    human = owners.pop()
    # Movers: Marines that travelled at least half as far as the furthest one, so a
    # bystander nudged by collision is not counted.
    moved = {}
    for uid in marines:
        points = track(series, uid, t0)
        if points:
            moved[uid] = math.hypot(points[-1][1] - points[0][1], points[-1][2] - points[0][2])
    longest = max(moved.values(), default=0)
    movers = [uid for uid, d in moved.items() if longest and d >= longest / 2]
    if len(movers) != 5:
        raise SystemExit(f't1-move: {len(movers)} Marines moved; the scenario needs exactly five')
    last_sf = series['frames'][-1]['sf']
    for uid in movers:
        points = track(series, uid, t0)
        kept = all(f['units'].get(uid, (None, None))[:2] == (MARINE, human) for f in series['frames'] if f['sf'] >= t0)
        if not points or points[-1][0] != last_sf or not kept:
            raise SystemExit(f't1-move: Marine row {uid} does not stay a human Marine to the end of the capture')
    m, err = move_measure(series, sorted(movers), t0)
    if err:
        raise SystemExit('t1-move: ' + err)
    if m['min_settled'] < 30:
        raise SystemExit(f't1-move: capture ends {m["min_settled"]} frames after the last arrival; need 30')
    starts = [first[uid] for uid in sorted(movers)]
    target = (round(m['centroid_end'][0]), round(m['centroid_end'][1]))
    gba = {
        'scenario': 'move', 'profile': 'terran',
        # Identity join: type, owner and halved position at frame 0.
        'units': [[u[2] // 2, u[3] // 2] for u in starts],
        'target': [target[0] // 2, target[1] // 2],
        'at_step': 5,
        'steps': int((m['latency'] + m['arrival_last'] + 120) * 5 / 4) + 10,
    }
    notes = [f'human owner {human}; {len(movers)} Marines moved; the target is the movers\' final centroid '
             f'(inferred: cursor RAM is not located), N64 ({target[0]},{target[1]})']
    return {'observations': move_observations(m), 'gba': gba, 'notes': notes,
            'curves': {'move.remaining': {'n64': m['remaining_curve'], 'tolerance': 2 * MOVE_SPREAD_TOLERANCE,
                                          'unit': 'N64 px from the group centroid to the target, per source frame from the start'}},
            'measured': {k: v for k, v in m.items() if k != 'remaining_curve'}}


def move_measure_gba(series, golden, report):
    params = golden['gba']
    movers = report.get('movers') or []
    if len(movers) != len(params['units']):
        return None, f'joined {len(movers)} of {len(params["units"])} Marines'
    t0 = report['command_sf']
    last_sf = series['frames'][-1]['sf']
    for uid in movers:
        points = track(series, uid, t0)
        if not points or points[-1][0] != last_sf:
            return None, f'Marine slot {uid} is gone before the end of the run'
    goal = (2 * params['target'][0], 2 * params['target'][1])
    m, err = move_measure(series, movers, t0, goal)
    if err:
        return None, err
    if m['min_settled'] < 30:
        return None, f'a Marine was still moving {m["min_settled"]} frames before the end (need 30 settled)'
    return {'values': {
        'move.cruise_speed': m['cruise'], 'move.arrival_first': m['arrival_first'], 'move.arrival_last': m['arrival_last'],
        'move.group_spread': m['spread_end'], 'move.start_latency': m['latency']},
        'curves': {'move.remaining': m['remaining_curve']}, 'measured': {k: v for k, v in m.items() if k != 'remaining_curve'}}, None


# ---- Blood Bath worker income ---------------------------------------------

INCOME_FIELDS = ['banks.minerals_gathered', 'banks.minerals', 'unit.type', 'unit.owner', 'unit.sprite',
                 'unit.x', 'unit.y', 'clock.tick_counter', 'clock.pause_halfword']
INCOME_HORIZON = 2000   # source frames from the first in-mission frame
INCOME_WORKERS = 4
INCOME_CLEAR_RADIUS = 320  # N64 px: no foreign unit this close to the human's workers during the window


def gathered_curve(series, owner, t0, horizon):
    """Minerals gathered since t0, per source frame t0..t0+horizon (last value carried)."""
    by_sf = {}
    for frame in series['frames']:
        bank = frame['banks'].get(owner)
        if bank is None:
            continue
        minerals, gathered = bank
        by_sf[frame['sf']] = gathered if gathered is not None else minerals
    if t0 not in by_sf:
        earlier = [sf for sf in by_sf if sf <= t0]
        if not earlier:
            return None
        by_sf[t0] = by_sf[max(earlier)]
    base = by_sf[t0]
    curve, value = [], 0
    for sf in range(t0, t0 + horizon + 1):
        if sf in by_sf:
            value = by_sf[sf] - base
        curve.append(value)
    if max(by_sf) < t0 + horizon:
        return None
    return curve


def income_measure(curve, horizon):
    deltas = [b - a for a, b in zip(curve, curve[1:]) if b != a]
    first = next((k for k, v in enumerate(curve) if v > 0), None)
    # Two deliveries can land in one frame (or one GBA frame bucket), so the
    # yield is the greatest common divisor of the credits.
    positive = [d for d in deltas if d > 0]
    trip = functools.reduce(math.gcd, positive) if positive else 0
    return {'rate': curve[horizon] / horizon, 'total': curve[horizon], 'first_delivery': first,
            'trip_yield': trip, 'credits': len(positive),
            'whole_trips': all(d % trip == 0 for d in positive) if trip else False,
            'negative': sum(1 for d in deltas if d < 0)}


def income_observations(r, horizon):
    trip = TRIP_MINERALS * INCOME_WORKERS
    t = MOVE_TIME_TOLERANCE
    return [
        observation('income.rate', 'human workers x4', r['rate'], (r['total'] - trip) / horizon, (r['total'] + trip) / horizon,
                    f'+-one trip per worker ({trip} minerals) over {horizon} frames', True, INCOME_FIELDS,
                    unit='minerals gathered per source frame'),
        observation('income.first_delivery', 'human workers x4', r['first_delivery'], r['first_delivery'] - t,
                    r['first_delivery'] + t, f'+-{t} source frames', True, INCOME_FIELDS,
                    unit='source frames from the first in-mission frame to the first delivery'),
        observation('income.trip_yield', 'Mineral Field', r['trip_yield'], r['trip_yield'], r['trip_yield'], 'exact', True,
                    INCOME_FIELDS, unit='minerals per delivery'),
    ]


def income_measure_n64(cap, marks):
    """Both engines start a skirmish's workers mining by themselves, so the window
    opens at the first in-mission frame and no command is given."""
    series = cap.series(gating=True)
    first = series['frames'][0]['units']
    drones = {u[1] for u in first.values() if u[0] == DRONE}
    if len(drones) != 1:
        raise SystemExit(f'bb-income: Drones belong to owners {sorted(drones)}; expected one human (Zerg against a non-Zerg computer)')
    human = drones.pop()
    workers = [uid for uid, u in first.items() if u[0] == DRONE and u[1] == human]
    if len(workers) != INCOME_WORKERS:
        raise SystemExit(f'bb-income: the human has {len(workers)} Drones, not {INCOME_WORKERS}')
    t0 = series['frames'][0]['sf']
    curve = gathered_curve(series, human, t0, INCOME_HORIZON)
    if curve is None:
        raise SystemExit(f'bb-income: capture shorter than {INCOME_HORIZON} frames after the command')
    r = income_measure(curve, INCOME_HORIZON)
    # Cross-check of the two bank arrays: nothing is spent, so the spendable
    # bank must rise exactly as the gathered total does.
    bank0 = next(f['banks'][human][0] for f in series['frames'] if f['sf'] >= t0)
    bank_end = next(f['banks'][human][0] for f in series['frames'] if f['sf'] >= t0 + INCOME_HORIZON)
    if bank_end - bank0 != curve[INCOME_HORIZON] or r['negative'] or not r['whole_trips']:
        raise SystemExit(f'bb-income: bank rose {bank_end - bank0} but gathered {curve[INCOME_HORIZON]}')
    # The window must be undisturbed: no other player's unit near the human's workers.
    home = [(u[2], u[3]) for uid, u in first.items() if uid in workers]
    cx, cy = sum(p[0] for p in home) / len(home), sum(p[1] for p in home) / len(home)
    closest = min((math.hypot(u[2] - cx, u[3] - cy) for f in series['frames'] if t0 <= f['sf'] <= t0 + INCOME_HORIZON
                   for u in f['units'].values() if u[1] != human and u[0] not in MINERAL_TYPES and u[1] < 8),
                  default=float('inf'))
    if closest < INCOME_CLEAR_RADIUS:
        raise SystemExit(f'bb-income: another player came within {closest:.0f} px of the human workers')
    players = sorted({u[1] for u in first.values() if u[1] < 8})
    gba = {'scenario': 'income', 'profile': 'zerg-z10', 'at_step': 5,
           'steps': int((INCOME_HORIZON + 40) * 5 / 4) + 10, 'horizon': INCOME_HORIZON}
    notes = [f'human owner {human} (the only Drone owner) of players {players}; the gathered array includes the '
             f'starting 50, so the curve is its rise; bank rise equals gathered rise over the window '
             f'({curve[INCOME_HORIZON]} minerals); no other unit within {closest:.0f} px of the human workers']
    return {'observations': income_observations(r, INCOME_HORIZON), 'gba': gba, 'notes': notes,
            'curves': {'income.gathered': {'n64': curve, 'tolerance': TRIP_MINERALS * INCOME_WORKERS,
                                           'unit': 'minerals gathered since the command, per source frame'}},
            'measured': r}


def income_measure_gba(series, golden, report):
    horizon = golden['gba']['horizon']
    curve = gathered_curve(series, report['human_owner'], series['frames'][0]['sf'], horizon)
    if curve is None:
        return None, f'GBA run shorter than {horizon} frames after the command'
    r = income_measure(curve, horizon)
    if r['negative']:
        return None, 'the human bank fell: the passive human spent minerals'
    if r['credits'] and not r['whole_trips']:
        return None, f'a bank credit is not a whole number of {r["trip_yield"]}-mineral trips'
    return {'values': {'income.rate': r['rate'], 'income.first_delivery': r['first_delivery'],
                       'income.trip_yield': r['trip_yield']},
            'curves': {'income.gathered': curve}, 'measured': r}, None


# The GBA profile and runner scenario are fixed per scenario id here, never
# taken from a golden file.
SCENARIOS = {
    't1-move': {'title': 'T1 Wasteland: five Marines ordered to one point', 'measure_n64': move_measure_n64,
                'measure_gba': move_measure_gba, 'profile': 'terran', 'runner': 'move'},
    'bb-income': {'title': 'Blood Bath 1v1: passive human Zerg, four starting Drones mining', 'measure_n64': income_measure_n64,
                  'measure_gba': income_measure_gba, 'profile': 'zerg-z10', 'runner': 'income'},
}

# Field-map scenarios (orders, fog, animation; tools/sc64_parity_fieldmap.py) join the table.
from sc64_parity_fieldmap import FIELD_SCENARIOS  # noqa: E402
SCENARIOS.update(FIELD_SCENARIOS)

# ---- unit-class labs (WP8) -------------------------------------------------
#
# One human subject gets timed orders from a compiled tape (tools/
# sc64_tape_compile.py) on the N64 and the same orders, at the same source-
# frame offsets, through the injector on the GBA (tests/sc64_parity_run.c
# lab-t1 / lab-bb). Every number below is measured by the same function on
# both engines. Subjects and the human player are found by cartridge unit id,
# never by position, so no placement is written into tracked code.

# The labs read the subject's order byte and gate on the order values they accept: Move or
# AttackMove for a move lab, AttackMove alone for an attack lab.
LAB_FIELDS = MOVE_FIELDS + ['unit.order', 'order.6', 'order.14']
LAB_HP_FIELDS = MOVE_FIELDS + ['unit.order', 'order.14', 'unit.hp']
LAB_ACCEL_FRACTION = 0.9     # acceleration ends at the first chord this close to cruise
LAB_ACCEL_CHORD = 4          # shortest chord, source frames
LAB_ACCEL_GRID_STEPS = 2     # a chord spans at least this many GBA position steps at cruise:
LAB_GBA_GRID = 2.0           # N64 px per GBA pixel. A slow mover (an Overlord, 0.84 px a frame)
                             # moves one GBA pixel every 3-4 frames, so a 4-frame chord holds one
                             # step or two by phase alone and its reading swung 3 -> 7 -> 11 when
                             # the start moved a few frames. The N64 side picks the chord from its
                             # cruise and records it in the golden; the GBA side uses the same one.
LAB_RANGE_TOLERANCE = 8.0    # N64 px: one GBA pixel each way per axis, plus a movement step's overshoot
LAB_SETTLE = 20              # source frames a leg must stay still before its end
LAB_STILL = 8.0              # N64 px a non-subject human unit of the subject's type may drift
LAB_ORDER_WINDOW = 12        # ticks after a press within which the subject's order must change
ORDER_MOVE, ORDER_ATTACK_MOVE = 6, 14   # proven unit.order values (layout 'orders')
SC64_BURN_Q8 = 20   # Terran building burn per source frame, 1/256 hp (starcraft/sc64_building_damage.h)
LAB_REACH = 288.0            # N64 px: 9 tiles, past every weapon but siege mode, for single-attacker checks
LAB_ARRIVE_TOLERANCE = 32.0  # N64 px a leg may settle from its order point: one tile, for the GBA's
                             # 2-px grain and a destination another unit already holds


def lab_command_frames(series, marks, names):
    return [n64_command_frame(series, marks[name]) for name in names]


def lab_accel_chord(cruise):
    """Chord length (source frames) for the acceleration reading at this cruise speed."""
    if cruise <= 0:
        return LAB_ACCEL_CHORD
    chord = max(LAB_ACCEL_CHORD, math.ceil(LAB_ACCEL_GRID_STEPS * LAB_GBA_GRID / cruise))
    # Rounding can leave the product a hair short of two steps: lengthen until it is not.
    while chord > LAB_ACCEL_CHORD and lab_accel_floor(cruise, chord) < LAB_ACCEL_GRID_STEPS * LAB_GBA_GRID:
        chord += 1
    return chord


def lab_accel_floor(cruise, chord):
    """Whole GBA grid steps (N64 px) a constant-speed mover at this cruise covers in a chord."""
    return LAB_GBA_GRID * math.floor(cruise * chord / LAB_GBA_GRID)


def lab_leg(series, uid, t0, t1, chord=None, floor_px=None):
    """Movement of one leg: from its command frame t0 to the next command (or the end).
    chord: the acceleration chord in source frames (default: from this leg's cruise);
    floor_px: the reference's whole-step distance for a lengthened chord (default: this leg's)."""
    points = [p for p in track(series, uid, t0) if t1 is None or p[0] <= t1]
    m = movement(points)
    if m is None:
        return None
    if chord is None:
        chord = lab_accel_chord(m['cruise'])
    # Acceleration: first chord from the start reaching the cruise fraction. In a lengthened
    # (slow mover's) chord a constant-speed mover on the GBA grid covers at least its cruise
    # distance rounded down to whole grid steps, so that cutoff never asks for more than that;
    # the shortest chord keeps its plain cutoff and every faster lab reads as before.
    # The allowance comes from the reference (N64) cruise, so a slightly slower GBA cruise
    # estimate cannot lower the cutoff to one grid step.
    need = LAB_ACCEL_FRACTION * m['cruise'] * chord
    if floor_px is None:
        floor_px = lab_accel_floor(m['cruise'], chord)
    if chord > LAB_ACCEL_CHORD:
        need = min(need, floor_px)
    by_sf = {sf: (x, y) for sf, x, y in points}
    accel = None
    for sf in range(m['start'] - 1, m['arrive'] - chord + 1):
        a, b = by_sf.get(sf), by_sf.get(sf + chord)
        if a is None or b is None:
            continue
        if math.hypot(b[0] - a[0], b[1] - a[1]) >= need:
            accel = sf + chord - m['start']
            break
    m['accel'] = accel
    m['accel_chord'] = chord
    m['accel_floor'] = floor_px
    m['latency'] = m['start'] - t0
    # The subject must be observed alive to the end of the leg, or its settling is unknown.
    end = t1 if t1 is not None else series['frames'][-1]['sf']
    m['observed_to_end'] = points[-1][0] >= end - 1
    m['duration'] = m['arrive'] - m['start']
    m['settled_until'] = points[-1][0]
    return m


def lab_subject(series, t0, subject_type, human, kind='moved'):
    """The one human unit of subject_type that moved after t0; others of its type stay put."""
    first = next((f['units'] for f in series['frames'] if f['sf'] >= t0), None)
    if first is None:
        return None, 'no frame at the command'
    moved = {}
    for uid, unit in first.items():
        if unit[0] != subject_type or unit[1] != human:
            continue
        points = track(series, uid, t0)
        moved[uid] = max((math.hypot(p[1] - points[0][1], p[2] - points[0][2]) for p in points), default=0.0)
    movers = [uid for uid, d in moved.items() if d > LAB_STILL]
    if len(movers) != 1:
        return None, f'{len(movers)} human units of type {subject_type} moved after the command (need exactly one)'
    return movers[0], None


def lab_move_measure(series, uid, frames, targets, chord=None, floor_px=None):
    """targets: the point each leg's order was given at (N64 px). A leg counts only when it
    settles within LAB_ARRIVE_TOLERANCE of it, so a stop anywhere else never reads as arrival.
    chord, floor_px: the acceleration chord and whole-step floor for every leg (default: the
    first leg's, from its cruise)."""
    if len(targets) != len(frames):
        return None, 'one target per leg is needed'
    legs = []
    for k, t0 in enumerate(frames):
        t1 = frames[k + 1] if k + 1 < len(frames) else None
        leg = lab_leg(series, uid, t0, t1, chord, floor_px)
        if leg is not None and chord is None:
            chord, floor_px = leg['accel_chord'], leg['accel_floor']
        if leg is None:
            return None, f'leg {k}: the subject never moved after frame {t0}'
        if not leg['observed_to_end']:
            return None, f'leg {k}: the subject was not observed to the end of the leg'
        end = leg['settled_until']   # settling counts only through the last sample seen
        if end - leg['arrive'] < LAB_SETTLE:
            return None, f'leg {k}: the subject was still moving {end - leg["arrive"]} frames before the leg ended'
        if k == 0 and leg['cruise'] <= 0:
            return None, f'leg {k}: the subject never held a cruise speed'
        if leg['accel'] is None and k == 0:   # only the first leg's reading gates
            return None, f'leg {k}: the subject never reached {LAB_ACCEL_FRACTION:.0%} of its cruise speed'
        miss = math.hypot(leg['end'][0] - targets[k][0], leg['end'][1] - targets[k][1])
        if miss > LAB_ARRIVE_TOLERANCE:
            return None, (f'leg {k}: the subject settled at {tuple(leg["end"])}, {miss:.0f} px from its order point '
                          f'{tuple(targets[k])} (allowed {LAB_ARRIVE_TOLERANCE})')
        leg['target_miss'] = round(miss, 2)
        legs.append(leg)
    # The cruise band spans the legs that held a cruise: a later leg too short to (one that
    # settles after a few moving frames reads cruise 0 or never reaches the cruise fraction)
    # is left out, so it cannot drop the band's lower bound. Leg 0 always counts (gated above).
    band = [leg for leg in legs if leg['cruise'] > 0 and leg['accel'] is not None]
    result = {'legs': [{k: (list(v) if isinstance(v, tuple) else v) for k, v in leg.items()} for leg in legs],
              'cruise': legs[0]['cruise'], 'accel': legs[0]['accel'], 'duration': legs[0]['duration'],
              'latency': legs[0]['latency'],
              'cruise_min': min(leg['cruise'] for leg in band), 'cruise_max': max(leg['cruise'] for leg in band),
              'accel_chord': chord, 'accel_floor': floor_px}
    left_out = [k for k, leg in enumerate(legs) if not (leg['cruise'] > 0 and leg['accel'] is not None)]
    if left_out:
        result['cruise_left_out'] = left_out
    if len(legs) > 1:
        # The turn costs whatever the reversal adds to the start latency; input lag cancels.
        result['turn_cost'] = legs[1]['latency'] - legs[0]['latency']
        result['return_cruise'] = legs[1]['cruise']
    return result, None


def lab_move_observations(m, name):
    t = MOVE_TIME_TOLERANCE
    obs = [
        observation('lab.cruise_speed', name, m['cruise'], m['cruise_min'] * (1 - MOVE_SPEED_TOLERANCE),
                    m['cruise_max'] * (1 + MOVE_SPEED_TOLERANCE), f'N64 range over both legs widened by '
                    f'{MOVE_SPEED_TOLERANCE:.0%}' + (
                        f'; leg {", ".join(map(str, m["cruise_left_out"]))} left out: it never held a cruise speed'
                        if m.get('cruise_left_out') else ''), True, LAB_FIELDS, unit='N64 px per source frame'),
        observation('lab.accel_frames', name, m['accel'], m['accel'] - t, m['accel'] + t, f'+-{t} source frames', True,
                    LAB_FIELDS, unit=f'source frames from the first movement to a {m.get("accel_chord", LAB_ACCEL_CHORD)}'
                    f'-frame chord at {LAB_ACCEL_FRACTION:.0%} of cruise'),
        observation('lab.leg_duration', name, m['duration'], m['duration'] - t, m['duration'] + t, f'+-{t} source frames',
                    True, LAB_FIELDS, unit='source frames from the first movement to arrival, first leg'),
        observation('lab.start_latency', name, m['latency'], m['latency'] - t, m['latency'] + t,
                    'report only: the N64 latency includes the pad-to-game input lag', False, LAB_FIELDS,
                    unit='source frames from the command to the first movement'),
    ]
    if 'turn_cost' in m:
        obs.append(observation('lab.turn_cost', name, m['turn_cost'], m['turn_cost'] - t, m['turn_cost'] + t,
                               f'+-{t} source frames', True, LAB_FIELDS,
                               unit='start latency of the reversed second leg minus that of the first'))
    return obs


def lab_gba_steps(frames, at_step=5):
    """GBA command steps whose pre-command frames keep the N64 offsets between commands.

    One source frame in four spans two GBA steps, and neither of them owns it
    (gba_boundary_ok), so a later command may land one frame after its N64
    offset; the GBA side measures from the frames its runner reports."""
    steps = [at_step]
    base = (at_step - 1) * 4 // 5
    for t in frames[1:]:
        want = base + (t - frames[0])
        s = want * 5 // 4 + 1
        while (s - 1) * 4 // 5 < want or not gba_boundary_ok(s):
            s += 1
        while s <= steps[-1] or not gba_boundary_ok(s):
            s += 1   # two N64 orders a frame apart may round onto one GBA step
        if (s - 1) * 4 // 5 > want + 1:
            raise SystemExit(f'no GBA step owns source frame {want} or the next')
        steps.append(s)
    return steps


def lab_move_n64(spec):
    def measure(cap, marks):
        series = cap.series(gating=True)
        names = spec['marks']
        frames = lab_command_frames(series, marks, names)
        first = next(f['units'] for f in series['frames'] if f['sf'] >= frames[0])
        # The human is resolved at the lab's first frame, before any computer unit changes type.
        owners = {u[1] for u in series['frames'][0]['units'].values() if u[0] == spec['human_type']}
        if len(owners) != 1:
            raise SystemExit(f'{spec["id"]}: units of type {spec["human_type"]} belong to {sorted(owners)}; '
                             'expected one human owner')
        human = owners.pop()
        uid, err = lab_subject(series, frames[0], spec['subject_type'], human)
        if err:
            raise SystemExit(f'{spec["id"]}: {err}')
        # The GBA replays the order the tape gave: a move to the world point under the cursor.
        clicked = [cap.world_point(marks[name]) for name in names]
        # The order each press gave (C-Right over other units of the human can give an
        # attack-move): the GBA replays that order, a move or an attack-move.
        observed = []
        for name in names:
            order, seen = n64_order_after(cap, uid, marks[name], (ORDER_MOVE, ORDER_ATTACK_MOVE))
            if order is None:
                raise SystemExit(f'{spec["id"]}: after mark {name} the subject\'s orders were {seen}, '
                                 'neither Move nor AttackMove')
            observed.append(order)
        m, err = lab_move_measure(series, uid, frames, clicked)
        if err:
            raise SystemExit(f'{spec["id"]}: {err}')
        start = first[uid][2:4]
        steps = lab_gba_steps(frames)
        cmds = []
        for (px, py), step, order in zip(clicked, steps, observed):
            kind = 1 if order == ORDER_ATTACK_MOVE else 0
            if spec.get('relative'):
                cmds.append([step, kind | 16, round((px - start[0]) / 2), round((py - start[1]) / 2)])
            else:
                cmds.append([step, kind, px // 2, py // 2])
        gba = {'scenario': spec['runner'], 'profile': spec['profile'], 'at_step': steps[0], 'cmds': cmds,
               'accel_chord': m['accel_chord'], 'accel_floor': m['accel_floor'],
               'steps': steps[-1] + int((m['legs'][-1]['latency'] + m['legs'][-1]['duration'] + 60) * 5 / 4) + 10}
        if spec.get('relative'):
            gba['subject_raw'] = spec['subject_type']
        else:
            gba['subject'] = [start[0] // 2, start[1] // 2]
        if 'mission' in spec:
            gba['mission'] = spec['mission']
        notes = [f'human owner {human}; subject row {uid}, type {spec["subject_type"]}; N64 orders '
                 + ', '.join('AttackMove' if o == ORDER_ATTACK_MOVE else 'Move' for o in observed) + ' at '
                 + ', '.join(f'({x},{y})' for x, y in clicked) + '; legs end at '
                 + ', '.join(f'({leg["end"][0]},{leg["end"][1]})' for leg in m['legs'])
                 + ('; GBA orders are offsets from the subject (start positions are shuffled)' if spec.get('relative') else '')]
        return {'observations': lab_move_observations(m, spec['name']), 'gba': gba, 'notes': notes, 'measured': m}
    return measure


def n64_order_after(cap, row, step, expect, window=LAB_ORDER_WINDOW):
    """Whether the N64 subject row's order byte (+0x4D, proven) takes one of `expect` within
    `window` ticks after frontend step `step`, and the orders seen: the GBA replays the order
    the cartridge actually gave. Returns (the first expected order seen or None, orders seen)."""
    expect = (expect,) if isinstance(expect, int) else tuple(expect)
    # The order before the press, then the first different order within `window` N64 ticks of it:
    # a byte already set before the press is no evidence of the press.
    # The boundary is the last follow-up read taken at or before the press step, as for every
    # lab time. A follow-up read one step after the press ran with the button held, so it is
    # already after the press, even when its tick began at the press step.
    before = [r for r in cap.ticks if r['step'] <= step and row in r['rows']]
    if not before:
        return None, []
    prior, tick0 = cap.unit(before[-1]['rows'][row], 'order'), before[-1]['tick']
    identity = (cap.unit(before[-1]['rows'][row], 'type'), cap.unit(before[-1]['rows'][row], 'owner'))
    # Every tick from the boundary to the first changed order must have been read: a tick that
    # began and ended inside one step has no follow-up, and it could have held the press's first
    # order. The latest change read by frontend step, not by tick value (the counter can wrap or
    # restart): every change record in capture order.
    reads = getattr(cap, 'change_reads', None)
    if reads is None:
        reads = sorted((at, t) for t, at in getattr(cap, 'change_steps', {}).items())
    def paired(record):
        # The change read this follow-up belongs to: the latest one of its tick before it.
        own = [(at, t) for at, t in reads if t == record['tick'] and at < record['step']]
        return max(own) if own else None
    start = paired(before[-1])
    if reads and start is None:
        return None, ['the order at the press was not read']
    after = reads[reads.index(start) + 1:] if reads else []
    seen = []
    followed = set()
    for record in cap.ticks:
        if record['step'] <= step:
            continue
        if (record['tick'] - tick0) & 0xFFFFFFFF > window:
            break
        followed.add(paired(record))
        # The subject must stay the same unit until its order changes: a row that vanished or
        # went on as another unit could show a replacement's order.
        if row not in record['rows'] or \
                (cap.unit(record['rows'][row], 'type'), cap.unit(record['rows'][row], 'owner')) != identity:
            return None, seen + ['the subject row was gone or replaced before its order changed']
        order = cap.unit(record['rows'][row], 'order')
        seen.append(order)
        if order != prior:
            unread = [at for at, t in after if at < record['step'] and (at, t) not in followed]
            if unread:
                return None, seen + ['the order at the press was not read' if min(unread) <= step
                                     else 'a tick between the press and this read was not read']
            return (order if order in expect else None), seen
    return None, seen


def lab_report_frames(report):
    return [entry[1] for entry in report.get('lab', [])]


def lab_move_gba(spec):
    def measure(series, golden, report):
        uid = report.get('subject')
        frames = lab_report_frames(report)
        if uid is None or len(frames) != len(golden['gba']['cmds']):
            return None, 'the runner reported no subject or a different number of orders'
        # The points the runner issued the orders at (after clamping to the map), in N64 px.
        issued = report.get('lab', [])
        if any(len(entry) < 6 for entry in issued):
            return None, 'the runner did not report where it issued its orders'
        targets = [(2 * entry[4], 2 * entry[5]) for entry in issued]
        # The N64 side's chord. Goldens from decoder revision 1 predate it and used the
        # shortest; a later golden must carry a valid one.
        params = golden['gba']
        revision = golden.get('decoder_revision', 1)
        if type(revision) is not int or revision < 1:
            return None, f'the golden has a malformed decoder revision ({revision!r})'
        if revision == 1 and 'accel_chord' not in params and 'accel_floor' not in params:
            chord, floor_px = LAB_ACCEL_CHORD, 0.0   # the shortest chord never uses the floor
        else:
            chord, floor_px = params.get('accel_chord'), params.get('accel_floor')
        if type(chord) is not int or not LAB_ACCEL_CHORD <= chord <= 10000:
            return None, f'the golden has no valid acceleration chord ({chord!r})'
        # A lengthened chord's floor is whole grid steps covering at least two of them, as
        # lab_accel_chord guarantees; anything less would let one step pass for cruise.
        floor_ok = (type(floor_px) in (int, float) and -1 < floor_px < 1e6 and
                    math.isfinite(floor_px) and floor_px >= 0)
        if floor_ok and chord > LAB_ACCEL_CHORD:
            steps = floor_px / LAB_GBA_GRID
            floor_ok = steps == int(steps) and steps >= LAB_ACCEL_GRID_STEPS
        if not floor_ok:
            return None, f'the golden has no valid acceleration floor ({floor_px!r})'
        m, err = lab_move_measure(series, uid, frames, targets, chord, float(floor_px))
        if err:
            return None, err
        values = {'lab.cruise_speed': m['cruise'], 'lab.accel_frames': m['accel'], 'lab.leg_duration': m['duration'],
                  'lab.start_latency': m['latency']}
        if 'turn_cost' in m:
            values['lab.turn_cost'] = m['turn_cost']
        return {'values': values, 'measured': m}, None
    return measure


def hp_track(series, uid, since, until=None, key='hp'):
    """[(sf, hp)] for one unit (key='shield': its shields); a frame without the row ends it."""
    out = []
    for frame in series['frames']:
        if frame['sf'] < since or (until is not None and frame['sf'] > until):
            continue
        hp = frame.get(key, {}).get(uid)
        if hp is None:
            if out:
                break
            continue
        out.append((frame['sf'], hp))
    return out


def hits_left(series, uid, since, until):
    """[(sf, shield points left, frames since the previous sample)] after each shield drop (see hits)."""
    track = hp_track(series, uid, since, until, key='shield')
    return [(b[0], b[1], b[0] - a[0]) for a, b in zip(track, track[1:]) if a[1] - b[1] >= 0.5]


def drop_gaps(points):
    """{sf: frames since the previous sample} for each sample of a track (see hits)."""
    return {b[0]: b[0] - a[0] for a, b in zip(points, points[1:])}


def hits(points, lethal=False):
    """[(sf, damage)] where hit points fall by at least half a point (Zerg regeneration is smaller).

    A drop that ends at 0 or below is left out on both engines unless lethal=True: it shows only
    what was left (the N64 row keeps 0), and the GBA removes the row in that step, so it leaves no
    record there. Shield tracks pass lethal=True: shields at 0 leave the unit alive."""
    return [(b[0], a[1] - b[1]) for a, b in zip(points, points[1:])
            if a[1] - b[1] >= 0.5 and (lethal or b[1] > 0)]


def cartridge_type(frame, vid, unit_raw=None):
    """A unit's cartridge id: the row's own on the N64 and on GBA traces that carry it; else the
    engine type through the runner's unit_raw map (zerg-z10 rows carry no raw ids)."""
    unit = frame['units'].get(vid)
    if unit is None:
        return None
    if unit[0] >= 0 or unit_raw is None:
        return unit[0]
    engine = frame.get('etype', {}).get(vid)
    return unit_raw[engine] if engine is not None and 0 <= engine < len(unit_raw) else None


def unit_lifetimes(series, t0, keep):
    """The series from frame t0 with each row split into lifetimes: a row that reappears after
    a gap, changes type or owner (an Egg hatching, a slot reused by a new unit), or is reborn
    in a GBA birth record, continues under a new key, so no drop, type or track spans two units.
    Keys are strings ("ROW", then "ROW.N"), except the subject's first lifetime, which keeps
    `keep`. series['lifetime_end'] says why a lifetime ended early: 'changed' when the row went
    on in place as another unit (a hatch or an owner change: not a death), 'reborn' when a GBA
    birth record reused the slot (the old unit was removed: a death).

    On the GBA a unit can die and its slot be reused inside one engine step, which the trace
    shows only as a type change, so there every in-place change of another row is 'reborn'.
    The N64 cannot: a dying row keeps 0 hit points for frames before its slot is freed."""
    births = {}
    for ev in series.get('events', []):
        if ev[0] == '+U':   # GBA: a unit born in this slot at this step
            births.setdefault((ev[1] or 0) * 4 // 5, set()).add(ev[2])
    frames, life, last, ended = [], {}, {}, {}
    previous = set()
    gba = series.get('source') == 'gba'
    name = lambda vid, n: (keep if n == 0 else f'{vid}.{n}') if vid == keep else (str(vid) if n == 0 else f'{vid}.{n}')
    for frame in series['frames']:
        if frame['sf'] < t0:
            continue
        etype = frame.get('etype', {})
        names = {}
        for vid, unit in frame['units'].items():
            ident = (unit[0], unit[1], etype.get(vid))
            if vid in last:
                reborn = vid in previous and (vid in births.get(frame['sf'], ()) or
                                              (gba and vid != keep and last[vid] != ident))
                changed = vid in previous and last[vid] != ident and not reborn
                if changed or reborn or vid not in previous:
                    if changed or reborn:
                        ended[name(vid, life.get(vid, 0))] = 'changed' if changed else 'reborn'
                    life[vid] = life.get(vid, 0) + 1
            last[vid] = ident
            names[vid] = name(vid, life.get(vid, 0))
        previous = set(frame['units'])
        out = dict(frame)
        for key in ('units', 'hp', 'shield', 'etype'):
            if key in frame:
                out[key] = {names[k]: v for k, v in frame[key].items() if k in names}
        frames.append(out)
    return dict(series, frames=frames, lifetime_end=ended)


def lab_attack_measure(series, uid, t0, foe_type=None, unit_raw=None):
    """The subject's hits on enemy units and theirs on it, from the command to its death or the end.

    With foe_type, only enemy units of that cartridge id are victims: hits on any other row,
    buildings included (the N64 table holds them, the GBA unit series does not), never count."""
    removed = [ev for ev in series.get('events', []) if ev[0] == '-B']
    series = unit_lifetimes(series, t0, uid)
    first = next((f for f in series['frames'] if f['sf'] >= t0), None)
    if first is None or uid not in first['units']:
        return None, 'the subject is not in the trace after the command'
    human = first['units'][uid][1]
    own = hp_track(series, uid, t0)
    if not own:
        return None, 'the subject has no hit points after the command'
    death = None
    last_sf = series['frames'][-1]['sf']
    # The subject's row going on in place as another unit (an owner change) is no death: the
    # fight is measured up to it and survival is left unmeasured.
    subject_changed = series['lifetime_end'].get(uid) == 'changed' and own[-1][1] > 0
    # A GBA row vanishes on its killing hit, so a missing row is a death, unless the trace's
    # removal record shows the subject left with hit points (loaded, or removed by the mission):
    # then it is lost, and survival is left unmeasured.
    gone = next((r for r in series.get('removals', []) if r[0] == 'U' and r[2] == uid and
                 (r[1] or 0) * 4 // 5 >= own[-1][0]), None) if own[-1][1] > 0 else None
    subject_lost = not subject_changed and own[-1][0] < last_sf and gone is not None and gone[6] > 0
    if not subject_changed and not subject_lost and (own[-1][0] < last_sf or own[-1][1] <= 0):
        death = next((sf for sf, hp in own if hp <= 0), own[-1][0] + 1)
    end = death if death is not None else (own[-1][0] if subject_changed or subject_lost else last_sf)
    window = [f for f in series['frames'] if t0 <= f['sf'] <= end]
    # Every row seen during the fight, with the first frame it was seen on: a unit born after
    # the command (a spawned enemy) is a victim, a barrier and a rival like any other.
    seen = {}
    for frame in window:
        for vid in frame['units']:
            seen.setdefault(vid, frame)

    def enemy(vid):
        unit = seen[vid]['units'][vid]
        return (unit[1] != human and unit[1] < 8 and
                (foe_type is None or cartridge_type(seen[vid], vid, unit_raw) == foe_type))
    victims = {}
    for vid in seen:
        if not enemy(vid):
            continue
        got = hits(hp_track(series, vid, t0, end))
        if got:
            victims[vid] = got
    # Shields take hits before hit points; a victim's shield drops are hits of their own.
    shielded = {}
    for vid in seen:
        if not enemy(vid):
            continue
        got = hits(hp_track(series, vid, t0, end, key='shield'), lethal=True)
        if got:
            shielded[vid] = got
            victims.setdefault(vid, [])
    # Exact shield damage only from drops that leave more shields than could have regenerated
    # since the last sample: a drop to 0 shows only what was left. Clipped drops still time hits.
    clipped = {(vid, sf) for vid in shielded for sf, left, frames in hits_left(series, vid, t0, end)
               if left <= SHIELD_REGEN * frames}
    # A drop counts for the subject only when the subject is the nearest human unit to the victim
    # at that frame; other drops (another human unit was closer) are listed as ambiguous.
    at_frame = {f['sf']: f for f in series['frames']}
    gaps_of = {}
    for vid in set(victims) | set(shielded):
        gaps_of[vid] = drop_gaps(hp_track(series, vid, t0, end))
        gaps_of[vid].update({sf: max(g, gaps_of[vid].get(sf, 1))
                             for sf, g in drop_gaps(hp_track(series, vid, t0, end, key='shield')).items()})

    def sole_attacker(sf, vid):
        frame = at_frame.get(sf, {})
        units, hp = frame.get('units', {}), frame.get('hp', {})
        v = units.get(vid)
        if v is None or uid not in units:
            return False
        # A verified single attacker: the subject is alive and the only living human row within
        # LAB_REACH of the victim. Any other human unit in reach, a GBA building of the human
        # included (the N64 table holds buildings as rows already), leaves the hit ambiguous.
        near = [k for k, u in units.items()
                if u[1] == human and hp.get(k, 1) > 0 and math.hypot(u[2] - v[2], u[3] - v[3]) <= LAB_REACH]
        near += [('B', k) for k, b in frame.get('buildings', {}).items()
                 if b[0] == human and math.hypot(b[1] - v[2], b[2] - v[3]) <= LAB_REACH]
        return near == [uid]

    def ours(sf, vid):
        # On the sample that shows the drop and on the one before it: a second attacker that
        # fired and then died or left inside the interval is still in reach on one of them.
        # A drop seen across a sampling gap may have come from a unit that left reach meanwhile.
        return (sole_attacker(sf, vid) and sole_attacker(sf - 1, vid)
                and gaps_of.get(vid, {}).get(sf, 1) == 1)
    unclear = {sf for vid, got in list(victims.items()) + list(shielded.items()) for sf, _ in got if not ours(sf, vid)}
    barriers = set()
    # A killing drop leaves no exact damage, but it still splits the timing: no cooldown
    # interval may span it. Kills of enemies of any type split it (the subject may have shot a
    # unit outside the configured foe between two hits), and so do their other drops while
    # they are within reach of the subject.
    def any_enemy(vid):
        unit = seen[vid]['units'][vid]
        return unit[1] != human and unit[1] < 8

    def near_subject(sf, vid):
        units = at_frame.get(sf, {}).get('units', {})
        me, v = units.get(uid), units.get(vid)
        return me is not None and v is not None and math.hypot(v[2] - me[2], v[3] - me[3]) <= LAB_REACH
    # Damage the subject may have dealt off the foe: drops and kills of other enemies in reach of
    # it, and of GBA enemy buildings (their hit points are in the trace's building rows). They
    # split the timing, and they are evidence of a fight, so a check they leave without a value
    # is unmeasured, not absent.
    nearby = set()
    for vid in seen:
        if not any_enemy(vid) or enemy(vid):
            continue
        for key in ('hp', 'shield'):
            track_ = hp_track(series, vid, t0, end, key=key)
            nearby.update(sf for sf, _ in hits(track_, lethal=True) if near_subject(sf, vid) or near_subject(sf - 1, vid))

    def point_near(sf, x, y):
        # The subject in reach of a point on the sample at sf or the one before it.
        for s_ in (sf, sf - 1):
            me = at_frame.get(s_, {}).get('units', {}).get(uid)
            if me is not None and math.hypot(x - me[2], y - me[3]) <= LAB_REACH:
                return True
        return False

    def burning(hp, max_hp):
        # A Terran building at or below a third of its hit points burns 20/256 hp a source frame
        # (starcraft/sc64_building_damage.h): its hit-point loss there is no evidence of an attack.
        return max_hp is not None and max_hp > 0 and hp * 100 < max_hp * 34

    def burn_loss(frames):
        return (SC64_BURN_Q8 * max(1, frames) + 255) // 256
    # GBA enemy buildings in reach that lose shields, or more hit points than burning explains.
    # A building slot removed or reborn between two samples holds two buildings: never compare them.
    turnover = {(r[2], (r[1] or 0) * 4 // 5) for r in series.get('removals', []) if r[0] == 'B'}
    turnover |= {(ev[2], (ev[1] or 0) * 4 // 5) for ev in series.get('events', []) if ev[0] in ('+B', '-B')}
    for before_, after_ in zip(window, window[1:]):
        for k, b in before_.get('buildings', {}).items():
            a = after_.get('buildings', {}).get(k)
            if b[0] == human or b[0] >= 8 or a is None or len(b) < 5 or len(a) < 5 or a[0] != b[0]:
                continue
            if any(slot == k and before_['sf'] < sf <= after_['sf'] for slot, sf in turnover):
                continue
            drop, max_hp = b[3] - a[3], (b[5] if len(b) > 5 else None)
            hit = b[4] - a[4] >= 0.5 or (drop >= 0.5 and not (burning(b[3], max_hp) and
                                                               drop <= burn_loss(after_['sf'] - before_['sf'])))
            if hit and point_near(after_['sf'], b[1], b[2]):
                nearby.add(after_['sf'])
    # GBA removals (a unit or building born and removed inside one source frame shows only here):
    # an enemy killed in reach of the subject, unless a burning building could have burned out.
    removed_foes = set()
    for record in series.get('removals', []):
        kind, step, slot, owner, x, y, hp, max_hp, last_hp, *rest = record
        raw, etype_, last_step = (list(rest) + [None] * 3)[:3]
        sf = (step or 0) * 4 // 5
        if not t0 <= sf <= end or owner == human or owner >= 8 or hp > 0:
            continue
        barriers.add(sf)
        if kind == 'U' and foe_type is not None:
            # The foe's cartridge id from the row, or from the runner's engine-type map.
            cart = raw if raw is not None and raw >= 0 else (
                unit_raw[etype_] if unit_raw and etype_ is not None and 0 <= etype_ < len(unit_raw) else None)
            if cart == foe_type:
                removed_foes.add(sf)   # a foe killed, even one born and killed inside one source frame
                continue
        # A burning Terran building whose last hit points its burn could have taken since its
        # last row may have burned out; any other building death is a kill.
        if kind == 'B' and last_hp is not None and last_step is not None and burning(last_hp, max_hp) and \
                last_hp <= burn_loss(((step or 0) - last_step) * 4 // 5):
            continue
        if point_near(sf, x, y):
            nearby.add(sf)
    unclear |= nearby
    foe_kills = set()
    for vid in seen:
        if not any_enemy(vid):
            continue
        track_ = hp_track(series, vid, t0, end)
        events = [b[0] for a, b in zip(track_, track_[1:]) if a[1] > 0 >= b[1]]   # killing drops only
        if track_ and track_[-1][0] < end and track_[-1][1] > 0 and \
                series['lifetime_end'].get(vid) != 'changed':   # a hatch or a morph is no kill
            events.append(track_[-1][0] + 1)   # a GBA row removed by a hit the trace never shows
        # Every kill splits the timing, the subject's own included: the killing hit is never in
        # the hit list (see hits), so an interval across it, or touching it, would skip a shot.
        barriers.update(events)
        if enemy(vid):
            foe_kills.update(events)
    foe_kills |= removed_foes
    # GBA buildings carry no hit points in the series: an enemy building that disappears
    # during the fight was destroyed, and splits the timing too.
    for before_, after_ in zip(window, window[1:]):
        for k, b in before_.get('buildings', {}).items():
            if b[0] != human and b[0] < 8 and k not in after_.get('buildings', {}):
                barriers.add(after_['sf'])
    # A building destroyed and replaced in its slot inside one source frame shows only in the
    # trace's removal events (GBA steps, 5 to 4 source frames).
    barriers.update((step or 0) * 4 // 5 for _, step, *_ in removed if t0 <= (step or 0) * 4 // 5 <= end)
    ambiguous = len({(sf, vid) for vid, got in list(victims.items()) + list(shielded.items())
                     for sf, _ in got if not ours(sf, vid)})
    victims_all, shielded_all = victims, shielded
    victims = {vid: [(sf, d) for sf, d in got if ours(sf, vid)] for vid, got in victims.items()}
    shielded = {vid: [(sf, d) for sf, d in got if ours(sf, vid)] for vid, got in shielded.items()}
    shield_hits = sorted((sf, vid, round(d)) for vid, got in shielded.items() for sf, d in got)
    shield_exact = [d for vid, got in shielded.items() for sf, d in got if (vid, sf) not in clipped]
    hp_frames = {(sf, vid) for vid, got in victims.items() for sf, _ in got}
    landed = sorted([(sf, vid, round(d)) for vid, got in victims.items() for sf, d in got] +
                    [(sf, vid, 0) for sf, vid, _ in shield_hits if (sf, vid) not in hp_frames])
    taken = [(sf, round(d)) for sf, d in hits(own)]

    def single_foe(sf):
        # The same single living enemy in reach on the sample before the drop and on the one
        # that shows it, and a unit of the configured foe type on both: a lone building, another
        # enemy type, or a row that changed type in between never stands for the foe's damage.
        one = sole_foe(sf)
        if one is None or one != sole_foe(sf - 1) or isinstance(one, tuple):
            return False
        return foe_type is None or all(cartridge_type(at_frame[s], one, unit_raw) == foe_type
                                       for s in (sf - 1, sf) if s in at_frame)

    def sole_foe(sf):
        frame = at_frame.get(sf, {})
        units, hp = frame.get('units', {}), frame.get('hp', {})
        me = units.get(uid)
        if me is None:
            return None
        near = [k for k, u in units.items() if u[1] != human and u[1] < 8 and hp.get(k, 1) > 0
                and math.hypot(u[2] - me[2], u[3] - me[3]) <= LAB_REACH]
        # GBA buildings live outside the unit rows (the N64 table holds them as rows already).
        near += [('B', k) for k, b in frame.get('buildings', {}).items()
                 if b[0] != human and b[0] < 8 and math.hypot(b[1] - me[2], b[2] - me[3]) <= LAB_REACH]
        return near[0] if len(near) == 1 else None
    own_gaps = drop_gaps(own)
    # Each hit taken must come between consecutive samples with one enemy in reach.
    taken_single = all(single_foe(sf) and own_gaps.get(sf, 1) == 1 for sf, _ in taken)
    # Survival runs from the first hit taken, the killing one included (a one-shot death is 0).
    first_taken = next((sf for sf, _ in hits(own, lethal=True)), None)
    # What the subject took is measured even when it never lands a hit.
    rng = sf0 = None
    # The first hit's range and time stand only when no earlier foe drop or kill could have
    # been the subject's first shot.
    foe_unclear = {sf for vid, got in list(victims_all.items()) + list(shielded_all.items())
                   for sf, _ in got if not ours(sf, vid)}
    earlier = min(foe_unclear | foe_kills, default=None)
    if landed and (earlier is None or earlier > landed[0][0]):
        sf0, v0, _ = landed[0]
        at = {f['sf']: f['units'] for f in series['frames']}
        su, vu = at[sf0].get(uid), at[sf0].get(v0)
        rng = math.hypot(su[2] - vu[2], su[3] - vu[3]) if su and vu else None
    # Cooldown intervals never span an ambiguous hit: it may have been the subject's.
    gaps = [b[0] - a[0] for a, b in zip(landed, landed[1:])
            if not any(a[0] < sf < b[0] for sf in unclear) and not any(a[0] <= sf <= b[0] for sf in barriers)]
    hp_hits = [d for _, _, d in landed if d]
    # The subject was hit: hit points, or shields (a Protoss subject's first hits).
    struck = bool(hits(own, lethal=True) or hits(hp_track(series, uid, t0, end, key='shield'), lethal=True))
    result = {
        'hits': len(landed), 'ambiguous_hits': ambiguous, 'damage_values': sorted(set(hp_hits)),
        'damage': statistics.median(hp_hits) if hp_hits else None,
        'shield_hits': len(shield_hits), 'shield_values': sorted({round(d) for d in shield_exact}),
        'shield_damage': statistics.median(round(d) for d in shield_exact) if shield_exact else None,
        'cooldown': statistics.median(gaps) if gaps else None, 'gaps': gaps,
        'range_first': rng, 'first_hit': sf0 - t0 if sf0 is not None else None,
        # The first credited hit, resolved or not (sizes the GBA replay).
        'first_landed': landed[0][0] - t0 if landed else None,
        'taken': len(taken), 'counter_values': sorted({d for _, d in taken}), 'taken_single': taken_single,
        'counter_damage': statistics.median(d for _, d in taken) if taken else None,
        # A GBA row goes in the step of the killing hit, so a one-shot death shows no hit at all:
        # its first hit taken is the death itself, as on the N64.
        'survival': (death - (first_taken if first_taken is not None else death)) if death is not None else None,
        'death': (death - t0) if death is not None else None,
        # Evidence that a fight happened: any drop or kill of the foe, any drop or kill of another
        # enemy or enemy building in reach of the subject, or any hit on the subject (its death
        # included). With it, a check left without a value is inconclusive, never absent.
        'combat': bool(victims_all or shielded_all or foe_kills or nearby or struck or death is not None),
        # The same without the drops near the subject: a fight with the foe, or one the subject took.
        'foe_combat': bool(victims_all or shielded_all or foe_kills or struck or death is not None),
        'subject_changed': subject_changed, 'subject_lost': subject_lost,
        # A dying N64 row outlives its hit points (16 frames for a Marine); a GBA row goes in
        # the step of the killing hit, so that hit leaves no hit-point record there. A victim
        # counts as killed when its points reach 0 or its row is gone before the end.
        'victims': {str(v): {'type': seen[v]['units'][v][0], 'hits': len(g),
                             'killed': (lambda tr: not tr or tr[-1][1] <= 0 or tr[-1][0] < end)(
                                 hp_track(series, v, t0, end))}
                    for v, g in victims.items()},
    }
    return result, None


def lab_attack_observations(m, name, foe):
    obs = []
    if m['damage'] is not None:
        obs.append(observation('lab.damage', f'{name} on {foe}', m['damage'], m['damage'], m['damage'], 'exact', True,
                               LAB_HP_FIELDS, unit='hit points per hit (median; every hit listed in the golden)'))
    if m.get('shield_damage') is not None:
        obs.append(observation('lab.shield_damage', f'{name} on {foe}', m['shield_damage'], m['shield_damage'],
                               m['shield_damage'], 'exact', True, LAB_HP_FIELDS + ['unit.shields'],
                               unit='shield points per hit while shields last (median)'))
    # Withheld when an earlier foe drop or kill was unresolved (it may have been the first shot).
    if m['range_first'] is not None:
        obs.append(observation('lab.range_first', f'{name} on {foe}', m['range_first'],
                               m['range_first'] - LAB_RANGE_TOLERANCE, m['range_first'] + LAB_RANGE_TOLERANCE,
                               f'+-{LAB_RANGE_TOLERANCE} N64 px', True, LAB_HP_FIELDS,
                               unit='centre distance from the subject to its target at the first hit, N64 px'))
    if m['first_hit'] is not None:
        obs.append(observation('lab.first_hit', f'{name} on {foe}', m['first_hit'], m['first_hit'] - MOVE_TIME_TOLERANCE,
                               m['first_hit'] + MOVE_TIME_TOLERANCE, 'report only: includes the walk and the N64 input lag',
                               False, LAB_HP_FIELDS, unit='source frames from the command to the first hit'))
    if m['cooldown'] is not None:
        obs.append(observation('lab.cooldown', name, m['cooldown'], m['cooldown'] - 1, m['cooldown'] + 1,
                               '+-1 source frame', True, LAB_HP_FIELDS,
                               unit='source frames between the subject\'s hits (median)'))
    if m['counter_damage'] is not None:
        # Hits taken carry no source on the N64: they gate only when every one is the same size.
        single = len(m['counter_values']) == 1 and m.get('taken_single', False)
        obs.append(observation('lab.counter_damage', f'{foe} on {name}', m['counter_damage'], m['counter_damage'],
                               m['counter_damage'], 'exact' if single else
                               f'report only: hits of sizes {m["counter_values"]}, not all from one enemy in reach',
                               single, LAB_HP_FIELDS, unit='hit points per hit taken (median)'))
    if m['survival'] is not None:
        obs.append(observation('lab.survival', name, m['survival'], m['survival'] - 2 * MOVE_TIME_TOLERANCE,
                               m['survival'] + 2 * MOVE_TIME_TOLERANCE, f'+-{2 * MOVE_TIME_TOLERANCE} source frames',
                               True, LAB_HP_FIELDS,
                               unit='source frames from the first hit the subject takes to its death'))
    return obs


def lab_attack_n64(spec):
    def measure(cap, marks):
        series = cap.series(gating=True, hp=True, shields=spec.get('shields', False))
        t0, = lab_command_frames(series, marks, ['command'])
        first = next(f['units'] for f in series['frames'] if f['sf'] >= t0)
        owners = {u[1] for u in series['frames'][0]['units'].values() if u[0] == spec['human_type']}
        if len(owners) != 1:
            raise SystemExit(f'{spec["id"]}: units of type {spec["human_type"]} belong to {sorted(owners)}')
        human = owners.pop()
        uid, err = lab_subject(series, t0, spec['subject_type'], human)
        if err:
            raise SystemExit(f'{spec["id"]}: {err}')
        m, err = lab_attack_measure(series, uid, t0, spec['foe_type'])
        if err or not m['hits']:
            raise SystemExit(f'{spec["id"]}: {err or "the subject hit nothing"}')
        if not any(obs['gating'] for obs in lab_attack_observations(m, spec['name'], spec['foe'])):
            raise SystemExit(f'{spec["id"]}: no gating check: the fight gave no attributable value')
        if len(m['damage_values']) > 1 or len(m['shield_values']) > 1:
            raise SystemExit(f'{spec["id"]}: the subject\'s hits differ ({m["damage_values"]}, shields '
                             f'{m["shield_values"]}); one weapon expected')
        px, py = cap.world_point(marks['command'])
        # The GBA replays an attack-move: refuse a capture where the press gave another order
        # (a direct attack on a visible unit, for one).
        order, seen = n64_order_after(cap, uid, marks['command'], ORDER_ATTACK_MOVE)
        if order is None:
            raise SystemExit(f'{spec["id"]}: after the command the subject\'s orders were {seen}, never AttackMove')
        start = first[uid]
        step = lab_gba_steps([t0])[0]
        gba = {'scenario': spec['runner'], 'profile': spec['profile'], 'at_step': step,
               'subject': [start[2] // 2, start[3] // 2], 'cmds': [[step, 1, px // 2, py // 2]],
               # Long enough for a slower GBA approach: 400 frames past the later of the first hit and the death.
               'steps': step + int((max(m['death'] or 0, m['first_landed']) + 400) * 5 / 4) + 10}
        if 'mission' in spec:
            gba['mission'] = spec['mission']
        notes = [f'human owner {human}; subject row {uid}; attack-move at ({px},{py}); victims ' +
                 ', '.join(f'row {k} type {x["type"]} hits {x["hits"]} killed {x["killed"]}' for k, x in m['victims'].items()),
                 f'hits {m["hits"]} gaps {m["gaps"]}; hits taken {m["taken"]} {m["counter_values"]}']
        return {'observations': lab_attack_observations(m, spec['name'], spec['foe']), 'gba': gba, 'notes': notes,
                'measured': m}
    return measure


# GBA value of a check whose event never happened in the GBA run (listed under 'absent'). It is
# far outside every band, and tools/sc64_parity.py never lets a measured value inherit a KNOWN
# verdict from it (nor an absence inherit one from a measured baseline).
ABSENT = -999999
# GBA value of an attack-lab check when the subject fought only enemies other than the foe (drops
# or kills in its reach, GBA enemy buildings included) and the foe and the subject were never hit.
# Like ABSENT it is a marker, never a measurement: it inherits KNOWN only from the same marker in
# the baseline, so a run that fought where the baseline recorded no fight (or the reverse) fails.
OFF_TARGET = -999998
MARKERS = (ABSENT, OFF_TARGET)


def absent_values(golden, values):
    """Every golden check the GBA run gave no value for reads ABSENT, so the gap is a
    number the baseline can hold (and the audit lists) rather than a run error."""
    missing = sorted({obs['check'] for obs in golden['observations']} - set(values))
    values.update({check: ABSENT for check in missing})
    return missing


def lab_attack_gba(spec):
    def measure(series, golden, report):
        uid = report.get('subject')
        frames = lab_report_frames(report)
        if uid is None or len(frames) != 1:
            return None, 'the runner reported no subject or more than one order'
        m, err = lab_attack_measure(series, uid, frames[0], spec['foe_type'], report.get('unit_raw'))
        values = {}
        if m is not None:
            for key, check in (('range_first', 'lab.range_first'), ('first_hit', 'lab.first_hit'),
                               ('damage', 'lab.damage'), ('shield_damage', 'lab.shield_damage'),
                               ('cooldown', 'lab.cooldown'), ('counter_damage', 'lab.counter_damage'),
                               ('survival', 'lab.survival')):
                if m[key] is not None:
                    values[check] = m[key]
            if not m.get('taken_single', False):
                # Hits taken from several enemies in reach cannot stand for one enemy's damage.
                values.pop('lab.counter_damage', None)
            # Exact checks hold for every hit, not the median: submit the GBA hit furthest from
            # the N64 value, so one wrong hit among right ones still fails.
            n64 = {obs['check']: obs['value'] for obs in golden['observations']
                   if obs['gating'] and obs['lo'] == obs['hi']}
            for check, sizes in (('lab.damage', m['damage_values']), ('lab.shield_damage', m['shield_values']),
                                 ('lab.counter_damage', m['counter_values'] if m.get('taken_single') else [])):
                if sizes and check in n64:
                    values[check] = max(sizes, key=lambda v: (abs(v - n64[check]), v))
        measured = dict(m or {}, error=err)
        # Combat that happened but could not be attributed is not combat that never happened:
        # those checks get no value (a gating REGRESSION, "not measured"), never ABSENT, which
        # a baseline recording no fight would otherwise accept.
        # An observation that ended early is no evidence of absence either: a subject that
        # changed owner or type or left with hit points, a run that stopped before its requested
        # steps (the mission left play), or a measure that found no subject. What it did not reach is unmeasured,
        # survival included.
        requested, ran = golden.get('gba', {}).get('steps'), report.get('steps')
        truncated = requested is not None and ran is not None and int(ran) < int(requested)
        measured['truncated'] = truncated
        inconclusive, off_target = set(), False
        ended = m is None or m['subject_changed'] or m.get('subject_lost') or truncated
        if not ended and m['combat'] and not m['foe_combat']:
            # The subject fought only other enemies: what it never did to the foe is the
            # OFF_TARGET marker, which only a baseline holding the same marker accepts.
            off_target = True
        elif ended or m['combat']:
            # Survival is the one check whose absence is an outcome: the subject did not die.
            inconclusive = {obs['check'] for obs in golden['observations']} - set(values)
            if not ended and m['death'] is None:
                inconclusive.discard('lab.survival')
        inconclusive &= {obs['check'] for obs in golden['observations']}
        measured['inconclusive'] = sorted(inconclusive)
        missing = absent_values(golden, values)
        for check in inconclusive:
            values.pop(check, None)
        measured['absent'] = [c for c in missing if c not in inconclusive]
        measured['off_target'] = []
        if off_target:
            # Survival stays absent: the subject did not die.
            measured['off_target'] = [c for c in measured['absent'] if c != 'lab.survival']
            values.update({c: OFF_TARGET for c in measured['off_target']})
            measured['absent'] = [c for c in measured['absent'] if c == 'lab.survival']
        return {'values': values, 'measured': measured}, None
    return measure


# Cartridge unit ids the labs name (units.dat order).
UNIT_IDS = {'Marine': MARINE, 'SCV': SCV, 'Zergling': ZERGLING, 'Hydralisk': 38, 'Drone': DRONE, 'Overlord': 42}

LAB_SPECS = [
    {'id': 'lab-marine-move', 'kind': 'move', 'name': 'Marine', 'subject_type': MARINE, 'human_type': MARINE,
     'marks': ['command', 'turn'], 'profile': 'terran', 'runner': 'lab-t1',
     'title': 'T1 lab: one Marine moved out and back (ground, small)'},
    {'id': 'lab-scv-move', 'kind': 'move', 'name': 'SCV', 'subject_type': SCV, 'human_type': MARINE,
     'marks': ['command', 'turn'], 'profile': 'terran', 'runner': 'lab-t1',
     'title': 'T1 lab: one SCV moved out and back (worker)'},
    {'id': 'lab-marine-zergling', 'kind': 'attack', 'name': 'Marine', 'foe': 'Zergling', 'foe_type': ZERGLING,
     'subject_type': MARINE,
     'human_type': MARINE, 'profile': 'terran', 'runner': 'lab-t1',
     'title': 'T1 lab: one Marine attacks a Zergling pair (weapon, damage, range, death)'},
    {'id': 'lab-overlord-move', 'kind': 'move', 'name': 'Overlord', 'subject_type': 42, 'human_type': DRONE,
     'marks': ['command', 'turn'], 'profile': 'zerg-z10', 'runner': 'lab-bb', 'relative': True,
     'title': 'Blood Bath lab: the human Overlord moved out and back (air)'},
    # Staged starts: missions entered through the progress-byte guest edit (n64-staged).
    {'id': 'lab-vulture-move', 'kind': 'move', 'name': 'Vulture', 'subject_type': 2, 'human_type': MARINE,
     'marks': ['command', 'turn'], 'profile': 'terran', 'runner': 'lab-mission', 'mission': 6,
     'title': 'T3 lab: one Vulture moved out and back (hover)'},
    {'id': 'lab-hydralisk-move', 'kind': 'move', 'name': 'Hydralisk', 'subject_type': 38, 'human_type': DRONE,
     'marks': ['command', 'turn'], 'profile': 'zerg-z10', 'runner': 'lab-mission', 'mission': 1,
     'title': 'Z2 lab: one Hydralisk moved out and back (ground, medium)'},
    {'id': 'lab-mutalisk-move', 'kind': 'move', 'name': 'Mutalisk', 'subject_type': 43, 'human_type': DRONE,
     'marks': ['command', 'turn'], 'profile': 'zerg-z10', 'runner': 'lab-mission', 'mission': 1,
     'title': 'Z2 lab: one Mutalisk moved out and back (air)'},
    {'id': 'lab-tank-move', 'kind': 'move', 'name': 'Siege Tank', 'subject_type': 5, 'human_type': 5,
     'marks': ['command', 'turn'], 'profile': 'terran', 'runner': 'lab-mission', 'mission': 13,
     'title': 'T10 lab: one Siege Tank moved out and back (ground, large)'},
    {'id': 'lab-hydralisk-zealot', 'kind': 'attack', 'name': 'Hydralisk', 'foe': 'Zealot', 'foe_type': 65,
     'subject_type': 38,
     'human_type': DRONE, 'profile': 'zerg-z10', 'runner': 'lab-mission', 'mission': 1, 'shields': True,
     'title': 'Z2 lab: one Hydralisk attacks a Zealot (explosive on small, shields then hit points)'},
]
for _spec in LAB_SPECS:
    _make = (lab_move_n64, lab_move_gba) if _spec['kind'] == 'move' else (lab_attack_n64, lab_attack_gba)
    # world_points: the measurement replays the points the tape's orders were given at, so every
    # run of a bundle must agree on them (tools/parity_decode.py write_golden).
    SCENARIOS[_spec['id']] = {'title': _spec['title'], 'measure_n64': _make[0](_spec), 'measure_gba': _make[1](_spec),
                              'profile': _spec['profile'], 'runner': _spec['runner'], 'world_points': True,
                              # 2: order-byte checks, single-attacker hits, observed-order replay.
                              'decoder_revision': 2,
                              # An attack golden must check something (tools/sc64_parity.py judge).
                              'require_gating': _spec['kind'] == 'attack'}
    if _spec['kind'] == 'move':
        # 2 added the acceleration chord spanning two GBA position steps at the N64 cruise
        # (gba.accel_chord); 3 adds the order-byte checks and observed-order replay.
        SCENARIOS[_spec['id']]['decoder_revision'] = 3


# ---- passive train and build lab (Blood Bath computers) ---------------------
#
# Computer players train workers and raise buildings on their own from the
# first frame, so the S4 Blood Bath captures already hold train and build
# times. N64: a row born after the first in-mission frame with the status
# bit (+0xD8 bit 31) clear completes when the bit is set; its build timer
# (+0xA8) starts at the table value (S2). GBA: a unit's training runs from its
# producer's queue progress reading 1 to the unit's birth; a building's
# construction from its birth to its under-construction flag clearing. Both
# are reported in source frames.

SHIELD_REGEN = 7 / 256   # shield points regained per tick (Z2 Zealot): a drop that leaves no more
                         # than this many per elapsed frame may have hit 0 in between, so it is clipped
TRAIN_FIELDS = ['unit.type', 'unit.owner', 'unit.sprite', 'unit.status', 'unit.build_timer', 'clock.tick_counter',
                'clock.pause_halfword']
TRAIN_NAMES = {SCV: 'SCV', 64: 'Probe', MARINE: 'Marine', 106: 'Command Center', 109: 'Supply Depot',
               110: 'Refinery', 111: 'Barracks', 154: 'Nexus', 156: 'Pylon', 160: 'Gateway', 157: 'Assimilator'}
TRAIN_TOLERANCE = 2
TRAIN_GBA_EXTRA = 2400


def train_n64(cap, human):
    """{raw type: [(birth sf, duration, initial timer)]} for completed computer-made rows."""
    out = {}
    born = {}
    first = None
    for record in cap.ticks:
        if not record['table'] or record['paused'] or record['sf'] < 0:
            continue
        rows = record['rows']
        if first is None:
            first = set(rows)
            continue
        # A freed row index can be reused by a later unit: forget the old instance, and stop
        # treating the index as one that was already present when the capture began.
        for key in [k for k in born if k[0] not in rows]:
            del born[key]
        first &= set(rows)
        for index, row in rows.items():
            typ = cap.unit(row, 'type')
            owner = cap.unit(row, 'owner')
            done = cap.unit(row, 'status') >> 31
            key = (index, typ)
            if index in first:
                continue
            if key not in born:
                if owner == human or owner >= 8 or done:
                    born[key] = None
                    continue
                born[key] = (record['sf'], cap.unit(row, 'build_timer'))
            elif born[key] is not None and done:
                sf0, timer = born[key]
                out.setdefault(typ, []).append((sf0, record['sf'] - sf0, timer))
                born[key] = None
    return out


def train_gba(series, human, unit_raw, building_raw, ambiguous=None):
    """The same table from a GBA trace's births and building rows.

    Events are taken a step at a time. A producer completes its queue head in a step when its row
    then shows fewer queued units, a restarted progress or another head; a unit born in that step
    is credited to the one producer of its owner that completed its type there. A birth with no
    such producer, or with several, is counted in `ambiguous` (a list, when given) and left out."""
    out = {}
    queue = {}      # producer slot -> (start step, head engine type)
    state = {}      # producer slot -> (owner, count, progress, head) as last seen
    building = {}   # slot -> (birth step, engine type, owner)
    by_step = {}
    for event in series.get('events', []):
        if event[1] is not None:
            by_step.setdefault(event[1], []).append(event)
    for step in sorted(by_step):
        births, completed = [], {}
        for event in by_step[step]:
            tag = event[0]
            if tag == '+B':
                _, _, slot, typ, owner = event
                if step > 1:   # pre-placed buildings are born complete by the launch
                    building[slot] = (step, typ, owner)
            elif tag == '-B':
                _, _, slot, _, _ = event   # destroyed: its pending job and construction end here
                queue.pop(slot, None)
                building.pop(slot, None)
                state.pop(slot, None)
            elif tag == 'B':
                _, _, slot, typ, owner, uc, count, progress, head = event
                if slot in building and not uc:
                    step0, btyp, bowner = building.pop(slot)
                    if bowner != human:
                        raw = building_raw[btyp] if 0 <= btyp < len(building_raw) else -1
                        out.setdefault(raw, []).append((step0 * 4 // 5, (step - step0) * 4 / 5, None))
                before = state.get(slot)
                # The head completed when its progress restarts or the head changes; fewer queued
                # units with the same head still in progress is a cancelled tail item.
                if before and slot in queue and before[1] and (
                        progress < before[2] or head != before[3] or not count):
                    completed.setdefault((owner, before[3]), []).append((slot, queue.pop(slot)[0]))
                state[slot] = (owner, count, progress, head)
                if count and progress == 1 and owner != human:
                    queue[slot] = (step, head)
                elif not count:
                    queue.pop(slot, None)   # an empty queue holds no job (a cancel, or the last one done)
            elif tag == '+U':
                births.append(event)
        for _, _, slot, typ, owner in births:
            if owner == human or step <= 1:   # step 1 holds the units placed at launch
                continue
            candidates = completed.get((owner, typ), [])
            if len(candidates) != 1:
                if ambiguous is not None:
                    ambiguous.append((step, slot, typ, len(candidates)))
                continue
            _, step0 = candidates.pop()
            raw = unit_raw[typ] if 0 <= typ < len(unit_raw) else -1
            out.setdefault(raw, []).append((step0 * 4 // 5, (step - step0 + 1) * 4 / 5, None))
    return out


def train_observations(table, gba_types):
    obs = []
    for raw in sorted(table):
        durations = [d for _, d, _ in table[raw]]
        timers = sorted({t for _, _, t in table[raw] if t is not None})
        name = TRAIN_NAMES.get(raw, f'unit {raw}')
        value = statistics.median(durations)
        gating = raw in gba_types
        obs.append(observation('train.duration', name, value, value - TRAIN_TOLERANCE, value + TRAIN_TOLERANCE,
                               f'+-{TRAIN_TOLERANCE} source frames' + ('' if gating else
                               '; report only: no GBA computer of this race in the lobby'),
                               gating, TRAIN_FIELDS, unit='source frames from the start of training or '
                               f'construction to completion; N64 initial timer {timers}, {len(durations)} completions'))
    return obs


# GBA lobby (zerg-z10 income runner): one Terran computer, so only Terran types gate.
TRAIN_GBA_TYPES = {SCV, MARINE, 106, 109, 110, 111}


def train_measure_n64(cap, marks):
    series = cap.series(gating=True)
    first = series['frames'][0]['units']
    drones = {u[1] for u in first.values() if u[0] == DRONE}
    if len(drones) != 1:
        raise SystemExit(f'bb-train: Drones belong to owners {sorted(drones)}; expected one human')
    human = drones.pop()
    table = train_n64(cap, human)
    if SCV not in table:
        raise SystemExit('bb-train: no computer SCV completed in the capture')
    last = series['frames'][-1]['sf']
    # The GBA computer starts its buildings later than the N64 ones, so its run is longer
    # than the capture by TRAIN_GBA_EXTRA frames: long enough for a Barracks to finish.
    gba = {'scenario': 'income', 'profile': 'zerg-z10', 'at_step': 5,
           'steps': int((last + TRAIN_GBA_EXTRA) * 5 / 4) + 10, 'horizon': INCOME_HORIZON}
    notes = [f'human owner {human}; computer completions by type: ' +
             ', '.join(f'{TRAIN_NAMES.get(k, k)} {[d for _, d, _ in v]}' for k, v in sorted(table.items()))]
    return {'observations': train_observations(table, TRAIN_GBA_TYPES), 'gba': gba, 'notes': notes,
            'measured': {str(k): v for k, v in table.items()}}


def train_measure_gba(series, golden, report):
    unit_raw, building_raw = report.get('unit_raw'), report.get('building_raw')
    if not unit_raw or not building_raw:
        return None, 'the runner reported no engine-to-cartridge id maps'
    ambiguous = []
    table = train_gba(series, report['human_owner'], unit_raw, building_raw, ambiguous)
    values, absent = {}, []
    for obs in golden['observations']:
        raw = next((k for k, v in TRAIN_NAMES.items() if v == obs['subject']), None)
        if raw in table:
            values[(obs['check'], obs['subject'])] = statistics.median(d for _, d, _ in table[raw])
        elif obs['gating']:
            values[(obs['check'], obs['subject'])] = ABSENT
            absent.append(obs['subject'])
    return {'values': values, 'measured': {'table': {str(k): v for k, v in table.items()}, 'absent': absent,
                                           'ambiguous_births': ambiguous}}, None


SCENARIOS['bb-train'] = {'title': 'Blood Bath: computer train and build times (passive)', 'measure_n64': train_measure_n64,
                         'measure_gba': train_measure_gba, 'profile': 'zerg-z10', 'runner': 'income'}
