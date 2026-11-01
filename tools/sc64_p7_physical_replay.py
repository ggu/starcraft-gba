#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a pinned P7 host input trace via physical buttons; read guest state only."""
import argparse
import hashlib
import json
from pathlib import Path

import sc64_headless_physical as physical
import sc64_races_profile as races
import sc64_z7_normal_replay as shared
from emulator_smoke import symbols, symbol_sizes

TRACE_SHA='ca2d6f422064a84340f588e760f17171b37dd1dffcd0632d7939315cfacdde89'
ROM_SHA='378ab490ea78915da2d0f6e190f0db5dff003a685e1f39e1c8444c69fb6bd795'
ELF_SHA='17e8398093c91a2ef1a0922e2cdbc0e9391455abdf822ae2d7e5ddcd0e2001fb'
RUNNER_SHA='166c017b227d3611a751aa0cce7a4179cbe3077b025a9aa108d1dc2ed2e79a20'

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def plan(path,stop):
    rows=[json.loads(line) for line in path.read_text().splitlines()]
    meta=rows[0];final=[r for r in rows if r.get('kind')=='final'];verdict=[r for r in rows if r.get('kind')=='strict-verdict']
    if meta.get('schema')!=1 or meta.get('mission')!=26 or meta.get('profile')!='PROTOSS_P7_CANONICAL_FLAGS' or len(final)!=1 or len(verdict)!=1:
        raise ValueError('wrong source trace')
    if (final[0].get('total_calls'),final[0].get('frame'),final[0].get('won'),final[0].get('lost'),verdict[0].get('ok'))!=(49970,48739,1,0,1):
        raise ValueError('trace lacks pinned strict victory')
    events=[];held=0;last=-1
    for row in rows:
        if row.get('kind')!='input':continue
        i,v,p=row['call_index'],row['held'],row['pressed']
        if any(type(n)!=int for n in (i,v,p)) or not 0<=i<49970 or i<=last or not 0<=v<=1023 or v==held or v&48==48 or v&192==192 or p!=(v & ~held):
            raise ValueError('invalid input transition')
        if i>=49968:
            if (i,v,p,row['frame_before'],row['frame_after'])!=(49968,0,0,48739,48739):raise ValueError('unexpected post-result input')
        else:events.append((i,v))
        held=v;last=i
    if not events or events[0]!=(0,1) or last!=49968:raise ValueError('unexpected launch or terminal transition')
    # Host input() counts calls after RESULT without invoking game_update.  The
    # final neutral transition at call 49968 and next call are not replayed.
    if stop not in (1000,49968):raise ValueError('only reviewed prefix/full bounds allowed')
    return events

def checked_symbol(sym,sizes,name,width):
    addr=sym.get(name);size=sizes.get(name)
    if type(addr)!=int or size!=width or not (0x02000000<=addr and addr+size<=0x02040000 or 0x03000000<=addr and addr+size<=0x03008000):raise ValueError('invalid symbol '+name)
    return addr

def layout(elf,sym,sizes):
    names=('Sc64P7PersistentState','Sc64P7RuntimeState','Sc64P7State','Sc64RaceCampaignState','Unit','Sc64ZpCompactBuilding')
    out={name:races.dwarf_struct_layout(elf,name) for name in names}
    for symbol,name in [('race_p7',names[0]),('race_campaign',names[3])]:checked_symbol(sym,sizes,symbol,out[name][0])
    for symbol,name in [('u','Unit'),('b','Sc64ZpCompactBuilding')]:
        width=sizes.get(symbol,0)
        if not width or width%out[name][0]:raise ValueError('invalid entity stride')
        checked_symbol(sym,sizes,symbol,width)
    for name in ('frame','screen','won','lost','race_selected_mission'):
        checked_symbol(sym,sizes,name,4)
    for name,fields in [('Sc64P7PersistentState',('runtime',)),('Sc64P7RuntimeState',('model','action_ticks','switch_bits','fault')),('Sc64P7State',('started','outcome','action_cursor')),('Sc64RaceCampaignState',('mission','phase')),('Unit',('alive','owner','raw_type','raw_owner')),('Sc64ZpCompactBuilding',('alive','raw_type','raw_owner','hp'))]:
        length,members=out[name]
        for field in fields:
            if field not in members or not 0<=members[field]<length:raise ValueError('invalid DWARF member '+name+'.'+field)
    persistent=out['Sc64P7PersistentState'];runtime=out['Sc64P7RuntimeState'];model=out['Sc64P7State']
    if (persistent[1]['runtime']+runtime[0]>persistent[0] or runtime[1]['model']+model[0]>runtime[0] or
        runtime[1]['action_ticks']+20>runtime[0] or model[1]['action_cursor']+10>model[0]):
        raise ValueError('invalid nested P7 DWARF layout')
    return out

def snapshot(rsp,sym,sizes,l):
    def scalar(n):return int.from_bytes(rsp.read(sym[n],4),'little',signed=True)
    def field(raw,name,key,base=0,width=None,signed=False):
        size,m=l[name];w=width or m.widths.get(key)
        if w not in (1,2,4) or m[key]+w>size:raise ValueError('invalid field width '+name+'.'+key)
        return int.from_bytes(raw[base+m[key]:base+m[key]+w],'little',signed=signed)
    out={n:scalar(n) for n in ('frame','screen','won','lost','race_selected_mission')}
    campaign=rsp.read(sym['race_campaign'],sizes['race_campaign'])
    out['campaign_mission']=field(campaign,'Sc64RaceCampaignState','mission');out['phase']=field(campaign,'Sc64RaceCampaignState','phase')
    p=rsp.read(sym['race_p7'],sizes['race_p7']);base=l['Sc64P7PersistentState'][1]['runtime'];model=base+l['Sc64P7RuntimeState'][1]['model']
    out['outcome']=field(p,'Sc64P7State','outcome',model);out['started']=field(p,'Sc64P7State','started',model)
    off=model+l['Sc64P7State'][1]['action_cursor'];out['victory_cursor']=p[off+5]
    out['switch_bits']=field(p,'Sc64P7RuntimeState','switch_bits',base);out['fault']=field(p,'Sc64P7RuntimeState','fault',base)
    off=base+l['Sc64P7RuntimeState'][1]['action_ticks'];out['action_timers_clear']=not any(p[off:off+20])
    units=rsp.read(sym['u'],sizes['u']);us=l['Unit'][0];heroes={75:0,78:0,79:0}
    for i in range(0,len(units),us):
        if (field(units,'Unit','alive',i) and field(units,'Unit','owner',i) in (0,3) and
            field(units,'Unit','raw_owner',i,signed=True)==2):
            raw=field(units,'Unit','raw_type',i,signed=True)
            if raw in heroes:heroes[raw]+=1
    out['heroes']=heroes
    buildings=rsp.read(sym['b'],sizes['b']);bs=l['Sc64ZpCompactBuilding'][0];out['nexuses']=[ ]
    for i in range(0,len(buildings),bs):
        if field(buildings,'Sc64ZpCompactBuilding','raw_type',i,signed=True)==154:
            out['nexuses'].append({'slot':i//bs,'alive':field(buildings,'Sc64ZpCompactBuilding','alive',i),'source_owner':field(buildings,'Sc64ZpCompactBuilding','raw_owner',i,signed=True),'hp':field(buildings,'Sc64ZpCompactBuilding','hp',i,signed=True)})
    return out

def main():
    ap=argparse.ArgumentParser();
    for n in ('trace','rom','elf','runner','output'):ap.add_argument('--'+n,type=Path,required=True)
    ap.add_argument('--stop',type=int,choices=(1000,49968),required=True);a=ap.parse_args()
    for p,h in ((a.trace,TRACE_SHA),(a.rom,ROM_SHA),(a.elf,ELF_SHA),(a.runner,RUNNER_SHA)):
        if sha(p)!=h:raise ValueError('hash mismatch '+str(p))
    sources=[Path(__file__),Path(physical.__file__),Path(races.__file__),Path(shared.__file__),Path(shared.headless.__file__),Path(shared.z7.__file__),Path(shared.scheduler.__file__),Path(shared.save.__file__),Path(__file__).with_name('emulator_smoke.py')]
    paths=[a.trace,a.rom,a.elf,a.runner,*sources];before=shared.input_hashes(paths)
    events=plan(a.trace,a.stop);sym=symbols(a.elf);sizes=symbol_sizes(a.elf);layouts=layout(a.elf,sym,sizes)
    a.output.mkdir(parents=True,exist_ok=False);proc=rsp=log=None;records=[]
    try:
        proc,rsp,log=physical.start(a.runner,a.rom,a.output,sym,sizes)
        if int.from_bytes(rsp.read(sym['screen'],4),'little')!=0 or int.from_bytes(rsp.read(sym['frame'],4),'little')!=0:
            raise RuntimeError('not at fresh title')
        for end in ([12,1000] if a.stop==1000 else [12,1000,49968]):
            rsp.play_to(end,events);state=snapshot(rsp,sym,sizes,layouts);records.append({'updates':end,**state})
            if state['race_selected_mission']!=26 or state['campaign_mission']!=26:raise RuntimeError('wrong mission')
            if end==12 and (state['frame'],state['screen'])!=(1,1):raise RuntimeError('launch mismatch')
            if end==1000 and (state['frame'],state['screen'])!=(963,1):raise RuntimeError('prefix host mapping mismatch')
            if end==49968 and not(state['screen']==3 and state['won']==1 and state['lost']==0 and state['outcome']==1 and state['fault']==0 and state['switch_bits']==1 and state['victory_cursor']==0 and state['action_timers_clear'] and not(state['started']&15) and state['started']&(1<<5) and all(state['heroes'][i]>=1 for i in (75,78,79)) and not any(x['alive'] and x['source_owner']==0 for x in state['nexuses'])):raise RuntimeError('strict P7 result mismatch')
        for p,h in ((a.trace,TRACE_SHA),(a.rom,ROM_SHA),(a.elf,ELF_SHA),(a.runner,RUNNER_SHA)):
            if sha(p)!=h:raise RuntimeError('input changed')
        shared.require_unchanged(paths,before)
        (a.output/'result.json').write_text(json.dumps({'status':'prefix' if a.stop==1000 else 'win','checkpoints':records,'sha256':before,'scope':'physical buttons, named P7 fields, not full-world equality'},indent=2)+'\n')
    finally:
        if proc is not None:
            proc.terminate()
            try:proc.wait(timeout=3)
            except Exception:proc.kill();proc.wait()
            if proc.stdin:proc.stdin.close()
            if proc.stdout:proc.stdout.close()
            if log:log.close()
if __name__=='__main__':main()
