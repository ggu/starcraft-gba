#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the experimental ares frontend with an isolated ROM copy and save path."""
import argparse,fcntl,hashlib,json,os,pathlib,shutil,subprocess,sys
P=argparse.ArgumentParser();P.add_argument('--binary',type=pathlib.Path,required=True);P.add_argument('--source',type=pathlib.Path,required=True);P.add_argument('--moltenvk',type=pathlib.Path,required=True);P.add_argument('--rom',type=pathlib.Path,required=True);P.add_argument('--output',type=pathlib.Path,required=True);P.add_argument('--scenario',choices=('neutral','tap','start','mission','advance','calibrate','select','select2','select3','select4','build','grid','custom','customstart','protosssetup','protossstart','protossselect','probe2','probe3'),required=True);P.add_argument('--frames',type=int,default=24);a=P.parse_args()
if not 1<=a.frames<=9000:P.error('frames must be 1..9000')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
expected='e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
expected_moltenvk='432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99'
def validate():
 if subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD'],text=True).strip()!='0aafd85789215e84e1e43415c07d4c88461b7899':raise SystemExit('wrong source revision')
 if subprocess.check_output(['git','-C',str(a.source),'status','--porcelain'],text=True).strip():raise SystemExit('source checkout is dirty')
 if sha(a.rom)!=expected:raise SystemExit('wrong owned ROM')
 if a.moltenvk.resolve()!=pathlib.Path('/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib'):raise SystemExit('wrong MoltenVK path')
 if sha(a.moltenvk)!=expected_moltenvk:raise SystemExit('wrong MoltenVK binary')
 if a.output.exists():raise SystemExit('output must be new')
validate()
with open('/tmp/sc64-native-build-20261031.lock','w') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX)
 validate()
 parent=a.output.resolve().parent
 while not parent.exists(): parent=parent.parent
 if shutil.disk_usage(parent).free < 5*1024**3: raise SystemExit('disk reserve')
 a.output.mkdir(parents=True);(a.output/'saves').mkdir();(a.output/'systems').mkdir();(a.output/'input').mkdir();copy=a.output/'input/game.z64';shutil.copyfile(a.rom,copy)
 if sha(copy)!=expected:raise SystemExit('ROM copy hash mismatch')
 cmd=[str(a.binary.resolve()),str(copy.resolve()),str(a.output.resolve()),a.scenario,str(a.frames)]
 resources=[a.source/'ares/ares/resource/resource.cpp',a.source/'ares/ares/resource/resource.hpp',a.source/'mia/resource/resource.cpp',a.source/'mia/resource/resource.hpp']
 if not all(p.is_file() for p in resources):raise SystemExit('missing generated resources')
 probe_dir=pathlib.Path(__file__).resolve().parent
 probe_inputs={name:sha(probe_dir/name) for name in ('main.cpp','inject.cmake','run.py')}
 r={'probe_source_sha256':probe_inputs,'generated_resource_sha256':{str(p.relative_to(a.source)):sha(p) for p in resources},'moltenvk_sha256':sha(a.moltenvk),'source_commit':'0aafd85789215e84e1e43415c07d4c88461b7899','binary_sha256':sha(a.binary),'rom_sha256':expected,'scenario':a.scenario,'frames_requested':a.frames,'command':cmd,'scope':'experimental emulator callbacks; no OS input, guest-memory access, or savestate injection'}
 try:
  with (a.output/'run.log').open('w') as log:
   try:q=subprocess.run(cmd,cwd=a.output.resolve(),env={**os.environ,'DYLD_LIBRARY_PATH':str(a.moltenvk.resolve().parent)},stdout=log,stderr=subprocess.STDOUT,timeout=180);r['returncode']=q.returncode
   except subprocess.TimeoutExpired:r['returncode']=None;r['error']='timeout'
 finally:
  r['original_rom_unchanged']=sha(a.rom)==expected;r['copied_rom_unchanged']=sha(copy)==expected
  r['binary_unchanged']=sha(a.binary)==r['binary_sha256'];r['moltenvk_unchanged']=sha(a.moltenvk)==expected_moltenvk
  r['probe_source_unchanged']={name:sha(probe_dir/name) for name in probe_inputs}==probe_inputs
  r['generated_resources_unchanged']={str(p.relative_to(a.source)):sha(p) for p in resources}==r['generated_resource_sha256']
  r['upstream_tracked_unchanged']=(subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD'],text=True).strip()==r['source_commit'] and not subprocess.check_output(['git','-C',str(a.source),'status','--porcelain'],text=True).strip())
  files=sorted(a.output.glob('frame-*.argb'));r['output_bytes']=sum(p.stat().st_size for p in files);r['frame_sha256']={p.name:sha(p) for p in files};r['frames_csv_sha256']=sha(a.output/'frames.csv') if (a.output/'frames.csv').is_file() else None
  r['steps_csv_sha256']=sha(a.output/'steps.csv') if (a.output/'steps.csv').is_file() else None
  r['inputs_csv_sha256']=sha(a.output/'inputs.csv') if (a.output/'inputs.csv').is_file() else None
  r['callbacks_csv_sha256']=sha(a.output/'callbacks.csv') if (a.output/'callbacks.csv').is_file() else None
  r['status_log_sha256']=sha(a.output/'status.log') if (a.output/'status.log').is_file() else None
  r['ok']=r.get('returncode')==0 and r['original_rom_unchanged'] and r['copied_rom_unchanged'] and r['binary_unchanged'] and r['moltenvk_unchanged'] and r['probe_source_unchanged'] and r['generated_resources_unchanged'] and r['upstream_tracked_unchanged'] and 1<=len(files)<=32 and r['output_bytes']<=64*1024**2
  (a.output/'manifest.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({'ok':r['ok'],'returncode':r.get('returncode'),'frames':len(files),'bytes':r['output_bytes']}))
 sys.exit(0 if r['ok'] else 1)
