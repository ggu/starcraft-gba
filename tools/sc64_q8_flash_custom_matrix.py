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
    ap.add_argument('--legacy-fixture',type=Path)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args()
    if a.output.exists():ap.error('output must be new')
    a.output.mkdir(parents=True)
    fixture=a.fixture.resolve();pin=hashlib.sha256(fixture.read_bytes()).hexdigest()
    legacy=a.legacy_fixture.resolve() if a.legacy_fixture else None
    legacy_pin=hashlib.sha256(legacy.read_bytes()).hexdigest() if legacy else None
    env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'halt_on_error=1'}
    def run(args,binary=fixture):
        p=subprocess.run([str(binary),*args],capture_output=True,text=True,env=env,timeout=30)
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
                    image.unlink();row={'map':map_id,'human':human,'bots':bots,'save_sha256':digest}
                    if legacy:
                        run(args,binary=legacy);old=image.read_bytes()
                        if len(old)!=131072 or old[32768:]!=bytes([255])*98304:raise ValueError('predecessor changed new-layout area')
                        old_digest=hashlib.sha256(old).hexdigest();run(['read-legacy',*args])
                        if hashlib.sha256(image.read_bytes()).hexdigest()!=old_digest:raise ValueError('legacy reader changed saved image')
                        image.unlink();row['legacy_save_sha256']=old_digest
                    rows.append(row)
        print(f'PASS map {map_id}: all three humans and 1–3 independently chosen AI races',flush=True)
    if hashlib.sha256(fixture.read_bytes()).hexdigest()!=pin:raise ValueError('fixture changed')
    if legacy and hashlib.sha256(legacy.read_bytes()).hexdigest()!=legacy_pin:raise ValueError('legacy fixture changed')
    report={'fixture_sha256':pin,'maps':maps,'cases':len(rows),'rows':rows,
            'legacy_fixture_sha256':legacy_pin,
            'scope':'authored HOST actual-codec writer/fresh-reader and invalid-roster refusals; not ARM or gameplay'}
    (a.output/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS {len(rows)} authored writer/fresh-reader cases',flush=True)


if __name__=='__main__':main()
