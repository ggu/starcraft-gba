# SPDX-License-Identifier: GPL-3.0-or-later
"""Shared physical-replay checkpoint observations for P9 and P10."""
from sc64_skirmish_sram_continuation import UNIT_FIELDS, BUILDING_FIELDS

def observe_p9(g, n, runtime, runner, runner_size):
    r, sym, sizes = g.rsp, g.sym, g.sizes
    u = {k: i + 1 for i, k in enumerate(UNIT_FIELDS)}
    b = {k: i + 1 for i, k in enumerate(BUILDING_FIELDS)}
    s = g.state()
    units, buildings = s['units'], s['buildings']
    def hero(raw, fields):
        return next(([x[u[k]] for k in fields] for x in units if x[u['raw_type']]==raw),
                    [-1]*len(fields))
    switches=n('race_p9', runtime, 'switches')
    cerebrate=[0,-1]
    for i,owner in enumerate((6,5)):
        if not switches & (1<<i):
            actor=next((x for x in buildings if x[b['raw_type']]==151 and
                        x[b['raw_owner']]==owner),None)
            if actor:
                cerebrate=[i+1,actor[b['hp']]]
                break
    state=r.read(sym['race_p9'],sizes['race_p9'])
    def runner_value(i,key):
        start=runtime['runner']+runner_size*i+runner[key]
        return int.from_bytes(state[start:start+runner.widths[key]],'little')
    return dict(frame=s['frame'], screen=s['screen'], won=n('won'), lost=n('lost'),
                side_units=sum(x[u['owner']]==0 for x in units),
                probes=sum(x[u['owner']]==0 and x[u['raw_type']]==64 for x in units),
                enemy_units=sum(x[u['owner']]==1 for x in units),
                enemy_buildings=sum(x[b['owner']]==1 for x in buildings),
                cerebrate=cerebrate, zeratul=hero(75,('owner','hp','shield','x','y')),
                fenix=hero(78,('owner','hp','shield')), bank=[s['minerals'],s['gas']],
                supply=[n('supply_used'),min(n('supply_cap'), 400)], switches=switches,
                done=[runner_value(i,'done') for i in range(4)], outcome=runner_value(0,'outcome'))

def observe_p10(g, n):
    u = {k: i + 1 for i, k in enumerate(UNIT_FIELDS)}
    b = {k: i + 1 for i, k in enumerate(BUILDING_FIELDS)}
    s = g.state()
    units, buildings = s['units'], s['buildings']
    def hero(raw):
        return next(([x[u['owner']], x[u['hp']]] for x in units if x[u['raw_type']] == raw), [-1, -1])
    return dict(frame=s['frame'], screen=s['screen'], won=n('won'), lost=n('lost'),
                side_units=sum(x[u['owner']]==0 for x in units),
                scvs=sum(x[u['owner']]==0 and x[u['raw_type']]==7 for x in units),
                enemy_units=sum(x[u['owner']]==1 for x in units),
                enemy_buildings=sum(x[b['owner']]==1 for x in buildings),
                overmind_hp=next((x[b['hp']] for x in buildings if x[b['raw_type']]==147), -1),
                zeratul=hero(75), gantrithor=hero(82), hyperion=hero(28),
                bank=[s['minerals'], s['gas']], cursor=[n('cx'), n('cy')])
