#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Experimental bounded command bridge; commands are steps mask x y capture."""
import argparse,fcntl,hashlib,json,os,pathlib,selectors,shutil,subprocess,sys,time
p=argparse.ArgumentParser()
for arg in ('binary','source','moltenvk','rom','output'):p.add_argument('--'+arg,type=pathlib.Path,required=True)
a=p.parse_args(); sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
rom='e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'; vk='432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99'; rev='0aafd85789215e84e1e43415c07d4c88461b7899'
def clean():return subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD'],text=True).strip()==rev and not subprocess.check_output(['git','-C',str(a.source),'status','--porcelain'],text=True).strip()
if not clean() or sha(a.rom)!=rom or a.moltenvk.resolve()!=pathlib.Path('/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib') or sha(a.moltenvk)!=vk:raise SystemExit('input provenance mismatch')
if a.output.exists():raise SystemExit('output must be new')
with open('/tmp/sc64-native-build-20261031.lock','w') as lock:
 fcntl.flock(lock,fcntl.LOCK_EX)
 if shutil.disk_usage('.').free<5*1024**3:raise SystemExit('disk reserve')
 if not clean() or sha(a.rom)!=rom or sha(a.moltenvk)!=vk:raise SystemExit('input provenance changed while queued')
 a.output.mkdir(parents=True);[(a.output/x).mkdir() for x in ('input','saves','systems')];copy=a.output/'input/game.z64';shutil.copyfile(a.rom,copy)
 if sha(copy)!=rom:raise SystemExit('ROM copy mismatch')
 sources=[pathlib.Path(__file__).resolve(),pathlib.Path(__file__).with_name('interactive.cpp').resolve(),pathlib.Path(__file__).with_name('inject.cmake').resolve()]
 resources=[a.source/'ares/ares/resource/resource.cpp',a.source/'ares/ares/resource/resource.hpp',a.source/'mia/resource/resource.cpp',a.source/'mia/resource/resource.hpp'];inputs={str(x):sha(x) for x in sources+resources};binary=sha(a.binary)
 cmd=[str(a.binary.resolve()),str(copy.resolve()),str(a.output.resolve()),'interactive','14000']; started=time.monotonic();result={'command':cmd,'binary_sha256':binary,'input_sha256':inputs,'rom_sha256':rom,'moltenvk_sha256':vk,'source_commit':rev}
 with (a.output/'bridge.log').open('wb') as log,(a.output/'stderr.log').open('wb') as err:
  child=subprocess.Popen(cmd,cwd=a.output.resolve(),env={**os.environ,'DYLD_LIBRARY_PATH':str(a.moltenvk.resolve().parent)},stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,bufsize=0)
  sel=selectors.DefaultSelector();sel.register(sys.stdin.buffer,selectors.EVENT_READ,'input');sel.register(child.stdout,selectors.EVENT_READ,'output');deadline=started+180
  while child.poll() is None and time.monotonic()<deadline:
   for key,_ in sel.select(min(1,max(0,deadline-time.monotonic()))):
    data=os.read(key.fileobj.fileno(),4096)
    if key.data=='input':
     if not data:sel.unregister(key.fileobj);child.stdin.close()
     else:
      try:child.stdin.write(data);child.stdin.flush()
      except BrokenPipeError:sel.unregister(key.fileobj)
    elif data:log.write(data);log.flush();sys.stdout.buffer.write(data);sys.stdout.buffer.flush()
    else:sel.unregister(key.fileobj)
  if child.poll() is None:child.kill();result['timeout']=True
  result['returncode']=child.wait();result['seconds']=round(time.monotonic()-started,3)
 result['unchanged']=clean() and sha(a.rom)==rom and sha(copy)==rom and sha(a.moltenvk)==vk and sha(a.binary)==binary and all(sha(pathlib.Path(k))==v for k,v in inputs.items())
 files=sorted(a.output.glob('live-*.argb'));result['frames']={x.name:sha(x) for x in files};result['frame_bytes']=sum(x.stat().st_size for x in files);result['ok']=result['returncode']==0 and result['unchanged'] and len(files)<=32 and result['frame_bytes']<=64*1024**2
 for name in ('commands.csv','inputs.csv','callbacks.csv','status.log'):result[name+'_sha256']=sha(a.output/name) if (a.output/name).is_file() else None
 (a.output/'manifest.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'ok':result['ok'],'returncode':result['returncode'],'frames':len(files)}),flush=True)
 sys.exit(0 if result['ok'] else 1)
