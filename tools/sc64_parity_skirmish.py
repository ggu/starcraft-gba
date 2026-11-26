#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Blood Bath skirmish parity scenarios: a passive human against one to three
computers (parity design WP11, with the WP9 economy curves that fall out of
them). docs/PARITY-HARNESS.md describes the harness; the measured gaps are in
docs/audits/2026-11-26-parity-skirmish.md.

Each scenario id names one N64 lobby: the human race, the computer rows and
the launch delay that picks the seed (sk-zvt-w37: human Zerg, one Terran
computer, Start pressed 37 steps late). The N64 seed (milliseconds since
power-on at launch, read back from 0x800EFF20) is passed to the GBA runner's
`match` scenario, whose native Blood Bath launch draws the same start corners
from it.

Both engines are reduced to one normalised frame stream and measured by the
same code (measure):

  {'sf': source frame,
   'units': {key: (cartridge type id, owner, x, y, complete)},   N64 map px
   'gathered': {owner: (minerals, gas or None)},  cumulative, since launch
   'bank': {owner: minerals},
   'supply': {owner: (used, cap)},                 doubled units
   'result': {owner: value}}                       the game's result byte per player (0 none, 2 defeated)

N64: unit rows from tap.bin (complete = status bit 31), the gathered and bank
arrays from tap.bin, supply and the result bytes (one per owner) from watch.csv. GBA: U
and B rows of the trace (type ids through the runner's engine-to-cartridge
map, complete = not under construction), bank and supply from P rows plus the
supply of queue heads in training, gathered totals from the runner's --gather file,
the human's result from the S rows' lost flag.

Players are joined by role and race, never by owner number: the human (the
only player whose bank never falls), and each computer by its race (lobbies
never repeat a computer race). Observations, per player label ('human',
'cpu Terran', ...):

  start.corner             corner of the starting town hall (0 NW, 1 NE, 2 SW, 3 SE)
  build.first.<Type>       first source frame a computer has the type: a
                           building when placed, a unit when complete
  econ.gathered@<sf>       minerals gathered since launch, at checkpoints
  econ.rate_k<n>           minerals per frame while the player had n workers (report only)
  supply.used@<sf>, supply.cap@<sf>, workers@<sf>
  attack.first_wave        first frame three of a computer's combat units
                           stand within 12 tiles of the human's town hall
  attack.first_contact     the same for one unit (report only)
  attack.supply_at_wave    the computer's supply at its first wave (report only)
  result.human_defeated    first frame the game has declared the human defeated (N64
                           result byte of the human's owner at 0x800B936C, GBA lost flag)
  result.eliminated        first frame a player owns no building row (report only: dying
                           N64 buildings keep their rows at 0 HP for a few frames)

A time never reached within the N64 run (a type never made, no wave, no
elimination) reads NEVER on either engine, so "never" matches only "never";
a GBA checkpoint after its match ended reads -1. Bands: each seed's value
widened only by the measurement's quantisation (times +-2 frames, gathered
minerals +-one trip per worker, counts exact); `envelope` then widens every
band of a lobby group except the start corner to the union over its seeds,
so the seed envelope is the tolerance only where the N64 itself varies
across seeds.

  sc64_parity_skirmish.py envelope [--home DIR]   union the bands of each lobby group's goldens
  sc64_parity_skirmish.py summary RUN             measure one N64 capture (no golden)
"""
import argparse
import bisect
import hashlib
import json
import math
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# ---- lobbies -----------------------------------------------------------------

RACE_NAME = {'T': 'Terran', 'Z': 'Zerg', 'P': 'Protoss'}
PROFILE = {'T': 'terran', 'Z': 'zerg-z10', 'P': 'protoss-p10'}   # the cartridge of the human's race
GBA_RACE = {0: 'T', 1: 'Z', 2: 'P'}            # R_TER, R_ZERG, R_PROTOSS in the runner report
N64_SUPPLY_RACE = {'Z': 0, 'T': 1, 'P': 2}      # supply block order on the cartridge

# id: (human, computer rows in seat order, launch delay in frontend steps)
LOBBIES = {
    'sk-tvt-w0': ('T', ['T'], 0), 'sk-tvz-w0': ('T', ['Z'], 0), 'sk-tvp-w0': ('T', ['P'], 0),
    'sk-zvt-w0': ('Z', ['T'], 0), 'sk-zvz-w0': ('Z', ['Z'], 0), 'sk-zvp-w0': ('Z', ['P'], 0),
    'sk-pvt-w0': ('P', ['T'], 0), 'sk-pvz-w0': ('P', ['Z'], 0), 'sk-pvp-w0': ('P', ['P'], 0),
    'sk-ffa-w0': ('T', ['Z', 'T', 'P'], 0),
    'sk-zvt-w37': ('Z', ['T'], 37), 'sk-tvz-w37': ('T', ['Z'], 37), 'sk-tvp-w37': ('T', ['P'], 37),
    'sk-ffa-w37': ('T', ['Z', 'T', 'P'], 37),
    'sk-zvt-w101': ('Z', ['T'], 101), 'sk-tvz-w101': ('T', ['Z'], 101), 'sk-tvp-w101': ('T', ['P'], 101),
}


def group_of(scenario_id):
    """Lobby group: the id without its launch-delay suffix (sk-zvt-w37 -> sk-zvt)."""
    return scenario_id.rsplit('-w', 1)[0]


# ---- cartridge unit ids --------------------------------------------------------

NAMES = {
    0: 'Marine', 1: 'Ghost', 2: 'Vulture', 3: 'Goliath', 5: 'Siege Tank', 7: 'SCV', 8: 'Wraith',
    9: 'Science Vessel', 11: 'Dropship', 12: 'Battlecruiser', 13: 'Spider Mine', 30: 'Siege Tank (sieged)',
    32: 'Firebat', 34: 'Medic', 35: 'Larva', 36: 'Egg', 37: 'Zergling', 38: 'Hydralisk', 39: 'Ultralisk',
    41: 'Drone', 42: 'Overlord', 43: 'Mutalisk', 44: 'Guardian', 45: 'Queen', 46: 'Defiler', 47: 'Scourge',
    50: 'Infested Terran', 58: 'Valkyrie', 59: 'Cocoon', 60: 'Corsair', 61: 'Dark Templar', 62: 'Devourer', 63: 'Dark Archon', 64: 'Probe',
    65: 'Zealot', 66: 'Dragoon', 67: 'High Templar', 68: 'Archon', 69: 'Shuttle', 70: 'Scout', 71: 'Arbiter',
    72: 'Carrier', 73: 'Interceptor', 83: 'Reaver', 84: 'Observer', 85: 'Scarab', 97: 'Lurker Egg',
    103: 'Lurker',
    106: 'Command Center', 107: 'Comsat Station', 108: 'Nuclear Silo', 109: 'Supply Depot', 110: 'Refinery',
    111: 'Barracks', 112: 'Academy', 113: 'Factory', 114: 'Starport', 115: 'Control Tower',
    116: 'Science Facility', 117: 'Covert Ops', 118: 'Physics Lab', 120: 'Machine Shop',
    122: 'Engineering Bay', 123: 'Armory', 124: 'Missile Turret', 125: 'Bunker', 130: 'Infested Command Center',
    131: 'Hatchery', 132: 'Lair', 133: 'Hive', 134: 'Nydus Canal', 135: 'Hydralisk Den', 136: 'Defiler Mound',
    137: 'Greater Spire', 138: "Queen's Nest", 139: 'Evolution Chamber', 140: 'Ultralisk Cavern', 141: 'Spire',
    142: 'Spawning Pool', 143: 'Creep Colony', 144: 'Spore Colony', 146: 'Sunken Colony', 149: 'Extractor',
    154: 'Nexus', 155: 'Robotics Facility', 156: 'Pylon', 157: 'Assimilator', 159: 'Observatory',
    160: 'Gateway', 162: 'Photon Cannon', 163: 'Citadel of Adun', 164: 'Cybernetics Core',
    165: 'Templar Archives', 166: 'Forge', 167: 'Stargate', 169: 'Fleet Beacon', 170: 'Arbiter Tribunal',
    171: 'Robotics Support Bay', 172: 'Shield Battery',
}
TOWN_HALLS = {106: 'T', 131: 'Z', 132: 'Z', 133: 'Z', 154: 'P'}
# Each race's melee types, for build-order expectations: a type the N64 computer never made within
# its run is expected NEVER, so a GBA computer that makes it is judged too.
RACE_TYPES = {
    'T': {0, 1, 2, 3, 5, 8, 9, 11, 12, 32, 34, 58, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117,
          118, 120, 122, 123, 124, 125},
    'Z': {37, 38, 39, 42, 43, 44, 45, 46, 47, 50, 62, 103, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
          141, 142, 143, 144, 146, 149},
    'P': {60, 61, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 83, 84, 154, 155, 156, 157, 159, 160, 162, 163, 164, 165,
          166, 167, 169, 170, 171, 172},
}
DEFEAT = 2                              # a player's result byte when it is defeated
SAME_TYPE = {30: 5}                     # a sieged Siege Tank is the same build-order type
WORKERS = {7, 41, 64}
# A computer's army: units with a weapon of their own. Workers, transports, detectors, casters,
# Medics, Overlords, morph stages, spawned parts and unmapped actors never count.
COMBAT = {0, 1, 2, 3, 5, 8, 12, 30, 32, 37, 38, 39, 43, 44, 47, 50, 58, 60, 61, 62, 65, 66, 68, 70, 71, 72,
          83, 103}
# Never in a build order: morph stages and spawned parts.
NOT_BUILD_ORDER = {35, 36, 59, 73, 85, 97}
BUILDING_IDS = range(106, 176)          # player structures; 176+ are resources and markers
PLAYERS = 8                             # owners 8+ are neutral

MAP_PX = 2048                           # Blood Bath is 64 x 64 tiles of 32 N64 px
WAVE_RADIUS = 384                       # 12 tiles around the human's town hall
WAVE_UNITS = 3
CHECKPOINTS = (1000, 2000, 4000, 6000, 8000)
TIME_TOLERANCE = 2                      # source frames, as the S4 intervals
DECODER_REVISION = 2                     # the skirmish scenarios' measurement revision
NEVER = 999999                          # a time not reached within the N64 run
ENDED = -1                              # a GBA checkpoint after its match ended
TRIP = 8                                # minerals per delivery
RATE_MIN_FRAMES = 300                   # frames at one worker count before its rate is measured

N64_FIELDS = ['unit.type', 'unit.owner', 'unit.x', 'unit.y', 'unit.sprite', 'unit.status', 'unit.hp',
              'clock.tick_counter', 'clock.pause_halfword', 'rng.seed']
BANK_FIELDS = ['banks.minerals', 'banks.minerals_gathered']
SUPPLY_FIELDS = ['supply.available', 'supply.used']


def corner(x, y):
    return (1 if x >= MAP_PX // 2 else 0) + (2 if y >= MAP_PX // 2 else 0)


def type_name(raw):
    return NAMES.get(raw, f'type{raw}')


# ---- the measurement (both engines) ----------------------------------------------

def measure(frames, human=None, horizon=None):
    """One pass over a normalised frame stream. human: the human owner if the caller
    knows it (GBA); otherwise the only player whose bank never falls (N64). horizon: stop
    after this source frame (the GBA side measures the N64 run's window only)."""
    first_sf = None
    halls = {}                     # owner -> (race, x, y) at the first frame
    roster = {}                    # owner -> types present at the first frame
    firsts = {}                    # owner -> {type: sf}
    bank_fell = {}
    series = {}                    # owner -> {'gathered': [], 'used': [], 'cap': [], 'workers': [], 'gas': []}
    near = {}                      # (attacker, defender) -> [first sf with >= 1 unit, first with >= WAVE_UNITS]
    buildings_gone = {}            # owner -> first sf with no building after the first frame
    last_bank = {}
    last_sf = None
    result_at = {}                 # owner -> first frame with a declared result
    sampled = []                   # source frames that had a record (the N64 loses a few)
    base = {}
    for frame in frames:
        sf = frame['sf']
        if horizon is not None and sf > horizon:
            continue  # read on, so a GBA trace's footer is still checked
        if first_sf is None:
            first_sf = sf
            for unit in frame['units'].values():
                raw, owner, x, y, _ = unit
                if owner < PLAYERS and raw in TOWN_HALLS and owner not in halls:
                    halls[owner] = (TOWN_HALLS[raw], x, y)
            for owner in halls:
                roster[owner] = sorted({u[0] for u in frame['units'].values() if u[1] == owner})
                firsts[owner] = {}
                bank_fell[owner] = False
                series[owner] = {'gathered': [], 'gas': [], 'used': [], 'cap': [], 'workers': [], 'bank': []}
                g = frame['gathered'].get(owner, (0, None))
                base[owner] = (g[0], g[1])
        index = sf  # launch-relative on both engines (the N64 may have lost the first record)
        counts = {o: {'workers': 0, 'buildings': 0} for o in halls}
        army = {}
        for raw, owner, x, y, complete in frame['units'].values():
            if owner not in halls:
                continue
            raw = SAME_TYPE.get(raw, raw)
            c = counts[owner]
            if raw in BUILDING_IDS:
                c['buildings'] += 1
            if raw in WORKERS and complete:
                c['workers'] += 1
            seen = raw in BUILDING_IDS or complete
            if seen and raw not in NOT_BUILD_ORDER and raw not in roster[owner] and raw not in firsts[owner]:
                firsts[owner][raw] = index
            if complete and raw in COMBAT:
                army.setdefault(owner, []).append((x, y))
        for attacker, points in army.items():
            for defender, (_, hx, hy) in halls.items():
                if defender == attacker:
                    continue
                inside = sum(1 for x, y in points if math.hypot(x - hx, y - hy) <= WAVE_RADIUS)
                rec = near.setdefault((attacker, defender), [None, None])
                if inside >= 1 and rec[0] is None:
                    rec[0] = index
                if inside >= WAVE_UNITS and rec[1] is None:
                    rec[1] = index
        for owner in halls:
            if counts[owner]['buildings'] == 0 and owner not in buildings_gone and index > 0:
                buildings_gone[owner] = index
            bank = frame['bank'].get(owner, 0)
            if owner in last_bank and bank < last_bank[owner]:
                bank_fell[owner] = True
            last_bank[owner] = bank
            g = frame['gathered'].get(owner, (0, None))
            used, cap = frame['supply'].get(owner, (None, None))
            s = series[owner]
            # One entry per source frame: a missing frame repeats the last value.
            while len(s['gathered']) < index:
                for key in s:
                    s[key].append(s[key][-1] if s[key] else 0)
            if len(s['gathered']) > index:
                continue
            s['gathered'].append(g[0] - base[owner][0])
            s['gas'].append(None if g[1] is None or base[owner][1] is None else g[1] - base[owner][1])
            s['used'].append(used)
            s['cap'].append(cap)
            s['workers'].append(counts[owner]['workers'])
            s['bank'].append(bank)
        for owner, value in frame.get('result', {}).items():
            if value and owner not in result_at:
                result_at[owner] = (index, value)
        last_sf = index
        sampled.append(index)
    if first_sf is None:
        raise ValueError('no in-mission frame')
    if human is None:
        passive = [o for o in halls if not bank_fell[o] and not firsts[o]]
        if len(passive) != 1:
            raise ValueError(f'cannot tell the passive human: candidates {passive} of players {sorted(halls)}')
        human = passive[0]
    if human not in halls:
        raise ValueError(f'human owner {human} has no town hall')
    labels = {}
    for owner, (race, _, _) in halls.items():
        labels[owner] = 'human' if owner == human else f'cpu {RACE_NAME[race]}'
    if len(set(labels.values())) != len(labels):
        raise ValueError(f'two players share a label: {labels}')
    players = {}
    for owner in halls:
        s = series[owner]
        rates = {}
        for k in range(1, 9):
            frames_at = [i for i in range(1, len(s['workers'])) if s['workers'][i - 1] == k]
            if len(frames_at) >= RATE_MIN_FRAMES:
                gain = sum(s['gathered'][i] - s['gathered'][i - 1] for i in frames_at)
                rates[k] = {'rate': gain / len(frames_at), 'frames': len(frames_at)}
        players[labels[owner]] = {
            'owner': owner, 'race': halls[owner][0], 'corner': corner(halls[owner][1], halls[owner][2]),
            'hall': [halls[owner][1], halls[owner][2]], 'roster': roster[owner],
            'firsts': {type_name(t): sf for t, sf in sorted(firsts[owner].items(), key=lambda kv: kv[1])},
            'first_ids': {str(t): sf for t, sf in firsts[owner].items()},
            'eliminated': buildings_gone.get(owner),
            'wave': near.get((owner, human), [None, None])[1],
            'contact': near.get((owner, human), [None, None])[0],
            'rates': rates, 'series': s,
        }
    return {'human': human, 'end': last_sf, 'players': players, 'sampled': sampled,
            'defeat': result_at[human][0] if result_at.get(human, (0, 0))[1] == DEFEAT else None,
            'results': {labels[o]: {'frame': sf, 'value': v} for o, (sf, v) in result_at.items() if o in labels}}


def at(curve, index):
    """Value of a per-frame curve at a frame; ENDED past its end."""
    if not curve:
        return None
    return curve[index] if index < len(curve) else ENDED


def checkpoint_frames(m, horizon):
    """Each checkpoint's frame: the first sampled frame at or after it, so a checkpoint never
    reads a value carried over a lost N64 record. The golden keeps them for the GBA side."""
    out = {}
    for cp in CHECKPOINTS:
        k = bisect.bisect_left(m['sampled'], cp)
        if k < len(m['sampled']) and m['sampled'][k] <= horizon:
            out[cp] = m['sampled'][k]
    return out


def values(m, horizon, frames=None, never_types=False):
    """Every measured number keyed (check, subject). A time later than the horizon (the N64
    run's last frame) or never reached reads NEVER. frames: checkpoint -> source frame
    (default checkpoint_frames)."""
    def time(sf):
        return sf if sf is not None and sf <= horizon else NEVER
    frames = checkpoint_frames(m, horizon) if frames is None else frames
    out = {}
    for label, p in m['players'].items():
        out[('start.corner', label)] = p['corner']
        s = p['series']
        for cp, f in frames.items():
            out[(f'econ.gathered@{cp}', label)] = at(s['gathered'], f)
            out[(f'supply.used@{cp}', label)] = at(s['used'], f)
            out[(f'supply.cap@{cp}', label)] = at(s['cap'], f)
            out[(f'workers@{cp}', label)] = at(s['workers'], f)
        for k, r in p['rates'].items():
            out[(f'econ.rate_k{k}', label)] = r['rate']
        out[('result.eliminated', label)] = time(p['eliminated'])
        if label == 'human':
            out[('result.human_defeated', label)] = time(m['defeat'])
            continue
        for name, sf in p['firsts'].items():
            out[(f'build.first.{name}', label)] = time(sf)
        if never_types:
            for t in sorted(RACE_TYPES[p['race']] - set(p['roster'])):
                out.setdefault((f'build.first.{type_name(t)}', label), NEVER)
        out[('attack.first_wave', label)] = time(p['wave'])
        out[('attack.first_contact', label)] = time(p['contact'])
        if p['wave'] is not None and p['wave'] <= horizon:
            out[('attack.supply_at_wave', label)] = at(s['used'], p['wave'])
    return out


def observation(check, subject, value, band, tolerance, gating, fields, unit):
    lo, hi = band
    return {'check': check, 'subject': subject, 'value': round(value, 4) if isinstance(value, float) else value,
            'lo': round(lo, 4) if isinstance(lo, float) else lo, 'hi': round(hi, 4) if isinstance(hi, float) else hi,
            'tolerance': tolerance, 'gating': gating, 'layout_fields': fields, 'unit': unit}


def observations(m, horizon):
    """Observations of one N64 measurement, each band its value widened by quantisation only."""
    obs = []
    frames = checkpoint_frames(m, horizon)
    vals = values(m, horizon, frames, never_types=True)
    for (check, subject), value in sorted(vals.items()):
        if value is None:
            continue
        p = m['players'][subject]
        fields = list(N64_FIELDS)
        kind = check.split('@')[0].split('.')[0]
        if check == 'start.corner':
            band, tol, unit = (value, value), 'exact', 'corner of the starting town hall: 0 NW, 1 NE, 2 SW, 3 SE'
        elif check.startswith('econ.gathered@'):
            workers = at(p['series']['workers'], frames[int(check.split('@')[1])]) or 0
            slack = TRIP * max(workers, 1)
            # Never below zero, so a GBA checkpoint after its match ended (ENDED) is always outside.
            band, tol, unit = (max(0, value - slack), value + slack), f'+-one trip per worker ({slack} minerals)', \
                'minerals gathered since launch'
            fields += BANK_FIELDS
        elif check.startswith('econ.rate_k'):
            k = int(check[len('econ.rate_k'):])
            span = p['rates'][k]['frames']
            slack = TRIP * k / span
            band, tol, unit = (value - slack, value + slack), f'+-one trip per worker over {span} frames', \
                f'minerals gathered per source frame while the player had {k} workers'
            fields += BANK_FIELDS
        elif kind == 'supply':
            band, tol, unit = (value, value), 'exact', 'doubled supply units (a Marine is 2)'
            fields += SUPPLY_FIELDS
        elif kind == 'workers':
            band, tol, unit = (value, value), 'exact', 'complete workers'
        elif check == 'attack.supply_at_wave':
            band, tol, unit = (value, value), 'exact', 'the computer\'s doubled supply used at its first wave'
            fields += SUPPLY_FIELDS
        elif value == NEVER:
            band, tol, unit = (NEVER, NEVER), 'exact: not within the N64 run', \
                f'source frames since launch ({NEVER}: not within the N64 run)'
        else:
            t = TIME_TOLERANCE
            band, tol, unit = (value - t, value + t), f'+-{t} source frames', \
                f'source frames since launch ({NEVER}: not within the N64 run)'
        # Report only: contact is any one unit (a scout counts), supply at the wave depends on the
        # wave, and a rate at a worker count exists only for the counts a player happened to hold
        # long enough (the checkpoints gate the same gathered curve).
        gating = check not in ('attack.first_contact', 'attack.supply_at_wave', 'result.eliminated') and \
            not check.startswith('econ.rate_k')
        if check == 'result.human_defeated':
            fields.append('result.word')
        o = observation(check, subject, value, band, tol, gating, fields, unit)
        # Outcomes, not numbers: the judge compares a sentinel only with the same sentinel.
        o['sentinels'] = [NEVER, ENDED]
        if '@' in check:
            o['frame'] = frames[int(check.split('@')[1])]
            o['unit'] += f' at source frame {o["frame"]} ({ENDED}: the GBA match had ended)'
        obs.append(o)
    return obs


CURVE_STEP = 8


def curves(m, horizon):
    """Per-player curves for first-divergence reports, every CURVE_STEP source frames."""
    out = {}
    for label, p in m['players'].items():
        s = p['series']
        for name, key, tol, checks in (('gathered', 'gathered', 4 * TRIP, 'econ.gathered@'),
                                       ('supply', 'used', 0, 'supply.used@'),
                                       ('workers', 'workers', 0, 'workers@')):
            series = s[key][:horizon + 1:CURVE_STEP]
            if any(v is None for v in series):
                continue
            out[f'{name}:{label}'] = {
                'n64': series, 'tolerance': tol, 'step': CURVE_STEP,
                'explains': [[f'{checks}{cp}', label] for cp in CHECKPOINTS if cp <= horizon],
                'unit': f'{name} per {CURVE_STEP} source frames from launch'}
    return out


def gba_curves(m, golden):
    out = {}
    for name, curve in golden.get('curves', {}).items():
        kind, label = name.split(':', 1)
        p = m['players'].get(label)
        if p is None:
            continue
        key = {'gathered': 'gathered', 'supply': 'used', 'workers': 'workers'}[kind]
        out[name] = p['series'][key][::curve.get('step', 1)]
    return out


# ---- N64 side ------------------------------------------------------------------

def read_watch(run):
    """watch.csv as {address: (bytes, [steps], [values])}, sorted by step. The frontend logs a
    field on the step it is set and then whenever it changes."""
    out = {}
    path = pathlib.Path(run) / 'watch.csv'
    lines = path.read_text().splitlines()[1:]
    for line in lines:
        step, _tick, address, row_width, value = line.split(',')
        width, steps, vals = out.setdefault(int(address, 16) | 0x80000000, (int(row_width), [], []))
        if int(row_width) != width:
            raise ValueError(f'{path}: {address} watched with two widths')
        steps.append(int(step))
        vals.append(int(value, 16))
    return out


def watched(watch, address, step, width=4):
    """The watched word's value after frontend step `step`."""
    entry = watch.get(address)
    if entry is None or entry[0] != width:
        raise ValueError(f'the capture did not watch {address:#x} as {width} bytes')
    k = bisect.bisect_right(entry[1], step) - 1
    if k < 0:
        raise ValueError(f'{address:#x} has no watched value by step {step}')
    return entry[2][k]


def n64_frames(cap, gating=True):
    """Normalised frames of one capture (the decoder's unpaused source frames)."""
    layout = cap.layout
    watch = read_watch(cap.run)
    for name in ('available', 'used'):
        layout.supply_array(name, gating)
    result = int(layout.result_word(gating)['address'], 16)
    from sc64_parity_metrics import frontend_lines
    tape = (cap.run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')
    if any(w[:2] == ['watch', 'off'] for w in frontend_lines(tape)):
        raise ValueError(f'{cap.run}: the tape turns its watches off; supply and result would go stale')
    maximum = 400
    races = {}
    for record in cap.ticks:
        # The decoder's unpaused clock, which also counts a tick whose follow-up read was lost
        # because the next step changed the tick again (frequent in whole matches).
        if not record['table'] or record['paused'] or record['sf'] < 0:
            continue
        units = {}
        for index, row in record['rows'].items():
            # A unit at 0 HP keeps its row while it dies; it counts neither as a worker nor in a wave.
            # A dying building keeps its row for the building census (report only).
            raw = cap.unit(row, 'type', gating)
            if raw not in BUILDING_IDS and cap.unit(row, 'hp', gating) == 0:
                continue
            units[index] = (raw, cap.unit(row, 'owner', gating), cap.unit(row, 'x', gating),
                            cap.unit(row, 'y', gating), cap.unit(row, 'status', gating) >> 31)
        bank, gathered, supply = {}, {}, {}
        # One result byte per owner, owner 0 in the word's most significant byte.
        word = watched(watch, result, record['step'])
        for owner in range(4):
            bank[owner] = cap.bank(record, 'minerals', owner, gating)
            gathered[owner] = (cap.bank(record, 'minerals_gathered', owner, gating),
                               cap.bank(record, 'gas_gathered', owner, False))
            if owner not in races:
                for race in range(3):
                    if watched(watch, layout.supply_address('available', race, owner), record['step']):
                        races[owner] = race
            if owner in races:
                r = races[owner]
                supply[owner] = (watched(watch, layout.supply_address('used', r, owner), record['step']),
                                 min(maximum, watched(watch, layout.supply_address('available', r, owner), record['step'])))
        yield {'sf': record['sf'], 'step': record['step'], 'units': units, 'bank': bank, 'gathered': gathered,
               'supply': supply, 'result': {o: (word >> (24 - 8 * o)) & 0xFF for o in range(4)}}


def seed_of(cap, gating=True):
    """The launch seed (0x800EFF20), read back from the first in-mission tick."""
    field = cap.layout.rng_field('seed', gating)
    words = cap.seed()   # the rng block's bytes, as tap.bin records them
    if not words or field.get('type') != 'u32':
        return None
    at_byte = 2 * (int(field['address'], 16) - int(cap.layout.doc['globals']['rng']['address'], 16))
    return int(words[at_byte:at_byte + 8], 16)


def lobby_tape(human, cpus, wait, until_max=150000):
    """The capture tape of one lobby, as the captures were made: load the lobby state (Scenario
    tab, Free For All, Blood Bath, cursor on the Map row), set the human row (Right cycles Terran,
    Protoss, Random, Zerg) and the three computer rows (default Zerg, Terran, Protoss; Right cycles
    Zerg, Terran, Protoss, Random, Closed), watch the seed, result and supply words, wait 60 + wait
    steps, press Start, record every tick, and stop 600 steps after the result word changes. A
    golden's tape must be exactly this text, which pins the lobby and the launch delay."""
    rights = {'T': 0, 'P': 1, 'Z': 3}
    cycle, default = ['Z', 'T', 'P', 'R', '-'], ['Z', 'T', 'P']
    rows = list(cpus) + ['-'] * (3 - len(cpus))
    lines = ['load lobby.state 5120']

    def press(mask, gap=50):
        lines.extend([f'10 {mask} 0 0', f'{gap} 0 0 0'])
    press(4, 60)
    for _ in range(rights[human]):
        press(16)
    for row in range(3):
        press(4, 60)
        for _ in range((cycle.index(rows[row]) - cycle.index(default[row])) % 5):
            press(16)
    lines.append('shot lobby')
    lines += ['watch 800EFF20 4', 'watch 800B936C 4', 'watch 8010DBE0 4']
    for race in range(3):
        for array in (0, 0x30):
            for owner in range(4):
                lines.append(f'watch {0x800B3AC0 + race * 0x90 + array + 4 * owner:08X} 4')
    lines += [f'{60 + wait} 0 0 0', 'tap tick rec', '# mark command', '10 2 0 0',
              f'until 800B936C 4 ne 0 {until_max} 0 0 0', '600 0 0 0', 'shot end', 'quit']
    return '\n'.join(lines) + '\n'


def read_watch_ticks(run, address):
    """[(step, tick, value)] of one watched 4-byte word, from watch.csv."""
    out = []
    for line in (pathlib.Path(run) / 'watch.csv').read_text().splitlines()[1:]:
        step, tick, addr, width, value = line.split(',')
        if int(addr, 16) | 0x80000000 == address:
            if width != '4':
                raise ValueError(f'{run}: {addr} watched as {width} bytes')
            out.append((int(step), int(tick), int(value, 16)))
    return out


def tick_frames(cap):
    """(frame_of, last): tick -> source frame on the decoder's unpaused clock. A follow-up record
    gives its own frame (a paused tick keeps the last unpaused frame), a skipped unpaused tick
    the frame its change read was counted at; a tick after the last follow-up adds the unpaused
    in-mission change reads up to it; any other tick keeps the frame of the last dated tick
    before it. last: the run's last unpaused frame, trailing ticks included. The ticks after the
    last follow-up must run on without a gap (a missing change read leaves its pause state
    unknown), and a tick past the capture's last read has no frame: both raise ValueError."""
    # A skirmish capture never leaves its mission: a read without a unit table after the first
    # one would be a tick the clock cannot place (the decoder adds skipped frames only before
    # in-mission records), so it is refused rather than dated.
    start = next((i for i, r in enumerate(cap.ticks) if r['table']), None)
    if start is not None and (any(not r['table'] for r in cap.ticks[start:]) or
                              any(not table for _, _, table in getattr(cap, 'trailing_changes', []))):
        raise ValueError(f'{cap.run}: a read without a unit table after the mission started')
    known = {r['tick']: r['sf'] for r in cap.ticks if r['table'] and r['sf'] >= 0}
    skipped = dict(getattr(cap, 'skipped_frames', []))
    for tick, step in getattr(cap, 'change_steps', {}).items():
        if step in skipped and tick not in known:
            known[tick] = skipped[step]
    if not known:
        return None, None
    ticks = sorted(known)
    covered = ticks[-1]
    # Trailing tick -> (paused, in mission) from its latest read (the decoder's rule for a tick's
    # change read). Every read must keep the order; only unpaused ticks advance the clock.
    state = {}
    for tick, paused, table in getattr(cap, 'trailing_changes', []):
        if tick == covered:
            if tick in state:
                state[tick] = (paused, table)   # another read of the same tick
            continue
        if tick != covered + 1:
            raise ValueError(f'{cap.run}: change reads after the last follow-up go from tick {covered} to {tick}')
        state[tick] = (paused, table)
        covered = tick
    trailing = sorted(t for t, (paused, table) in state.items() if table and not paused)

    def frame_of(tick):
        if tick in known:
            return known[tick]
        if tick > covered:
            raise ValueError(f'{cap.run}: tick {tick} is after the capture\'s last read (tick {covered})')
        k = bisect.bisect_right(ticks, tick) - 1
        if k < 0:
            return 0
        if k == len(ticks) - 1:
            return known[ticks[-1]] + bisect.bisect_right(trailing, tick)
        return known[ticks[k]]
    return frame_of, known[ticks[-1]] + len(trailing)


def result_frames(cap):
    """{owner: (source frame, value)} of each owner's first non-zero result byte. watch.csv names
    the tick during which the byte changed; its frame is that tick's on the decoder's unpaused
    clock (tick_frames), so paused ticks (the result screen) add nothing."""
    address = int(cap.layout.result_word(True)['address'], 16)
    frame_of, _ = tick_frames(cap)
    if frame_of is None:
        return {}
    out = {}
    for step, tick, word in read_watch_ticks(cap.run, address):
        for owner in range(4):
            value = (word >> (24 - 8 * owner)) & 0xFF
            if value and owner not in out:
                out[owner] = (frame_of(tick), value)
    return out


def make_measure_n64(scenario_id):
    human_race, cpus, wait = LOBBIES[scenario_id]

    def measure_n64(cap, marks):
        first = next((r for r in cap.ticks if r['table']), None)
        if first is None or first['tick'] != cap.first_table_tick:
            # The clock would start one tick late: every frame would be labelled one too early.
            raise SystemExit(f'{scenario_id}: the first in-mission tick has no follow-up read; capture again')
        m = measure(n64_frames(cap))
        # Result bytes on the clock itself, so a change on a tick whose follow-up read was lost,
        # or on the last unpaused tick, is not missed.
        labels = {p['owner']: label for label, p in m['players'].items()}
        try:
            results = result_frames(cap)
            _, last = tick_frames(cap)
        except ValueError as error:
            raise SystemExit(f'{scenario_id}: {error}; capture again') from None
        m['results'] = {labels[o]: {'frame': sf, 'value': v} for o, (sf, v) in results.items() if o in labels}
        mine = results.get(m['human'])
        m['defeat'] = mine[0] if mine and mine[1] == DEFEAT else None
        human = m['players']['human']
        if human['race'] != human_race:
            raise SystemExit(f'{scenario_id}: the passive player is {RACE_NAME[human["race"]]}, the lobby says '
                             f'{RACE_NAME[human_race]}')
        races = sorted(p['race'] for label, p in m['players'].items() if label != 'human')
        if races != sorted(cpus):
            raise SystemExit(f'{scenario_id}: computers {races}, the lobby says {sorted(cpus)}')
        seed = seed_of(cap)
        if seed is None:
            raise SystemExit(f'{scenario_id}: no seed word in the capture')
        if (cap.run / 'tape.txt').read_bytes() != lobby_tape(human_race, cpus, wait).encode():
            raise SystemExit(f'{scenario_id}: the capture\'s tape is not this lobby\'s tape (lobby_tape)')
        # The tape stops 600 steps after the first result byte changes (the human's, or in a
        # free-for-all possibly a computer's); the result screen pauses the game, so only a few
        # unpaused frames follow the human's defeat.
        if not m['results']:
            raise SystemExit(f'{scenario_id}: the capture ends before any result byte changes')
        # The run's last unpaused frame, sampled or skipped (a defeat can land on a skipped tick),
        # including the unpaused in-mission ticks after the last follow-up read (paused ones add
        # nothing).
        horizon = max([m['end'], last or 0] + [sf for _, sf in getattr(cap, 'skipped_frames', [])] +
                      [r['frame'] for r in m['results'].values()])
        gba = {'scenario': 'match', 'profile': PROFILE[human_race], 'at_step': 5, 'seed': seed,
               'horizon': horizon, 'steps': int((horizon + 40) * 5 / 4) + 10}
        notes = [f'seed {seed} (0x800EFF20, read back); launch delay {wait} steps; owners '
                 + ', '.join(f'{label} {p["owner"]}' for label, p in sorted(m['players'].items()))
                 + f'; result bytes set: ' + (', '.join(f'{k} {v["value"]} at source frame {v["frame"]}'
                                                        for k, v in sorted(m['results'].items()))) +
                 f'; capture ends at source frame {horizon}']
        measured = {label: {k: v for k, v in p.items() if k != 'series'} for label, p in m['players'].items()}
        return {'observations': observations(m, horizon), 'gba': gba, 'notes': notes,
                'curves': curves(m, horizon), 'measured': {'players': measured, 'end': horizon}}
    return measure_n64


# ---- GBA side --------------------------------------------------------------------

def gba_frames(trace_path, gather_path, report):
    """Normalised frames of one GBA match trace (last row per source frame, N64 px). A malformed
    or incomplete trace raises ValueError."""
    try:
        yield from _gba_frames(trace_path, gather_path, report)
    except (IndexError, KeyError) as error:
        raise ValueError(f'{trace_path}: malformed record ({error!r})') from None


def _gba_frames(trace_path, gather_path, report):
    from sc64_parity_selfgate import record_ok
    match = report['match']
    # Type maps: every runner report carries them; older match reports kept them in the match block.
    unit_raw = report.get('unit_raw', match.get('unit_raw'))
    building_raw = report.get('building_raw', match.get('building_raw'))
    unit_supply = match['unit_supply']
    human = next(o['owner'] for o in match['owners'] if o['human'])
    gather = {}
    if gather_path is not None:
        # The runner ends the file with "# end STEPS": a truncated log is an error, never a total.
        lines = pathlib.Path(gather_path).read_text().splitlines()
        end = lines[-1].split() if lines else []
        if len(end) != 3 or end[:2] != ['#', 'end'] or int(end[2]) != report.get('steps'):
            raise ValueError(f'{gather_path}: no "# end {report.get("steps")}" footer; the gather log is incomplete')
        for line in lines[:-1]:
            if line.startswith('#'):
                continue
            step, owner, ore, gas = (int(w) for w in line.split())
            gather.setdefault(step, []).append((owner, ore, gas))
    units, buildings, queues, banks = {}, {}, {}, {}
    lost = False
    totals = {}
    step, pending, complete = None, None, False

    def snapshot():
        # The cartridge's used supply counts a unit from the tick its training starts and never
        # a waiting order or a head stalled at the cap (docs/audits/2026-11-25-waiting-training-
        # supply.md): add only queue heads that are progressing.
        supply = {}
        for owner, (minerals, used, cap) in banks.items():
            queued = sum(unit_supply[head] for (o, head) in queues.values()
                         if o == owner and head is not None and 0 <= head < len(unit_supply))
            supply[owner] = (used + queued, cap)
        merged = dict(units)
        merged.update(buildings)
        return {'sf': step * 4 // 5, 'step': step, 'units': merged, 'result': {human: DEFEAT if lost else 0},
                'bank': {o: v[0] for o, v in banks.items()},
                'gathered': {o: (v[0], v[1]) for o, v in totals.items()}, 'supply': supply}

    with open(trace_path) as handle:
        if not handle.readline().startswith('#sc64-parity-trace 1'):
            raise ValueError(f'{trace_path}: not a sc64-parity-trace/1 file')
        for line in handle:
            words = line.split()
            if not words:
                continue
            # Every record must have its schema's shape (the self-gate's check), so a damaged row
            # is an error rather than a silently missing unit or building.
            if words[0] != 'Z' and not record_ok(line.rstrip('\n').encode()):  # Z is checked below
                raise ValueError(f'{trace_path}: malformed record {line[:40]!r}')
            tag = words[0]
            if tag == 'S':
                if int(words[1]) != (step or 0) + 1:
                    raise ValueError(f'{trace_path}: step {words[1]} follows step {step}')
                if step is not None:
                    frame = snapshot()
                    if pending is not None and pending['sf'] != frame['sf']:
                        yield pending
                    pending = frame
                step = int(words[1])
                lost = int(words[7]) != 0
                for owner, ore, gas in gather.get(step, ()):
                    totals[owner] = (ore, gas)
            elif tag == 'U':
                slot, typ, raw, owner, raw_owner, x, y, hp = (int(w) for w in words[1:9])
                raw = raw if raw >= 0 else (unit_raw[typ] if 0 <= typ < len(unit_raw) else -1)
                if hp > 0:
                    units[slot] = (raw, owner, 2 * x, 2 * y, 1)
                else:
                    units.pop(slot, None)  # the N64 rule: a unit at 0 HP no longer counts
            elif tag == '-U':
                units.pop(int(words[1]), None)
            elif tag == 'B':
                f = [int(w) for w in words[1:]]
                slot, typ, raw, owner, x, y, building = f[0], f[1], f[2], f[3], f[5], f[6], f[10]
                raw = raw if raw >= 0 else (building_raw[typ] if 0 <= typ < len(building_raw) else -1)
                buildings[100000 + slot] = (raw, owner, 2 * x, 2 * y, 0 if building else 1)
                queues[slot] = (owner, f[15] if f[13] > 0 and f[14] > 0 and f[15] >= 0 else None)
            elif tag == '-B':
                buildings.pop(100000 + int(words[1]), None)
                queues.pop(int(words[1]), None)
            elif tag == 'P':
                owner, minerals, _gas, used, cap = (int(w) for w in words[1:6])
                banks[owner] = (minerals, used, cap)
            elif tag == 'Z':
                fields = dict(w.split('=', 1) for w in words[1:] if '=' in w)
                complete = fields.get('overflow') == '0' and fields.get('write_errors') == '0' and \
                    fields.get('steps') == str(step or 0)
                if any(line.strip() for line in handle):
                    raise ValueError(f'{trace_path}: records after the Z line')
    if step is not None:
        frame = snapshot()
        if pending is not None and pending['sf'] != frame['sf']:
            yield pending
        yield frame
    if not complete:
        raise ValueError(f'{trace_path}: incomplete trace (no clean Z line)')


def gba_series(trace, report):
    """The comparator's hook: a lazy frame source, measured by measure_gba."""
    gather = pathlib.Path(str(trace) + '.gather')
    return {'source': 'gba', 'trace': str(trace), 'gather': str(gather) if gather.is_file() else None,
            'report': report}


def gba_args(scenario_id, params, trace):
    """Runner arguments beyond the common ones: races from the scenario table, integers from the golden."""
    human, cpus, _ = LOBBIES[scenario_id]
    args = ['--human', human.lower()]
    for race in cpus:
        args += ['--bot', race.lower()]
    return args + ['--seed', str(int(params['seed'])), '--gather', str(trace) + '.gather']


def make_measure_gba(scenario_id):
    def measure_gba(series, golden, report):
        match = report.get('match')
        if not match:
            return None, 'the runner report has no match block'
        human_race, cpus, wait = LOBBIES[scenario_id]
        tape = hashlib.sha256(lobby_tape(human_race, cpus, wait).encode()).hexdigest()
        if golden.get('bundle', {}).get('tape_sha256') != tape:
            return None, 'the golden was captured with another tape than lobby_tape(); write it again'
        human = next((o['owner'] for o in match['owners'] if o['human']), None)
        # The whole trace first: footer against the run report, record counts and the body hash
        # (the self-gate's check), so a damaged trace never yields a verdict.
        from sc64_parity_selfgate import check_trace_complete
        try:
            check_trace_complete(pathlib.Path(series['trace']), report, verify_body=True)
        except (RuntimeError, KeyError, ValueError, IndexError) as error:
            return None, f'{series["trace"]}: incomplete trace ({error!r})' if isinstance(error, IndexError) else str(error)
        horizon = int(golden['gba']['horizon'])
        timed = ('build.first.', 'attack.first', 'result.')

        def reach_of(obs):
            # A finite timing band can end after the N64 run's last frame (a defeat on that
            # frame, +-2): the GBA time counts through the band's upper edge. NEVER
            # expectations and the checkpoints keep the run's own cutoff.
            hi = obs.get('hi')
            if obs['check'].startswith(timed) and isinstance(hi, (int, float)) and hi != NEVER \
                    and obs.get('value') != NEVER:
                return max(horizon, int(hi))
            return horizon
        reach = max([horizon] + [reach_of(o) for o in golden['observations']])
        try:
            m = measure(gba_frames(series['trace'], series['gather'], report), human, horizon)
            late = (measure(gba_frames(series['trace'], series['gather'], report), human, reach)
                    if reach > horizon else None)
        except ValueError as error:
            return None, str(error)
        if series['gather'] is None:
            return None, 'the runner wrote no --gather file'
        frames = {int(o['check'].split('@')[1]): int(o['frame']) for o in golden['observations'] if 'frame' in o}
        got = values(m, horizon, frames)
        got_late = values(late, reach, frames) if late is not None else {}
        flat, late_waves = {}, []
        for obs in golden['observations']:
            key = (obs['check'], obs['subject'])
            v = got.get(key)
            # A type the GBA never made within the horizon has no entry: NEVER, as on the N64 side;
            # one the GBA player already had at launch reads frame 0.
            if v is None and key[1] in m['players'] and obs['check'].startswith(('build.first.', 'attack.first')):
                roster = {type_name(t) for t in m['players'][key[1]]['roster']}
                v = 0 if obs['check'][len('build.first.'):] in roster and obs['check'].startswith('build.first.') \
                    else NEVER
            if v == NEVER and obs['check'].startswith(timed):
                # The same events, measured on to the band's edge (frames up to the run's end agree).
                after = got_late.get(key)
                if after is not None and after != NEVER and horizon < after <= reach_of(obs):
                    v = after
                    if obs['check'] == 'attack.first_wave':
                        late_waves.append(key[1])
            flat[key] = v
        for label in late_waves:
            # A wave admitted past the run's end reports its supply from the same measurement.
            supply = ('attack.supply_at_wave', label)
            if supply in got_late and supply in {(o['check'], o['subject']) for o in golden['observations']}:
                flat[supply] = got_late[supply]
        extra = sorted(f'{label}: {name}' for label, p in m['players'].items() if label != 'human'
                       for name in p['firsts'] if (f'build.first.{name}', label) not in
                       {(o['check'], o['subject']) for o in golden['observations']})
        measured = {label: {k: v for k, v in p.items() if k != 'series'} for label, p in m['players'].items()}
        return {'values': flat, 'curves': gba_curves(m, golden),
                'measured': {'players': measured, 'end': m['end'], 'gba_only_types': extra,
                             'start_order': match.get('start_order')}}, None
    return measure_gba


def scenarios():
    """Entries for tools/sc64_parity_metrics.py SCENARIOS."""
    out = {}
    for scenario_id, (human, cpus, wait) in LOBBIES.items():
        rows = ' and '.join(RACE_NAME[c] for c in cpus)
        out[scenario_id] = {
            'title': f'Blood Bath Free for All: passive human {RACE_NAME[human]} against computer {rows}, '
                     f'launch delay {wait} steps',
            'measure_n64': make_measure_n64(scenario_id), 'measure_gba': make_measure_gba(scenario_id),
            'profile': PROFILE[human], 'runner': 'match', 'gba_args': gba_args, 'gba_series': gba_series,
            'group': group_of(scenario_id),
            # Supply and result come from watch.csv, outside the hash chain: both captures' logs
            # must be identical (parity_decode.same_bundle).
            'same_watch': True,
            # 2: result bytes and the run's end on the decoder's unpaused clock (paused and
            # trailing ticks); goldens measured before are stale.
            'decoder_revision': DECODER_REVISION,
        }
    return out


# ---- seed envelopes ----------------------------------------------------------------

def envelope(goldens):
    """Union every observation band over the goldens of one lobby group (several seeds), so the
    tolerance is the N64's own spread. Rows a previous run synthesised are dropped first and
    each seed's own band (seed_lo, seed_hi) is used, so re-running never widens twice. The
    start corner stays exact per seed.

    NEVER is an outcome, not the top of a range. A seed's NEVER for a timing counts for the
    others only if that seed's run lasted past the earliest time another seed saw it (a capture
    that ended earlier says nothing about it); then the band covers the finite seeds and lists
    NEVER under also_ok. A finite time later than a seed's own run is left out of its band."""
    docs = [json.loads(p.read_text()) for p in goldens]
    layouts = {d.get('layout_sha256') for d in docs}
    if len(layouts) != 1:
        raise ValueError(f'{[d["id"] for d in docs]}: goldens decoded with different layouts; write them again')
    for doc in docs:
        human, cpus, wait = LOBBIES[doc['id']]
        if doc.get('bundle', {}).get('tape_sha256') != hashlib.sha256(lobby_tape(human, cpus, wait).encode()).hexdigest():
            raise ValueError(f'{doc["id"]}: captured with another tape than lobby_tape(); write it again')
    revisions = {d.get('decoder_revision', 1) for d in docs}
    if revisions != {DECODER_REVISION}:
        raise ValueError(f'{[d["id"] for d in docs]}: goldens measured by decoder revisions {sorted(revisions)}, '
                         f'the scenarios are at {DECODER_REVISION}; write them again')
    seeds = [d['gba']['seed'] for d in docs]
    if len(set(seeds)) != len(seeds):
        raise ValueError(f'{[d["id"] for d in docs]}: two goldens of one lobby share a seed ({sorted(seeds)})')
    for doc in docs:
        own = []
        for obs in doc['observations']:
            if obs.get('envelope_fill'):
                continue
            obs = dict(obs)
            obs['lo'], obs['hi'] = obs.pop('seed_lo', obs['lo']), obs.pop('seed_hi', obs['hi'])
            obs['tolerance'] = obs.pop('seed_tolerance', obs['tolerance'])
            obs.pop('also_ok', None)
            own.append(obs)
        doc['observations'] = own
    keys = {}
    for doc in docs:
        for obs in doc['observations']:
            keys.setdefault((obs['check'], obs['subject']), obs)

    def member(doc, key):
        obs = next((o for o in doc['observations'] if (o['check'], o['subject']) == key), None)
        if obs is None and key[0].startswith(('build.first.', 'attack.first')):
            template = keys[key]
            obs = dict(template, value=NEVER, lo=NEVER, hi=NEVER, tolerance='exact: not within the N64 run',
                       envelope_fill=True)
        return obs

    published = []
    for doc in docs:
        out = []
        horizon = doc['gba']['horizon']
        for key in sorted(keys):
            own = member(doc, key)
            if own is None:
                continue
            own = dict(own, seed_lo=own['lo'], seed_hi=own['hi'], seed_tolerance=own['tolerance'])
            seen = [(o, d['gba']['horizon']) for o, d in ((member(d, key), d) for d in docs) if o is not None]
            if key[0] != 'start.corner':
                # A time later than this seed's run is out of its reach; counts and totals are not times.
                timed = key[0].startswith(('build.first.', 'attack.first', 'result.'))
                finite = [o for o, _ in seen if o['value'] != NEVER and (not timed or o['value'] <= horizon)]
                if finite:
                    first = min(o['value'] for o in finite)
                    own['lo'] = min(o['lo'] for o in finite)
                    own['hi'] = max(o['hi'] for o in finite)
                    if own['value'] == NEVER or any(o['value'] == NEVER and h >= first for o, h in seen):
                        own['also_ok'] = [NEVER]
                values = sorted({o['value'] for o, _ in seen})
                if len(values) > 1:
                    own['tolerance'] = (f'seed envelope over {len(seen)} seeds ({values[0]}-{values[-1]}), '
                                        f'widened by: {own["seed_tolerance"]}')
            out.append(own)
        published.append(out)
    # Every band above came from the seeds' own rows; publish only now.
    for doc, out in zip(docs, published):
        doc['observations'] = out
        doc['envelope'] = {'group': group_of(doc['id']), 'members': sorted(d['id'] for d in docs),
                           'seeds': sorted(seeds)}
    return docs


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    e = sub.add_parser('envelope')
    e.add_argument('--home', type=pathlib.Path)
    s = sub.add_parser('summary')
    s.add_argument('run', type=pathlib.Path)
    args = parser.parse_args(argv)
    sys.path.insert(0, str(ROOT / 'tools'))
    import parity_decode
    if args.command == 'summary':
        cap = parity_decode.Capture(args.run)
        m = measure(n64_frames(cap))
        print('faults', cap.faults, 'verification', cap.verified or 'ok')
        for label, p in sorted(m['players'].items()):
            p = {k: v for k, v in p.items() if k != 'series'}
            print(label, json.dumps(p))
        print('end', m['end'], 'seed', seed_of(cap), 'defeat', m['defeat'])
        return 0
    home = args.home or parity_decode.default_home()
    groups = {}
    for path in sorted((home / 'golden').glob('sk-*.json')):
        groups.setdefault(group_of(path.stem), []).append(path)
    from sc64_parity_tables import check_private, write_private
    for group, paths in sorted(groups.items()):
        # A singleton group too: it drops rows and bands a removed seed left behind.
        docs = envelope(paths)
        check_private(ROOT, paths[0].parent)
        write_private(ROOT, {p: json.dumps(d, indent=1, sort_keys=True) + '\n' for p, d in zip(paths, docs)})
        print(f'{group}: {len(paths)} seeds, {len(docs[0]["observations"])} observations')
    return 0


if __name__ == '__main__':
    sys.exit(main())
