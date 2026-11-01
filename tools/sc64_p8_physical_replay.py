#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ordinary physical P8 controller replay; no guest writes or scheduler injection.

The recorded pause save is exported after its B release and before START. A
fresh process loads with L and resumes with the original START-release update.
Only named state and bounded semantic world fields are compared.
"""
import argparse
import hashlib
import json
from pathlib import Path

import sc64_headless_physical as physical
import sc64_p8_arm_replay as tape
import sc64_p8_normal_replay as normal
import sc64_p8_physical_save as save
import sc64_races_profile as races
from emulator_smoke import symbols, symbol_sizes


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def scalar(data, layout, name):
    offset = layout[1][name]
    width = layout[1].widths[name]
    if width not in (1, 2, 4) or offset < 0 or offset + width > len(data):
        raise ValueError('invalid DWARF scalar ' + name)
    return int.from_bytes(data[offset:offset + width], 'little', signed=layout[1].signed.get(name, False))


def snapshot(rsp, sym, sizes, elf):
    def read(name):
        if sizes.get(name) != 4:
            raise ValueError('invalid scalar symbol ' + name)
        return int.from_bytes(rsp.read(sym[name], 4), 'little', signed=True)
    result = {k: read(k) for k in ('screen', 'frame', 'won', 'lost', 'race_selected_mission', 'race_mode', 'race_completed')}
    state = races.dwarf_struct_layout(elf, 'Sc64P8PersistentState')
    runtime = races.dwarf_struct_layout(elf, 'Sc64P8RuntimeState')
    if sizes['race_p8'] != state[0] or 'runtime' not in state[1]:
        raise ValueError('unexpected P8 state layout')
    raw = rsp.read(sym['race_p8'], state[0]); base = state[1]['runtime']
    if base + 3 * runtime[0] > len(raw):
        raise ValueError('P8 runtime array exceeds state')
    result['runners'] = [{n: scalar(raw[base+i*runtime[0]:base+(i+1)*runtime[0]], runtime, n)
                          for n in ('owner','outcome','fault','started','running','done','waiting')}
                         for i in range(3)]
    result['p8_persistent_sha256'] = hashlib.sha256(raw).hexdigest()
    result['wait_ms'] = [int.from_bytes(raw[state[1]['wait_ms']+i*4:state[1]['wait_ms']+i*4+4], 'little') for i in range(3)]
    result['pass_timer'] = scalar(raw,state,'pass_timer')
    campaign = races.dwarf_struct_layout(elf, 'Sc64RaceCampaignState')
    if sizes.get('race_campaign') != campaign[0]:
        raise ValueError('invalid campaign state layout')
    campaign_raw = rsp.read(sym['race_campaign'],campaign[0])
    result['campaign'] = {n:scalar(campaign_raw,campaign,n) for n in
                          ('elapsed_frames','objective_flags','mission','phase')}
    building = races.dwarf_struct_layout(elf, 'Sc64ZpCompactBuilding')
    if sizes['b'] % building[0]: raise ValueError('invalid building bank')
    bank = rsp.read(sym['b'],sizes['b'])
    result['stasis_cells'] = [scalar(bank[i:i+building[0]],building,'hp')
        for i in range(0,len(bank),building[0])
        if scalar(bank[i:i+building[0]],building,'alive') and
           scalar(bank[i:i+building[0]],building,'raw_type') == 168]
    result['world'] = normal.persisted_summary(rsp, sym, sizes, elf)
    return result


def validate_plan(path):
    plan = tape.load_trace(path)
    final = plan['final']; mark = plan['save']
    if not mark or final['won'] != 1 or final['lost'] != 0 or final['screen'] != 3:
        raise ValueError('trace needs one ordinary save and a final win')
    # Trace indices are zero-based game_update calls.  The save's START, B,
    # START taps each take two calls; stop after B release while still paused.
    cut = mark['first_call'] + 4
    expected = [(mark['first_call']+i, v) for i,v in enumerate((8,0,2,0,8,0))]
    if [(t,v) for t,v in plan['transitions'] if mark['first_call'] <= t < cut+2] != expected:
        raise ValueError('unexpected pause-save button sequence')
    if mark['resume_call'] != cut + 2:
        raise ValueError('unexpected resume alignment')
    checkpoints = {}
    for line in path.read_text().splitlines():
        row = json.loads(line)
        if row.get('kind') == 'checkpoint':
            tick = row['call_index']
            if type(tick) is not int or tick in checkpoints or not 0 <= tick <= plan['stop']:
                raise ValueError('invalid or duplicate checkpoint')
            checkpoints[tick] = row
    if plan['stop'] not in checkpoints:
        raise ValueError('missing final checkpoint')
    plan['checkpoints'] = checkpoints
    return plan, cut


def resume_events(transitions, cut):
    # One fresh L update replaces the original START-close update at cut.
    return [(0,512)] + [(t-cut,v) for t,v in transitions if t > cut]


def resume_boundary(tick, cut):
    if tick <= cut: raise ValueError('resume boundary must follow cut')
    return tick-cut


def run(trace, rom, elf, runner, output, *, resume=None, summary=None, stop=None):
    plan, cut = validate_plan(trace)
    if bool(resume) != bool(summary):
        raise ValueError('resume requires save and summary')
    if stop is None: stop = plan['stop'] if resume else cut
    if not cut <= stop <= plan['stop'] or (not resume and stop != cut):
        raise ValueError('export stops at the save cut; resume stop must follow it')
    inputs = (trace,rom,elf,runner) + ((resume,summary) if resume else ())
    before = {str(p.resolve()): digest(p) for p in inputs}
    sym, sizes = symbols(elf), symbol_sizes(elf)
    layout = races.dwarf_struct_layout(elf, 'RaceSave')
    if resume:
        seed = json.loads(summary.read_text())
        if seed['provenance'] != {str(p.resolve()): digest(p) for p in (trace,rom,elf,runner)} or seed['save_sha256'] != digest(resume):
            raise ValueError('save provenance differs')
        save.validate_neutral_journal(resume.read_bytes(), layout, seed['snapshot']['frame'])
        events = resume_events(plan['transitions'],cut)
        end = resume_boundary(stop,cut)
    else:
        events = [(t,v) for t,v in plan['transitions'] if t < cut]
        end = cut
    output.mkdir(parents=True, exist_ok=False)
    proc = rsp = log = None
    try:
        proc,rsp,log = physical.start(runner,rom,output,sym,sizes,save_input=resume,
                                       save_output=None if resume else output/'saved.sav')
        if resume:
            rsp.play_to(1, events)
            restored = snapshot(rsp,sym,sizes,elf)
            saved = seed['snapshot']
            if restored['screen'] != 1 or restored['race_selected_mission'] != 27:
                raise RuntimeError('fresh process did not load P8')
            normal.verify_restored(saved['world'],restored['world'])
            if restored['p8_persistent_sha256'] != saved['p8_persistent_sha256']:
                raise RuntimeError('P8 persistent state differs after load')
            (output/'restored.json').write_text(json.dumps(restored,indent=2)+'\n')
        records=[]
        boundaries = sorted(t for t in plan['checkpoints'] if (cut if resume else 0) < t <= stop)
        if stop not in boundaries: boundaries.append(stop)
        for tick in boundaries:
            native_tick = resume_boundary(tick,cut) if resume else tick
            rsp.play_to(native_tick,events)
            observed = snapshot(rsp,sym,sizes,elf)
            if tick in plan['checkpoints']:
                expected=plan['checkpoints'][tick]
                for name in ('screen','frame','won','lost'):
                    if observed[name] != expected[name]:
                        raise RuntimeError(f'checkpoint {tick} {name} differs')
                if [r['fault'] for r in observed['runners']] != expected['fault'] or observed['runners'][0]['outcome'] != expected['outcome']:
                    raise RuntimeError(f'checkpoint {tick} runtime differs')
                hp=observed['stasis_cells'][0] if len(observed['stasis_cells']) == 1 else -1
                if hp != expected['stasis_hp']:
                    raise RuntimeError(f'checkpoint {tick} Stasis Cell differs')
                records.append({'tick':tick,'snapshot':observed})
                (output/'progress.json').write_text(json.dumps(records,indent=2)+'\n')

        if observed['race_selected_mission'] != 27 or any(r['fault'] for r in observed['runners']):
            raise RuntimeError('wrong mission or P8 runtime fault')
        result = {'status':'prefix','cut':cut,'stop':stop,'snapshot':observed,'inputs':before,
                  'scope':'physical inputs; named P8 runtime and bounded saved-world fields','checkpoints':records}
        if not resume:
            if observed['screen'] != 2 or observed['frame'] != plan['save']['frame']-1:
                raise RuntimeError('physical pause/save frame differs from host trace')
            rsp.export_sram()
            image=output/'saved.sav'
            result['journal']=save.validate_neutral_journal(image.read_bytes(),layout,observed['frame'])
            saved={'snapshot':observed,'provenance':before,'save_sha256':digest(image)}
            (output/'saved-state.json').write_text(json.dumps(saved,indent=2)+'\n')
        elif stop == plan['stop']:
            if (observed['screen'] != 3 or observed['won'] != 1 or observed['lost'] != 0 or observed['runners'][0]['outcome'] != 1 or observed['race_mode'] != 0 or not observed['race_completed'] & (1 << 27) or not observed['runners'][0]['done'] & (1 << 4) or observed['campaign']['mission'] != 27 or observed['campaign']['phase'] != 3 or not observed['campaign']['objective_flags'] & 1 or observed['stasis_cells']):
                raise RuntimeError('physical continuation did not reach P8 WIN')
            result['status']='won'
        if before != {str(p.resolve()):digest(p) for p in inputs}:
            raise RuntimeError('replay input changed')
        (output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        return result
    finally:
        if proc is not None:
            proc.terminate()
            try: proc.wait(timeout=3)
            except Exception: proc.kill(); proc.wait()
            if proc.stdin: proc.stdin.close()
            if proc.stdout: proc.stdout.close()
            if log: log.close()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('trace','rom','elf','runner','output'): p.add_argument('--'+n,required=True,type=Path)
    p.add_argument('--resume',type=Path);p.add_argument('--summary',type=Path);p.add_argument('--stop',type=int)
    a=p.parse_args();print(json.dumps(run(a.trace,a.rom,a.elf,a.runner,a.output,resume=a.resume,summary=a.summary,stop=a.stop)))

if __name__ == '__main__': main()
