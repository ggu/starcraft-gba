#!/usr/bin/env python3
"""Load a genuine Z7 journal and exercise campaign save refusal with physical input."""
import argparse,hashlib,json,pathlib,sys,traceback
ROOT=pathlib.Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from sc64_skirmish_sram_continuation import PhysicalGame,fingerprint
import sc64_z7_arm_replay as save
p=argparse.ArgumentParser();p.add_argument('--rom',type=pathlib.Path,required=True);p.add_argument('--elf',type=pathlib.Path,required=True);p.add_argument('--runner',type=pathlib.Path,required=True);p.add_argument('--seed',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True);p.add_argument('--paint',action='store_true');a=p.parse_args()
seed=a.seed.read_bytes();sha=lambda b:hashlib.sha256(b).hexdigest();assert sha(seed)=='182407d91e782c4e879e65047223a70a0ec5f83bbec0131d4e205debddbd0d39';records,errors=save.sram_check('prefix',{'final':{'kind':'cut'}},seed,None,'cargo-transport1');assert not errors
assert not a.out.exists();a.out.mkdir(parents=True);r={'seed_sha256':sha(seed),'seed_records':records,'inputs':[],'samples':[]};g=None
try:
 g=PhysicalGame(a.runner,a.rom,a.elf,a.out/'phase',save_input=a.seed,save_output=a.out/'after.sav')
 def step(key=0,n=1):r['inputs'].append([g.rsp.completed,key,n]);g.step(key,n)
 def paint(label):
  if not a.paint:return
  low=0x03005000;sp=g.u32('sc64_stack_paint_sp');end=g.u32('sc64_stack_paint_end');status=g.u32('sc64_stack_paint_status');assert status==2 and low<=end<=sp-512 and sp<=0x03007e00 and end%4==0
  data=b''.join(g.rsp.read(i,min(512,end-i)) for i in range(low,end,512));marker=bytes.fromhex('c33c5aa5');n=0
  for i in range(0,len(data),4):
   if data[i:i+4]!=marker:break
   n+=4
  assert n>0;r['samples'].append({'phase':label,'sp_at_paint':sp,'paint_end':end,'untouched':n,'painted':end-low})
 step(0,2);step(512);step();r['screen']=g.u32('screen');r['front_load_error']=g.u32('race_front_load_error');r['mode']=g.u32('race_mode');r['mission']=g.u32('race_selected_mission');r['loaded']=r['screen']==1 and r['mode']==0 and r['mission']==18;paint('after-load-attempt')
 if r['loaded']:
  state=g.state();r['loaded_frame']=state['frame'];r['loaded_fingerprint']=fingerprint(state);step(8);step();r['pause_screen']=g.u32('screen');step(2);r['notice']=g.u32('pause_notice')&0xff;r['save_refused']=(r['notice']&0xc0)==0x80;paint('rejected-save')
 g.rsp.export_sram();r['after_sha256']=sha((a.out/'after.sav').read_bytes());r['unchanged']=r['after_sha256']==r['seed_sha256'];r['ok']=bool(r['loaded'] and r.get('save_refused') and r['unchanged'])
except BaseException as e:r['error']=repr(e);r['ok']=False;traceback.print_exc()
finally:
 if g:g.close()
 (a.out/'result.json').write_text(json.dumps(r,indent=2)+'\n')
 print(json.dumps(r),flush=True)
sys.exit(0 if r['ok'] else 1)
