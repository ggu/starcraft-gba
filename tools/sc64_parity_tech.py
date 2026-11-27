#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Tech-tree production labs (N64 parity, phase 2): build, train and add-on times and costs.

An N64 tape (tools/sc64_tape_compile.py `build`/`train` ops, private scenario file) starts a
Blood Bath lobby against one computer, sets the human bank by a guest edit, holds the computer's
bank at zero (guest edits every 200 steps, so it never builds or attacks), and makes every
building, add-on and unit of the human race once. Both engines are measured by the same rules:

  build time   a human building row's birth (status bit clear) to its completion (bit set);
               GBA: a building's birth to its under-construction flag clearing
  train time   a human unit row's birth to its completion; GBA: the producer's queue progress
               reading 1 to the unit's birth (the bb-train rule, for the human's own rows)
  cost         the human bank's debit when the product is paid for, corrected for minerals
               gathered in the same tick: a building or add-on at its birth on the N64 (the worker
               pays when it arrives), a unit when it enters its producer's queue

The GBA run (`match` runner with --tcmd and --starve, tests/sc64_parity_run.c) launches the same
lobby with the N64 seed, sets the same bank, starves the computer over the same steps and issues
each product at the N64 frame it was paid for: a building at the halved site of the N64 building,
a unit or add-on at the human building nearest the halved N64 producer. A product the engine has
no identity or production path for reads absent (a listed GBA defect), never skipped.
"""
import hashlib
import json
import statistics
import struct

import sc64_parity_skirmish as skirmish

# scenario id -> (human race letter, computer races, runner profile)
TECH = {'tech-terran': ('T', ('Z',), 'terran')}
WORKERS = {7: 'T', 41: 'Z', 64: 'P'}
# Subunits born with their parent (turrets) and products that are not made by a command.
SUBUNITS = {4, 6, 18, 31}
ADDONS = {107, 108, 115, 117, 118, 120}   # Comsat, Silo, Control Tower, Covert Ops, Physics Lab, Machine Shop
QUEUE_OFFSET, QUEUE_SLOTS, QUEUE_EMPTY = 0x94, 5, 228
TIME_TOLERANCE = 2
UNIT_DEBIT_WINDOW = 12     # frames after a unit's mark within which its queue debit must fall
RUNNER_PATIENCE = 6000     # PR_TECH_PATIENCE in tests/sc64_parity_run.c
DECODER_REVISION = 1
FIELDS = ['unit.type', 'unit.owner', 'unit.sprite', 'unit.status', 'unit.x', 'unit.y', 'clock.tick_counter',
          'clock.pause_halfword']
COST_FIELDS = ['banks.minerals', 'banks.minerals_gathered', 'clock.tick_counter', 'clock.pause_halfword']


def name_of(raw):
    return skirmish.NAMES.get(raw, f'unit {raw}')


def mark_name(raw):
    return name_of(raw).lower().replace("'", '').replace(' ', '_')


MARK_RAW = {mark_name(raw): raw for raw in skirmish.NAMES}


def mark_product(mark):
    """The product a tape mark names: its name in lower case with underscores, optionally followed
    by digits (supply_depot2), or None for any other mark."""
    return MARK_RAW.get(mark.rstrip('0123456789'))


def observation(check, subject, value, lo, hi, tolerance, gating, fields, unit):
    return {'check': check, 'subject': subject, 'value': value, 'lo': lo, 'hi': hi, 'tolerance': tolerance,
            'gating': gating, 'n_runs': 2, 'provenance': 'n64-staged', 'layout_fields': fields, 'unit': unit}


# ---- N64 ---------------------------------------------------------------------------

def n64_production(cap):
    """The human's products from a capture: births with completion, queue entries and bank debits."""
    records = [r for r in cap.ticks if r['table'] and not r['paused'] and r['sf'] >= 0]
    if not records:
        raise SystemExit('tech: no in-mission frame')
    first = records[0]
    owners = {cap.unit(row, 'owner') for row in first['rows'].values() if cap.unit(row, 'type') in WORKERS}
    humans = {cap.unit(row, 'owner') for row in first['rows'].values()
              if cap.unit(row, 'type') in WORKERS and cap.bank(first, 'minerals', cap.unit(row, 'owner')) >= 5000}
    if len(humans) != 1:
        raise SystemExit(f'tech: expected one worker owner with the edited bank, found {sorted(humans)} of {sorted(owners)}')
    human = humans.pop()
    # A product is a new (row, type): a Refinery takes its geyser's row.
    present = {(i, cap.unit(row, 'type')) for i, row in first['rows'].items()}
    born, products, queued = {}, [], []
    queues = {}
    debits = []
    prev = None
    for record in records:
        rows = record['rows']
        present &= {(i, cap.unit(row, 'type')) for i, row in rows.items()}
        for key in [k for k in born if k[0] not in rows]:
            born.pop(key)   # a row freed before completing: no product
        for index, row in rows.items():
            typ, done = cap.unit(row, 'type'), cap.unit(row, 'status') >> 31
            if cap.unit(row, 'owner') != human:
                continue
            if typ >= 106 and done:
                # Queue entries of a finished building, the start's Command Center included
                # (candidate field: commands only, never a check).
                slots = struct.unpack('>5H', bytes(row[QUEUE_OFFSET:QUEUE_OFFSET + 2 * QUEUE_SLOTS]))
                before = queues.get(index)
                if before is not None:
                    for k in range(QUEUE_SLOTS):
                        if slots[k] != QUEUE_EMPTY and before[k] != slots[k]:
                            queued.append({'raw': slots[k], 'sf': record['sf'], 'x': cap.unit(row, 'x'),
                                           'y': cap.unit(row, 'y')})
                queues[index] = slots
            if (index, typ) in present:
                continue
            key = (index, typ)
            if key not in born and not done:
                born[key] = (record['sf'], cap.unit(row, 'x'), cap.unit(row, 'y'))
            elif key in born and born[key] is not None and done:
                sf0, x, y = born[key]
                products.append({'raw': typ, 'born': sf0, 'done': record['sf'], 'x': x, 'y': y})
                born[key] = None
        minerals = cap.bank(record, 'minerals', human)
        gathered = cap.bank(record, 'minerals_gathered', human)
        gas = cap.bank(record, 'gas', human, gating=False)
        gas_gathered = cap.bank(record, 'gas_gathered', human, gating=False)
        if prev is not None:
            dm = prev[0] - minerals + gathered - prev[1]
            dg = prev[2] - gas + gas_gathered - prev[3]
            # Debits are found on the proven mineral fields alone; the gas change of the same
            # tick is read beside it for the report.
            if dm > 0:
                debits.append((record['sf'], dm, dg))
        prev = (minerals, gathered, gas, gas_gathered)
    start_buildings = [{'raw': cap.unit(row, 'type'), 'born': 0, 'done': 0, 'x': cap.unit(row, 'x'), 'y': cap.unit(row, 'y')}
                       for row in first['rows'].values()
                       if cap.unit(row, 'owner') == human and cap.unit(row, 'type') >= 106]
    return {'human': human, 'products': products, 'queued': queued, 'debits': debits, 'start_buildings': start_buildings,
            'last': records[-1]['sf']}


def poke_window(cap):
    """Source frames of the first and last guest edit of the run (events.csv), on the capture's clock."""
    events = cap.run / 'events.csv'
    manifest = json.loads((cap.run / 'manifest.json').read_text())
    want = manifest.get('outputs_sha256', {}).get('events.csv')
    data = events.read_bytes()   # hashed and parsed from the same bytes
    if want is None or hashlib.sha256(data).hexdigest() != want:
        raise SystemExit('tech: events.csv does not match its run manifest')
    steps = []
    for line in data.decode().splitlines()[1:]:
        parts = line.split(',')
        if len(parts) > 1 and parts[1] == 'poke':
            steps.append(int(parts[0]))
    if not steps:
        raise SystemExit('tech: the capture made no guest edit; the tape must set the bank')
    sf = lambda step: max((r['sf'] for r in cap.ticks if r['table'] and r['sf'] >= 0 and r['step'] <= step), default=0)
    return sf(min(steps)), sf(max(steps))


def debit_at(debits, sf, slack=0):
    found = [d for d in debits if sf <= d[0] <= sf + slack]
    return found[0] if len(found) == 1 else None


def make_measure_n64(scenario_id):
    human_race, cpus, profile = TECH[scenario_id]

    def measure_n64(cap, marks):
        m = n64_production(cap)
        seed = skirmish.seed_of(cap)
        if seed is None:
            raise SystemExit(f'{scenario_id}: no seed word in the capture')
        start, end = poke_window(cap)
        obs, cmds, notes = [], [], []
        durations = {}
        for p in m['products']:
            if p['raw'] in SUBUNITS:
                continue
            durations.setdefault(p['raw'], []).append(p['done'] - p['born'])
        for raw in sorted(durations):
            check = 'tech.build_time' if raw >= 106 else 'tech.train_time'
            value = statistics.median(durations[raw])
            obs.append(observation(check, name_of(raw), value, value - TIME_TOLERANCE, value + TIME_TOLERANCE,
                                   f'+-{TIME_TOLERANCE} source frames', True, FIELDS,
                                   f'source frames from birth to completion; N64 {durations[raw]}'))
        # Costs, from the tape's marks: each product's command is marked with its name (the
        # mark_product rule), and the tape stopped unless the press showed its evidence (a
        # worker's PlaceBuilding order, or the queue slot holding exactly that unit). A building
        # or add-on pays at its birth after its mark, a unit at the first debit after its mark.
        # A debit any other claim could also take is no evidence for either.
        frame_of = lambda step: max((r['sf'] for r in cap.ticks if r['table'] and r['sf'] >= 0 and r['step'] <= step),
                                    default=None)
        claims = []
        for mark, step in sorted(marks.items(), key=lambda kv: kv[1]):
            raw = mark_product(mark)
            t0 = frame_of(step)
            if raw is None or t0 is None:
                continue
            if raw >= 106:
                born = [p['born'] for p in m['products'] if p['raw'] == raw and p['born'] >= t0]
                window = (born[0], born[0]) if born else None
            else:
                window = (t0 + 1, t0 + UNIT_DEBIT_WINDOW)
            claims.append((mark, raw, window))
        for mark, raw, window in claims:
            found = [d for d in m['debits'] if window and window[0] <= d[0] <= window[1]]
            found = found[:1] if raw < 106 else found
            shared = found and any(w and w[0] <= found[0][0] <= w[1] for other, _, w in claims if other != mark)
            if any(o['check'] == 'tech.cost' and o['subject'] == name_of(raw) for o in obs):
                continue   # one cost line per product: a repeat (supply_depot2) adds nothing
            if len(found) != 1 or shared:
                notes.append(f'{mark}: no debit only it can claim')
                continue
            debit = found[0]
            obs.append(observation('tech.cost', name_of(raw), debit[1], debit[1], debit[1], 'exact', True,
                                   COST_FIELDS, f'minerals debited after mark {mark}, corrected for minerals gathered'))
            obs.append(observation('tech.gas_cost', name_of(raw), debit[2], debit[2], debit[2], 'exact', False,
                                   ['banks.gas', 'banks.gas_gathered'],
                                   'gas debited (report only: the gas bank is a candidate field)'))
        # GBA commands at the frame each product was paid for (steps = frames x 5/4).
        step_of = lambda sf: max(2, (sf * 5 + 3) // 4)
        cmds.append([2, 0, 0, 10000, 10000])
        for p in m['products']:
            if p['raw'] in SUBUNITS or p['raw'] < 106:
                continue
            if p['raw'] in ADDONS:
                parent = nearest_building(m, p)
                cmds.append([step_of(p['born']), 3, p['raw'], parent[0] // 2, parent[1] // 2])
            else:
                cmds.append([step_of(p['born']), 1, p['raw'], p['x'] // 2, p['y'] // 2])
        for q in m['queued']:
            cmds.append([step_of(q['sf']), 2, q['raw'], q['x'] // 2, q['y'] // 2])
        cmds.sort()
        # Long enough for every command to get its whole patience (tests/sc64_parity_run.c).
        gba = {'scenario': 'match', 'profile': profile, 'at_step': 5, 'seed': seed,
               'steps': max(step_of(m['last']) + 400, max(c[0] for c in cmds) + RUNNER_PATIENCE + 10), 'tech_cmds': cmds}
        # The computer stays starved from the N64's first edit to the end of the replay, the
        # retry tail included, so it never spends while a late product is measured.
        gba['starve'] = [step_of(start), gba['steps'] + 1]
        notes.append(f'human owner {m["human"]}; {len(m["products"])} products, {len(m["queued"])} queue entries')
        return {'observations': obs, 'gba': gba, 'notes': notes,
                'measured': {'products': m['products'], 'queued': m['queued'], 'debits': m['debits']}}
    return measure_n64


ADDON_OFFSET = (96, 16)   # an add-on's centre from its parent's (N64 px)


def nearest_building(m, product):
    """An add-on's parent: the finished human building nearest the add-on's centre less ADDON_OFFSET."""
    best = None
    ax, ay = product['x'] - ADDON_OFFSET[0], product['y'] - ADDON_OFFSET[1]
    for p in m['products'] + m['start_buildings']:
        if p['raw'] < 106 or p['raw'] in ADDONS or p['done'] > product['born']:
            continue
        d = (p['x'] - ax) ** 2 + (p['y'] - ay) ** 2
        if best is None or d < best[0]:
            best = (d, p['x'], p['y'])
    if best is None:
        raise SystemExit(f'tech: no finished building next to the add-on {name_of(product["raw"])}')
    return best[1], best[2]


# ---- GBA ---------------------------------------------------------------------------

def gba_args(scenario_id, params, trace):
    human, cpus, _ = TECH[scenario_id]
    args = ['--human', human.lower()]
    for race in cpus:
        args += ['--bot', race.lower()]
    args += ['--seed', str(int(params['seed'])), '--gather', str(trace) + '.gather',
             # Starved to the end of the run whatever cutoff an older golden saved.
             '--starve', f'{int(params["starve"][0])},{int(params["steps"]) + 1}']
    for cmd in params['tech_cmds']:
        args += ['--tcmd', ','.join(str(int(v)) for v in cmd)]
    return args


def gba_series(trace, report):
    return {'source': 'gba', 'trace': str(trace), 'gather': str(trace) + '.gather', 'report': report}


def read_gba(path, gather_path, human, unit_raw, building_raw):
    """Births and completions of the human's products and per-step bank debits.

    Rows carry engine types: the runner's unit_raw and building_raw maps give cartridge ids.
    A unit's training starts when its producer's queue progress reads 1 with it at the head, and
    ends at the step that producer completes its head (progress restarts, the head changes or the
    queue shrinks) and a human unit of that type is born; a birth that more than one producer, or
    none, could have made in that step is left out (the bb-train rule). The runner's gather log
    is required: a debit is corrected by the minerals and gas gathered in the same step."""
    braw = lambda t: building_raw[t] if 0 <= t < len(building_raw) else -1
    uraw = lambda t: unit_raw[t] if 0 <= t < len(unit_raw) else -1
    done, train, debits = [], [], {}
    bank, step = {}, 0
    state, start, building = {}, {}, {}
    completed, births = {}, {}
    with open(path) as handle:
        for line in handle:
            w = line.split()
            if not w:
                continue
            tag = w[0]
            if tag == 'S':
                step = int(w[1])
            elif tag == '+B' and int(w[3]) == human and step > 1:
                building[int(w[1])] = step
            elif tag == 'B':
                slot, raw, owner, uc = int(w[1]), braw(int(w[2])), int(w[4]), int(w[11])
                if owner != human:
                    continue
                if slot in building and not uc:
                    done.append({'raw': raw, 'born': building.pop(slot), 'done': step})
                count, progress = int(w[14]), int(w[15])
                head = uraw(int(w[16])) if count else -1
                before = state.get(slot)
                if before and before[0] and slot in start and (progress < before[1] or head != before[2] or
                                                                 count < before[0]):
                    completed.setdefault(step, []).append((slot, before[2], start.pop(slot)))
                if count and progress == 1:
                    start[slot] = step
                elif not count:
                    start.pop(slot, None)
                state[slot] = (count, progress, head)
            elif tag == 'U' and int(w[4]) == human and int(w[1]) in births.get(step, {}):
                births[step][int(w[1])] = uraw(int(w[2]))
            elif tag == '+U' and int(w[3]) == human and step > 1:
                births.setdefault(step, {})[int(w[1])] = None
            elif tag == 'P' and int(w[1]) == human:
                bank[step] = (int(w[2]), int(w[3]))
    for s in sorted(births):
        for slot, raw in births[s].items():
            makers = [c for c in completed.get(s, []) if c[1] == raw]
            if raw is None or len(makers) != 1:
                continue
            train.append({'raw': raw, 'start': makers[0][2], 'born': s})
            completed[s].remove(makers[0])
    gathered = {}
    with open(gather_path) as handle:   # required: no correction is no measurement
        for line in handle:
            if line.startswith('#'):
                continue
            s, owner, g_min, g_gas = (int(v) for v in line.split())
            if owner == human:
                gathered[s] = (g_min, g_gas)
    prev, prev_g = None, (0, 0)
    for s in sorted(set(bank) | set(gathered)):
        cur = bank.get(s, prev)
        g = gathered.get(s, prev_g)
        if prev is not None and cur is not None:
            dm = prev[0] - cur[0] + g[0] - prev_g[0]
            dg = prev[1] - cur[1] + g[1] - prev_g[1]
            if dm > 0:   # as on the N64: found by minerals, gas read beside
                debits[s] = (dm, dg)
        prev, prev_g = cur, g
    return {'done': done, 'train': train, 'debits': debits}


def gba_costs(tech, debits):
    """{raw: (minerals, gas)} for commands with a debit only they can claim: one debit step within a
    step of the step the command was taken (a worker's building: the step its foundation was
    seen), and no other taken command's window holding it."""
    windows = [(c['raw'], c['issued'] - 1, c['issued'] + 1) for c in tech if c['state'] == 1 and c['kind']]
    cost = {}
    for k, (raw, lo, hi) in enumerate(windows):
        found = [s for s in sorted(debits) if lo <= s <= hi]
        if len(found) != 1:
            continue
        if any(o_lo <= found[0] <= o_hi for j, (_, o_lo, o_hi) in enumerate(windows) if j != k):
            continue
        cost.setdefault(raw, debits[found[0]])
    return cost


def make_measure_gba(scenario_id):
    def measure_gba(series, golden, report):
        match = report.get('match')
        tech = report.get('tech')
        if not match or tech is None:
            return None, 'the runner report has no match or tech block'
        human = next((o['owner'] for o in match['owners'] if o['human']), None)
        unit_raw, building_raw = report.get('unit_raw'), report.get('building_raw')
        if not unit_raw or not building_raw:
            return None, 'the runner reported no engine-to-cartridge id maps'
        try:
            g = read_gba(series['trace'], series['gather'], human, unit_raw, building_raw)
        except FileNotFoundError as error:
            return None, f'no gather log: {error}'
        values, absent, refused = {}, [], []
        absent_raw = {c['raw'] for c in tech if c['state'] == 2}
        refused_raw = {c['raw'] for c in tech if c['state'] == 3} - absent_raw
        train, build = {}, {}
        for t in g['train']:
            train.setdefault(t['raw'], []).append((t['born'] - t['start'] + 1) * 4 / 5)
        for d in g['done']:
            build.setdefault(d['raw'], []).append((d['done'] - d['born']) * 4 / 5)
        cost = gba_costs(tech, g['debits'])
        for obs in golden['observations']:
            raw = next((r for r, n in skirmish.NAMES.items() if n == obs['subject']), None)
            key = (obs['check'], obs['subject'])
            table = {'tech.build_time': build, 'tech.train_time': train}.get(obs['check'])
            if table is not None and raw in table:
                values[key] = statistics.median(table[raw])
            elif obs['check'] == 'tech.cost' and raw in cost:
                values[key] = cost[raw][0]
            elif obs['check'] == 'tech.gas_cost' and raw in cost:
                values[key] = cost[raw][1]
            elif raw in absent_raw:
                # No engine identity or production path: the absent marker (a listed defect).
                values[key] = skirmish_absent()
                absent.append(obs['subject'])
            elif raw in refused_raw:
                values[key] = refused_marker()
                refused.append(obs['subject'])
            # Otherwise (made but not timed, a debit not uniquely its own, or a command the run's
            # end cut short) the check has no value: not measured.
        return {'values': values, 'measured': {'train': train, 'build': build, 'cost': {str(k): v for k, v in cost.items()},
                                               'absent': sorted(set(absent)), 'refused': sorted(set(refused)),
                                               'tech': tech}}, None
    return measure_gba


def skirmish_absent():
    import sc64_parity_metrics as metrics
    return metrics.ABSENT


def refused_marker():
    import sc64_parity_metrics as metrics
    return metrics.REFUSED


def scenarios():
    out = {}
    for scenario_id, (human, cpus, profile) in TECH.items():
        out[scenario_id] = {
            'title': f'Blood Bath tech tree: human {skirmish.RACE_NAME[human]} builds and trains every product '
                     f'(bank edited, computer {"/".join(skirmish.RACE_NAME[c] for c in cpus)} starved)',
            'measure_n64': make_measure_n64(scenario_id), 'measure_gba': make_measure_gba(scenario_id),
            'profile': profile, 'runner': 'match', 'gba_args': gba_args, 'gba_series': gba_series,
            'decoder_revision': DECODER_REVISION,
        }
    return out
