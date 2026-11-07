#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Normal-SRAM Z9/Z10 HUD pair using only physical buttons and read-only state.

Runs under its own shared native lock. Compares each physical update's full
visible Mode-4 page, palette, named world and UI state. No guest writes.
"""
import argparse
import fcntl
import hashlib
import json
from pathlib import Path
import resource
import shutil
import signal
import subprocess
import sys
import sc64_skirmish_sram_continuation as world
LOCK='/tmp/sc64-native-build-20261031.lock';RESERVE=4*1024**3;LIMIT=64*1024**2
RUNNER='4199fe903b18c77e0833db1860c4968509cfc82e506bc6f2f9be7582b720fae2'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def alarm(_signal,_frame):raise TimeoutError('normal HUD pair monitored deadline')
def video(g):
 d=int.from_bytes(g.rsp.read(0x04000000,2),'little')
 if d&7!=4 or d&0x1000:raise ValueError('unsupported display mode or OBJ')
 return [d,hashlib.sha256(g.rsp.read(0x06000000+((d>>4)&1)*0xa000,38400)).hexdigest(),hashlib.sha256(g.rsp.read(0x05000000,512)).hexdigest()]
def snap(g):
 s=g.state()
 for key in ('selected_building','menu_open','menu_cursor','mode','toast_timer','toast_id','race_selected_mission'):
  if g.sizes.get(key)!=4:raise ValueError('missing UI symbol '+key)
  s[key]=g.u32(key)
 s['visible_sha256']=hashlib.sha256(g.rsp.read(g.sym['visible'],g.sizes['visible'])).hexdigest()
 s['explored_sha256']=hashlib.sha256(g.rsp.read(g.sym['explored'],g.sizes['explored'])).hexdigest()
 return s
def valid_sram(p):
 data=p.read_bytes()
 if len(data)!=32768:raise ValueError('wrong SRAM length')
 found=[]
 for off in (0,16384):
  tag=int.from_bytes(data[off:off+4],'little');n=int.from_bytes(data[off+4:off+8],'little');check=int.from_bytes(data[off+8:off+12],'little');seq=int.from_bytes(data[off+12:off+16],'little')
  if not tag or tag==0xffffffff or not 0<n<=16368 or not seq:continue
  h=2166136261
  for byte in data[off+16:off+16+n]:h=((h^byte)*16777619)&0xffffffff
  if h==check:found.append({'slot':off//16384,'tag':tag,'length':n,'sequence':seq})
 if not found:raise ValueError('no checksum-valid committed SRAM record')
 return found

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ('off_rom','off_elf','on_rom','on_elf','runner','blank','receipt','output'):ap.add_argument('--'+name.replace('_','-'),type=Path,required=True)
 ap.add_argument('--profile',choices=('z9','z10'),required=True);ap.add_argument('--mode',choices=('custom','campaign'),required=True)
 ap.add_argument('--driver-head',required=True);ap.add_argument('--deadline',type=int,default=600)
 a=ap.parse_args();paths=(a.off_rom,a.off_elf,a.on_rom,a.on_elf,a.runner,a.blank,a.receipt)
 if a.output.exists() or not 1<=a.deadline<=1200:ap.error('invalid output/deadline')
 if sha(a.runner)!=RUNNER or a.blank.read_bytes()!=bytes([255])*32768:ap.error('runner or erased SRAM differs')
 pins={str(x.resolve()):sha(x) for x in paths};receipt=json.loads(a.receipt.read_text())
 if not receipt.get('unchanged') or not receipt.get('vectors_verified',{}).get(a.profile):raise ValueError('unverified build receipt')
 for variant,rom,elf in (('off',a.off_rom,a.off_elf),('on',a.on_rom,a.on_elf)):
  for kind,p in (('rom',rom),('elf',elf)):
   record=receipt.get('products',{}).get(a.profile,{}).get(variant,{}).get(kind,{})
   if record!={'path':str(p.resolve()),'sha256':sha(p)}:raise ValueError('wrong product role')
 def check():
  if subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()!=a.driver_head or subprocess.run(['git','diff','--quiet','HEAD','--']).returncode:raise ValueError('source changed')
  changed=set(subprocess.check_output(['git','diff','--name-only',receipt['head'],'HEAD','--'],text=True).splitlines())
  if not changed.issubset({'tools/sc64_zerg_normal_hud_pair.py'}):raise ValueError('product source differs from build')
  if any(sha(Path(p))!=v for p,v in pins.items()) or shutil.disk_usage(a.output.parent).free<RESERVE:raise ValueError('input or disk changed')
 resource.setrlimit(resource.RLIMIT_FSIZE,(LIMIT,LIMIT));old=signal.signal(signal.SIGALRM,alarm);term=signal.signal(signal.SIGTERM,alarm);signal.alarm(a.deadline)
 try:
  with open(LOCK,'a') as lock:
   fcntl.flock(lock,fcntl.LOCK_EX);check();a.output.mkdir(parents=True);games=[];records=[];diag={'profile':a.profile,'mode':a.mode,'stage':'starting'}
   try:
    for name,rom,elf in [('off',a.off_rom,a.off_elf),('on',a.on_rom,a.on_elf)]:
     folder=a.output/name;folder.mkdir();games.append(world.PhysicalGame(a.runner,rom,elf,folder,save_input=a.blank,save_output=folder/'guest.sav'))
    def step(key):
     for g in games:g.step(key)
     states=[snap(g) for g in games];images=[video(g) for g in games]
     diag.update(stage='step',key=key,states=states,views=images)
     if states[0]!=states[1] or images[0]!=images[1]:raise ValueError('OFF/ON state or displayed frame mismatch')
     records.append({'frame':states[0]['frame'],'screen':states[0]['screen'],'view':images[0],'selected_building':states[0]['selected_building'],'menu_open':states[0]['menu_open']})
     if len(records)%50==0:check()
     if sum(p.stat().st_size for p in a.output.rglob('*') if p.is_file())>LIMIT:raise ValueError('output limit')
    def tap(key):step(key);step(0)
    def front(name,index=0):
     x=[g.front_field(name,index) for g in games]
     if x[0]!=x[1]:raise ValueError('lobby differs')
     return x[0]
    if a.mode=='custom':
     tap(128);tap(1)
     if front('page')!=world.FRONT_SKIRMISH:raise ValueError('not in custom lobby')
     tap(128)
     for _ in range(3):
      if front('race')==1:break
      tap(16)
     if front('race')!=1:raise ValueError('cannot select Zerg')
     tap(128)
     for _ in range(3):
      if front('bots')==1:break
      tap(32)
     if front('bots')!=1:raise ValueError('cannot select one bot')
     tap(128)
     for _ in range(3):
      if front('bot_races',0)==0:break
      tap(16)
     if front('bot_races',0)!=0:raise ValueError('cannot select Terran opponent')
     while front('row')<4:tap(128)
     tap(1)
    else:
     tap(1)
     if front('page')!=2:raise ValueError('not in campaign selector')
     for _ in range(4):tap(16)
     if front('mission_page')!=4:raise ValueError('late campaign page unavailable')
     if a.profile=='z10':tap(128)
     tap(1)
     if front('page')!=3:raise ValueError('briefing not reached')
     tap(8)
    state=snap(games[0]);target=20 if a.profile=='z9' else 21
    if state['screen']!=world.SCREEN_PLAY or (a.mode=='custom' and (state['race_mode'],state['race_human'],state['race_bots'])!=(1,1,1)) or (a.mode=='campaign' and (state['race_mode']!=0 or state['race_selected_mission']!=target)):raise ValueError('requested scene did not start')
    # Advance and handle visible text with ordinary A taps; do not inspect enemies.
    for _ in range(600):
     if int.from_bytes(games[0].rsp.read(0x06013fe0,2),'little'):tap(1)
     else:step(0)
    # Exercise camera/cursor movement and the ordinary command-menu overlay.
    for _ in range(30):step(16)
    step(0);tap(1);tap(2)
    if snap(games[0])['menu_open']!=1:raise ValueError('command menu did not open')
    tap(2)
    if snap(games[0])['menu_open']!=0:raise ValueError('command menu did not close')
    # Save only the actual running game via its pause menu.
    tap(8)
    if snap(games[0])['screen']!=world.SCREEN_PAUSE:raise ValueError('pause not reached')
    tap(2);saved=[snap(g) for g in games]
    for g in games:g.rsp.export_sram()
    saves=[a.output/name/'guest.sav' for name in ('off','on')]
    if sha(saves[0])!=sha(saves[1]):raise ValueError('saved SRAM differs')
    records_valid=[valid_sram(p) for p in saves]
    for g in reversed(games):g.close()
    games=[]
    for name,rom,elf,save in [('off',a.off_rom,a.off_elf,saves[0]),('on',a.on_rom,a.on_elf,saves[1])]:
     folder=a.output/(name+'-fresh');folder.mkdir();games.append(world.PhysicalGame(a.runner,rom,elf,folder,save_input=save))
    tap(512);loaded=[snap(g) for g in games]
    if loaded[0]['screen']!=world.SCREEN_PLAY or loaded[0]['frame']<saved[0]['frame']:raise ValueError('fresh SRAM load not reached')
    for _ in range(60):step(0)
    check();report={'source':a.driver_head,'compiled_source':receipt['head'],'profile':a.profile,'mode':a.mode,'pins':pins,'sram_sha256':sha(saves[0]),'records':records_valid,'saved_ui':{k:saved[0][k] for k in ('frame','screen','selected_building','menu_open','menu_cursor')},'loaded_ui':{k:loaded[0][k] for k in ('frame','screen','selected_building','menu_open','menu_cursor')},'steps':len(records),'comparisons':records,'scope':'physical input, full displayed indexed frame/palette and named world/UI at each step; cross-path selection reset recorded, not equated'}
    (a.output/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'steps':len(records),'saved_ui':report['saved_ui'],'loaded_ui':report['loaded_ui']}))
   except Exception as exc:
    diag['error']=str(exc);(a.output/'failure.json').write_text(json.dumps(diag,indent=2)+'\n');raise
   finally:
    for g in reversed(games):
     try:g.close()
     except Exception:pass
 finally:signal.alarm(0);signal.signal(signal.SIGALRM,old);signal.signal(signal.SIGTERM,term)
if __name__=='__main__':main()
