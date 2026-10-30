#!/usr/bin/env python3
"""Prepare an ignored include containing the exact pinned OpenBW IScript loader.

SPDX-License-Identifier: GPL-3.0-or-later
No game data is read or emitted. The reference checkout is read-only.
"""
import argparse
import hashlib
from pathlib import Path

PINNED_SHA256 = '5ee316ca2c008c9252f7fde7444ca5dc7f35440549be2576dc37fa228fb31ae6'
START = b'\tauto load_iscript_bin = [&]() {'
END = b'\n\tauto load_images = [&]() {'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--bwgame', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    local = (root / '.local').resolve()
    if not out.is_relative_to(local):
        parser.error('output must be under this worktree .local')
    data = args.bwgame.read_bytes()
    if hashlib.sha256(data).hexdigest() != PINNED_SHA256:
        parser.error('bwgame.h does not match the pinned source hash')
    if data.count(START) != 1 or data.count(END) != 1:
        parser.error('loader anchors are missing or ambiguous')
    start = data.index(START)
    end = data.index(END, start)
    fragment = data[start:end]
    if not fragment.rstrip().endswith(b'};'):
        parser.error('unexpected loader ending')
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(fragment + b'\n\tload_iscript_bin();\n')
    print('source_sha256=' + PINNED_SHA256)
    print('fragment_sha256=' + hashlib.sha256(fragment).hexdigest())

if __name__ == '__main__':
    main()
