#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a normal-input Z10 controller from a retained Z9 result.

The tape's menu/briefing prefix is replaced by loading the prior result and
pressing A. All observable checkpoint fields below are required to match; the
controller-only route leg and lifetime peak counters are not ROM state.
"""
import argparse
import hashlib
import json
from pathlib import Path
from sc64_p8_arm_replay import load_trace
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import BUILDING_FIELDS, UNIT_FIELDS, PhysicalGame, summary


def require(ok, message):
    if not ok: raise ValueError(message)


def compare_checkpoint(row, observed):
    diff = {k: [row.get(k), v] for k, v in observed.items() if row.get(k) != v}
    require(not diff, f'checkpoint {row.get("call_index")} mismatch: {diff}')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('seed','trace','rom','elf','runner','out'):
        p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args()
    inputs=(a.seed,a.trace,a.rom,a.elf,a.runner)
    hashes={str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in inputs}
    plan=load_trace(a.trace, mission=21)
    rows=[json.loads(x) for x in a.trace.read_text().splitlines() if x.strip()]
    checks=[x for x in rows if x.get('kind')=='checkpoint']
    require(checks and checks[0]['label']=='launch' and checks[0]['frame']==1 and
            checks[0]['call_index']==38 and checks[-1]==plan['final'] and checks[-1]['won']==1,
            'unexpected launch or final checkpoint')
    require(all(x['call_index']<=y['call_index'] for x,y in zip(checks,checks[1:])), 'unordered checkpoints')
    keys=[(i-32,k) for i,k in plan['transitions'] if i>=38]
    u={k:i+1 for i,k in enumerate(UNIT_FIELDS)}; b={k:i+1 for i,k in enumerate(BUILDING_FIELDS)}
    _,core=dwarf_struct_layout(a.elf,'Sc64Z8SchedulerState'); _,late=dwarf_struct_layout(a.elf,'Sc64ZlSchedulerState')
    _,ai=dwarf_struct_layout(a.elf,'Sc64ZlAiState')
    unit_rule_size, unit_rule=dwarf_struct_layout(a.elf,'Sc64RaceUnitRule')
    building_rule_size, building_rule=dwarf_struct_layout(a.elf,'Sc64RaceBuildingRule')
    a.out.mkdir();
    def start(name,seed,dest):
        path=a.out/name;path.mkdir();(path/'headless.log').symlink_to('/dev/null')
        return PhysicalGame(a.runner,a.rom,a.elf,path,save_input=seed,save_output=dest)
    def scalar(g,name,layout,field,offset=0):
        pos=offset+layout[field]; width=layout.widths[field]
        require(width in (1,2,4) and pos+width<=g.sizes[name], f'invalid {name}.{field}')
        return int.from_bytes(g.rsp.read(g.sym[name]+pos,width),'little')
    def terminal(g):
        return dict(world=summary(g.state()),completed=g.u32('race_completed'),mission=g.u32('race_selected_mission'),
                    won=g.u32('won'),lost=g.u32('lost'),unsupported=g.u32('mission_unsupported_count'),
                    runtime=hashlib.sha256(g.rsp.read(g.sym['race_zl_sched'],g.sizes['race_zl_sched'])).hexdigest())
    g=start('play',a.seed,a.out/'victory.sav'); observations=[]
    try:
        for k in (0,0,512,0):g.step(k)
        prior=g.u32('race_completed')
        require(prior & (1<<20) and g.u32('won') and g.u32('race_selected_mission')==20,
                'seed is not Z9 victory')
        g.tap(1); require(g.u32('frame')==1 and g.u32('race_selected_mission')==21,'launch failed')
        urules=g.rsp.read(g.sym['sc64_race_unit_rules'],g.sizes['sc64_race_unit_rules'])
        brules=g.rsp.read(g.sym['sc64_race_building_rules'],g.sizes['sc64_race_building_rules'])
        def rule(data, size, layout, typ, field):
            off=typ*size+layout[field];width=layout.widths[field]
            require(typ>=0 and width in (1,2,4) and off+width<=len(data), 'invalid supply rule')
            return int.from_bytes(data[off:off+width], 'little')
        for row in checks:
            g.rsp.play_to(row['call_index']-32,keys);s=g.state();us=s['units'];bs=s['buildings']
            own=[x for x in us if x[u['owner']]==0];halls=[x for x in bs if x[b['owner']]==0 and x[b['type']] in (6,24,33)];town=[x for x in halls if not x[b['under_construction']]]
            observed=dict(frame=s['frame'],screen=s['screen'],won=g.u32('won'),lost=g.u32('lost'),
                side_units=len(own),hydras=sum(x[u['type']]==5 for x in own),mutas=sum(x[u['type']]==20 for x in own),
                drones=sum(x[u['type']]==7 for x in own),hatch=len(town),larva=sum(x[b['larva']] for x in halls),
                queued=sum(x[b['queue_count']] for x in halls),sunk=sum(x[b['owner']]==0 and x[b['type']]==21 and not x[b['under_construction']] for x in bs),
                enemy_units=sum(x[u['owner']]==1 for x in us),enemy_buildings=sum(x[b['owner']]==1 for x in bs),
                bank=[s['minerals'],s['gas']],cursor=[g.u32('cx'),g.u32('cy')],
                supply=[sum(rule(urules,unit_rule_size,unit_rule,x[u['type']],'supply') for x in own),
                        min(400,16*sum(x[u['type']]==8 for x in own)+sum(rule(brules,building_rule_size,building_rule,x[b['type']],'supply_provided') for x in bs if x[b['owner']]==0 and not x[b['under_construction']]))],
                towns=scalar(g,'race_zl_ai',ai,'started'),fault=scalar(g,'race_zl_sched',core,'fault',late['core']))
            compare_checkpoint(row, observed)
            observations.append(dict(call_index=row['call_index'],**observed));print(f'matched {row["call_index"]} frame {s["frame"]}',flush=True)
        end=terminal(g)
        require((end['completed'],end['mission'],end['won'],end['lost'],end['unsupported'])==(prior|(1<<21),21,1,0,0),'final victory mismatch')
        g.rsp.export_sram()
    finally:g.close()
    g=start('fresh',a.out/'victory.sav',a.out/'readback.sav')
    try:
        g.step(512);require(terminal(g)==end,'fresh result differs');g.rsp.export_sram()
        require((a.out/'readback.sav').read_bytes()==(a.out/'victory.sav').read_bytes(),'SRAM changed on reload')
    finally:g.close()
    require(hashes=={str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in inputs},'input changed during replay')
    (a.out/'result.json').write_text(json.dumps(dict(inputs=hashes,terminal=end,checkpoints=observations,
        excluded=['leg','peak_units','peak_buildings','countdown']),indent=2)+'\n')

if __name__=='__main__':main()
