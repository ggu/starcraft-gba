# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of the N64 field map: extra.bin decoding, heap blocks, fog
masks, image records, the launch census and bank joins, the table anchor and the
t1-fields measures, on synthetic data only (no ROM, capture or GBA build)."""
import pathlib
import struct
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
import parity_decode  # noqa: E402
import sc64_parity  # noqa: E402
import sc64_parity_fieldmap as fm  # noqa: E402
import sc64_parity_metrics as metrics  # noqa: E402


def heap(size, body):
    return struct.pack('>IIII', size, 0xBABEFACE, 0xBABEFACE, 0xBABEFACE) + body


class ExtraFile(unittest.TestCase):
    def test_round_trip_and_truncation(self):
        regions = [(0x80100000, 4), (0x80200000, 2)]
        data = b'SC64XTR\0' + struct.pack('<2I', 1, 2)
        data += b''.join(struct.pack('<2I', a, n) for a, n in regions)
        data += struct.pack('<3I', 10, 5, 7) + b'abcd' + b'ef'
        with tempfile.TemporaryDirectory() as d:
            path = pathlib.Path(d) / 'extra.bin'
            path.write_bytes(data)
            got_regions, records = parity_decode.read_extra(path)
            self.assertEqual(got_regions, regions)
            self.assertEqual(records, [(10, 5, 7, [b'abcd', b'ef'])])
            path.write_bytes(data[:-1])
            with self.assertRaises(ValueError):
                parity_decode.read_extra(path)
            path.write_bytes(b'SC64TAP\0' + data[8:])
            with self.assertRaises(ValueError):
                parity_decode.read_extra(path)


class HeapAndFog(unittest.TestCase):
    def test_blocks_and_fog_block_choice(self):
        width, height = 4, 2
        good = bytearray(b'\x00\x01\xff\xff' * 8)
        good[4 * 5 + 2] = 0xFD   # player 1 explored tile 5
        good[4 * 5 + 3] = 0xFD   # and sees it
        good[4 * 6 + 2] = 0xFD   # explored tile 6 only
        bad = bytearray(good)
        bad[4 * 7 + 3] = 0xFD    # visible but not explored: not a fog block
        ram = b'\0' * 16 + heap(32, bytes(bad)) + heap(32, bytes(good)) + heap(8, b'\1' * 8)
        blocks = fm.heap_blocks(ram)
        self.assertEqual([n for _, n in blocks], [32, 32, 8])
        self.assertEqual(fm.find_fog_block(ram, width, height), [blocks[1][0]])
        visible, explored = fm.fog_masks(bytes(good), width, height, 1)
        self.assertEqual((visible, explored), ({5}, {5, 6}))
        self.assertEqual(fm.fog_consistency(bytes(bad), width, height)['visible_not_explored'], 1)

    def test_gba_fog_replay(self):
        keyframe = (0, {1, 2}, {1, 2, 3})
        changes = [(3, 4, 1, 1), (5, 1, 0, 1), (9, 2, 0, 1)]
        self.assertEqual(fm.fog_at(keyframe, changes, 4), ({1, 2, 4}, {1, 2, 3, 4}))
        self.assertEqual(fm.fog_at(keyframe, changes, 5), ({2, 4}, {1, 2, 3, 4}))
        frames = {sf: fm.fog_at(keyframe, changes, sf) for sf in range(12)}
        m = fm.fog_measure(frames.get, 0, 12, 4, 11)
        self.assertEqual(m, {'first_reveal': 3, 'first_clear': 5, 'visible_at_rest': 1, 'explored_at_rest': 4})


class Images(unittest.TestCase):
    def test_unit_images_follow_the_sprite(self):
        base = 0x80001000
        ram = bytearray(0x3000)
        sprite, image, shadow = base, base + 0x100, base + 0x200
        row = bytearray(312)
        struct.pack_into('>I', row, 0x0C, sprite)
        struct.pack_into('>I', ram, sprite - fm.RAM_BASE + 0x18, image)
        struct.pack_into('>I', ram, image - fm.RAM_BASE + 0x04, shadow)
        struct.pack_into('>HBB', ram, image - fm.RAM_BASE + 0x08, 239, 0x1C, 0)
        struct.pack_into('>HH', ram, image - fm.RAM_BASE + 0x14, 0x70C8, 0x71BA)
        struct.pack_into('>H', ram, shadow - fm.RAM_BASE + 0x08, 240)
        images = fm.unit_images(bytes(ram), bytes(row))
        self.assertEqual([i['image_id'] for i in images], [239, 240])
        self.assertEqual((images[0]['header'], images[0]['offset']), (0x70C8, 0x71BA))

    def test_walk_period_and_order_spans(self):
        cycle = [0x71BA + 7 * k for k in range(9)]
        points = [(sf, cycle[sf % 9], True) for sf in range(40)] + [(40, 0x7103, False)]
        self.assertEqual(fm.walk_period(points), 9)
        self.assertIsNone(fm.walk_period(points[:3]))
        history = [(0, 3), (5, 6), (90, 2), (91, 3), (120, 6)]
        self.assertEqual(fm.order_times(history, 2, 200, {2, 3}, {6}), (5, 90))
        self.assertIsNone(fm.order_times(history, 2, 80, {2, 3}, {6}))


class Census(unittest.TestCase):
    def test_joins_moved_turrets_larvae_and_amounts(self):
        chk = [{'type': 7, 'owner': 5, 'x': 100, 'y': 100, 'resources': 0, 'section': 'UNIT'},
               {'type': 3, 'owner': 1, 'x': 200, 'y': 200, 'resources': 0, 'section': 'UNIT'},
               {'type': 131, 'owner': 3, 'x': 300, 'y': 300, 'resources': 0, 'section': 'UNIT'},
               {'type': 176, 'owner': 11, 'x': 400, 'y': 400, 'resources': 1500, 'section': 'UNIT'},
               {'type': 214, 'owner': 1, 'x': 0, 'y': 0, 'resources': 0, 'section': 'UNIT'},
               {'type': 200, 'owner': 1, 'x': 9, 'y': 9, 'resources': 0, 'section': 'THG2', 'disabled': True}]
        n64 = [{'type': 7, 'owner': 5, 'x': 101, 'y': 99, 'resource_amount': 0},
               {'type': 3, 'owner': 1, 'x': 200, 'y': 200, 'resource_amount': 0},
               {'type': 4, 'owner': 1, 'x': 200, 'y': 200, 'resource_amount': 0},
               {'type': 131, 'owner': 3, 'x': 300, 'y': 300, 'resource_amount': 0},
               {'type': 35, 'owner': 3, 'x': 280, 'y': 330, 'resource_amount': 0},
               {'type': 176, 'owner': 11, 'x': 400, 'y': 400, 'resource_amount': 1500},
               {'type': 200, 'owner': 1, 'x': 9, 'y': 9, 'resource_amount': 0}]
        r = fm.census_compare(chk, n64, {3: 4})
        self.assertEqual(r['verdict'], 'exact')
        self.assertEqual((r['matched'], len(r['moved']), r['turrets'], r['larvae']), (5, 1, {4: 1}, 1))
        self.assertEqual(r['doodads'], {'rows': 1, 'owners': [(1, 1, True)]})
        n64[5]['resource_amount'] = 1400
        n64.append({'type': 37, 'owner': 4, 'x': 1, 'y': 1, 'resource_amount': 0})
        r = fm.census_compare(chk, n64, {3: 4})
        self.assertEqual(r['verdict'], 'differs')
        self.assertEqual(len(r['extra']), 1)
        self.assertEqual(r['amount_differs'][0]['n64'], 1400)

    def test_banks_match_within_the_window(self):
        rows = [{'trigger': 1, 'players': [3], 'resource': 'minerals+gas', 'mode': 'set', 'amount': 200, 'when': 'x'}]
        twelve = lambda k, v: tuple(v if i == k else 0 for i in range(12))
        series = [(0, twelve(3, 0), twelve(3, 0)), (2, twelve(3, 200), twelve(3, 0)), (40, twelve(3, 200), twelve(3, 200))]
        out = fm.bank_compare(rows, series)
        self.assertEqual([(o['resource'], o['verdict'], o['source_frame']) for o in out],
                         [('minerals', 'match', 2), ('gas', 'no match', None)])


class CensusClasses(unittest.TestCase):
    def test_hangar_children_and_displacement_need_evidence(self):
        unit = lambda t, o, x, y: {'type': t, 'owner': o, 'x': x, 'y': y, 'resources': 0, 'section': 'UNIT'}
        row = lambda t, o, x, y: {'type': t, 'owner': o, 'x': x, 'y': y, 'resource_amount': 0}
        chk = [unit(72, 1, 100, 100), unit(83, 1, 300, 300), unit(8, 1, 600, 600),     # isolated Wraith
               unit(0, 1, 900, 900), unit(0, 1, 910, 900)]                            # stacked Marines
        n64 = [row(72, 1, 100, 100), row(73, 1, 100, 100), row(83, 1, 300, 300), row(85, 1, 300, 300),
               row(8, 1, 640, 600), row(0, 1, 900, 900), row(0, 1, 950, 900)]
        r = fm.census_compare(chk, n64, {})
        self.assertEqual(r['hangar'], 2)
        self.assertEqual([d['type'] for d in r['displaced']], [0])
        self.assertEqual(([m['type'] for m in r['missing']], [e['type'] for e in r['extra']]), ([8], [8]))

    def test_owner_changes_pair_one_to_one(self):
        chk = [{'type': 5, 'owner': 2, 'x': x, 'y': 10, 'resources': 0, 'section': 'UNIT'} for x in (100, 108)]
        for order in ((94, 101), (101, 94)):
            n64 = [{'type': 5, 'owner': 1, 'x': x, 'y': 10, 'resource_amount': 0} for x in order]
            r = fm.census_compare(chk, n64, {})
            self.assertEqual((r['verdict'], len(r['owner_changed'])), ('exact', 2))

    def test_exact_positions_pair_first(self):
        chk = [{'type': 176, 'owner': 11, 'x': x, 'y': 5, 'resources': a, 'section': 'UNIT'} for x, a in ((100, 900), (103, 1200))]
        live = [{'type': 176, 'owner': 3, 'x': x, 'y': 5, 'resource_amount': a} for x, a in ((100, 900), (107, 1200))]
        for rows in (live, live[::-1]):
            for placed in (chk, chk[::-1]):
                r = fm.census_compare(placed, rows, {})
                self.assertEqual((r['verdict'], len(r['owner_changed'])), ('exact', 2))

    def test_pair_count_beats_exactness_and_amounts_break_ties(self):
        unit = lambda x, a=0: {'type': 5, 'owner': 2, 'x': x, 'y': 10, 'resources': a, 'section': 'UNIT'}
        row = lambda x, a=0: {'type': 5, 'owner': 1, 'x': x, 'y': 10, 'resource_amount': a}
        for live in ([row(100), row(92)], [row(92), row(100)]):
            r = fm.census_compare([unit(100), unit(108)], live, {})
            self.assertEqual((r['verdict'], len(r['owner_changed']), r['missing'], r['extra']), ('exact', 2, [], []))
            self.assertEqual([o['x'] for o in r['owner_changed']], [100, 108])   # CHK order
        fields = [dict(unit(50, a), type=176, owner=11) for a in (900, 1200)]
        for live in ([1200, 900], [900, 1200]):
            r = fm.census_compare(fields, [dict(row(50, a), type=176, owner=3) for a in live], {})
            self.assertEqual((r['verdict'], r['amount_differs']), ('exact', []))

    def test_isolated_rows_do_not_enlarge_the_search(self):
        fields = [{'type': 176, 'owner': 11, 'x': x, 'y': 50, 'resources': a, 'section': 'UNIT'}
                  for x, a in ((50, 900), (50, 1200))] + \
                 [{'type': 176, 'owner': 11, 'x': 1000 + 100 * k, 'y': 50, 'resources': 7, 'section': 'UNIT'} for k in range(5)]
        for amounts in ((1200, 900), (900, 1200)):
            live = [{'type': 176, 'owner': 3, 'x': 50, 'y': 50, 'resource_amount': a} for a in amounts] + \
                   [{'type': 176, 'owner': 3, 'x': 1000 + 100 * k, 'y': 50, 'resource_amount': 7} for k in range(5)]
            r = fm.census_compare(fields, live, {})
            self.assertEqual((r['verdict'], len(r['owner_changed']), r['amount_differs']), ('exact', 7, []))

    def test_non_resources_ignore_raw_amounts(self):
        chk = [{'type': 7, 'owner': 2, 'x': 100, 'y': 10, 'resources': 0, 'section': 'UNIT'}]
        live = [{'type': 7, 'owner': 1, 'x': 107, 'y': 10, 'resource_amount': 0},
                {'type': 7, 'owner': 1, 'x': 101, 'y': 10, 'resource_amount': 33}]
        r = fm.census_compare(chk, live, {})
        self.assertEqual([e['x'] for e in r['extra']], [107])

    def test_moved_and_displaced_are_one_matching(self):
        unit = lambda x: {'type': 0, 'owner': 1, 'x': x, 'y': 10, 'resources': 0, 'section': 'UNIT'}
        row = lambda x: {'type': 0, 'owner': 1, 'x': x, 'y': 10, 'resource_amount': 0}
        r = fm.census_compare([unit(100), unit(112)], [row(107), row(168)], {})
        self.assertEqual((r['verdict'], len(r['moved']), len(r['displaced'])), ('exact', 1, 1))

    def test_distance_ranks_before_amounts(self):
        unit = lambda x, a: {'type': 176, 'owner': 11, 'x': x, 'y': 10, 'resources': a, 'section': 'UNIT'}
        row = lambda x, a: {'type': 176, 'owner': 11, 'x': x, 'y': 10, 'resource_amount': a}
        r = fm.census_compare([unit(100, 900), unit(108, 1200)], [row(101, 1200), row(107, 900)], {})
        self.assertEqual(sorted((d['chk'], d['n64']) for d in r['amount_differs']), [(900, 1200), (1200, 900)])

    def test_assignment_is_optimal(self):
        import itertools
        import random
        rng = random.Random(7)
        for _ in range(200):
            rows, cols = rng.randint(1, 5), rng.randint(1, 5)
            w = [[rng.choice([0, 0, rng.randint(1, 50)]) for _ in range(cols)] for _ in range(rows)]
            got = sum(w[i][j] for i, j in fm.assignment(w))
            best = 0
            for perm in itertools.permutations(range(max(rows, cols)), rows):
                best = max(best, sum(w[i][j] for i, j in enumerate(perm) if j < cols))
            self.assertEqual(got, best)

    def test_rectangular_groups_stay_small(self):
        w = [[5] * 900]
        self.assertEqual(fm.assignment(w), [(0, 0)])
        self.assertEqual(fm.assignment([[0, 3], [4, 0], [0, 0]]), [(0, 1), (1, 0)])

    def test_large_groups_keep_the_priorities(self):
        unit = lambda x, a: {'type': 176, 'owner': 11, 'x': x, 'y': 10, 'resources': a, 'section': 'UNIT'}
        row = lambda x, a: {'type': 176, 'owner': 11, 'x': x, 'y': 10, 'resource_amount': a}
        chk = [unit(100, 900), unit(106, 1200)] + [unit(118 + 12 * k, 7) for k in range(5)]
        live = [row(101, 1200), row(114, 900)] + [row(124 + 12 * k, 7) for k in range(5)]
        for rows in (live, live[::-1]):
            r = fm.census_compare(chk, rows, {})
            self.assertEqual(len(r['amount_differs']), 2)   # nearest pairs: the swapped amounts show

    def test_reowned_resource_amounts(self):
        chk = [{'type': 176, 'owner': 11, 'x': 5, 'y': 5, 'resources': 900, 'section': 'UNIT'}]
        same = fm.census_compare(chk, [{'type': 176, 'owner': 3, 'x': 5, 'y': 5, 'resource_amount': 900}], {})
        self.assertEqual((len(same['owner_changed']), same['amount_differs']), (1, []))
        other = fm.census_compare(chk, [{'type': 176, 'owner': 3, 'x': 5, 'y': 5, 'resource_amount': 800}], {})
        self.assertEqual(other['amount_differs'][0]['n64'], 800)


class TableAnchor(unittest.TestCase):
    def test_hinted_base_confirmed_by_the_next_column(self):
        a = struct.pack('<2H', 1, 2)
        b = bytes([4, 5, 6, 7])
        columns = [('t.a', 'H', 2, a), ('t.b', 'B', 4, b)]
        decoy = struct.pack('>2H', 1, 2) + b'\0' * 4          # an unchanged copy of the first column elsewhere
        base = struct.pack('>2H', 1, 9) + bytes([4, 5, 6, 7])  # the real table: first column changed
        ram = decoy + b'\xEE' * 8 + base
        found = fm.anchor_columns(ram, columns, {'t.a': f'0x{fm.RAM_BASE + 16:08X}'})
        self.assertEqual([(f['status'], f['address']) for f in found],
                         [('differs', f'{fm.RAM_BASE + 16:08x}'), ('equal', f'{fm.RAM_BASE + 20:08x}')])

    def test_ambiguous_bases_are_reported(self):
        a, b = struct.pack('<2H', 1, 2), bytes([4, 5, 6, 7])
        columns = [('t.a', 'H', 2, a), ('t.b', 'B', 4, b)]
        copy = struct.pack('>2H', 1, 2) + b
        found = fm.anchor_columns(copy + copy, columns)
        self.assertTrue(found[0].get('ambiguous'))
        found = fm.anchor_columns(copy + struct.pack('>2H', 1, 2) + b'\0' * 4, columns)
        self.assertFalse(found[0].get('ambiguous'))
        self.assertEqual(found[0]['address'], f'{fm.RAM_BASE:08x}')

    def test_columns_found_aligned_and_differences_listed(self):
        a = struct.pack('<3H', 1, 2, 3)           # u16 column, 6 bytes
        b = bytes([4, 5, 6])                      # u8 column right after, 4-byte aligned in RAM
        c = bytes([7, 8, 9])
        columns = [('t.a', 'H', 3, a), ('t.b', 'B', 3, b), ('t.c', 'B', 3, c)]
        ram = b'\xAA' * 8 + struct.pack('>3H', 1, 2, 3) + b'\0\0' + b'\4\5\6\0' + b'\7\x10\x09'
        found = fm.anchor_columns(ram, columns)
        self.assertEqual([f['status'] for f in found], ['equal', 'equal', 'differs'])
        self.assertEqual(found[0]['address'], f'{fm.RAM_BASE + 8:08x}')
        self.assertEqual(found[2]['diffs'], [{'row': 1, 'rom': 8, 'ram': 16}])


class Scenario(unittest.TestCase):
    def test_registered_and_tape_marks_count_pulse(self):
        self.assertIn('t1-fields', metrics.SCENARIOS)
        with tempfile.TemporaryDirectory() as d:
            tape = pathlib.Path(d) / 'tape.txt'
            tape.write_text('10 0 0 0\npulse 800F0C20 2 ne 0 50 2 3 60\n# mark command\n4 64 0 0\nquit\n')
            (pathlib.Path(d) / 'commands.csv').write_text('begin,steps,mask,x,y\n0,10,0,0,0\n10,50,2,0,0\n60,4,64,0,0\n')
            self.assertEqual(metrics.tape_commands(tape), {'command': 60})

    def test_a_lethal_first_hit_keeps_its_victim(self):
        with tempfile.TemporaryDirectory() as d:
            trace = pathlib.Path(d) / 't.trace'
            u = lambda slot, owner: f'U {slot} 0 0 0 {owner} 10 10 40 40 0 -1 0 0 0 -1 0 0 0 0 0 0 0 0'
            trace.write_text('#sc64-parity-trace 1\nH max_units=240 x=1\nS 1\n' + u(3, 1) + '\n' + u(4, 4) + '\n'
                             'S 6\n-U 3 0 1 0\nD 3 40 0 4\nS 7\nD 3 40 0 4\nZ overflow=0 write_errors=0\n')
            t = fm.gba_timeline(trace, {1})
            self.assertEqual(t['first_hit'], {'sf': 4, 'type': 0, 'owner': 1})

    def test_gba_series_keeps_orders_walks_and_fog(self):
        with tempfile.TemporaryDirectory() as d:
            trace = pathlib.Path(d) / 't.trace'
            u = lambda step, order, walk, anim: ('U 3 0 0 0 1 10 10 40 40 0 -1 %d 0 0 -1 0 0 0 0 0 0 %d %d' % (order, walk, anim))
            trace.write_text('#sc64-parity-trace 1\nS 1\nFV 4 1 03\nFE 4 1 07\n' + u(1, 0, 0, 0) + '\n'
                             'S 6\n' + u(6, 1, 2, 1) + '\nF 3 1 1\nZ overflow=0 write_errors=0\n')
            s = sc64_parity.gba_series(trace)
            self.assertEqual(s['orders'][3], [(0, 0), (4, 1)])
            self.assertEqual(s['walks'][3], [(0, 0, 0), (4, 2, 1)])
            self.assertEqual(s['fog']['keyframe'], (0, {0, 1}, {0, 1, 2}))
            self.assertEqual(s['fog']['changes'], [(4, 3, 1, 1)])


class ExtraJoin(unittest.TestCase):
    """extra.bin records join the tick's record and chain only when they belong to it."""

    def capture(self, d, name, extra_records, status='dual_differ=0 ', record_extra=True, regions=((0x80100000, 2),)):
        import hashlib
        import json
        sys.path.insert(0, str(ROOT / 'tests'))
        import test_sc64_parity_harness as harness
        run = pathlib.Path(d) / name
        harness.write_capture(run, harness.marine_frames(ticks=6, start=2, settle=2), command_step=4)
        data = b'SC64XTR\0' + struct.pack('<2I', 1, len(regions))
        data += b''.join(struct.pack('<2I', a, n) for a, n in regions)
        for step, tick, payload in extra_records:
            data += struct.pack('<3I', step, tick, 0) + payload
        (run / 'extra.bin').write_bytes(data)
        (run / 'status.log').write_text('frontend=capture\ntap_reads=1 ' + status + 'x=1\n')
        manifest = json.loads((run / 'manifest.json').read_text())
        outputs = manifest['outputs_sha256']
        outputs['status.log'] = hashlib.sha256((run / 'status.log').read_bytes()).hexdigest()
        if record_extra:
            outputs['extra.bin'] = hashlib.sha256(data).hexdigest()
        (run / 'manifest.json').write_text(json.dumps(manifest))
        return run

    def test_join_and_rejections(self):
        with tempfile.TemporaryDirectory() as d:
            # Follow-up reads are at steps 3, 5, 7 ... for ticks 1, 2, 3 ... (write_capture).
            good = self.capture(d, 'good', [(3, 1, b'ab'), (5, 2, b'cd')])
            cap = parity_decode.Capture(good)
            self.assertEqual(cap.verified, [])
            self.assertEqual([r.get('extra') for r in cap.ticks[:3]], [[b'ab'], [b'cd'], None])
            self.assertEqual(cap.faults['extra'], 0)
            moved = parity_decode.Capture(self.capture(d, 'moved', [(3, 1, b'ab'), (5, 2, b'cd')],
                                                       regions=((0x80200000, 2),)))
            self.assertNotEqual(moved.chain, cap.chain)   # region metadata joins the chain
            wrong_tick = parity_decode.Capture(self.capture(d, 'tick', [(3, 2, b'ab')]))
            self.assertEqual(wrong_tick.faults['extra'], 2)   # not this read's, and joined to none
            orphan = parity_decode.Capture(self.capture(d, 'orphan', [(3, 1, b'ab'), (2, 1, b'zz')]))
            self.assertEqual(orphan.faults['extra'], 1)       # step 2 is a change read, not a follow-up
            self.assertNotIn('extra', wrong_tick.ticks[0])
            duplicate = parity_decode.Capture(self.capture(d, 'dup', [(3, 1, b'ab'), (3, 1, b'ab')]))
            self.assertEqual(duplicate.faults['extra'], 1)
            conflict = parity_decode.Capture(self.capture(d, 'conflict', [(3, 1, b'ab')], status='dual_differ=2 '))
            self.assertTrue(any('cache-alias' in p for p in conflict.verified))
            stray = self.capture(d, 'stray', [(3, 1, b'ab')], status='dual_differ=2 ')
            text = 'note dual_differ=0 \n' + (stray / 'status.log').read_text()
            (stray / 'status.log').write_text(text)
            import hashlib
            import json
            manifest = json.loads((stray / 'manifest.json').read_text())
            manifest['outputs_sha256']['status.log'] = hashlib.sha256(text.encode()).hexdigest()
            (stray / 'manifest.json').write_text(json.dumps(manifest))
            self.assertTrue(any('cache-alias' in p for p in parity_decode.Capture(stray).verified))
            unrecorded = parity_decode.Capture(self.capture(d, 'unrecorded', [(3, 1, b'ab')], record_extra=False))
            self.assertTrue(any('extra.bin does not match' in p for p in unrecorded.verified))
            missing = self.capture(d, 'missing', [(3, 1, b'ab')])
            (missing / 'extra.bin').unlink()
            self.assertTrue(any('recorded in the manifest but missing' in p
                                for p in parity_decode.Capture(missing).verified))

    def test_table_offset_joins_the_chain_and_size_is_capped(self):
        import os
        import struct as st
        with tempfile.TemporaryDirectory() as d:
            run = self.capture(d, 'a', [(3, 1, b'ab'), (5, 2, b'cd')])
            before = parity_decode.Capture(run).chain
            tap = bytearray((run / 'tap.bin').read_bytes())
            # The first follow-up record (after one change record of 48 + 0xCC bytes): its table offset.
            st.pack_into('<I', tap, 24 + 48 + 0xCC + 9 * 4, 0x300010)
            (run / 'tap.bin').write_bytes(bytes(tap))
            self.assertNotEqual(parity_decode.Capture(run).chain, before)
            os.environ['SC64_PARITY_MAX_CAPTURE_BYTES'] = '100'
            try:
                with self.assertRaises(ValueError):
                    parity_decode.Capture(run)
            finally:
                del os.environ['SC64_PARITY_MAX_CAPTURE_BYTES']

    def test_capture_wrapper_needs_the_budget(self):
        sys.path.insert(0, str(ROOT / 'experiments' / 'ares-headless'))
        import capture
        with tempfile.TemporaryDirectory() as d:
            old, new = pathlib.Path(d) / 'old', pathlib.Path(d) / 'new'
            old.write_bytes(b'\0frontend\0')
            new.write_bytes(b'\0SC64_CAPTURE_BUDGET_BYTES\0')
            self.assertEqual((capture.budget_supported(old), capture.budget_supported(new)), (False, True))
        self.assertTrue(capture.budget_acknowledged('pokes=0 output_written=5 output_budget=8589934592\n', 8589934592))
        self.assertFalse(capture.budget_acknowledged('pokes=0 output_written=5\n', 8589934592))

    def test_fog_needs_one_region_of_its_size(self):
        with tempfile.TemporaryDirectory() as d:
            two = parity_decode.Capture(self.capture(d, 'two', [(3, 1, b'\xff' * 8 + b'\xff' * 8)],
                                                     regions=((0x80100000, 8), (0x80200000, 8))))
            with self.assertRaises(parity_decode.LayoutError):
                two.fog(two.ticks[0], 1, 2, 1)
            one = parity_decode.Capture(self.capture(d, 'one', [(3, 1, b'\x00\x01\xfd\xfd' + b'\x00\x01\xfd\xff')],
                                                     regions=((0x80100000, 8),)))
            self.assertEqual(one.fog(one.ticks[0], 1, 2, 1), ({0}, {0, 1}))
            decoy = parity_decode.Capture(self.capture(d, 'decoy', [(3, 1, b'\x00\x01\xff\xfd' + b'\x00\x01\xff\xff')],
                                                       regions=((0x80100000, 8),)))
            with self.assertRaises(parity_decode.LayoutError):   # visible but unexplored: not fog
                decoy.fog(decoy.ticks[0], 1, 2, 1)


if __name__ == '__main__':
    unittest.main()
