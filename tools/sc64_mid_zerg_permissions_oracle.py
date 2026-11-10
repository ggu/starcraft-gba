#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare the compiled private Z5/Z6 accessor against authenticated PUNI."""
from __future__ import annotations
import argparse
import fcntl
import hashlib
import os
from pathlib import Path
import resource
import shutil
import signal
import subprocess
import tempfile
from tools.sc64_zp_scenario_pack import parse_source

ROOT=Path(__file__).resolve().parents[1]
LOCK='/tmp/sc64-native-build-20261031.lock'

def digest(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()

def run(source: Path, bank: Path) -> int:
    files=sorted(p for n in ('z5','z6') for p in (source/n).rglob('*') if p.is_file())
    inputs={p:digest(p) for p in files}
    header=bank/'sc64_mid_zerg_permissions_generated.h';inputs[header]=digest(header)
    fixture=ROOT/'tests/sc64_mid_zerg_permissions_oracle.c';inputs[fixture]=digest(fixture)
    def limits():
        resource.setrlimit(resource.RLIMIT_FSIZE,(67108864,67108864))
        os.setsid()
    def bounded(command: list[str]) -> bytes:
        child=subprocess.Popen(command,cwd=ROOT,stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,preexec_fn=limits)
        try:
            stdout,stderr=child.communicate(timeout=120)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid,signal.SIGKILL)
            child.communicate()
            raise
        if child.returncode:
            raise subprocess.CalledProcessError(child.returncode,command,stdout,stderr)
        return stdout
    with open(LOCK,'w') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX)
        if shutil.disk_usage(ROOT).free<4*1024**3: raise RuntimeError('less than 4 GiB free')
        if any(digest(p)!=h for p,h in inputs.items()): raise ValueError('input changed before run')
        with tempfile.TemporaryDirectory(prefix='mid-zerg-oracle-',dir=ROOT/'.local') as temp:
            exe=Path(temp)/'oracle'
            bounded(['clang','-std=c11','-O1','-fsanitize=address,undefined',
                     '-I'+str(bank),str(fixture),'-o',str(exe)])
            observed=bounded([str(exe)])
            expected=bytearray()
            for mission in ('Z5','Z6'):
                puni=parse_source(source,mission)['puni']
                for player in range(12):
                    for raw in range(228):
                        index=player*228+raw
                        value=puni[2736+raw] if puni[2964+index] else puni[index]
                        expected.append(49 if value else 48)
            if observed!=expected or len(observed)!=5472: raise ValueError('accessor differs from source PUNI')
        if any(digest(p)!=h for p,h in inputs.items()): raise ValueError('input changed during run')
    return len(observed)

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True);p.add_argument('--bank',type=Path,required=True)
    args=p.parse_args();print(f'{run(args.source.resolve(),args.bank.resolve())} accessor comparisons passed')
if __name__=='__main__': main()
