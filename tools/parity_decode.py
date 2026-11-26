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
EXTRA_MAGIC = b'SC64XTR\0'
MAX_CAPTURE_BYTES = 3 << 30   # tap.bin + extra.bin of one capture; golden writing holds two
HEADER_FIELDS = ('step', 'tick', 'phase', 'live', 'seconds', 'pause', 'selector', 'running', 'countdown',
                 'table', 'flags', 'rows')
FLAG_ROWS, FLAG_TABLE, FLAG_DUAL_DIFFER, FLAG_BAD_LINKS = 1, 2, 4, 8
TYPES = {'u8': ('>B', 1), 'u16': ('>H', 2), 'u32': ('>I', 4)}


def file_sha256(path):
    """SHA-256 of a file, read in blocks (captures run to hundreds of MB)."""
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


# Shield-bearing unit and building ids (units.dat order): Protoss units and heroes from the Corsair
# (60) to Artanis (88) except the Zerg Devourer (62), and Protoss buildings from the Nexus (154) to
# the Shield Battery (172) except the Stasis Cell/Prison (168).
PROTOSS_TYPES = (frozenset(range(60, 89)) - {62}) | (frozenset(range(154, 173)) - {168})


class LayoutError(Exception):
    """A field was asked for that the layout does not allow for that use."""


# Layout keys that describe evidence or status rather than where and how a value is read:
# editing them never changes what a golden decoded.
NON_SEMANTIC = ('status', 'evidence', 'about')


def semantic(value):
    """A layout definition without its evidence, status and notes."""
    if isinstance(value, dict):
        return {k: semantic(v) for k, v in value.items() if k not in NON_SEMANTIC}
    if isinstance(value, list):
        return [semantic(v) for v in value]
    return value


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
        self.used, self.used_gating = set(), set()

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
        """Status of a dependency key as observations name it: unit.<f>, banks.<a>, clock.<c>,
        pool.<sprite|image|bullet|fog_tiles>, order.<value>, player.<table>, tables.dat."""
        kind, _, name = key.partition('.')
        table = {'unit': self.doc['unit_fields'], 'banks': self.doc['globals']['banks']['arrays'],
                 'clock': self.doc['clock'], 'pool': self.doc.get('pools', {}),
                 'order': self.doc.get('orders', {}).get('values', {}),
                 'player': self.doc.get('player_tables', {}),
                 'tables': {'dat': self.doc.get('dat_tables', {})}}.get(kind, {})
        field = table.get(name)
        return field.get('status') if isinstance(field, dict) else None

    def definition(self, key):
        """The layout entry a dependency key names (None when the layout has no such entry).
        A bank array carries its block's address and player count with it."""
        kind, _, name = key.partition('.')
        doc = self.doc
        if kind == 'banks':
            banks = doc['globals']['banks']
            field = banks['arrays'].get(name)
            return None if field is None else {'block': {k: banks[k] for k in ('address', 'players', 'type')},
                                                'field': field}
        table = {'unit': doc['unit_fields'], 'clock': doc['clock'], 'pool': doc.get('pools', {}),
                 'order': doc.get('orders', {}).get('values', {}), 'player': doc.get('player_tables', {}),
                 'tables': {'dat': doc.get('dat_tables')}}.get(kind)
        return None if table is None else table.get(name)

    def fields_digest(self, keys):
        """SHA-256 of what decoding the named fields depends on: their definitions without
        evidence or status, plus the decoder's fixed parts (the unit table locator and row size,
        the bank block and the record rule). A golden stays valid under any layout edit that
        leaves this digest unchanged (an added field, a promotion, a new evidence line)."""
        payload = {'core': semantic({'unit_table': self.doc['unit_table'],
                                     'banks': {k: self.doc['globals']['banks'][k] for k in ('address', 'players', 'type')},
                                     'rng': self.doc['globals'].get('rng'),
                                     'record_rule': self.doc['clock'].get('record_rule')}),
                   'fields': {key: semantic(self.definition(key)) for key in sorted(set(keys))}}
        return hashlib.sha256(json.dumps(payload, sort_keys=True).encode()).hexdigest()

    def pool(self, name, gating):
        field = self.doc.get('pools', {}).get(name)
        if not isinstance(field, dict):
            raise LayoutError(f'{self.name}: no pool {name!r}')
        return self._check(f'pool.{name}', field, gating)

    def pool_field(self, pool, name, gating):
        """(offset, struct format) of one field of a pool record, parsed from the layout's
        "0xNN type ..." definition; a definition the decoders cannot honour raises LayoutError."""
        fields = self.pool(pool, gating).get('fields', {})
        text = fields.get(name)
        words = text.split() if isinstance(text, str) else []
        fmt = {'u8': '>B', 'u16': '>H', 'u32': '>I'}.get(words[1] if len(words) > 1 else None)
        try:
            offset = int(words[0], 16) if words else None
        except ValueError:
            offset = None
        if offset is None or fmt is None or offset < 0 or offset + struct.calcsize(fmt) > self.pool(pool, gating)['record_bytes']:
            raise LayoutError(f'{self.name}: pool.{pool} field {name!r} is {text!r}; the decoders need "0xNN u8|u16|u32" '
                              'inside the record')
        return offset, fmt

    def order_name(self, value, gating):
        """Name of an order value; gating use needs that value proven."""
        field = self.doc.get('orders', {}).get('values', {}).get(str(value))
        if field is None:
            if gating:
                raise LayoutError(f'{self.name}: order value {value} is not named')
            return f'order{value}'
        return self._check(f'order.{value}', field, gating)['name']

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
        if gating:
            self.used_gating.add(key)
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


def read_extra(path):
    """extra.bin ("region ADDR BYTES" then "region on"): ([(KSEG0 address, bytes)],
    yield of (step, tick, ordinal, [region bytes])). Format in capture_view.h."""
    data = pathlib.Path(path).read_bytes()
    if data[:8] != EXTRA_MAGIC:
        raise ValueError(f'{path}: not an extra.bin file')
    version, count = struct.unpack_from('<2I', data, 8)
    if version != 1 or not 0 < count <= 16:
        raise ValueError(f'{path}: unsupported extra.bin version {version} or region count {count}')
    regions = [struct.unpack_from('<2I', data, 16 + 8 * k) for k in range(count)]
    total = sum(n for _, n in regions)
    at = 16 + 8 * count
    records = []
    while at < len(data):
        if at + 12 + total > len(data):
            raise ValueError(f'{path}: truncated record at byte {at}')
        step, tick, ordinal = struct.unpack_from('<3I', data, at)
        at += 12
        blocks = []
        for _, n in regions:
            blocks.append(data[at:at + n])
            at += n
        records.append((step, tick, ordinal, blocks))
    return regions, records


class Capture:
    """One capture run: per-tick records keyed by tick value, the unpaused clock and a hash chain."""

    def __init__(self, run, layout=None):
        self.run = pathlib.Path(run)
        self.layout = layout or Layout()
        self.manifest = json.loads((self.run / 'manifest.json').read_text())
        # Decoding holds a whole capture in memory: refuse clearly, before reading it, instead of
        # exhausting memory.
        limit = int(os.environ.get('SC64_PARITY_MAX_CAPTURE_BYTES') or MAX_CAPTURE_BYTES)
        size = sum((self.run / name).stat().st_size for name in ('tap.bin', 'extra.bin') if (self.run / name).is_file())
        if size > limit:
            raise ValueError(f'{self.run}: tap.bin and extra.bin hold {size} bytes, more than the decoder holds in '
                             f'memory ({limit}; SC64_PARITY_MAX_CAPTURE_BYTES); record a shorter window or every Kth tick')
        self.verified = self._verify_artifacts()
        self.ticks = []          # follow-up records in tick order: dicts
        self.faults = {'dual_differ': 0, 'bad_links': 0, 'missing_follow': 0, 'tick_skips': 0,
                       'skipped_ticks': 0, 'unexplained_skips': 0, 'interior_missing': 0}
        skips = []   # (record index, skipped tick values) for ticks that began and ended inside one step
        change = {}
        chain = hashlib.sha256()
        last_tick = None
        self.regions, self.extra, extra_faults, joined = [], {}, 0, set()
        if (self.run / 'extra.bin').is_file():
            self.regions, records = read_extra(self.run / 'extra.bin')
            for step, tick, ordinal, blocks in records:
                if step in self.extra:
                    extra_faults += 1   # two extra records for one step
                self.extra[step] = {'tick': tick, 'ordinal': ordinal, 'blocks': blocks}
            # The region list joins the chain once, so equal payloads under other addresses differ.
            chain.update(b'extra-regions' + b''.join(struct.pack('<2I', a, n) for a, n in self.regions))
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
                gap = (tick - last_tick) & 0xFFFFFFFF
                if 0 < gap <= 8:
                    skips.append((len(self.ticks), [(last_tick + k) & 0xFFFFFFFF for k in range(1, gap)]))
                else:
                    # A backward step or a long jump cannot be explained by change-step reads.
                    self.faults['unexplained_skips'] += 1
            last_tick = tick
            paused = change.get(tick, head)['pause'] != 0
            record = {'tick': tick, 'step': head['step'], 'paused': paused, 'selector': head['selector'],
                      'seconds': head['seconds'], 'table': bool(head['flags'] & FLAG_TABLE),
                      'table_offset': head['table'],
                      'globals': glob, 'rows': rows}
            self.ticks.append(record)
            # Everything the measurements read: the tick, the frontend step (the
            # command mark is located by step), the change-step pause state (the
            # unpaused clock), table presence, globals and rows.
            digest = hashlib.sha256()
            digest.update(struct.pack('<IIBB', tick, head['step'], paused, record['table']))
            if self.regions:
                # Extra-region decoders resolve unit pointers through the table offset: bind it.
                # Captures without extra.bin keep the chain they always had.
                digest.update(struct.pack('<I', head['table']))
            digest.update(glob)
            for index in sorted(rows):
                digest.update(struct.pack('>H', index))
                digest.update(rows[index])
            # Extra regions (extra.bin) recorded at this follow-up read join the chain; a
            # capture without extra.bin keeps the chain it always had.
            if head['step'] in self.extra:
                if self.extra[head['step']]['tick'] != tick:
                    extra_faults += 1   # an extra record that is not this follow-up read's
                elif head['step'] in joined:
                    extra_faults += 1   # one extra record joined twice
                else:
                    record['extra'] = self.extra[head['step']]['blocks']
                    joined.add(head['step'])
                    for block in record['extra']:   # only joined blocks enter the chain
                        digest.update(block)
            chain.update(digest.digest())
            record['chain'] = chain.hexdigest()
        self.chain = chain.hexdigest()
        if (self.run / 'extra.bin').is_file():
            # Every extra record must belong to exactly one follow-up record (step and tick).
            self.faults['extra'] = extra_faults + sum(1 for step in self.extra if step not in joined)
        followed = {r['tick'] for r in self.ticks}
        self.faults['missing_follow'] = sum(1 for t in change if t not in followed)
        # Interior misses only: the tap can start on a change step, and the run ends after one.
        lo, hi = (min(followed), max(followed)) if followed else (0, -1)
        self.faults['interior_missing'] = sum(1 for t in change if t not in followed and lo < t < hi)
        # A tick that began and ended inside one frontend step has a change-step read (its
        # pause state) but no follow-up record: it still advances the clock when it ran
        # unpaused. A skipped tick with no change-step read leaves the clock unknown.
        # A genuine skip: the next tick's change read comes on the very next frontend step, so
        # there was no step left for a follow-up read. Anything else is data loss.
        extra = {}
        for index, skipped in skips:
            self.faults['skipped_ticks'] += len(skipped)
            for value in skipped:
                nxt = change.get((value + 1) & 0xFFFFFFFF)
                if value not in change or nxt is None or nxt['step'] != change[value]['step'] + 1:
                    self.faults['unexplained_skips'] += 1
                elif change[value]['pause'] == 0 and change[value]['flags'] & FLAG_TABLE:
                    # Its own change read says it ran in mission and unpaused.
                    extra.setdefault(index, []).append(change[value]['step'])
        # A capture can also start on a tick that ran inside one step, before any follow-up
        # read: the same proof (the next tick's change read on the very next step) admits it.
        if self.ticks:
            lead = (self.ticks[0]['tick'] - 1) & 0xFFFFFFFF
            head0 = change.get(self.ticks[0]['tick'])
            if (lead in change and head0 is not None and head0['step'] == change[lead]['step'] + 1 and
                    change[lead]['pause'] == 0 and change[lead]['flags'] & FLAG_TABLE):
                extra.setdefault(0, []).insert(0, change[lead]['step'])
                self.faults['skipped_ticks'] += 1
        # Unpaused clock: one source frame per tick whose change-step read
        # showed no pause; numbered from the first in-mission (table) tick.
        clock = -1
        self.skipped_frames = []   # (frontend step of the change read, source frame) of skipped ticks
        for index, record in enumerate(self.ticks):
            if record['table']:
                for step in extra.get(index, []):
                    clock += 1
                    self.skipped_frames.append((step, clock))
            if record['table'] and not record['paused']:
                clock += 1
            record['sf'] = clock

    def _verify_artifacts(self):
        """tap.bin and tape.txt must be the files the capture wrote (manifest digests)."""
        problems = []
        recorded = self.manifest.get('outputs_sha256', {}).get('tap.bin')
        if not recorded or recorded != file_sha256(self.run / 'tap.bin'):
            problems.append('tap.bin does not match the manifest')
        extra = self.run / 'extra.bin'
        recorded_extra = self.manifest.get('outputs_sha256', {}).get('extra.bin')
        if recorded_extra and not extra.is_file():
            problems.append('extra.bin is recorded in the manifest but missing')
        if extra.is_file() and (not recorded_extra or recorded_extra != file_sha256(extra)):
            problems.append('extra.bin does not match the manifest')
        if extra.is_file():
            # Extra-region reads count their cache-alias conflicts into status.log's totals
            # (the tap record flag covers only the clocks, banks and rows).
            status = self.run / 'status.log'
            recorded_status = self.manifest.get('outputs_sha256', {}).get('status.log')
            text = status.read_text() if status.is_file() else ''
            if not recorded_status or recorded_status != hashlib.sha256(status.read_bytes() if status.is_file() else b'').hexdigest():
                problems.append('status.log does not match the manifest')
            else:
                summary = [line for line in text.splitlines() if line.startswith('tap_reads=')]
                values = [w.split('=', 1)[1] for w in summary[0].split() if w.startswith('dual_differ=')] \
                    if len(summary) == 1 else []
                if values != ['0']:
                    problems.append('a recorded read saw two different cache-alias copies, or status.log has no '
                                    'single tap summary (dual_differ)')
        if self.manifest.get('tape_sha256') != hashlib.sha256((self.run / 'tape.txt').read_bytes()).hexdigest():
            problems.append('tape.txt does not match the manifest')
        if not self.manifest.get('run_id'):
            problems.append('the manifest has no run id (capture.py before S4)')
        # Recording must stay on once started: a later tap line without `rec` stops tap.bin early.
        taps = [w for w in metrics.frontend_lines((self.run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape'))
                if w[0] == 'tap']
        first_rec = next((i for i, w in enumerate(taps) if 'rec' in w[1:]), None)
        # Any later tap line restarts the tick tracking, which can hide a follow-up read.
        if first_rec is not None and taps[first_rec + 1:]:
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
            if not staged.is_file() or file_sha256(staged) != digest:
                problems.append(f'state input {name} does not match the manifest')
        return problems

    def world_point(self, step):
        """The world point under the cursor (camera s32 + cursor u16, S6) before frontend step
        `step`, from the run's watch.csv, which must be the log the capture wrote. A compiled
        tape's order acts at this point, so the GBA lab replays it. The log is hashed in blocks
        and read once per capture, as a stream of full-width camera and cursor samples."""
        if not hasattr(self, '_view_samples'):
            watch = self.run / 'watch.csv'
            recorded = self.manifest.get('outputs_sha256', {}).get('watch.csv')
            self._view_samples = None
            if watch.is_file() and recorded:
                # One pass: the bytes parsed are the bytes hashed, and the samples are kept only
                # when that digest is the manifest's. Only full-width samples count: camera
                # words are 4 bytes, cursor halfwords 2.
                width = {b'800a2ee8': b'4', b'800a2eec': b'4', b'800afdcc': b'2', b'800afdce': b'2'}
                digest, samples = hashlib.sha256(), []
                with open(watch, 'rb') as handle:
                    for number, line in enumerate(handle):
                        digest.update(line)
                        fields = line.rstrip(b'\n').split(b',')
                        if number and len(fields) == 5 and width.get(fields[2]) == fields[3]:
                            samples.append((int(fields[0]), fields[2].decode(), int(fields[4], 16)))
                if digest.hexdigest() == recorded:
                    self._view_samples = samples
        if self._view_samples is None:
            raise ValueError(f'{self.run}: watch.csv is missing or does not match the manifest')
        last = {}
        for at, address, value in self._view_samples:
            if at > step:
                break
            last[address] = value
        if any(a not in last for a in ('800a2ee8', '800a2eec', '800afdcc', '800afdce')):
            raise ValueError(f'{self.run}: the tape did not watch the camera and cursor before step {step}')
        signed = lambda v: v - (1 << 32) if v & 0x80000000 else v
        return signed(last['800a2ee8']) + last['800afdcc'], signed(last['800a2eec']) + last['800afdce']

    def bank(self, record, array, owner, gating=True):
        field = self.layout.bank_array(array, gating)
        offset = int(field['offset'], 16)
        return struct.unpack_from('>I', record['globals'], offset + 4 * owner)[0]

    def unit(self, row_bytes, name, gating=True):
        field = self.layout.unit_field(name, gating)
        fmt, _ = TYPES[field['type']]
        return struct.unpack_from(fmt, row_bytes, int(field['offset'], 16))[0]

    def extra_bytes(self, record, address, length):
        """Guest bytes [address, address + length) from this tick's extra.bin regions, or None
        when no recorded region covers them."""
        for (start, size), block in zip(self.regions, record.get('extra', ())):
            if start <= address and address + length <= start + size:
                return block[address - start:address - start + length]
        return None

    def fog(self, record, player, width, height, gating=True):
        """(visible, explored) tile index sets (x + y * width) of one player at this tick,
        read from the recorded fog tile block (the region of width * height * 4 bytes)."""
        field = self.layout.pool('fog_tiles', gating)
        size = width * height * field['record_bytes']
        if sum(1 for _, n in self.regions if n == size) != 1:
            raise LayoutError(f'{self.run.name}: not exactly one recorded region of {size} bytes for the fog tiles')
        for (start, n), block in zip(self.regions, record.get('extra', ())):
            if n == size:
                step = field['record_bytes']
                (ex, ex_fmt), (vi, vi_fmt) = (self.layout.pool_field('fog_tiles', 'explored_mask', gating),
                                              self.layout.pool_field('fog_tiles', 'visible_mask', gating))
                if ex_fmt != '>B' or vi_fmt != '>B':
                    raise LayoutError(f'{self.layout.name}: the fog decoder reads one-byte player masks')
                # The tiles a fog block holds: no player sees a tile it has not explored.
                if any(~block[step * i + vi] & block[step * i + ex] & 0xFF for i in range(width * height)):
                    raise LayoutError(f'{self.run.name}: the {size}-byte region at {start:08x} breaks the '
                                      'visible-within-explored rule; it is not the fog block')
                bit = 1 << player
                visible = {i for i in range(width * height) if not block[step * i + vi] & bit}
                explored = {i for i in range(width * height) if not block[step * i + ex] & bit}
                return visible, explored
        return None

    def unit_images(self, record, index, gating=True):
        """The images of one live unit at this tick, primary first: dicts of the layout's
        image fields, through its sprite (both pools read from the extra regions). Links are
        followed from the primary image (sprite +0x18) while they stay in recorded memory."""
        sprite_pool, image_pool = self.layout.pool('sprite', gating), self.layout.pool('image', gating)
        get = lambda blob, pool, name: struct.unpack_from((f := self.layout.pool_field(pool, name, gating))[1], blob, f[0])[0]
        sprite = self.unit(record['rows'][index], 'sprite', gating)
        blob = self.extra_bytes(record, sprite, sprite_pool['record_bytes'])
        if blob is None:
            return None
        at, out = get(blob, 'sprite', 'image_primary'), []
        while at and len(out) < 16:
            image = self.extra_bytes(record, at, image_pool['record_bytes'])
            if image is None:
                # The primary must be recorded; a linked image outside the recorded regions
                # (a selection circle from another pool, say) ends the list.
                return out or None
            frame = get(image, 'image', 'frame_and_direction')
            out.append({'address': at, 'image_id': get(image, 'image', 'image_id'),
                        'draw_function': get(image, 'image', 'draw_function'),
                        'frame_set': get(image, 'image', 'frame_set_x4') // 4,
                        'frame_index': frame >> 6, 'direction': frame & 63,
                        'iscript_header': get(image, 'image', 'iscript_header'),
                        'iscript_offset': get(image, 'image', 'iscript_offset'),
                        'animation': get(image, 'image', 'animation'), 'wait': get(image, 'image', 'wait')})
            at = get(image, 'image', 'next')
        return out

    def orders(self, index, gating=True):
        """[(source frame, order value)] each time a row's order byte changes, in-mission ticks only."""
        out = []
        for record in self.ticks:
            row = record['rows'].get(index)
            if row is None or not record['table'] or record['sf'] < 0:
                continue
            value = self.unit(row, 'order', gating)
            if not out or out[-1][1] != value:
                out.append((record['sf'], value))
        return out

    def seed(self):
        """The RNG words of the first in-mission tick (read back, never set)."""
        for record in self.ticks:
            if record['table']:
                return record['globals'][0xC0:0xCC].hex()
        return None

    def series(self, gating=True, hp=False, shields=False):
        """Normalised time series for tools/sc64_parity_metrics.py, N64 map pixels.

        With hp=True each frame also carries 'hp': {row: hit points} (the 8.8
        fixed-point field divided by 256), and with shields=True 'shield' (the
        same for +0x60), for the unit-class labs."""
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
            frame = {'sf': record['sf'], 'step': record['step'], 'tick': record['tick'],
                     'units': units, 'banks': banks}
            if hp:
                frame['hp'] = {index: self.unit(row, 'hp', gating) / 256.0 for index, row in record['rows'].items()}
            if shields:
                # Protoss rows only: other races' rows hold a constant 100 there (layout evidence).
                frame['shield'] = {index: self.unit(row, 'shields', gating) / 256.0
                                   for index, row in record['rows'].items()
                                   if units[index][0] in PROTOSS_TYPES}
            frames.append(frame)
        return {'source': 'n64', 'scale': 1, 'frames': frames, 'skipped': list(getattr(self, 'skipped_frames', []))}

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
            'pokes': self.manifest.get('pokes', 0), 'n64_staged': bool(self.manifest.get('n64_staged')),
            'extra_regions': ['%08x+%d' % r for r in self.regions],
            'extra_records': sum(1 for r in self.ticks if 'extra' in r),
        }


def same_bundle(a, b, allow_skips=False):
    """Two captures form a golden bundle only if they are fresh processes of the same inputs
    whose per-tick hash chains agree. allow_skips admits ticks without a follow-up record (busy
    missions run some ticks one step apart); only report-only scenarios may ask for it."""
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
        # Skipped ticks are allowed only when each has its change-step read (so the clock is
        # exact) and both runs skip the same ticks; scenarios that admit others gate nothing.
        if not allow_skips and (cap.faults['unexplained_skips'] or
                                cap.faults['interior_missing'] > cap.faults['skipped_ticks']):
            problems.append(f'{cap.run.name}: ticks skipped or missing follow-up reads {cap.faults}')
        if not cap.manifest.get('ok'):
            problems.append(f'{cap.run.name}: capture did not finish cleanly')
        if cap.faults['dual_differ'] or cap.faults['bad_links'] or cap.faults.get('extra'):
            problems.append(f'{cap.run.name}: read faults {cap.faults}')
    if a.run.resolve() == b.run.resolve():
        problems.append('the two runs are the same directory')
    if a.faults != b.faults:
        problems.append(f'read faults differ between the runs ({a.faults} vs {b.faults})')
    # Skipped ticks advance the clock from their change-step reads, which the chain does not cover.
    if [r['sf'] for r in a.ticks] != [r['sf'] for r in b.ticks] or a.skipped_frames != b.skipped_frames:
        problems.append('the unpaused clocks differ between the runs')
    if len(a.ticks) != len(b.ticks):
        problems.append(f'record counts differ ({len(a.ticks)} vs {len(b.ticks)})')
    for ra, rb in zip(a.ticks, b.ticks):
        if ra['chain'] != rb['chain']:
            problems.append(f'hash chains diverge at tick {ra["tick"]} (step {ra["step"]})')
            break
    return problems


# What the capture frontend records at fixed addresses (experiments/ares-headless/capture.cpp:
# kTick, kPause, kSeconds, kSelector, kBanks, kRng; the unit table rows). tap.bin holds those
# bytes, so a golden can only be (re)written while the layout reads the same places.
TAP_CONTRACT = {('clock', 'tick_counter'): '0x800B1E84', ('clock', 'pause_halfword'): '0x800F0C20',
                ('clock', 'seconds'): '0x800B9368', ('clock', 'selector'): '0x800D13E4'}
TAP_BANKS, TAP_RNG, TAP_ROWS = '0x800B1D44', '0x800EFF20', (900, 312)


# The capture-time format as tap.bin holds it: the clock fields' addresses and widths, the
# unit table locator, row size and liveness rule, and the recorded bank and RNG blocks (their
# array offsets are decoded later and may change). Fixed by the frontend, not by the layout.
TAP_FORMAT_SHA256 = '131ffcd2fc7d0c7c766b75613b64c5403ef58639be236ba6d7db00fb31f127fb'


def tap_format(layout):
    doc = layout.doc

    def norm(value, key=None):   # addresses compare by value, however they are spelled
        if isinstance(value, dict):
            return {k: norm(v, k) for k, v in value.items()}
        if isinstance(value, str) and key == 'address':
            try:
                return f'0x{int(value.strip(), 16):08X}'
            except ValueError:
                return value
        return value
    if 'record_rule' not in doc['clock']:
        raise LayoutError(f'{layout.name}: the layout has no clock.record_rule; the tap format needs one')
    return norm(semantic({'clock': {k: doc['clock'][k] for k in ('tick_counter', 'pause_halfword', 'seconds', 'selector',
                                                                 'record_rule')},
                          'unit_table': doc['unit_table'],
                          'banks': {k: doc['globals']['banks'][k] for k in ('address', 'players', 'type')},
                          'rng': doc['globals']['rng']}))


def check_tap_contract(layout):
    """Raise LayoutError when the layout describes recorded fields other than tap.bin holds them."""
    doc = layout.doc
    wrong = [f'{a}.{b}' for (a, b), address in TAP_CONTRACT.items()
             if int(doc[a][b]['address'], 16) != int(address, 16)]
    if int(doc['globals']['banks']['address'], 16) != int(TAP_BANKS, 16):
        wrong.append('banks')
    if int(doc['globals']['rng']['address'], 16) != int(TAP_RNG, 16):
        wrong.append('rng')
    if (doc['unit_table']['rows'], doc['unit_table']['row_bytes']) != TAP_ROWS:
        wrong.append('unit_table')
    if wrong:
        raise LayoutError(f'{layout.name}: {", ".join(wrong)} moved from where the capture frontend records them; '
                          'tap.bin holds the old addresses, so capture again with a frontend that reads the new ones')
    digest = hashlib.sha256(json.dumps(tap_format(layout), sort_keys=True).encode()).hexdigest()
    if digest != TAP_FORMAT_SHA256:
        raise LayoutError(f'{layout.name}: the clock, unit table, bank or RNG definitions differ from the format the '
                          'capture frontend records (TAP_FORMAT_SHA256); tap.bin holds the old reads, so capture again '
                          'with a frontend that records the new ones')


def write_golden(scenario_id, runs, home, layout, note=None):
    if scenario_id not in metrics.SCENARIOS:
        raise SystemExit(f'unknown scenario {scenario_id}; known: {", ".join(metrics.SCENARIOS)}')
    check_tap_contract(layout)
    layout.used, layout.used_gating = set(), set()   # the fields this measurement reads, recorded with each observation
    caps = [Capture(run, layout) for run in runs]
    if len(caps) < 2:
        raise SystemExit('a golden bundle needs two fresh-process captures')
    if len({c.run.resolve() for c in caps}) != len(caps) or len({c.manifest.get('run_id') for c in caps}) != len(caps):
        raise SystemExit('not golden: every capture must be a different run directory with its own run id')
    spec = metrics.SCENARIOS[scenario_id]
    allow_skips = bool(spec.get('tick_skips_ok'))
    problems = []
    for other in caps[1:]:
        problems += same_bundle(caps[0], other, allow_skips)
    if problems:
        raise SystemExit('not golden: ' + '; '.join(problems))
    marks = [metrics.tape_commands(c.run / 'tape.txt') for c in caps]
    if any(m != marks[0] for m in marks[1:]):
        raise SystemExit(f'not golden: tape marks resolve to different steps {marks}')
    tape_steps = marks[0]
    # The chain covers unit rows, not the cursor: every mark must have been given at the same
    # world point in each run. A run without a verified point beside one with it is refused;
    # tapes that never watched the cursor (all runs without) are left to their measurement.
    def watches_view(words):
        # Any watch of a camera or cursor word, however its width is spelled: the frontend takes
        # a physical offset or its KSEG0/KSEG1 alias, any case, optional 0x. world_point() then
        # uses only full-width samples.
        if len(words) < 2 or words[0] != 'watch':
            return False
        text = words[1][2:] if words[1][:2].lower() == '0x' else words[1]
        try:
            value = int(text, 16)
        except ValueError:
            return False
        return value & 0x1FFFFFFF in (0x000A2EE8, 0x000A2EEC, 0x000AFDCC, 0x000AFDCE) and value >> 29 in (0, 4, 5)
    tapes = [list(metrics.frontend_lines((c.run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')))
             for c in caps]
    # Only scenarios whose measurement replays the clicked points need them verified; other tapes
    # may watch the view (or clear their watches) for diagnosis.
    uses_points = spec.get('world_points', False)
    watched = [uses_points and any(watches_view(w) for w in words) for words in tapes]
    if any(watched) and any(w[:2] == ['watch', 'off'] for words in tapes for w in words):
        raise SystemExit('not golden: a tape that watches the cursor clears its watches (`watch off`), '
                         'so its logged points cannot be tied to the marks')
    for name, step in tape_steps.items():
        if not uses_points:
            break
        if not any(watched):
            break   # a tape that never watched the cursor (the S4 tapes): nothing to compare
        points = []
        for c in caps:
            try:
                points.append(c.world_point(step))
            except ValueError:
                points.append(None)
        if None in points or any(p != points[0] for p in points):
            raise SystemExit(f'not golden: mark {name} has no single verified world point across the runs {points}')
    result = spec['measure_n64'](caps[0], tape_steps)
    if allow_skips and any(obs['gating'] for obs in result['observations']):
        raise SystemExit(f'not golden: {scenario_id} admits tick skips, so none of its observations may gate')
    # A tape that writes guest memory (`poke`, owner decision 2026-11-26) stages its
    # situation: its evidence is labelled n64-staged, never plain n64-ram.
    # A compiled tape whose start state came from such a run says so in a `# n64-staged` line.
    tape_text = (caps[0].run / 'tape.txt').read_bytes().decode('utf-8', 'surrogateescape')
    staged = (any(w[0] == 'poke' for w in metrics.frontend_lines(tape_text)) or
              any(line.startswith('# n64-staged') for line in tape_text.split('\n')) or
              any(c.manifest.get('pokes') or c.manifest.get('n64_staged') for c in caps))
    for obs in result['observations']:
        obs['n_runs'] = len(caps)
        obs['provenance'] = 'n64-staged' if staged else 'n64-ram'
        obs['layout_fields'] = sorted(set(obs.get('layout_fields', [])))
        if obs['gating']:
            layout.require_all(obs['layout_fields'])
        # Every field the measurement read (qualification checks included) joins the digest.
        obs['layout_reads'] = sorted(set(obs['layout_fields']) | layout.used)
        if obs['gating']:
            # Every field read under the proven rule must stay proven for the check to gate.
            obs['layout_proven'] = sorted(set(obs['layout_fields']) | layout.used_gating)
        obs['layout_fields_sha256'] = layout.fields_digest(obs['layout_reads'])
        if staged:
            obs['tags'] = sorted(set(obs.get('tags', [])) | {'n64-staged'})
    golden = {
        'format': 'sc64-parity-golden/1', 'id': scenario_id, 'scenario': spec['title'],
        'layout': layout.name, 'layout_sha256': layout.sha256,
        'timebase': 'source frames: N64 unpaused ticks; GBA simulation steps * 4 / 5',
        'observations': result['observations'], 'curves': result.get('curves', {}),
        'gba': result['gba'], 'notes': result.get('notes', []) + ([note] if note else []),
        # A capture that wrote guest memory ("poke", owner decision 2026-11-26) is staged evidence.
        'tags': ['n64-staged'] if staged else [],
        'decoder_revision': spec.get('decoder_revision', 1),
        'bundle': {
            'runs': [c.run.name for c in caps], 'chain': caps[0].chain, 'seed': caps[0].seed(),
            'run_ids': [c.manifest['run_id'] for c in caps],
            'binary_sha256': caps[0].manifest['binary_sha256'], 'tape_sha256': caps[0].manifest['tape_sha256'],
            'rom_sha256': caps[0].manifest['rom_sha256'], 'mode': caps[0].manifest['mode'],
            'selectors': sorted({r['selector'] for r in caps[0].ticks if r['table']}),
            'records': len(caps[0].ticks), 'faults': caps[0].faults,
            # Skipped ticks advance the clock from their change-step reads, which the chain does
            # not cover: record where each fell, so equal chains with other timings differ here.
            'skipped_frames': [list(x) for x in caps[0].skipped_frames],
            # v2: extra-region captures also hash their regions, region list and table offset.
            'chain_format': 2 if caps[0].regions else 1,
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
