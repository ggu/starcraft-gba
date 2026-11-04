#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build two independent contact fixtures against pinned OpenBW and P10 flags.

Inputs: pinned OpenBW checkout, normal product compiler argv JSON and prepared
private assets. No assets are copied into the source tree by this script.
"""
import argparse, fcntl, hashlib, json, os, pathlib, re, shutil, subprocess
p=argparse.ArgumentParser()
p.add_argument('--openbw', required=True); p.add_argument('--p10-argv', required=True); p.add_argument('--bwapi-unit-type', required=True); p.add_argument('--out', required=True)
a=p.parse_args(); root=pathlib.Path(__file__).resolve().parent.parent; ref=pathlib.Path(a.openbw).resolve(); out=pathlib.Path(a.out).resolve()
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=ref,text=True).strip()=='4b046d5f65302b10cb0a745f0fecd37ec85b20a8'
bw=pathlib.Path(a.bwapi_unit_type).read_bytes(); assert hashlib.sha256(bw).hexdigest()=='7f16832dac82b8a739a07616ec0e61bc2c68a34b873f779cca8742de4832c76b'
part=bw.decode().split('unitDimensions[UnitTypes::Enum::MAX] = {',1)[1].split('};',1)[0]
rows=[tuple(map(int,x.split(','))) for x in re.findall(r'\{\s*([\d,\s]+)\}',part)]
assert rows[19][2:]==(16,16,15,15) and rows[67][2:]==(12,10,11,13)
assert not out.exists(); argv=json.load(open(a.p10_argv)); flags=[v for v in argv[1:] if v.startswith(('-D','-I'))]
sources=[v for v in argv if v.endswith('.c') and v not in ('common/main.c','common/memory.c','starcraft/game.c')]
print('PID',os.getpid(),flush=True)
with open('/tmp/sc64-native-build-20261031.lock','a+') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX); print('LOCK ACQUIRED',flush=True)
 if shutil.disk_usage(root).free<2*1024**3: raise SystemExit('disk guard')
 out.mkdir(parents=True)
 with (out/'build.log').open('w') as log:
  subprocess.run(['clang++','-std=c++17','-O1','-I'+str(ref),'tests/sc64_openbw_contact_oracle.cpp','-o',str(out/'openbw')],cwd=root,check=True,stdout=log,stderr=log)
  subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-fsanitize=address,undefined','-DGBA_HOST',*flags,'tests/sc64_gba_contact_oracle.c',*sources,'-o',str(out/'gba')],cwd=root,check=True,stdout=log,stderr=log)
  for name in ('openbw','gba'):
   with (out/(name+'.csv')).open('w') as rows: subprocess.run([str(out/name)],cwd=root,check=True,stdout=rows,stderr=log)
  source=dict(line.split(',') for line in (out/'openbw.csv').read_text().splitlines())
  gba=dict(line.split(',') for line in (out/'gba.csv').read_text().splitlines())
  assert {k:(source[k],gba[k]) for k in gba} == {'approach':('1','0'),'escape':('0','0'),'south':('1','0'),'overlap_closer':('0','1'),'overlap_escape':('0','0')}
  print('openbw',source,'gba',gba,flush=True)
