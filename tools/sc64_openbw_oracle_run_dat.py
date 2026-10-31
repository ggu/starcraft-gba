#!/usr/bin/env python3
"""Authenticate staged private tables, then invoke the OpenBW DAT parser probe.

SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse
import hashlib
import subprocess
from pathlib import Path
from sc64_openbw_oracle_tables import TABLES

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--tables',type=Path,required=True)
    p.add_argument('--probe',type=Path,required=True)
    a=p.parse_args()
    for name, (_, expected) in TABLES.items():
        if hashlib.sha256((a.tables / name).read_bytes()).hexdigest() != expected:
            p.error('table authentication failed: ' + name)
    subprocess.run([str(a.probe.resolve()), str(a.tables.resolve())], check=True)

if __name__ == '__main__': main()
