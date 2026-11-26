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

    A mark falls where the next step-advancing command (a pad line or `until`) began. When
    the run's commands.csv is beside the tape, those begin steps are the executed ones, which
    covers `load NAME STEP` renumbering and `until` waits of any length. Without it the pad
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
            elif kind == 'command' and (steps(word) or word == 'until'):
                index += 1
        if index != len(begins):
            raise SystemExit(f'{tape}: {index} step commands but {len(begins)} rows in commands.csv')
        return marks
    marks, step = {}, 0
    for kind, word in lines:
        if kind == 'command' and word in ('load', 'until'):
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
                 'clock.tick_counter', 'clock.pause_halfword']
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
