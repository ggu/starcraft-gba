#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay ordinary research controls through physical buttons; inspect only.

The input tape is private. The backend may import/export real 32 KiB SRAM at
boot/exit; it never writes guest memory and never injects gameplay state.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

import sc64_headless_physical as physical
from emulator_smoke import symbols, symbol_sizes
from sc64_races_profile import dwarf_struct_layout


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def transitions(path):
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    rows = [r for r in rows if r.get('kind') == 'input']
    return [(r['call_index'], r['held']) for r in rows]


def snapshot(rsp, sym, sizes, layout):
    def scalar(name):
        return int.from_bytes(rsp.read(sym[name], sizes[name]), 'little', signed=True)
    stride, off = layout
    data = rsp.read(sym['b'], sizes['b'])
    jobs = []
    for i in range(len(data) // stride):
        row = data[i*stride:(i+1)*stride]
        def field(name):
            n = off.widths[name]
            return int.from_bytes(row[off[name]:off[name]+n], 'little', signed=off.signed.get(name, False))
        if field('alive') and field('level') and field('queue_progress'):
            queue = list(row[off['queue']:off['queue']+5])
            jobs.append({'slot':i,'owner':field('owner'),'type':field('type'),
                         'family_plus_one':field('level'),'timer':field('queue_progress'),'queue':queue})
    levels = list(rsp.read(sym['sc64_experimental_research_levels'], 48))
    return {'frame':scalar('frame'),'screen':scalar('screen'),'bots':scalar('race_bots'),
            'minerals':scalar('minerals'),'gas':scalar('gas'),'lost':scalar('lost'),
            'jobs':jobs,'levels':levels}


def main():
    ap = argparse.ArgumentParser()
    for name in ('rom','elf','runner','tape','resume_tape','output'):
        ap.add_argument('--'+name.replace('_','-'), type=Path, required=True)
    a=ap.parse_args(); a.output.mkdir(parents=True,exist_ok=False)
    sym,sizes=symbols(a.elf),symbol_sizes(a.elf)
    required=('b','frame','screen','race_bots','minerals','gas','lost','sc64_experimental_research_levels')
    if any(k not in sym or k not in sizes for k in required): raise ValueError('missing observed symbols')
    layout=dwarf_struct_layout(a.elf,'Building'); stride,off=layout
    if sizes['b']%stride or off.arrays.get('queue')!=(5,1,False): raise ValueError('unexpected building ABI')
    pins={str(p):sha(p) for p in (a.rom,a.elf,a.runner,a.tape,a.resume_tape)}
    events=transitions(a.tape); resume=transitions(a.resume_tape)
    saved=a.output/'physical.sav';first=a.output/'first';first.mkdir();proc=None
    try:
        proc,rsp,log=physical.start(a.runner,a.rom,first,sym,sizes,save_output=saved)
        rsp.play_to(3044,events); paid=snapshot(rsp,sym,sizes,layout)
        if paid['screen']!=1 or paid['lost'] or paid['bots']!=1 or not any(j['owner']==0 and j['family_plus_one']==3 and j['queue'][:2]==[0,1] for j in paid['jobs']):
            raise RuntimeError(f'ordinary tape did not reach the paid job: {paid}')
        rsp.play_to(3048,events); paused=snapshot(rsp,sym,sizes,layout)
        if paused['screen']!=2: raise RuntimeError(f'pause/save not reached: {paused}')
        rsp.export_sram();proc.terminate();proc.wait(timeout=3);log.close();proc=None
        if saved.stat().st_size!=32768: raise RuntimeError('bad exported SRAM length')
        raw=saved.read_bytes();tags=[int.from_bytes(raw[i:i+4],'little') for i in (0,16384)]
        if not any((v>>16)&255==0x45 for v in tags): raise RuntimeError(f'no active-job tag: {tags}')
        second=a.output/'fresh';second.mkdir()
        proc,rsp,log=physical.start(a.runner,a.rom,second,sym,sizes,save_input=saved)
        rsp.play_to(2,resume);loaded=snapshot(rsp,sym,sizes,layout)
        if loaded['screen']!=1 or loaded['bots']!=1 or not any(j['owner']==0 and j['family_plus_one']==3 and j['queue'][:2]==[0,1] for j in loaded['jobs']):
            raise RuntimeError(f'fresh physical load did not restore the job: {loaded}')
        rsp.play_to(5002,resume);finished=snapshot(rsp,sym,sizes,layout)
        if finished['screen']!=1 or finished['lost'] or finished['levels'][2]!=1:
            raise RuntimeError(f'ordinary research did not complete: {finished}')
        proc.terminate();proc.wait(timeout=3);log.close();proc=None
        if any(sha(Path(k))!=v for k,v in pins.items()): raise RuntimeError('input changed')
        (a.output/'result.json').write_text(json.dumps({'pins':pins,'save_sha256':sha(saved),'tags':tags,
            'paid':paid,'paused':paused,'loaded':loaded,'finished':finished,
            'scope':'physical buttons; named read-only state; constructed controller decisions, not full-world or hardware proof'},indent=2)+'\n')
        print((a.output/'result.json').read_text())
    finally:
        if proc is not None:
            proc.terminate()
            try: proc.wait(timeout=3)
            except subprocess.TimeoutExpired: proc.kill();proc.wait()

if __name__=='__main__': main()
