#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authored HOST Flash writer/fresh-reader matrix, not guest gameplay proof.

The caller owns the shared native lock. Each process uses the actual game
codec and the fake Flash command device. No saved image is retagged.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args()
    if a.output.exists():ap.error('output must be new')
    a.output.mkdir(parents=True)
    fixture=a.fixture.resolve();pin=hashlib.sha256(fixture.read_bytes()).hexdigest()
    env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'halt_on_error=1'}
    def run(args):
        p=subprocess.run([str(fixture),*args],capture_output=True,text=True,env=env,timeout=30)
        if p.returncode or any(x in p.stderr for x in ('runtime error:','AddressSanitizer','UndefinedBehaviorSanitizer')):
            raise RuntimeError(f'fixture {args}: rc={p.returncode}\n{p.stdout[-2000:]}\n{p.stderr[-2000:]}')
        return p.stdout
    maps=int(run(['catalog']).strip())
    if not 1<=maps<=9:raise ValueError('unsupported map catalog size')
    rows=[];image=a.output/'authored.sav'
    for map_id in range(maps):
        for human in ('terran','zerg','protoss'):
            for count in range(1,4):
                for letters in itertools.product('tzp',repeat=count):
                    bots=''.join(letters);args=[human,bots,str(map_id),str(image.resolve())]
                    run(['write',*args]);digest=hashlib.sha256(image.read_bytes()).hexdigest()
                    run(['read',*args])
                    if hashlib.sha256(image.read_bytes()).hexdigest()!=digest:raise ValueError('reader changed saved image')
                    image.unlink();rows.append({'map':map_id,'human':human,'bots':bots,'save_sha256':digest})
        print(f'PASS map {map_id}: all three humans and 1–3 independently chosen AI races',flush=True)
    if hashlib.sha256(fixture.read_bytes()).hexdigest()!=pin:raise ValueError('fixture changed')
    report={'fixture_sha256':pin,'maps':maps,'cases':len(rows),'rows':rows,
            'scope':'authored HOST actual-codec writer/fresh-reader and invalid-roster refusals; not ARM or gameplay'}
    (a.output/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS {len(rows)} authored writer/fresh-reader cases',flush=True)


if __name__=='__main__':main()
