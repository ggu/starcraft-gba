#!/usr/bin/env python3
"""Ordinary Z7 launch and rejected campaign save, with optional stack read."""
import argparse,hashlib,json,pathlib,sys,traceback
ROOT=pathlib.Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from sc64_skirmish_sram_continuation import PhysicalGame,fingerprint
from sc64_z7_arm_replay import save_records
p=argparse.ArgumentParser();p.add_argument('--rom',type=pathlib.Path,required=True);p.add_argument('--elf',type=pathlib.Path,required=True);p.add_argument('--runner',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True);p.add_argument('--seed',type=pathlib.Path,required=True);p.add_argument('--paint',action='store_true');a=p.parse_args();seed=a.seed.read_bytes();assert len(seed)==32768;assert not a.out.exists();a.out.mkdir(parents=True);r={'inputs':[],'samples':[],'seed_sha256':hashlib.sha256(seed).hexdigest()};g=None
try:
 g=PhysicalGame(a.runner,a.rom,a.elf,a.out/'phase',save_input=a.seed,save_output=a.out/'after.sav')
 def step(key=0,n=1):r['inputs'].append([g.rsp.completed,key,n]);g.step(key,n)
 def tap(key):step(key);step()
 def paint(label):
  if not a.paint:return
  low=0x03005000;sp=g.u32('sc64_stack_paint_sp');end=g.u32('sc64_stack_paint_end');status=g.u32('sc64_stack_paint_status');assert status==2 and low<=end<=sp-512 and sp<=0x03007e00 and end%4==0
  data=b''.join(g.rsp.read(i,min(512,end-i)) for i in range(low,end,512));marker=bytes.fromhex('c33c5aa5');n=0
  for i in range(0,len(data),4):
   if data[i:i+4]!=marker:break
   n+=4
  assert n>0;r['samples'].append({'phase':label,'sp_at_paint':sp,'paint_end':end,'untouched':n,'painted':end-low})
 step(0,2);tap(1)
 for _ in range(3):tap(16)
 tap(1);tap(8)
 for _ in range(120):
  if g.u32('screen')==1:break
  step()
 r['screen']=g.u32('screen');r['mode']=g.u32('race_mode');r['mission']=g.u32('race_selected_mission');assert (r['screen'],r['mode'],r['mission'])==(1,0,18)
 for _ in range(100):
  if not int.from_bytes(g.rsp.read(0x06013fe0,2),'little'):break
  tap(1)
 else:raise RuntimeError('modal limit')
 start=g.u32('frame')
 for _ in range(1000):
  if g.u32('frame')>=start+30:break
  if int.from_bytes(g.rsp.read(0x06013fe0,2),'little'):tap(1)
  else:step()
 else:raise RuntimeError('frame limit')
 step(8);step();assert g.u32('screen')==2;step(2);r['notice']=g.u32('pause_notice')&0xff;r['save_refused']=(r['notice']&0xc0)==0x80;state=g.state();r['frame']=state['frame'];r['fingerprint']=fingerprint(state);paint('rejected-save')
 g.rsp.export_sram();data=(a.out/'after.sav').read_bytes();r['sram_sha256']=hashlib.sha256(data).hexdigest();r['sram_records']=save_records(data);r['sram_unchanged']=data==seed;r['ok']=bool(r['save_refused'] and len(data)==32768 and r['sram_unchanged'])
except Exception as e:r['error']=repr(e);r['ok']=False;traceback.print_exc()
finally:
 if g:g.close()
 (a.out/'result.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r),flush=True)
sys.exit(0 if r['ok'] else 1)
