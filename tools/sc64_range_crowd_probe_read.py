#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Physical zero-input crowded skirmish sampler; no guest writes or SRAM."""
import argparse,hashlib,json,statistics,subprocess
from pathlib import Path
from emulator_smoke import symbol_sizes,symbols
from sc64_headless_physical import PhysicalReplay

def main():
 p=argparse.ArgumentParser()
 for name in ('runner','rom','elf','output'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--ticks',type=int,default=500);p.add_argument('--initial',type=int,required=True)
 a=p.parse_args();assert 1<=a.ticks<=10000
 a.output.mkdir(parents=True,exist_ok=False)
 syms,sizes=symbols(a.elf),symbol_sizes(a.elf)
 names=('demo_frame','demo_last_cycles','demo_update_cycles','probe_tick','probe_fault','probe_world_hash','probe_initial_alive','probe_alive','probe_moving','probe_moves','probe_attacks','probe_damage','probe_owner0','probe_owner1','probe_owner2','probe_owner3')
 for n in names:
  if sizes.get(n)!=4:raise ValueError('missing four-byte symbol '+n)
 samples=[];error=None;proc=None;log=None
 try:
  proc,replay,log=PhysicalReplay.start(a.runner,a.rom,a.output,syms,sizes)
  for tick in range(1,a.ticks+1):
   replay.play_to(tick,[])
   row={n:int.from_bytes(replay.read(syms[n],4),'little') for n in names};samples.append(row)
   if row['demo_frame']!=tick or row['probe_tick']!=tick or row['probe_fault'] or row['probe_initial_alive']!=a.initial:
    raise RuntimeError(f'invalid scene at tick {tick}: {row}')
 except Exception as exc:error=repr(exc)
 finally:
  if proc:
   proc.terminate()
   try:proc.wait(timeout=5)
   except subprocess.TimeoutExpired:proc.kill();proc.wait()
  if log:log.close()
 cycles=[r['demo_last_cycles'] for r in samples]
 result={'scope':'authored real-terrain zero-input scene, game update/audio/render and present wait plus guest measurement overhead; emulated ARM cycles, not hardware FPS','hashes':{n:hashlib.sha256(x.read_bytes()).hexdigest() for n,x in [('runner',a.runner),('rom',a.rom),('elf',a.elf)]},'samples':samples,'error':error}
 if cycles:
  result.update(cycles_mean=statistics.mean(cycles),cycles_peak=max(cycles),cycles_p95=sorted(cycles)[(95*len(cycles)+99)//100-1],above_two_refresh_frames=sum(c>561792 for c in cycles),initial_world_hash=samples[0]['probe_world_hash'],final={n:samples[-1][n] for n in names if n.startswith('probe_')})
 (a.output/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:v for k,v in result.items() if k!='samples'},sort_keys=True),flush=True)
 if error:raise SystemExit(1)
if __name__=='__main__':main()
