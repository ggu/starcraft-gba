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
import sc64_z7_normal_save as save
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


def model_snapshot(rsp, sym, layouts):
    out = {}
    p, r, m = (layouts[n] for n in ('Sc64Z7PersistentState','Sc64Z7RuntimeState','Sc64Z7State'))
    raw = rsp.read(sym['race_z7'],p[0]); base = p[1]['runtime']; model = base + r[1]['model']
    def value(base, layout, name, width):
        off = _field(layout,name,width); return int.from_bytes(raw[base+off:base+off+width], 'little')
    for key,width in [('started',2),('outcome',1)]: out[key]=value(model,m,key,width)
    for key,width in [('switch10',1),('ai_script_mask',1),('ai_plan_ticks',2),('ai_plan_phase',1),('fault',1)]: out[key]=value(base,r,key,width)
    for key,layout,member,offset,width in [('action_cursors',m,'action_cursor',model,1),('action_ticks',r,'action_ticks',base,2)]:
        off=_field(layout,member,width,array=8);out[key]=[int.from_bytes(raw[offset+off+i*width:offset+off+(i+1)*width], 'little') for i in range(8)]
    return out


def snapshot(rsp, sym, sizes, layouts):
    def scalar(name):
        if sizes.get(name) != 4: raise ValueError(f'invalid {name} symbol width')
        return int.from_bytes(rsp.read(sym[name],4), 'little', signed=True)
    out = {k: scalar(k) for k in ('screen','frame','won','lost','minerals','gas')}
    out.update(model_snapshot(rsp,sym,layouts))
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


def timing_snapshot(rsp, sym, sizes):
    """Read the shipped main-loop counters while execution is stopped."""
    widths={'demo_frame':4,'demo_total_cycles':8,'demo_peak_cycles':4,'demo_tick_buckets':32}
    if any(name not in sym for name in widths):
        return None
    if any(sizes.get(name)!=width for name,width in widths.items()):
        raise ValueError('invalid main-loop timing symbol width')
    values={name:rsp.read(sym[name],width) for name,width in widths.items()}
    return {'updates':int.from_bytes(values['demo_frame'],'little'),
            'cycles':int.from_bytes(values['demo_total_cycles'],'little'),
            'peak':int.from_bytes(values['demo_peak_cycles'],'little'),
            'buckets':list(struct.unpack('<8I',values['demo_tick_buckets']))}


def timing_delta(before, after, updates):
    if before is None or after is None:
        return None
    count=after['updates']-before['updates']
    cycles=after['cycles']-before['cycles']
    buckets=[new-old for old,new in zip(before['buckets'],after['buckets'])]
    if count!=updates or count<=0 or cycles<=0 or any(n<0 for n in buckets) or sum(buckets)!=count:
        raise RuntimeError('main-loop timing counters do not match replay updates')
    return {'updates':count,'guest_cycles':cycles,'mean_cycles':cycles/count,
            'effective_main_loop_hz':16777216*count/cycles,'peak_since_boot':after['peak'],
            'vblank_buckets':buckets,
            'scope':'emulated main-loop work including presentation waits and UI; not physical-device FPS'}


def save_format_provenance(provenance, save_format):
    """Bind the new namespace while retaining older legacy summaries."""
    if save_format not in save.SAVE_FORMATS:
        raise ValueError('unsupported Z7 save format')
    return ({**provenance, 'save_format': save_format}
            if save_format != 'legacy' else dict(provenance))


def resume_window(plan, cut, resume_stop):
    """Bound resume at a recorded observation, retaining all prior inputs."""
    end = plan['stop'] if resume_stop is None else resume_stop
    if (type(end) is not int or not cut < end <= plan['stop'] or
            end not in plan['observations']):
        raise ValueError('resume stop must be an observation after the cut and at or before trace end')
    transitions = [(tick, held) for tick, held in plan['transitions'] if tick <= end]
    observations = {tick: row for tick, row in plan['observations'].items() if cut < tick <= end}
    return end, transitions, observations


def run(trace, rom, elf, runner, output, stop=None, export_save=False, save_input=None, saved_state=None, cut=None, physical=False, save_format='legacy', resume_stop=None):
    if save_format not in save.SAVE_FORMATS:
        raise ValueError('unsupported Z7 save format')
    if export_save and (save_input or saved_state or stop is not None):
        raise ValueError('save export requires a cut, without resume or prefix options')
    if bool(save_input) != bool(saved_state) or (save_input and (export_save or stop is not None)):
        raise ValueError('resume requires both save and saved summary, without prefix/export options')
    if resume_stop is not None and not save_input:
        raise ValueError('resume stop requires a save input')
    if bool(export_save or save_input) != (cut is not None):
        raise ValueError('save export and resume require an explicit cut')
    inputs=(trace,rom,elf,runner) + ((save_input,saved_state) if save_input else ())
    before=input_hashes(inputs)
    provenance={name:before[str(path.resolve())] for name,path in
                zip(('trace','rom','elf','runner'),(trace,rom,elf,runner))}
    provenance=save_format_provenance(provenance,save_format)
    plan,pages=build_plan(trace,cut if export_save else stop)
    if cut is not None: save.validate_cut(plan,cut)
    trace_stop=plan['stop']
    original_stop=trace_stop
    resume_transitions=plan['transitions']
    resume_observations=plan['observations']
    if save_input:
        original_stop,resume_transitions,resume_observations=resume_window(plan,cut,resume_stop)
    load_updates=1 if physical else 2
    if export_save or save_input:
        events = ([(t-1,h) for t,h in plan['transitions'] if t <= cut] + save.save_inputs(cut)
                  if export_save else save.resume_inputs(resume_transitions,cut,load_updates))
        end = cut + 4 if export_save else load_updates + original_stop - cut
        checks = ({t:t for t in plan['observations'] if t <= cut} if export_save else
                  {load_updates+t-cut:t for t in resume_observations})
        boundaries=sorted(set(range(1000,end+1,1000)) | set(checks) | {end})
        pages=[]; first=0; index=0
        for boundary in boundaries:
            page_events=[]
            while index < len(events) and events[index][0] < boundary:
                if events[index][0] < first: raise ValueError('input events are out of order')
                page_events.append(events[index]); index+=1
            pages.append({'first':first,'end':boundary,'events':page_events}); first=boundary
        if index != len(events): raise ValueError('input extends past replay')
    else:
        checks={t:t for t in plan['observations']}
    stop=pages[-1]['end']
    sym,sizes=symbols(elf),symbol_sizes(elf)
    if physical:
        import sc64_headless_physical as transport
        capacity = None
    else:
        transport = headless
        mode,capacity=scheduler.schedule_mode(sym,sizes)
        if mode!='windowed' or scheduler.checkpoint_ticks(sym)!=1000:
            raise ValueError('scheduler replay requires the 1000-tick window scheduler; use --physical for release ROMs')
    layouts=make_layout(elf,sizes)
    save_layout = races.dwarf_struct_layout(elf,'RaceSave') if cut is not None else None
    if save_input:
        seed_summary=save.read_summary(saved_state)
        if not isinstance(seed_summary,dict) or seed_summary.get('cut') != cut:
            raise ValueError('saved summary has wrong cut')
        save.verify_provenance(seed_summary,{**provenance,'save':before[str(save_input.resolve())]})
        save.verify_cut_summary(seed_summary,plan['observations'][cut],cut,2)
        save.validate_journal(save_input.read_bytes(),save_layout,seed_summary['world']['frame'],save_format)
    def sample_snapshot(rsp,sym,sizes): return snapshot(rsp,sym,sizes,layouts)
    output.mkdir(parents=True,exist_ok=False)
    proc=rsp=log=None; records=[]
    def u32(name): return int.from_bytes(rsp.read(sym[name],4),'little')
    def write_page(events):
        rsp.write(sym['demo_schedule_events'],b''.join(struct.pack('<2I',*item) for item in events))
        rsp.write(sym['demo_schedule_count'],struct.pack('<I',len(events)))
    try:
        proc,rsp,log=transport.start(runner,rom,output,sym,sizes, save_input=save_input,
                                    save_output=output / "saved.sav" if export_save else None)
        if u32('screen')!=0 or u32('frame')!=0: raise RuntimeError('fresh boot is not at title')
        if not physical:
            write_page(pages[0]['events'][:capacity]);rsp.write(sym['demo_schedule_index'],struct.pack('<I',0))
            rsp.write(sym['demo_schedule_length'],struct.pack('<I',pages[0]['end']))
            rsp.write(sym['demo_schedule_active'],struct.pack('<I',1))
            rsp.breakpoint(sym['game_update']&~1,False);rsp.breakpoint(sym['demo_replay_checkpoint']&~1,True)
            rsp.tick(expected=sym['demo_replay_checkpoint']&~1)
            if u32('demo_schedule_tick')!=0: raise RuntimeError('scheduler did not start at zero')
        initial_timing=timing_snapshot(rsp,sym,sizes)
        if save_input:
            if physical:
                rsp.play_to(load_updates, events)
            else:
                rsp.breakpoint(sym['demo_replay_checkpoint']&~1,False)
                rsp.breakpoint(sym['game_update']&~1,True)
                for _ in range(16):
                    rsp.tick(expected=sym['game_update']&~1)
                    if u32('demo_schedule_tick') == 2: break
                else: raise RuntimeError('fresh-process load did not reach two updates')
            restored=save.saved_summary(rsp,sym,sizes,elf,cut,sample_snapshot)
            saved=save.read_summary(saved_state)
            save.verify_restored(saved,restored,cut,z7.MISSION)
            save.verify_cut_summary(restored,plan['observations'][cut],cut,1)
            (output/'restored-state.json').write_text(json.dumps(restored,indent=2)+'\n')
            if not physical:
                rsp.breakpoint(sym['game_update']&~1,False)
                rsp.breakpoint(sym['demo_replay_checkpoint']&~1,True)
        for i,page in enumerate(pages):
            tick=page['end']
            if physical:
                rsp.play_page(page)
                if rsp.completed != tick:
                    raise RuntimeError('physical input did not reach its update boundary')
            else:
                chunks=[page['events'][n:n+capacity] for n in range(0,len(page['events']),capacity)] or [[]]; active=0
                if len(chunks)>1:
                    active=play_dense(rsp,sym,page,chunks,write_page)
                rsp.tick(expected=sym['demo_replay_checkpoint']&~1)
                if u32('demo_schedule_tick')!=tick or active!=len(chunks)-1 or u32('demo_schedule_index')!=len(chunks[-1]) or u32('demo_schedule_count')!=len(chunks[-1]):
                    raise RuntimeError('scheduler did not consume its page')
            if tick in checks:
                state=snapshot(rsp,sym,sizes,layouts)
                if u32('race_selected_mission')!=z7.MISSION: raise RuntimeError('unexpected selected mission')
                compare(plan['observations'][checks[tick]],state,checks[tick])
                state['main_loop_counters']=timing_snapshot(rsp,sym,sizes)
                records.append({'tick':tick,**state});(output/'progress.json').write_text(json.dumps(records,indent=2)+'\n')
            if not physical and i+1<len(pages):
                write_page(pages[i+1]['events'][:capacity]);rsp.write(sym['demo_schedule_length'],struct.pack('<I',pages[i+1]['end']))
        if export_save:
            paused_state=snapshot(rsp,sym,sizes,layouts)
            expected=plan['observations'][cut]
            mismatch={k:[expected[k],paused_state[k]] for k in z7.SCALARS+z7.LISTS
                      if k != 'screen' and expected[k] != paused_state[k]}
            if mismatch: raise RuntimeError(f'pause save changed cut state: {mismatch}')
            sample=save.saved_summary(rsp,sym,sizes,elf,cut,sample_snapshot)
            if sample['mission']!=z7.MISSION or sample['world']['screen']!=2:
                raise RuntimeError('ordinary save did not remain in Z7 pause')
            save.verify_cut_summary(sample,expected,cut,2)
            rsp.export_sram()
            if not (output/'saved.sav').is_file() or (output/'saved.sav').stat().st_size != 32768:
                raise RuntimeError('SRAM export is missing or incomplete')
            journal=save.validate_journal((output/'saved.sav').read_bytes(),save_layout,sample['world']['frame'],save_format)
            sample['provenance']={**provenance,'save':hashlib.sha256((output/'saved.sav').read_bytes()).hexdigest()}
            (output/'saved-state.json').write_text(json.dumps(sample,indent=2)+'\n')
        full=(bool(save_input) and original_stop==trace_stop) or (not export_save and not save_input and stop==trace_stop)
        if full and (not records or records[-1]['tick']!=stop or records[-1]['won']!=1 or records[-1]['outcome']!=1 or records[-1]['lost']!=0): raise RuntimeError('normal ROM did not win')
        require_unchanged(inputs,before)
        result={'status':'won' if full else 'prefix','stop':stop,'checkpoint_count':len(records),'checkpoints':records,'scope':'named Z7 model fields, entity counts and economy; not full entity arrays','sha256':before,
                'transport': 'physical-buttons' if physical else 'debug-scheduler',
                'timing':timing_delta(initial_timing,timing_snapshot(rsp,sym,sizes),stop)}
        if export_save:
            result['save']={'cut':cut,'journal':journal,'sha256':input_hashes((output/'saved.sav',output/'saved-state.json'))}
        if save_input:
            result['resume']={'cut':cut,'restored_matches':True,'seed_file_unchanged':True}
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
    ap.add_argument('--stop',type=int); ap.add_argument('--cut',type=int)
    ap.add_argument('--resume-stop',type=int,help='Bound a resume at a recorded observation; short resumes are prefixes')
    ap.add_argument('--save-format',choices=save.SAVE_FORMATS,default='legacy',
                    help='Explicit journal namespace; transport1 accepts only the zero-impact campaign trailer')
    ap.add_argument('--physical',action='store_true',help='Use emulator buttons; supports release ROMs without scheduler symbols')
    ap.add_argument('--export-save',action='store_true'); ap.add_argument('--save-input',type=Path); ap.add_argument('--saved-state',type=Path)
    a=ap.parse_args();run(a.trace,a.rom,a.elf,a.runner,a.output,a.stop,a.export_save,a.save_input,a.saved_state,a.cut,a.physical,a.save_format,a.resume_stop)

if __name__=='__main__':main()
