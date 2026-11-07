#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded read-only actor observer; commands are steps mask x y capture."""
import argparse,csv,fcntl,hashlib,json,os,pathlib,resource,selectors,shutil,signal,subprocess,sys,time

COMMAND_SHA='b1943362dd674102b852c5c62d0de329a54cb32d28942bf564e39a26495c0205'
EXPECTED_STEPS=4230
HYBRID_STEP=4200
EXPECTED_FRAME_STEPS=(4209,4229)
ROM_SHA='e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
VK_SHA='432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99'
REV='0aafd85789215e84e1e43415c07d4c88461b7899'
# Qualified independently from the historical non-diagnostic binary.
BINARY_SHA=None
LOCK='/tmp/sc64-native-build-20261031.lock'
VK_PATH=pathlib.Path('/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib')

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def require_binary(path):
 if BINARY_SHA is None or sha(path)!=BINARY_SHA: raise ValueError('instrumented observer binary not qualified')

def output_ancestor(path):
 p=path.parent
 while not p.exists() and p!=p.parent:p=p.parent
 if not p.is_dir():raise ValueError('output parent is not a directory')
 return p

def check_reserve(path):
 if shutil.disk_usage(output_ancestor(path)).free<5*1024**3:raise ValueError('output filesystem disk reserve')

def clean(source):
 return subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()==REV and not subprocess.check_output(['git','-C',str(source),'status','--porcelain'],text=True).strip()

def terminate_child(child):
 if child is None:return
 if child.poll() is None:
  child.terminate()
  try:child.wait(timeout=1)
  except subprocess.TimeoutExpired:
   if child.poll() is None:child.kill()
   try:child.wait(timeout=2)
   except subprocess.TimeoutExpired:raise RuntimeError('observer did not terminate')
 for stream in (child.stdin,child.stdout,child.stderr):
  if stream is not None:stream.close()

def stopped_child(child):
 try:
  event=os.waitid(os.P_PID,child.pid,os.WSTOPPED|os.WNOHANG|os.WNOWAIT)
  return event is not None and event.si_pid==child.pid and event.si_status==signal.SIGSTOP
 except ChildProcessError:return False

def inspect_stopped_child(child,cwd,remaining,result):
 """Inspect only the owned stopped child; never send SIGCONT."""
 result['signal_stop']=True
 if remaining<=2:
  result['fd_inspection']='deadline';return
 path=pathlib.Path(cwd)/'stopped-fds.txt'
 oldmask=signal.pthread_sigmask(signal.SIG_BLOCK,{signal.SIGINT,signal.SIGTERM})
 def limit():
  resource.setrlimit(resource.RLIMIT_FSIZE,(65536,65536))
  signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
 helper=None
 try:
  with path.open('xb') as out:
   helper=subprocess.Popen(['/usr/sbin/lsof','-nP','-a','-p',str(child.pid),'-F','pftasn'],
     stdin=subprocess.DEVNULL,stdout=out,stderr=subprocess.STDOUT,preexec_fn=limit,
     start_new_session=True)
   signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
   try:rc=helper.wait(timeout=min(2,max(0.1,remaining-2)))
   except subprocess.TimeoutExpired:result['fd_inspection']='timeout'
   else:
    result['fd_inspection']='complete' if rc==0 else 'failed_or_capped'
    result['fd_inspection_returncode']=rc
 except OSError:result['fd_inspection']='error'
 finally:
  signal.pthread_sigmask(signal.SIG_BLOCK,{signal.SIGINT,signal.SIGTERM})
  try:
   if helper is not None and helper.poll() is None:
    helper.kill()
    helper.wait(timeout=2)
  finally:signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)

def limits_for_mode(mode):
 if mode=='strict':return (2*1024**2,8*1024**2)
 if mode=='metal-compatible':return (8*1024**2,8*1024**2)
 raise ValueError('unsupported file-limit mode')

def planned_rows(data):
 rows=[];step=0
 for line in data.decode('ascii').splitlines():
  fields=[int(x) for x in line.split()]
  if len(fields)!=5:raise ValueError('invalid command')
  count,mask,x,y,capture=fields
  if not 1<=count<=1000 or mask<0 or mask&~1023 or not -32767<=x<=32767 or not -32767<=y<=32767 or capture not in (0,1,2):raise ValueError('invalid command')
  rows.append([step,*fields]);step+=count
 if step!=EXPECTED_STEPS or tuple(row[0]+row[1]-1 for row in rows if row[-1]==1)!=EXPECTED_FRAME_STEPS or [row[0]+row[1] for row in rows if row[-1]==2]!=[HYBRID_STEP]:raise ValueError('unexpected grid plan')
 return rows

def planned_witness(path,expected):
 try:
  with (path/'commands.csv').open(newline='') as f:commands=list(csv.reader(f))
  if commands[0]!=['begin','steps','mask','x','y','capture'] or [[int(v) for v in x] for x in commands[1:]]!=expected:return False
  with (path/'frames.csv').open(newline='') as f:frames=list(csv.DictReader(f))
  if tuple(int(x['observed_step']) for x in frames)!=EXPECTED_FRAME_STEPS:return False
  if [x.name for x in sorted(path.glob('live-*.argb'))]!=['live-000.argb','live-001.argb']:return False
  status=(path/'status.log').read_text()
  return (f'completed_steps={EXPECTED_STEPS} ' in status and 'frontend=hybrid_authentic jit_before_cpu=1 jit_before_rsp=1 homebrew=0 hybrid_transition=1 exact_view_equal=1 cpu_after=0 rsp_after=0' in status and 'gpu_ready=1 ' in status)
 except (OSError,ValueError,KeyError,IndexError):return False

def owned_guard(path):
 total=sum(x.stat().st_size for x in pathlib.Path(path).rglob('*') if x.is_file() and not x.is_symlink())
 if total>63*1024**2:raise ValueError('owned output threshold')
 if shutil.disk_usage(path).free<5*1024**3:raise ValueError('disk reserve')
 return total

def relay(cmd,cwd,env,log,err,result,selector_factory=selectors.DefaultSelector,mode='strict',input_stream=None):
 limits=limits_for_mode(mode)
 """Own one child; cleanup also runs on selector errors and signals."""
 child=None;sel=None;started=time.monotonic()
 def terminate(signum,_frame):raise SystemExit(128+signum)
 oldterm=signal.signal(signal.SIGTERM,terminate)
 oldmask=signal.pthread_sigmask(signal.SIG_BLOCK,{signal.SIGINT,signal.SIGTERM})
 try:
  def setup():
   resource.setrlimit(resource.RLIMIT_FSIZE,limits)
   signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
  child=subprocess.Popen(cmd,cwd=cwd,env=env,stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,bufsize=0,preexec_fn=setup,start_new_session=True)
  signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
  source=input_stream if input_stream is not None else sys.stdin.buffer
  sel=selector_factory();sel.register(source,selectors.EVENT_READ,'input');sel.register(child.stdout,selectors.EVENT_READ,'output');sel.register(child.stderr,selectors.EVENT_READ,'error')
  deadline=started+180;written={'output':0,'error':0}
  while child.poll() is None and time.monotonic()<deadline:
   try:owned_guard(cwd)
   except (OSError,ValueError) as exc:result['owned_guard_error']=str(exc);break
   if stopped_child(child):
    inspect_stopped_child(child,cwd,deadline-time.monotonic(),result)
    break
   for key,_ in sel.select(min(0.1,max(0,deadline-time.monotonic()))):
    data=os.read(key.fileobj.fileno(),4096)
    if key.data=='input':
     if not data:sel.unregister(key.fileobj);child.stdin.close()
     else:
      try:child.stdin.write(data);child.stdin.flush()
      except BrokenPipeError:sel.unregister(key.fileobj)
    elif data:
     written[key.data]+=len(data)
     if written[key.data]>(256*1024 if key.data=='output' else 512*1024):
      result['log_budget_exceeded']=True;break
     if key.data=='output':log.write(data);log.flush();sys.stdout.buffer.write(data);sys.stdout.buffer.flush()
     else:err.write(data);err.flush()
    else:sel.unregister(key.fileobj)
   if result.get('log_budget_exceeded'):break
  if child.poll() is None and not result.get('log_budget_exceeded') and not result.get('signal_stop') and not result.get('owned_guard_error'):result['timeout']=True
 finally:
  signal.pthread_sigmask(signal.SIG_BLOCK,{signal.SIGINT,signal.SIGTERM})
  try:
   if sel is not None:sel.close()
   if child is not None and result.get('signal_stop') and child.poll() is None:
    child.kill()  # SIGKILL works on a stopped process; no guest resume.
   terminate_child(child)
  finally:
   signal.signal(signal.SIGTERM,oldterm)
   signal.pthread_sigmask(signal.SIG_SETMASK,oldmask)
 if child is None:raise RuntimeError('observer failed to start')
 result['returncode']=child.returncode;result['seconds']=round(time.monotonic()-started,3)

def main(argv=None):
 p=argparse.ArgumentParser()
 for arg in ('binary','source','moltenvk','rom','output'):p.add_argument('--'+arg,type=pathlib.Path,required=True)
 p.add_argument('--command-file',type=pathlib.Path,required=True)
 p.add_argument('--file-limit-mode',choices=('strict','metal-compatible'),default='strict')
 a=p.parse_args(argv)
 command_data=a.command_file.read_bytes()
 if hashlib.sha256(command_data).hexdigest()!=COMMAND_SHA:raise SystemExit('command hash mismatch')
 expected=planned_rows(command_data)
 if not clean(a.source) or sha(a.rom)!=ROM_SHA or a.moltenvk.resolve()!=VK_PATH or sha(a.moltenvk)!=VK_SHA:raise SystemExit('input provenance mismatch')
 require_binary(a.binary)
 if a.output.exists():raise SystemExit('output must be new')
 with open(LOCK,'a') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX)
  check_reserve(a.output);require_binary(a.binary)
  if a.command_file.read_bytes()!=command_data:raise SystemExit('command changed while queued')
  if a.output.exists() or not clean(a.source) or sha(a.rom)!=ROM_SHA or a.moltenvk.resolve()!=VK_PATH or sha(a.moltenvk)!=VK_SHA:raise SystemExit('input provenance or output changed while queued')
  a.output.mkdir(parents=True);[(a.output/x).mkdir() for x in ('input','saves','systems')];copy=a.output/'input/game.z64';shutil.copyfile(a.rom,copy)
  if sha(copy)!=ROM_SHA:raise SystemExit('ROM copy mismatch')
  sources=[pathlib.Path(__file__).resolve(),pathlib.Path(__file__).with_name('actor_observer.cpp').resolve(),pathlib.Path(__file__).with_name('input_mapping.h').resolve(),pathlib.Path(__file__).with_name('observer_read.h').resolve(),pathlib.Path(__file__).with_name('observer_file_budget.h').resolve(),pathlib.Path(__file__).with_name('observer_sigxfsz.h').resolve(),pathlib.Path(__file__).with_name('observer_images.h').resolve(),pathlib.Path(__file__).with_name('visual_capture.h').resolve(),pathlib.Path(__file__).with_name('hybrid_contract.h').resolve(),pathlib.Path(__file__).with_name('inject.cmake').resolve()]
  resources=[a.source/'ares/ares/resource/resource.cpp',a.source/'ares/ares/resource/resource.hpp',a.source/'mia/resource/resource.cpp',a.source/'mia/resource/resource.hpp'];inputs={str(x):sha(x) for x in sources+resources};binary=sha(a.binary)
  cmd=[str(a.binary.resolve()),str(copy.resolve()),str(a.output.resolve()),'interactive','14000'];result={'command':cmd,'binary_sha256':binary,'input_sha256':inputs,'rom_sha256':ROM_SHA,'moltenvk_sha256':VK_SHA,'source_commit':REV,'command_sha256':COMMAND_SHA,'expected_steps':EXPECTED_STEPS,'expected_frame_steps':EXPECTED_FRAME_STEPS,'frontend_role':'authentic_same_process_jit_to_interpreter','state_scope':'official restoration of same-process state; no arbitrary guest-memory writes','ram_capture_enabled':False,'cache_view_bytes':8*1024**2,'in_memory_state_max_bytes':16*1024**2,'file_limit_mode':a.file_limit_mode,'rlimit_fsize_soft':limits_for_mode(a.file_limit_mode)[0],'rlimit_fsize_hard':limits_for_mode(a.file_limit_mode)[1],'owned_limits':{'ram_count':0,'ram_each_bytes':0,'ram_total_bytes':0,'frame_total_bytes':2*1024**2,'input_csv_bytes':2*1024**2,'recorded_output_pre_manifest_bytes':63*1024**2,'outer_wall_seconds':180,'child_wall_seconds':175,'inventory_bytes':67596,'fd_inspection_bytes':65536}}
  readfd,writefd=os.pipe()
  try:os.write(writefd,command_data)
  finally:os.close(writefd)
  with os.fdopen(readfd,'rb') as source,(a.output/'bridge.log').open('wb') as log,(a.output/'stderr.log').open('wb') as err:
   relay(cmd,a.output.resolve(),{**os.environ,'DYLD_LIBRARY_PATH':str(a.moltenvk.resolve().parent)},log,err,result,mode=a.file_limit_mode,input_stream=source)
  result['unchanged']=clean(a.source) and sha(a.rom)==ROM_SHA and sha(copy)==ROM_SHA and sha(a.moltenvk)==VK_SHA and sha(a.binary)==binary==BINARY_SHA and all(sha(pathlib.Path(k))==v for k,v in inputs.items())
  files=sorted(a.output.glob('live-*.argb'));result['frames']={x.name:sha(x) for x in files};result['frame_bytes']=sum(x.stat().st_size for x in files);result['ok']=result['returncode']==0 and result['unchanged'] and 1<=len(files)<=3 and result['frame_bytes']<=2*1024**2
  ram=sorted(a.output.glob('ram-*.bin'));result['ram']={x.name:sha(x) for x in ram};result['ram_bytes']=sum(x.stat().st_size for x in ram);result['ok']=result['ok'] and len(ram)==0 and result['ram_bytes']==0
  for name in ('ram.csv','commands.csv','inputs.csv','callbacks.csv','status.log','sigxfsz.bin','images.bin','stopped-fds.txt'):result[name+'_sha256']=sha(a.output/name) if (a.output/name).is_file() else None
  diagnostic=a.output/'sigxfsz.bin';images=a.output/'images.bin';result['ok']=result['ok'] and diagnostic.is_file() and diagnostic.stat().st_size in (0,40) and images.is_file() and images.stat().st_size<=12+128*528
  result['planned_witness_complete']=planned_witness(a.output,expected)
  result['ok']=result['ok'] and result['planned_witness_complete'] and not result.get('owned_guard_error')
  result['output_bytes']=sum(x.stat().st_size for x in a.output.rglob('*') if x.is_file() and not x.is_symlink());result['ok']=result['ok'] and result['output_bytes']<=63*1024**2 and not result.get('log_budget_exceeded',False)
  (a.output/'manifest.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'ok':result['ok'],'returncode':result['returncode'],'frames':len(files)}),flush=True)
  return 0 if result['ok'] else 1

if __name__=='__main__':sys.exit(main())
