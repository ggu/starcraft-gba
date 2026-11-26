#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the parity capture spike frontend on one tape in a fresh process.

The tape is passed to the child's stdin line by line. Lines are frontend
commands ("steps mask x y", "hash", "every N", "tap off|tick|step [raw [K]] [rec]",
"verify N branch|same [full]", "mode jit|interp", "watch ADDR BYTES|off",
"until ADDR BYTES OP VALUE MAX MASK X Y [HOLD GAP]", "save NAME.state",
"load NAME [STEP]",
"flush", "peek ADDR N", "dump NAME", "shot NAME", "quit"); see CAPTURE.md.
State files given with --state-input are copied into the run's input/
folder, where "load" finds them. Outputs are ROM-derived: keep them in an
ignored private directory.
"""
import argparse
import fcntl
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import time
import uuid

ROM_SHA = 'e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
MOLTENVK = pathlib.Path('/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib')
MOLTENVK_SHA = '432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99'
ARES_COMMIT = '0aafd85789215e84e1e43415c07d4c88461b7899'
# The shared native lock serialises captures and native builds across sessions. A session may
# name its own lock in SC64_CAPTURE_LOCK to run its captures alongside others': emulation is
# deterministic under host load (S1-S3), but compare such a run with a solo one before relying on it.
LOCK = os.environ.get('SC64_CAPTURE_LOCK') or '/tmp/sc64-native-build-20261031.lock'
FLASH_BYTES = 128 * 1024
RESOURCES = ('ares/ares/resource/resource.cpp', 'ares/ares/resource/resource.hpp',
             'mia/resource/resource.cpp', 'mia/resource/resource.hpp')
OUTPUTS = ('checkpoints.csv', 'regions.csv', 'ticks.csv', 'verify.csv', 'events.csv',
           'commands.csv', 'status.log', 'polls.bin', 'ticks.raw', 'tap.bin', 'watch.csv', 'extra.bin')
STATE_LIMIT = 16 * 1024 * 1024  # a synchronized N64 state is 8,512,920 bytes
NAME_CHARS = set('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-')


def sha(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


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


def check_loads(tape_text, names):
    """Every `load` the frontend will run must name a staged state exactly: the run directory
    may sit on a case-insensitive volume, where a different spelling would still open."""
    for words in frontend_lines(tape_text):
        if len(words) >= 2 and words[0] == 'load' and words[1] not in names:
            raise SystemExit('the tape loads a state not given exactly with --state-input')


def hex64(value):
    return isinstance(value, str) and len(value) == 64 and all(c in '0123456789abcdef' for c in value)


def state_source(state, digest, legacy_producer=False):
    """The manifest of the capture that wrote `state`, if it records this digest under this
    name and that capture finished cleanly; otherwise None."""
    try:
        producer = json.loads((state.parent / 'manifest.json').read_text())
    except (OSError, ValueError):
        return None
    if not isinstance(producer, dict) or producer.get('ok') is not True or producer.get('unrecorded_state'):
        return None
    # The producer's own identity: its binary always, its run id unless it predates run ids
    # and the caller asked for that explicitly.
    run_id = producer.get('run_id')
    if not hex64(producer.get('binary_sha256')):
        return None
    if not (isinstance(run_id, str) and run_id or (legacy_producer and run_id is None)):
        return None   # the legacy switch covers a missing run id only, never a malformed one
    states = producer.get('states_sha256')
    if not isinstance(states, dict) or states.get(state.name) != digest:
        return None
    # A producer that itself loaded states must have recorded where they came from; otherwise
    # its output rests on an unproven input. Its own tape (checked against its manifest) says
    # which states it actually loaded.
    tape = state.parent / 'tape.txt'
    if not tape.is_file() or producer.get('tape_sha256') != sha(tape):
        return None
    loaded = [w[1] for w in frontend_lines(tape.read_bytes().decode('utf-8', 'surrogateescape'))
              if w[0] == 'load' and len(w) >= 2]
    inputs, sources = producer.get('state_input_sha256'), producer.get('state_input_source')
    if loaded and not (isinstance(inputs, dict) and isinstance(sources, dict) and all(
            hex64(inputs.get(name)) and producer_ok(sources.get(name)) for name in loaded)):
        return None
    return producer


def producer_ok(source):
    """A recorded state producer: a nonempty run id string and a 64-hex binary digest."""
    if not isinstance(source, dict):
        return False
    run_id, binary = source.get('run_id'), source.get('binary_sha256')
    return isinstance(run_id, str) and bool(run_id) and hex64(binary)


def state_recorded(state, digest, legacy_producer=False):
    return state_source(state, digest, legacy_producer) is not None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('binary', 'source', 'rom', 'tape', 'output'):
        parser.add_argument('--' + name, type=pathlib.Path, required=True)
    parser.add_argument('--flash-input', type=pathlib.Path)
    parser.add_argument('--state-input', type=pathlib.Path, action='append', default=[],
                        help='state file for the tape\'s "load" command (repeatable)')
    parser.add_argument('--legacy-producer', action='store_true',
                        help='stage states from a capture made before run ids (its source then records no run id)')
    parser.add_argument('--unrecorded-state', action='store_true',
                        help='stage state files that no capture manifest records (for refusal tests only; '
                             'the manifest then records unrecorded_state and ok is false)')
    parser.add_argument('--mode', choices=('jit', 'interp'), default='jit')
    parser.add_argument('--timeout', type=int, default=6 * 3600)
    parser.add_argument('--reserve-gib', type=int, default=5)
    parser.add_argument('--output-budget-gib', type=int, default=8,
                        help='most the frontend may write into the run directory; it stops with exit 10 past it, '
                             'or before free space falls below --reserve-gib')
    args = parser.parse_args()

    def provenance():
        head = subprocess.check_output(['git', '-C', str(args.source), 'rev-parse', 'HEAD'], text=True).strip()
        dirty = subprocess.check_output(['git', '-C', str(args.source), 'status', '--porcelain', '--untracked-files=no'], text=True).strip()
        if head != ARES_COMMIT or dirty:
            raise SystemExit('ares source must be a clean checkout of ' + ARES_COMMIT)
        if sha(args.rom) != ROM_SHA or sha(MOLTENVK) != MOLTENVK_SHA:
            raise SystemExit('ROM or MoltenVK provenance mismatch')
    if args.output.exists():
        raise SystemExit('output must be new')
    if args.flash_input and (not args.flash_input.is_file() or args.flash_input.stat().st_size != FLASH_BYTES):
        raise SystemExit('flash input must be a 128 KiB file')
    names = [state.name for state in args.state_input]
    for state in args.state_input:
        if not state.is_file() or state.name.startswith('.') or not set(state.name) <= NAME_CHARS or len(state.name) > 64:
            raise SystemExit('state input must be a file with a plain name: ' + str(state))
    for state in args.state_input:
        if state.stat().st_size > STATE_LIMIT:
            raise SystemExit(f'state input larger than {STATE_LIMIT} bytes: {state}')
    check_loads(args.tape.read_bytes().decode('utf-8', 'surrogateescape'), names)
    for state in args.state_input:   # checked again on the staged copy, under the lock
        if not args.unrecorded_state and not state_recorded(state, sha(state), args.legacy_producer):
            raise SystemExit(f'state input is not recorded by a clean producer manifest: {state} '
                             '(a state from a capture made before run ids needs --legacy-producer)')
    # Compare case-folded: the run directory may sit on a case-insensitive volume.
    folded = [name.casefold() for name in names]
    if len(set(folded)) != len(folded) or 'game.z64' in folded:
        raise SystemExit('state input names must be distinct, ignoring case, and not game.z64')
    provenance()
    frontend = pathlib.Path(__file__).resolve().parent
    sources = {name: sha(frontend / name) for name in ('capture.cpp', 'capture_view.h', 'inject.cmake', 'capture.py')}
    with open(LOCK, 'a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        provenance()
        parent = args.output.resolve().parent
        parent.mkdir(parents=True, exist_ok=True)
        if shutil.disk_usage(parent).free < args.reserve_gib * 1024 ** 3:
            raise SystemExit('disk reserve')
        out = args.output
        for sub in ('input', 'saves', 'systems'):
            (out / sub).mkdir(parents=True)
        rom = out / 'input' / 'game.z64'
        shutil.copyfile(args.rom, rom)
        tape_copy = out / 'tape.txt'
        shutil.copyfile(args.tape, tape_copy)
        check_loads(tape_copy.read_bytes().decode('utf-8', 'surrogateescape'), names)   # the tape bytes the child will read
        states, state_sources, legacy_sources, staged_inputs = {}, {}, {}, []
        for state in args.state_input:
            staged = out / 'input' / state.name
            shutil.copyfile(state, staged)
            if staged.stat().st_size > STATE_LIMIT:
                raise SystemExit(f'state input grew past {STATE_LIMIT} bytes while staging: {state}')
            states[state.name] = sha(staged)
            # The core trusts a state's payload, so only states a capture wrote are staged: the
            # manifest beside the source file must record this digest under this name. The same
            # read supplies the provenance recorded below.
            producer = state_source(state, states[state.name], args.legacy_producer)
            if producer is not None and (producer.get('pokes') or producer.get('n64_staged')):
                staged_inputs.append(state.name)   # the state carries the producer's guest edits
            if not args.unrecorded_state and producer is None:
                raise SystemExit(f'state input is not recorded by a clean producer manifest: {state} '
                             '(a state from a capture made before run ids needs --legacy-producer)')
            if producer is not None and not producer.get('run_id'):
                # A producer from before run ids: its provenance is kept apart, so the per-state
                # source stays absent and the reports treat the run as legacy evidence.
                legacy_sources[state.name] = {'binary_sha256': producer.get('binary_sha256')}
            else:
                state_sources[state.name] = {k: (producer or {}).get(k) for k in ('run_id', 'binary_sha256')}
        if sha(rom) != ROM_SHA:
            raise SystemExit('staged ROM changed while staging inputs')
        flash_sha = None
        if args.flash_input:
            flash = out / 'saves' / 'Nintendo 64' / 'game.flash'
            flash.parent.mkdir()
            shutil.copyfile(args.flash_input, flash)
            flash_sha = sha(flash)
        manifest = {
            'binary_sha256': sha(args.binary), 'frontend_sha256': sources, 'ares_commit': ARES_COMMIT,
            'generated_resource_sha256': {r: sha(args.source / r) for r in RESOURCES},
            'rom_sha256': ROM_SHA, 'moltenvk_sha256': MOLTENVK_SHA, 'tape_sha256': sha(tape_copy),
            'flash_input_sha256': flash_sha, 'state_input_sha256': states, 'mode': args.mode,
            # Where each staged state came from: the capture that wrote it (run id, binary).
            'state_input_source': state_sources,
            # A fresh id per process, so a copied run directory never counts as a second capture.
            'run_id': str(uuid.uuid4()),
        }
        command = [str(args.binary.resolve()), str(rom.resolve()), str(out.resolve()), args.mode]
        started = time.monotonic()
        env = {**os.environ, 'DYLD_LIBRARY_PATH': str(MOLTENVK.parent),
               'SC64_CAPTURE_BUDGET_BYTES': str(args.output_budget_gib * 1024 ** 3),
               'SC64_CAPTURE_RESERVE_BYTES': str(args.reserve_gib * 1024 ** 3)}
        manifest['output_budget_bytes'] = int(env['SC64_CAPTURE_BUDGET_BYTES'])
        with open(tape_copy, 'rb') as tape, open(out / 'bridge.log', 'wb') as log, open(out / 'stderr.log', 'wb') as err:
            child = subprocess.Popen(command, cwd=out.resolve(), env=env, stdin=tape, stdout=log, stderr=err)
            try:
                manifest['returncode'] = child.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                child.kill()
                manifest['returncode'] = child.wait()
                manifest['timeout'] = True
        manifest['seconds'] = round(time.monotonic() - started, 3)
        manifest['inputs_unchanged'] = (sha(args.binary) == manifest['binary_sha256'] and sha(rom) == ROM_SHA and
                                        all(sha(out / 'input' / name) == digest for name, digest in states.items()))
        manifest['outputs_sha256'] = {name: sha(out / name) for name in OUTPUTS if (out / name).is_file()}
        # State files written by "save": each is recorded only if it still has the size and
        # digest the frontend logged when it wrote it (events.csv: "STEP,save,NAME SIZE SHA").
        events = out / 'events.csv'
        saved, mismatched = {}, []
        for line in (events.read_text().split('\n')[1:] if events.is_file() else []):
            fields = line.split(',', 2)
            if len(fields) == 3 and fields[1] == 'save' and len(fields[2].split(' ')) == 3:
                name, size, digest = fields[2].split(' ')
                path = out / name
                if path.is_file() and str(path.stat().st_size) == size and sha(path) == digest:
                    saved[name] = digest
                else:
                    mismatched.append(name)
        manifest['states_sha256'] = saved
        # Guest writes ("poke"): a run with any is staged evidence (fixtures tagged n64-staged).
        pokes = [line.split(',', 2)[2] for line in (events.read_text().split('\n')[1:] if events.is_file() else [])
                 if line.split(',', 2)[1:2] == ['poke']]
        manifest['pokes'] = len(pokes)
        if pokes or staged_inputs:
            manifest['n64_staged'] = True
        if staged_inputs:
            manifest['n64_staged_inputs'] = staged_inputs
        if mismatched:
            manifest['states_changed_after_save'] = mismatched
        flash_out = out / 'saves' / 'Nintendo 64' / 'game.flash'
        # The frontend flushes the cartridge save only on a "flush" command; otherwise this is the staged input copy.
        manifest['flash_file_after_sha256'] = sha(flash_out) if flash_out.is_file() else None
        if legacy_sources:
            # States from captures made before run ids (--legacy-producer): the producer binary is
            # recorded here; the reports treat the run as legacy evidence.
            manifest['legacy_producer'] = legacy_sources
        if args.unrecorded_state:
            # A refusal test only: such a run is never evidence, so it is never ok.
            manifest['unrecorded_state'] = True
        manifest['ok'] = (manifest['returncode'] == 0 and manifest['inputs_unchanged'] and not manifest.get('timeout')
                          and not args.unrecorded_state
                          and not manifest.get('states_changed_after_save'))
        (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        print(json.dumps({'ok': manifest['ok'], 'returncode': manifest['returncode'], 'seconds': manifest['seconds']}))
        return 0 if manifest['ok'] else 1


if __name__ == '__main__':
    sys.exit(main())
