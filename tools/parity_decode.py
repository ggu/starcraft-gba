#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Decode N64 parity captures and write golden bundles.

A capture is a run directory written by experiments/ares-headless/capture.py
with a tape that turns on `tap tick rec`: the frontend then writes tap.bin, one
record per tapped read (format in experiments/ares-headless/capture_view.h).
Decoding is offline against a versioned layout file (tools/parity/sc64-usa-v1.json)
whose fields are each proven, candidate or unknown, so a wrong offset costs a
re-decode, never a recapture. A check that gates may only read proven fields:
asking for any other field with gating=True raises LayoutError.

  parity_decode.py summary RUN                 clocks, table, hash chain
  parity_decode.py golden --scenario ID RUN_A RUN_B [--home DIR] [--seed-note TEXT]

`golden` requires two fresh-process captures of the same tape, binary and ROM
whose per-tick hash chains are identical, then measures the scenario with
tools/sc64_parity_metrics.py and writes $SC64_PARITY_HOME/golden/<ID>.json.
The capture directories hold ROM-derived data and stay private; the golden
file holds derived numbers only and is written only under the fixture home.
"""
import argparse
import hashlib
import json
import os
import pathlib
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import sc64_parity_metrics as metrics  # noqa: E402

LAYOUT = ROOT / 'tools' / 'parity' / 'sc64-usa-v1.json'
MAGIC = b'SC64TAP\0'
HEADER_FIELDS = ('step', 'tick', 'phase', 'live', 'seconds', 'pause', 'selector', 'running', 'countdown',
                 'table', 'flags', 'rows')
FLAG_ROWS, FLAG_TABLE, FLAG_DUAL_DIFFER, FLAG_BAD_LINKS = 1, 2, 4, 8
TYPES = {'u8': ('>B', 1), 'u16': ('>H', 2), 'u32': ('>I', 4)}


class LayoutError(Exception):
    """A field was asked for that the layout does not allow for that use."""


def default_home():
    env = os.environ.get('SC64_PARITY_HOME')
    if env:
        return pathlib.Path(env)
    # One shared home for every worktree: the main checkout's .local/parity.
    out = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', '--path-format=absolute', '--git-common-dir'],
                         capture_output=True, text=True)
    common = out.stdout.strip() if out.returncode == 0 else ''
    base = pathlib.Path(common).parent if common else ROOT
    return base / '.local' / 'parity'


class Layout:
    def __init__(self, path=LAYOUT):
        self.path = pathlib.Path(path)
        self.raw = self.path.read_bytes()
        self.doc = json.loads(self.raw)
        self.name = self.doc['layout']
        self.sha256 = hashlib.sha256(self.raw).hexdigest()
        self.used = set()

    def unit_field(self, name, gating):
        field = self.doc['unit_fields'].get(name)
        if field is None:
            raise LayoutError(f'{self.name}: no unit field {name!r}')
        return self._check(f'unit.{name}', field, gating)

    def bank_array(self, name, gating):
        field = self.doc['globals']['banks']['arrays'].get(name)
        if field is None:
            raise LayoutError(f'{self.name}: no bank array {name!r}')
        return self._check(f'banks.{name}', field, gating)

    def clock(self, name, gating):
        return self._check(f'clock.{name}', self.doc['clock'][name], gating)

    def status(self, key):
        """Status of a dependency key as observations name it: unit.<f>, banks.<a>, clock.<c>."""
        kind, _, name = key.partition('.')
        table = {'unit': self.doc['unit_fields'], 'banks': self.doc['globals']['banks']['arrays'],
                 'clock': self.doc['clock']}.get(kind, {})
        field = table.get(name)
        return field.get('status') if isinstance(field, dict) else None

    def require_all(self, keys):
        """Raise unless every key is a proven field (and the unit table itself is proven)."""
        if self.doc['unit_table'].get('status') != 'proven':
            raise LayoutError(f'{self.name}: the unit table locator is not proven')
        bad = [k for k in keys if self.status(k) != 'proven']
        if bad:
            raise LayoutError(f'{self.name}: not proven: {", ".join(sorted(bad))}')

    def _check(self, key, field, gating):
        if gating and field.get('status') != 'proven':
            raise LayoutError(f'{self.name}: {key} is {field.get("status")!r}, not proven; '
                              'it cannot back a gating check')
        self.used.add(key)
        return field


def read_tap(path):
    """Yield (header dict, globals bytes, {row index: 312 bytes})."""
    data = pathlib.Path(path).read_bytes()
    if data[:8] != MAGIC:
        raise ValueError(f'{path}: not a tap.bin file')
    version, header_bytes, globals_bytes, row_bytes = struct.unpack_from('<4I', data, 8)
    if version != 1 or header_bytes != 4 * len(HEADER_FIELDS) or row_bytes != 314:
        raise ValueError(f'{path}: unsupported tap.bin version {version}')
    at = 24
    while at < len(data):
        if at + header_bytes + globals_bytes > len(data):
            raise ValueError(f'{path}: truncated record at byte {at}')
        head = dict(zip(HEADER_FIELDS, struct.unpack_from(f'<{len(HEADER_FIELDS)}I', data, at)))
        at += header_bytes
        glob = data[at:at + globals_bytes]
        at += globals_bytes
        rows = {}
        if head['flags'] & FLAG_ROWS:
            end = at + head['rows'] * row_bytes
            if end > len(data):
                raise ValueError(f'{path}: truncated rows at byte {at}')
            for k in range(head['rows']):
                index = struct.unpack_from('>H', data, at)[0]
                rows[index] = data[at + 2:at + row_bytes]
                at += row_bytes
        yield head, glob, rows


class Capture:
    """One capture run: per-tick records keyed by tick value, the unpaused clock and a hash chain."""

    def __init__(self, run, layout=None):
        self.run = pathlib.Path(run)
        self.layout = layout or Layout()
        self.manifest = json.loads((self.run / 'manifest.json').read_text())
        self.verified = self._verify_artifacts()
        self.ticks = []          # follow-up records in tick order: dicts
        self.faults = {'dual_differ': 0, 'bad_links': 0, 'missing_follow': 0, 'tick_skips': 0}
        change = {}
        chain = hashlib.sha256()
        last_tick = None
        for head, glob, rows in read_tap(self.run / 'tap.bin'):
            if head['flags'] & FLAG_DUAL_DIFFER:
                self.faults['dual_differ'] += 1
            if head['flags'] & FLAG_BAD_LINKS:
                self.faults['bad_links'] += 1
            if head['phase'] == 0:
                # The change record a follow-up read belongs to is the latest one
                # for its tick (a tick value can repeat if the counter restarts).
                change[head['tick']] = head
                continue
            if not head['flags'] & FLAG_ROWS:
                continue
            tick = head['tick']
            if last_tick is not None and tick != (last_tick + 1) & 0xFFFFFFFF:
                self.faults['tick_skips'] += 1
            last_tick = tick
            paused = change.get(tick, head)['pause'] != 0
            record = {'tick': tick, 'step': head['step'], 'paused': paused, 'selector': head['selector'],
                      'seconds': head['seconds'], 'table': bool(head['flags'] & FLAG_TABLE),
                      'globals': glob, 'rows': rows}
            self.ticks.append(record)
            # Everything the measurements read: the tick, the frontend step (the
            # command mark is located by step), the change-step pause state (the
            # unpaused clock), table presence, globals and rows.
            digest = hashlib.sha256()
            digest.update(struct.pack('<IIBB', tick, head['step'], paused, record['table']))
            digest.update(glob)
            for index in sorted(rows):
                digest.update(struct.pack('>H', index))
                digest.update(rows[index])
            chain.update(digest.digest())
            record['chain'] = chain.hexdigest()
        self.chain = chain.hexdigest()
        followed = {r['tick'] for r in self.ticks}
        self.faults['missing_follow'] = sum(1 for t in change if t not in followed)
        # Unpaused clock: one source frame per tick whose change-step read
        # showed no pause; numbered from the first in-mission (table) tick.
        clock = -1
        for record in self.ticks:
            if record['table'] and not record['paused']:
                clock += 1
            record['sf'] = clock

    def _verify_artifacts(self):
        """tap.bin and tape.txt must be the files the capture wrote (manifest digests)."""
        problems = []
        recorded = self.manifest.get('outputs_sha256', {}).get('tap.bin')
        if not recorded or recorded != hashlib.sha256((self.run / 'tap.bin').read_bytes()).hexdigest():
            problems.append('tap.bin does not match the manifest')
        if self.manifest.get('tape_sha256') != hashlib.sha256((self.run / 'tape.txt').read_bytes()).hexdigest():
            problems.append('tape.txt does not match the manifest')
        if not self.manifest.get('run_id'):
            problems.append('the manifest has no run id (capture.py before S4)')
        # Recording must stay on once started: a later tap line without `rec` stops tap.bin early.
        taps = [w for w in metrics.frontend_lines((self.run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape'))
                if w[0] == 'tap']
        first_rec = next((i for i, w in enumerate(taps) if 'rec' in w[1:]), None)
        if first_rec is not None and any('rec' not in w[1:] for w in taps[first_rec + 1:]):
            problems.append('the tape stops or changes recording after it starts (tap.bin covers part of the run)')
        # commands.csv places tape marks (S5); it must be the log the capture wrote.
        commands = self.run / 'commands.csv'
        recorded = self.manifest.get('outputs_sha256', {}).get('commands.csv')
        if recorded and not commands.is_file():
            problems.append('commands.csv is recorded in the manifest but missing')
        elif commands.is_file() and (not recorded or recorded != hashlib.sha256(commands.read_bytes()).hexdigest()):
            problems.append('commands.csv does not match the manifest')
        # State files the tape loaded (S5); manifests from before S5 have none. Every `load`
        # in the tape needs a recorded digest, so two runs cannot both omit the map.
        states = self.manifest.get('state_input_sha256') or {}
        for words in metrics.frontend_lines((self.run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')):
            if len(words) >= 2 and words[0] == 'load' and words[1] not in states:
                problems.append(f'the tape loads {words[1]} but the manifest records no digest for it')
        if self.manifest.get('unrecorded_state'):
            problems.append('the capture staged a state no capture recorded (--unrecorded-state)')
        # Golden evidence needs the producer of every loaded state: its run id and binary.
        sources = self.manifest.get('state_input_source') or {}
        for words in metrics.frontend_lines((self.run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')):
            if len(words) >= 2 and words[0] == 'load':
                source = sources.get(words[1]) if isinstance(sources, dict) else None
                binary = source.get('binary_sha256') if isinstance(source, dict) else None
                run_id = source.get('run_id') if isinstance(source, dict) else None
                if not (isinstance(run_id, str) and run_id and isinstance(binary, str) and len(binary) == 64
                        and all(c in '0123456789abcdef' for c in binary)):
                    problems.append(f'state input {words[1]} has no recorded producer (run id and binary)')
        for name, digest in states.items():
            staged = self.run / 'input' / name
            if not staged.is_file() or hashlib.sha256(staged.read_bytes()).hexdigest() != digest:
                problems.append(f'state input {name} does not match the manifest')
        return problems

    def bank(self, record, array, owner, gating=True):
        field = self.layout.bank_array(array, gating)
        offset = int(field['offset'], 16)
        return struct.unpack_from('>I', record['globals'], offset + 4 * owner)[0]

    def unit(self, row_bytes, name, gating=True):
        field = self.layout.unit_field(name, gating)
        fmt, _ = TYPES[field['type']]
        return struct.unpack_from(fmt, row_bytes, int(field['offset'], 16))[0]

    def seed(self):
        """The RNG words of the first in-mission tick (read back, never set)."""
        for record in self.ticks:
            if record['table']:
                return record['globals'][0xC0:0xCC].hex()
        return None

    def series(self, gating=True):
        """Normalised time series for tools/sc64_parity_metrics.py, N64 map pixels."""
        frames = []
        for record in self.ticks:
            if not record['table'] or record['paused'] or record['sf'] < 0:
                continue
            units = {}
            for index, row in record['rows'].items():
                units[index] = (self.unit(row, 'type', gating), self.unit(row, 'owner', gating),
                                self.unit(row, 'x', gating), self.unit(row, 'y', gating))
            banks = {owner: (self.bank(record, 'minerals', owner, gating),
                             self.bank(record, 'minerals_gathered', owner, gating)) for owner in range(12)}
            frames.append({'sf': record['sf'], 'step': record['step'], 'tick': record['tick'],
                           'units': units, 'banks': banks})
        return {'source': 'n64', 'scale': 1, 'frames': frames}

    def summary(self):
        mission = [r for r in self.ticks if r['table']]
        return {
            'run': str(self.run), 'ok': self.manifest.get('ok'), 'binary_sha256': self.manifest.get('binary_sha256'),
            'tape_sha256': self.manifest.get('tape_sha256'), 'mode': self.manifest.get('mode'),
            'records': len(self.ticks), 'mission_ticks': len(mission),
            'unpaused_ticks': sum(1 for r in mission if not r['paused']),
            'selectors': sorted({r['selector'] for r in mission}),
            'first_tick': mission[0]['tick'] if mission else None, 'faults': self.faults,
            'chain': self.chain, 'seed': self.seed(),
            # Provenance and digest problems found when the capture was opened: totals above are
            # unproven while this list is not empty.
            'verified': not self.verified and self.manifest.get('ok') is True,
            'capture_ok': self.manifest.get('ok') is True, 'verification_problems': list(self.verified),
        }


def same_bundle(a, b):
    """Two captures form a golden bundle only if they are fresh processes of the same inputs
    whose per-tick hash chains agree."""
    problems = []
    for key in ('binary_sha256', 'tape_sha256', 'rom_sha256', 'mode', 'flash_input_sha256'):
        if a.manifest.get(key) != b.manifest.get(key):
            problems.append(f'{key} differs')
    if (a.manifest.get('state_input_sha256') or {}) != (b.manifest.get('state_input_sha256') or {}):
        problems.append('state_input_sha256 differs')
    if a.manifest.get('run_id') and a.manifest.get('run_id') == b.manifest.get('run_id'):
        problems.append('both runs carry the same run id: a copied capture is not a second capture')
    for cap in (a, b):
        problems += [f'{cap.run.name}: {p}' for p in cap.verified]
        if cap.faults['tick_skips'] or cap.faults['missing_follow'] > 1:
            problems.append(f'{cap.run.name}: ticks skipped or missing follow-up reads {cap.faults}')
        if not cap.manifest.get('ok'):
            problems.append(f'{cap.run.name}: capture did not finish cleanly')
        if cap.faults['dual_differ'] or cap.faults['bad_links']:
            problems.append(f'{cap.run.name}: read faults {cap.faults}')
    if a.run.resolve() == b.run.resolve():
        problems.append('the two runs are the same directory')
    if len(a.ticks) != len(b.ticks):
        problems.append(f'record counts differ ({len(a.ticks)} vs {len(b.ticks)})')
    for ra, rb in zip(a.ticks, b.ticks):
        if ra['chain'] != rb['chain']:
            problems.append(f'hash chains diverge at tick {ra["tick"]} (step {ra["step"]})')
            break
    return problems


def write_golden(scenario_id, runs, home, layout, note=None):
    if scenario_id not in metrics.SCENARIOS:
        raise SystemExit(f'unknown scenario {scenario_id}; known: {", ".join(metrics.SCENARIOS)}')
    caps = [Capture(run, layout) for run in runs]
    if len(caps) < 2:
        raise SystemExit('a golden bundle needs two fresh-process captures')
    if len({c.run.resolve() for c in caps}) != len(caps) or len({c.manifest.get('run_id') for c in caps}) != len(caps):
        raise SystemExit('not golden: every capture must be a different run directory with its own run id')
    problems = []
    for other in caps[1:]:
        problems += same_bundle(caps[0], other)
    if problems:
        raise SystemExit('not golden: ' + '; '.join(problems))
    spec = metrics.SCENARIOS[scenario_id]
    marks = [metrics.tape_commands(c.run / 'tape.txt') for c in caps]
    if any(m != marks[0] for m in marks[1:]):
        raise SystemExit(f'not golden: tape marks resolve to different steps {marks}')
    tape_steps = marks[0]
    result = spec['measure_n64'](caps[0], tape_steps)
    for obs in result['observations']:
        obs['n_runs'] = len(caps)
        obs['provenance'] = 'n64-ram'
        obs['layout_fields'] = sorted(set(obs.get('layout_fields', [])))
        if obs['gating']:
            layout.require_all(obs['layout_fields'])
    golden = {
        'format': 'sc64-parity-golden/1', 'id': scenario_id, 'scenario': spec['title'],
        'layout': layout.name, 'layout_sha256': layout.sha256,
        'timebase': 'source frames: N64 unpaused ticks; GBA simulation steps * 4 / 5',
        'observations': result['observations'], 'curves': result.get('curves', {}),
        'gba': result['gba'], 'notes': result.get('notes', []) + ([note] if note else []),
        'bundle': {
            'runs': [c.run.name for c in caps], 'chain': caps[0].chain, 'seed': caps[0].seed(),
            'run_ids': [c.manifest['run_id'] for c in caps],
            'binary_sha256': caps[0].manifest['binary_sha256'], 'tape_sha256': caps[0].manifest['tape_sha256'],
            'rom_sha256': caps[0].manifest['rom_sha256'], 'mode': caps[0].manifest['mode'],
            'selectors': sorted({r['selector'] for r in caps[0].ticks if r['table']}),
            'records': len(caps[0].ticks), 'faults': caps[0].faults,
        },
    }
    # Golden files carry ROM-derived curves: write them only where no checkout tracks
    # them, through a fresh temporary file (the table differ's private writer).
    from sc64_parity_tables import check_private, write_private
    out = (pathlib.Path(home) / 'golden').resolve() / f'{scenario_id}.json'
    check_private(ROOT, out.parent)
    out.parent.mkdir(parents=True, exist_ok=True)
    check_private(ROOT, out.parent)
    write_private(ROOT, {out: json.dumps(golden, indent=1, sort_keys=True) + '\n'})
    return out, golden


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    s = sub.add_parser('summary')
    s.add_argument('run', type=pathlib.Path)
    g = sub.add_parser('golden')
    g.add_argument('--scenario', required=True)
    g.add_argument('--home', type=pathlib.Path, default=None)
    g.add_argument('--note')
    g.add_argument('runs', nargs='+', type=pathlib.Path)
    for p in (s, g):
        p.add_argument('--layout', type=pathlib.Path, default=LAYOUT)
    args = parser.parse_args(argv)
    layout = Layout(args.layout)
    if args.command == 'summary':
        print(json.dumps(Capture(args.run, layout).summary(), indent=1))
        return 0
    out, golden = write_golden(args.scenario, args.runs, args.home or default_home(), layout, args.note)
    print(json.dumps({'golden': str(out), 'observations': [
        {k: o[k] for k in ('check', 'subject', 'lo', 'hi', 'value')} for o in golden['observations']]}, indent=1))
    return 0


if __name__ == '__main__':
    sys.exit(main())
