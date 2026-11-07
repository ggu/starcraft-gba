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
import subprocess

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
def research_job(state):
    fields=world.BUILDING_FIELDS
    owner=1+fields.index('owner');family=1+fields.index('level');timer=1+fields.index('queue_progress')
    matches=[(row[0],row[timer]) for row in state['buildings']
             if row[owner]==0 and row[family]==3 and row[timer]>0]
    if len(matches)!=1:raise ValueError('expected one paid human ground-weapons job')
    return matches[0]
def validate_flash_image(path):
    data=path.read_bytes()
    if len(data)!=131072 or data[:32768]!=bytes([255])*32768:raise ValueError('unexpected legacy Flash region')
    commit=int.from_bytes(data[32768:32772],'little');meta=36864
    tag=int.from_bytes(data[meta:meta+4],'little');length=int.from_bytes(data[meta+4:meta+8],'little')
    if commit!=tag or (tag>>16)&255!=0x48 or not 0<length<=28656:raise ValueError('missing committed Flash record')
    value=2166136261
    for byte in data[meta+16:meta+16+length]:value=((value^byte)*16777619)&0xffffffff
    if value!=int.from_bytes(data[meta+8:meta+12],'little'):raise ValueError('Flash payload checksum mismatch')

def verify_build_roles(receipt,profile,off_rom,off_elf,on_rom,on_elf):
    products=receipt.get('products',{}).get(profile,{})
    for variant,rom,elf in (('off',off_rom,off_elf),('on',on_rom,on_elf)):
        for kind,path in (('rom',rom),('elf',elf)):
            entry=products.get(variant,{}).get(kind,{})
            if entry.get('path')!=str(path.resolve()) or entry.get('sha256')!=sha(path):
                raise ValueError('ROM/ELF role differs from paired build receipt')

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('off_rom','off_elf','on_rom','on_elf','runner','tape','output'):
        ap.add_argument('--'+name.replace('_','-'),type=Path,required=True)
    ap.add_argument('--build-receipt',type=Path,required=True)
    ap.add_argument('--profile',choices=('z9','z10'),required=True)
    ap.add_argument('--blank-flash',type=Path)
    ap.add_argument('--resume-tape',type=Path)
    ap.add_argument('--ticks',type=int,default=2500)
    ap.add_argument('--deadline',type=int,default=600)
    a=ap.parse_args()
    if not 1<=a.ticks<=5000 or not 1<=a.deadline<=1200:ap.error('ticks/deadline out of bounds')
    if bool(a.blank_flash)!=bool(a.resume_tape):ap.error('blank Flash and resume tape must be supplied together')
    paths=(a.off_rom,a.off_elf,a.on_rom,a.on_elf,a.runner,a.tape,a.build_receipt)
    if a.blank_flash:
        if a.blank_flash.read_bytes()!=bytes([255])*131072:ap.error('Flash backing must be erased')
        paths+=a.blank_flash,a.resume_tape
    expected_runner=('75b8ba5f71919802a981516ffef05baf29866256f97b7f67f46276171d9690e2' if a.blank_flash else '4199fe903b18c77e0833db1860c4968509cfc82e506bc6f2f9be7582b720fae2')
    expected_tape='d6bb9e46f9011a8af8c32eb62942ab8d611974b5592df85e5788358900c32df5'
    if a.output.exists():raise ValueError('output already exists')
    pins={str(p.resolve()):sha(p) for p in paths}
    receipt=json.loads(a.build_receipt.read_text())
    if not receipt.get('unchanged') or not receipt.get('vectors_verified',{}).get(a.profile):raise ValueError('missing verified paired build receipt')
    required={'hud-ab-test-roles'} | {f'hud-ab-{p}-{v}' for p in ('z9','z10') for v in ('off','on')} | {f'hud-ab-host-{p}-{v}' for p in ('z9','z10') for v in ('off','on')}
    results=receipt.get('results',[])
    if {row.get('name') for row in results if row.get('rc')==0}!=required or len(results)!=len(required):raise ValueError('paired build/host checks incomplete or failed')
    if subprocess.check_output(['git','rev-parse','--short=8','HEAD'],text=True).strip()!=receipt.get('head') or subprocess.run(['git','diff','--quiet','HEAD','--']).returncode:raise ValueError('source does not match build receipt')
    verify_build_roles(receipt,a.profile,a.off_rom,a.off_elf,a.on_rom,a.on_elf)
    if sha(a.off_rom)==sha(a.on_rom) or sha(a.off_elf)==sha(a.on_elf):raise ValueError('paired variants unexpectedly identical')
    if sha(a.runner)!=expected_runner or sha(a.tape)!=expected_tape:raise ValueError('unapproved runner/tape')
    if a.resume_tape and sha(a.resume_tape)!='389a025f68e66c1cd723338109ab4dcd1d51467317937ded2ecd7a073580329f':raise ValueError('unapproved resume tape')
    if a.blank_flash:
        import sc64_headless_flash_physical as flash_physical
        sys.modules['sc64_headless_physical']=flash_physical
    events=[]
    for line in a.tape.read_text().splitlines():
        x=json.loads(line)
        if x.get('kind')=='input':events.append((x['call_index'],x['held']))
    if not events or any(type(t)!=int or type(k)!=int or t<0 or k<0 or k>1023 or k&48==48 or k&192==192 for t,k in events) or any(events[i][0]>=events[i+1][0] for i in range(len(events)-1)):raise ValueError('invalid input tape')
    def check():
        if subprocess.check_output(['git','rev-parse','--short=8','HEAD'],text=True).strip()!=receipt.get('head') or subprocess.run(['git','diff','--quiet','HEAD','--']).returncode:
            raise ValueError('source changed after build')
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
                    folder=a.output/label;folder.mkdir();games.append(world.PhysicalGame(a.runner,rom,elf,folder,save_input=a.blank_flash,save_output=(folder/'guest.sav') if a.blank_flash else None))
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
                if last['race_mode']!=1 or last['race_human']!=2 or last['race_bots']!=1 or counts['advancing']<1000:raise ValueError('custom scene did not reach expected advancing state')
                save_equal=None
                if a.blank_flash:
                    if a.ticks!=3048 or last['screen']!=world.SCREEN_PAUSE:raise ValueError('save checkpoint not reached')
                    for g in games:g.rsp.export_sram()
                    saves=[a.output/label/'guest.sav' for label in ('off','on')]
                    save_equal=sha(saves[0])==sha(saves[1])
                    if not save_equal:raise ValueError('guest Flash images differ')
                    for save in saves:validate_flash_image(save)
                    saved=states[0];saved_job=research_job(saved)
                    # Match the fresh title-load's one advancing release update
                    # with ordinary START/release on the uninterrupted games.
                    for g in games:g.step(8);g.step(0)
                    reference=[g.state() for g in games];reference_video=[video(g) for g in games]
                    if reference[0]!=reference[1] or reference_video[0]!=reference_video[1]:raise ValueError('uninterrupted post-save pair mismatch')
                    for g in reversed(games):g.close()
                    games=[]
                    for label,rom,elf,save in [('off',a.off_rom,a.off_elf,saves[0]),('on',a.on_rom,a.on_elf,saves[1])]:
                        folder=a.output/(label+'-fresh');folder.mkdir()
                        games.append(world.PhysicalGame(a.runner,rom,elf,folder,save_input=save))
                    resume=[]
                    for line in a.resume_tape.read_text().splitlines():
                        x=json.loads(line)
                        if x.get('kind')=='input':resume.append((x['call_index'],x['held']))
                    for g in games:g.rsp.play_to(2,resume)
                    restored=[g.state() for g in games];v=[video(g) for g in games]
                    if (restored[0]!=restored[1] or v[0]!=v[1] or
                        restored[0]!=reference[0] or v[0][1:]!=reference_video[0][1:] or
                        (v[0][0]&~16)!=(reference_video[0][0]&~16) or
                        restored[0]['screen']!=world.SCREEN_PLAY):
                        raise ValueError('fresh load differs from uninterrupted state/render')
                    loaded=restored[0];loaded_job=research_job(loaded)
                    if (loaded['frame']!=saved['frame']+1 or
                        (loaded['minerals'],loaded['gas'])!=(saved['minerals'],saved['gas']) or
                        loaded_job!=(saved_job[0],saved_job[1]-1)):
                        raise ValueError('fresh load did not restore the saved paid job and wallet')
                elif last['screen']!=world.SCREEN_PLAY:raise ValueError('custom game no longer playing')
                check();report={'pins':pins,'source_head':receipt['head'],'profile':a.profile,'ticks':a.ticks,'counts':counts,'paired_every_tick_sha256':digest.hexdigest(),
                    'flash_images_equal':save_equal,'cycles':[{'mean':sum(v)/len(v),'max':max(v)} for v in elapsed],
                    'scope':'ordinary custom tape; full displayed indexed frame and palette plus named state every prefix tick; optional fresh Flash load; no campaign proof'}
                (a.output/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
            finally:
                for g in reversed(games):
                    try:g.close()
                    except Exception:pass
    finally:signal.alarm(0);signal.signal(signal.SIGALRM,old);signal.signal(signal.SIGTERM,old_term)

if __name__=='__main__':main()
