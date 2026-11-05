#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare baseline/table host contact or Showcase-scene byte streams.

Needs prepared profile inputs, generated contact table and a captured normal
P10 GCC argv JSON. Outputs are private; no native assets are published.
"""
import argparse, fcntl, hashlib, json, os, pathlib, shlex, shutil, subprocess
p=argparse.ArgumentParser();p.add_argument('--profile',choices=('p10','showcase'),required=True);p.add_argument('--fixture',choices=('contact','scenes'),required=True);p.add_argument('--p10-argv');p.add_argument('--out',required=True);a=p.parse_args()
root=pathlib.Path(__file__).resolve().parent.parent;out=pathlib.Path(a.out).resolve()
if out.exists():p.error('output directory must be new')
if a.profile=='p10':
 if not a.p10_argv:p.error('--p10-argv is required for P10')
 vector=json.load(open(a.p10_argv));flags=[x for x in vector[1:] if x.startswith(('-D','-I'))];sources=[x for x in vector if x.endswith('.c') and x not in ('common/main.c','common/memory.c','starcraft/game.c')]
else:
 make=subprocess.check_output(['make','-pn'],cwd=root,text=True)
 line=next(line for line in make.splitlines() if line.startswith('SHOWCASE_FLAGS :='));flags=shlex.split(line.split(':=',1)[1]);sources=['common/gba.c','starcraft/sc64_bc_body_art.c','starcraft/sc64_interceptor_motion.c','starcraft/sc64_interceptor_pool_codec.c','starcraft/sc64_interceptor_orders.c']
if a.fixture=='scenes' and a.profile!='showcase':p.error('scenes requires Showcase')
flags=[x for x in flags if x!='-DSC64_CONTACT_MASK_TABLE'];source='tests/sc64_contact_cache.c' if a.fixture=='contact' else 'tests/sc64_showcase_scene_trace.c';envname='SC64_CONTACT_TRACE_PATH' if a.fixture=='contact' else 'SC64_SHOWCASE_TRACE_PATH'
with open('/tmp/sc64-native-build-20261031.lock','a+') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX)
 if shutil.disk_usage(root).free<2*1024**3:raise SystemExit('less than 2 GiB free')
 out.mkdir(parents=True)
 with (out/'build-and-run.log').open('w') as log:
  for mode in ('baseline','table'):
   if shutil.disk_usage(root).free<2*1024**3:raise SystemExit('less than 2 GiB free')
   extra=['-DSC64_CONTACT_MASK_TABLE','-Ibuild/generated'] if mode=='table' else []
   exe=out/mode;trace=out/(mode+'.bin');cmd=['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-fsanitize=address,undefined','-DGBA_HOST','-Icommon',*flags,*extra,source,*sources,'-o',str(exe)]
   subprocess.run(cmd,cwd=root,check=True,stdout=log,stderr=log);env=dict(os.environ,**{envname:str(trace)});subprocess.run([str(exe)],cwd=root,env=env,check=True,stdout=log,stderr=log)
 left=(out/'baseline.bin').read_bytes();right=(out/'table.bin').read_bytes();print('bytes',len(left),'equal',left==right,'sha256',hashlib.sha256(left).hexdigest());
 if left!=right:raise SystemExit(1)
