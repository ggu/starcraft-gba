#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Derive the narrow interpreter hook for a disposable pinned ares checkout."""
import argparse
from pathlib import Path
import subprocess

CPU = 'ares/n64/cpu/cpu.cpp'
BEFORE = b'  instructionPrologue(ipu.pc, *data);\n'
AFTER = BEFORE + b'''  // Optional SC64 read-only probe; interpreter path only.
  switch ((u32)ipu.pc) {
  case 0x8008db2c: case 0x8008db58: case 0x8008dbbc:
  case 0x8008d9a8: case 0x801357a4: case 0x8008dbec: case 0x8008ce0c:
    ::sc64_result_before_instruction((u32)ipu.pc, (u32)*data);
  }
'''
DECL = b'extern "C" void sc64_result_before_instruction(unsigned, unsigned);\n'


def expected(source, revision):
    raw = subprocess.check_output(['git', '-C', str(source), 'show', f'{revision}:{CPU}'])
    if raw.count(BEFORE) != 1:
        raise ValueError('unexpected interpreter source')
    return DECL + raw.replace(BEFORE, AFTER)


def matches(source, revision, patched):
    head = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    status = subprocess.check_output(['git', '-C', str(source), 'status', '--porcelain'], text=True)
    if head != revision:
        return False
    if not patched:
        return not status
    return status == f' M {CPU}\n' and (source / CPU).read_bytes() == expected(source, revision)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('disposable_checkout', type=Path)
    parser.add_argument('--revision', default='0aafd85789215e84e1e43415c07d4c88461b7899')
    args = parser.parse_args()
    if not matches(args.disposable_checkout, args.revision, False):
        parser.error('expected a clean disposable checkout at the pinned revision')
    (args.disposable_checkout/CPU).write_bytes(expected(args.disposable_checkout, args.revision))
