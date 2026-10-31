#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read a completed diagnostic through the port-free runner; never writes guest RAM."""
import argparse, json, select, struct, subprocess
from pathlib import Path
from sc64_headless_replay import HeadlessReplay

p = argparse.ArgumentParser()
p.add_argument('--runner', required=True)
p.add_argument('--rom', required=True)
p.add_argument('--elf', required=True)
p.add_argument('--out', required=True, help='new private JSON file')
a = p.parse_args()
if Path(a.out).exists():
    p.error('output must be a new private file')
nm = subprocess.check_output(['arm-none-eabi-nm', '-n', a.elf], text=True)
symbols = {parts[2]: int(parts[0], 16) for line in nm.splitlines()
           if len(parts := line.split()) == 3}
# The transport demands five nonoverlapping EWRAM allowlist regions. Only the
# first two are real scheduler symbols; the remaining gaps are inert. This
# reader never issues its write command, including to the scheduler.
keys, frame = symbols['demo_keys'], symbols['demo_frame']
args = [a.runner, a.rom, f'{keys:x}', '2', f'{frame:x}', '4',
        f'{frame+4:x}', '1', f'{frame+5:x}', '1', f'{frame+6:x}', '1']
proc = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                        text=True, bufsize=1)
try:
    if not select.select([proc.stdout], [], [], 60)[0] or proc.stdout.readline().strip() != 'READY':
        raise RuntimeError('runner not ready within 60 seconds')
    replay = HeadlessReplay(proc)
    done = symbols['diag_done'] & ~1
    replay.breakpoint(done, True)
    replay.tick(expected=done)
    totals = struct.unpack('<17Q', replay.read(symbols['totals'], 17*8))
    peaks = struct.unpack('<17I', replay.read(symbols['peaks'], 17*4))
    data = replay.read(symbols['checkpoints'], 1440)
    with open(a.out, 'x') as out:
        json.dump({'totals': totals, 'peaks': peaks,
                   'checkpoints': list(struct.iter_unpack('<4I', data))}, out, indent=2)
    proc.stdin.write('q\n'); proc.stdin.flush()
finally:
    if proc.poll() is None:
        proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
