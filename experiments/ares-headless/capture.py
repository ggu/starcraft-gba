#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the parity capture spike frontend on one tape in a fresh process.

The tape is passed to the child's stdin line by line. Lines are frontend
commands ("steps mask x y", "hash", "every N", "tap off|tick|step [raw]",
"verify N interp|same|serialize [full]", "mode jit|interp", "peek ADDR N",
"dump NAME", "shot NAME", "quit"). Outputs are ROM-derived: keep them in an
ignored private directory.
"""
import argparse
import fcntl
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys
import time
import uuid

ROM_SHA = 'e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
MOLTENVK = pathlib.Path('/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib')
MOLTENVK_SHA = '432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99'
ARES_COMMIT = '0aafd85789215e84e1e43415c07d4c88461b7899'
LOCK = '/tmp/sc64-native-build-20261031.lock'
FLASH_BYTES = 128 * 1024
RESOURCES = ('ares/ares/resource/resource.cpp', 'ares/ares/resource/resource.hpp',
             'mia/resource/resource.cpp', 'mia/resource/resource.hpp')
OUTPUTS = ('checkpoints.csv', 'regions.csv', 'ticks.csv', 'verify.csv', 'events.csv',
           'commands.csv', 'status.log', 'polls.bin', 'ticks.raw', 'tap.bin')


def sha(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('binary', 'source', 'rom', 'tape', 'output'):
        parser.add_argument('--' + name, type=pathlib.Path, required=True)
    parser.add_argument('--flash-input', type=pathlib.Path)
    parser.add_argument('--mode', choices=('jit', 'interp'), default='jit')
    parser.add_argument('--timeout', type=int, default=6 * 3600)
    parser.add_argument('--reserve-gib', type=int, default=5)
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
            'flash_input_sha256': flash_sha, 'mode': args.mode,
            # A fresh id per process, so a copied run directory never counts as a second capture.
            'run_id': str(uuid.uuid4()),
        }
        command = [str(args.binary.resolve()), str(rom.resolve()), str(out.resolve()), args.mode]
        started = time.monotonic()
        env = {**os.environ, 'DYLD_LIBRARY_PATH': str(MOLTENVK.parent)}
        with open(tape_copy, 'rb') as tape, open(out / 'bridge.log', 'wb') as log, open(out / 'stderr.log', 'wb') as err:
            child = subprocess.Popen(command, cwd=out.resolve(), env=env, stdin=tape, stdout=log, stderr=err)
            try:
                manifest['returncode'] = child.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                child.kill()
                manifest['returncode'] = child.wait()
                manifest['timeout'] = True
        manifest['seconds'] = round(time.monotonic() - started, 3)
        manifest['inputs_unchanged'] = sha(args.binary) == manifest['binary_sha256'] and sha(rom) == ROM_SHA
        manifest['outputs_sha256'] = {name: sha(out / name) for name in OUTPUTS if (out / name).is_file()}
        flash_out = out / 'saves' / 'Nintendo 64' / 'game.flash'
        # The frontend does not flush cartridge saves on exit, so this is the staged input copy.
        manifest['flash_file_after_sha256'] = sha(flash_out) if flash_out.is_file() else None
        manifest['ok'] = manifest['returncode'] == 0 and manifest['inputs_unchanged'] and not manifest.get('timeout')
        (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
        print(json.dumps({'ok': manifest['ok'], 'returncode': manifest['returncode'], 'seconds': manifest['seconds']}))
        return 0 if manifest['ok'] else 1


if __name__ == '__main__':
    sys.exit(main())
