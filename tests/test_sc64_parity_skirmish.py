# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of the skirmish parity measurements (tools/sc64_parity_skirmish.py):
the shared frame measurement, the GBA trace reader, seed envelopes and the
comparator's per-player values, on synthetic data only."""
import hashlib
import json
import pathlib
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tests'))
import sc64_parity  # noqa: E402
import sc64_parity_metrics as metrics  # noqa: E402
import sc64_parity_skirmish as sk  # noqa: E402

try:
    from compression import zstd as _zstd  # noqa: F401  (Python 3.14)
    HAVE_ZSTD = True
except ImportError:
    HAVE_ZSTD = False

HUMAN, CPU = 1, 2
HUMAN_HALL = (288, 1808)
CPU_HALL = (128, 240)


def frame(sf, units, gathered=None, bank=None, supply=None):
    return {'sf': sf, 'units': dict(enumerate(units)), 'gathered': gathered or {},
            'bank': bank or {HUMAN: 50, CPU: 50}, 'supply': supply or {}}


def match_frames(n=40, wave_at=30, cpu_bank_dip=5, eliminate_at=None):
    """A Zerg human (passive) and a Terran computer that builds a Barracks at 10,
    trains a Marine (in training 12-19, complete at 20) and sends three Marines
    at wave_at."""
    out = []
    for sf in range(n):
        units = [(131, HUMAN, *HUMAN_HALL, 1)] if eliminate_at is None or sf < eliminate_at else []
        units += [(41, HUMAN, 300 + k, 1800, 1) for k in range(4)]
        units += [(35, HUMAN, 290, 1830, 1)]                   # larva never counts as a new type
        units += [(106, CPU, *CPU_HALL, 1)] + [(7, CPU, 140 + k, 260, 1) for k in range(4 + (sf >= 8))]
        if sf >= 10:
            units.append((111, CPU, 200, 300, 1 if sf >= 30 else 0))
        if 12 <= sf < 20:
            units.append((0, CPU, 200, 330, 0))                # a Marine in training
        if sf >= 20:
            near = sf >= wave_at
            for k in range(3):
                x, y = (HUMAN_HALL[0] + 50 * k, HUMAN_HALL[1] - 100) if near else (400, 400)
                units.append((0, CPU, x, y, 1))
        bank = {HUMAN: 50 + 8 * (sf // 10), CPU: 50 - (8 if sf == cpu_bank_dip else 0)}
        gathered = {HUMAN: (50 + 8 * (sf // 10), 0), CPU: (50 + 8 * (sf // 5), 0)}
        supply = {HUMAN: (8, 18), CPU: (8 + 2 * (sf >= 8) + 2 * (sf >= 12) + 4 * (sf >= 20), 20)}
        out.append(frame(sf, units, gathered, bank, supply))
    return out


class Measure(unittest.TestCase):
    def test_players_firsts_wave_and_series(self):
        m = sk.measure(iter(match_frames()))
        self.assertEqual(m['human'], HUMAN)          # the only player whose bank never fell
        self.assertEqual(m['end'], 39)
        cpu = m['players']['cpu Terran']
        self.assertEqual(cpu['corner'], 0)
        self.assertEqual(m['players']['human']['corner'], 2)
        # Buildings count when placed, units when complete; larva and starting types never count.
        self.assertEqual(cpu['firsts'], {'Barracks': 10, 'Marine': 20})
        self.assertEqual(m['players']['human']['firsts'], {})
        self.assertEqual(cpu['wave'], 30)
        self.assertEqual(cpu['contact'], 30)
        self.assertEqual(cpu['series']['workers'][7:9], [4, 5])
        self.assertEqual(cpu['series']['gathered'][:6], [0, 0, 0, 0, 0, 8])
        self.assertIsNone(m['players']['human']['eliminated'])

    def test_elimination_never_and_ended(self):
        m = sk.measure(iter(match_frames(eliminate_at=35)))
        self.assertEqual(m['players']['human']['eliminated'], 35)
        late = sk.measure(iter(match_frames(wave_at=99)))
        v = sk.values(late, 39)
        self.assertEqual(v[('attack.first_wave', 'cpu Terran')], sk.NEVER)
        self.assertEqual(v[('result.eliminated', 'human')], sk.NEVER)
        self.assertNotIn(('attack.supply_at_wave', 'cpu Terran'), v)
        # A time past the horizon is NEVER too, and a checkpoint past a run's end reads ENDED.
        self.assertEqual(sk.values(m, 30)[('result.eliminated', 'human')], sk.NEVER)
        self.assertEqual(sk.at([1, 2, 3], 2), 3)
        self.assertEqual(sk.at([1, 2, 3], 3), sk.ENDED)

    def test_defeat_declaration_and_launch_relative_frames(self):
        frames = match_frames(eliminate_at=33)
        for f in frames:
            # The computer's byte is set first (as a computer losing in a free-for-all would): only
            # the human's own byte is its defeat.
            f['result'] = {HUMAN: 2 if f['sf'] >= 35 else 0, CPU: 2 if f['sf'] >= 20 else 0}
        m = sk.measure(iter(frames[1:]))            # the first record lost: frames stay launch-relative
        self.assertEqual(m['sampled'][0], 1)
        self.assertEqual(m['defeat'], 35)
        obs = {(o['check'], o['subject']): o for o in sk.observations(m, 39)}
        self.assertEqual(obs[('result.human_defeated', 'human')]['value'], 35)
        self.assertIn('result.word', obs[('result.human_defeated', 'human')]['layout_fields'])
        self.assertEqual(obs[('result.eliminated', 'human')]['value'], 33)
        self.assertFalse(obs[('result.eliminated', 'human')]['gating'])

    def test_only_a_defeat_byte_is_a_defeat(self):
        frames = match_frames()
        for f in frames:
            f['result'] = {HUMAN: 3 if f['sf'] >= 30 else 0}
        m = sk.measure(iter(frames))
        self.assertIsNone(m['defeat'])
        self.assertEqual(m['results']['human'], {'frame': 30, 'value': 3})

    def test_race_types_never_made_are_expected_never(self):
        m = sk.measure(iter(match_frames()))
        obs = {(o['check'], o['subject']): o for o in sk.observations(m, 39)}
        vulture = obs[('build.first.Vulture', 'cpu Terran')]
        self.assertEqual((vulture['value'], vulture['lo'], vulture['hi']), (sk.NEVER,) * 3)
        self.assertNotIn(('build.first.SCV', 'cpu Terran'), obs)              # in the starting roster
        self.assertNotIn(('build.first.Zealot', 'cpu Terran'), obs)           # another race
        self.assertEqual(obs[('build.first.Barracks', 'cpu Terran')]['sentinels'], [sk.NEVER, sk.ENDED])
        # The GBA side keeps the N64 window: a later frame is cut off.
        late = sk.measure(iter(match_frames()), horizon=15)
        self.assertEqual(late['end'], 15)

    def test_a_sieged_tank_is_a_tank(self):
        frames = match_frames()
        for f in frames[25:]:
            f['units'][800] = (30, CPU, 400, 400, 1)
        self.assertEqual(sk.measure(iter(frames))['players']['cpu Terran']['firsts']['Siege Tank'], 25)

    def test_units_without_weapons_are_no_wave(self):
        frames = match_frames(n=40, wave_at=99)
        for f in frames[30:]:
            f['units'].update({900 + k: ((34, 11, -1, 14)[k], CPU, HUMAN_HALL[0], HUMAN_HALL[1] - 50, 1)
                               for k in range(4)})
        self.assertIsNone(sk.measure(iter(frames))['players']['cpu Terran']['wave'])

    def test_human_given_and_ambiguity(self):
        frames = match_frames(cpu_bank_dip=-1)
        frames = [dict(f, units={k: u for k, u in f['units'].items() if u[0] not in (111, 0)}) for f in frames]
        with self.assertRaises(ValueError):
            sk.measure(iter(frames))                   # two passive players: no human to pick
        self.assertEqual(sk.measure(iter(frames), HUMAN)['human'], HUMAN)

    def test_missing_frames_repeat_the_last_value(self):
        frames = [f for f in match_frames() if f['sf'] not in (5, 6)]
        m = sk.measure(iter(frames))
        s = m['players']['cpu Terran']['series']
        self.assertEqual(len(s['gathered']), 40)
        self.assertEqual(s['gathered'][5], s['gathered'][4])
        # A checkpoint on a lost frame reads the next sampled frame, never a carried value.
        old, sk.CHECKPOINTS = sk.CHECKPOINTS, (5, 8)
        try:
            self.assertEqual(sk.checkpoint_frames(m, 39), {5: 7, 8: 8})
            obs = {(o['check'], o['subject']): o for o in sk.observations(m, 39)}
            self.assertEqual(obs[('workers@5', 'cpu Terran')]['frame'], 7)
            g = obs[('econ.gathered@5', 'human')]
            self.assertEqual(g['lo'], 0)                    # never below zero: ENDED (-1) always fails
            self.assertEqual(sk.values(m, 39, {5: 99})[('workers@5', 'human')], sk.ENDED)
        finally:
            sk.CHECKPOINTS = old

    def test_observation_bands(self):
        old, sk.RATE_MIN_FRAMES = sk.RATE_MIN_FRAMES, 10
        try:
            m = sk.measure(iter(match_frames()))
        finally:
            sk.RATE_MIN_FRAMES = old
        self.assertAlmostEqual(m['players']['human']['rates'][4]['rate'], 24 / 39)
        obs = {(o['check'], o['subject']): o for o in sk.observations(m, 39)}
        b = obs[('build.first.Barracks', 'cpu Terran')]
        self.assertEqual((b['lo'], b['hi'], b['gating']), (8, 12, True))
        self.assertIn('rng.seed', b['layout_fields'])
        self.assertFalse(obs[('econ.rate_k4', 'human')]['gating'])
        never = {(o['check'], o['subject']): o for o in sk.observations(sk.measure(iter(match_frames(wave_at=99))), 39)}
        w = never[('attack.first_wave', 'cpu Terran')]
        self.assertEqual((w['value'], w['lo'], w['hi']), (sk.NEVER,) * 3)
        self.assertEqual(obs[('start.corner', 'human')]['lo'], obs[('start.corner', 'human')]['hi'])
        self.assertFalse(obs[('attack.first_contact', 'cpu Terran')]['gating'])
        self.assertIn('unit.status', b['layout_fields'])


TRACE = """#sc64-parity-trace 1
H schema=sc64-parity-trace/1
S 1 0 1 1 3 0 0 0 -1 -1 0
U 0 0 -1 0 -1 144 904 40 40 0 -1 4 0 0 -1 0 0 -1 0 0 0 0 0
B 0 6 -1 0 -1 144 904 1250 1250 0 0 0 0 0 0 -1 -1 -1 -1 -1 3 0
B 1 0 -1 1 -1 64 120 1500 1500 0 0 0 0 1 10 0 -1 -1 -1 -1 3 0
U 1 0 -1 1 -1 70 130 40 40 0 -1 4 0 0 -1 0 0 -1 0 0 0 0 0
P 0 50 0 2 18
P 1 50 0 2 20
S 2 1 2 2 3 0 0 0 -1 -1 0
S 3 2 3 3 3 0 0 0 -1 -1 0
B 2 1 -1 1 -1 100 150 100 1000 0 1 0 0 0 0 -1 -1 -1 -1 -1 0 0
-U 0 0 0 0
S 4 3 4 4 3 0 1 0 -1 -1 0
Z steps=4 overflow=0 write_errors=0
"""
REPORT = {'steps': 4, 'match': {'owners': [{'owner': 0, 'human': 1, 'race': 1}, {'owner': 1, 'human': 0, 'race': 0}],
                    'unit_supply': [2, 2], 'start_order': 3},
          'unit_raw': [7, 0], 'building_raw': [106, 111, 109, 110, 112, 122, 131]}


class GbaTrace(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.trace = pathlib.Path(self.dir.name) / 'm.trace'
        self.trace.write_text(TRACE)
        pathlib.Path(str(self.trace) + '.gather').write_text('# header\n2 0 8 0\n4 1 16 0\n# end 4\n')

    def tearDown(self):
        self.dir.cleanup()

    def test_frames(self):
        frames = list(sk.gba_frames(self.trace, str(self.trace) + '.gather', REPORT))
        # Steps 1..4 are source frames 0, 1, 2, 3; the last row per frame wins.
        self.assertEqual([f['sf'] for f in frames], [0, 1, 2, 3])
        first = frames[0]
        self.assertEqual(first['units'][0], (7, 0, 288, 1808, 1))          # engine U_SCV -> cartridge 7, N64 px
        self.assertEqual(first['units'][100000], (131, 0, 288, 1808, 1))   # engine B_HATCHERY -> 131
        # The CPU Command Center trains engine type 0 (2 supply, progress 10): used 2 + 2.
        self.assertEqual(first['supply'][1], (4, 20))
        # A head stalled at the cap (progress 0) and the orders waiting behind it hold no supply.
        stalled = TRACE.replace('0 0 0 0 1 10 0 -1 -1 -1 -1 3 0', '0 0 0 0 3 0 0 0 0 -1 -1 3 0')
        self.trace.write_text(stalled)
        self.assertEqual(next(sk.gba_frames(self.trace, None, REPORT))['supply'][1], (2, 20))
        self.assertEqual(frames[1]['gathered'][0], (8, 0))
        self.assertEqual(frames[2]['units'][100002], (111, 1, 200, 300, 0))  # under construction
        self.assertNotIn(0, frames[3]['units'])
        self.assertEqual([f['result'] for f in frames], [{0: 0}, {0: 0}, {0: 0}, {0: sk.DEFEAT}])
        # A unit row at 0 HP no longer counts (the N64 keeps dying rows for a few frames).
        self.trace.write_text(TRACE.replace('U 1 0 -1 1 -1 70 130 40 40', 'U 1 0 -1 1 -1 70 130 0 40'))
        self.assertNotIn(1, next(sk.gba_frames(self.trace, None, REPORT))['units'])

    def test_a_gather_log_needs_its_footer(self):
        gather = pathlib.Path(str(self.trace) + '.gather')
        for text in ('# header\n2 0 8 0\n', '# header\n2 0 8 0\n# end 3\n', ''):
            gather.write_text(text)
            with self.assertRaises(ValueError):
                list(sk.gba_frames(self.trace, str(gather), REPORT))

    def test_incomplete_trace_is_refused(self):
        self.trace.write_text(TRACE.replace('Z steps=4 overflow=0', 'Z steps=4 overflow=1'))
        with self.assertRaises(ValueError):
            list(sk.gba_frames(self.trace, None, REPORT))
        # ... also when the frames past the N64 horizon are not measured.
        with self.assertRaises(ValueError):
            sk.measure(sk.gba_frames(self.trace, None, REPORT), 0, horizon=1)
        for bad in (TRACE + 'S 5 4 5 5 3 0 0 0 -1 -1 0\n', TRACE.replace('S 3 2 3 3 3 0 0 0 -1 -1 0', 'S 3 2 3'),
                    TRACE.replace('B 2 1 -1', 'Q 2 1 -1'),            # an unknown record kind
                    TRACE.replace('S 4 3 4 4 3 0 1 0 -1 -1 0\n', ''),  # a step missing before Z
                    TRACE.replace('S 3 2 3 3 3', 'S 5 2 3 3 3')):      # steps out of order
            self.trace.write_text(bad)                      # a record after Z; a truncated row
            with self.assertRaises(ValueError):
                list(sk.gba_frames(self.trace, None, REPORT))

    def test_a_damaged_trace_gives_no_verdict(self):
        # The footer lacks the run's hashes and counts: the self-gate's completeness check refuses it.
        tape = hashlib.sha256(sk.lobby_tape(*sk.LOBBIES['sk-zvt-w0']).encode()).hexdigest()
        golden = {'bundle': {'tape_sha256': tape}, 'gba': {'horizon': 3}, 'observations': []}
        report = dict(REPORT, world='0', world_chain='0', steps=4, host_ticks=4)
        series = sk.gba_series(self.trace, report)
        values, error = metrics.SCENARIOS['sk-zvt-w0']['measure_gba'](series, golden, report)
        self.assertIsNone(values)
        self.assertIn('incomplete trace', error)
        self.trace.write_text(TRACE.replace('S 3 2 3 3 3 0 0 0 -1 -1 0', 'S'))   # a step record cut to its tag
        values, error = metrics.SCENARIOS['sk-zvt-w0']['measure_gba'](series, golden, report)
        self.assertIsNone(values)
        self.assertIn('incomplete trace', error)

    def test_a_type_owned_at_launch_reads_frame_zero(self):
        tape = hashlib.sha256(sk.lobby_tape(*sk.LOBBIES['sk-zvt-w0']).encode()).hexdigest()
        golden = {'bundle': {'tape_sha256': tape}, 'gba': {'horizon': 3},
                  'observations': [{'check': 'build.first.Command Center', 'subject': 'cpu Terran', 'value': sk.NEVER,
                                    'lo': sk.NEVER, 'hi': sk.NEVER, 'gating': True}]}
        frames = list(sk.gba_frames(self.trace, str(self.trace) + '.gather', REPORT))
        original = sk.gba_frames
        try:
            sk.gba_frames = lambda *a: iter(frames)
            import sc64_parity_selfgate as selfgate
            check, selfgate.check_trace_complete = selfgate.check_trace_complete, lambda *a, **k: None
            values, error = metrics.SCENARIOS['sk-zvt-w0']['measure_gba'](
                sk.gba_series(self.trace, REPORT), golden, REPORT)
        finally:
            sk.gba_frames = original
            selfgate.check_trace_complete = check
        self.assertIsNone(error)
        self.assertEqual(values['values'][('build.first.Command Center', 'cpu Terran')], 0)

    def test_a_timing_counts_through_its_bands_edge(self):
        # The N64 run ends at frame 2 with the human defeated there (band 0-4); the GBA human
        # falls at frame 3: past the run's last frame, inside the band.
        tape = hashlib.sha256(sk.lobby_tape(*sk.LOBBIES['sk-zvt-w0']).encode()).hexdigest()

        def defeat(lo, hi, value=2, extra=(), report=REPORT):
            golden = {'bundle': {'tape_sha256': tape}, 'gba': {'horizon': 2},
                      'observations': [{'check': 'result.human_defeated', 'subject': 'human', 'value': value,
                                        'lo': lo, 'hi': hi, 'gating': True},
                                       {'check': 'workers@1000', 'subject': 'human', 'value': 1, 'lo': 1, 'hi': 1,
                                        'gating': True}, *extra]}
            import sc64_parity_selfgate as selfgate
            check, selfgate.check_trace_complete = selfgate.check_trace_complete, lambda *a, **k: None
            try:
                values, error = metrics.SCENARIOS['sk-zvt-w0']['measure_gba'](
                    sk.gba_series(self.trace, report), golden, report)
            finally:
                selfgate.check_trace_complete = check
            self.assertIsNone(error)
            cutoffs.append(values['cutoff'])
            return values['values'][('result.human_defeated', 'human')]
        cutoffs = []
        self.assertEqual(defeat(0, 4), 3)
        self.assertEqual(defeat(0, 2), sk.NEVER)            # past both the run and the band
        # A run that ended in the human's defeat reports the last frame its measurement saw (through
        # the band's edge, or the run's cutoff); any other end reports none.
        self.assertEqual(cutoffs, [None, None])
        lost = dict(REPORT, lost=1)
        defeat(0, 4, report=lost)
        defeat(0, 2, report=lost)
        self.assertTrue(isinstance(cutoffs[2], int) and 3 <= cutoffs[2] <= 4, cutoffs)
        self.assertTrue(isinstance(cutoffs[3], int) and cutoffs[3] <= 2, cutoffs)
        self.assertEqual(defeat(sk.NEVER, sk.NEVER, sk.NEVER), sk.NEVER)   # a NEVER keeps the run's cutoff
        # A seed that never saw it, inside an envelope reaching past its run (another check
        # extends the measurement): still the run's cutoff.
        wave = {'check': 'attack.first_wave', 'subject': 'cpu Terran', 'value': 2, 'lo': 0, 'hi': 4, 'gating': True}
        self.assertEqual(defeat(0, 4, sk.NEVER, [wave]), sk.NEVER)

    def test_runner_arguments(self):
        args = sk.gba_args('sk-ffa-w37', {'seed': 84421}, self.trace)
        self.assertEqual(args, ['--human', 't', '--bot', 'z', '--bot', 't', '--bot', 'p', '--seed', '84421',
                                '--gather', str(self.trace) + '.gather'])
        spec = metrics.SCENARIOS['sk-zvt-w0']
        self.assertEqual((spec['profile'], spec['runner'], spec['group']), ('zerg-z10', 'match', 'sk-zvt'))
        golden = {'gba': {'steps': 100, 'at_step': 5, 'seed': 7}}
        # A golden whose tape is not the lobby's tape is refused when judged.
        result = metrics.SCENARIOS['sk-zvt-w0']['measure_gba'](
            {}, {'bundle': {'tape_sha256': 'x'}, 'gba': {'horizon': 9}}, REPORT)
        self.assertIn('lobby_tape', result[1])
        _, cmd = sc64_parity.gba_command('sk-zvt-w0', golden, self.trace)
        self.assertIn('--seed', cmd)
        self.assertEqual(cmd[cmd.index('--human') + 1], 'z')


def golden(scenario_id, seed, end, obs):
    tape = hashlib.sha256(sk.lobby_tape(*sk.LOBBIES[scenario_id]).encode()).hexdigest()
    return {'id': scenario_id, 'gba': {'seed': seed, 'horizon': end}, 'layout_sha256': 'L',
            'decoder_revision': sk.DECODER_REVISION,
            'bundle': {'tape_sha256': tape},
            'observations': [{'check': c, 'subject': s, 'value': v, 'lo': v - t, 'hi': v + t,
                              'tolerance': 'x', 'gating': True} for c, s, v, t in obs]}


class Tape(unittest.TestCase):
    def test_lobby_tape_pins_rows_and_delay(self):
        zvt = sk.lobby_tape('Z', ['T'], 37).splitlines()
        self.assertEqual(zvt[0], 'load lobby.state 5120')
        # Human row: three Rights (Terran, Protoss, Random, Zerg); computer rows: Terran, then two Closed.
        self.assertEqual(zvt[zvt.index('tap tick rec') - 1], '97 0 0 0')
        self.assertEqual(zvt[zvt.index('tap tick rec') + 2], '10 2 0 0')
        self.assertEqual(sum(1 for line in zvt if line.startswith('watch ')), 27)
        self.assertNotEqual(sk.lobby_tape('Z', ['T'], 0), sk.lobby_tape('Z', ['T'], 37))
        self.assertNotEqual(sk.lobby_tape('Z', ['T'], 0), sk.lobby_tape('Z', ['P'], 0))
        # The free-for-all keeps the default rows: no Right on any computer row.
        ffa = sk.lobby_tape('T', ['Z', 'T', 'P'], 0).splitlines()
        self.assertEqual(ffa[1:10], ['10 4 0 0', '60 0 0 0'] * 4 + ['shot lobby'])


class ResultFrames(unittest.TestCase):
    def test_a_result_maps_to_the_tick_it_changed_in(self):
        with tempfile.TemporaryDirectory() as d:
            run = pathlib.Path(d)
            # Ticks 1-40 are frames 0-39; tick 16 lost its follow-up read. The computer (owner 2)
            # falls during tick 16, the human (owner 0) during tick 41, after the last read.
            (run / 'watch.csv').write_text('step,tick,address,bytes,value\n10,0,800b936c,4,0\n'
                                           '33,16,800b936c,4,200\n90,41,800b936c,4,2000200\n')
            ticks = [{'tick': k + 1, 'step': 2 * k + 3, 'sf': k, 'table': True, 'paused': False}
                     for k in range(40) if k != 15]
            cap = type('Cap', (), {'run': run, 'ticks': ticks, 'skipped_frames': [(32, 15)],
                                   'change_steps': {16: 32}, 'trailing_changes': [(41, False, True)],
                                   'layout': sc64_parity.parity_decode.Layout()})()
            self.assertEqual(sk.result_frames(cap), {2: (15, 2), 0: (40, 2)})
            self.assertEqual(sk.tick_frames(cap)[1], 40)

    def test_paused_ticks_add_no_frames(self):
        with tempfile.TemporaryDirectory() as d:
            run = pathlib.Path(d)
            # Ticks 1-10 are frames 0-9; ticks 11-14 are paused (the result screen). The computer
            # (owner 2) falls during paused tick 13; the human (owner 0) during tick 15, the one
            # tick after the last read (unpaused).
            log = ['step,tick,address,bytes,value', '10,0,800b936c,4,0', '30,13,800b936c,4,200',
                   '32,15,800b936c,4,2000200']
            (run / 'watch.csv').write_text(chr(10).join(log) + chr(10))
            ticks = [{'tick': k + 1, 'step': 2 * k + 3, 'sf': min(k, 9), 'table': True, 'paused': k >= 10}
                     for k in range(14)]
            cap = type('Cap', (), {'run': run, 'ticks': ticks, 'skipped_frames': [], 'change_steps': {},
                                   'change_tables': [True] * 15, 'trailing_changes': [(15, False, True)],
                                   'layout': sc64_parity.parity_decode.Layout()})()
            # A raw tick count would date them 12 and 14.
            self.assertEqual(sk.result_frames(cap), {2: (9, 2), 0: (10, 2)})
            self.assertEqual(sk.tick_frames(cap)[1], 10)
            # A paused trailing tick adds nothing; a tick read again keeps its latest state.
            cap.trailing_changes = [(15, True, True)]
            self.assertEqual(sk.tick_frames(cap)[1], 9)
            cap.trailing_changes = [(15, True, True), (15, False, True)]
            self.assertEqual(sk.tick_frames(cap)[1], 10)
            # Refused: a gap or a step back after the last follow-up, more than the one cut-off
            # tick, a read without a unit table after the mission started, a result past the last read.
            for bad in ([(16, False, True)], [(15, False, True), (14, False, True)],
                        [(15, False, True), (16, False, True)], [(15, False, False)]):
                cap.trailing_changes = bad
                with self.assertRaises(ValueError):
                    sk.tick_frames(cap)
            cap.trailing_changes = [(15, False, True)]
            # Interior change reads: the mission starts at the first read with a table in either
            # phase (here tick 1's follow-up, step 3); a later read without one is refused. The
            # lobby's reads before it may carry any tick number (the counter restarts at launch).
            cap.first_table_step = 3
            cap.change_reads = [(1, 30), (2, 1)] + [(2 * k + 2, k + 1) for k in range(1, 15)]
            cap.change_tables = [False, False] + [True] * 14
            self.assertEqual(sk.tick_frames(cap)[1], 10)
            cap.change_tables = [False, False, False] + [True] * 13
            with self.assertRaises(ValueError):
                sk.tick_frames(cap)
            cap.change_tables = [True] * 16
            cap.ticks = ticks + [{'tick': 15, 'step': 31, 'sf': 9, 'table': False, 'paused': False}]
            with self.assertRaises(ValueError):
                sk.tick_frames(cap)
            cap.ticks = ticks
            (run / 'watch.csv').write_text(chr(10).join(log[:3] + ['40,18,800b936c,4,2000200']) + chr(10))
            with self.assertRaises(ValueError):
                sk.result_frames(cap)

    def test_goldens_of_an_older_revision_are_stale(self):
        self.assertEqual(metrics.SCENARIOS['sk-zvt-w0']['decoder_revision'], sk.DECODER_REVISION)
        g = {'decoder_revision': 1, 'observations': [
            {'check': 'result.human_defeated', 'subject': 'human', 'value': 12, 'lo': 10, 'hi': 14, 'gating': True,
             'provenance': 'n64-ram', 'n_runs': 2, 'tolerance': 'x'}]}
        rows = sc64_parity.judge('sk-zvt-w0', g, {('result.human_defeated', 'human'): 12}, {}, {})
        self.assertEqual(rows[0]['verdict'], 'STALE-GOLDEN')
        old, new = golden('sk-zvt-w0', 1, 50, []), golden('sk-zvt-w101', 2, 50, [])
        old['decoder_revision'] = 1
        with tempfile.TemporaryDirectory() as d:
            paths = []
            for doc in (old, new):
                path = pathlib.Path(d) / f'{doc["id"]}.json'
                path.write_text(json.dumps(doc))
                paths.append(path)
            with self.assertRaises(ValueError):
                sk.envelope(paths)


class LayoutDigest(unittest.TestCase):
    def test_skirmish_fields_join_the_per_field_digest(self):
        layout = sc64_parity.parity_decode.Layout()
        keys = ['supply.used', 'result.word', 'rng.seed']
        before = layout.fields_digest(keys)
        self.assertTrue(all(layout.status(k) == 'proven' for k in keys))
        layout.doc['unit_fields']['order']['status'] = 'proven'          # an unrelated edit
        self.assertEqual(layout.fields_digest(keys), before)
        layout.doc['globals']['supply']['race_stride'] = '0x94'          # the block a golden read
        self.assertNotEqual(layout.fields_digest(keys), before)
        layout = sc64_parity.parity_decode.Layout()
        layout.doc['globals']['result']['address'] = '0x800B9370'
        self.assertNotEqual(layout.fields_digest(keys), before)


class Watch(unittest.TestCase):
    def test_width_and_coverage(self):
        with tempfile.TemporaryDirectory() as d:
            (pathlib.Path(d) / 'watch.csv').write_text('step,tick,address,bytes,value\n10,0,800b3af0,4,0\n'
                                                       '20,3,800b3af0,4,8\n10,0,800b3b20,1,0\n')
            w = sk.read_watch(d)
            self.assertEqual(sk.watched(w, 0x800B3AF0, 19), 0)
            self.assertEqual(sk.watched(w, 0x800B3AF0, 20), 8)
            with self.assertRaises(ValueError):
                sk.watched(w, 0x800B3AF0, 9)       # no value logged yet
            with self.assertRaises(ValueError):
                sk.watched(w, 0x800B3B20, 30)      # watched as one byte
            with self.assertRaises(ValueError):
                sk.watched(w, 0x800B3B50, 30)      # never watched


class Envelope(unittest.TestCase):
    def test_union_and_censoring(self):
        with tempfile.TemporaryDirectory() as d:
            a = pathlib.Path(d) / 'sk-zvt-w0.json'
            b = pathlib.Path(d) / 'sk-zvt-w37.json'
            a.write_text(json.dumps(golden('sk-zvt-w0', 1, 5000, [('build.first.Barracks', 'cpu Terran', 1000, 2),
                                                                  ('workers@1000', 'cpu Terran', 6, 0),
                                                                  ('build.first.Bunker', 'cpu Terran', 3000, 2)])))
            b.write_text(json.dumps(golden('sk-zvt-w37', 2, 4000, [('build.first.Barracks', 'cpu Terran', 1100, 2),
                                                                   ('workers@1000', 'cpu Terran', 6, 0)])))
            docs = {d['id']: d for d in sk.envelope([a, b])}
            for doc in docs.values():
                obs = {o['check']: o for o in doc['observations']}
                self.assertEqual((obs['build.first.Barracks']['lo'], obs['build.first.Barracks']['hi']), (998, 1102))
                self.assertIn('seed envelope', obs['build.first.Barracks']['tolerance'])
                self.assertEqual((obs['workers@1000']['lo'], obs['workers@1000']['hi']), (6, 6))   # no seed spread
                self.assertEqual(obs['workers@1000']['tolerance'], 'x')
                # A type one seed never made counts as NEVER for that seed.
                # NEVER stays a separate outcome: the range is the finite seeds' only.
                bunker = obs['build.first.Bunker']
                self.assertEqual((bunker['lo'], bunker['hi'], bunker['also_ok']), (2998, 3002, [sk.NEVER]))
            self.assertTrue(next(o for o in docs['sk-zvt-w37']['observations']
                                 if o['check'] == 'build.first.Bunker').get('envelope_fill'))
            self.assertEqual(docs['sk-zvt-w37']['envelope']['seeds'], [1, 2])
            for path in (a, b):
                path.write_text(json.dumps(docs[path.stem]))
            again = {d['id']: d for d in sk.envelope([a, b])}   # re-run over its own output: same result
            self.assertEqual(again['sk-zvt-w0']['observations'], docs['sk-zvt-w0']['observations'])
            self.assertEqual(again['sk-zvt-w37']['observations'], docs['sk-zvt-w37']['observations'])
            # Regenerated without the only seed that built a Bunker: the synthesised row goes away.
            a.write_text(json.dumps(golden('sk-zvt-w0', 1, 5000, [('workers@1000', 'cpu Terran', 6, 0)])))
            third = {d['id']: d for d in sk.envelope([a, b])}
            self.assertNotIn('build.first.Bunker', {o['check'] for o in third['sk-zvt-w37']['observations']})

    def test_a_shorter_run_waives_nothing_after_its_end(self):
        with tempfile.TemporaryDirectory() as d:
            a = pathlib.Path(d) / 'sk-ffa-w0.json'
            b = pathlib.Path(d) / 'sk-ffa-w37.json'
            a.write_text(json.dumps(golden('sk-ffa-w0', 1, 3582, [('build.first.Academy', 'cpu Terran', sk.NEVER, 0),
                                                                  ('build.first.Barracks', 'cpu Terran', 1292, 2)])))
            b.write_text(json.dumps(golden('sk-ffa-w37', 2, 4954, [('build.first.Academy', 'cpu Terran', 4831, 2),
                                                                   ('build.first.Barracks', 'cpu Terran', 1300, 2)])))
            docs = {doc['id']: {o['check']: o for o in doc['observations']} for doc in sk.envelope([a, b])}
            # w0 ended at 3,582: its missing Academy says nothing about 4,831.
            self.assertNotIn('also_ok', docs['sk-ffa-w37']['build.first.Academy'])
            self.assertEqual((docs['sk-ffa-w37']['build.first.Academy']['lo'],
                              docs['sk-ffa-w37']['build.first.Academy']['hi']), (4829, 4833))
            # ... and w37's Academy came after w0's run, so w0 keeps its own NEVER.
            self.assertEqual(docs['sk-ffa-w0']['build.first.Academy']['hi'], sk.NEVER)
            self.assertEqual((docs['sk-ffa-w0']['build.first.Barracks']['lo'],
                              docs['sk-ffa-w0']['build.first.Barracks']['hi']), (1290, 1302))

    def test_bands_come_from_the_seeds_own_rows(self):
        def write(d, name, end, at):
            path = pathlib.Path(d) / f'{name}.json'
            path.write_text(json.dumps(golden(name, int(name.rsplit('w', 1)[1]) + 1, end,
                                              [('build.first.Bunker', 'cpu Terran', at, 2)])))
            return path
        results = []
        with tempfile.TemporaryDirectory() as d:
            paths = [write(d, 'sk-zvt-w0', 5000, 1000), write(d, 'sk-zvt-w37', 2000, 1200),
                     write(d, 'sk-zvt-w101', 5000, 3000)]
            for order in (paths, paths[::-1], [paths[1], paths[2], paths[0]]):
                docs = {doc['id']: doc['observations'][0] for doc in sk.envelope(order)}
                results.append({k: (v['lo'], v['hi']) for k, v in docs.items()})
        self.assertEqual(results[0], results[1])
        self.assertEqual(results[0], results[2])
        # The run that ended at 2,000 never takes the 3,000-frame band, directly or through another seed.
        self.assertEqual(results[0]['sk-zvt-w37'], (998, 1202))
        self.assertEqual(results[0]['sk-zvt-w0'], (998, 3002))

    def test_a_member_with_another_tape_is_refused(self):
        with tempfile.TemporaryDirectory() as d:
            a = pathlib.Path(d) / 'sk-zvt-w0.json'
            b = pathlib.Path(d) / 'sk-zvt-w37.json'
            a.write_text(json.dumps(golden('sk-zvt-w0', 1, 5000, [('workers@1000', 'cpu Terran', 6, 0)])))
            other = golden('sk-zvt-w37', 2, 5000, [('workers@1000', 'cpu Terran', 9, 0)])
            other['bundle']['tape_sha256'] = 'f' * 64
            b.write_text(json.dumps(other))
            with self.assertRaises(ValueError):
                sk.envelope([a, b])

    def test_counts_are_not_cut_at_the_horizon(self):
        with tempfile.TemporaryDirectory() as d:
            a = pathlib.Path(d) / 'sk-tvz-w0.json'
            b = pathlib.Path(d) / 'sk-tvz-w37.json'
            a.write_text(json.dumps(golden('sk-tvz-w0', 1, 4016, [('econ.gathered@4000', 'cpu Zerg', 600, 40)])))
            b.write_text(json.dumps(golden('sk-tvz-w37', 2, 15000, [('econ.gathered@4000', 'cpu Zerg', 4200, 40)])))
            docs = {doc['id']: doc['observations'][0] for doc in sk.envelope([a, b])}
            self.assertEqual((docs['sk-tvz-w0']['lo'], docs['sk-tvz-w0']['hi']), (560, 4240))

    def test_duplicate_seeds_are_refused(self):
        with tempfile.TemporaryDirectory() as d:
            a = pathlib.Path(d) / 'sk-zvt-w0.json'
            b = pathlib.Path(d) / 'sk-zvt-w37.json'
            a.write_text(json.dumps(golden('sk-zvt-w0', 5, 5000, [('workers@1000', 'cpu Terran', 6, 0)])))
            b.write_text(json.dumps(golden('sk-zvt-w37', 5, 5000, [('workers@1000', 'cpu Terran', 6, 0)])))
            with self.assertRaises(ValueError):
                sk.envelope([a, b])

    def test_singleton_group_returns_to_its_own_band(self):
        with tempfile.TemporaryDirectory() as d:
            a = pathlib.Path(d) / 'sk-zvt-w0.json'
            b = pathlib.Path(d) / 'sk-zvt-w37.json'
            a.write_text(json.dumps(golden('sk-zvt-w0', 1, 5000, [('workers@1000', 'cpu Terran', 6, 0),
                                                                  ('build.first.Bunker', 'cpu Terran', 3000, 2)])))
            b.write_text(json.dumps(golden('sk-zvt-w37', 2, 5000, [('workers@1000', 'cpu Terran', 7, 0)])))
            for path, doc in zip((a, b), sk.envelope([a, b])):
                path.write_text(json.dumps(doc))
            alone = sk.envelope([b])[0]
            self.assertEqual([(o['check'], o['lo'], o['hi']) for o in alone['observations']], [('workers@1000', 7, 7)])

    def test_corner_stays_exact_and_layouts_must_match(self):
        with tempfile.TemporaryDirectory() as d:
            a = pathlib.Path(d) / 'sk-tvz-w0.json'
            b = pathlib.Path(d) / 'sk-tvz-w37.json'
            a.write_text(json.dumps(golden('sk-tvz-w0', 1, 5000, [('start.corner', 'human', 0, 0)])))
            b.write_text(json.dumps(golden('sk-tvz-w37', 2, 5000, [('start.corner', 'human', 3, 0)])))
            for doc in sk.envelope([a, b]):
                obs = doc['observations'][0]
                self.assertEqual(obs['lo'], obs['hi'])
            other = json.loads(b.read_text())
            other['layout_sha256'] = 'x'
            b.write_text(json.dumps(other))
            with self.assertRaises(ValueError):
                sk.envelope([a, b])

    def test_seed_bands_are_kept(self):
        with tempfile.TemporaryDirectory() as d:
            a = pathlib.Path(d) / 'sk-zvt-w0.json'
            b = pathlib.Path(d) / 'sk-zvt-w37.json'
            a.write_text(json.dumps(golden('sk-zvt-w0', 1, 5000, [('workers@1000', 'cpu Terran', 6, 0)])))
            b.write_text(json.dumps(golden('sk-zvt-w37', 2, 5000, [('workers@1000', 'cpu Terran', 7, 0)])))
            for path, doc in zip((a, b), sk.envelope([a, b])):
                path.write_text(json.dumps(doc))
            # A re-run over enveloped files unions the seeds' own bands, not the widened ones.
            b.write_text(json.dumps(golden('sk-zvt-w37', 2, 5000, [('workers@1000', 'cpu Terran', 6, 0)])))
            docs = sk.envelope([a, b])
            obs = docs[0]['observations'][0]
            self.assertEqual((obs['lo'], obs['hi']), (6, 6))


class Judge(unittest.TestCase):
    def test_values_keyed_by_subject_and_curve_steps(self):
        g = {'decoder_revision': sk.DECODER_REVISION, 'observations': [
            {'check': 'workers@1000', 'subject': 'human', 'value': 4, 'lo': 4, 'hi': 4, 'gating': True,
             'provenance': 'n64-ram', 'n_runs': 2, 'tolerance': 'exact'},
            {'check': 'workers@1000', 'subject': 'cpu Zerg', 'value': 9, 'lo': 9, 'hi': 9, 'gating': True,
             'provenance': 'n64-ram', 'n_runs': 2, 'tolerance': 'exact'}],
            'curves': {'workers:cpu Zerg': {'n64': [4, 4, 5, 6], 'tolerance': 0, 'step': 8,
                                            'explains': [['workers@1000', 'cpu Zerg']]}}}
        rows = sc64_parity.judge('sk-zvz-w0', g, {('workers@1000', 'human'): 4, ('workers@1000', 'cpu Zerg'): 7},
                                 {'workers:cpu Zerg': [4, 4, 4, 5]}, {})
        by = {r['subject']: r for r in rows}
        self.assertEqual(by['human']['verdict'], 'PASS')
        self.assertEqual(by['cpu Zerg']['verdict'], 'REGRESSION')
        self.assertEqual(by['cpu Zerg']['first_divergent_frame'], 16)
        self.assertNotIn('first_divergent_frame', by['human'])

    def test_a_stale_row_keeps_the_players_value(self):
        g = {'observations': [{'check': 'workers@1000', 'subject': 'cpu Zerg', 'value': 9, 'lo': 9, 'hi': 9,
                               'gating': True, 'provenance': 'n64-ram', 'n_runs': 2, 'tolerance': 'exact',
                               'layout_fields': ['unit.type'], 'layout_reads': ['unit.type'],
                               'layout_proven': ['unit.type'], 'layout_fields_sha256': 'stale'}]}
        rows = sc64_parity.judge('sk-zvz-w0', g, {('workers@1000', 'cpu Zerg'): 7}, {}, {},
                                 sc64_parity.parity_decode.Layout(), allow_stale=True)
        self.assertEqual((rows[0]['verdict'], rows[0]['gba']), ('REPORT', 7))

    def test_also_ok_admits_never_only(self):
        g = {'decoder_revision': sk.DECODER_REVISION, 'observations': [{'check': 'build.first.Bunker', 'subject': 'cpu Terran', 'value': 3000, 'lo': 2998,
                               'hi': 3002, 'also_ok': [sk.NEVER], 'gating': True, 'provenance': 'n64-ram',
                               'n_runs': 2, 'tolerance': 'x'}]}
        verdict = lambda v: sc64_parity.judge('sk-zvt-w0', g, {('build.first.Bunker', 'cpu Terran'): v}, {}, {})[0]['verdict']
        self.assertEqual(verdict(sk.NEVER), 'PASS')
        self.assertEqual(verdict(3001), 'PASS')
        self.assertEqual(verdict(8000), 'REGRESSION')
        # A baseline holding NEVER (once outside, now admitted) allows no far-out time.
        known = {'sk-zvt-w0:build.first.Bunker:cpu Terran': {'gba': sk.NEVER}}
        row = sc64_parity.judge('sk-zvt-w0', g, {('build.first.Bunker', 'cpu Terran'): 8000}, {}, known)[0]
        self.assertEqual(row['verdict'], 'REGRESSION')

    def test_sentinels_never_count_as_distance(self):
        def obs(value, lo, hi):
            return {'observations': [{'check': 'c', 'subject': 's', 'value': value, 'lo': lo, 'hi': hi,
                                      'sentinels': [sk.NEVER, sk.ENDED], 'gating': True, 'provenance': 'n64-ram',
                                      'n_runs': 2, 'tolerance': 'x'}]}
        def verdict(golden, gba, known):
            return sc64_parity.judge('sk', golden, {('c', 's'): gba}, {}, {'sk:c:s': {'gba': known}})[0]['verdict']
        defeat = obs(10138, 10136, 10140)
        self.assertEqual(verdict(defeat, 8500, sk.NEVER), 'REGRESSION')    # was NEVER, now a time
        self.assertEqual(verdict(defeat, sk.NEVER, 8500), 'REGRESSION')    # was a time, now NEVER
        self.assertEqual(verdict(defeat, sk.NEVER, sk.NEVER), 'KNOWN')
        gathered = obs(0, 0, 32)
        self.assertEqual(verdict(gathered, sk.ENDED, 40), 'REGRESSION')    # the GBA match ended early
        self.assertEqual(verdict(gathered, 41, 40), 'KNOWN')



class EnvelopeSlack(unittest.TestCase):
    def test_a_worse_result_is_a_regression_in_a_wide_envelope(self):
        # Two seeds widen the band to 736-3245; each seed's own band is +-2 frames.
        def golden(seed_lo, seed_hi):
            return {'decoder_revision': sk.DECODER_REVISION, 'observations': [{'check': 'build.first.Spawning Pool', 'subject': 'cpu Zerg', 'value': 738,
                                      'lo': 736, 'hi': 3245, 'seed_lo': seed_lo, 'seed_hi': seed_hi,
                                      'gating': True, 'provenance': 'n64-ram', 'n_runs': 2, 'tolerance': 'x'}]}
        known = {'sk-tvz-w0:build.first.Spawning Pool:cpu Zerg': {'gba': 3250}}

        def verdict(g, v):
            return sc64_parity.judge('sk-tvz-w0', g, {('build.first.Spawning Pool', 'cpu Zerg'): v}, {}, known)[0]['verdict']
        g = golden(736, 740)
        self.assertEqual(verdict(g, 4200), 'REGRESSION')
        self.assertEqual(verdict(g, 3251), 'KNOWN')        # within the seed's own +-2
        self.assertEqual(verdict(g, 3000), 'IMPROVED')
        # Without seed bands (any other scenario) the band's own half width stays the slack.
        plain = {'decoder_revision': sk.DECODER_REVISION, 'observations': [dict(golden(0, 0)['observations'][0])]}
        for k in ('seed_lo', 'seed_hi'):
            del plain['observations'][0][k]
        self.assertEqual(verdict(plain, 4200), 'KNOWN')


class FrameOffsets(unittest.TestCase):
    """Skirmish lines are judged on the spread of three GBA runs whose frame counters start 0, 1
    and 2 frames apart (the GBA's own one-frame noise)."""
    KEY = 'sk-pvt-w0:supply.cap@8000:cpu Terran'

    def golden(self, value=84, lo=84, hi=84, **extra):
        return {'decoder_revision': sk.DECODER_REVISION, 'observations': [dict(
            {'check': 'supply.cap@8000', 'subject': 'cpu Terran', 'value': value, 'lo': lo, 'hi': hi,
             'sentinels': [sk.NEVER, sk.ENDED], 'gating': True, 'provenance': 'n64-ram', 'n_runs': 2,
             'tolerance': 'exact'}, **extra)]}

    def row(self, spread, baseline=None, golden=None):
        golden = golden or self.golden()
        obs = golden['observations'][0]
        key = (obs['check'], obs['subject'])
        name = f'sk-pvt-w0:{obs["check"]}:{obs["subject"]}'
        more = [(k, {key: v}) for k, v in zip((1, 2), spread[1:])]
        known = {} if baseline is None else {name: {'key': name, 'gba': baseline[0], 'gba_offsets': baseline}}
        return sc64_parity.judge('sk-pvt-w0', golden, {key: spread[0]}, {}, known, more=more)[0]

    def verdict(self, spread, baseline=None, golden=None):
        return self.row(spread, baseline, golden)['verdict']

    def test_offsets_are_the_scenarios_and_reach_the_runner(self):
        spec = metrics.SCENARIOS['sk-pvt-w0']
        self.assertEqual(spec['frame_offsets'], (0, 1, 2))
        golden = {'gba': {'steps': 100, 'at_step': 5, 'seed': 7}}
        _, plain = sc64_parity.gba_command('sk-pvt-w0', golden, pathlib.Path('x.trace'))
        _, shifted = sc64_parity.gba_command('sk-pvt-w0', golden, pathlib.Path('x.trace'), 2)
        self.assertNotIn('--frame-offset', plain)       # offset 0 is the single-run command
        self.assertEqual(shifted, plain + ['--frame-offset', '2'])
        with self.assertRaises(ValueError):
            sc64_parity.gba_command('sk-pvt-w0', golden, pathlib.Path('x.trace'), 3)
        self.assertNotIn('frame_offsets', metrics.SCENARIOS['bb-train'])
        with self.assertRaises(ValueError):
            sc64_parity.gba_command('bb-train', {'gba': {'steps': 10, 'at_step': 5}}, pathlib.Path('x.trace'), 1)

    def test_jitter_within_the_noise_never_flips(self):
        # The engine session's measurement: 116 / 132 / 116 against the N64's 84.
        base = [116, 132, 116]
        for spread in ([132, 132, 116], [132, 116, 132], [116, 116, 116], [132, 132, 132], [100, 116, 132]):
            row = self.row(spread, base)
            self.assertEqual(row['verdict'], 'KNOWN', spread)
            self.assertEqual((row['gba'], row['gba_offsets'], row['baseline_offsets']), (spread[0], spread, base))
        # A single run of the same noise against a single-run baseline flips both ways.
        self.assertEqual(self.verdict([132], [116]), 'REGRESSION')

    def test_a_consistent_shift_still_reads_regression(self):
        base = [116, 132, 116]
        row = self.row([140, 148, 140], base)
        self.assertEqual(row['verdict'], 'REGRESSION')
        self.assertIn('at every frame offset', row['why'])
        self.assertEqual(self.verdict([132, 148, 140], base), 'KNOWN')     # one offset still at the baseline's edge
        # The price of robustness: a shift no larger than the line's own spread reads KNOWN.
        self.assertEqual(self.verdict([132, 148, 132], base), 'KNOWN')
        # A line with no baseline: a regression when every offset is outside the band ...
        self.assertEqual(self.verdict([90, 92, 91]), 'REGRESSION')
        # ... and KNOWN (recorded by --accept) while the noise still reaches the band.
        row = self.row([84, 92, 84])
        self.assertEqual(row['verdict'], 'KNOWN')
        self.assertIn('frame offset +1, inside at +0, +2 (GBA frame noise)', row['why'])
        self.assertNotIn('baseline', row)

    def test_improved_only_when_every_offset_is_inside(self):
        self.assertEqual(self.verdict([84, 84, 100], [116, 132, 116]), 'KNOWN')
        self.assertEqual(self.verdict([84, 84, 84], [116, 132, 116]), 'IMPROVED')
        self.assertEqual(self.verdict([84, 84, 84]), 'PASS')

    def test_the_seed_bands_slack_still_holds(self):
        # The #988 case: two seeds widen the band to 736-3245, each seed's own band is +-2.
        golden = self.golden(738, 736, 3245, seed_lo=736, seed_hi=740)
        base = [3250, 3260, 3255]                               # 5, 15, 10 frames out
        self.assertEqual(self.verdict([3261, 3262, 3250], base, golden), 'KNOWN')
        self.assertEqual(self.verdict([3262, 3262, 3262], base, golden), 'KNOWN')       # 17 = 15 + 2
        self.assertEqual(self.verdict([3263, 3264, 3263], base, golden), 'REGRESSION')  # 18 > 15 + 2

    def test_outcomes_are_compared_as_outcomes(self):
        wave = self.golden(17045, 17043, 17047)
        # Some offsets never reach the event: noise as long as an outcome is shared with the baseline.
        self.assertEqual(self.verdict([sk.NEVER, 15000, sk.NEVER], [sk.NEVER, sk.NEVER, 14000], wave), 'KNOWN')
        self.assertEqual(self.verdict([14000, 14000, 14000], [sk.NEVER, 14000, 14100], wave), 'KNOWN')
        row = self.row([8000, 8100, 8050], [sk.NEVER, sk.NEVER, sk.NEVER], wave)
        self.assertEqual(row['verdict'], 'REGRESSION')
        self.assertIn('outcome changed', row['why'])
        self.assertEqual(self.verdict([sk.NEVER] * 3, [8000, 8100, 8050], wave), 'REGRESSION')
        # An offset that keeps an allowed outcome (an also_ok NEVER) or lands in the band hides nothing:
        # numbers that all moved past the recorded numbers, or a new outcome, still fail.
        allowed = self.golden(17045, 17043, 17047, also_ok=[sk.NEVER])
        self.assertEqual(self.verdict([sk.NEVER, 15950, 16010], [sk.NEVER, 16000, 15900], allowed), 'KNOWN')
        self.assertEqual(self.verdict([sk.NEVER, 12000, 12000], [sk.NEVER, 16000, 15900], allowed), 'REGRESSION')
        # A check that is not a timing (no time for NEVER): an in-band offset keeps it KNOWN ...
        self.assertEqual(self.verdict([17045, 12000, 12000], [sk.NEVER] * 3, wave), 'KNOWN')
        # ... while for a timing the far-early offsets are judged on their own.
        timed = dict(self.golden(17045, 17043, 17047, check='attack.first_wave', subject='cpu Zerg'),
                     gba={'horizon': 17600})
        self.assertEqual(self.verdict([17045, 12000, 12000], [sk.NEVER] * 3, timed), 'REGRESSION')
        self.assertEqual(self.verdict([17045, 17044, 17046], [sk.NEVER] * 3, wave), 'IMPROVED')
        # A checkpoint some offsets no longer reach (the GBA match ended) is noise while another offset
        # keeps the baseline's outcome; the match's end is gated by its own result line.
        self.assertEqual(self.verdict([116, sk.ENDED, sk.ENDED], [116, 132, 116]), 'KNOWN')
        self.assertEqual(self.verdict([sk.ENDED] * 3, [116, 132, 116]), 'REGRESSION')
        # Against a band that is itself an outcome (the N64 never built it), a time a frame earlier
        # is not further out; a line with no baseline still fails when every offset builds it.
        never = self.golden(sk.NEVER, sk.NEVER, sk.NEVER)
        self.assertEqual(self.verdict([8262, 8261, sk.NEVER], [sk.NEVER, 8264, 8263], never), 'KNOWN')
        self.assertEqual(self.verdict([5000, 5000, 5000], [sk.NEVER, 8264, 8263], never), 'KNOWN')
        self.assertEqual(self.verdict([8262, 8261, 8260], None, never), 'REGRESSION')
        self.assertEqual(self.verdict([sk.NEVER] * 3, [sk.NEVER, 8264, 8263], never), 'IMPROVED')

    def test_a_changed_outcome_is_judged_by_its_own_time(self):
        # sk-tvt-w0: the N64 human falls at 10,076; on the GBA the human survived the run (NEVER, "not
        # before frame 10,677") at every recorded offset.
        defeat = dict(self.golden(10076, 10074, 10078, check='result.human_defeated', subject='human'),
                      gba={'horizon': 10676})
        self.assertEqual(sk.never_time(defeat, defeat['observations'][0]), (sk.NEVER, 10677))
        survived = [sk.NEVER] * 3
        row = self.row([sk.NEVER, 1000, 1001], survived, defeat)        # Codex's case: two offsets fall 9,000 early
        self.assertEqual(row['verdict'], 'REGRESSION')
        self.assertIn('1000 at frame offset +1', row['why'])
        self.assertEqual(self.verdict([1000] * 3, survived, defeat), 'REGRESSION')
        # A defeat between the band and "never" is nearer the N64 than surviving: not worse.
        self.assertEqual(self.verdict([sk.NEVER, 10500, sk.NEVER], survived, defeat), 'KNOWN')
        self.assertEqual(self.verdict([10500] * 3, survived, defeat), 'KNOWN')
        # A small shift of an existing defeat, an offset now surviving included, stays within noise.
        late = [10300, 10320, sk.NEVER]
        self.assertEqual(self.verdict([10310, sk.NEVER, 10330], late, defeat), 'KNOWN')
        self.assertEqual(self.verdict([10302, 10322, 10318], late, defeat), 'KNOWN')
        self.assertEqual(self.verdict([1000, 10320, sk.NEVER], late, defeat), 'KNOWN')   # numbers: whole range
        self.assertEqual(self.verdict([1000, 1010, sk.NEVER], late, defeat), 'REGRESSION')
        # The offset that survived now falls 9,000 frames early: judged against its own offset.
        self.assertEqual(self.verdict([10300, 10320, 1000], late, defeat), 'REGRESSION')
        # Time to NEVER only bounds the time from below (the run's reach): sk-tvt-w0's wave at
        # 10,077-10,079 reads NEVER at offsets +3 and +4 of the same engine, so one offset is noise ...
        wave = dict(self.golden(9539, 9537, 9541, check='attack.first_wave', subject='cpu Terran'),
                    gba={'horizon': 10080})
        self.assertEqual(self.verdict([sk.NEVER, 10078, 10077], [10079, 10078, 10077], wave), 'KNOWN')
        self.assertEqual(self.verdict([10077, sk.NEVER, sk.NEVER], [10079, 10078, 10077], wave), 'KNOWN')
        # ... and every offset past the run's reach is a regression once the reach lies beyond every
        # recorded time by more than the slack: not with a run that ended a frame after the wave
        # (NEVER reads 10,081, 2 frames past 10,079), but with one that went on to 10,085.
        self.assertEqual(self.verdict([sk.NEVER] * 3, [10079, 10078, 10077], wave), 'KNOWN')
        longer = dict(wave, gba={'horizon': 10085})
        self.assertEqual(self.verdict([sk.NEVER] * 3, [10079, 10078, 10077], longer), 'REGRESSION')
        # A line with no baseline: its band's outcome at some offset keeps it KNOWN (recorded by --accept).
        self.assertEqual(self.verdict([10076, sk.NEVER, 10075], None, defeat), 'KNOWN')
        self.assertEqual(self.verdict([10076, 10090, 10075], None, defeat), 'KNOWN')

    def test_a_new_time_must_clear_the_gba_noise_of_its_kind(self):
        # sk-pvt-w0's Factory: the N64 builds it at 9,126; the GBA never does at offsets 0-2 but does at
        # 8,059 at +4 of the same engine. Build-order firsts move up to 1,352 frames with the offset
        # (sk-zvp-w0's Stargate), so 1,067 frames early is within noise; 4,126 is not.
        self.assertEqual(metrics.SCENARIOS['sk-pvt-w0']['frame_noise'], sk.FRAME_NOISE)
        self.assertEqual(sk.FRAME_NOISE['build.first'], 1352)
        factory = dict(self.golden(9126, 9124, 9128, check='build.first.Factory', subject='cpu Terran'),
                       gba={'horizon': 9300})
        key = ('build.first.Factory', 'cpu Terran')
        line = 'sk-pvt-w0:build.first.Factory:cpu Terran'
        baseline = {line: {'key': line, 'gba': sk.NEVER, 'gba_offsets': [sk.NEVER] * 3}}

        def verdict(spread, known):
            return sc64_parity.judge('sk-pvt-w0', factory, {key: spread[0]}, {}, known,
                                     more=[(1, {key: spread[1]}), (2, {key: spread[2]})])[0]['verdict']
        self.assertEqual(verdict([sk.NEVER, sk.NEVER, 8059], baseline), 'KNOWN')
        self.assertEqual(verdict([sk.NEVER, sk.NEVER, 5000], baseline), 'REGRESSION')
        # The allowance is a fixed calibration: other baseline lines (accepted, removed, or absurd)
        # never change it.
        other = 'sk-zvp-w0:build.first.Stargate:cpu Protoss'
        for extra in ({}, {other: {'key': other, 'gba': 0, 'gba_offsets': [0, 1000000000, 0]}}):
            self.assertEqual(verdict([sk.NEVER, sk.NEVER, 5000], dict(baseline, **extra)), 'REGRESSION')
            self.assertEqual(verdict([sk.NEVER, sk.NEVER, 8059], dict(baseline, **extra)), 'KNOWN')
        # A finite time moving between offsets of a mixed baseline: within the noise it stays KNOWN,
        # beyond it it fails.
        mixed = {line: {'key': line, 'gba': sk.NEVER, 'gba_offsets': [sk.NEVER, 8059, sk.NEVER]}}
        self.assertEqual(verdict([8056, sk.NEVER, sk.NEVER], mixed), 'KNOWN')
        self.assertEqual(verdict([8056, 8055, sk.NEVER], mixed), 'KNOWN')
        self.assertEqual(verdict([sk.NEVER, 8050, sk.NEVER], mixed), 'KNOWN')
        self.assertEqual(verdict([5000, sk.NEVER, sk.NEVER], mixed), 'REGRESSION')
        self.assertEqual(verdict([sk.NEVER, 5000, sk.NEVER], mixed), 'REGRESSION')

    def test_never_stands_for_each_runs_own_cutoff(self):
        # Codex follow-up: the GBA human fell at 9,000 at every offset, so its traces end there and the
        # Terran wave reads NEVER. Against the run's reach (12,000) that NEVER would be 2,460 frames
        # out; it means only "not before 9,001", which is no further out than the recorded waves.
        wave = dict(self.golden(9539, 9537, 9541, check='attack.first_wave', subject='cpu Terran'),
                    gba={'horizon': 12000})
        key = ('attack.first_wave', 'cpu Terran')
        line = 'sk-pvt-w0:attack.first_wave:cpu Terran'
        known = {line: {'key': line, 'gba': 10079, 'gba_offsets': [10079, 10078, 10077]}}

        def verdict(cutoffs):
            more = [(1, {key: sk.NEVER}), (2, {key: sk.NEVER})]
            return sc64_parity.judge('sk-pvt-w0', wave, {key: sk.NEVER}, {}, known, more=more,
                                     cutoffs=cutoffs)[0]['verdict']
        self.assertEqual(verdict(None), 'REGRESSION')
        self.assertEqual(verdict([9000, 9000, 9000]), 'KNOWN')
        self.assertEqual(verdict([12000, 12000, 12000]), 'REGRESSION')   # a run that went on: past the reach
        # A run that ended before the band says nothing about the distance: it might have come in it.
        self.assertEqual(verdict([5000, 5000, 5000]), 'KNOWN')

        # With no baseline line: cut short below the band at every offset is not shown outside it
        # (KNOWN, not PASS); every run reaching past the band without the wave is.
        def unlisted(cutoffs):
            more = [(1, {key: sk.NEVER}), (2, {key: sk.NEVER})]
            return sc64_parity.judge('sk-pvt-w0', wave, {key: sk.NEVER}, {}, {}, more=more,
                                     cutoffs=cutoffs)[0]['verdict']
        self.assertEqual(unlisted([9000, 9000, 9000]), 'KNOWN')
        row = sc64_parity.judge('sk-pvt-w0', wave, {key: sk.NEVER}, {}, {}, cutoffs=[9000, 9600, 9000],
                                more=[(1, {key: sk.NEVER}), (2, {key: sk.NEVER})])[0]
        self.assertIn('outside the band at frame offset +1;', row['why'])
        self.assertNotIn('noise', row['why'])
        self.assertIn('cut short by the human\'s defeat before it could show the band at +0, +2', row['why'])
        # In-band and cut-short offsets with none outside: both groups named.
        gate = dict(self.golden(9539, 9537, 9541, check='attack.first_wave', subject='cpu Terran'),
                    gba={'horizon': 12000})
        row = sc64_parity.judge('sk-pvt-w0', gate, {key: 9539}, {}, {}, cutoffs=[None, 9000, None],
                                more=[(1, {key: sk.NEVER}), (2, {key: 9540})])[0]
        self.assertEqual(row['verdict'], 'KNOWN')
        self.assertIn('outside the band at no offset, inside at +0, +2', row['why'])
        self.assertIn('show the band at +1', row['why'])
        # A regression the numbers show at offset 0 alone, the others cut short, names offset 0.
        line = 'sk-pvt-w0:attack.first_wave:cpu Terran'
        known = {line: {'key': line, 'gba': 10079, 'gba_offsets': [10079, 10078, 10077]}}
        row = sc64_parity.judge('sk-pvt-w0', gate, {key: 11000}, {}, known, cutoffs=[None, 9000, 9000],
                                more=[(1, {key: sk.NEVER}), (2, {key: sk.NEVER})])[0]
        self.assertEqual(row['verdict'], 'REGRESSION')
        self.assertTrue(row['why'].endswith('at frame offset +0'), row['why'])
        self.assertEqual(unlisted([9600, 9600, 9600]), 'REGRESSION')
        self.assertEqual(unlisted([9600, 9000, 9600]), 'KNOWN')          # each run's own cutoff
        self.assertEqual(unlisted(None), 'REGRESSION')
        self.assertEqual(sk.never_time(wave, wave['observations'][0], 9000), (sk.NEVER, 9001))
        self.assertEqual(sk.never_time(wave, wave['observations'][0], 15000), (sk.NEVER, 12001))

    def test_an_allowed_never_counts_only_from_a_run_that_reached_the_band(self):
        # sk-ffa-w0's Gateway: band 2,249-2,253 with NEVER allowed by the seed envelope. Runs that all
        # ended at 2,149 never reached the band: with a baseline line KNOWN, never IMPROVED (which
        # --accept would drop); without one PASS, as before (an early end adds no lines).
        gate = dict(self.golden(2251, 2249, 2253, check='build.first.Gateway', subject='cpu Protoss',
                                also_ok=[sk.NEVER]), gba={'horizon': 9000})
        key = ('build.first.Gateway', 'cpu Protoss')
        line = 'sk-pvt-w0:build.first.Gateway:cpu Protoss'
        known = {line: {'key': line, 'gba': 1794, 'gba_offsets': [1794, 1804, 1804]}}

        def verdict(cutoffs, baseline):
            more = [(1, {key: sk.NEVER}), (2, {key: sk.NEVER})]
            return sc64_parity.judge('sk-pvt-w0', gate, {key: sk.NEVER}, {}, baseline, more=more,
                                     cutoffs=cutoffs)[0]['verdict']
        self.assertEqual(verdict([2149] * 3, known), 'KNOWN')
        self.assertEqual(verdict([2149, 2300, 2300], known), 'KNOWN')
        self.assertEqual(verdict([2149] * 3, {}), 'PASS')
        # The edge: a run whose last measured frame is just before the band has not reached it.
        self.assertEqual(verdict([2248] * 3, known), 'KNOWN')
        self.assertEqual(verdict([2249] * 3, known), 'KNOWN')      # reached the band, short of the reach
        self.assertEqual(verdict([9000] * 3, known), 'IMPROVED')
        # An early end at the band's edge at offset 0, the others in the band: the line and its
        # recorded offset 0 are kept, for a line with offsets and for a single run's line.
        for baseline in (known, {line: {'key': line, 'gba': 1794}}):
            row = sc64_parity.judge('sk-pvt-w0', gate, {key: sk.NEVER}, {}, baseline, cutoffs=[2249, None, None],
                                    more=[(1, {key: 2251}), (2, {key: 2252})])[0]
            self.assertEqual(row['verdict'], 'KNOWN')
            with tempfile.TemporaryDirectory() as temp:
                path = pathlib.Path(temp) / 'b.json'
                sc64_parity.write_baseline(path, [{'id': 'sk-pvt-w0', 'status': 'RUN', 'rows': [row]}], {})
                self.assertEqual(sc64_parity.load_baseline(path)[line]['gba_offsets'], [1794, 2251, 2252])
        # Runs that reached the band without building it: the allowed NEVER counts (a line with a
        # baseline line is dropped only once the runs also reached the run's reach).
        self.assertEqual(verdict([2300] * 3, known), 'KNOWN')
        self.assertEqual(verdict([2300] * 3, {}), 'PASS')
        self.assertEqual(verdict(None, {}), 'PASS')

    def test_a_never_band_needs_runs_that_reached_the_reach(self):
        # The N64 never builds a Factory within its run (to 10,676); the baseline recorded one at
        # 10,079. Runs that all ended at 9,000 have not shown the absence: KNOWN, not IMPROVED.
        never = dict(self.golden(sk.NEVER, sk.NEVER, sk.NEVER, check='build.first.Factory', subject='cpu Terran'),
                     gba={'horizon': 10676})
        key = ('build.first.Factory', 'cpu Terran')
        line = 'sk-pvt-w0:build.first.Factory:cpu Terran'
        known = {line: {'key': line, 'gba': 10079, 'gba_offsets': [10079, 10079, 10079]}}

        def verdict(cutoffs, baseline):
            more = [(1, {key: sk.NEVER}), (2, {key: sk.NEVER})]
            return sc64_parity.judge('sk-pvt-w0', never, {key: sk.NEVER}, {}, baseline, more=more,
                                     cutoffs=cutoffs)[0]['verdict']
        self.assertEqual(verdict([9000] * 3, known), 'KNOWN')
        self.assertEqual(verdict([10675] * 3, known), 'KNOWN')
        self.assertEqual(verdict([10676] * 3, known), 'IMPROVED')
        self.assertEqual(verdict([9000] * 3, {}), 'PASS')     # no line: the result check gates the end
        self.assertEqual(verdict([10676] * 3, {}), 'PASS')
        # Accepting those cut-short runs records NEVER at every offset; a Factory built later at every
        # offset is then still a change of outcome.
        cut = {line: {'key': line, 'gba': sk.NEVER, 'gba_offsets': [sk.NEVER] * 3}}
        more = [(1, {key: 8000}), (2, {key: 8001})]
        row = sc64_parity.judge('sk-pvt-w0', never, {key: 8002}, {}, cut, more=more, cutoffs=[9000] * 3)[0]
        self.assertEqual(row['verdict'], 'REGRESSION')
        self.assertIn('outcome changed', row['why'])
        mixed = sc64_parity.judge('sk-pvt-w0', never, {key: sk.NEVER}, {}, cut, more=more, cutoffs=[9000] * 3)[0]
        self.assertEqual(mixed['verdict'], 'KNOWN')

    def test_accept_keeps_values_an_early_end_left_inconclusive(self):
        never = dict(self.golden(sk.NEVER, sk.NEVER, sk.NEVER, check='build.first.Factory', subject='cpu Terran'),
                     gba={'horizon': 10676})
        key = ('build.first.Factory', 'cpu Terran')
        line = 'sk-pvt-w0:build.first.Factory:cpu Terran'
        known = {line: {'key': line, 'gba': 10079, 'gba_offsets': [10079, 10078, 10077]}}
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 'baseline.json'
            for cutoffs, spread, want in (([9000] * 3, [sk.NEVER] * 3, [10079, 10078, 10077]),
                                          ([None, 9000, None], [10070, sk.NEVER, 10071], [10070, 10078, 10071])):
                more = [(1, {key: spread[1]}), (2, {key: spread[2]})]
                row = sc64_parity.judge('sk-pvt-w0', never, {key: spread[0]}, {}, known, more=more, cutoffs=cutoffs)[0]
                self.assertEqual(row['verdict'], 'KNOWN')
                sc64_parity.write_baseline(path, [{'id': 'sk-pvt-w0', 'status': 'RUN', 'rows': [row]}], {})
                accepted = sc64_parity.load_baseline(path)
                self.assertEqual((accepted[line]['gba'], accepted[line]['gba_offsets']), (want[0], want))
                # Replaying the original timings against what was accepted stays KNOWN.
                again = sc64_parity.judge('sk-pvt-w0', never, {key: 10079}, {}, accepted,
                                          more=[(1, {key: 10078}), (2, {key: 10077})])[0]
                self.assertEqual(again['verdict'], 'KNOWN')

    def test_accept_keeps_recorded_times_behind_a_disallowed_never(self):
        wave = dict(self.golden(9539, 9537, 9541, check='attack.first_wave', subject='cpu Terran'),
                    gba={'horizon': 12000})
        key = ('attack.first_wave', 'cpu Terran')
        line = 'sk-pvt-w0:attack.first_wave:cpu Terran'
        known = {line: {'key': line, 'gba': 10079, 'gba_offsets': [10079, 10078, 10077]}}
        more = [(1, {key: sk.NEVER}), (2, {key: sk.NEVER})]
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 'baseline.json'
            row = sc64_parity.judge('sk-pvt-w0', wave, {key: sk.NEVER}, {}, known, more=more, cutoffs=[9000] * 3)[0]
            self.assertEqual(row['verdict'], 'KNOWN')
            sc64_parity.write_baseline(path, [{'id': 'sk-pvt-w0', 'status': 'RUN', 'rows': [row]}], {})
            accepted = sc64_parity.load_baseline(path)
            self.assertEqual(accepted[line]['gba_offsets'], [10079, 10078, 10077])
            # A later full-length run without the wave still fails against what was kept.
            later = sc64_parity.judge('sk-pvt-w0', wave, {key: sk.NEVER}, {}, accepted, more=more,
                                      cutoffs=[12000] * 3)[0]
            self.assertEqual(later['verdict'], 'REGRESSION')
            # Runs that ended after the recorded waves, still without one, show the wave moved: that
            # REGRESSION, once accepted, records what was seen, and the same rerun is then KNOWN.
            row = sc64_parity.judge('sk-pvt-w0', wave, {key: sk.NEVER}, {}, known, more=more, cutoffs=[10500] * 3)[0]
            self.assertEqual(row['verdict'], 'REGRESSION')
            sc64_parity.write_baseline(path, [{'id': 'sk-pvt-w0', 'status': 'RUN', 'rows': [row]}], {})
            accepted = sc64_parity.load_baseline(path)
            self.assertEqual(accepted[line]['gba_offsets'], [sk.NEVER] * 3)
            rerun = sc64_parity.judge('sk-pvt-w0', wave, {key: sk.NEVER}, {}, accepted, more=more, cutoffs=[10500] * 3)[0]
            self.assertEqual(rerun['verdict'], 'KNOWN')
            # A single run's line keeps its offset-0 value when offset 0 is cut short.
            legacy = {line: {'key': line, 'gba': 10079}}
            row = sc64_parity.judge('sk-pvt-w0', wave, {key: sk.NEVER}, {}, legacy, more=[(1, {key: 10080}), (2, {key: 10081})],
                                    cutoffs=[9000, None, None])[0]
            sc64_parity.write_baseline(path, [{'id': 'sk-pvt-w0', 'status': 'RUN', 'rows': [row]}], {})
            self.assertEqual(sc64_parity.load_baseline(path)[line]['gba_offsets'], [10079, 10080, 10081])

    def test_a_checkpoint_against_a_single_runs_line_keeps_every_offset(self):
        line = 'sk-pvt-w0:supply.cap@8000:cpu Terran'
        legacy = {line: {'key': line, 'gba': 116}}
        key = ('supply.cap@8000', 'cpu Terran')

        def verdict(spread):
            more = [(1, {key: spread[1]}), (2, {key: spread[2]})]
            return sc64_parity.judge('sk-pvt-w0', self.golden(), {key: spread[0]}, {}, legacy, more=more)[0]['verdict']
        self.assertEqual(verdict([148, 116, 116]), 'KNOWN')
        self.assertEqual(verdict([148, 140, 150]), 'REGRESSION')

    def test_a_single_runs_line_speaks_for_offset_0_only(self):
        defeat = dict(self.golden(10076, 10074, 10078, check='result.human_defeated', subject='human'),
                      gba={'horizon': 10676})
        key = ('result.human_defeated', 'human')
        line = 'sk-pvt-w0:result.human_defeated:human'
        legacy = {line: {'key': line, 'gba': sk.NEVER}}

        def verdict(spread):
            more = [(1, {key: spread[1]}), (2, {key: spread[2]})]
            return sc64_parity.judge('sk-pvt-w0', defeat, {key: spread[0]}, {}, legacy, more=more)[0]['verdict']
        self.assertEqual(verdict([sk.NEVER, 1000, 1001]), 'KNOWN')       # offsets +1, +2 were never recorded
        self.assertEqual(verdict([1000, sk.NEVER, sk.NEVER]), 'REGRESSION')
        # The whole-spread numbers comparison keeps every matching reading: against a legacy Gateway
        # line (2,239), numbers that all moved 1,200 frames earlier fail even beside an allowed NEVER.
        gate = dict(self.golden(2251, 2249, 2253, check='build.first.Gateway', subject='cpu Protoss',
                                also_ok=[sk.NEVER]), gba={'horizon': 9000})
        gkey = ('build.first.Gateway', 'cpu Protoss')
        gline = 'sk-pvt-w0:build.first.Gateway:cpu Protoss'

        def gate_verdict(spread):
            more = [(1, {gkey: spread[1]}), (2, {gkey: spread[2]})]
            return sc64_parity.judge('sk-pvt-w0', gate, {gkey: spread[0]}, {}, {gline: {'key': gline, 'gba': 2239}},
                                     more=more)[0]['verdict']
        self.assertEqual(gate_verdict([sk.NEVER, 1000, 1001]), 'REGRESSION')
        self.assertEqual(gate_verdict([1000, 1001, 1002]), 'REGRESSION')
        self.assertEqual(gate_verdict([sk.NEVER, 2238, 2240]), 'KNOWN')
        # Codex: a legacy timing of 10,079 (band ends 9,541, two frames of slack).
        wave = dict(self.golden(9539, 9537, 9541, check='attack.first_wave', subject='cpu Terran'),
                    gba={'horizon': 12000})
        wkey = ('attack.first_wave', 'cpu Terran')
        wline = 'sk-pvt-w0:attack.first_wave:cpu Terran'

        def wave_verdict(spread):
            more = [(1, {wkey: spread[1]}), (2, {wkey: spread[2]})]
            return sc64_parity.judge('sk-pvt-w0', wave, {wkey: spread[0]}, {}, {wline: {'key': wline, 'gba': 10079}},
                                     more=more)[0]['verdict']
        self.assertEqual(wave_verdict([10082, 10079, 10079]), 'KNOWN')
        self.assertEqual(wave_verdict([10082, 10082, 10082]), 'REGRESSION')

    def test_a_newly_finite_time_is_compared_with_the_outcome_it_replaced(self):
        # Codex: the only recorded time becomes NEVER and the newly finite defeats are nearer the band
        # than the NEVER they replaced (reach 12,000): not worse.
        defeat = dict(self.golden(10076, 10074, 10078, check='result.human_defeated', subject='human'),
                      gba={'horizon': 12000})
        self.assertEqual(self.verdict([sk.NEVER, 11000, 11001], [10100, sk.NEVER, sk.NEVER], defeat), 'KNOWN')
        self.assertEqual(self.verdict([sk.NEVER, 1000, 1001], [10100, sk.NEVER, sk.NEVER], defeat), 'REGRESSION')

    def test_a_checkpoint_reached_again_keeps_its_spread_verdict(self):
        # An offset whose match ended before the checkpoint now reaches it: counts are not timings.
        self.assertEqual(self.verdict([200, 116, 132], [sk.ENDED, 116, 132]), 'KNOWN')
        self.assertEqual(self.verdict([200, 150, 150], [sk.ENDED, 116, 132]), 'REGRESSION')
        # Counts keep every reading in the spread, a newly reached one included.
        self.assertEqual(self.verdict([200, sk.ENDED, sk.ENDED], [sk.ENDED, 116, 132]), 'REGRESSION')
        self.assertEqual(self.verdict([84, 150, 150], [sk.ENDED, 116, 132]), 'KNOWN')
        # Every offset now reaches it where none did: one in the band keeps it KNOWN; none in it fails.
        self.assertEqual(self.verdict([84, 118, 118], [sk.ENDED] * 3), 'KNOWN')
        self.assertEqual(self.verdict([116, 118, 118], [sk.ENDED] * 3), 'REGRESSION')

    def test_a_seed_that_never_saw_the_event_in_a_finite_envelope(self):
        # sk-ffa-w37: this seed's own band is NEVER, other seeds made the envelope finite.
        def envelope(check, subject, lo, hi):
            return dict(self.golden(sk.NEVER, lo, hi, check=check, subject=subject, also_ok=[sk.NEVER],
                                    seed_lo=sk.NEVER, seed_hi=sk.NEVER), gba={'horizon': 4000})
        wave = envelope('attack.first_wave', 'cpu Zerg', 2950, 2960)
        # Both a time and NEVER are this band's outcomes: one frame out at one offset is noise.
        self.assertEqual(self.verdict([2950, 2949, sk.NEVER], None, wave), 'KNOWN')
        # Times keep the measurement's +-2 frames of slack, not the NEVER seed band's zero width.
        defeat = envelope('result.human_defeated', 'human', 3000, 3400)
        stable = [3562] * 3
        self.assertEqual(self.verdict([3563, 3564, 3564], stable, defeat), 'KNOWN')
        self.assertEqual(self.verdict([3565, 3565, 3566], stable, defeat), 'REGRESSION')
        # A defeat that some offsets never reach is as unstable as the GBA gets: its times must also
        # clear the family's frame noise (508 frames for defeats).
        mixed = [sk.NEVER, 3562, 3562]
        self.assertEqual(self.verdict([sk.NEVER, 3570, 3570], mixed, defeat), 'KNOWN')
        self.assertEqual(self.verdict([sk.NEVER, 4100, 4100], mixed, defeat), 'REGRESSION')
        # The edge: 162 frames recorded, + 2 slack + 508 noise = 672.
        self.assertEqual(self.verdict([sk.NEVER, 3400 + 672, 3400 + 672], mixed, defeat), 'KNOWN')
        self.assertEqual(self.verdict([sk.NEVER, 3400 + 673, 3400 + 673], mixed, defeat), 'REGRESSION')
        # A newly finite time inside the band keeps the numbers KNOWN, as any in-band number does.
        exact = dict(envelope('result.human_defeated', 'human', 3198, 3202))
        self.assertEqual(self.verdict([3200, 2300, 2300], [sk.NEVER, 3562, 3562], exact), 'KNOWN')
        # ... but not a second newly finite time far beyond the noise.
        self.assertEqual(self.verdict([3200, 1000, sk.NEVER], [sk.NEVER, sk.NEVER, sk.NEVER], exact), 'REGRESSION')

    def test_a_value_missing_at_one_offset_fails(self):
        key = ('supply.cap@8000', 'cpu Terran')
        row = sc64_parity.judge('sk-pvt-w0', self.golden(), {key: 116}, {}, {}, more=[(1, {key: 120}), (2, {})])[0]
        self.assertEqual(row['verdict'], 'REGRESSION')
        self.assertIn('frame offset +2', row['why'])
        # --accept refuses it, rather than recording offset 0 alone.
        self.assertEqual(row['gba_offsets'], [116, 120, None])
        # Offset 0 missing: the spread is kept and names the offset.
        first = sc64_parity.judge('sk-pvt-w0', self.golden(), {}, {}, {}, more=[(1, {key: 120}), (2, {key: 116})])[0]
        self.assertEqual((first['verdict'], first['gba_offsets']), ('REGRESSION', [None, 120, 116]))
        self.assertIn('frame offset +0', first['why'])
        with tempfile.TemporaryDirectory() as temp:
            for bad in (row, first):
                with self.assertRaises(ValueError):
                    sc64_parity.write_baseline(pathlib.Path(temp) / 'b.json', [{'id': 'sk-pvt-w0', 'status': 'RUN',
                                                                                'rows': [bad]}], {})

    def test_baseline_lines_keep_every_offset(self):
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 'baseline.json'
            results = [{'id': 'sk-pvt-w0', 'status': 'RUN', 'rows': [
                {'key': self.KEY, 'gba': 116, 'gba_offsets': [116, 132, 116], 'gating': True, 'verdict': 'KNOWN'}]}]
            sc64_parity.write_baseline(path, results, {})
            self.assertEqual(json.loads(path.read_text())['known'],
                             [{'key': self.KEY, 'gba': 116, 'gba_offsets': [116, 132, 116]}])
            self.assertIn('"gba_offsets": [116, 132, 116]\n', path.read_text())   # one line per entry's offsets
            self.assertEqual(sc64_parity.load_baseline(path)[self.KEY]['gba_offsets'], [116, 132, 116])
            for bad in ([132, 116, 116], [], [116, 'x'], 116, None, [116, 132], [116, 132, 116, 116],
                        [116, 132, float('inf')], [116, True, 116]):
                path.write_text(json.dumps({'known': [{'key': self.KEY, 'gba': 116, 'gba_offsets': bad}]}))
                with self.assertRaises(SystemExit):
                    sc64_parity.load_baseline(path)
            # A value past the float range parses as infinity: refused too, for gba itself as well.
            for text in ('{"known": [{"key": "%s", "gba": 116, "gba_offsets": [116, 132, 1e309]}]}' % self.KEY,
                         '{"known": [{"key": "%s", "gba": 1e309}]}' % self.KEY,
                         '{"known": [{"key": "bb-train:x:s", "gba": 1, "gba_offsets": [1]}]}',
                         '{"known": [{"key": "%s", "gba": 116, "gba_offsets": [116, 132, 1%s]}]}' % (self.KEY, '0' * 309)):
                path.write_text(text)
                with self.assertRaises(SystemExit):
                    sc64_parity.load_baseline(path)
            # A line of a scenario this checkout does not know is kept as it is.
            path.write_text(json.dumps({'known': [{'key': 'gone-lab:x:s', 'gba': 1, 'gba_offsets': [1, 2]}]}))
            self.assertEqual(sc64_parity.load_baseline(path)['gone-lab:x:s']['gba_offsets'], [1, 2])
            # Finite but absurd values would overflow a spread (and the noise allowance): refused.
            path.write_text(json.dumps({'known': [{'key': self.KEY, 'gba': 1e308, 'gba_offsets': [1e308, -1e308, 1e308]}]}))
            with self.assertRaises(SystemExit):
                sc64_parity.load_baseline(path)
            colon = 'sk-pvt-w0:x.a:cpu Terran: west'   # a subject may hold a colon
            path.write_text(json.dumps({'known': [{'key': colon, 'gba': 1}]}))
            self.assertIn(colon, sc64_parity.load_baseline(path))
            for bad_key in ('sk-zvz-w0', 'sk-pvt-w0::cpu Terran', ':x:s', 'sk-pvt-w0:x:'):   # not scenario:check:subject
                path.write_text(json.dumps({'known': [{'key': bad_key, 'gba': 1}]}))
                with self.assertRaises(SystemExit):
                    sc64_parity.load_baseline(path)
            path.write_text(json.dumps({'known': [{'key': 'gone-lab:x:s', 'gba': 1, 'gba_offsets': 1}]}))
            with self.assertRaises(SystemExit):
                sc64_parity.load_baseline(path)
            # A single run's line (no gba_offsets) stays valid for a scenario with offsets.
            path.write_text(json.dumps({'known': [{'key': self.KEY, 'gba': 116}]}))
            self.assertEqual(sc64_parity.load_baseline(path)[self.KEY], {'key': self.KEY, 'gba': 116})
            # Only the offsets are compacted: other text in a line (a note) is written as it is.
            note = {'key': 'demo:x:s', 'gba': 1, 'note': 'see [ 1,2 ] and [\n 3 ]'}
            sc64_parity.write_baseline(path, [], {'demo:x:s': note})
            self.assertEqual(sc64_parity.load_baseline(path)['demo:x:s'], note)

    def test_a_scenario_runs_once_per_offset(self):
        key = ('supply.cap@8000', 'cpu Terran')
        calls = []

        def fake(scenario_id, golden, keep, frame_offset=0):
            calls.append(frame_offset)
            if frame_offset == 2 and fail:
                return {'status': 'ERROR', 'why': 'GBA run exited 3: x'}
            return {'measured': {'values': {key: 116 + 16 * (frame_offset == 1)}}, 'err': None,
                    'report': {'cpu_seconds': 1.0}, 'cmd': ['runner', str(frame_offset)]}
        original = sc64_parity.run_gba
        sc64_parity.run_gba = fake
        try:
            with tempfile.TemporaryDirectory() as temp:
                path = pathlib.Path(temp) / 'sk-pvt-w0.json'
                layout = sc64_parity.parity_decode.Layout().sha256   # judged as current, never stale
                path.write_text(json.dumps(dict(self.golden(), gba={'at_step': 5}, layout_sha256=layout)))
                fail = False
                out = sc64_parity.run_scenario('sk-pvt-w0', path, {}, None)
                self.assertEqual((calls, out['frame_offsets'], out['gba_cpu_seconds']), ([0, 1, 2], [0, 1, 2], 3.0))
                self.assertEqual((out['rows'][0]['gba_offsets'], out['command']), ([116, 132, 116], ['runner', '0']))
                self.assertEqual([(r['frame_offset'], r['command'], r['gba_cpu_seconds']) for r in out['offset_runs']],
                                 [(0, ['runner', '0'], 1.0), (1, ['runner', '1'], 1.0), (2, ['runner', '2'], 1.0)])
                fail = True
                out = sc64_parity.run_scenario('sk-pvt-w0', path, {}, None)
                self.assertEqual(out['status'], 'ERROR')
                self.assertTrue(out['why'].endswith('(frame offset +2)'))
        finally:
            sc64_parity.run_gba = original

    def test_a_worker_failure_is_this_scenarios_error(self):
        import concurrent.futures.process

        class Pool:
            def __init__(self, error):
                self.error = error

            def submit(self, fn, scenario_id, golden, keep, k):
                job = concurrent.futures.Future()
                if k == 1:
                    job.set_exception(self.error)
                else:
                    job.set_result({'measured': {'values': {}}, 'err': None, 'report': {}, 'cmd': []})
                return job

        class Broken:
            def submit(self, *args):
                raise concurrent.futures.process.BrokenProcessPool('gone')

        def run(pool):
            sc64_parity.RUN_POOL = pool
            try:
                return sc64_parity.run_scenario('sk-pvt-w0', path, {}, None)
            finally:
                sc64_parity.RUN_POOL = None
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / 'sk-pvt-w0.json'
            path.write_text(json.dumps(dict(self.golden(), gba={'at_step': 5})))
            # A run that raised in its worker: that offset's error.
            out = run(Pool(ValueError('bad trace')))
            self.assertEqual(out['status'], 'ERROR')
            self.assertIn('bad trace', out['why'])
            self.assertTrue(out['why'].endswith('(frame offset +1)'))
            # A worker that died breaks every pending run: the pool, not the offset, is named.
            out = run(Pool(concurrent.futures.process.BrokenProcessPool('worker died')))
            self.assertEqual(out['status'], 'ERROR')
            self.assertIn('worker pool broke during this scenario', out['why'])
            out = run(Broken())
            self.assertEqual(out['status'], 'ERROR')
            self.assertIn('while this scenario submitted its runs', out['why'])


class Bundles(unittest.TestCase):
    """Skirmish goldens read watched words (supply, result) outside the hash chain, so both
    captures' watch.csv must match; whole-match captures may keep their tap compressed."""

    def capture(self, root, name):
        import test_sc64_parity_harness as harness
        run = pathlib.Path(root) / name
        harness.write_capture(run, harness.marine_frames(30))
        return sc64_parity.parity_decode.Capture(run)

    def test_watch_logs_compared_only_when_measured(self):
        same_bundle = sc64_parity.parity_decode.same_bundle
        with tempfile.TemporaryDirectory() as d:
            a, b = self.capture(d, 'a'), self.capture(d, 'b')
            b.manifest['outputs_sha256']['watch.csv'] = 'x'
            self.assertFalse([p for p in same_bundle(a, b) if 'watch' in p])
            self.assertIn('watch.csv differs', same_bundle(a, b, same_watch=True))
            # A pair that never recorded a watch log regenerates unless the scenario reads one.
            del b.manifest['outputs_sha256']['watch.csv']
            self.assertFalse([p for p in same_bundle(a, b) if 'watch' in p])
            self.assertTrue(any('no recorded watch.csv' in p for p in same_bundle(a, b, same_watch=True)))
            self.assertTrue(metrics.SCENARIOS['sk-zvt-w0']['same_watch'])
            # Trailing change reads (the run's end) lie outside the chain as well.
            self.assertFalse([p for p in same_bundle(a, b, same_watch=True) if 'after the last follow-up' in p])
            b.trailing_changes = a.trailing_changes + [(999, False, True)]
            self.assertIn('the change reads after the last follow-up differ between the runs',
                          same_bundle(a, b, same_watch=True))
            b.trailing_changes = a.trailing_changes
            b.change_tables = a.change_tables[:-1] + [not a.change_tables[-1]]
            for pair in ((a, b), (b, a)):
                self.assertIn("the change reads' unit-table flags differ between the runs",
                              same_bundle(*pair, same_watch=True))
            # The same flags on other reads (another step) differ too.
            b.change_tables = a.change_tables
            b.change_reads = [(step + 1, tick) for step, tick in a.change_reads]
            self.assertIn("the change reads' unit-table flags differ between the runs", same_bundle(a, b, same_watch=True))

    def test_a_lost_first_tick_is_refused(self):
        import hashlib as _hashlib
        decode = sc64_parity.parity_decode
        with tempfile.TemporaryDirectory() as d:
            cap = self.capture(d, 'a')
            self.assertEqual(cap.first_table_tick, next(r for r in cap.ticks if r['table'])['tick'])
            data = (cap.run / 'tap.bin').read_bytes()
            out, at = bytearray(data[:24]), 24
            for head, glob, rows in decode.read_tap(cap.run / 'tap.bin'):
                size = 48 + len(glob) + len(rows) * 314
                if not (head['phase'] == 1 and head['tick'] == 1):
                    out += data[at:at + size]
                at += size
            (cap.run / 'tap.bin').write_bytes(bytes(out))
            manifest = json.loads((cap.run / 'manifest.json').read_text())
            manifest['outputs_sha256']['tap.bin'] = _hashlib.sha256(bytes(out)).hexdigest()
            (cap.run / 'manifest.json').write_text(json.dumps(manifest))
            lost = decode.Capture(cap.run)
            with self.assertRaises(SystemExit):
                metrics.SCENARIOS['sk-zvt-w0']['measure_n64'](lost, {})

    @unittest.skipUnless(HAVE_ZSTD, 'compression.zstd needs Python 3.14')
    def test_a_compressed_tap_decodes_like_the_plain_one(self):
        from compression import zstd
        with tempfile.TemporaryDirectory() as d:
            plain = self.capture(d, 'a')
            tap = plain.run / 'tap.bin'
            (plain.run / 'tap.bin.zst').write_bytes(zstd.compress(tap.read_bytes()))
            tap.unlink()
            packed = sc64_parity.parity_decode.Capture(plain.run)
            self.assertEqual(packed.verified, [])
            self.assertEqual(packed.chain, plain.chain)

    @unittest.skipUnless(HAVE_ZSTD, 'compression.zstd needs Python 3.14')
    def test_compressed_tap_respects_the_memory_limit(self):
        from compression import zstd
        decode = sc64_parity.parity_decode
        with tempfile.TemporaryDirectory() as d:
            tap = pathlib.Path(d) / 'tap.bin'
            # Two frames, the second of unknown size: the limit holds for the whole stream.
            (pathlib.Path(d) / 'tap.bin.zst').write_bytes(zstd.compress(b'x' * 10) + zstd.compress(b'y' * 4990))
            self.assertEqual(decode.tap_bytes(tap, 5000), b'x' * 10 + b'y' * 4990)
            with self.assertRaises(ValueError):
                decode.tap_bytes(tap, 4999)

if __name__ == '__main__':
    unittest.main()
