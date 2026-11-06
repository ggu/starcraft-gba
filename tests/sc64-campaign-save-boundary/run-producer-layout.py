# SPDX-License-Identifier: GPL-3.0-or-later
"""Recompile only the ABI witness with the frozen predecessor flag vector."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''): h.update(chunk)
    return h.hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('--producer',type=Path,required=True)
    p.add_argument('--record-root',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    producer=a.producer.resolve();record=a.record_root.resolve();out=a.out.resolve()
    if subprocess.check_output(['git','-C',str(producer),'rev-parse','HEAD'],text=True).strip()!='f0dc1929629c44b9156d8127d5b23ed65ed7b0d1':raise ValueError('producer commit mismatch')
    manifest=record/'build/z7-host-sram-contract/prefix/inputs-before.json'
    argv_path=record/'build/z7-host-sram-contract/prefix/baseline.argv.json'
    if sha(manifest)!='979dc9f6bbca68fac02563474622dc331adb4e5e667730d793dc2b43d065a7be' or sha(argv_path)!='3019c4b4b9c2e1000a13e70bee60666398008e0f4a0e97cbd870f4fe53f67fbf':raise ValueError('receipt mismatch')
    for name,expected in json.loads(manifest.read_text()).items():
        path=Path(name)
        if path.is_relative_to(record):
            rel=path.relative_to(record); candidate=producer/rel
            if candidate.is_file() and not str(rel).startswith(('.local/','build/')): path=candidate
        if not path.is_file() or sha(path)!=expected:raise ValueError(f'input mismatch: {name}')
    argv=json.loads(argv_path.read_text())
    if argv[-3:-1]!=['tests/sc64_z7_arm_replay.c','-o']:raise ValueError('unexpected recorded compiler command')
    root=Path(__file__).resolve().parents[2];out.parent.mkdir(parents=True,exist_ok=True)
    argv=argv[:-3]+[f'-DSC64_PRODUCER_GAME="{producer}/starcraft/game.c"',f'-DSC64_PRODUCER_GBA="{producer}/common/gba.c"',str(root/'tests/sc64-campaign-save-boundary/producer-layout.c'),'-o',str(out)]
    subprocess.run(argv,cwd=producer,check=True)
    subprocess.run([str(out)],cwd=producer,check=True)
if __name__=='__main__':main()
