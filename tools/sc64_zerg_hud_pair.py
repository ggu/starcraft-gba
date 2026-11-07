#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Paired ordinary-input page-HUD probe. Owns the shared native lock.

Reads displayed Mode 4 pixels/palette and named state; never writes guest
memory. No save, text-box, or campaign coverage is implied by this tape.
"""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import resource
import shutil
import signal
import sys
import time

import sc64_skirmish_sram_continuation as world

LOCK='/tmp/sc64-native-build-20261031.lock'
RESERVE=4*1024**3
LIMIT=64*1024**2

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def alarm(_sig,_frame):raise TimeoutError('HUD pair exceeded monitored wall deadline')
def video(g):
    display=int.from_bytes(g.rsp.read(0x04000000,2),'little')
    if display&7!=4 or display&0x1000:raise ValueError('unexpected display mode or OBJ')
    page=(display>>4)&1
    pixels=g.rsp.read(0x06000000+page*0xa000,38400)
    palette=g.rsp.read(0x05000000,512)
    return (display,sha_bytes(pixels),sha_bytes(palette))
def sha_bytes(b):return hashlib.sha256(b).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('off_rom','off_elf','on_rom','on_elf','runner','tape','output'):
        ap.add_argument('--'+name.replace('_','-'),type=Path,required=True)
    ap.add_argument('--ticks',type=int,default=2500)
    ap.add_argument('--deadline',type=int,default=600)
    a=ap.parse_args()
    if not 1<=a.ticks<=5000 or not 1<=a.deadline<=1200:ap.error('ticks/deadline out of bounds')
    paths=(a.off_rom,a.off_elf,a.on_rom,a.on_elf,a.runner,a.tape)
    expected_runner='4199fe903b18c77e0833db1860c4968509cfc82e506bc6f2f9be7582b720fae2'
    expected_tape='d6bb9e46f9011a8af8c32eb62942ab8d611974b5592df85e5788358900c32df5'
    if a.output.exists():raise ValueError('output already exists')
    pins={str(p.resolve()):sha(p) for p in paths}
    if sha(a.runner)!=expected_runner or sha(a.tape)!=expected_tape:raise ValueError('unapproved runner/tape')
    events=[]
    for line in a.tape.read_text().splitlines():
        x=json.loads(line)
        if x.get('kind')=='input':events.append((x['call_index'],x['held']))
    if not events or any(type(t)!=int or type(k)!=int or t<0 or k<0 or k>1023 or k&48==48 or k&192==192 for t,k in events) or any(events[i][0]>=events[i+1][0] for i in range(len(events)-1)):raise ValueError('invalid input tape')
    def check():
        if any(sha(Path(p))!=h for p,h in pins.items()):raise ValueError('input changed')
        if shutil.disk_usage(a.output.parent).free<RESERVE:raise ValueError('disk reserve')
    resource.setrlimit(resource.RLIMIT_FSIZE,(LIMIT,LIMIT))
    old=signal.signal(signal.SIGALRM,alarm);old_term=signal.signal(signal.SIGTERM,alarm);signal.alarm(a.deadline)
    try:
        with open(LOCK,'a') as lock:
            fcntl.flock(lock,fcntl.LOCK_EX);check();a.output.mkdir(parents=True)
            games=[]
            try:
                for label,rom,elf in [('off',a.off_rom,a.off_elf),('on',a.on_rom,a.on_elf)]:
                    folder=a.output/label;folder.mkdir();games.append(world.PhysicalGame(a.runner,rom,elf,folder))
                counts={'advancing':0,'other':0};digest=hashlib.sha256();held=0;idx=0;elapsed=[[],[]]
                for tick in range(a.ticks):
                    if tick%100==0:check()
                    while idx<len(events) and events[idx][0]==tick:held=events[idx][1];idx+=1
                    before=[g.u32('frame') for g in games]
                    for i,g in enumerate(games):
                        clock=g.rsp.clock();g.step(held);elapsed[i].append(g.rsp.clock()-clock)
                    states=[g.state() for g in games];views=[video(g) for g in games]
                    if states[0]!=states[1] or views[0]!=views[1]:raise ValueError(f'pair mismatch at physical tick {tick}, frame {before}, display {views}')
                    if any(int.from_bytes(g.rsp.read(0x06013fe0,2),'little') for g in games):raise ValueError('unexpected modal: no automatic acknowledgement in this tape')
                    advancing=states[0]['frame']==before[0]+1 and states[0]['screen']==world.SCREEN_PLAY
                    counts['advancing' if advancing else 'other']+=1
                    digest.update(json.dumps([tick,states[0]['frame'],views[0]],separators=(',',':')).encode())
                    if sum(p.stat().st_size for p in a.output.rglob('*') if p.is_file())>LIMIT:raise ValueError('aggregate output threshold')
                last=states[0]
                if last['screen']!=world.SCREEN_PLAY or last['race_mode']!=1 or last['race_human']!=2 or last['race_bots']!=1 or counts['advancing']<1000:raise ValueError('custom scene did not reach expected advancing state')
                check();report={'pins':pins,'ticks':a.ticks,'counts':counts,'paired_every_tick_sha256':digest.hexdigest(),
                    'cycles':[{'mean':sum(v)/len(v),'max':max(v)} for v in elapsed],
                    'scope':'ordinary custom tape; full displayed indexed frame and palette plus named state every tick; no save or campaign proof'}
                (a.output/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
            finally:
                for g in reversed(games):
                    try:g.close()
                    except Exception:pass
    finally:signal.alarm(0);signal.signal(signal.SIGALRM,old);signal.signal(signal.SIGTERM,old_term)

if __name__=='__main__':main()
