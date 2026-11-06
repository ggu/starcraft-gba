#!/usr/bin/env python3
"""Bounded ordinary-button early Protoss save/load probe; GPL-3.0-or-later.
Uses the production ROM and physical backend; only guest reads are used.
The result covers one save point, not a mission win or arbitrary-save support.
"""
import argparse,hashlib,json,sys
from pathlib import Path
from sc64_skirmish_sram_continuation import (PhysicalGame,SCREEN_TITLE,SCREEN_PLAY,SCREEN_PAUSE,
 KEY_A,KEY_B,KEY_START,KEY_RIGHT,KEY_DOWN,KEY_L,fingerprint)
ROM_SHA='ff94a2ea9f98404c97f44a8b6b7bf557b3572c761de530d867032eeb8f76b10b'
ELF_SHA='f961647217516de367b2f2e28b1b82dee4e744e3bef7f9c7b0f46e4100f55587'
RUNNER_SHA='4199fe903b18c77e0833db1860c4968509cfc82e506bc6f2f9be7582b720fae2'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def snapshot(g):
 state=g.state();state.pop('screen');state['mission']=g.u32('race_selected_mission');state['race_mode']=g.u32('race_mode')
 return {'frame':state['frame'],'mission':state['mission'],'mode':state['race_mode'],
         'units':len(state['units']),'buildings':len(state['buildings']),'fingerprint':fingerprint(state)}
def main():
 ap=argparse.ArgumentParser();
 for arg in ('rom','elf','runner','output'):ap.add_argument('--'+arg,type=Path,required=True)
 ap.add_argument('--mission',type=int,choices=range(1,8),default=1)
 ap.add_argument('--play',type=int,default=120);ap.add_argument('--continue-updates',type=int,default=120)
 a=ap.parse_args();assert 1<=a.play<=600 and 1<=a.continue_updates<=600
 for path,expect in ((a.rom,ROM_SHA),(a.elf,ELF_SHA),(a.runner,RUNNER_SHA)):
  if sha(path)!=expect:raise ValueError('unapproved input hash: '+str(path))
 a.output.mkdir(parents=True,exist_ok=False); result={'mission':a.mission,'play':a.play,'continue_updates':a.continue_updates,'rom_sha256':ROM_SHA,'elf_sha256':ELF_SHA,'runner_sha256':RUNNER_SHA,'transport':'physical buttons; guest reads only','ok':False};sram=a.output/'phase1.sav'
 try:
  g=PhysicalGame(a.runner,a.rom,a.elf,a.output/'phase1',save_output=sram)
  try:
   g.step(0,2);assert g.u32('screen')==SCREEN_TITLE
   g.tap(KEY_A)
   for _ in range((a.mission-1)//2):g.tap(KEY_RIGHT)
   if (a.mission-1)%2:g.tap(KEY_DOWN)
   assert g.front_field('mission_page')==(a.mission-1)//2 and g.front_field('row')==(a.mission-1)%2
   g.tap(KEY_A);expected=g.front_field('mission');g.tap(KEY_START)
   assert g.u32('screen')==SCREEN_PLAY and g.u32('race_mode')==0
   result['selected_mission']=g.u32('race_selected_mission');assert result['selected_mission']==expected
   g.step(0,a.play);assert g.u32('screen')==SCREEN_PLAY
   result['pause_attempts']=[]
   for _ in range(16):
    g.tap(KEY_START);screen=g.u32('screen');result['pause_attempts'].append(screen)
    if screen==SCREEN_PAUSE:break
    if screen!=SCREEN_PLAY:raise RuntimeError('unexpected screen while attempting pause')
    g.tap(KEY_A) # ordinary acknowledgement if a modal blocks START
   if g.u32('screen')!=SCREEN_PAUSE:raise RuntimeError('pause unavailable after bounded acknowledgments')
   g.step(KEY_B);notice=g.rsp.read(g.sym['pause_notice'],1)[0];result['save_notice']=notice;result['save_status']=('saved' if notice&0xc0==0x40 else 'failed' if notice&0xc0==0x80 else 'other')
   result['saved']=snapshot(g);g.rsp.export_sram()
   if result['save_status']!='saved':return 2
   g.step(0);g.step(KEY_A);g.step(0,a.continue_updates);result['reference']=snapshot(g)
  finally:g.close()
  original=sram.read_bytes();assert len(original)==32768;result['sram_sha256']=sha(sram)
  after=a.output/'phase2.sav';g=PhysicalGame(a.runner,a.rom,a.elf,a.output/'phase2',save_input=sram,save_output=after)
  try:
   g.step(0,2);g.step(KEY_L);result['loaded']=(g.u32('screen')==SCREEN_PLAY and g.u32('race_mode')==0)
   result['restored']=snapshot(g)
   if not result['loaded']:return 3
   g.step(0,a.continue_updates);result['resumed']=snapshot(g);g.rsp.export_sram()
  finally:g.close()
  result['sram_unchanged']=after.read_bytes()==original
  result['checks']={'saved_matches_restored':result['saved']==result['restored'],
     'continuation_matches':result['reference']==result['resumed'],
     'sram_unchanged':result['sram_unchanged']}
  result['ok']=all(result['checks'].values());return 0 if result['ok'] else 4
 except BaseException as exc:
  result['error']=type(exc).__name__+': '+str(exc);raise
 finally:(a.output/'result.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':sys.exit(main())
