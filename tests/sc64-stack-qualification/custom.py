#!/usr/bin/env python3
"""Read-only system-stack paint sampling during physical custom SRAM continuation."""
import argparse,json,pathlib,sys,traceback
ROOT=pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
import sc64_skirmish_sram_continuation as continuation
P=argparse.ArgumentParser();P.add_argument('--rom',type=pathlib.Path,required=True);P.add_argument('--elf',type=pathlib.Path,required=True);P.add_argument('--runner',type=pathlib.Path,required=True);P.add_argument('--output',type=pathlib.Path,required=True);P.add_argument('--paint',action='store_true');P.add_argument('--play',type=int,default=600);P.add_argument('--continue-updates',type=int,default=120);a=P.parse_args()
if not 60<=a.play<=60000 or not 60<=a.continue_updates<=20000: P.error('invalid run lengths')
SAMPLES=[]
INPUTS=[]
class MeasuredGame(continuation.PhysicalGame):
 def __init__(self,*args,**kwargs):
  super().__init__(*args,**kwargs);self.phase=len({e[0] for e in INPUTS})
 def step(self,keys=0,n=1):
  INPUTS.append([self.phase,self.rsp.completed,keys,n]);return super().step(keys,n)
 def state(self):
  result=super().state()
  if a.paint:
   low=0x03005000;sp=self.u32('sc64_stack_paint_sp');end=self.u32('sc64_stack_paint_end');status=self.u32('sc64_stack_paint_status')
   if status!=2 or not low<=end<=sp-512 or sp>0x03007e00 or end%4:raise RuntimeError('invalid painter bounds or status')
   data=b''.join(self.rsp.read(p,min(512,end-p)) for p in range(low,end,512)); marker=bytes.fromhex('c33c5aa5');unused=0
   for pos in range(0,len(data),4):
    if data[pos:pos+4]!=marker:break
    unused+=4
   if unused==0:raise RuntimeError('stack paint reached guard')
   SAMPLES.append({'updates':self.rsp.completed,'frame':result['frame'],'sp_at_paint':sp,'paint_end':end,'painted':end-low,'untouched':unused})
  return result
continuation.PhysicalGame=MeasuredGame
args=argparse.Namespace(rom=a.rom,elf=a.elf,runner=a.runner,output=a.output,map=1,human='z',bots='p',play=a.play,continue_updates=a.continue_updates,physical=True,tamper_sram=False)
try:
 r=continuation.run(args);r['stack_samples']=SAMPLES;r['input_steps']=INPUTS;r['paint_scope']='system stack only, excludes startup, live-frame gap, IRQ and BIOS' if a.paint else 'uninstrumented';(a.output/'result.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({'ok':r['ok'],'checks':r['checks'],'samples':SAMPLES}));sys.exit(0 if r['ok'] else 1)
except Exception as e:
 if a.output.is_dir():(a.output/'error.json').write_text(json.dumps({'error':repr(e),'stack_samples':SAMPLES,'input_steps':INPUTS},indent=2)+'\n')
 traceback.print_exc();sys.exit(1)
