# SPDX-License-Identifier: GPL-3.0-or-later
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_z7_arm_replay as z7
import sc64_z7_normal_replay as replay


def observation(tick, kind='checkpoint'):
    row = {name: 0 for name in z7.SCALARS}
    row.update({name: [0]*8 for name in z7.LISTS})
    row.update(kind=kind, host_tick=tick, screen=1)
    if kind == 'won':
        row.update(screen=3, won=1, outcome=1, started=254, switch10=1)
    return row


def trace(path, events):
    path.write_text(json.dumps({'schema': 1, 'profile':'zerg-z7', 'mission':18, 'events':events}))


class NormalZ7PlanTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / 'trace.json'

    def test_one_based_inputs_and_union_of_boundaries(self):
        trace(self.path, [
            {'kind':'input','host_tick':1,'held':1,'pressed':1},
            observation(12),
            {'kind':'input','host_tick':13,'held':0,'pressed':0},
            observation(1000), observation(1001, 'won')])
        _, pages = replay.build_plan(self.path)
        self.assertEqual([p['end'] for p in pages], [12,1000,1001])
        self.assertEqual(pages[0]['events'], [(0,1)])
        self.assertEqual(pages[1]['events'], [(12,0)])
        self.assertEqual(pages[2]['events'], [])

    def test_dense_boundary_preserves_every_transition(self):
        events=[]
        for tick in range(1,302):
            held = tick % 2
            events.append({'kind':'input','host_tick':tick,'held':held,'pressed':held})
        events.append(observation(302,'won'))
        trace(self.path, events)
        _, pages = replay.build_plan(self.path)
        self.assertEqual(len(pages[0]['events']), 301)
        self.assertEqual(pages[0]['events'][0], (0,1))
        self.assertEqual(pages[0]['events'][-1], (300,1))
        self.assertEqual(sum(len(p['events']) for p in pages),301)

    def test_prefix_and_terminal_checks_fail_closed(self):
        for key, value in [('won',0),('lost',1),('outcome',0),('fault',1),('started',0),('switch10',0)]:
            end=observation(3,'won');end[key]=value
            trace(self.path,[{'kind':'input','host_tick':1,'held':1,'pressed':1},end])
            with self.subTest(key=key), self.assertRaises(ValueError): replay.build_plan(self.path)
        end=observation(3,'won');end['action_ticks']=[0]*7
        trace(self.path,[{'kind':'input','host_tick':1,'held':1,'pressed':1},end])
        with self.assertRaises(ValueError): replay.build_plan(self.path)

    def test_observation_compares_all_model_and_counts(self):
        expected=observation(42)
        actual={k:expected[k] for k in z7.SCALARS+z7.LISTS}
        replay.compare(expected,actual,42)
        for field in z7.SCALARS+z7.LISTS:
            changed=actual.copy();changed[field]=([1]+[0]*7 if field in z7.LISTS else actual[field]+1)
            with self.subTest(field=field), self.assertRaises(RuntimeError): replay.compare(expected,changed,42)

    def test_dense_refill_at_update_breakpoint_and_fail_late(self):
        import struct
        sym={'demo_schedule_tick':1, 'demo_schedule_index':2,
             'demo_replay_checkpoint':4, 'game_update':6}
        events=[(i, (i+1)%2) for i in range(301)]
        chunks=[events[:256],events[256:]]
        class Fake:
            def __init__(self, late=False):
                self.tick_value=-1;self.index=0;self.page=chunks[0];self.seen=[];self.late=late
            def breakpoint(self, address, enabled): pass
            def tick(self, expected):
                self.tick_value+=1
                if self.index<len(self.page) and self.page[self.index][0]==self.tick_value:
                    self.seen.append(self.page[self.index]);self.index+=1
                if self.late and self.tick_value==255:self.index-=1
                if self.late and self.tick_value==256:self.index+=1
            def read(self, address, length):
                return struct.pack('<I', self.tick_value if address==1 else self.index)
            def write(self, address, data):
                assert address==2;self.index=struct.unpack('<I',data)[0]
        f=Fake()
        active=replay.play_dense(f,sym,{'first':0,'end':301},chunks,lambda page:setattr(f,'page',page))
        self.assertEqual(active,1);self.assertEqual(f.seen,events)
        f=Fake(late=True)
        with self.assertRaisesRegex(RuntimeError,'too late'):
            replay.play_dense(f,sym,{'first':0,'end':301},chunks,lambda page:setattr(f,'page',page))

    def test_dense_repeated_breakpoint_stops_are_bounded(self):
        import struct
        sym={'demo_schedule_tick':1,'demo_schedule_index':2,
             'demo_replay_checkpoint':4,'game_update':6}
        events=[(i, i%2) for i in range(301)]
        chunks=[events[:256],events[256:]]
        class Fake:
            def __init__(self, repeats):
                self.tick_value=-1; self.index=0; self.page=chunks[0]; self.seen=[]
                self.repeats=repeats
            def breakpoint(self, address, enabled): pass
            def tick(self, expected):
                if self.tick_value==42 and self.repeats:
                    self.repeats-=1; return
                self.tick_value+=1
                if self.index<len(self.page) and self.page[self.index][0]==self.tick_value:
                    self.seen.append(self.page[self.index]); self.index+=1
            def read(self,address,length):
                return struct.pack('<I',self.tick_value if address==1 else self.index)
            def write(self,address,data):self.index=struct.unpack('<I',data)[0]
        f=Fake(8)
        self.assertEqual(replay.play_dense(f,sym,{'first':0,'end':301},chunks,lambda p:setattr(f,'page',p)),1)
        self.assertEqual(f.seen,events)
        f=Fake(9)
        with self.assertRaisesRegex(RuntimeError,'stalled'):
            replay.play_dense(f,sym,{'first':0,'end':301},chunks,lambda p:setattr(f,'page',p))

    def test_input_hashes_reject_every_changed_artifact(self):
        paths=[Path(self.tmp.name)/n for n in ('trace','rom','elf','runner')]
        for p in paths:p.write_bytes(b'original')
        before=replay.input_hashes(paths)
        replay.require_unchanged(paths,before)
        for p in paths:
            p.write_bytes(b'changed')
            with self.subTest(path=p.name),self.assertRaisesRegex(RuntimeError,'changed'):
                replay.require_unchanged(paths,before)
            p.write_bytes(b'original')

    def test_dwarf_snapshot_counts_and_model_arrays(self):
        from unittest.mock import patch
        from sc64_races_profile import DwarfOffsets
        def offsets(pos, widths=None, signed=None, arrays=None):
            return DwarfOffsets(pos,widths or {},signed or {},arrays or {})
        layouts={
            'Sc64Z7PersistentState':(40,offsets({'runtime':4})),
            'Sc64Z7RuntimeState':(36,offsets(
                {'model':0,'action_ticks':12,'switch10':28,'ai_script_mask':29,
                 'ai_plan_ticks':30,'ai_plan_phase':32,'fault':33},
                {'switch10':1,'ai_script_mask':1,'ai_plan_ticks':2,'ai_plan_phase':1,'fault':1},
                arrays={'action_ticks':(8,2,False)})),
            'Sc64Z7State':(12,offsets({'started':0,'outcome':2,'action_cursor':3},
                {'started':2,'outcome':1},arrays={'action_cursor':(8,1,False)})),
            'Unit':(8,offsets({'alive':0,'owner':1,'raw_type':2,'raw_owner':4},
                {'alive':1,'owner':1,'raw_type':2,'raw_owner':2},
                {'raw_type':True,'raw_owner':True})),
            'Sc64ZpCompactBuilding':(8,offsets({'alive':0,'owner':1,'raw_type':2,'raw_owner':4},
                {'alive':1,'owner':1,'raw_type':2,'raw_owner':2},
                {'raw_type':True,'raw_owner':True}))}
        sizes={'race_z7':40,'u':16,'b':16,**{n:4 for n in ('screen','frame','won','lost','minerals','gas')}}
        with patch.object(replay.races,'dwarf_struct_layout',side_effect=lambda elf,name:layouts[name]):
            decoded=replay.make_layout(Path('fake.elf'),sizes)
        memory=bytearray(256);sym={'race_z7':0,'u':64,'b':80}
        for i,name in enumerate(('screen','frame','won','lost','minerals','gas')):
            sym[name]=100+i*4;memory[100+i*4:104+i*4]=(i+1).to_bytes(4,'little')
        memory[4:6]=(254).to_bytes(2,'little');memory[6]=1
        memory[7:15]=bytes(range(8))
        for i in range(8):memory[16+2*i:18+2*i]=(i+20).to_bytes(2,'little')
        memory[32]=1;memory[33]=15;memory[34:36]=(158).to_bytes(2,'little')
        memory[36]=2;memory[37]=0
        # One friendly Drone and one enemy row in each bank.
        for pos,owner,raw,raw_owner in [(64,0,41,3),(72,1,38,4),(80,0,132,3),(88,1,144,4)]:
            memory[pos]=1;memory[pos+1]=owner
            memory[pos+2:pos+4]=raw.to_bytes(2,'little',signed=True)
            memory[pos+4:pos+6]=raw_owner.to_bytes(2,'little',signed=True)
        class Rsp:
            def read(self,address,length):return bytes(memory[address:address+length])
        out=replay.snapshot(Rsp(),sym,sizes,decoded)
        self.assertEqual((out['started'],out['outcome'],out['switch10'],out['ai_plan_ticks']),(254,1,1,158))
        self.assertEqual(out['action_cursors'],list(range(8)))
        self.assertEqual(out['action_ticks'],list(range(20,28)))
        self.assertEqual((out['side_units'],out['enemy_units'],out['workers'],out['side_buildings'],out['enemy_buildings']),(1,1,1,1,1))
        layouts['Sc64Z7State'][1].arrays['action_cursor']=(7,1,False)
        with patch.object(replay.races,'dwarf_struct_layout',side_effect=lambda elf,name:layouts[name]):
            with self.assertRaises(ValueError):replay.make_layout(Path('fake.elf'),sizes)

    def test_bad_transition_pressed_mask_rejected(self):
        trace(self.path,[{'kind':'input','host_tick':1,'held':1,'pressed':0},observation(3,'won')])
        with self.assertRaises(ValueError): replay.build_plan(self.path)

if __name__ == '__main__': unittest.main()
