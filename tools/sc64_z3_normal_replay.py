#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a Z3 host trace through physical keys on the pinned normal Z7 ROM."""
import argparse
import json
from pathlib import Path
import subprocess

import sc64_headless_physical as physical
import sc64_races_profile as races
import sc64_z7_normal_replay as shared
from emulator_smoke import symbols, symbol_sizes

FIELDS = ('frame','elapsed','screen','phase','switch7','sequence','outcome',
          'action_ticks','side_units','side_workers','side_army','enemy_units',
          'side_buildings','enemy_buildings','foes_any','win_presence_condition',
          'ore','gas','enemy_ore','enemy_gas','supply','supply_cap','gas_workers',
          'extractor','hydra_den','chrysalis_hp','cursor_x','cursor_y','selected',
          'selected_building','train_type')
ROM_SHA='59bd05448a2a579e6addd54cd37809a5678371b38feee0413bf9d52fff052b29'
ELF_SHA='f8fe62b4006c47fe521fddab7fc43c63490e11a9783957de9f5a24c93168a281'
RUNNER_SHA='95b3a8ccf0d2a04b7820366ee484012912e2657acb5bddf940ce473c1f06a18b'


def build_plan(path, stop=None):
    data=json.loads(path.read_text())
    if not isinstance(data,dict) or type(data.get('schema')) is not int or data['schema']!=1 or data.get('mission')!=14 or data.get('policy')!='hybrid':
        raise ValueError('expected schema-1 hybrid Z3 trace')
    rows=data.get('events')
    if not isinstance(rows,list) or not rows: raise ValueError('missing events')
    inputs=[]; observations={}; last_event=0; last_input=0; held=0; start=final=None
    for row in rows:
        if not isinstance(row,dict): raise ValueError('invalid event')
        tick=row.get('host_tick');kind=row.get('kind')
        if type(tick) is not int or tick<1 or tick<last_event or tick>=1<<32 or final is not None:
            raise ValueError('unordered or post-terminal event')
        if kind=='input':
            value=row.get('held');pressed=row.get('pressed')
            if (type(value) is not int or not 0<=value<=1023 or value==held or tick<=last_input or
                value & 48 == 48 or value & 192 == 192 or type(pressed) is not int or pressed != value & ~held):
                raise ValueError('invalid physical key transition')
            inputs.append((tick-1,value));last_input=tick;held=value
        elif kind in ('start','periodic','won'):
            if kind != 'start' and start is None: raise ValueError('observation before start')
            if any(type(row.get(k)) is not int for k in FIELDS): raise ValueError('incomplete observation')
            if tick in observations and any(observations[tick][k]!=row[k] for k in FIELDS):
                raise ValueError('conflicting observations')
            observations[tick]=row
            if kind=='start':
                if start is not None: raise ValueError('duplicate start')
                start=row
            if kind=='won': final=row
        else: raise ValueError('unsupported event')
        last_event=tick
    if (not inputs or inputs[0][0]!=0 or start is None or final is None or rows[-1] is not final or
        (start['host_tick'],start['frame'],start['screen'],start['phase'])!=(7,0,1,1) or
        dict(inputs).get(6)!=8 or final['host_tick']<=last_input or
        final['screen']!=3 or final['phase']!=3 or final['outcome']!=1 or final['switch7']!=1 or
        final['chrysalis_hp']<=0 or final['foes_any']!=final['enemy_units']+final['enemy_buildings'] or
        final['win_presence_condition']!=1 or not (final['foes_any']<=6 or final['enemy_buildings']==0)):
        raise ValueError('trace lacks genuine Z3 deployment or victory')
    terminal=final['host_tick']; stop=terminal if stop is None else stop
    if type(stop) is not int or not 0<stop<=terminal: raise ValueError('invalid prefix')
    boundaries=sorted(set(range(1000,stop+1,1000)) | {t for t in observations if t<=stop} | {stop})
    pages=[];first=0;i=0
    for end in boundaries:
        events=[]
        while i<len(inputs) and inputs[i][0]<end:
            if inputs[i][0]<first: raise ValueError('input outside page')
            events.append(inputs[i]);i+=1
        pages.append({'first':first,'end':end,'events':events});first=end
    return {'observations':observations,'terminal':terminal,'final':final},pages


def check_symbol(sym,sizes,name,width):
    address=sym.get(name);size=sizes.get(name)
    if (type(address) is not int or size!=width or width<=0 or
        not (0x02000000<=address and address+width<=0x02040000 or
             0x03000000<=address and address+width<=0x03008000)):
        raise ValueError(f'invalid symbol {name}')
    return address


def make_layout(elf,sym,sizes):
    layouts={n:races.dwarf_struct_layout(elf,n) for n in ('Sc64Z3State','Sc64RaceCampaignState','Unit','Sc64ZpCompactBuilding')}
    for symbol,name in (('race_z3','Sc64Z3State'),('race_campaign','Sc64RaceCampaignState')):
        check_symbol(sym,sizes,symbol,layouts[name][0])
    for symbol,name in (('u','Unit'),('b','Sc64ZpCompactBuilding')):
        width=sizes.get(symbol,0)
        if not width or width%layouts[name][0]: raise ValueError(f'invalid {symbol} array')
        check_symbol(sym,sizes,symbol,width)
    for layout,fields in [('Sc64Z3State',('switch7','sequence','outcome')),
                          ('Sc64RaceCampaignState',('elapsed_frames','phase','mission')),
                          ('Unit',('alive','owner','type','order','selected')),
                          ('Sc64ZpCompactBuilding',('alive','owner','type','raw_type','hp','under_construction'))]:
        for field in fields:
            width=layouts[layout][1].widths.get(field)
            if width not in (1,2,4): raise ValueError(f'invalid {layout}.{field} width')
            shared._field(layouts[layout],field,width)
    for k in ('screen','frame','won','lost','minerals','gas','supply_used','supply_cap','cx','cy','selected_building','train_type','race_selected_mission'):
        check_symbol(sym,sizes,k,4)
    check_symbol(sym,sizes,'race_z3_action_ticks',2)
    for k in ('race_minerals','race_gas'): check_symbol(sym,sizes,k,20)
    for k,n in (('demo_frame',4),('demo_total_cycles',8),('demo_peak_cycles',4),('demo_tick_buckets',32)):
        check_symbol(sym,sizes,k,n)
    return layouts


def snapshot(rsp,sym,sizes,layouts):
    def scalar(k,n=4,signed=True): return int.from_bytes(rsp.read(sym[k],n),'little',signed=signed)
    out={dst:scalar(src) for src,dst in [('frame','frame'),('screen','screen'),('minerals','ore'),('gas','gas'),('supply_used','supply'),('supply_cap','supply_cap'),('cx','cursor_x'),('cy','cursor_y'),('selected_building','selected_building'),('train_type','train_type')]}
    def obj(symbol,name):
        layout=layouts[name];data=rsp.read(sym[symbol],sizes[symbol]);return data,layout
    def val(data,pos,layout,key):
        length,fields=layout;width=fields.widths[key];off=shared._field(layout,key,width)
        return int.from_bytes(data[pos+off:pos+off+width],'little',signed=fields.signed.get(key,False))
    data,z=obj('race_z3','Sc64Z3State')
    for k in ('switch7','sequence','outcome'):out[k]=val(data,0,z,k)
    out['action_ticks']=scalar('race_z3_action_ticks',2,False)
    data,c=obj('race_campaign','Sc64RaceCampaignState')
    out['elapsed']=val(data,0,c,'elapsed_frames');out['phase']=val(data,0,c,'phase')
    if val(data,0,c,'mission')!=14 or scalar('race_selected_mission')!=14: raise RuntimeError('unexpected mission')
    for src,dst in [('race_minerals','enemy_ore'),('race_gas','enemy_gas')]:
        raw=rsp.read(sym[src],20);out[dst]=int.from_bytes(raw[4:8],'little',signed=True)
    units,ul=obj('u','Unit'); buildings,bl=obj('b','Sc64ZpCompactBuilding')
    out.update(side_units=0,side_workers=0,side_army=0,enemy_units=0,side_buildings=0,enemy_buildings=0,gas_workers=0,selected=0,extractor=0,hydra_den=0,chrysalis_hp=-1)
    for pos in range(0,len(units),ul[0]):
        get=lambda k:val(units,pos,ul,k)
        if not get('alive'):continue
        owner=get('owner');kind=get('type');order=get('order')
        if owner==0:
            out['side_units']+=1;out['side_workers']+=kind==7;out['side_army']+=kind!=7
            out['gas_workers']+=kind==7 and order==6
        elif owner==1:out['enemy_units']+=1
        # Z7 vector enables Z5 consume then transport, making transported order 9.
        out['selected']+=bool(get('selected') and order!=9)
    for pos in range(0,len(buildings),bl[0]):
        get=lambda k:val(buildings,pos,bl,k)
        if not get('alive'):continue
        owner=get('owner');kind=get('type')
        if owner==0:
            out['side_buildings']+=1
            if get('raw_type')==150:out['chrysalis_hp']=get('hp')
            out['extractor']|=kind==20
            out['hydra_den']|=kind==11 and not get('under_construction')
        elif owner==1:out['enemy_buildings']+=1
    out['foes_any']=out['enemy_units']+out['enemy_buildings']
    out['win_presence_condition']=int(out['foes_any']<=6 or out['enemy_buildings']==0)
    return out


def compare(expected,actual,tick):
    mismatch={k:(expected[k],actual[k]) for k in FIELDS if expected[k]!=actual[k]}
    if mismatch:raise RuntimeError(f'host observation {tick} differs: {mismatch}')


def run(trace,rom,elf,runner,output,stop=None):
    source=[Path(__file__),Path(physical.__file__),Path(shared.__file__),Path(races.__file__),
            Path(shared.headless.__file__),Path(shared.z7.__file__),Path(shared.scheduler.__file__),
            Path(shared.save.__file__),Path(__file__).with_name('emulator_smoke.py')]
    paths=(trace,rom,elf,runner,*source);before=shared.input_hashes(paths)
    if any(before[str(p.resolve())]!=h for p,h in ((rom,ROM_SHA),(elf,ELF_SHA),(runner,RUNNER_SHA))):
        raise ValueError('normal ROM, ELF or runner hash mismatch')
    plan,pages=build_plan(trace,stop);sym=symbols(elf);sizes=symbol_sizes(elf);layouts=make_layout(elf,sym,sizes)
    output.mkdir(parents=True,exist_ok=False);proc=rsp=log=None;records=[]
    try:
        proc,rsp,log=physical.start(runner,rom,output,sym,sizes)
        def u32(n):return int.from_bytes(rsp.read(sym[n],4),'little')
        if u32('screen')!=0 or u32('frame')!=0:raise RuntimeError('not at fresh title')
        timing=shared.timing_snapshot(rsp,sym,sizes)
        for page in pages:
            rsp.play_page(page);tick=page['end']
            if rsp.completed!=tick:raise RuntimeError('physical boundary mismatch')
            if tick in plan['observations']:
                state=snapshot(rsp,sym,sizes,layouts);compare(plan['observations'][tick],state,tick)
                records.append({'tick':tick,**state});(output/'progress.json').write_text(json.dumps(records,indent=2)+'\n')
        end=pages[-1]['end'];full=end==plan['terminal']
        if full and (not records or records[-1]['tick']!=end or u32('won')!=1 or u32('lost')!=0):
            raise RuntimeError('normal ROM did not win')
        shared.require_unchanged(paths,before)
        result={'status':'won' if full else 'prefix','stop':end,'checkpoint_count':len(records),'checkpoints':records,
                'transport':'physical-buttons','sha256':before,'timing':shared.timing_delta(timing,shared.timing_snapshot(rsp,sym,sizes),end),
                'scope':'named Z3/campaign, economy, counts, cursor and selection; not full entity arrays'}
        (output/'result.json').write_text(json.dumps(result,indent=2)+'\n');return result
    finally:
        if proc is not None:
            proc.terminate()
            try:proc.wait(timeout=3)
            except subprocess.TimeoutExpired:proc.kill();proc.wait()
            if proc.stdin:proc.stdin.close()
            if proc.stdout:proc.stdout.close()
            if log:log.close()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for n in ('trace','rom','elf','runner','output'):ap.add_argument('--'+n,type=Path,required=True)
    ap.add_argument('--stop',type=int);a=ap.parse_args();run(a.trace,a.rom,a.elf,a.runner,a.output,a.stop)
if __name__=='__main__':main()
