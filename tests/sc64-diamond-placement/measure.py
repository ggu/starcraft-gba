# SPDX-License-Identifier: GPL-3.0-or-later
"""Measure the isolated primitive in two valid ROMs using read-only debugger commands."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

EXPECTED_RUNNER = '0ee4cae88eea4dca4c744ab771fb5fa0b630def593f970c0ca89cc41594cb74d'
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def symbols(elf):
    rows = subprocess.check_output(['arm-none-eabi-nm', '-S', '--defined-only', str(elf)], text=True)
    return {p[3]: int(p[0], 16) for line in rows.splitlines() if len(p := line.split()) == 4}
def run(runner, directory, variant):
    elf, rom = directory / (variant + '.elf'), directory / (variant + '.gba')
    sym = symbols(elf)
    pc = sym['diamond_bench_done'] & ~1
    commands = f"Z{pc:x}\nc\nm{sym['bench_done_flag']:x},4\nm{sym['bench_cycles']:x},16\nm{sym['bench_checksum']:x},16\nq\n"
    home = directory / ('home-' + variant)
    home.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, HOME=str(home.resolve()), XDG_CONFIG_HOME=str((home / '.config').resolve()))
    proc = subprocess.run([str(runner.resolve()), str(rom.resolve()), '2000000', '4', '2000004', '4', '2000008', '4', '200000c', '4', '2000010', '4'], input=commands,
                          capture_output=True, text=True, timeout=60, env=env, check=True)
    (directory / (variant + '.runner.log')).write_text(proc.stderr)
    lines = proc.stdout.splitlines()
    if len(lines) != 6 or lines[:3] != ['READY', 'OK', f'STOP {pc:08x}']:
        raise RuntimeError(f'unexpected runner output: {lines}')
    if struct.unpack('<I', bytes.fromhex(lines[3])) != (1,):
        raise RuntimeError('benchmark did not finish')
    return {'elf_sha256': sha(elf), 'rom_sha256': sha(rom),
            'cycles': struct.unpack('<4I', bytes.fromhex(lines[4])),
            'pixel_checksums': struct.unpack('<4I', bytes.fromhex(lines[5]))}
def main():
    p = argparse.ArgumentParser(); p.add_argument('--runner', type=Path, required=True)
    p.add_argument('--dir', type=Path, default=Path('.local/diamond-bench')); a = p.parse_args()
    if sha(a.runner) != EXPECTED_RUNNER: raise RuntimeError('runner hash mismatch')
    results = {name: run(a.runner, a.dir, name) for name in ('iwram', 'rom')}
    if results['iwram']['pixel_checksums'] != results['rom']['pixel_checksums']:
        raise RuntimeError('pixel checksum mismatch')
    report = {'runner_sha256': EXPECTED_RUNNER, 'counts': [1, 32, 128, 128],
              'note': 'Isolated synthetic primitive batches; final batch is scene-clipped. Not whole-scene FPS.',
              'results': results}
    (a.dir / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
if __name__ == '__main__': main()
