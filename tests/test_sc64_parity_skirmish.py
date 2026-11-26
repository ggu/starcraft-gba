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

        def defeat(lo, hi, value=2, extra=()):
            golden = {'bundle': {'tape_sha256': tape}, 'gba': {'horizon': 2},
                      'observations': [{'check': 'result.human_defeated', 'subject': 'human', 'value': value,
                                        'lo': lo, 'hi': hi, 'gating': True},
                                       {'check': 'workers@1000', 'subject': 'human', 'value': 1, 'lo': 1, 'hi': 1,
                                        'gating': True}, *extra]}
            import sc64_parity_selfgate as selfgate
            check, selfgate.check_trace_complete = selfgate.check_trace_complete, lambda *a, **k: None
            try:
                values, error = metrics.SCENARIOS['sk-zvt-w0']['measure_gba'](
                    sk.gba_series(self.trace, REPORT), golden, REPORT)
            finally:
                selfgate.check_trace_complete = check
            self.assertIsNone(error)
            return values['values'][('result.human_defeated', 'human')]
        self.assertEqual(defeat(0, 4), 3)
        self.assertEqual(defeat(0, 2), sk.NEVER)            # past both the run and the band
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
            # Interior change reads: the mission starts at the first tick with a table in either
            # read (here tick 1, found by its follow-up); a later read without one is refused.
            cap.first_table_tick = 1
            cap.change_reads = [(2 * k + 2, k + 1) for k in range(15)]
            cap.change_tables = [False] + [True] * 14
            self.assertEqual(sk.tick_frames(cap)[1], 10)
            cap.change_tables = [False, False] + [True] * 13
            with self.assertRaises(ValueError):
                sk.tick_frames(cap)
            cap.change_tables = [True] * 15
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
