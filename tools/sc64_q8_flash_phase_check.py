# SPDX-License-Identifier: GPL-3.0-or-later
"""Bounded actual-game legacy phase migration; requires prepared private inputs."""
import argparse,fcntl,hashlib,json,pathlib,shutil,subprocess,resource,time,os,signal
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline',type=pathlib.Path,required=True)
parser.add_argument('--output',type=pathlib.Path,required=True)
parser.add_argument('--inputs',type=pathlib.Path,required=True)
parser.add_argument('--input-root',type=pathlib.Path,required=True)
args=parser.parse_args()
r=pathlib.Path.cwd();o=args.output.resolve();base=args.baseline.resolve();inputs=args.input_root.resolve()
if o.exists():raise SystemExit('output must be new')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pins=json.loads(args.inputs.read_text())
head=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip();rows=[];olds={}

def check():
 assert subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()==head and subprocess.run(['git','diff','--quiet','HEAD','--']).returncode==0
 assert subprocess.check_output(['git','-C',str(base),'rev-parse','HEAD'],text=True).strip()=='8127f7af1abe1ad9b5a911f7ea3554bcc691fba7'
 assert subprocess.run(['git','-C',str(base),'diff','--quiet','HEAD','--']).returncode==0
 assert all(sha(inputs/p)==h for p,h in pins.items());assert all(sha(o/p)==h for p,h in olds.items())
def cap():resource.setrlimit(resource.RLIMIT_FSIZE,(256*1024**2,256*1024**2));os.setsid()
def run(name,cmd):
 check();assert shutil.disk_usage(r).free>=4*1024**3;start=time.monotonic();print('RUN '+name,flush=True)
 with (o/(name+'.log')).open('w') as f:
  p=subprocess.Popen(cmd,stdout=f,stderr=subprocess.STDOUT,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'halt_on_error=1'},preexec_fn=cap)
  try:rc=p.wait(timeout=300)
  except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait();rc=124
 text=(o/(name+'.log')).read_text(errors='replace');errors=[x for x in ('runtime error:','ERROR: AddressSanitizer','SUMMARY: UndefinedBehaviorSanitizer') if x in text];rc=rc or (97 if errors else 0)
 check();rows.append({'name':name,'argv':cmd,'rc':rc,'diagnostics':errors,'seconds':round(time.monotonic()-start,3),'log_sha256':sha(o/(name+'.log'))});(o/'results.json').write_text(json.dumps({'head':head,'inputs':len(pins),'old_saves':olds,'results':rows,'unchanged':True},indent=2)+'\n');print(json.dumps(rows[-1]),flush=True)
 if rc:print(text[-3500:],flush=True);raise SystemExit(rc)
make=['make','-f','Makefile','-f','tests/sc64-research-q8/Makefile','SC64_AUDIO_DIR=.local/warp-audio-qualification/bank','Q8_BASELINE_ROOT='+str(base)]
print('QUEUED '+head,flush=True)
with open('/tmp/sc64-native-build-20261031.lock','a') as f:
 fcntl.flock(f,fcntl.LOCK_EX);check();assert shutil.disk_usage(r).free>=4*1024**3;o.mkdir(parents=True,exist_ok=False)
 run('phase-build',make+['build-sc64-q8-flash-phase'])
 for buildings,limit,batch in [(0,1280,160),(1,320,100)]:
  for n in range(0,limit,batch):
   name=('buildings' if buildings else 'units')+'-'+str(n);old=o/(name+'.old.sav');flash=o/(name+'.flash')
   run(name+'-write',['build/research-q8-flash-phase/old-producer','write',str(old),str(buildings),str(n)]);olds[old.name]=sha(old)
   shutil.copyfile(old,flash)
   if buildings and n==0:run('ambiguous-building-reject',['build/research-q8-flash-phase/consumer','reject-build-phase',str(old),'1','0'])
   run(name+'-migrate',['build/research-q8-flash-phase/consumer','migrate',str(flash),str(buildings),str(n)])
   run(name+'-read',['build/research-q8-flash-phase/consumer','read',str(flash),str(buildings),str(n)])
 run('retained-flash',make+['test-sc64-q8-flash-host-p10'])
 print('PASS:1280 unit phases,320 building phases,47 advances and retained Flash suite',flush=True)
