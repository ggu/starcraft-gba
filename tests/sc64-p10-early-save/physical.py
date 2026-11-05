#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ordinary-input P1-P7 fresh-process early save probe (requires physical runner).

No guest writes or synthetic SRAM. Results are finite early neutral continuations,
not arbitrary-save or victory qualification. Output directory must be new.
"""
import argparse, hashlib, json, pathlib, sys, traceback
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from sc64_skirmish_sram_continuation import PhysicalGame, fingerprint

p = argparse.ArgumentParser()
p.add_argument('--runner', type=pathlib.Path, required=True)
p.add_argument('--rom', type=pathlib.Path, required=True)
p.add_argument('--elf', type=pathlib.Path, required=True)
p.add_argument('--out', type=pathlib.Path, required=True)
p.add_argument('--mission', type=int, choices=range(1, 8), required=True)
p.add_argument('--continue-frames', type=int, default=120)
a = p.parse_args(); a.out.mkdir(parents=True, exist_ok=False)
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
record = {'mission': a.mission, 'inputs': {str(x): sha(x) for x in (a.runner,a.rom,a.elf)}, 'trace': [], 'snapshots': {}}
g = None

def step(keys=0,n=1):
    record['trace'].append([g.rsp.completed, keys, n]); g.step(keys,n)
def tap(key): step(key); step()
def modal(): return int.from_bytes(g.rsp.read(0x06013fe0,2),'little')
def dismiss():
    for _ in range(100):
        if not modal(): return
        tap(1)
    raise RuntimeError('modal limit')
def advance(target):
    for _ in range(10000):
        if g.u32('frame') >= target: return
        if modal(): dismiss()
        else: step()
    raise RuntimeError('frame limit')
def snap(name):
    s=g.state(); s['mission']=g.u32('race_selected_mission')
    names=['race_campaign','race_p3_action_ticks','race_p3','race_p4','race_p5','race_p6','race_p7','race_p7_projectiles','race_p7_projectile_fault','supply_used','supply_cap']
    s['extended']={n:g.rsp.read(g.sym[n],g.sizes[n]).hex() for n in names if n in g.sym and n in g.sizes}
    (a.out/(name+'.json')).write_text(json.dumps(s,indent=2)+'\n')
    record['snapshots'][name]={'frame':s['frame'],'screen':s['screen'],'fingerprint':fingerprint({k:v for k,v in s.items() if k!='screen'})}
    print(name,record['snapshots'][name],flush=True); return s
def diff(x,y): return [k for k in x if k!='screen' and x[k]!=y[k]]
try:
    g=PhysicalGame(a.runner,a.rom,a.elf,a.out/'phase1',save_output=a.out/'actual.sav')
    step(0,2); tap(1)
    for _ in range((a.mission-1)//2): tap(16)
    if (a.mission-1)%2: tap(128)
    tap(1); tap(8)
    for _ in range(120):
        if g.u32('screen')==1: break
        step()
    expected={1:2,2:3,3:22,4:23,5:24,6:25,7:26}[a.mission]
    assert g.u32('screen')==1 and g.u32('race_selected_mission')==expected, ('launch',g.u32('screen'),g.u32('race_selected_mission'),expected)
    dismiss(); advance(g.u32('frame')+30)
    tap(8); assert g.u32('screen')==2
    tap(2); saved=snap('saved'); assert g.u32('pause_notice')&0xc0==0x40, ('save rejected',g.u32('pause_notice'))
    g.rsp.export_sram(); assert (a.out/'actual.sav').stat().st_size==32768
    record['sram_sha256']=sha(a.out/'actual.sav')
    tap(8); advance(saved['frame']+a.continue_frames); ref=snap('reference')
    g.close();g=None
    g=PhysicalGame(a.runner,a.rom,a.elf,a.out/'phase2',save_input=a.out/'actual.sav',save_output=a.out/'after-load.sav')
    step(0,2); step(512); restored=snap('restored')
    record['restored_differences']=diff(saved,restored)
    assert restored['screen']==1 and not record['restored_differences'], ('restore mismatch',record['restored_differences'])
    step(); advance(saved['frame']+a.continue_frames); resumed=snap('resumed')
    record['continued_differences']=diff(ref,resumed)
    assert not record['continued_differences'], ('continuation mismatch',record['continued_differences'])
    g.rsp.export_sram(); record['sram_unchanged']=sha(a.out/'after-load.sav')==record['sram_sha256']
    assert record['sram_unchanged']; record['ok']=True
except BaseException as exc:
    record['ok']=False;record['error']=repr(exc);traceback.print_exc()
finally:
    if g: g.close()
    (a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n')
if not record['ok']: sys.exit(1)
