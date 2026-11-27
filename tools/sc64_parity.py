#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""N64 parity comparator and developer loop (`./dev parity`).

Runs the ROM-table differ (tools/sc64_parity_tables.py) and every scenario
that has a golden file under $SC64_PARITY_HOME/golden, in parallel. Each
scenario runs its GBA host binary (build/parity/<profile>-fast-trace, from
mk/parity/Makefile) with the parameters its golden file gives, measures the
trace with the same code that measured the N64 capture
(tools/sc64_parity_metrics.py), and judges every observation:

  PASS        inside the N64 band
  KNOWN       outside the band, listed in the baseline, and not further out
              than the baseline recorded
  REGRESSION  outside the band and not in the baseline, or further out than
              the baseline recorded; fails the run
  STALE-GOLDEN  the golden was decoded against other definitions of the
              layout fields it reads (offsets, types, names; evidence and
              status edits do not count); fails the run until the golden is
              regenerated with tools/parity_decode.py golden, or reports with
              --allow-stale (recording work only)
  IMPROVED    in the baseline but now inside the band (notice)
  STALE       a baseline line whose observation no longer exists (notice)
  REPORT      a non-gating observation (never fails)
  SKIP        a scenario that could not run because private inputs are absent

`--accept` rewrites the baseline (tools/parity/baseline.json) from this run:
every gating observation outside its band becomes a KNOWN line with its GBA
value; IMPROVED and STALE lines are dropped. The baseline holds only GBA
values and verdict keys; N64 numbers stay in the private fixture home.

The scorecard prints, per observation, the first divergent source frame (for
checks with an N64 curve), the band, the GBA value and the provenance, then the
fixture-set hash: a SHA-256 over the golden files it judged against. A run
that skips everything while .local/sc64 exists exits non-zero, so a broken
private setup never reads as a pass.
"""
import argparse
import concurrent.futures
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import parity_decode  # noqa: E402
import sc64_parity_metrics as metrics  # noqa: E402

BASELINE = ROOT / 'tools' / 'parity' / 'baseline.json'
PARITY_DIR = ROOT / 'build' / 'parity'
FAILING = {'REGRESSION', 'STALE-GOLDEN'}
STALE_WHY = ('golden decoded against other definitions of the layout fields it reads; '
             'regenerate it with tools/parity_decode.py golden')


def default_home():
    env = os.environ.get('SC64_PARITY_HOME')
    if env:
        return pathlib.Path(env)
    out = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', '--path-format=absolute', '--git-common-dir'],
                         capture_output=True, text=True)
    base = pathlib.Path(out.stdout.strip()).parent if out.returncode == 0 and out.stdout.strip() else ROOT
    return base / '.local' / 'parity'


# ---- GBA trace --------------------------------------------------------------

def gba_series(trace_path):
    """Normalise a sc64-parity-trace/1 file: last row per source frame, N64 pixels.

    Also kept, for the field-map checks: each unit slot's order changes
    (series['orders'][slot] = [(sf, engine order)]), stride phase and animation kind
    changes (series['walks'][slot] = [(sf, walk, anim)]) and the human fog sets
    (series['fog'] = {'cols', 'rows', 'keyframe': (sf, visible, explored), 'changes':
    [(sf, cell, visible, explored)]}, cells col + row * cols)."""
    units, hp, shield, etype, banks, frames = {}, {}, {}, {}, {}, []
    live = {}        # slot -> live engine owner: a unit that changed sides keeps its cartridge owner
    buildings = {}   # slot -> (owner, x, y in N64 px, hp, shields, max hp, engine type, cartridge type
    #                  or -1, under construction, live engine owner), for the labs' enemy checks
    # Every removal with the actor's last owner, position and hit points, read before its row is
    # dropped: a unit or building born and removed inside one source frame never shows in frames.
    hits = []        # (step, target, damage, source engine type, source owner): weapon hits as the damage
    #                  path saw them; target is a unit slot or max_units + a building slot
    max_units = None
    removals = []    # (kind 'U' or 'B', step, slot, owner, x, y, hp at removal, max hp or None, last row's hp,
    #                  cartridge type (-1 when the row has none), engine type, step of the last row or None,
    #                  under construction at the last row or None)
    building_step = {}   # slot -> step of its last building row
    orders, walks, fog = {}, {}, {'cols': None, 'rows': None, 'keyframe': None, 'changes': []}
    keyframe = {}
    # Births and building rows with engine type ids, for the passive train and build lab
    # (the runner reports each profile's engine -> cartridge id maps).
    events = []
    step, complete = None, False

    def flush():
        if step is None:
            return
        sf = step * 4 // 5
        frame = {'sf': sf, 'step': step, 'units': dict(units), 'hp': dict(hp), 'shield': dict(shield),
                 'etype': dict(etype), 'live': dict(live), 'buildings': dict(buildings),
                 'banks': {o: (v, None) for o, v in banks.items()}}
        if frames and frames[-1]['sf'] == sf:
            frames[-1] = frame
        else:
            frames.append(frame)

    with open(trace_path) as handle:
        first = handle.readline()
        if not first.startswith('#sc64-parity-trace 1'):
            raise ValueError(f'{trace_path}: not a sc64-parity-trace/1 file')
        for line in handle:
            words = line.split()
            if not words:
                continue
            tag = words[0]
            if tag == 'S':
                flush()
                step = int(words[1])
            elif tag == 'U':
                # The cartridge's ids: raw_type, and raw_owner where the profile has one.
                slot, raw_type, owner, raw_owner = int(words[1]), int(words[3]), int(words[4]), int(words[5])
                x, y = int(words[6]), int(words[7])
                units[slot] = (raw_type, raw_owner if raw_owner >= 0 else owner, 2 * x, 2 * y)
                order = int(words[12]) if len(words) > 12 else None
                history = orders.setdefault(slot, [])
                if order is not None and (not history or history[-1][1] != order):
                    history.append(((step or 0) * 4 // 5, order))
                if len(words) > 23:
                    walk = (int(words[22]), int(words[23]))   # stride phase, animation kind
                    strides = walks.setdefault(slot, [])
                    if not strides or strides[-1][1:] != walk:
                        strides.append(((step or 0) * 4 // 5,) + walk)
                hp[slot] = float(int(words[8]))
                shield[slot] = float(max(0, int(words[10])))
                etype[slot] = int(words[2])   # engine type: the runner's unit_raw map gives its cartridge id
                live[slot] = owner
            elif tag == '-U':
                gone = units.get(int(words[1]))
                if gone is not None:
                    removals.append(('U', step, int(words[1]), gone[1], gone[2], gone[3], int(words[4]), None,
                                     hp.get(int(words[1])), gone[0], int(words[2]), None, None))
                units.pop(int(words[1]), None)
                hp.pop(int(words[1]), None)
                shield.pop(int(words[1]), None)
                etype.pop(int(words[1]), None)
                live.pop(int(words[1]), None)
            elif tag in ('OU', 'TU') and len(words) == 4:
                # An owner or type change in place: a slot refilled inside one engine step shows so.
                events.append((tag, step, int(words[1]), int(words[2]), int(words[3])))
            elif tag in ('+U', '+B', '-B'):
                events.append((tag, step, int(words[1]), int(words[2]), int(words[3])))
                if tag == '-B':
                    gone = buildings.pop(int(words[1]), None)
                    if gone is not None:
                        removals.append(('B', step, int(words[1]), gone[0], gone[1], gone[2],
                                         int(words[4]) if len(words) > 4 else gone[3], gone[5], gone[3],
                                         gone[7], int(words[2]), building_step.get(int(words[1])), gone[8]))
            elif tag == 'B':
                raw_owner = int(words[5])
                building_step[int(words[1])] = step
                buildings[int(words[1])] = (raw_owner if raw_owner >= 0 else int(words[4]),
                                            2 * int(words[6]), 2 * int(words[7]),
                                            int(words[8]), max(0, int(words[10])), int(words[9]),
                                            int(words[2]), int(words[3]), int(words[11]), int(words[4]))
                # slot, engine type, owner, under construction, queue count, queue progress, queue head
                events.append(('B', step, int(words[1]), int(words[2]), int(words[4]), int(words[11]),
                               int(words[14]), int(words[15]), int(words[16])))
            elif tag == 'D' and len(words) == 5:
                hits.append((step, int(words[1]), int(words[2]), int(words[3]), int(words[4])))
            elif tag == 'H':
                fields = dict(w.split('=', 1) for w in words[1:] if '=' in w)
                if 'max_units' in fields:
                    max_units = int(fields['max_units'])
            elif tag == 'P':
                banks[int(words[1])] = int(words[2])
            elif tag in ('FV', 'FE') and len(words) == 4:
                cols, rows, bits = int(words[1]), int(words[2]), bytes.fromhex(words[3])
                cells = {c for c in range(cols * rows) if bits[c >> 3] >> (c & 7) & 1}
                fog['cols'], fog['rows'] = cols, rows
                keyframe[tag] = (step, cells)
                if tag == 'FE' and keyframe.get('FV', (None,))[0] == step:
                    # A new keyframe (first step, or a new map size) restarts the change list.
                    fog['keyframe'] = ((step or 0) * 4 // 5, keyframe['FV'][1], cells)
                    fog['changes'] = []
            elif tag == 'F' and len(words) == 4:
                fog['changes'].append(((step or 0) * 4 // 5, int(words[1]), int(words[2]), int(words[3])))
            elif tag == 'Z':
                fields = dict(w.split('=', 1) for w in words[1:] if '=' in w)
                complete = fields.get('overflow') == '0' and fields.get('write_errors') == '0'
    flush()
    if not complete:
        raise ValueError(f'{trace_path}: incomplete trace (no clean Z line)')
    return {'source': 'gba', 'scale': 2, 'frames': frames, 'orders': orders, 'walks': walks, 'fog': fog,
            'events': events, 'removals': removals, 'hits': hits, 'max_units': max_units}


def gba_command(scenario_id, golden, trace):
    """The runner command. Binary and runner scenario come from SCENARIOS, never
    from the golden file; the golden supplies only integers."""
    spec = metrics.SCENARIOS[scenario_id]
    params = golden['gba']
    binary = PARITY_DIR / f'{spec["profile"]}-fast-trace'
    if spec['runner'] == 'census':
        # A campaign opening: menu launch of one catalog mission, then input-free steps.
        return binary, [str(binary), '--scenario', 'census', '--mission', str(int(params['mission'])),
                        '--steps', str(int(params['steps'])), '--trace', str(trace), '--no-route-checks',
                        '--profile', spec['profile']]
    ints = [int(params['steps']), int(params['at_step'])]
    cmd = [str(binary), '--scenario', spec['runner'], '--steps', str(ints[0]), '--trace', str(trace),
           '--no-route-checks', '--profile', spec['profile'], '--at', str(ints[1])]
    for x, y in params.get('units', []):
        cmd += ['--unit', f'{int(x)},{int(y)}']
    if 'target' in params:
        cmd += ['--target', f'{int(params["target"][0])},{int(params["target"][1])}']
    if 'gba_args' in spec:
        cmd += spec['gba_args'](scenario_id, params, trace)
    # Unit-class labs: the subject (an exact point, or a cartridge unit id) and timed orders,
    # passed only to the lab runners that take them.
    if not spec['runner'].startswith('lab-'):
        return binary, cmd
    if 'subject' in params:
        cmd += ['--subject', f'{int(params["subject"][0])},{int(params["subject"][1])}']
    if spec['runner'] == 'lab-mission':
        cmd += ['--mission', str(int(params['mission']))]
    if 'subject_raw' in params:
        cmd += ['--subject-raw', str(int(params['subject_raw']))]
    for entry in params.get('cmds', []):
        cmd += ['--cmd', ','.join(str(int(v)) for v in entry[:4])]
    return binary, cmd


# ---- judging ------------------------------------------------------------------

def outside(value, lo, hi):
    """How far a value lies outside [lo, hi]; 0 inside."""
    return lo - value if value < lo else value - hi if value > hi else 0


def first_divergence(n64, gba, tolerance):
    for k, (a, b) in enumerate(zip(n64, gba)):
        if abs(a - b) > tolerance:
            return k
    if len(gba) < len(n64):
        return len(gba)
    return None


def stale(obs, golden, layout):
    """True when the layout no longer decodes this observation's fields as the golden did:
    its per-field digest differs, or (goldens from before per-field digests) the whole layout
    changed."""
    if layout is None:
        return False
    want = obs.get('layout_fields_sha256')
    if any(k.startswith('pool.') for k in obs.get('layout_reads', []) + obs.get('layout_fields', [])) and \
            golden.get('bundle', {}).get('chain_format') != 2:
        return True   # extra-region decoding needs the chain that binds regions and table offsets
    if want is None or 'layout_reads' not in obs or (obs.get('gating') and 'layout_proven' not in obs):
        # Before per-field digests, or before they covered every read: the whole layout must match.
        return golden.get('layout_sha256') != layout.sha256
    return want != layout.fields_digest(obs.get('layout_reads', obs.get('layout_fields', [])))


def judge(scenario_id, golden, values, curves, baseline, layout=None, allow_stale=False):
    rows = []
    revision = metrics.SCENARIOS.get(scenario_id, {}).get('decoder_revision', 1)
    if not golden.get('observations'):
        # A golden that checks nothing never passes quietly.
        return [{'key': f'{scenario_id}:golden:-', 'check': 'golden', 'subject': '-', 'n64': None, 'gba': None,
                 'provenance': '-', 'gating': False, 'n_runs': 0, 'tolerance': '-',
                 'verdict': 'REPORT' if allow_stale else 'STALE-GOLDEN', 'stale': True,
                 'why': 'the golden has no observations; ' + STALE_WHY}]
    if metrics.SCENARIOS.get(scenario_id, {}).get('require_gating') and \
            not any(obs['gating'] for obs in golden['observations']):
        # A lab whose golden holds report-only rows would pass without checking parity.
        return [{'key': f'{scenario_id}:golden:-', 'check': 'golden', 'subject': '-', 'n64': None, 'gba': None,
                 'provenance': '-', 'gating': True, 'n_runs': 0, 'tolerance': '-', 'verdict': 'REGRESSION',
                 'why': 'the golden has no gating check; re-record the lab'}]
    for obs in golden['observations']:
        gating = obs['gating']
        # A scenario with several subjects per check keys its values by (check, subject).
        value = values.get((obs['check'], obs['subject']), values.get(obs['check']))
        unproven = []
        if gating and layout is not None:
            unproven = [k for k in obs.get('layout_proven', obs.get('layout_fields', [])) if layout.status(k) != 'proven']
            if layout.doc['unit_table'].get('status') != 'proven':
                unproven.append('unit_table')
        # A golden measured by an older revision of its scenario's decoder is stale too.
        old_decoder = golden.get('decoder_revision', 1) != revision
        if stale(obs, golden, layout) or unproven or old_decoder:
            key = f'{scenario_id}:{obs["check"]}:{obs["subject"]}'
            row = {'key': key, 'check': obs['check'], 'subject': obs['subject'], 'lo': obs['lo'], 'hi': obs['hi'],
                   'n64': obs['value'], 'gba': value, 'provenance': provenance(obs, golden),
                   'gating': False, 'n_runs': obs['n_runs'], 'tolerance': obs['tolerance']}
            # A stale golden fails unless recording work asks for it (--allow-stale); either way it never gates.
            # So does a gating check whose fields the layout no longer proves.
            row['verdict'] = 'REPORT' if allow_stale else 'STALE-GOLDEN'
            row['stale'] = True   # --accept refuses any run with a stale row, --allow-stale or not
            row['why'] = STALE_WHY + (f'; the layout no longer proves {", ".join(unproven)}' if unproven else '') + \
                (f'; measured by decoder revision {golden.get("decoder_revision", 1)}, the scenario is at {revision}'
                 if old_decoder else '') + (' (--allow-stale)' if allow_stale else '')
            rows.append(row)
            continue
        key = f'{scenario_id}:{obs["check"]}:{obs["subject"]}'
        row = {'key': key, 'check': obs['check'], 'subject': obs['subject'], 'lo': obs['lo'], 'hi': obs['hi'],
               'n64': obs['value'], 'gba': value, 'provenance': provenance(obs, golden), 'gating': obs['gating'],
               'n_runs': obs['n_runs'], 'tolerance': obs['tolerance'], 'tags': golden_tags(obs, golden)}
        if value is None:
            row['verdict'] = 'REGRESSION' if gating else 'REPORT'
            row['why'] = 'not measured on the GBA' if obs['value'] is not None else 'recorded on neither engine'
            rows.append(row)
            continue
        if isinstance(value, float):
            row['gba'] = round(value, 4)
        if obs['value'] is None or obs['lo'] is None or obs['hi'] is None:
            # An event only the GBA recorded (a report-only observation the N64 never saw) has no band.
            row['verdict'] = 'REGRESSION' if gating else 'REPORT'
            row['why'] = 'recorded on the GBA only; the N64 capture has no such event'
            rows.append(row)
            continue
        if obs['value'] and value not in metrics.MARKERS:
            row['ratio'] = round(value / obs['value'], 3)
        gap = outside(value, obs['lo'], obs['hi'])
        if value in obs.get('also_ok', ()):
            gap = 0  # an outcome outside the numeric band that the N64 also showed (skirmish NEVER)
        known = baseline.get(key)
        if not gating:
            row['verdict'] = 'REPORT'
        elif gap == 0:
            row['verdict'] = 'IMPROVED' if known else 'PASS'
        elif known is None:
            row['verdict'] = 'REGRESSION'
            row['why'] = 'outside the band and not in the baseline'
        elif (value in metrics.MARKERS or known['gba'] in metrics.MARKERS) and value != known['gba']:
            # An event the GBA now produces, or no longer produces, is never the known gap, and
            # neither is a fight with other enemies where the baseline recorded none (or the reverse).
            row['verdict'] = 'REGRESSION'
            row['why'] = {metrics.ABSENT: 'the event is absent on the GBA; the baseline holds ' +
                          ('a fight with other enemies' if known['gba'] == metrics.OFF_TARGET else 'a measurement'),
                          metrics.OFF_TARGET: 'the GBA subject fought only other enemies; the baseline holds ' +
                          ('no fight' if known['gba'] == metrics.ABSENT else 'a measurement')}.get(
                value, 'the baseline records the event as ' +
                ('absent' if known['gba'] == metrics.ABSENT else 'a fight with other enemies') +
                ' on the GBA; it is now measured outside the band')
        elif value != known['gba'] and (value in obs.get('sentinels', ()) or known['gba'] in obs.get('sentinels', ())):
            # A sentinel (skirmish NEVER, ENDED) is an outcome, not a number: changing to or from one is
            # never within the baseline's slack.
            row['verdict'] = 'REGRESSION'
            row['why'] = f'the outcome changed from the baseline ({known["gba"]})'
        else:
            was = 0 if known['gba'] in obs.get('also_ok', ()) else outside(known['gba'], obs['lo'], obs['hi'])
            # The slack is the measurement's own tolerance: a seed envelope (skirmish) keeps each
            # seed's band as seed_lo/seed_hi, and the N64's spread over seeds is no licence for
            # the GBA to move further out.
            slack = max((obs.get('seed_hi', obs['hi']) - obs.get('seed_lo', obs['lo'])) / 2, 1e-9)
            if gap > was + slack:
                row['verdict'] = 'REGRESSION'
                row['why'] = f'further outside the band than the baseline ({known["gba"]})'
            else:
                row['verdict'] = 'KNOWN'
                row['baseline'] = known['gba']
        for name, curve in golden.get('curves', {}).items():
            # A curve belongs to the checks it explains: arrival for the move, rate for income.
            explains = {'move.remaining': ('move.arrival_first', 'move.arrival_last'),
                        'income.gathered': ('income.rate', 'income.first_delivery')}.get(name, ())
            if 'explains' in curve:  # [[check, subject], ...], written with the curve
                explains = [c for c, subject in curve['explains'] if subject == obs['subject']]
            if obs['check'] in explains and name in curves:
                k = first_divergence(curve['n64'], curves[name], curve['tolerance'])
                if k is not None:
                    k *= curve.get('step', 1)  # curves sampled every `step` source frames
                row['first_divergent_frame'] = k
                row['curve'] = name
        rows.append(row)
    return rows


def golden_tags(obs, golden):
    """Tags an observation carries: its own and its bundle's (n64-staged: the captures wrote
    guest memory, owner decision 2026-11-26)."""
    return sorted(set(obs.get('tags', [])) | set(golden.get('tags', [])))


def provenance(obs, golden):
    tags = golden_tags(obs, golden)
    return obs['provenance'] + ''.join(f' {t}' for t in tags)


def run_scenario(scenario_id, golden_path, baseline, keep, allow_stale=False):
    golden = json.loads(golden_path.read_text())
    spec = metrics.SCENARIOS.get(scenario_id)
    result = {'id': scenario_id, 'golden': str(golden_path), 'golden_sha256': hashlib.sha256(golden_path.read_bytes()).hexdigest()}
    if spec is None:
        return {**result, 'status': 'SKIP', 'why': 'no measurement code for this scenario id'}
    if not all(metrics.gba_boundary_ok(int(step)) for step in
               [golden['gba']['at_step']] + [entry[0] for entry in golden['gba'].get('cmds', [])]):
        return {**result, 'status': 'ERROR', 'why': f'GBA command step {golden["gba"]["at_step"]} shares a source frame '
                'with its pre-command step'}
    with tempfile.TemporaryDirectory(prefix=f'sc64-parity-{scenario_id}-') as temp:
        trace = pathlib.Path(keep or temp) / f'{scenario_id}.trace'
        binary, cmd = gba_command(scenario_id, golden, trace)
        if not binary.exists():
            return {**result, 'status': 'ERROR', 'why': f'{binary} is missing; build it with make (mk/parity/Makefile)'}
        run = subprocess.run(cmd, capture_output=True, text=True)
        if run.returncode != 0:
            return {**result, 'status': 'ERROR', 'why': f'GBA run exited {run.returncode}: {run.stderr.strip()[-400:]}',
                    'command': cmd}
        report = json.loads(run.stdout.strip().splitlines()[-1])
        if 'gba_series' in spec:
            # Scenarios that stream their own trace (skirmish): measured inside the temporary
            # directory, which holds the trace the series reads lazily.
            series = spec['gba_series'](trace, report)
            measured, err = spec['measure_gba'](series, golden, report)
            return finish_scenario(scenario_id, golden, result, measured, err, baseline, report, cmd, allow_stale)
        series = gba_series(trace)
        if 'parse_gba' in spec:
            # Scenarios that read more of the trace than the series keeps (buildings, hits).
            report['parsed'] = spec['parse_gba'](trace)
    report['movers'] = [int(x) for x in report.get('movers', [])] if isinstance(report.get('movers'), list) else []
    measured, err = spec['measure_gba'](series, golden, report)
    return finish_scenario(scenario_id, golden, result, measured, err, baseline, report, cmd, allow_stale)


def finish_scenario(scenario_id, golden, result, measured, err, baseline, report, cmd, allow_stale=False):
    if err:
        return {**result, 'status': 'ERROR', 'why': 'GBA measurement: ' + err}
    layout = parity_decode.Layout()
    rows = judge(scenario_id, golden, measured['values'], measured.get('curves', {}), baseline, layout, allow_stale)
    return {**result, 'status': 'RUN', 'rows': rows, 'gba_measured': measured.get('measured'),
            'gba_cpu_seconds': report.get('cpu_seconds'), 'command': cmd}


def build_binaries(goldens):
    """Build the GBA parity binaries the goldens name; {profile: error text or None}."""
    profiles = sorted({metrics.SCENARIOS[g.stem]['profile'] for g in goldens if g.stem in metrics.SCENARIOS})
    if not profiles:
        return {}
    cmd = ['make', '-s']
    descriptor = ROOT / '.local' / 'build' / 'z9-inputs' / 'z8-descriptor.json'
    if descriptor.exists():
        cmd.append(f'Z8_DESCRIPTOR={descriptor.relative_to(ROOT)}')  # as the Zerg Z10 ROM is built
    cmd += [f'build/parity/{p}-fast-trace' for p in profiles]
    run = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if run.returncode == 0:
        return {p: None for p in profiles}
    # Never fall back to an older binary: every profile of a failed build is an error.
    errors = [line for line in (run.stdout + run.stderr).splitlines() if ' error: ' in line or 'Error ' in line]
    text = ('make failed: ' + ' | '.join(errors[-3:] or [(run.stderr or run.stdout).strip()[-300:]]))[-400:]
    return {p: text for p in profiles}


def run_tables(home):
    """The ROM-table differ: report-only scorecard lines (counts by class)."""
    # A private directory per invocation, so concurrent runs never share a report.
    home.mkdir(parents=True, exist_ok=True)
    out = pathlib.Path(tempfile.mkdtemp(prefix='tables-', dir=home))
    cmd = [sys.executable, str(ROOT / 'tools' / 'sc64_parity_tables.py'), '--out', str(out)]
    run = subprocess.run(cmd, capture_output=True, text=True)
    try:
        return tables_result(run, out)
    finally:
        for f in out.iterdir():
            f.unlink()
        out.rmdir()


def tables_result(run, out):
    if run.returncode != 0:
        return {'id': 'tables', 'status': 'ERROR', 'why': run.stderr.strip()[-400:]}
    if 'SKIP' in run.stderr and not (out / 'rom-table-diff.json').exists():
        return {'id': 'tables', 'status': 'SKIP', 'why': 'private ROM inputs absent'}
    doc = json.loads((out / 'rom-table-diff.json').read_text())
    counts = doc['counts']
    return {'id': 'tables', 'status': 'RUN', 'rows': [
        {'key': 'tables:diff:gameplay', 'check': 'tables.diff', 'subject': 'gameplay cells', 'gba': counts['total gameplay'],
         'provenance': 'rom-table', 'verdict': 'REPORT', 'gating': False},
        {'key': 'tables:diff:all', 'check': 'tables.diff', 'subject': 'all cells', 'gba': counts['total'],
         'provenance': 'rom-table', 'verdict': 'REPORT', 'gating': False},
        {'key': 'tables:absent', 'check': 'tables.absent', 'subject': 'ROM rows the engine lacks', 'gba': counts['absent'],
         'provenance': 'rom-table', 'verdict': 'REPORT', 'gating': False}]}


def load_baseline(path):
    if not path.exists():
        return {}
    doc = json.loads(path.read_text())
    for line in doc.get('known', []):
        if not isinstance(line.get('gba'), (int, float)) or isinstance(line.get('gba'), bool):
            raise SystemExit(f'{path}: baseline line {line.get("key")!r} has no numeric GBA value')
    return {line['key']: line for line in doc.get('known', [])}


def write_baseline(path, results, old):
    known = []
    seen = set()
    for result in results:
        for row in result.get('rows', []):
            seen.add(row['key'])
            if row['gating'] and row['verdict'] in ('REGRESSION', 'KNOWN'):
                if not isinstance(row['gba'], (int, float)):
                    raise ValueError(f'{row["key"]}: no GBA value to accept')
                line = {'key': row['key'], 'gba': row['gba']}
                note = old.get(row['key'], {}).get('note')
                if note:
                    line['note'] = note
                known.append(line)
    # Lines of scenarios that did not run this time are kept as they were.
    ran = {r['id'] for r in results if r.get('status') == 'RUN'}
    for key, line in old.items():
        if key.split(':')[0] not in ran and key not in seen:
            known.append(line)
    known.sort(key=lambda line: line['key'])
    doc = {'format': 'sc64-parity-baseline/1',
           'about': 'Known GBA/N64 parity gaps: GBA values only (N64 bands stay in the private fixture home). '
                    'Rewritten by ./dev parity --accept.', 'known': known}
    path.write_text(json.dumps(doc, indent=1) + '\n')


def fixture_hash(goldens):
    digest = hashlib.sha256()
    for path in sorted(goldens):
        digest.update(path.name.encode() + b'\0' + hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def fmt(v):
    if v is None:
        return '-'
    if v == metrics.ABSENT:
        return 'absent'
    if v == metrics.OFF_TARGET:
        return 'off-target'
    if isinstance(v, float):
        return f'{v:.4g}'
    return str(v)


def scorecard(results, home, goldens):
    lines = []
    counts = {}
    for result in results:
        if result['status'] != 'RUN':
            lines.append(f'{result["status"]:<10} {result["id"]}: {result.get("why", "")}')
            counts[result['status']] = counts.get(result['status'], 0) + 1
            continue
        for row in result['rows']:
            counts[row['verdict']] = counts.get(row['verdict'], 0) + 1
            band = f'band [{fmt(row.get("lo"))}, {fmt(row.get("hi"))}]' if 'lo' in row else ''
            extra = []
            if row.get('ratio') is not None and row['verdict'] not in ('PASS',):
                extra.append(f'GBA/N64 {row["ratio"]}')
            if row.get('first_divergent_frame') is not None:
                extra.append(f'first divergent source frame {row["first_divergent_frame"]}')
            if row.get('why'):
                extra.append(row['why'])
            lines.append(f'{row["verdict"]:<10} {row["key"]:<44} gba {fmt(row.get("gba")):>9} '
                         f'n64 {fmt(row.get("n64")):>9} {band:<27} {row["provenance"]}'
                         + (f'  ({"; ".join(extra)})' if extra else ''))
    summary = ', '.join(f'{k} {v}' for k, v in sorted(counts.items()))
    lines.append(f'parity: {summary or "nothing ran"}')
    lines.append(f'fixture set {fixture_hash(goldens)[:16]} ({len(goldens)} golden files in {home})')
    return lines, counts


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--home', type=pathlib.Path, default=None, help='fixture home (default $SC64_PARITY_HOME)')
    parser.add_argument('--baseline', type=pathlib.Path, default=BASELINE)
    parser.add_argument('--accept', action='store_true', help='rewrite the baseline from this run')
    parser.add_argument('--only', action='append', help='run only these scenario ids (repeatable)')
    parser.add_argument('--no-tables', action='store_true', help='skip the ROM-table differ')
    parser.add_argument('--keep', type=pathlib.Path, help='keep GBA traces in this directory')
    parser.add_argument('--json', type=pathlib.Path, help='also write the full results here')
    parser.add_argument('--explain', metavar='ID', help='print the full measurements of one scenario')
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--no-build', action='store_true', help='use the GBA binaries as they are')
    parser.add_argument('--allow-stale', action='store_true',
                        help='report goldens decoded against other layout field definitions instead of failing '
                             '(recording work only)')
    args = parser.parse_args(argv)
    home = args.home or default_home()
    baseline = load_baseline(args.baseline)
    goldens = sorted((home / 'golden').glob('*.json')) if (home / 'golden').is_dir() else []
    if args.only:
        goldens = [g for g in goldens if g.stem in args.only]
    if args.explain:
        goldens = [g for g in goldens if g.stem == args.explain]
    if args.keep:
        args.keep.mkdir(parents=True, exist_ok=True)
    built = {} if args.no_build else build_binaries(goldens)
    broken = {p: why for p, why in built.items() if why}
    jobs = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        if not args.no_tables and not args.explain and not args.only:
            jobs.append(pool.submit(run_tables, home))
        for path in goldens:
            profile = metrics.SCENARIOS.get(path.stem, {}).get('profile')
            if profile in broken:
                jobs.append(pool.submit(lambda p=path, why=broken[profile]: {'id': p.stem, 'status': 'ERROR', 'why': why}))
            else:
                jobs.append(pool.submit(run_scenario, path.stem, path, baseline, args.keep, args.allow_stale))
        results = [job.result() for job in jobs]
    ran_ids = {r['id'] for r in results if r['status'] == 'RUN'}
    seen = {row['key'] for r in results for row in r.get('rows', [])}
    stale = [{'key': key, 'check': key.split(':')[1], 'subject': key.split(':', 2)[2], 'gba': line.get('gba'),
              'provenance': '-', 'verdict': 'STALE', 'gating': False, 'why': 'baseline line with no observation'}
             for key, line in sorted(baseline.items()) if key.split(':')[0] in ran_ids and key not in seen]
    if stale:
        results.append({'id': 'baseline', 'status': 'RUN', 'rows': stale})
    if not goldens:
        results.append({'id': 'scenarios', 'status': 'SKIP',
                        'why': f'no golden files in {home / "golden"} (private N64 captures; see docs/PARITY-HARNESS.md)'})
    if args.explain:
        print(json.dumps(results, indent=1, default=str))
        counts = {}
        for r in results:
            for row in r.get('rows', []):
                counts[row['verdict']] = counts.get(row['verdict'], 0) + 1
        if not results or any(r['status'] != 'RUN' for r in results):
            return 1
        return 1 if any(counts.get(v) for v in FAILING) else 0
    lines, counts = scorecard(results, home, goldens)
    print('\n'.join(lines))
    if args.json:
        args.json.write_text(json.dumps({'results': results, 'fixture_set': fixture_hash(goldens)}, indent=1, default=str) + '\n')
    if args.accept:
        if any(row.get('stale') for r in results for row in r.get('rows', [])):
            print('parity: not accepting while a golden was decoded against other layout field definitions; '
                  'regenerate it first. The baseline is unchanged', file=sys.stderr)
            return 1
        if any(r['status'] == 'ERROR' for r in results):
            print('parity: not accepting a run with errors; the baseline is unchanged', file=sys.stderr)
            return 1
        try:
            write_baseline(args.baseline, results, baseline)
        except ValueError as error:
            print(f'parity: not accepting: {error}; the baseline is unchanged', file=sys.stderr)
            return 1
        print(f'baseline rewritten: {args.baseline}')
    code = exit_code(results, counts, (ROOT / '.local' / 'sc64').exists(), args.accept)
    if code and not any(r['status'] in ('RUN', 'ERROR') for r in results):
        print('parity: every check skipped while .local/sc64 exists; prepare the fixture home or fix the inputs',
              file=sys.stderr)
    return code


def exit_code(results, counts, private_inputs, accept=False):
    """1 on any ERROR, on a REGRESSION (unless accepting) or when everything
    skipped although private inputs exist; else 0."""
    if any(r['status'] == 'ERROR' for r in results):
        return 1
    if not any(r['status'] == 'RUN' for r in results):
        return 1 if private_inputs else 0
    if accept:
        return 0
    return 1 if any(counts.get(v) for v in FAILING) else 0


if __name__ == '__main__':
    sys.exit(main())
