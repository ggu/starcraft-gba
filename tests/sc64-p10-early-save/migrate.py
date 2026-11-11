#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Load physical SRAM from a completed old-product probe into a new product.

The old result, snapshots and SRAM must be from physical.py's successful run.
No SRAM mutation or guest-memory writes are performed. Use a new output path.
"""
import argparse, hashlib, json, pathlib, sys, traceback
ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from sc64_skirmish_sram_continuation import PhysicalGame, fingerprint

p=argparse.ArgumentParser()
for arg in ('runner','rom','elf','baseline','out'):
    p.add_argument('--'+arg,type=pathlib.Path,required=True)
a=p.parse_args(); a.out.mkdir(parents=True,exist_ok=False)
sha=lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
old=json.loads((a.baseline/'result.json').read_text())
saved=json.loads((a.baseline/'saved.json').read_text())
reference=json.loads((a.baseline/'reference.json').read_text())
sram=a.baseline/'actual.sav'
assert old['ok'] and old['sram_unchanged'] and sram.stat().st_size==32768
assert sha(sram)==old['sram_sha256']
for name,s in (('saved',saved),('reference',reference)):
    assert fingerprint({k:v for k,v in s.items() if k!='screen'})==old['snapshots'][name]['fingerprint']
assert reference['frame']>saved['frame']
record={'mission':old['mission'],'baseline_result_sha256':sha(a.baseline/'result.json'),
        'baseline_sram_sha256':sha(sram),'inputs':{str(x):sha(x) for x in (a.runner,a.rom,a.elf)},
        'trace':[],'snapshots':{}}
g=None

def step(keys=0,n=1):
    record['trace'].append([g.rsp.completed,keys,n]); g.step(keys,n)
def tap(key): step(key); step()
def modal(): return int.from_bytes(g.rsp.read(0x06013fe0,2),'little')
def advance(target):
    for _ in range(20000):
        if g.u32('frame')>=target:return
        if modal():
            for _ in range(100):
                if not modal():break
                tap(1)
            else:raise RuntimeError('modal limit')
        else:step()
    raise RuntimeError('frame limit')
def snap(name):
    s=g.state();s['mission']=g.u32('race_selected_mission')
    names=['race_campaign','race_p3_action_ticks','race_p3','race_p4','race_p5','race_p6','race_p7','race_p7_projectiles','race_p7_projectile_fault','supply_used','supply_cap']
    s['extended']={n:g.rsp.read(g.sym[n],g.sizes[n]).hex() for n in names if n in g.sym and n in g.sizes}
    (a.out/(name+'.json')).write_text(json.dumps(s,indent=2)+'\n')
    record['snapshots'][name]={'frame':s['frame'],'screen':s['screen'],'fingerprint':fingerprint({k:v for k,v in s.items() if k!='screen'})}
    print(name,record['snapshots'][name],flush=True);return s
def diff(x,y):return [k for k in x if k!='screen' and fingerprint(x[k])!=fingerprint(y[k])]
try:
    g=PhysicalGame(a.runner,a.rom,a.elf,a.out/'phase',save_input=sram,save_output=a.out/'after-load.sav')
    step(0,2);step(512);restored=snap('restored')
    record['restored_differences']=diff(saved,restored)
    assert restored['screen']==1 and not record['restored_differences'],('restore mismatch',record['restored_differences'])
    step();advance(reference['frame']);resumed=snap('resumed')
    record['continued_differences']=diff(reference,resumed)
    assert not record['continued_differences'],('continuation mismatch',record['continued_differences'])
    g.rsp.export_sram();record['sram_unchanged']=sha(a.out/'after-load.sav')==old['sram_sha256']
    assert record['sram_unchanged'] and sha(sram)==old['sram_sha256'];record['ok']=True
except BaseException as exc:
    record['ok']=False;record['error']=repr(exc);traceback.print_exc()
finally:
    if g:g.close()
    record['inputs_unchanged']=all(sha(path)==record['inputs'][str(path)] for path in (a.runner,a.rom,a.elf))
    record['baseline_unchanged']=(sha(sram)==record['baseline_sram_sha256'] and sha(a.baseline/'result.json')==record['baseline_result_sha256'])
    if not record['inputs_unchanged'] or not record['baseline_unchanged']:
        record['ok']=False;record['error']='input or baseline changed during run'
    (a.out/'result.json').write_text(json.dumps(record,indent=2)+'\n')
if not record['ok']:sys.exit(1)
