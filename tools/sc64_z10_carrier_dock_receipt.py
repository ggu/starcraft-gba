#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay the Z10 probe's physical withdrawal, docking, and save."""
import argparse
import fcntl
import hashlib
import resource
import shutil
import signal
import struct
from pathlib import Path

from sc64_skirmish_sram_continuation import PhysicalGame, UNIT_FIELDS

PINS = {
    'runner': '4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549',
    'rom': 'f4145128abbf64a1ef710baeea4278bf45debeeadd3196d2df33678dc5e5b47c',
    'elf': '57f0f83975e1cf78c68f5e23250225e00751ced4f2d5b0cbfcbd223af56daff6',
    'seed': '63fc427aedd20c6bf92b930ed294d15de4bd091fed5dc1d39e69c5b078878a74',
}
WITHDRAW = [(0, 512), (1, 2), (2, 0), (3, 1), (4, 0), (5, 288), (13, 384),
            (19, 128), (20, 0), (21, 1), (22, 0), (38, 8), (39, 0), (40, 2)]
RESAVE = [(0, 512), (1, 8), (2, 0), (3, 2)]
TAG = 0x564E5A61


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def payload(path, length, sequence):
    raw = path.read_bytes()
    bank = next(off for off in (0, 16384)
                if struct.unpack_from('<IIII', raw, off)[::3] == (TAG, sequence))
    assert struct.unpack_from('<I', raw, bank + 4)[0] == length
    return raw[bank + 16:bank + 16 + length]


def read(game):
    state = game.state()
    return state, {v[0]: dict(zip(UNIT_FIELDS, v[1:])) for v in state['units']}


def check_docked(game, screen):
    state, actors = read(game)
    assert (state['frame'], state['screen'], state['minerals'], state['gas']) == (12881, screen, 2301, 520)
    child = actors[12]
    assert tuple(child[k] for k in ('type', 'owner', 'build', 'flags', 'order',
                                     'target', 'hp', 'shield', 'carry', 'work_timer')) == (
                                         49, 0, 5, 512, 0, -1, 40, 40, 0, 0)
    assert (game.u32('supply_used'), game.u32('supply_cap')) == (22, 34)
    return state


def timed_out(*_):
    raise TimeoutError('bounded Z10 Carrier dock receipt')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for k in ('runner', 'rom', 'elf', 'seed', 'output'):
        p.add_argument('--' + k, required=True, type=Path)
    args = p.parse_args()
    for k in ('runner', 'seed'):
        assert sha(getattr(args, k)) == PINS[k], k
    assert (sha(args.rom), sha(args.elf)) in {
        (PINS['rom'], PINS['elf']),
        ('2719456571efc210638a832bcca8b069078a39475075e97a4193e63df66b58ed',
         '5d22af38b5091d61af2c65afa7910f9d8d78245c2dfac49b4f6b1f0dbae8daa2'),
    }
    assert not args.output.exists() and args.output.parent.is_dir()
    assert shutil.disk_usage(args.output.parent).free > 5 * 1024**3
    resource.setrlimit(resource.RLIMIT_FSIZE, (8 * 1024**2,) * 2)
    signal.signal(signal.SIGALRM, timed_out)
    with open('/tmp/sc64-native-build-20261031.lock', 'a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        signal.alarm(75)
        args.output.mkdir()
        docked = args.output / 'docked.sav'
        fresh = args.output / 'resaved.sav'
        game = PhysicalGame(args.runner, args.rom, args.elf, args.output / 'guest',
                            save_input=args.seed, save_output=docked)
        try:
            game.rsp.play_to(1, WITHDRAW)
            state, actors = read(game)
            assert (state['frame'], actors[12]['flags'] >> 8, actors[12]['hp']) == (12847, 3, 40)
            game.rsp.play_to(23, WITHDRAW)
            state, actors = read(game)
            assert state['frame'] == 12866
            assert tuple(actors[5][k] for k in ('type', 'order', 'tx', 'ty')) == (45, 1, 101, 134)
            game.rsp.play_to(38, WITHDRAW)
            check_docked(game, 1)
            game.rsp.play_to(41, WITHDRAW)
            saved = check_docked(game, 2)
            game.rsp.export_sram()
        finally:
            game.close()
        game = PhysicalGame(args.runner, args.rom, args.elf, args.output / 'fresh',
                            save_input=docked, save_output=fresh)
        try:
            game.rsp.play_to(1, RESAVE)
            restored = check_docked(game, 1)
            assert all(saved[k] == restored[k] for k in ('units', 'buildings'))
            game.rsp.play_to(4, RESAVE)
            game.rsp.export_sram()
        finally:
            game.close()
        assert sha(docked) == 'fb0ff07012fd7361be5ba4251e93da1f1293d2fa3b6dfe579bd0fa2f19f51438'
        assert sha(fresh) == '57fe342476226bac3a236fec7ab394758c51e8f5dfe9c9623730ce8117821fea'
        body = payload(docked, 14081, 14)
        assert body == payload(fresh, 14081, 15)
        assert struct.unpack_from('<HHBB', body, 14050) == (11, 16, 1, 0)
        signal.alarm(0)
    print('Verified Z10 withdrawal, docked Interceptor, and exact fresh title-L journal payload.')


if __name__ == '__main__':
    main()
