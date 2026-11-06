#!/usr/bin/env python3
"""Bounded ordinary-button early Protoss save/load probe; GPL-3.0-or-later.
Uses the production ROM and physical backend; only guest reads are used.
The result covers one save point, not a mission win or arbitrary-save support.
"""
import argparse,fcntl,hashlib,json,os,shutil,signal,subprocess,sys
from pathlib import Path
from sc64_skirmish_sram_continuation import (PhysicalGame,SCREEN_TITLE,SCREEN_PLAY,SCREEN_PAUSE,
 KEY_A,KEY_B,KEY_START,KEY_RIGHT,KEY_DOWN,KEY_L,fingerprint)
ROM_SHA='ff94a2ea9f98404c97f44a8b6b7bf557b3572c761de530d867032eeb8f76b10b'
ELF_SHA='f961647217516de367b2f2e28b1b82dee4e744e3bef7f9c7b0f46e4100f55587'
RUNNER_SHA='4199fe903b18c77e0833db1860c4968509cfc82e506bc6f2f9be7582b720fae2'
LOCK_PATH='/tmp/sc64-native-build-20261031.lock'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def snapshot(g):
 state=g.state();state.pop('screen');state['mission']=g.u32('race_selected_mission');state['race_mode']=g.u32('race_mode')
 return {'frame':state['frame'],'mission':state['mission'],'mode':state['race_mode'],
         'units':len(state['units']),'buildings':len(state['buildings']),'fingerprint':fingerprint(state)}
def bounded_updates(value):
 try: value=int(value)
 except ValueError as exc: raise argparse.ArgumentTypeError('expected an integer') from exc
 if not 1<=value<=600: raise argparse.ArgumentTypeError('must be between 1 and 600')
 return value

def validate(a):
 if a.output.exists(): raise ValueError('output already exists: '+str(a.output))
 ancestor=a.output.parent
 while not ancestor.exists() and ancestor!=ancestor.parent: ancestor=ancestor.parent
 if shutil.disk_usage(ancestor).free<4294967296: raise ValueError('less than 4 GiB free at output')
 for path,expect in ((a.rom,ROM_SHA),(a.elf,ELF_SHA),(a.runner,RUNNER_SHA)):
  if sha(path)!=expect:raise ValueError('unapproved input hash: '+str(path))

def run(a):
 validate(a)
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
def main(argv=None):
 ap=argparse.ArgumentParser()
 for arg in ('rom','elf','runner','output'):ap.add_argument('--'+arg,type=Path,required=True)
 ap.add_argument('--mission',type=int,choices=range(1,8),default=1)
 ap.add_argument('--play',type=bounded_updates,default=120)
 ap.add_argument('--continue-updates',type=bounded_updates,default=120)
 ap.add_argument('--internal-worker',action='store_true',help=argparse.SUPPRESS)
 a=ap.parse_args(argv)
 if a.internal_worker:
  try:
   fd=int(os.environ.get('SC64_EARLY_PROBE_LOCK_FD','-1'))
   actual=os.fstat(fd); expected=os.stat(LOCK_PATH)
   if (actual.st_dev,actual.st_ino)!=(expected.st_dev,expected.st_ino):raise ValueError('wrong lock file')
   fcntl.flock(fd,fcntl.LOCK_EX|fcntl.LOCK_NB)
  except (ValueError,OSError):ap.error('internal worker requires the held shared lock')
  return run(a)
 with open(LOCK_PATH,'w') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX)
  try: validate(a)
  except (OSError,ValueError) as exc:ap.error(str(exc))
  env=os.environ.copy();env['SC64_EARLY_PROBE_LOCK_FD']=str(lock.fileno())
  args=[sys.executable,str(Path(__file__).resolve()),'--internal-worker',
        '--rom',str(a.rom),'--elf',str(a.elf),'--runner',str(a.runner),
        '--output',str(a.output),'--mission',str(a.mission),'--play',str(a.play),
        '--continue-updates',str(a.continue_updates)]
  proc=subprocess.Popen(args,env=env,start_new_session=True,pass_fds=(lock.fileno(),))
  try:return proc.wait(timeout=240)
  except subprocess.TimeoutExpired:
   try:os.killpg(proc.pid,signal.SIGTERM)
   except ProcessLookupError:pass
   try:proc.wait(timeout=5)
   except subprocess.TimeoutExpired:
    try:os.killpg(proc.pid,signal.SIGKILL)
    except ProcessLookupError:pass
    try:proc.wait(timeout=5)
    except subprocess.TimeoutExpired:pass
   print('physical probe timed out after 240 seconds',file=sys.stderr)
   return 124
if __name__=='__main__':sys.exit(main())
