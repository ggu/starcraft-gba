#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded read-only actor observer; commands are steps mask x y capture."""
import argparse,fcntl,hashlib,json,os,pathlib,resource,selectors,shutil,signal,subprocess,sys,time

ROM_SHA='e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
VK_SHA='432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99'
REV='0aafd85789215e84e1e43415c07d4c88461b7899'
BINARY_SHA='3f779db1c809a8de385c85e1581f6e172a51ef5c0747373e3eb799ea153342b3'
LOCK='/tmp/sc64-native-build-20261031.lock'
VK_PATH=pathlib.Path('/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib')

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def require_binary(path):
 if sha(path)!=BINARY_SHA: raise ValueError('observer binary does not match qualified probe')

def output_ancestor(path):
 p=path.parent
 while not p.exists() and p!=p.parent:p=p.parent
 if not p.is_dir():raise ValueError('output parent is not a directory')
 return p

def check_reserve(path):
 if shutil.disk_usage(output_ancestor(path)).free<5*1024**3:raise ValueError('output filesystem disk reserve')

def clean(source):
 return subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()==REV and not subprocess.check_output(['git','-C',str(source),'status','--porcelain'],text=True).strip()

def terminate_group(child):
 if child is None:return
 try:os.killpg(child.pid,signal.SIGTERM)
 except ProcessLookupError:pass
 try:child.wait(timeout=1)
 except subprocess.TimeoutExpired:pass
 try:os.killpg(child.pid,signal.SIGKILL)
 except ProcessLookupError:pass
 if child.poll() is None:
  try:child.wait(timeout=2)
  except subprocess.TimeoutExpired:raise RuntimeError('observer did not terminate')
 for stream in (child.stdin,child.stdout,child.stderr):
  if stream is not None:stream.close()

def relay(cmd,cwd,env,log,err,result,selector_factory=selectors.DefaultSelector):
 """Own one process group; cleanup also runs on selector errors and signals."""
 child=None;sel=None;started=time.monotonic()
 def terminate(signum,_frame):raise SystemExit(128+signum)
 oldterm=signal.signal(signal.SIGTERM,terminate)
 oldmask=signal.pthread_sigmask(signal.SIG_BLOCK,{signal.SIGINT,signal.SIGTERM})
 try:
  def setup():
   resource.setrlimit(resource.RLIMIT_FSIZE,(8*1024**2,8*1024**2))
   signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
  child=subprocess.Popen(cmd,cwd=cwd,env=env,stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,bufsize=0,preexec_fn=setup,start_new_session=True)
  signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
  sel=selector_factory();sel.register(sys.stdin.buffer,selectors.EVENT_READ,'input');sel.register(child.stdout,selectors.EVENT_READ,'output');sel.register(child.stderr,selectors.EVENT_READ,'error')
  deadline=started+180;written={'output':0,'error':0}
  while child.poll() is None and time.monotonic()<deadline:
   for key,_ in sel.select(min(1,max(0,deadline-time.monotonic()))):
    data=os.read(key.fileobj.fileno(),4096)
    if key.data=='input':
     if not data:sel.unregister(key.fileobj);child.stdin.close()
     else:
      try:child.stdin.write(data);child.stdin.flush()
      except BrokenPipeError:sel.unregister(key.fileobj)
    elif data:
     written[key.data]+=len(data)
     if written[key.data]>(1024**2 if key.data=='output' else 2*1024**2):
      result['log_budget_exceeded']=True;break
     if key.data=='output':log.write(data);log.flush();sys.stdout.buffer.write(data);sys.stdout.buffer.flush()
     else:err.write(data);err.flush()
    else:sel.unregister(key.fileobj)
   if result.get('log_budget_exceeded'):break
  if child.poll() is None and not result.get('log_budget_exceeded'):result['timeout']=True
 finally:
  signal.pthread_sigmask(signal.SIG_BLOCK,{signal.SIGINT,signal.SIGTERM})
  try:
   if sel is not None:sel.close()
   terminate_group(child)
  finally:
   signal.signal(signal.SIGTERM,oldterm)
   signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
 if child is None:raise RuntimeError('observer failed to start')
 result['returncode']=child.returncode;result['seconds']=round(time.monotonic()-started,3)

def main(argv=None):
 p=argparse.ArgumentParser()
 for arg in ('binary','source','moltenvk','rom','output'):p.add_argument('--'+arg,type=pathlib.Path,required=True)
 a=p.parse_args(argv)
 if not clean(a.source) or sha(a.rom)!=ROM_SHA or a.moltenvk.resolve()!=VK_PATH or sha(a.moltenvk)!=VK_SHA:raise SystemExit('input provenance mismatch')
 require_binary(a.binary)
 if a.output.exists():raise SystemExit('output must be new')
 with open(LOCK,'a') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX)
  check_reserve(a.output);require_binary(a.binary)
  if a.output.exists() or not clean(a.source) or sha(a.rom)!=ROM_SHA or a.moltenvk.resolve()!=VK_PATH or sha(a.moltenvk)!=VK_SHA:raise SystemExit('input provenance or output changed while queued')
  a.output.mkdir(parents=True);[(a.output/x).mkdir() for x in ('input','saves','systems')];copy=a.output/'input/game.z64';shutil.copyfile(a.rom,copy)
  if sha(copy)!=ROM_SHA:raise SystemExit('ROM copy mismatch')
  sources=[pathlib.Path(__file__).resolve(),pathlib.Path(__file__).with_name('actor_observer.cpp').resolve(),pathlib.Path(__file__).with_name('input_mapping.h').resolve(),pathlib.Path(__file__).with_name('observer_read.h').resolve(),pathlib.Path(__file__).with_name('inject.cmake').resolve()]
  resources=[a.source/'ares/ares/resource/resource.cpp',a.source/'ares/ares/resource/resource.hpp',a.source/'mia/resource/resource.cpp',a.source/'mia/resource/resource.hpp'];inputs={str(x):sha(x) for x in sources+resources};binary=sha(a.binary)
  cmd=[str(a.binary.resolve()),str(copy.resolve()),str(a.output.resolve()),'interactive','14000'];result={'command':cmd,'binary_sha256':binary,'input_sha256':inputs,'rom_sha256':ROM_SHA,'moltenvk_sha256':VK_SHA,'source_commit':REV}
  with (a.output/'bridge.log').open('wb') as log,(a.output/'stderr.log').open('wb') as err:
   relay(cmd,a.output.resolve(),{**os.environ,'DYLD_LIBRARY_PATH':str(a.moltenvk.resolve().parent)},log,err,result)
  result['unchanged']=clean(a.source) and sha(a.rom)==ROM_SHA and sha(copy)==ROM_SHA and sha(a.moltenvk)==VK_SHA and sha(a.binary)==binary==BINARY_SHA and all(sha(pathlib.Path(k))==v for k,v in inputs.items())
  files=sorted(a.output.glob('live-*.argb'));result['frames']={x.name:sha(x) for x in files};result['frame_bytes']=sum(x.stat().st_size for x in files);result['ok']=result['returncode']==0 and result['unchanged'] and len(files)<=32 and result['frame_bytes']<=2*1024**2
  ram=sorted(a.output.glob('ram-*.bin'));result['ram']={x.name:sha(x) for x in ram};result['ram_bytes']=sum(x.stat().st_size for x in ram);result['ok']=result['ok'] and len(ram)==2 and all(x.stat().st_size==8*1024**2 for x in ram) and result['ram_bytes']<=16*1024**2
  for name in ('ram.csv','commands.csv','inputs.csv','callbacks.csv','status.log'):result[name+'_sha256']=sha(a.output/name) if (a.output/name).is_file() else None
  result['output_bytes']=sum(x.stat().st_size for x in a.output.rglob('*') if x.is_file() and not x.is_symlink());result['ok']=result['ok'] and result['output_bytes']<=63*1024**2 and not result.get('log_budget_exceeded',False)
  (a.output/'manifest.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'ok':result['ok'],'returncode':result['returncode'],'frames':len(files)}),flush=True)
  return 0 if result['ok'] else 1

if __name__=='__main__':sys.exit(main())
