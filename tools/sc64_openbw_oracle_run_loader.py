#!/usr/bin/env python3
"""Run the local OpenBW loader only on the authenticated private script.

SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse
import hashlib
import subprocess
from pathlib import Path

EXPECTED = 'f8e5f3d0fcca644470c37de65486e44b09eb30141eedccea3445afe7bc0f65bc'

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--script', type=Path, required=True)
    p.add_argument('--probe', type=Path, required=True)
    a = p.parse_args()
    if hashlib.sha256(a.script.read_bytes()).hexdigest() != EXPECTED:
        p.error('IScript authentication failed')
    subprocess.run([str(a.probe.resolve()), str(a.script.resolve())], check=True)

if __name__ == '__main__':
    main()
