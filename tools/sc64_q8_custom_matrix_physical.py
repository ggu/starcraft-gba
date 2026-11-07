#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded ordinary-button custom Flash matrix; experimental, no guest writes."""
import argparse
import fcntl
import hashlib
import itertools
import json
import os
from pathlib import Path
import resource
import shutil
import signal
import subprocess
import sys
import time
import traceback

import sc64_headless_flash_physical as flash
import sc64_skirmish_sram_continuation as world

LOCK = '/tmp/sc64-native-build-20261031.lock'
RUNNER = '75b8ba5f71919802a981516ffef05baf29866256f97b7f67f46276171d9690e2'
COMPILED_HEAD = 'd7e7554e513bb83e21fbe4e47afa8d64162d3260'
RESERVE, LIMIT = 4 * 1024**3, 64 * 1024**2
BOT_OWNERS = (1, 3, 4)


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def scenarios():
    return [(m, human, bots) for m in range(2) for human in range(3)
            for count in range(1, 4) for bots in itertools.product(range(3), repeat=count)]


def flash_record(data):
    if len(data) != 131072 or data[:32768] != b'\xff' * 32768:
        raise ValueError('Flash size or legacy area invalid')
    valid = []
    for base in (32768, 65536):
        commit = int.from_bytes(data[base:base+4], 'little')
        if commit == 0xffffffff: continue
        meta = base + 4096
        tag, length, checksum, sequence = (int.from_bytes(data[meta+i:meta+i+4], 'little') for i in (0,4,8,12))
        if tag != commit or ((tag >> 16) & 255) != 0x48 or not 1 <= length <= 28656 or sequence != 1:
            raise ValueError('invalid committed Flash header')
        value = 2166136261
        for byte in data[meta+16:meta+16+length]: value = ((value ^ byte) * 16777619) & 0xffffffff
        if value != checksum: raise ValueError('Flash checksum mismatch')
        valid.append({'base':base, 'tag':tag, 'length':length, 'sequence':sequence, 'checksum':checksum})
    if len(valid) != 1: raise ValueError('expected exactly one fresh Flash record')
    return valid[0]


def read_array(game, name, count):
    if game.sizes.get(name) != count * 4: raise ValueError('unexpected array ABI: ' + name)
    raw = game.rsp.read(game.sym[name], count * 4)
    return [int.from_bytes(raw[i:i+4], 'little', signed=True) for i in range(0, len(raw), 4)]


def snapshot(game, case):
    state = game.state()
    for name, size in (('sc64_pr_hp_debt', 400), ('sc64_experimental_research_levels', 48)):
        if game.sizes.get(name) != size: raise ValueError('unexpected Q8 ABI: ' + name)
        state[name] = game.rsp.read(game.sym[name], size).hex()
    state['players'] = read_array(game, 'race_player', 5)
    state['bot_races'] = read_array(game, 'race_bot_races', 3)
    state['won'] = game.u32('won'); state['lost'] = game.u32('lost')
    m, human, bots = case
    if (state['race_mode'], state['race_skirmish_map'], state['race_human'], state['race_bots']) != (1, m, human, len(bots)):
        raise ValueError('guest context differs from requested roster')
    if state['players'][0] != human or state['bot_races'][:len(bots)] != list(bots) or any(
            state['players'][BOT_OWNERS[i]] != race for i, race in enumerate(bots)):
        raise ValueError('guest current-controller roster differs')
    return state


def comparable(state): return {k:v for k,v in state.items() if k != 'screen'}


def validate_receipt(receipt, rom, elf):
    if receipt.get('head')!=COMPILED_HEAD or receipt.get('unchanged') is not True:
        raise ValueError('unqualified compiled source receipt')
    rows=receipt.get('results',[])
    if len(rows)!=4 or {row.get('name') for row in rows if row.get('rc')==0} != {'arm-p10','rom','header','boundary'}:
        raise ValueError('P10 build or boundary receipt incomplete')
    product=receipt.get('products',{}).get('p10',{})
    for kind,path in (('rom',rom),('elf',elf)):
        if product.get(kind)!={'path':str(path.resolve()),'sha256':sha(path)}:
            raise ValueError('P10 artifact role differs from receipt')


def guard(out):
    if shutil.disk_usage(out).free < RESERVE: raise ValueError('disk reserve')
    if sum(p.stat().st_size for p in out.rglob('*') if p.is_file() and not p.is_symlink()) > LIMIT:
        raise ValueError('aggregate output threshold')


def run_case(index, case, args, out):
    folder = out / f'case-{index:03d}'; folder.mkdir()
    image = folder / 'guest.sav'
    progress = {'index': index, 'map':case[0], 'human':case[1], 'bots':case[2], 'phase':'start'}
    def record(phase, value=None):
        progress['phase'] = phase
        if value is not None: progress[phase] = value
        (folder / 'progress.json').write_text(json.dumps(progress, indent=2) + '\n')
        guard(out)
    first = None
    try:
        first = world.PhysicalGame(args.runner,args.rom,args.elf,folder/'first',save_input=args.blank,save_output=image)
        first.launch(case[0],case[1],list(case[2])); start = snapshot(first,case); record('start',start)
        first.step(0,120); before_pause = snapshot(first,case)
        record('before_pause',before_pause)
        if before_pause['screen'] != world.SCREEN_PLAY or before_pause['won'] or before_pause['lost'] or before_pause['frame'] != start['frame'] + 120:
            raise ValueError('120 advancing play updates not reached')
        first.tap(world.KEY_START)
        if first.u32('screen') != world.SCREEN_PAUSE: raise ValueError('pause not reached')
        first.tap(world.KEY_B); saved = snapshot(first,case); record('saved',saved)
        first.rsp.export_sram(); saved_hash = sha(image); journal = flash_record(image.read_bytes())
        first.step(world.KEY_A); first.step(0,60); reference = snapshot(first,case)
        if reference['screen'] != world.SCREEN_PLAY or reference['frame'] != saved['frame']+60 or reference['won'] or reference['lost']:
            raise ValueError('uninterrupted 60-update reference failed')
        record('reference',reference)
    except BaseException as exc:
        progress['error']=repr(exc)
        if first is not None:
            try: progress['failure_world']=first.state()
            except Exception as state_exc: progress['failure_world_error']=repr(state_exc)
        (folder/'failure.json').write_text(json.dumps(progress,indent=2)+'\n')
        raise
    finally:
        if first is not None: first.close()
    second = None
    try:
        second = world.PhysicalGame(args.runner,args.rom,args.elf,folder/'fresh',save_input=image)
        second.step(world.KEY_L); restored = snapshot(second,case); record('restored',restored)
        if restored['screen'] != world.SCREEN_PLAY or comparable(restored) != comparable(saved):
            raise ValueError('fresh load differs from saved named world before release')
        second.step(0,60); resumed = snapshot(second,case); record('resumed',resumed)
        if resumed['screen'] != world.SCREEN_PLAY or resumed['frame'] != restored['frame']+60 or comparable(resumed) != comparable(reference):
            raise ValueError('fresh continuation differs from uninterrupted reference')
    except BaseException as exc:
        progress['error']=repr(exc)
        if second is not None:
            try: progress['failure_world']=second.state()
            except Exception as state_exc: progress['failure_world_error']=repr(state_exc)
        (folder/'failure.json').write_text(json.dumps(progress,indent=2)+'\n')
        raise
    finally:
        if second is not None: second.close()
    if sha(image) != saved_hash: raise ValueError('exported guest Flash image changed')
    record('pass',{'guest_sha256':saved_hash, 'journal':journal})
    return progress


def timeout(_sig,_frame): raise TimeoutError('custom Flash batch deadline')


def worker_exited(pid):
    return os.waitid(os.P_PID,pid,os.WEXITED|os.WNOHANG|os.WNOWAIT) is not None


def signal_group(pid, sig):
    try: os.killpg(pid,sig)
    except ProcessLookupError:
        try: os.kill(pid,sig)  # Child may not yet have completed setsid().
        except ProcessLookupError: pass


def terminate_and_reap(pid):
    # Retain the leader until the whole owned group has been signaled, so
    # its PID/PGID cannot be recycled between observation and cleanup.
    signal_group(pid,signal.SIGTERM)
    deadline=time.monotonic()+3
    while not worker_exited(pid) and time.monotonic()<deadline: time.sleep(.05)
    signal_group(pid,signal.SIGKILL)
    return os.waitpid(pid,0)[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('rom','elf','runner','blank','receipt','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--driver-head',required=True)
    parser.add_argument('--first',type=int,required=True)
    parser.add_argument('--count',type=int,required=True)
    parser.add_argument('--deadline',type=int,default=600)
    args=parser.parse_args(); cases=scenarios()
    if len(cases)!=234 or not 0<=args.first<len(cases) or not 1<=args.count<=12 or args.first+args.count>len(cases) or not 1<=args.deadline<=1200:
        parser.error('invalid batch bounds')
    if args.output.exists() or not args.output.parent.is_dir(): parser.error('output must be new with existing parent')
    inputs=(args.rom,args.elf,args.runner,args.blank,args.receipt)
    pins={str(p.resolve()):sha(p) for p in inputs}
    if sha(args.runner)!=RUNNER or args.blank.read_bytes()!=b'\xff'*131072:
        raise ValueError('runner or virgin Flash differs')
    receipt=json.loads(args.receipt.read_text())
    validate_receipt(receipt,args.rom,args.elf)
    def check():
        if subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()!=args.driver_head:
            raise ValueError('driver HEAD changed')
        if subprocess.check_output(['git','status','--porcelain'],text=True).strip():
            raise ValueError('driver tree not clean')
        diff=set(subprocess.check_output(['git','diff','--name-only',COMPILED_HEAD,'HEAD','--'],text=True).splitlines())
        if not diff.issubset({'tools/sc64_q8_custom_matrix_physical.py','tests/test_sc64_q8_custom_matrix_physical.py'}):
            raise ValueError('driver contains product source differences')
        if any(sha(Path(p))!=h for p,h in pins.items()): raise ValueError('pinned input changed')
        if args.output.exists(): guard(args.output)
        elif shutil.disk_usage(args.output.parent).free < RESERVE: raise ValueError('disk reserve')
    sys.modules['sc64_headless_physical']=flash
    resource.setrlimit(resource.RLIMIT_FSIZE,(LIMIT,LIMIT))
    old=signal.signal(signal.SIGALRM,timeout);oldterm=signal.signal(signal.SIGTERM,timeout)
    signal.alarm(args.deadline)
    try:
        with open(LOCK,'a') as lock:
            fcntl.flock(lock,fcntl.LOCK_EX);check();args.output.mkdir()
            result={'driver_head':args.driver_head,'compiled_head':COMPILED_HEAD,'pins':pins,
                    'first':args.first,'count':args.count,'cases':[],
                    'scope':'ordinary physical custom opening, save and 60-update named-world/Q8 continuation; no pixels, wins or hardware proof'}
            # Keep the native lock in the parent. Poll total owned output and
            # free space even while the worker blocks on its physical runner.
            blocked={signal.SIGTERM,signal.SIGINT,signal.SIGALRM}
            oldmask=signal.pthread_sigmask(signal.SIG_BLOCK,blocked)
            try: pid=os.fork()
            except BaseException:
                signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
                raise
            if pid==0:
                try:
                    os.setsid()  # All runner children inherit this group.
                    signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
                    with (args.output/'driver.log').open('wb') as log:
                        os.dup2(log.fileno(),1);os.dup2(log.fileno(),2)
                        for i in range(args.first,args.first+args.count):
                            check();result['cases'].append(run_case(i,cases[i],args,args.output));check()
                            (args.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
                    os._exit(0)
                except BaseException as exc:
                    result['error']=repr(exc)
                    try:
                        (args.output/'failure.json').write_text(json.dumps(result,indent=2)+'\n')
                        traceback.print_exc()
                    finally:os._exit(1)
            try:
                signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
                while not worker_exited(pid):
                    guard(args.output)  # 64 MiB is a polled threshold.
                    time.sleep(.2)
                # Kill any orphaned descendants before reaping the leader;
                # block termination through the reap/PID-disarm boundary.
                signal.pthread_sigmask(signal.SIG_BLOCK,blocked)
                try:
                    signal_group(pid,signal.SIGKILL)
                    _,status=os.waitpid(pid,0);pid=None
                finally:signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
                if not os.WIFEXITED(status) or os.WEXITSTATUS(status)!=0:
                    raise RuntimeError('physical matrix worker failed')
                check()
            finally:
                signal.pthread_sigmask(signal.SIG_BLOCK,blocked)
                try:
                    if pid is not None: terminate_and_reap(pid)
                finally:signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
            return 0
    finally:
        signal.alarm(0);signal.signal(signal.SIGALRM,old);signal.signal(signal.SIGTERM,oldterm)


if __name__=='__main__':sys.exit(main())
