#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read a completed diagnostic through the port-free runner; never writes guest RAM."""
import argparse, json, struct, subprocess

p = argparse.ArgumentParser()
p.add_argument('--runner', required=True)
p.add_argument('--rom', required=True)
p.add_argument('--elf', required=True)
p.add_argument('--out', required=True, help='new private JSON file')
a = p.parse_args()
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
    if proc.stdout.readline().strip() != 'READY': raise RuntimeError('runner not ready')
    def command(value):
        proc.stdin.write(value+'\n'); proc.stdin.flush()
        return proc.stdout.readline().strip()
    if command(f"Z{symbols['diag_done']:x}") != 'OK': raise RuntimeError('breakpoint failed')
    if not command('c').startswith('STOP '): raise RuntimeError('replay did not finish')
    def read(addr, size):
        return bytes.fromhex(command(f'm{addr:x},{size}'))
    totals = struct.unpack('<17Q', read(symbols['totals'], 17*8))
    peaks = struct.unpack('<17I', read(symbols['peaks'], 17*4))
    data = b''.join(read(symbols['checkpoints']+i, min(512, 1440-i))
                    for i in range(0, 1440, 512))
    with open(a.out, 'x') as out:
        json.dump({'totals': totals, 'peaks': peaks,
                   'checkpoints': list(struct.iter_unpack('<4I', data))}, out, indent=2)
    command('q')
finally:
    proc.terminate()
    proc.wait()
