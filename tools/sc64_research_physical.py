#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay ordinary research controls through physical buttons; inspect only.

The input tape is private. The backend may import/export real 32 KiB SRAM at
boot/exit; it never writes guest memory and never injects gameplay state.
"""
import argparse
import fcntl
import os
import shutil
import signal
import sys
import time
import hashlib
import json
from pathlib import Path
import subprocess

import sc64_headless_physical as physical
from emulator_smoke import symbols, symbol_sizes
from sc64_races_profile import dwarf_struct_layout


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def transitions(path):
    rows = [json.loads(line) for line in path.read_text().splitlines()]
    rows = [r for r in rows if r.get('kind') == 'input']
    return [(r['call_index'], r['held']) for r in rows]


def snapshot(rsp, sym, sizes, layout):
    def scalar(name):
        return int.from_bytes(rsp.read(sym[name], sizes[name]), 'little', signed=True)
    stride, off = layout
    data = rsp.read(sym['b'], sizes['b'])
    jobs = []
    for i in range(len(data) // stride):
        row = data[i*stride:(i+1)*stride]
        def field(name):
            n = off.widths[name]
            return int.from_bytes(row[off[name]:off[name]+n], 'little', signed=off.signed.get(name, False))
        if field('alive') and field('level') and field('queue_progress'):
            queue = list(row[off['queue']:off['queue']+5])
            jobs.append({'slot':i,'owner':field('owner'),'type':field('type'),
                         'family_plus_one':field('level'),'timer':field('queue_progress'),'queue':queue})
    levels = list(rsp.read(sym['sc64_experimental_research_levels'], 48))
    return {'frame':scalar('frame'),'screen':scalar('screen'),'bots':scalar('race_bots'),
            'minerals':scalar('minerals'),'gas':scalar('gas'),'lost':scalar('lost'),
            'jobs':jobs,'levels':levels}


LOCK_PATH = '/tmp/sc64-native-build-20261031.lock'
RUNNER_SHA = '4199fe903b18c77e0833db1860c4968509cfc82e506bc6f2f9be7582b720fae2'
WALL_SECONDS = 180
OUTPUT_LIMIT = 64 * 1024 * 1024
RESERVE = 4 * 1024 ** 3


def validate_inputs(a):
    if a.output.exists():
        raise ValueError('output already exists')
    ancestor = a.output.parent
    while not ancestor.exists() and ancestor != ancestor.parent:
        ancestor = ancestor.parent
    if not ancestor.is_dir() or shutil.disk_usage(ancestor).free < RESERVE:
        raise ValueError('output filesystem has less than 4 GiB free')
    inputs = (a.rom, a.elf, a.runner, a.tape, a.resume_tape)
    pins = {str(p): sha(p) for p in inputs}
    if pins[str(a.runner)] != RUNNER_SHA:
        raise ValueError('runner does not match the audited physical backend')
    return pins


def output_bytes(path):
    total = 0
    if not path.exists():
        return 0
    for root, dirs, files in os.walk(path, followlinks=False):
        for name in files:
            p = Path(root) / name
            if not p.is_symlink():
                total += p.stat().st_size
                if total > OUTPUT_LIMIT:
                    return total
    return total


def run_worker(a):
    validate_inputs(a)
    a.output.mkdir(parents=True,exist_ok=False)
    sym,sizes=symbols(a.elf),symbol_sizes(a.elf)
    required=('b','frame','screen','race_bots','minerals','gas','lost','sc64_experimental_research_levels')
    if any(k not in sym or k not in sizes for k in required): raise ValueError('missing observed symbols')
    layout=dwarf_struct_layout(a.elf,'Building'); stride,off=layout
    if sizes['b']%stride or off.arrays.get('queue')!=(5,1,False): raise ValueError('unexpected building ABI')
    pins={str(p):sha(p) for p in (a.rom,a.elf,a.runner,a.tape,a.resume_tape)}
    events=transitions(a.tape); resume=transitions(a.resume_tape)
    saved=a.output/'physical.sav';first=a.output/'first';first.mkdir();proc=None
    try:
        proc,rsp,log=physical.start(a.runner,a.rom,first,sym,sizes,save_output=saved)
        rsp.play_to(3044,events); paid=snapshot(rsp,sym,sizes,layout)
        if paid['screen']!=1 or paid['lost'] or paid['bots']!=1 or not any(j['owner']==0 and j['family_plus_one']==3 and j['queue'][:2]==[0,1] for j in paid['jobs']):
            raise RuntimeError(f'ordinary tape did not reach the paid job: {paid}')
        rsp.play_to(3048,events); paused=snapshot(rsp,sym,sizes,layout)
        if paused['screen']!=2: raise RuntimeError(f'pause/save not reached: {paused}')
        rsp.export_sram();proc.terminate();proc.wait(timeout=3);log.close();proc=None
        if saved.stat().st_size!=32768: raise RuntimeError('bad exported SRAM length')
        raw=saved.read_bytes();tags=[int.from_bytes(raw[i:i+4],'little') for i in (0,16384)]
        if not any((v>>16)&255==0x45 for v in tags): raise RuntimeError(f'no active-job tag: {tags}')
        second=a.output/'fresh';second.mkdir()
        proc,rsp,log=physical.start(a.runner,a.rom,second,sym,sizes,save_input=saved)
        rsp.play_to(2,resume);loaded=snapshot(rsp,sym,sizes,layout)
        if loaded['screen']!=1 or loaded['bots']!=1 or not any(j['owner']==0 and j['family_plus_one']==3 and j['queue'][:2]==[0,1] for j in loaded['jobs']):
            raise RuntimeError(f'fresh physical load did not restore the job: {loaded}')
        rsp.play_to(5002,resume);finished=snapshot(rsp,sym,sizes,layout)
        if finished['screen']!=1 or finished['lost'] or finished['levels'][2]!=1:
            raise RuntimeError(f'ordinary research did not complete: {finished}')
        proc.terminate();proc.wait(timeout=3);log.close();proc=None
        if any(sha(Path(k))!=v for k,v in pins.items()): raise RuntimeError('input changed')
        (a.output/'result.json').write_text(json.dumps({'pins':pins,'save_sha256':sha(saved),'tags':tags,
            'paid':paid,'paused':paused,'loaded':loaded,'finished':finished,
            'scope':'physical buttons; named read-only state; constructed controller decisions, not full-world or hardware proof'},indent=2)+'\n')
        print((a.output/'result.json').read_text())
    finally:
        if proc is not None:
            proc.terminate()
            try: proc.wait(timeout=3)
            except subprocess.TimeoutExpired: proc.kill();proc.wait()

def main(argv=None):
    ap = argparse.ArgumentParser(description='Physical replay; owns the shared native lock. Do not wrap in another lock.')
    for name in ('rom','elf','runner','tape','resume_tape','output'):
        ap.add_argument('--'+name.replace('_','-'), type=Path, required=True)
    ap.add_argument('--internal-worker', action='store_true', help=argparse.SUPPRESS)
    a = ap.parse_args(argv)
    if a.internal_worker:
        try:
            fd=int(os.environ.get('SC64_RESEARCH_PROBE_LOCK_FD','-1'))
            actual=os.fstat(fd); expected=os.stat(LOCK_PATH)
            if (actual.st_dev,actual.st_ino)!=(expected.st_dev,expected.st_ino):
                raise ValueError('wrong lock file')
            fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
        except (ValueError,OSError):
            ap.error('internal worker requires the held shared lock')
        return run_worker(a)
    deadline=time.monotonic()+WALL_SECONDS
    try:
        pins=validate_inputs(a)
        with open(LOCK_PATH,'a') as lock:
            while True:
                try:
                    fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
                    break
                except BlockingIOError:
                    if time.monotonic()>=deadline-3:
                        raise TimeoutError('shared native lock timed out')
                    time.sleep(min(0.2,max(0,deadline-3-time.monotonic())))
            if validate_inputs(a)!=pins:
                raise ValueError('input changed while waiting for lock')
            if time.monotonic()>=deadline-3:
                raise TimeoutError('physical replay exceeded wall limit before launch')
            env=os.environ.copy();env['SC64_RESEARCH_PROBE_LOCK_FD']=str(lock.fileno())
            args=[sys.executable,str(Path(__file__).resolve()),'--internal-worker']
            for name in ('rom','elf','runner','tape','resume_tape','output'):
                args += ['--'+name.replace('_','-'),str(getattr(a,name))]
            proc=None
            def terminate(signum,_frame):
                raise SystemExit(128+signum)
            oldterm=signal.signal(signal.SIGTERM,terminate)
            try:
                proc=subprocess.Popen(args,env=env,start_new_session=True,pass_fds=(lock.fileno(),))
                while proc.poll() is None:
                    if output_bytes(a.output)>OUTPUT_LIMIT:
                        raise ValueError('output exceeds 64 MiB')
                    if time.monotonic()>=deadline-3:
                        raise TimeoutError('physical replay exceeded wall limit')
                    time.sleep(0.2)
                if output_bytes(a.output)>OUTPUT_LIMIT:
                    raise ValueError('output exceeds 64 MiB')
                return proc.returncode
            finally:
                if proc is not None:
                    # Kill only our session, even if the worker exited first.
                    try: os.killpg(proc.pid,signal.SIGTERM)
                    except ProcessLookupError: pass
                    try: proc.wait(timeout=max(0,min(1,deadline-time.monotonic())))
                    except subprocess.TimeoutExpired: pass
                    try: os.killpg(proc.pid,signal.SIGKILL)
                    except ProcessLookupError: pass
                    if proc.poll() is None:
                        try: proc.wait(timeout=max(0,deadline-time.monotonic()))
                        except subprocess.TimeoutExpired: pass
                signal.signal(signal.SIGTERM,oldterm)
    except (OSError,ValueError,TimeoutError) as exc:
        print(str(exc),file=sys.stderr)
        return 124 if isinstance(exc,TimeoutError) else 2


if __name__=='__main__': sys.exit(main())
