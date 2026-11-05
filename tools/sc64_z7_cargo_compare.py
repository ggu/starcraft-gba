#!/usr/bin/env python3
"""Check every Z7 scalar observation has a matching host/ARM cargo digest."""
import argparse
import re
from pathlib import Path

CHECK = re.compile(r'Z7 CHECK (\w+) host_tick=(\d+)')
CARGO = re.compile(r'CARGO7 (\w+) tick=(\d+) digest=(\d+) end=1(?:\s|$)')

def read(path):
    checks, rows = [], []
    for line in Path(path).read_text().splitlines():
        if match := CHECK.search(line):
            checks.append((match[1], int(match[2])))
        if 'CARGO7 ' in line:
            match = CARGO.search(line)
            if not match or int(match[3]) > 0xffffffff:
                raise ValueError(f'malformed cargo record: {line}')
            rows.append((match[1], int(match[2]), int(match[3])))
    keys = [(kind, tick) for kind, tick, _ in rows]
    if not checks or checks != keys or len(set(keys)) != len(keys):
        raise ValueError(f'{path}: missing, repeated, or out-of-order cargo observations')
    return rows

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('host')
    parser.add_argument('arm')
    args = parser.parse_args()
    host, arm = read(args.host), read(args.arm)
    if host != arm:
        raise ValueError('host and ARM cargo observations differ')
    print(f'{len(host)} scalar checkpoints and cargo digests match')

if __name__ == '__main__':
    main()
