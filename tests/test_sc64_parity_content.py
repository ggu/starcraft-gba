#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checks for the offline parity layer's content half (tools/sc64_parity_content.py)
and the sound id map tools/sc64_audio_pack.py writes beside its bank.

Every test but the last builds small synthetic iscript programs, CHK rows,
census traces and audio tables, so no game data is needed. The last runs the
real tool only when the private ROM inputs are present and skips otherwise.
"""
import json
import os
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

from tools import sc64_audio_pack as pack
from tools import sc64_parity_content as pc
from tools import sc64_parity_tables as pt

ROOT = Path(__file__).resolve().parents[1]


def op(code, *operands):
    """Encode one iscript instruction; operands are (width, value) pairs."""
    out = bytes([code])
    for width, value in operands:
        out += value.to_bytes(width, 'little')
    return out


def playfram(f):
    return op(0, (2, f))


def wait(n):
    return op(5, (1, n))


def goto(target):
    return op(7, (2, target))


class IscriptWalkTest(unittest.TestCase):
    def test_linear_loop_with_sound(self):
        program = playfram(17) + wait(2) + op(24, (2, 5)) + playfram(34) + wait(1) + goto(0)
        seq = pc.rom_sequence(program, 0)
        self.assertEqual(seq['steps'], [(17, 2, 2), (34, 1, 1)])
        self.assertEqual(seq['end'], 'loop')
        self.assertEqual(seq['loop'], 0)
        self.assertEqual(seq['sounds'], [(1, [5])])
        self.assertEqual(seq['cycle'], [(17, 2, 2), (34, 1, 1)])

    def test_wait_before_the_first_playfram_shows_the_loop_frame(self):
        # move 4; wait 1; playfram 17; wait 1; playfram 34; goto start
        program = op(41, (1, 4)) + wait(1) + playfram(17) + wait(1) + playfram(34) + goto(0)
        seq = pc.rom_sequence(program, 0)
        self.assertEqual(seq['steps'], [(None, 1, 1), (17, 1, 1)])
        self.assertEqual(seq['cycle'], [(34, 1, 1), (17, 1, 1)])

    def test_hold_end_and_waitrand(self):
        program = playfram(1) + op(6, (1, 3), (1, 5)) + playfram(2) + op(48)
        seq = pc.rom_sequence(program, 0)
        self.assertEqual(seq['steps'], [(1, 3, 5)])
        self.assertEqual((seq['end'], seq['last_frame']), ('hold', 2))
        self.assertEqual(pc.per_tick(seq['steps'], 6, seq['last_frame']), [1, 1, 1, 2, 2, 2])
        end = pc.rom_sequence(playfram(3) + wait(2) + op(22), 0)
        self.assertEqual((end['steps'], end['end']), ([(3, 2, 2)], 'end'))

    def test_collapse_and_relabel(self):
        self.assertEqual(pc.collapse([(5, 2, 2), (5, 1, 1), (6, 1, 3)]), [(5, 3), (6, [1, 3])])
        self.assertEqual(pc.relabel([51, 34, 51, 0]), [0, 1, 0, 2])
        self.assertEqual(pc.relabel([7, 8, 7, 9]), pc.relabel([51, 34, 51, 0]))

    def test_construction_waits_are_compared_with_openbw_timing(self):
        class Fake:
            def sequence(self, image, animation):
                return pc.rom_sequence(playfram(0) + wait(125) + goto(0), 0)
        stage = {'raw_unit_id': 106, 'stage': 0, 'animation': 'Init', 'image_id': 325}
        same = pc.compare_construction(Fake(), stage, {(106, 0): {'steps': [(0, 125)], 'loop_start': 0,
                                                                   'frame_count': 3}})
        longer = pc.compare_construction(Fake(), stage, {(106, 0): {'steps': [(0, 126)], 'loop_start': 0,
                                                                     'frame_count': 3}})
        missing = pc.compare_construction(Fake(), stage, {})
        self.assertEqual(same['verdict'], 'match')
        self.assertEqual(longer['verdict'], 'differs')
        self.assertIn('[1]', longer['note'])
        self.assertEqual(missing['verdict'], 'absent')

    def test_waitless_loop_after_a_timed_intro_is_reported(self):
        program = playfram(1) + wait(2) + op(41, (1, 4)) + goto(5)
        seq = pc.rom_sequence(program, 0)
        self.assertEqual((seq['steps'], seq['end'], seq['cycle']), ([(1, 2, 2)], 'waitless loop', []))

    def test_construction_loop_against_a_held_last_frame(self):
        class Fake:
            def __init__(self, program):
                self.program = program
            def sequence(self, image, animation):
                return pc.rom_sequence(self.program, 0)
        stage = {'raw_unit_id': 131, 'stage': 0, 'animation': 'Init', 'image_id': 1}
        looping = Fake(playfram(1) + wait(2) + playfram(2) + wait(2) + goto(0))
        held = {(131, 0): {'steps': [(1, 2), (2, 2)], 'loop_start': pc.HOLD_LAST, 'frame_count': 3}}
        looped = {(131, 0): {'steps': [(1, 2), (2, 2)], 'loop_start': 0, 'frame_count': 3}}
        self.assertEqual(pc.compare_construction(looping, stage, held)['verdict'], 'differs')
        self.assertEqual(pc.compare_construction(looping, stage, looped)['verdict'], 'match')
        # Loop entry inside a run of one frame: A x2, A x3, B x1.
        runs = Fake(playfram(1) + wait(2) + wait(3) + playfram(2) + wait(1) + goto(5))
        # The loop re-enters at the second wait with frame 2 still shown (the
        # frame the body leaves), so the steady cycle is 2 x3, 2 x1.
        enters_second = {(131, 0): {'steps': [(1, 2), (2, 3), (2, 1)], 'loop_start': 1, 'frame_count': 3}}
        enters_first = {(131, 0): {'steps': [(1, 2), (1, 3), (2, 1)], 'loop_start': 1, 'frame_count': 3}}
        # No static program shows frame 1 there on the first pass and frame 2
        # in the steady loop, so both bakes differ, each on one of the passes.
        second = pc.compare_construction(runs, stage, enters_second)
        first = pc.compare_construction(runs, stage, enters_first)
        self.assertEqual((second['verdict'], first['verdict']), ('differs', 'differs'))
        self.assertIn('first pass: frame order differs', second['note'])
        self.assertIn('steady cycle: frame order differs', first['note'])
        holding = Fake(playfram(1) + wait(2) + playfram(2) + wait(2) + op(48))
        self.assertEqual(pc.compare_construction(holding, stage, held)['verdict'], 'match')
        self.assertEqual(pc.compare_construction(holding, stage, looped)['verdict'], 'differs')
        # iscript end removes the image; the engine holding it is a difference.
        ending = Fake(playfram(1) + wait(2) + playfram(2) + wait(2) + op(22))
        self.assertEqual(pc.compare_construction(ending, stage, held)['verdict'], 'differs')
        # Unequal step counts cannot hide a loop entry inside a merged run.
        merged = {(131, 0): {'steps': [(1, 5), (2, 1)], 'loop_start': 0, 'frame_count': 3}}
        self.assertEqual(pc.compare_construction(runs, stage, merged)['verdict'], 'differs')

    def test_attack_shorter_than_the_rom_path_differs(self):
        attack = playfram(0) + wait(1) + playfram(17) + wait(1) + playfram(34) + wait(1) + playfram(51) + \
            wait(1) + op(48)
        idle = playfram(17) + wait(1) + playfram(0) + wait(1) + goto(0)

        class Fake:
            def sequence(self, image, animation):
                return pc.rom_sequence(idle if 'ToIdle' in animation else attack, 0)
        def actor(ticks):
            return {'root_image_id': 1, 'modes': [{'target_air': 0, 'animation': 'GndAttkInit', 'status': 'ok',
                                                   'full_duration_source_ticks': ticks}]}
        frames = [0, 17, 34, 51, 17, 0]
        full = {(5, 'ground'): [(t, 100 + f) for t, f in enumerate(frames)]}
        short = {(5, 'ground'): [(t, 100 + f) for t, f in enumerate(frames[:2])]}
        self.assertEqual(pc.compare_attack(Fake(), 5, actor(6), full)[0]['verdict'], 'match')
        self.assertEqual(pc.compare_attack(Fake(), 5, actor(2), short)[0]['verdict'], 'differs')
        # A row keyed at the declared duration is never shown.
        keyed = {(5, 'ground'): full[(5, 'ground')] + [(6, 999)]}
        self.assertEqual(pc.compare_attack(Fake(), 5, actor(6), keyed)[0]['verdict'], 'match')

    def test_composite_and_unsupported_attack_entries(self):
        self.assertTrue(pc.refines([0, 1, 2, 3], [0, 1, 0, 1]))
        self.assertFalse(pc.refines([0, 0, 1], [0, 1, 1]))
        rows = pc.compare_attack(None, 5, {'raw_unit': 5, 'status': 'unsupported_actor_image'}, {})
        self.assertEqual((rows[0]['verdict'], 'unsupported' in rows[0]['note']), ('absent', True))

    def test_attack_with_an_unknown_leading_tick_is_not_a_match(self):
        attack = wait(1) + playfram(17) + wait(1) + op(48)

        class Fake:
            def sequence(self, image, animation):
                return pc.rom_sequence(attack, 0) if 'Init' in animation else None
        actor = {'root_image_id': 1, 'modes': [{'target_air': 0, 'animation': 'GndAttkInit', 'status': 'ok',
                                                'full_duration_source_ticks': 2}]}
        row = pc.compare_attack(Fake(), 5, actor, {(5, 'ground'): [(0, 1), (1, 2)]})[0]
        self.assertEqual(row['verdict'], 'not compared')

    def test_attack_with_no_known_rom_frame_is_not_compared(self):
        class Fake:
            def sequence(self, image, animation):
                return pc.rom_sequence(wait(1) + wait(1) + op(48), 0) if 'Init' in animation else None
        actor = {'root_image_id': 1, 'modes': [{'target_air': 0, 'animation': 'GndAttkInit', 'status': 'ok',
                                                'full_duration_source_ticks': 2}]}
        row = pc.compare_attack(Fake(), 5, actor, {(5, 'ground'): [(0, 1), (1, 2)]})[0]
        self.assertEqual(row['verdict'], 'not compared')

    def test_engine_tables_are_read_from_header_text(self):
        attack = ('static const Sc64UnitAttackPoseStep sc64_unit_attack_raw0_ground_full_steps[2] = {\n'
                  '    {0u,{&sc64_unit_attack_frame_51,&sc64_unit_attack_frame_51}},\n'
                  '    {1u,{&sc64_unit_attack_frame_34,&sc64_unit_attack_frame_34}},\n};\n')
        self.assertEqual(pc.engine_attack_frames(attack), {(0, 'ground'): [(0, 51), (1, 34)]})
        construction = ('static const SC64ConstructionAnimStep sc64_construction_raw_106_stage_0_layer_0_steps[] = {\n'
                        '    { 0u, 126u },\n    { 1u, 4u },\n};\n'
                        'static const SC64ConstructionAnimDesc sc64_construction_raw_106_stage_0_layer_0_animation = {\n'
                        '    sc64_construction_raw_106_stage_0_layer_0_steps, 2u, 0u, 3u\n};\n')
        self.assertEqual(pc.engine_construction(construction),
                         {(106, 0): {'steps': [(0, 126), (1, 4)], 'loop_start': 0, 'frame_count': 3}})


def chk(type_, owner, x, y, resources=0):
    return {'type': type_, 'owner': owner, 'x': x, 'y': y, 'resources': resources, 'section': 'UNIT'}


def gba(units=(), buildings=(), minerals=(), geysers=(), max_units=8, max_buildings=8):
    def row(t, o, x, y):
        return {'type': 0, 'raw_type': t, 'owner': o, 'raw_owner': o, 'x': x, 'y': y, 'hp': 1}
    return {'units': [row(*u) for u in units], 'buildings': [row(*b) for b in buildings],
            'minerals': [{'x': x, 'y': y, 'amount': a} for x, y, a in minerals],
            'geysers': [{'x': x, 'y': y} for x, y in geysers],
            'max_units': max_units, 'max_buildings': max_buildings, 'max_minerals': 8, 'banks': {}}


class CensusTest(unittest.TestCase):
    def test_exact_join_on_halved_position(self):
        rom = [chk(0, 1, 173, 95), chk(106, 1, 1056, 1744), chk(176, 11, 480, 912, 1500),
               chk(188, 11, 100, 200), chk(214, 1, 0, 0)]
        result = pc.compare_census(rom, gba(units=[(0, 1, 86, 47)], buildings=[(106, 1, 528, 872)],
                                            minerals=[(240, 456, 1500)], geysers=[(50, 100)]))
        self.assertEqual(result['verdict'], 'exact')
        self.assertEqual((result['matched'], result['minerals']['placed'], result['geysers']['placed']), (2, 1, 1))

    def test_stacked_placement_displaced_is_a_collision(self):
        rom = [chk(37, 4, 200, 200), chk(37, 4, 202, 200)]
        result = pc.compare_census(rom, gba(units=[(37, 4, 100, 100), (37, 4, 110, 100)]))
        self.assertEqual(result['verdict'], 'collision')
        self.assertEqual(result['rows']['collision'][0]['offset'], [9, 0])

    def test_unit_over_the_cap_is_cap_dropped(self):
        rom = [chk(0, 1, 10, 10), chk(0, 1, 100, 100)]
        result = pc.compare_census(rom, gba(units=[(0, 1, 5, 5)], max_units=1))
        self.assertEqual(result['verdict'], 'cap-dropped')
        self.assertEqual(result['counts']['cap-dropped'], 1)

    def test_missing_extra_or_moved_differs(self):
        rom = [chk(0, 1, 10, 10)]
        self.assertEqual(pc.compare_census(rom, gba(units=[(0, 1, 5, 5), (7, 1, 9, 9)]))['verdict'], 'differs')
        self.assertEqual(pc.compare_census(rom, gba())['counts']['missing'], 1)
        moved = pc.compare_census(rom, gba(units=[(0, 1, 50, 5)]))
        self.assertEqual((moved['verdict'], moved['counts']['moved']), ('differs', 1))
        amount = pc.compare_census([chk(176, 11, 10, 10, 1500)], gba(minerals=[(5, 5, 1000)]))
        self.assertEqual(amount['minerals']['amount_differs'], [{'half': [5, 5], 'rom': 1500, 'gba': 1000}])

    def test_extra_resources_and_doodad_caps(self):
        rows = [chk(0, 1, 10, 10), chk(176, 11, 100, 100, 1500)]
        extra = pc.compare_census(rows, gba(units=[(0, 1, 5, 5)], minerals=[(50, 50, 1500), (80, 80, 1500)]))
        self.assertEqual(extra['verdict'], 'differs')  # an extra mineral is never exact
        self.assertIn('extra 1', pc.resource_text(extra))
        door = dict(chk(209, 6, 400, 400), section='THG2')
        capped = pc.compare_census([door], gba(units=[(0, 1, 5, 5)], buildings=[(106, 1, 9, 9)],
                                               max_units=8, max_buildings=1))
        self.assertEqual(capped['counts']['cap-dropped'], 1)  # against the building table, not units

    def test_coincident_minerals_compare_each_amount(self):
        rom = [chk(176, 11, 10, 10, 1500), chk(176, 11, 11, 10, 1500)]
        guest = gba(minerals=[(5, 5, 500), (5, 5, 1500)])
        result = pc.compare_census(rom, guest)
        self.assertEqual(result['verdict'], 'differs')
        self.assertEqual(result['minerals']['placed'], 2)
        self.assertEqual(result['minerals']['amount_differs'], [{'half': [5, 5], 'rom': 1500, 'gba': 500}])

        rom[1]['resources'] = 500
        self.assertEqual(pc.compare_census(rom, guest)['verdict'], 'exact')

    def test_thg2_units_are_placements_and_sprites_are_not(self):
        payload = struct.pack('<HHHBBH', 209, 320, 96, 6, 0, 0x0000) + \
            struct.pack('<HHHBBH', 119, 64, 64, 6, 0, 0x1000) + \
            struct.pack('<HHHBBH', 205, 32, 48, 1, 0, 0x8000)
        rows = pc.doodad_units(payload)
        self.assertEqual([(r['type'], r['owner'], r['x'], r['y'], r['disabled']) for r in rows],
                         [(209, 6, 320, 96, False), (205, 1, 32, 48, True)])

    def test_trace_parse_keeps_launch_placements_and_bank_history(self):
        text = '\n'.join([
            '#sc64-parity-trace 1',
            'H schema=sc64-parity-trace/1 profile=terran scenario=census-m4 attack=x',
            'H max_units=192 max_buildings=96 max_minerals=80 owners=5 cell_cap=1',
            'S 0 0 5 0 1 0 0 0 1 -1 0',
            '+U 0 4 1', 'U 0 4 37 1 4 812 23 35 35 0 -1 0 812 23 -1 0 0 -1 0 0 0 0 0',
            'B 1 0 106 0 1 528 872 1500 1500 0 0 0 0 0 0 -1 -1 -1 -1 -1 3 0',
            'P 0 0 0 20 36', 'M 0 240 456 355', 'G 0 50 60',
            'S 1 0 6 1 1 0 0 0 1 -1 0',
            'U 0 4 37 1 4 813 23 35 35 0 -1 0 812 23 -1 0 0 -1 0 0 0 0 0',
            'P 0 40 0 20 36',
            'Z steps=1 host_ticks=6 births=2 removals=0 damage=0 sounds=0 text_boxes=0 overflow=0 '
            'write_errors=0 world=0 world_chain=0 body_fnv=0'])
        census = pc.parse_census_trace(text)
        self.assertEqual(census['units'][0]['x'], 812)
        self.assertEqual(census['buildings'][0]['raw_type'], 106)
        self.assertEqual(census['banks'][0]['minerals'], 0)
        self.assertEqual([h['minerals'] for h in census['bank_history'][0]], [0, 40])
        self.assertEqual((census['max_units'], census['geysers'], census['last_step']), (192, [{'x': 50, 'y': 60}], 1))
        with self.assertRaises(ValueError):
            pc.parse_census_trace(text.rsplit('\n', 1)[0])
        with self.assertRaises(ValueError):
            pc.parse_census_trace(text.replace('overflow=0', 'overflow=3'))
        with self.assertRaises(ValueError):  # a short record marks the census absent, not a crash
            pc.parse_census_trace(text.replace('M 0 240 456 355', 'M 0 240'))
        with self.assertRaises(ValueError):
            pc.parse_census_trace(text.replace('S 0 0 5 0 1 0 0 0 1 -1 0', 'S 0 0 5'))

    def test_unreadable_or_unpinned_bank_manifest_is_unavailable(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            header = root / 'bank.h'
            header.write_text('x\n')
            (root / 'cut.json').write_text('{"header_sha2')
            (root / 'list.json').write_text('[]')
            (root / 'bare.json').write_text('{"generated_header": "bank.h"}')
            import hashlib
            digest = hashlib.sha256(header.read_bytes()).hexdigest()
            (root / 'digest-only.json').write_text(json.dumps({'header_sha256': digest}))
            (root / 'empty.json').write_text(json.dumps({'header_sha256': digest, 'entries': []}))
            for name in ('cut.json', 'list.json', 'bare.json'):
                manifest, status = pc.load_manifest(root / name, header)
                self.assertIsNone(manifest, name)
                self.assertTrue(status.startswith('unavailable'), status)
            for name in ('digest-only.json', 'empty.json'):  # pinned, but its data section is missing
                manifest, status = pc.load_manifest(root / name, header, pc.BANK_SECTIONS['walk'])
                self.assertIsNone(manifest, name)
                self.assertTrue(status.startswith('unavailable'), status)

    def test_first_pass_set_resources_rows(self):
        def trigger(record, conditions, group, amount, executed=(0, 1)):
            return {'record': record, 'conditions': conditions,
                    'executed_for_player_values': list(executed) + [0] * 25,
                    'actions': [{'name': 'SetResources', 'group1': group, 'group2': amount,
                                 'unit_type': 0, 'modifier': 7}]}
        elapsed = lambda cmp, n: {'name': 'ElapsedTime', 'comparison': cmp, 'amount': n,
                                  'player_group': 0, 'unit_type': 0}
        command = {'name': 'Command', 'comparison': 0, 'amount': 1, 'player_group': 2, 'unit_type': 229}
        owners = bytes([6, 6, 5, 0, 0, 0, 0, 0])  # players 0-2 active, 3-7 inactive
        triggers = [trigger(0, [elapsed(0, 0)], 13, 40), trigger(1, [elapsed(0, 180)], 1, 99),
                    trigger(2, [command], 2, 5000), trigger(3, [dict(command, amount=5)], 2, 7000)]
        rows = pc.start_banks(triggers, bytes(8), [chk(0, 2, 1, 1)], owners)
        self.assertEqual([(r['trigger'], r['players'], r['amount']) for r in rows], [(0, [1], 40), (2, [2], 5000)])
        # Each executor is evaluated alone: only player 1 commands a unit.
        mine = dict(command, player_group=13)
        both = trigger(4, [mine], 13, 300, executed=(1, 1))
        rows4 = pc.start_banks([both], bytes(8), [chk(0, 1, 1, 1)], owners)
        self.assertEqual([r['players'] for r in rows4], [[1]])
        # A force grant reaches only the force's active slots.
        forces = bytes([0, 0, 1, 0, 0, 1, 1, 1])
        force = trigger(5, [elapsed(0, 0)], 18, 100)
        self.assertEqual(sorted(r['players'][0] for r in pc.start_banks([force], forces, [], owners)), [0, 1])
        census = {'step': 0, 'last_step': 120, 'bank_history': {
            0: [{'step': 0, 'minerals': 0, 'gas': 0}, {'step': 30, 'minerals': 40, 'gas': 0}],
            1: [{'step': 0, 'minerals': 0, 'gas': 0}]}, 'banks': {}}
        self.assertEqual(pc.compare_banks(rows[:1], census, {1: 0})[0]['verdict'], 'match at step 30')
        self.assertEqual(pc.compare_banks(rows[1:], census, {2: 0})[0]['verdict'],
                         'differs (first changed to [40] at step 30)')
        # Two source slots on the human's engine owner cannot be told apart either.
        self.assertTrue(pc.compare_banks(rows[:1], census, {1: 0, 2: 0})[0]['verdict'].startswith('not compared'))
        self.assertTrue(pc.compare_banks(rows[:1], census, {})[0]['verdict'].startswith('absent'))
        # A rescuable slot executes nothing.
        rescue = pc.start_banks([trigger(6, [elapsed(0, 0)], 13, 150, executed=(0, 1, 1))], bytes(8), [],
                                bytes([6, 6, 3, 0, 0, 0, 0, 0]))
        self.assertEqual([r['players'] for r in rescue], [[1]])
        # A minerals+gas grant must arrive as one change of both balances.
        split = [dict(rows[0], resource='minerals+gas', amount=40)]
        apart = {'step': 0, 'last_step': 9, 'bank_history': {0: [
            {'step': 0, 'minerals': 0, 'gas': 0}, {'step': 1, 'minerals': 40, 'gas': 0},
            {'step': 2, 'minerals': 40, 'gas': 40}]}, 'banks': {}}
        self.assertTrue(pc.compare_banks(split, apart, {1: 0})[0]['verdict'].startswith('differs'))
        # Two source slots on one engine owner cannot be told apart.
        shared = {'step': 0, 'last_step': 9, 'bank_history': {1: [{'step': 0, 'minerals': 5000, 'gas': 0}]},
                  'banks': {}}
        self.assertTrue(pc.compare_banks(rows[1:], shared, {2: 1, 3: 1})[0]['verdict'].startswith('not compared'))
        # A computer's grant may live in a campaign wallet the trace lacks.
        self.assertTrue(pc.compare_banks(rows[1:], census, {2: 1})[0]['verdict'].startswith('not compared'))
        # A census stopped early by a modal cannot show a grant that never arrived.
        stopped = dict(census, last_step=3, requested_steps=120)
        self.assertTrue(pc.compare_banks(rows[1:], stopped, {2: 0})[0]['verdict'].startswith('not compared'))
        # Mining that later reaches the granted total does not match it.
        mined = {'step': 0, 'last_step': 120, 'bank_history': {0: [
            {'step': 0, 'minerals': 0, 'gas': 0}, {'step': 20, 'minerals': 8, 'gas': 0},
            {'step': 90, 'minerals': 40, 'gas': 0}]}, 'banks': {}}
        self.assertTrue(pc.compare_banks(rows[:1], mined, {1: 0})[0]['verdict'].startswith('differs'))

    def test_distant_same_type_actor_is_not_a_collision(self):
        rom = [chk(37, 4, 200, 200), chk(37, 4, 202, 200)]
        result = pc.compare_census(rom, gba(units=[(37, 4, 100, 100), (37, 4, 900, 900)]))
        self.assertEqual((result['verdict'], result['counts']['moved']), ('differs', 1))

    def test_door_in_a_full_building_table_is_cap_dropped(self):
        rom = [chk(209, 6, 20, 20), chk(209, 6, 400, 400)]
        result = pc.compare_census(rom, gba(buildings=[(209, 6, 10, 10)], max_buildings=1))
        self.assertEqual(result['verdict'], 'cap-dropped')
        full_units = pc.compare_census(rom, gba(units=[(0, 1, 5, 5)], buildings=[(209, 6, 10, 10)],
                                                max_units=1))
        self.assertEqual(full_units['counts']['missing'], 1)

    def test_blood_bath_needs_each_base_on_its_own_start(self):
        starts = [dict(chk(214, i, 100 + 1000 * i, 100), type=214) for i in range(4)]
        def lobby(*halls):
            census = gba(buildings=[(-1, o, x, y) for o, x, y, _ in halls])
            for b, (_, _, _, kind) in zip(census['buildings'], halls):
                b['type'] = kind
            return census
        halls = {106: 0, 131: 6}  # raw town hall -> engine type in the profile
        good = lobby((0, 50, 50, 6), (1, 550, 50, 0))
        self.assertEqual(pc.skirmish_census(starts, good, halls)['verdict'], 'exact')
        for bad in (lobby((0, 50, 50, 6), (1, 300, 300, 0)),     # off its start
                    lobby((0, 50, 50, 6), (1, 550, 50, 2)),      # a Supply Depot, not a hall
                    lobby((0, 50, 50, 6), (0, 550, 50, 0)),      # both halls one owner
                    lobby((0, 50, 50, 6), (1, 52, 50, 0)),       # both on one start
                    lobby((0, 50, 50, 6), (4, 550, 50, 0)),      # a neutral hall, not the computer's
                    lobby((0, 50, 50, 0), (1, 550, 50, 6)),      # races swapped (Zerg human has a CC)
                    gba()):
            self.assertEqual(pc.skirmish_census(starts, bad, halls)['verdict'], 'differs')


class SkippedComparisonTest(unittest.TestCase):
    """A row whose comparison was partly skipped never reads as a match."""

    class Fake:
        def __init__(self, programs, directional=False):
            self.programs, self.turns = programs, directional
        def sequence(self, image, animation):
            program = self.programs.get(animation)
            return pc.rom_sequence(program, 0) if program is not None else None
        def directional(self, image):
            return self.turns

    def test_attack_recovery_uses_the_frame_shown_at_the_goto(self):
        attack = playfram(34) + wait(1) + playfram(51) + wait(1) + op(48)
        idle = playfram(0) + goto(0)  # ToIdle plays the idle pose and leaves at once
        fake = self.Fake({'GndAttkInit': attack, 'GndAttkToIdle': idle})
        actor = {'root_image_id': 1, 'modes': [{'target_air': 0, 'animation': 'GndAttkInit', 'status': 'ok',
                                                'full_duration_source_ticks': 4}]}
        right = {(5, 'ground'): [(0, 7), (1, 8), (2, 9), (3, 9)]}   # idle pose after the path
        holds = {(5, 'ground'): [(0, 7), (1, 8), (2, 8), (3, 8)]}   # wrongly keeps the attack frame
        self.assertEqual(pc.compare_attack(fake, 5, actor, right)[0]['verdict'], 'match')
        self.assertEqual(pc.compare_attack(fake, 5, actor, holds)[0]['verdict'], 'differs')

    def test_a_looping_attack_path_repeats_its_cycle(self):
        attack = playfram(0) + wait(1) + playfram(17) + wait(1) + goto(0)
        fake = self.Fake({'GndAttkInit': attack})
        actor = {'root_image_id': 1, 'modes': [{'target_air': 0, 'animation': 'GndAttkInit', 'status': 'ok',
                                                'full_duration_source_ticks': 4}]}
        two_cycles = {(5, 'ground'): [(0, 1), (1, 2), (2, 1), (3, 2)]}
        self.assertEqual(pc.compare_attack(fake, 5, actor, two_cycles)[0]['verdict'], 'match')
        huge = dict(actor, modes=[dict(actor['modes'][0], full_duration_source_ticks=10_000)])
        self.assertEqual(pc.compare_attack(fake, 5, huge, {(5, 'ground'): [(0, 1)]})[0]['verdict'], 'absent')

    def test_walk_without_a_steady_loop_is_not_compared(self):
        entry = {'unit_id': 0, 'image_id': 1, 'cycle_ticks': 2, 'steps': [{'tick': 0, 'pose': 1}, {'tick': 1, 'pose': 2}]}
        ending = self.Fake({'Walking': playfram(17) + wait(1) + playfram(34) + wait(1) + op(22)}, True)
        self.assertEqual(pc.compare_walk(ending, entry)['verdict'], 'not compared')
        looping = self.Fake({'Walking': playfram(17) + wait(1) + playfram(34) + wait(1) + goto(0)}, True)
        self.assertEqual(pc.compare_walk(looping, entry)['verdict'], 'match')

    def test_death_duration_counts_from_the_first_frame_and_honours_waitrand(self):
        actor = {'image_id': 1, 'layers': [{'image_id': 1, 'start': 0, 'end': 5, 'remnant': False}]}
        images = {1: {'source_frames': [0, 1]}}
        rand = self.Fake({'Death': playfram(0) + op(6, (1, 2), (1, 4)) + playfram(1) + wait(2) + op(22)})
        self.assertEqual(pc.compare_death(rand, 0, actor, images)[0]['verdict'], 'match')  # 5 in 4-6
        lead = self.Fake({'Death': wait(1) + playfram(0) + wait(2) + playfram(1) + wait(3) + op(22)})
        self.assertEqual(pc.compare_death(lead, 0, actor, images)[0]['verdict'], 'match')  # 2+3 = 5
        loops = self.Fake({'Death': playfram(0) + wait(2) + playfram(1) + wait(3) + goto(0)})
        self.assertEqual(pc.compare_death(loops, 0, actor, images)[0]['verdict'], 'not compared')
        remnant = {'image_id': 1, 'layers': [{'image_id': 1, 'start': 0, 'end': 720, 'remnant': True}]}
        self.assertEqual(pc.compare_death(loops, 0, remnant, images)[0]['verdict'], 'not compared')

    def test_construction_with_an_unknown_end_is_not_compared(self):
        class Fake:
            def sequence(self, image, animation):
                return pc.rom_sequence(playfram(1) + wait(2) + playfram(2) + op(41, (1, 4)) + goto(6), 0)
        stage = {'raw_unit_id': 106, 'stage': 0, 'animation': 'Init', 'image_id': 1}
        baked = {(106, 0): {'steps': [(1, 2)], 'loop_start': 0, 'frame_count': 3}}
        self.assertEqual(pc.compare_construction(Fake(), stage, baked)['verdict'], 'not compared')

        class Leading:
            def sequence(self, image, animation):
                return pc.rom_sequence(wait(3) + playfram(1) + wait(2) + op(48), 0)
        held = {(106, 0): {'steps': [(0, 3), (1, 2)], 'loop_start': pc.HOLD_LAST, 'frame_count': 3}}
        self.assertEqual(pc.compare_construction(Leading(), stage, held)['verdict'], 'not compared')

        class Inherited:
            def sequence(self, image, animation):
                return pc.rom_sequence(wait(3) + op(48), 0)
        same = {(106, 0): {'steps': [(0, 3)], 'loop_start': pc.HOLD_LAST, 'frame_count': 3}}
        longer = {(106, 0): {'steps': [(0, 4)], 'loop_start': pc.HOLD_LAST, 'frame_count': 3}}
        self.assertEqual(pc.compare_construction(Inherited(), stage, same)['verdict'], 'not compared')
        self.assertEqual(pc.compare_construction(Inherited(), stage, longer)['verdict'], 'differs')

        class Removed:
            def sequence(self, image, animation):
                return pc.rom_sequence(wait(3) + op(22), 0)
        self.assertEqual(pc.compare_construction(Removed(), stage, same)['verdict'], 'differs')

    def test_unknown_resource_type_disabled_rows_and_start_locations(self):
        always = [{'name': 'Always', 'comparison': 0, 'amount': 0, 'player_group': 0, 'unit_type': 0, 'flags': 0}]
        def trigger(actions, conditions=always):
            return {'record': 0, 'conditions': conditions, 'executed_for_player_values': [0, 1] + [0] * 25,
                    'actions': actions}
        def grant(kind, amount, flags=0):
            return {'name': 'SetResources', 'group1': 13, 'group2': amount, 'unit_type': kind, 'modifier': 7,
                    'flags': flags}
        owners = bytes([6, 6, 5, 0, 0, 0, 0, 0])
        rows = pc.start_banks([trigger([grant(3, 9), grant(0, 40, flags=pc.DISABLED)])], bytes(8), [], owners)
        self.assertEqual([r['resource'] for r in rows], ['resource 3'])  # the disabled action is skipped
        census = {'step': 0, 'last_step': 9, 'bank_history': {0: [{'step': 0, 'minerals': 0, 'gas': 0}]},
                  'banks': {}}
        self.assertTrue(pc.compare_banks(rows, census, {1: 0})[0]['verdict'].startswith('not compared'))
        # A disabled condition does not have to hold.
        never = dict(always[0], name='Never', flags=pc.DISABLED)
        self.assertEqual(len(pc.start_banks([trigger([grant(0, 40)], always + [never])], bytes(8), [], owners)), 1)
        # A start location is not a unit a Command condition counts.
        men = {'name': 'Command', 'comparison': 0, 'amount': 1, 'player_group': 13, 'unit_type': pc.MEN, 'flags': 0}
        starts = [{'type': pc.START_LOCATION, 'owner': 1, 'x': 0, 'y': 0}]
        self.assertEqual(pc.start_banks([trigger([grant(0, 40)], [men])], bytes(8), starts, owners), [])

    def test_blood_bath_without_town_hall_calibration_is_not_compared(self):
        starts = [dict(chk(214, i, 100 + 1000 * i, 100), type=214) for i in range(4)]
        census = gba(buildings=[(-1, 0, 50, 50), (-1, 1, 550, 50)])
        self.assertEqual(pc.skirmish_census(starts, census, {})['verdict'], 'not compared')
        # A missing mineral patch is a difference whatever the calibration.
        mineral = [chk(176, 11, 300, 300, 1500)]
        self.assertEqual(pc.skirmish_census(starts + mineral, census, {})['verdict'], 'differs')

    def test_collision_pairing_does_not_depend_on_chk_row_order(self):
        rows = [chk(37, 4, 200, 200), chk(37, 4, 204, 200)]
        census = gba(units=[(37, 4, 101, 100), (37, 4, 36, 100)])
        forward = pc.compare_census(rows, census)
        backward = pc.compare_census(list(reversed(rows)), census)
        self.assertEqual((forward['verdict'], forward['counts']), (backward['verdict'], backward['counts']))
        # 100 can pair with 101 or 36, 102 only with 101: both are collisions.
        both = pc.compare_census([chk(37, 4, 200, 200), chk(37, 4, 204, 200)],
                                 gba(units=[(37, 4, 101, 100), (37, 4, 60, 100)]))
        self.assertEqual((both['counts']['collision'], both['counts']['moved']), (2, 0))
        # Identical placements and actors far apart: two moves, nothing missing or extra.
        far = pc.compare_census([chk(0, 1, 10, 10), chk(0, 1, 10, 10)], gba(units=[(0, 1, 500, 5), (0, 1, 500, 5)]))
        self.assertEqual((far['counts']['moved'], far['counts']['missing'], far['counts']['extra']), (2, 0, 0))

    def test_an_untrusted_census_binary_is_not_run(self):
        with tempfile.TemporaryDirectory() as temp:
            binary = Path(temp) / 'terran-fast-trace'
            binary.write_text('#!/bin/sh\nexit 0\n')
            self.assertIn('error', pc.binary_record(binary, Path(temp), Path(temp)))  # no build record
            source = Path(temp) / 'game.c'
            source.write_text('int x;\n')
            record = {'binary_sha256': pc.file_sha256(binary),
                      'inputs': {'generated': {}, 'sources': {'game.c': pc.file_sha256(source)}},
                      'compile_command': 'touch should-not-run'}
            Path(str(binary) + '.inputs').write_text(json.dumps(record))
            work = Path(temp) / 'work'
            work.mkdir()
            good = pc.binary_record(binary, Path(temp), work)
            self.assertEqual(good['binary_sha256'], record['binary_sha256'])
            self.assertFalse((Path(temp) / 'should-not-run').exists())  # the command is data
            source.write_text('int y;\n')
            self.assertIn('inputs changed', pc.binary_record(binary, Path(temp), work)['error'])


class SoundTest(unittest.TestCase):
    HEADER = ('enum { SC64_AUDIO_READY, SC64_AUDIO_WHAT, SC64_AUDIO_YES, SC64_AUDIO_ATTACK, '
              'SC64_AUDIO_DEATH, SC64_AUDIO_BUILT };\n'
              'static const u16 sc64_audio_unit_first[6] __attribute__((unused)) = {\n0, 1, 1, 3, 3, 4\n};\n'
              'static const u8 sc64_audio_unit_count[6] __attribute__((unused)) = {\n1, 0, 2, 0, 1, 0\n};\n'
              'static const u16 sc64_audio_unit_sound[4] __attribute__((unused)) = {\n0, 1, 2, 0\n};\n')

    def id_map(self, **override):
        bank_index = {20: 0, 21: 1, 22: 2, 23: 0}
        sets = [{'ready': [20], 'what': [], 'yes': [21, 22], 'attack': [], 'death': [23], 'built': []}]
        return {**pack.sound_id_map(bank_index, 3, sets, 'x', 'y'), **override}

    def test_pack_side_file_lists_every_id_per_bank_index(self):
        ids = self.id_map()
        self.assertEqual(ids['bank'], [[20, 23], [21], [22]])
        self.assertEqual(ids['kinds'], list(pack.KINDS))
        self.assertEqual(ids['units'][0]['yes'], [21, 22])

    def test_engine_sounds_follow_the_header_indices(self):
        sounds = pc.engine_unit_sounds(self.HEADER, self.id_map())
        self.assertEqual(sounds[0], {'ready': [20], 'what': [], 'yes': [21, 22], 'attack': [],
                                     'death': [23], 'built': []})
        stale = self.id_map(units=[{'ready': [21], 'what': [], 'yes': [21, 22], 'attack': [],
                                    'death': [23], 'built': []}])
        with self.assertRaises(ValueError):
            pc.engine_unit_sounds(self.HEADER, stale)

    def test_sound_verdicts(self):
        audible = {1, 2, 3}
        self.assertEqual(pc.sound_verdict([1, 2], audible, [1, 2]), 'match')
        self.assertEqual(pc.sound_verdict([1, 2, 3], audible, [1]), 'partial')
        self.assertEqual(pc.sound_verdict([1], audible, []), 'absent')
        self.assertEqual(pc.sound_verdict([1], audible, [1, 9]), 'extra')
        self.assertEqual(pc.sound_verdict([], audible, []), 'none')
        self.assertEqual(pc.sound_verdict([7], audible, []), 'silent in ROM')

    def test_pack_side_file_replaces_links_instead_of_writing_through(self):
        with tempfile.TemporaryDirectory() as temp:
            tracked = Path(temp) / 'tracked.txt'
            tracked.write_text('keep\n')
            linked = Path(temp) / 'linked'
            linked.mkdir()
            (linked / 'link.json').symlink_to(tracked)
            with self.assertRaises(ValueError):
                pack.publish({linked / 'new.json': b'x\n', linked / 'link.json': b'{}\n'})
            self.assertFalse((linked / 'new.json').exists())  # nothing is published when one is refused
            bank = Path(temp) / 'bank'
            bank.mkdir()
            (bank / 'hard.json').hardlink_to(tracked)
            (bank / 'hard.json').chmod(0o600)
            pack.publish({bank / 'hard.json': b'{}\n'})
            self.assertEqual((bank / 'hard.json').stat().st_mode & 0o777, 0o600)
            self.assertEqual(tracked.read_text(), 'keep\n')
            self.assertEqual((bank / 'hard.json').read_text(), '{}\n')

    def test_pack_publishes_one_whole_generation(self):
        with tempfile.TemporaryDirectory() as temp:
            bank = Path(temp) / 'generated'
            bank.mkdir()
            (bank / 'keep.txt').write_text('other\n')
            (bank / 'a.bin').write_bytes(b'old')
            (bank / 'a.bin').chmod(0o600)
            pack.publish({bank / 'a.bin': b'new', bank / 'a.h': b'#define X 1\n'})
            self.assertEqual(((bank / 'a.bin').read_bytes(), (bank / 'a.h').read_bytes(),
                              (bank / 'keep.txt').read_text()), (b'new', b'#define X 1\n', 'other\n'))
            self.assertEqual((bank / 'a.bin').stat().st_mode & 0o777, 0o600)
            self.assertEqual(sorted(x.name for x in bank.iterdir()),
                             sorted(['a.bin', 'a.h', 'keep.txt', pack.LOCK_FILE]))  # no staging left
            with self.assertRaises(ValueError):
                pack.publish({bank / 'a.bin': b'x', Path(temp) / 'b.bin': b'y'})

    def test_an_interrupted_publish_leaves_the_old_commit_record(self):
        from unittest import mock
        with tempfile.TemporaryDirectory() as temp:
            bank = Path(temp) / 'generated'
            bank.mkdir()
            for name, data in (('a.bin', b'old'), ('ids.json', b'old ids')):
                (bank / name).write_bytes(data)
            real = os.replace
            def interrupt(src, dst, **dir_fds):
                result = real(src, dst, **dir_fds)
                if Path(dst).name == 'a.bin':
                    raise KeyboardInterrupt  # after the blob, before the id map
                return result
            with mock.patch('os.replace', interrupt), self.assertRaises(KeyboardInterrupt):
                pack.publish({bank / 'a.bin': b'new', bank / 'ids.json': b'new ids'})
            # The commit record still describes the old generation, so a reader
            # comparing its digests with the new blob refuses the bank.
            self.assertEqual(((bank / 'a.bin').read_bytes(), (bank / 'ids.json').read_bytes()), (b'new', b'old ids'))
            self.assertEqual([x.name for x in bank.iterdir() if x.name.startswith('.') and 'lock' not in x.name], [])

    def test_pack_publishes_into_the_current_folder(self):
        with tempfile.TemporaryDirectory() as temp:
            bank = Path(temp) / 'generated'
            bank.mkdir()
            here = os.getcwd()
            try:
                os.chdir(bank)
                pack.publish({Path('.') / 'a.bin': b'new'})
            finally:
                os.chdir(here)
            self.assertEqual((bank / 'a.bin').read_bytes(), b'new')

    def test_pack_refuses_a_bank_folder_that_is_a_link(self):
        with tempfile.TemporaryDirectory() as temp:
            real = Path(temp) / 'real'
            real.mkdir()
            (Path(temp) / 'generated').symlink_to(real)
            with self.assertRaises(OSError):  # opened without following the link
                pack.publish({Path(temp) / 'generated' / 'a.bin': b'new'})
            self.assertEqual(list(real.iterdir()), [])

    def test_pack_refuses_a_lock_that_is_not_a_regular_file(self):
        with tempfile.TemporaryDirectory() as temp:
            bank = Path(temp) / 'generated'
            bank.mkdir()
            os.mkfifo(bank / pack.LOCK_FILE)
            with self.assertRaises((ValueError, OSError)):  # refused at once, never blocked
                pack.publish({bank / 'a.bin': b'new'})
            self.assertFalse((bank / 'a.bin').exists())

    def test_pack_refuses_a_tracked_output_directory(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            subprocess.run(['git', 'init', '-q', str(repo)], check=True)
            (repo / '.gitignore').write_text('/private/\n/half/sc64_audio_sound_ids.json\n')
            (repo / 'private').mkdir()
            pack.refuse_tracked(repo / 'private' / 'bank')
            (repo / '.gitignore').write_text('/private/\n/half/sc64_audio_sound_ids.json\n/scratch*/\n')
            pack.refuse_tracked(repo, ('scratch-gen/', 'scratch-gen/x'))  # directory-only rule
            with self.assertRaises(ValueError):  # every bank output must be ignored, not one
                pack.refuse_tracked(repo / 'half')
            with self.assertRaises(ValueError):
                pack.refuse_tracked(repo / 'src' / 'bank')
        with tempfile.TemporaryDirectory() as temp:
            pack.refuse_tracked(Path(temp) / 'bank')  # outside any work tree
        from unittest import mock
        failed = subprocess.CompletedProcess([], 128, '', 'fatal: detected dubious ownership')
        with tempfile.TemporaryDirectory() as temp, mock.patch('subprocess.run', return_value=failed):
            with self.assertRaises(ValueError):  # an unverifiable destination is refused
                pack.refuse_tracked(Path(temp) / 'bank')
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            subprocess.run(['git', 'init', '-q', str(repo)], check=True)
            with mock.patch.dict(os.environ, {'GIT_DIR': str(repo / 'no-such-git-dir')}):
                with self.assertRaises(ValueError):  # an inherited GIT_DIR cannot make it look outside
                    pack.refuse_tracked(repo / 'src' / 'bank')


class SkipTest(unittest.TestCase):
    def test_missing_inputs_skip_cleanly(self):
        with tempfile.TemporaryDirectory() as temp:
            missing = str(Path(temp) / 'absent.z64')
            self.assertEqual(pc.main(['--rom', missing, '--extractor', missing]), 0)
            self.assertEqual(pc.main(['--rom', missing, '--extractor', missing, '--require-inputs']), 2)


class RomContentTest(unittest.TestCase):
    """Runs only where the private ROM and extractor are prepared."""

    def test_report_covers_every_mission_and_drawn_unit(self):
        try:
            pt.find_input(ROOT, '.local/sc64/rom/StarCraft64-USA.z64', None)
            pt.find_input(ROOT, 'references/sc64-maps/extract_sc64_maps.py', None)
        except pt.InputsMissing as missing:
            self.skipTest(f'private ROM inputs absent: {missing}')
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / 'content'
            run = subprocess.run(['python3', 'tools/sc64_parity_content.py', '--out', str(out)],
                                 cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            document = json.loads((out / 'l0-content.json').read_text())
            text = (out / 'l0-content.txt').read_text()
        self.assertEqual(document['provenance'], 'rom-table')
        missions = document['census']['missions']
        self.assertEqual([m['label'] for m in missions][:5], ['Z1', 'Z2', 'P1', 'P2', 'T1'])
        self.assertEqual(len(missions), 31)
        # Without census binaries every census is reported absent, never skipped.
        self.assertTrue(all(m['verdict'] == 'absent' and m['rom_placements'] > 0 for m in missions))
        for section in ('== opening censuses', '== iscript sequences', '== sounds', '== summary'):
            self.assertIn(section, text)
        units = {u['raw']: u for u in document['iscript']['units']}
        walks = [r for r in units.get(0, {}).get('rows', []) if r['animation'] == 'Walking']
        compared = [r for r in walks if r['verdict'] != 'absent']
        if compared:  # Marine: Walking is a nine-frame, one-tick-per-frame loop in the ROM
            self.assertEqual(compared[0].get('rom_waits'), [1] * 9)
        elif walks:  # the walk bank is not prepared here: the row reads absent, never skipped
            self.assertIn('walk bank', walks[0]['note'])


class RomCensusTest(unittest.TestCase):
    """Runs the real census through each prepared profile binary (Terran, Zerg
    Z10, Protoss P10), only where the private inputs and the binaries exist."""

    def test_every_profile_runs_its_censuses(self):
        try:
            pt.find_input(ROOT, '.local/sc64/rom/StarCraft64-USA.z64', None)
            pt.find_input(ROOT, 'references/sc64-maps/extract_sc64_maps.py', None)
        except pt.InputsMissing as missing:
            self.skipTest(f'private ROM inputs absent: {missing}')
        binaries = ROOT / 'build/parity'
        profiles = ('terran', 'zerg-z10', 'protoss-p10')
        if not all((binaries / f'{p}-fast-trace').is_file() for p in profiles):
            self.skipTest('census binaries not built (make parity-content builds them)')
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / 'content'
            run = subprocess.run(['python3', 'tools/sc64_parity_content.py', '--binaries', str(binaries),
                                  '--out', str(out)], cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            missions = json.loads((out / 'l0-content.json').read_text())['census']['missions']
        for profile in profiles:
            ran = [m for m in missions if m['profile'] == profile and m['verdict'] != 'absent']
            self.assertTrue(ran, f'no {profile} census ran: '
                            f"{[m.get('note') for m in missions if m['profile'] == profile][:2]}")
            self.assertTrue(all(m.get('gba_binary_sha256') for m in ran), profile)
        self.assertEqual(len(missions), 31)


if __name__ == '__main__':
    unittest.main()
