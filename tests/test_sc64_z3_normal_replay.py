# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import sc64_z3_normal_replay as replay


def observation(tick,kind):
    d={key:0 for key in replay.FIELDS}
    d.update(kind=kind,host_tick=tick,screen=1,phase=1,chrysalis_hp=250)
    if kind=='won':d.update(screen=3,phase=3,outcome=1,switch7=1,win_presence_condition=1)
    return d


def trace():
    return {'schema':1,'mission':14,'policy':'hybrid','events':[
        {'kind':'input','host_tick':1,'held':1,'pressed':1},
        {'kind':'input','host_tick':2,'held':0,'pressed':0},
        {'kind':'input','host_tick':7,'held':8,'pressed':8},
        observation(7,'start'),
        {'kind':'input','host_tick':8,'held':0,'pressed':0},
        observation(1001,'periodic'),observation(1300,'won')]}


class ContractTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.path=Path(self.tmp.name)/'trace.json'
    def tearDown(self):self.tmp.cleanup()
    def plan(self,data,stop=None):
        self.path.write_text(json.dumps(data));return replay.build_plan(self.path,stop)
    def test_pages_keep_boundaries_and_all_input(self):
        plan,pages=self.plan(trace())
        self.assertEqual([p['end'] for p in pages],[7,1000,1001,1300])
        self.assertEqual([e for p in pages for e in p['events']],[(0,1),(1,0),(6,8),(7,0)])
        self.assertEqual(plan['terminal'],1300)
        self.assertEqual(self.plan(trace(),1000)[1][-1]['end'],1000)
    def test_rejects_bad_contract_and_keys(self):
        for mutation in ('mission','no_start','pressed','opposed','terminal','duplicate','field'):
            d=trace()
            if mutation=='mission':d['mission']=15
            elif mutation=='no_start':d['events'].pop(3)
            elif mutation=='pressed':d['events'][0]['pressed']=0
            elif mutation=='opposed':d['events'][0].update(held=48,pressed=48)
            elif mutation=='terminal':d['events'][-1]['chrysalis_hp']=0
            elif mutation=='duplicate':d['events'][1]['host_tick']=1
            else:del d['events'][3]['cursor_x']
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):self.plan(d)
    def test_symbol_bounds(self):
        self.assertEqual(replay.check_symbol({'x':0x0203fffc},{'x':4},'x',4),0x0203fffc)
        with self.assertRaises(ValueError):replay.check_symbol({'x':0x0203fffe},{'x':4},'x',4)
        with self.assertRaises(ValueError):replay.check_symbol({'x':0x02000000},{'x':2},'x',4)
    def test_all_retained_fields_are_compared(self):
        d=observation(7,'start');actual={k:d[k] for k in replay.FIELDS}
        replay.compare(d,actual,7)
        for k in replay.FIELDS:
            a=copy.copy(actual);a[k]+=1
            with self.subTest(field=k),self.assertRaises(RuntimeError):replay.compare(d,a,7)


class Offsets(dict):
    def __init__(self,mapping,widths,signed=None):
        super().__init__(mapping);self.widths=widths;self.signed=signed or {};self.arrays={}


class LayoutTests(unittest.TestCase):
    def test_compact_hp_reads_dwarf_width_and_no_neighbor_bytes(self):
        # A two-byte HP followed by nonzero bytes catches accidental 4-byte reads.
        z=(3,Offsets({'switch7':0,'sequence':1,'outcome':2},{'switch7':1,'sequence':1,'outcome':1}))
        c=(6,Offsets({'elapsed_frames':0,'phase':4,'mission':5},{'elapsed_frames':4,'phase':1,'mission':1}))
        u=(5,Offsets({'alive':0,'owner':1,'type':2,'order':3,'selected':4},{k:1 for k in ('alive','owner','type','order','selected')}))
        b=(9,Offsets({'alive':0,'owner':1,'type':2,'raw_type':3,'hp':5,'under_construction':7},{'alive':1,'owner':1,'type':1,'raw_type':2,'hp':2,'under_construction':1}))
        layouts={'Sc64Z3State':z,'Sc64RaceCampaignState':c,'Unit':u,'Sc64ZpCompactBuilding':b}
        names=('frame','screen','minerals','gas','supply_used','supply_cap','cx','cy','selected_building','train_type','race_selected_mission','race_z3_action_ticks','race_z3','race_campaign','race_minerals','race_gas','u','b')
        sym={k:i*100 for i,k in enumerate(names)};sizes={k:4 for k in names};sizes.update(race_z3_action_ticks=2,race_z3=3,race_campaign=6,race_minerals=20,race_gas=20,u=5,b=9)
        memory={sym[k]:bytes(sizes[k]) for k in names};memory[sym['race_selected_mission']]=(14).to_bytes(4,'little');memory[sym['race_campaign']]=bytes([0,0,0,0,1,14]);memory[sym['b']]=bytes([1,0,0,150,0,250,0,0,0xfe])
        class Rsp:
            def read(self,address,n):
                self_size=memory[address];assert len(self_size)==n;return self_size
        self.assertEqual(replay.snapshot(Rsp(),sym,sizes,layouts)['chrysalis_hp'],250)
        b[1].widths['hp']=4; b[1]['hp']=6
        with self.assertRaises(ValueError):replay.snapshot(Rsp(),sym,sizes,layouts)

if __name__=='__main__':unittest.main()
