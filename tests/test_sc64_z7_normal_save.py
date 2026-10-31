# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import sys
import struct
from unittest.mock import patch
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_z7_normal_save as save
import sc64_z7_normal_replay as replay


class SaveTest(unittest.TestCase):
    def plan(self):
        return {'stop': 20, 'transitions': [(1, 1), (4, 0), (11, 4), (12, 0)],
                'observations': {10: {'screen': 1, 'won': 0, 'lost': 0, 'fault': 0}}}

    def test_cut_must_be_neutral_play_observation(self):
        plan = self.plan()
        save.validate_cut(plan, 10)
        for cut in (0, 4, 20, None):
            with self.assertRaises(ValueError): save.validate_cut(plan, cut)
        plan['transitions'].insert(2, (9, 2))
        with self.assertRaisesRegex(ValueError, 'neutral'): save.validate_cut(plan, 10)
        plan = self.plan(); plan['observations'][10]['screen'] = 2
        with self.assertRaisesRegex(ValueError, 'play'): save.validate_cut(plan, 10)

    def test_scheduler_save_sequence_and_boundary_transition(self):
        # Simulate the scheduler's held/pressed semantics and pause behavior.
        events = save.save_inputs(10) + [(14, 4), (15, 0)]
        held = previous = 0; screen = 1; actions = []
        for tick in range(10, 16):
            held = dict(events).get(tick, held); pressed = held & ~previous
            if screen == 1 and pressed & save.START: screen = 2; actions.append((tick, 'pause'))
            elif screen == 2 and pressed & save.B: actions.append((tick, 'save'))
            previous = held
        self.assertEqual(actions, [(10, 'pause'), (12, 'save')])
        self.assertEqual(save.resume_inputs(self.plan()['transitions'], 10),
                         [(0, 512), (1, 0), (2, 4), (3, 0)])

    def test_held_reestablish_and_immediate_override(self):
        self.assertEqual(save.resume_inputs([(1, 4), (15, 0)], 10),
                         [(0, 512), (1, 0), (2, 4), (6, 0)])
        self.assertEqual(save.resume_inputs([(1, 4), (11, 0)], 10),
                         [(0, 512), (1, 0), (2, 0)])

    def test_physical_load_aligns_first_simulation_update(self):
        # A title L update restores frame 10; later updates advance the world.
        class Fake:
            def __init__(self): self.frame=0; self.playing=False; self.previous=0; self.pressed=[]
            def update(self,held):
                pressed=held & ~self.previous; self.previous=held
                if not self.playing and pressed & save.L: self.playing=True; self.frame=10
                elif self.playing: self.frame+=1; self.pressed.append(pressed)
        for transitions, expected in (([(1,1),(4,0),(12,4)], [0,4]),
                                      ([(1,1),(4,0),(11,4),(12,0)], [4,0])):
            inputs=save.resume_inputs(transitions,10,load_updates=1)
            self.assertEqual(inputs[0],(0,save.L))
            fake=Fake();held=0; by_tick=dict(inputs)
            held=by_tick.get(0,held);fake.update(held)
            self.assertEqual(fake.frame,10)
            for tick in (1,2):
                held=by_tick.get(tick,held);fake.update(held)
            self.assertEqual(fake.frame,12);self.assertEqual(fake.pressed,expected)
        with self.assertRaisesRegex(ValueError,'cannot hold L'):
            save.resume_inputs([(1,1),(4,0),(11,save.L)],10,load_updates=1)
        with self.assertRaisesRegex(ValueError,'neutral'):
            save.resume_inputs([(1,1)],10,load_updates=1)
        self.assertEqual(save.resume_inputs([(1,1),(4,0),(11,4)],10,load_updates=2),
                         [(0,save.L),(1,0),(2,4)])

    def test_restored_and_tamper_fail_closed(self):
        world = {'frame': 10, 'screen': 2, 'fingerprint': 'a'*64}
        saved = {'cut': 10, 'mission': 18, 'world': world, 'model': {k: ([0]*8 if k in ('action_cursors','action_ticks') else 0) for k in save.MODEL_FIELDS}}
        restored = copy.deepcopy(saved); restored['world']['screen'] = 1
        save.verify_restored(saved, restored, 10, 18)
        for field, value in [('frame', 11), ('fingerprint', 'b'*64), ('screen', 2)]:
            changed = copy.deepcopy(restored); changed['world'][field] = value
            with self.subTest(field=field), self.assertRaises(RuntimeError):
                save.verify_restored(saved, changed, 10, 18)
        for field in save.MODEL_FIELDS:
            indices = range(8) if isinstance(restored['model'][field], list) else [None]
            for index in indices:
                changed = copy.deepcopy(restored)
                if index is None: changed['model'][field] += 1
                else: changed['model'][field][index] += 1
                with self.subTest(field=field, index=index), self.assertRaises(RuntimeError):
                    save.verify_restored(saved, changed, 10, 18)
        changed = copy.deepcopy(restored); changed['mission'] = 17
        with self.assertRaises(RuntimeError): save.verify_restored(saved, changed, 10, 18)
        with tempfile.TemporaryDirectory() as d:
            paths = [Path(d)/n for n in ('trace','rom','elf','runner','save','summary')]
            for path in paths: path.write_bytes(b'original')
            before = replay.input_hashes(paths)
            for path in paths:
                path.write_bytes(b'tampered')
                with self.subTest(path=path), self.assertRaises(RuntimeError): replay.require_unchanged(paths, before)
                path.write_bytes(b'original')

    def test_journal_independent_record_and_corruption(self):
        from sc64_races_profile import DwarfOffsets
        fields = DwarfOffsets({'version': 0, 'state': 8}, {'version': 4}, {}, {'state': (32,4,True)})
        layout = (160, fields)
        payload = bytearray(160)
        struct.pack_into('<I', payload, 0, 0x53435a54)
        struct.pack_into('<i', payload, 8+2*4, 18)
        struct.pack_into('<i', payload, 8+7*4, 10)
        # Independent producer uses direct FNV-1a arithmetic, then C's four-word header.
        checksum = 2166136261
        for b in payload: checksum = ((checksum ^ b) * 16777619) % 2**32
        image = bytearray(b'\xff'*32768)
        image[:16] = struct.pack('<IIII', 0x53435a54, len(payload), checksum, 1)
        image[16:16+len(payload)] = payload
        self.assertEqual(save.validate_journal(image,layout,10)['sequence'], 1)
        for offset in (0,4,8,12,16,24,32):
            corrupt=image.copy();corrupt[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(ValueError): save.validate_journal(corrupt,layout,10)
        for offset in (0, 8, 16):
            corrupt=image.copy(); corrupt[16+offset] ^= 1
            h=2166136261
            for byte in corrupt[16:16+160]: h=((h ^ byte)*16777619) % 2**32
            struct.pack_into('<I',corrupt,8,h)
            with self.subTest(payload_field=offset), self.assertRaises(ValueError):
                save.validate_journal(corrupt,layout,10)
        for bad in (b'\xff'*32768, image[:-1]):
            with self.assertRaises(ValueError): save.validate_journal(bad,layout,10)
        with self.assertRaises(ValueError): save.validate_journal(image,(161,fields),10)
        with self.assertRaises(ValueError): save.validate_journal(image,layout,11)

    def test_second_slot_and_provenance_are_fail_closed(self):
        from sc64_races_profile import DwarfOffsets
        fields=DwarfOffsets({'version':0,'state':8},{'version':4},{},{'state':(32,4,True)})
        payload=bytearray(160)
        struct.pack_into('<I',payload,0,save.CANONICAL_Z7_EGG_TAG)
        struct.pack_into('<i',payload,16,18)
        struct.pack_into('<i',payload,36,10)
        h=2166136261
        for b in payload: h=((h ^ b)*16777619) % 2**32
        image=bytearray(b'\xff'*32768)
        image[:16]=struct.pack('<4I',save.CANONICAL_Z7_EGG_TAG,160,h,1)
        image[16:176]=payload
        newer=image.copy();newer[16384:16384+176]=image[:176]
        struct.pack_into('<I',newer,16384+12,2)
        corrupt=image.copy();corrupt[16384]=0
        for bad in (newer,corrupt):
            with self.assertRaisesRegex(ValueError,'second SRAM slot'): save.validate_journal(bad,(160,fields),10)
        provenance={'trace':'a','rom':'b','elf':'c','runner':'d','save':'e'}
        summary={'provenance':provenance}
        save.verify_provenance(summary,provenance)
        for key in provenance:
            changed=dict(provenance);changed[key]='different'
            with self.subTest(key=key), self.assertRaises(ValueError): save.verify_provenance(summary,changed)

    def test_cut_sample_requires_all_host_fields_and_world_frame(self):
        import sc64_z7_arm_replay as z7
        reference={key:0 for key in z7.SCALARS}
        reference.update({key:[0]*8 for key in z7.LISTS})
        reference.update(screen=1, frame=10)
        model={key:copy.deepcopy(reference[key]) for key in save.MODEL_FIELDS}
        paused=copy.deepcopy(reference);paused['screen']=2
        summary={'cut':10,'mission':18,'world':{'frame':10,'screen':2,'units':0,'buildings':0},
                 'model':model,'observation':paused}
        save.verify_cut_summary(summary,reference,10,2)
        for key in z7.SCALARS+z7.LISTS:
            changed=copy.deepcopy(summary)
            changed['observation'][key]=[1]*8 if key in z7.LISTS else -1
            with self.subTest(field=key), self.assertRaises(RuntimeError):
                save.verify_cut_summary(changed,reference,10,2)
        changed=copy.deepcopy(summary);changed['world']['frame']=11
        with self.assertRaises(RuntimeError): save.verify_cut_summary(changed,reference,10,2)
        changed=copy.deepcopy(summary);changed['model']['started']=1
        with self.assertRaises(RuntimeError): save.verify_cut_summary(changed,reference,10,2)

    def test_fake_rsp_reads_actual_model_and_summary(self):
        from sc64_races_profile import DwarfOffsets
        def off(pos,widths=None,arrays=None):
            return DwarfOffsets(pos,widths or {},{},arrays or {})
        layouts={
            'Sc64Z7PersistentState':(40,off({'runtime':4})),
            'Sc64Z7RuntimeState':(36,off({'model':0,'action_ticks':12,'switch10':28,
                 'ai_script_mask':29,'ai_plan_ticks':30,'ai_plan_phase':32,'fault':33},
                 {'switch10':1,'ai_script_mask':1,'ai_plan_ticks':2,'ai_plan_phase':1,'fault':1},
                 {'action_ticks':(8,2,False)})),
            'Sc64Z7State':(12,off({'started':0,'outcome':2,'action_cursor':3},
                 {'started':2,'outcome':1},{'action_cursor':(8,1,False)}))}
        mem=bytearray(64); mem[4:6]=(254).to_bytes(2,'little'); mem[6]=1
        mem[7:15]=bytes(range(8))
        for i in range(8): struct.pack_into('<H',mem,16+2*i,20+i)
        mem[32:38]=bytes((1,15,158,0,2,0)); struct.pack_into('<I',mem,48,18)
        class FakeRsp:
            def read(self,address,length): return bytes(mem[address:address+length])
        rsp=FakeRsp(); sym={'race_z7':0,'race_selected_mission':48}
        world={'frame':10,'screen':2,'fingerprint':'a'*64,'units':4,'buildings':2}
        def fake_snapshot(r,s,z):
            state={key:0 for key in replay.z7.SCALARS}
            state.update(replay.model_snapshot(r,s,layouts)); state.update(frame=10,screen=2)
            return state
        with patch.object(save.p8,'persisted_summary',return_value=world):
            sample=save.saved_summary(rsp,sym,{},None,10,
                                      fake_snapshot)
        self.assertEqual(sample['model']['started'],254)
        self.assertEqual(sample['model']['action_cursors'],list(range(8)))
        self.assertEqual(sample['model']['action_ticks'],list(range(20,28)))
        self.assertEqual(sample['model']['ai_plan_ticks'],158)
        restored=copy.deepcopy(sample);restored['world']['screen']=1
        save.verify_restored(sample,restored,10,18)
        mem[32] ^= 1
        with patch.object(save.p8,'persisted_summary',return_value=restored['world']):
            changed=save.saved_summary(rsp,sym,{},None,10,
                                       fake_snapshot)
        with self.assertRaises(RuntimeError): save.verify_restored(sample,changed,10,18)


if __name__ == '__main__': unittest.main()
