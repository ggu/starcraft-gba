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
import select
import shutil
import signal
import subprocess
import sys
import time
import traceback

import sc64_headless_flash_physical as flash
import sc64_skirmish_sram_continuation as world
import sc64_research_flash_physical as research
from sc64_races_profile import dwarf_struct_layout
import sc64_q8_custom_matrix_physical as matrix
from sc64_q8_custom_matrix_physical import (LOCK, RUNNER, COMPILED_HEAD, RESERVE, LIMIT,
    sha, scenarios, flash_record, validate_receipt, guard, timeout, worker_exited,
    terminate_and_reap, require_no_children, await_completion)

def active_job(value):
    return [j for j in value['jobs'] if j['owner']==0 and j['family_plus_one']==3 and j['queue'][:2]==[0,1] and j['timer']>0]


def validate_paid(before, paid):
    if (paid['screen']!=1 or paid['lost'] or paid['bots']!=1 or len(active_job(paid))!=1 or
            paid['levels'][2]!=0 or before['minerals']-paid['minerals']!=100 or
            before['gas']-paid['gas']!=100 or active_job(before)):
        raise ValueError('paid Forge job and exact debit not observed')


def validate_restored(saved, restored):
    comparable=lambda value: {k:v for k,v in value.items() if k!='screen'}
    if restored['screen']!=1 or comparable(restored)!=comparable(saved):
        raise ValueError('before-release restored state differs')


def validate_finished(finished):
    if finished['screen']!=1 or finished['lost'] or finished['levels'][2]!=1 or active_job(finished):
        raise ValueError('level-one research did not complete')


def run_case(index, case, args, out):
    del index, case
    progress = {}
    def record(name, value):
        progress[name] = value
        (out / 'progress.json').write_text(json.dumps(progress, indent=2) + '\n')
        guard(out)
    def observed(game):
        value = research.snapshot(game.rsp, game.sym, game.sizes, dwarf_struct_layout(args.elf, 'Building'))
        value['debt'] = game.rsp.read(game.sym['sc64_pr_hp_debt'], 400).hex()
        context = matrix.snapshot(game, (0, 2, (0,)))
        value['context'] = {k:context[k] for k in ('race_mode','race_skirmish_map','race_human','race_bots','players','bot_races')}
        return value
    def compare(value): return {k:v for k,v in value.items() if k != 'screen'}
    image = out / 'guest.sav'
    events = research.transitions(args.tape); resume = research.transitions(args.resume_tape)
    if not resume or resume[0] != (0, 512): raise ValueError('resume must start with L')
    first = None
    try:
        first = world.PhysicalGame(args.runner,args.rom,args.elf,out/'first',save_input=args.blank,save_output=image)
        first.rsp.play_to(3042,events); before=observed(first); record('before_purchase',before)
        first.rsp.play_to(3044,events); paid=observed(first); record('paid',paid)
        validate_paid(before,paid)
        first.rsp.play_to(3048,events); saved=observed(first); record('saved',saved)
        if saved['screen']!=2 or compare(saved)!=compare(paid): raise ValueError('save boundary differs from paid state')
        first.rsp.export_sram(); journal=flash_record(image.read_bytes()); saved_hash=sha(image)
        record('journal',journal); record('save_sha256',saved_hash)
    finally:
        if first is not None: first.close()
    second = None
    try:
        second = world.PhysicalGame(args.runner,args.rom,args.elf,out/'fresh',save_input=image)
        second.rsp.play_to(1,resume); restored=observed(second); record('restored',restored)
        validate_restored(saved,restored)
        second.rsp.play_to(5002,resume); finished=observed(second); record('finished',finished)
        validate_finished(finished)
    finally:
        if second is not None: second.close()
    if sha(image)!=saved_hash: raise ValueError('guest save changed after fresh load')
    return progress


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('rom','elf','runner','blank','receipt','output','tape','resume_tape'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--driver-head',required=True)
    parser.add_argument('--first',type=int,required=True)
    parser.add_argument('--count',type=int,required=True)
    parser.add_argument('--deadline',type=int,default=600)
    args=parser.parse_args(); cases=scenarios()
    if args.first != 0 or args.count != 1 or not 1<=args.deadline<=1200:
        parser.error('invalid batch bounds')
    if args.output.exists() or not args.output.parent.is_dir(): parser.error('output must be new with existing parent')
    inputs=(args.rom,args.elf,args.runner,args.blank,args.receipt,args.tape,args.resume_tape)
    if sha(args.tape) != 'd6bb9e46f9011a8af8c32eb62942ab8d611974b5592df85e5788358900c32df5' or sha(args.resume_tape) != '389a025f68e66c1cd723338109ab4dcd1d51467317937ded2ecd7a073580329f': raise ValueError('tape pin mismatch')
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
        if not diff.issubset({'tools/sc64_q8_custom_matrix_physical.py','tests/test_sc64_q8_custom_matrix_physical.py','tools/sc64_q8_paid_research_physical.py','tests/test_sc64_q8_paid_research_physical.py'}):
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
                    'scope':'ordinary physical paid Forge job, Flash save, fresh load and level-one completion; named fields only, no pixels or hardware proof'}
            # Keep the native lock in the parent. Poll total owned output and
            # free space even while the worker blocks on its physical runner.
            done_read,done_write=os.pipe();ack_read,ack_write=os.pipe()
            blocked={signal.SIGTERM,signal.SIGINT,signal.SIGALRM}
            oldmask=signal.pthread_sigmask(signal.SIG_BLOCK,blocked)
            try: pid=os.fork()
            except BaseException:
                for fd in (done_read,done_write,ack_read,ack_write):os.close(fd)
                signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
                raise
            if pid==0:
                try:
                    os.close(done_read);os.close(ack_write)
                    os.setsid()  # All runner children inherit this group.
                    signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
                    with (args.output/'driver.log').open('wb') as log:
                        os.dup2(log.fileno(),1);os.dup2(log.fileno(),2)
                        for i in range(args.first,args.first+args.count):
                            check();result['cases'].append(run_case(i,cases[i],args,args.output));check()
                            (args.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
                    require_no_children()
                    if os.write(done_write,b'D')!=1 or os.read(ack_read,1)!=b'A':
                        raise RuntimeError('completion handshake failed')
                    os.close(done_write);os.close(ack_read)
                    os._exit(0)
                except BaseException as exc:
                    result['error']=repr(exc)
                    try:
                        (args.output/'failure.json').write_text(json.dumps(result,indent=2)+'\n')
                        traceback.print_exc()
                    finally:os._exit(1)
            os.close(done_write);os.close(ack_read)
            try:
                signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
                await_completion(pid,done_read,ack_write,args.output)
                # Worker proved it has reaped direct children before DONE;
                # do not signal a zombie-only group on successful completion.
                signal.pthread_sigmask(signal.SIG_BLOCK,blocked)
                try:
                    _,status=os.waitpid(pid,0);pid=None
                finally:signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
                if not os.WIFEXITED(status) or os.WEXITSTATUS(status)!=0:
                    raise RuntimeError('physical matrix worker failed')
                check()
            finally:
                signal.pthread_sigmask(signal.SIG_BLOCK,blocked)
                try:
                    if pid is not None: terminate_and_reap(pid)
                finally:
                    os.close(done_read);os.close(ack_write)
                    signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
            return 0
    finally:
        signal.alarm(0);signal.signal(signal.SIGALRM,old);signal.signal(signal.SIGTERM,oldterm)


if __name__=='__main__':sys.exit(main())
