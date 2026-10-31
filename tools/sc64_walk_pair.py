#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify that prepared walk poses reference the facing bank used to make them."""
import argparse
import hashlib
from pathlib import Path
import re

# Historical pair retained from the walk-cycle qualification before the
# facing digest was embedded. Other unmarked banks must be re-prepared.
LEGACY_PAIRS = frozenset({
    ('0b1172629a22c559322ff181d8b4551e01d7199328c3a675501dc170c9380415',
     'ade3d8f7126dfd737eac3d62f58506ada32476efcbc89b6f4096b900b83faed2'),
})
MARKER = re.compile(rb'^#define SC64_UNIT_WALK_FACING_SHA256 "([0-9a-f]{64})"$', re.M)

def verify(walk: bytes, facing: bytes) -> bool:
    walk_hash = hashlib.sha256(walk).hexdigest()
    facing_hash = hashlib.sha256(facing).hexdigest()
    markers = MARKER.findall(walk)
    if len(markers) == 1:
        return markers[0].decode('ascii') == facing_hash
    if markers or b'SC64_UNIT_WALK_FACING_SHA256' in walk:
        return False
    return (walk_hash, facing_hash) in LEGACY_PAIRS

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('walk', type=Path);parser.add_argument('facing', type=Path)
    args=parser.parse_args()
    if not verify(args.walk.read_bytes(), args.facing.read_bytes()):
        raise SystemExit('walk/facing banks do not match; re-prepare both from the same source')
    print(hashlib.sha256(args.walk.read_bytes()).hexdigest() + ':' + hashlib.sha256(args.facing.read_bytes()).hexdigest())
if __name__ == '__main__':main()
