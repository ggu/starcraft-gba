#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a validated Z7 input trace through the unmodified normal ROM.

Only scheduler controls are written. Every retained host observation is checked;
this compares the named model, entity counts and economy fields, not full arrays.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

import sc64_headless_replay as headless
import sc64_races_profile as races
import sc64_z3_controller_replay as scheduler
import sc64_z7_arm_replay as z7
from emulator_smoke import symbols, symbol_sizes


def build_plan(trace, stop=None):
    plan = z7.load_trace(trace)
    final = plan['final']
    if (final['kind'] != 'won' or final['screen'] != 3 or final['won'] != 1 or
        final['lost'] != 0 or final['outcome'] != 1 or final['fault'] != 0 or
        final['started'] != 0xfe or final['switch10'] != 1):
        raise ValueError('trace does not end in a verified Z7 victory')
    if any(len(row[field]) != 8 for row in plan['observations'].values() for field in z7.LISTS):
        raise ValueError('trace action arrays must contain eight entries')
    stop = plan['stop'] if stop is None else stop
    if type(stop) is not int or not 0 < stop <= plan['stop']:
        raise ValueError('prefix must lie within the validated trace')
    inputs = [(tick - 1, held) for tick, held in plan['transitions']]
    boundaries = sorted(set(range(1000, stop + 1, 1000)) |
                        {t for t in plan['observations'] if 0 < t <= stop} | {stop})
    pages, first, index = [], 0, 0
    for end in boundaries:
        events = []
        while index < len(inputs) and inputs[index][0] < end:
            if inputs[index][0] < first:
                raise ValueError('input events are out of order')
            events.append(inputs[index]); index += 1
        pages.append({'first': first, 'end': end, 'events': events})
        first = end
    return plan, pages


def _field(layout, name, size, *, array=None):
    length, offsets = layout
    if name not in offsets:
        raise ValueError(f'missing DWARF member {name}')
    offset = offsets[name]
    if array is None:
        if offsets.widths.get(name) != size:
            raise ValueError(f'invalid DWARF width for {name}')
        width = size
    else:
        if offsets.arrays.get(name) != (array, size, False):
            raise ValueError(f'invalid DWARF array for {name}')
        width = size * array
    if offset < 0 or offset + width > length:
        raise ValueError(f'DWARF member {name} exceeds its structure')
    return offset


def make_layout(elf, sizes):
    layouts = {n: races.dwarf_struct_layout(elf, n) for n in
               ('Sc64Z7PersistentState', 'Sc64Z7RuntimeState', 'Sc64Z7State', 'Unit', 'Sc64ZpCompactBuilding')}
    persistent, runtime, model = (layouts[n] for n in ('Sc64Z7PersistentState', 'Sc64Z7RuntimeState', 'Sc64Z7State'))
    po = persistent[1].get('runtime'); ro = runtime[1].get('model')
    if po is None or ro is None or po < 0 or ro < 0 or po + runtime[0] > persistent[0] or ro + model[0] > runtime[0] or sizes.get('race_z7') != persistent[0]:
        raise ValueError('invalid nested Z7 DWARF layout')
    for name, width in [('outcome', 1), ('started', 2)]: _field(model, name, width)
    _field(model, 'action_cursor', 1, array=8)
    for name, width in [('switch10',1), ('ai_script_mask',1), ('ai_plan_ticks',2), ('ai_plan_phase',1), ('fault',1)]: _field(runtime, name, width)
    _field(runtime, 'action_ticks', 2, array=8)
    for key, layout in [('u', layouts['Unit']), ('b', layouts['Sc64ZpCompactBuilding'])]:
        if not sizes.get(key) or sizes[key] % layout[0]: raise ValueError(f'invalid {key} bank size')
        for name in ('alive', 'owner', 'raw_type', 'raw_owner'):
            width = layout[1].widths.get(name)
            if width not in (1, 2, 4): raise ValueError(f'invalid {key}.{name} width')
            _field(layout, name, width)
    return layouts


def snapshot(rsp, sym, sizes, layouts):
    def scalar(name):
        if sizes.get(name) != 4: raise ValueError(f'invalid {name} symbol width')
        return int.from_bytes(rsp.read(sym[name],4), 'little', signed=True)
    out = {k: scalar(k) for k in ('screen','frame','won','lost','minerals','gas')}
    p, r, m = (layouts[n] for n in ('Sc64Z7PersistentState','Sc64Z7RuntimeState','Sc64Z7State'))
    raw = rsp.read(sym['race_z7'],p[0]); base = p[1]['runtime']; model = base + r[1]['model']
    def value(base, layout, name, width):
        off = _field(layout,name,width); return int.from_bytes(raw[base+off:base+off+width], 'little')
    for key,width in [('started',2),('outcome',1)]: out[key]=value(model,m,key,width)
    for key,width in [('switch10',1),('ai_script_mask',1),('ai_plan_ticks',2),('ai_plan_phase',1),('fault',1)]: out[key]=value(base,r,key,width)
    for key,layout,member,offset,width in [('action_cursors',m,'action_cursor',model,1),('action_ticks',r,'action_ticks',base,2)]:
        off=_field(layout,member,width,array=8);out[key]=[int.from_bytes(raw[offset+off+i*width:offset+off+(i+1)*width], 'little') for i in range(8)]
    for symbol, key, layout in [('u','units',layouts['Unit']),('b','buildings',layouts['Sc64ZpCompactBuilding'])]:
        data=rsp.read(sym[symbol],sizes[symbol]);stride,fields=layout
        def get(row,name):
            width=fields.widths[name]; pos=row+fields[name]
            return int.from_bytes(data[pos:pos+width],'little',signed=fields.signed.get(name,False))
        side=enemy=workers=0
        for pos in range(0,len(data),stride):
            if not get(pos,'alive'): continue
            owner=get(pos,'owner');raw_owner=get(pos,'raw_owner')
            side += owner == 0; enemy += raw_owner == 4
            if symbol=='u': workers += owner == 0 and get(pos,'raw_type') == 41
        out['side_'+key]=side;out['enemy_'+key]=enemy
        if symbol=='u': out['workers']=workers
    return out


def compare(reference, actual, tick):
    mismatch={k:[reference[k],actual[k]] for k in z7.SCALARS+z7.LISTS if reference[k]!=actual[k]}
    if mismatch: raise RuntimeError(f'host observation {tick} differs: {mismatch}')
    if actual['fault'] != 0: raise RuntimeError(f'Z7 fault at {tick}')


def play_dense(rsp, sym, page, chunks, write_page):
    """Refill only after consumption, stopping at actual game-update calls."""
    def u32(name): return int.from_bytes(rsp.read(sym[name],4),'little')
    rsp.breakpoint(sym['demo_replay_checkpoint']&~1,False)
    rsp.breakpoint(sym['game_update']&~1,True)
    last=page['first']-1; active=0; stale=0
    while last<page['end']-1:
        rsp.tick(expected=sym['game_update']&~1);tick=u32('demo_schedule_tick')
        if tick<last or tick>=page['end']: raise RuntimeError('invalid dense-page tick')
        if tick==last:
            stale+=1
            if stale>8: raise RuntimeError('dense-page breakpoint stalled')
            continue
        stale=0;last=tick;index=u32('demo_schedule_index')
        if index>len(chunks[active]): raise RuntimeError('invalid dense-page cursor')
        if index==len(chunks[active]) and active+1<len(chunks):
            active+=1
            if chunks[active][0][0]<=tick: raise RuntimeError('dense-page refill too late')
            write_page(chunks[active]);rsp.write(sym['demo_schedule_index'],struct.pack('<I',0))
    rsp.breakpoint(sym['game_update']&~1,False)
    rsp.breakpoint(sym['demo_replay_checkpoint']&~1,True)
    return active


def input_hashes(paths):
    return {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}


def require_unchanged(paths, before):
    if input_hashes(paths)!=before:
        raise RuntimeError('trace, ROM, ELF or runner changed during replay')


def run(trace, rom, elf, runner, output, stop=None):
    inputs=(trace,rom,elf,runner);before=input_hashes(inputs)
    plan,pages=build_plan(trace,stop);stop=pages[-1]['end']
    sym,sizes=symbols(elf),symbol_sizes(elf)
    mode,capacity=scheduler.schedule_mode(sym,sizes)
    if mode!='windowed' or scheduler.checkpoint_ticks(sym)!=1000:
        raise ValueError('normal Z7 replay requires the 1000-tick window scheduler')
    layouts=make_layout(elf,sizes)
    output.mkdir(parents=True,exist_ok=False)
    proc=rsp=log=None; records=[]
    def u32(name): return int.from_bytes(rsp.read(sym[name],4),'little')
    def write_page(events):
        rsp.write(sym['demo_schedule_events'],b''.join(struct.pack('<2I',*item) for item in events))
        rsp.write(sym['demo_schedule_count'],struct.pack('<I',len(events)))
    try:
        proc,rsp,log=headless.start(runner,rom,output,sym,sizes)
        if u32('screen')!=0 or u32('frame')!=0: raise RuntimeError('fresh boot is not at title')
        write_page(pages[0]['events'][:capacity]);rsp.write(sym['demo_schedule_index'],struct.pack('<I',0))
        rsp.write(sym['demo_schedule_length'],struct.pack('<I',pages[0]['end']))
        rsp.write(sym['demo_schedule_active'],struct.pack('<I',1))
        rsp.breakpoint(sym['game_update']&~1,False);rsp.breakpoint(sym['demo_replay_checkpoint']&~1,True)
        rsp.tick(expected=sym['demo_replay_checkpoint']&~1)
        if u32('demo_schedule_tick')!=0: raise RuntimeError('scheduler did not start at zero')
        for i,page in enumerate(pages):
            chunks=[page['events'][n:n+capacity] for n in range(0,len(page['events']),capacity)] or [[]]; active=0
            if len(chunks)>1:
                active=play_dense(rsp,sym,page,chunks,write_page)
            rsp.tick(expected=sym['demo_replay_checkpoint']&~1)
            tick=page['end']
            if u32('demo_schedule_tick')!=tick or active!=len(chunks)-1 or u32('demo_schedule_index')!=len(chunks[-1]) or u32('demo_schedule_count')!=len(chunks[-1]):
                raise RuntimeError('scheduler did not consume its page')
            if tick in plan['observations']:
                state=snapshot(rsp,sym,sizes,layouts)
                if u32('race_selected_mission')!=z7.MISSION: raise RuntimeError('unexpected selected mission')
                compare(plan['observations'][tick],state,tick)
                records.append({'tick':tick,**state});(output/'progress.json').write_text(json.dumps(records,indent=2)+'\n')
            if i+1<len(pages):
                write_page(pages[i+1]['events'][:capacity]);rsp.write(sym['demo_schedule_length'],struct.pack('<I',pages[i+1]['end']))
        full=stop==plan['stop']
        if full and (not records or records[-1]['tick']!=stop or records[-1]['won']!=1 or records[-1]['outcome']!=1 or records[-1]['lost']!=0): raise RuntimeError('normal ROM did not win')
        require_unchanged(inputs,before)
        result={'status':'won' if full else 'prefix','stop':stop,'checkpoint_count':len(records),'checkpoints':records,'scope':'named Z7 model fields, entity counts and economy; not full entity arrays','sha256':before}
        (output/'result.json').write_text(json.dumps(result,indent=2)+'\n');return result
    finally:
        if proc is not None:
            proc.terminate()
            try: proc.wait(timeout=3)
            except subprocess.TimeoutExpired: proc.kill();proc.wait()
            if proc.stdin:proc.stdin.close()
            if proc.stdout:proc.stdout.close()
            if log:log.close()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ('trace','rom','elf','runner','output'):ap.add_argument('--'+name,type=Path,required=True)
    ap.add_argument('--stop',type=int);a=ap.parse_args();run(a.trace,a.rom,a.elf,a.runner,a.output,a.stop)

if __name__=='__main__':main()
