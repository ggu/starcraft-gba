# SPDX-License-Identifier: GPL-3.0-or-later
"""Physical-button guest checkpoint; invoke only under the shared native lock.

The journal reader is a separate offline process. Snapshots cover named
persisted world fields, not framebuffer pixels or all transient state.
"""
import hashlib
import json
from pathlib import Path
import signal
import subprocess
import sys

import sc64_egg_custom_tape as tape
import sc64_skirmish_sram_continuation as world


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def check_state(state, info, active):
    if state['screen'] not in (world.SCREEN_PLAY, world.SCREEN_PAUSE):
        raise ValueError('guest left the match')
    if (state['race_mode'], state['race_human'], state['race_bots'], state['race_skirmish_map']) != (1, 1, 1, 0):
        raise ValueError('unexpected custom controller')
    if not (0 <= info['supply'] <= info['supply_cap']):
        raise ValueError('invalid supply')
    unit_fields = world.UNIT_FIELDS
    owner, typ, target, timer = (1 + unit_fields.index(x) for x in ('owner', 'type', 'target', 'work_timer'))
    eggs = [u for u in state['units'] if u[owner] == info['side'] and u[typ] == info['egg_type']]
    zerglings = [u for u in state['units'] if u[owner] == info['side'] and u[typ] == info['zergling_type']]
    if active:
        if len(eggs) != 1 or zerglings or eggs[0][0] != info['egg_slot'] or eggs[0][target] != info['zergling_type'] or eggs[0][timer] != info['egg_timer']:
            raise ValueError('active Egg differs from authenticated journal')
    elif eggs or len(zerglings) != 2:
        raise ValueError('expected exactly two friendly Zerglings and no Egg')


def comparable(state):
    return {k: v for k, v in state.items() if k != 'screen'}


def phase_timeout(_signum, _frame):
    raise TimeoutError('physical guest phase exceeded 600 seconds')


def run(trace, rom, elf, runner, reader, erased, out):
    pins = {str(p.resolve()): digest(p) for p in (trace, rom, elf, runner, reader, erased)}
    if erased.read_bytes() != b'\xff' * 32768:
        raise ValueError('virgin SRAM is not erased')
    plan = tape.validate(json.loads(trace.read_text()))
    cut, end, edges = plan['save_tick'], plan['birth_tick'], plan['transitions']
    if [(t, k) for t, k in edges if cut-4 <= t < cut+2] != [(cut-4,8),(cut-3,0),(cut-2,2),(cut-1,0),(cut,8),(cut+1,0)]:
        raise ValueError('unexpected pause/save/resume sequence')
    out.mkdir(parents=True, exist_ok=False)
    save = out / 'guest.sav'
    old_handler = signal.signal(signal.SIGALRM, phase_timeout)
    signal.alarm(600)
    game = world.PhysicalGame(runner, rom, elf, out / 'prefix', save_input=erased, save_output=save)
    try:
        game.rsp.play_to(cut, edges)
        saved = game.state()
        if saved['screen'] != world.SCREEN_PAUSE:
            raise ValueError('guest did not save from pause')
        game.rsp.export_sram()
        result = subprocess.run([str(reader), str(save)], capture_output=True, text=True, timeout=30, check=True)
        info = json.loads(result.stdout)
        if saved['frame'] != info['frame'] or game.u32('won') or game.u32('lost'):
            raise ValueError('saved frame or game outcome differs')
        check_state(saved, info, True)
        game.rsp.play_to(end, edges)
        reference = game.state()
        if game.u32('won') or game.u32('lost'):
            raise ValueError('uninterrupted game ended')
        check_state(reference, info, False)
    finally:
        game.close()
        signal.alarm(0)
    saved_sha = digest(save)
    signal.alarm(600)
    resumed = world.PhysicalGame(runner, rom, elf, out / 'resume', save_input=save)
    try:
        resumed.step(512)
        restored = resumed.state()
        if restored['screen'] != world.SCREEN_PLAY or comparable(restored) != comparable(saved):
            raise ValueError('fresh guest load differs from saved semantic state')
        check_state(restored, info, True)
        continuation = [(0,512)] + [(t-cut, k) for t,k in edges if t > cut]
        resumed.rsp.play_to(end-cut, continuation)
        final = resumed.state()
        if resumed.u32('won') or resumed.u32('lost'):
            raise ValueError('resumed game ended')
        check_state(final, info, False)
        if comparable(final) != comparable(reference):
            raise ValueError('fresh guest continuation differs from uninterrupted reference')
    finally:
        resumed.close()
        signal.alarm(0)
        signal.signal(signal.SIGALRM, old_handler)
    if digest(save) != saved_sha or pins != {str(p.resolve()): digest(p) for p in (trace,rom,elf,runner,reader,erased)}:
        raise ValueError('frozen input or guest image changed')
    report = {'inputs': pins, 'save_sha256': saved_sha, 'journal': info,
              'saved': world.summary(saved), 'reference': world.summary(reference),
              'restored': world.summary(restored), 'resumed': world.summary(final),
              'scope': 'physical buttons and named persisted-world fields; no pixel comparison'}
    (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    if len(sys.argv) != 8:
        raise SystemExit('usage: sc64_egg_custom_physical.py TRACE ROM ELF RUNNER READER ERASED OUTPUT')
    run(*(Path(x) for x in sys.argv[1:]))
