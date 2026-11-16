#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay the Terran probe's physical withdrawal, docking, and save."""
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
    'rom': 'eedcfd0e8fab20e566c8089f730fdc0986f73096c6c5410e95476deddfaeffdc',
    'elf': '7f4dc3c0f1ff21ff4100c18f543dd0d9a59bb8da17ffdc8ebdd3bf90e81f1142',
    'seed': '47ca82b628d288b52f05e07615a2598d30817cdde25dd9b72a08eeb93b4ea072',
}
WITHDRAW = [(0, 512), (1, 2), (2, 0), (3, 1), (4, 0), (5, 288), (44, 32),
            (45, 320), (55, 0), (56, 1), (57, 0), (85, 8), (86, 0), (87, 2)]
RESAVE = [(0, 512), (1, 8), (2, 0), (3, 2)]
TAG = 0x764D5442


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
    assert (state['frame'], state['screen'], state['minerals'], state['gas']) == (12972, screen, 2341, 504)
    child = actors[12]
    assert tuple(child[k] for k in ('type', 'owner', 'build', 'flags', 'order',
                                     'target', 'hp', 'shield', 'carry', 'work_timer')) == (
                                         45, 0, 5, 512, 0, -1, 40, 40, 0, 0)
    assert (game.u32('supply_used'), game.u32('supply_cap')) == (20, 34)
    return state


def timed_out(*_):
    raise TimeoutError('bounded Terran Carrier dock receipt')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for k in ('runner', 'rom', 'elf', 'seed', 'output'):
        p.add_argument('--' + k, required=True, type=Path)
    args = p.parse_args()
    for k, expected in PINS.items():
        assert sha(getattr(args, k)) == expected, k
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
            assert (state['frame'], actors[12]['flags'] >> 8, actors[12]['hp']) == (12891, 3, 35)
            game.rsp.play_to(58, WITHDRAW)
            state, actors = read(game)
            assert state['frame'] == 12945
            assert tuple(actors[5][k] for k in ('type', 'order', 'tx', 'ty')) == (44, 1, 101, 134)
            game.rsp.play_to(85, WITHDRAW)
            check_docked(game, 1)
            game.rsp.play_to(88, WITHDRAW)
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
        assert sha(docked) == '895462077f5e3a1a8492b9e42e72ee19f53e74b08af8ef2458dc6f77781c5b34'
        assert sha(fresh) == '3923933081e9ae6337d03e4ee66855ff804dff140ab8f98953d95002ed173045'
        body = payload(docked, 13685, 10)
        assert body == payload(fresh, 13685, 11)
        assert struct.unpack_from('<HHBB', body, 13654) == (11, 16, 1, 0)
        signal.alarm(0)
    print('Verified Terran withdrawal, docked Interceptor, and exact fresh title-L journal payload.')


if __name__ == '__main__':
    main()
