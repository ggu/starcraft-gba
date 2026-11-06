#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare bounded Z7 ARM resume diagnostics; inputs and outputs remain private."""
import argparse, fcntl, hashlib, json, pathlib, re, shutil, sys, time
import sc64_z7_arm_replay as replay

def cargo(text):
 d={}
 for line in text.splitlines():
  m=re.search(r'CARGO7 (\w+) tick=(\d+) digest=(\d+) end=1',line)
  if m:
   k=(m[1],int(m[2]))
   if k in d:raise RuntimeError('duplicate cargo record')
   d[k]=int(m[3])
 return d
def busy(text, cut):
 rows={}
 for line in text.splitlines():
  m=re.search(r'Z7B (COUNTS|MODAL|WORK|SAMPLE|DONE)(.*)',line)
  if not m:continue
  pairs=re.findall(r'(\w+)=(-?\d+)',m[2])
  if len({key for key,_ in pairs})!=len(pairs):raise RuntimeError('duplicate diagnostic field')
  rows.setdefault(m[1],[]).append({key:int(value) for key,value in pairs})
 expected={'COUNTS':1,'MODAL':4,'WORK':10,'SAMPLE':10,'DONE':1}
 if {k:len(rows.get(k,[])) for k in expected}!=expected:raise RuntimeError('missing or duplicated diagnostic records')
 count=rows['COUNTS'][0]
 count_fields={'calls','advancing','text_before','text_after','high50','high10'} | {f'{kind}{i}' for i in range(3) for kind in ('n','sum','peak')}
 if set(count)!=count_fields or any(v<0 for v in count.values()):raise RuntimeError('bad count fields')
 if count['calls']!=1000 or sum(count[f'n{i}'] for i in range(3))!=1000:raise RuntimeError('bad sample counts')
 if not 0<count['advancing']<=1000 or max(count['text_before'],count['text_after'])>1000:raise RuntimeError('bad advancement or text counts')
 for i in range(3):
  if count[f'n{i}']==0 and (count[f'sum{i}'] or count[f'peak{i}']):raise RuntimeError('empty timing category is nonzero')
  if count[f'n{i}'] and (not count[f'peak{i}'] or count[f'sum{i}']<count[f'peak{i}']):raise RuntimeError('bad timing category')
 if not 0<count['high50']<=count['high10']<=max(count[f'peak{i}'] for i in range(3)):raise RuntimeError('bad tail counts')
 modal_fields={'index','screen_before','screen_after','mode_before','mode_after'}
 for row in rows['MODAL']:
  if set(row)!=modal_fields or any(v<0 for v in row.values()):raise RuntimeError('bad modal fields')
 for key in modal_fields-{'index'}:
  if sum(row[key] for row in rows['MODAL'])!=1000:raise RuntimeError('bad '+key+' counts')
 if [r['index'] for r in rows['MODAL']]!=list(range(4)):raise RuntimeError('unordered modal records')
 for index,row in enumerate(rows['WORK']):
  fields={'index','first','last'} | ({'minimum'} if index<4 else set())
  if set(row)!=fields or row['index']!=index:raise RuntimeError('bad workload fields')
  if index<4 and not 0<=row['minimum']<=min(row['first'],row['last']):raise RuntimeError('bad workload minimum')
 for index,row in enumerate(rows['SAMPLE']):
  if set(row)!={'tick','state','pixels_palette'} or row['tick']!=cut+100*(index+1):raise RuntimeError('bad sample fields')
  if any(not 0<=row[k]<=0xffffffff for k in ('state','pixels_palette')):raise RuntimeError('bad sample hash')
 if rows['DONE']!=[{}]:raise RuntimeError('bad terminal record')
 return rows

def compare_pair(a, b):
 for key in ('MODAL','WORK','SAMPLE'):
  if a[key]!=b[key]:raise RuntimeError('paired '+key+' mismatch')
 for key in ('calls','advancing','text_before','text_after','n0','n1','n2'):
  if a['COUNTS'][0][key]!=b['COUNTS'][0][key]:raise RuntimeError('paired '+key+' mismatch')

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--trace',type=pathlib.Path,required=True)
 parser.add_argument('--cut',type=int,required=True)
 parser.add_argument('--baseline-rom',type=pathlib.Path,required=True)
 parser.add_argument('--candidate-rom',type=pathlib.Path,required=True)
 parser.add_argument('--baseline-seed',type=pathlib.Path,required=True)
 parser.add_argument('--candidate-seed',type=pathlib.Path,required=True)
 parser.add_argument('--baseline-prefix',type=pathlib.Path,required=True)
 parser.add_argument('--candidate-prefix',type=pathlib.Path,required=True)
 parser.add_argument('--baseline-host-resume',type=pathlib.Path,required=True)
 parser.add_argument('--candidate-host-resume',type=pathlib.Path,required=True)
 parser.add_argument('--inputs-manifest',type=pathlib.Path,required=True,
                     help='absolute paths and SHA-256 values captured at build')
 parser.add_argument('--output',type=pathlib.Path,required=True)
 parser.add_argument('--timeout',type=int,default=120)
 args=parser.parse_args()
 if not 1 <= args.timeout <= 120: parser.error('timeout must be 1..120 seconds')
 if args.output.exists() and any(args.output.iterdir()): parser.error('output must be empty')
 args.output.mkdir(parents=True,exist_ok=True)
 plan=replay.load_trace(args.trace);cut=args.cut;last=cut+1000
 if cut<=0 or last>plan['stop']:parser.error('window must fit the recorded tape')
 pins=json.loads(args.inputs_manifest.read_text())
 if not isinstance(pins,dict) or not pins:parser.error('input manifest must be a nonempty path/hash object')
 files={pathlib.Path(p) for p in pins}
 if any(not p.is_absolute() for p in files):parser.error('manifest paths must be absolute')
 files|={args.trace,args.baseline_rom,args.candidate_rom,args.baseline_seed,args.candidate_seed,
         args.baseline_prefix,args.candidate_prefix,args.baseline_host_resume,args.candidate_host_resume,
         args.inputs_manifest,pathlib.Path(__file__).resolve(),pathlib.Path(replay.__file__).resolve(),replay.MGBA}
 files={p.resolve() for p in files}
 def manifest():
  missing=[str(p) for p in files if not p.is_file()]
  if missing:raise RuntimeError('missing inputs '+str(missing))
  return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
 with open('/tmp/sc64-native-build-20261031.lock','w') as lock:
  fcntl.flock(lock,fcntl.LOCK_EX)
  before=manifest()
  for path,expected in pins.items():
   if before.get(str(pathlib.Path(path).resolve()))!=expected:raise RuntimeError('build input changed: '+path)
  (args.output/'inputs-before.json').write_text(json.dumps(before,indent=2))
  results={}
  for variant in ('baseline','candidate'):
   if shutil.disk_usage(args.output).free<3*1024**3+300*1024**2:raise RuntimeError('disk reserve')
   target=args.output/('run-'+variant);seed_path=getattr(args,variant+'_seed')
   seed=seed_path.read_bytes();prefix=getattr(args,variant+'_prefix').read_text()
   target.mkdir();print(variant,'started',time.strftime('%Y-%m-%dT%H:%M:%S%z'),flush=True)
   text,wall=replay.run_mgba(getattr(args,variant+'_rom'),target,args.timeout,seed_path,
                             (b'Z7B DONE',b'Z7 LOADED fail',b'Z7 EARLY_END'))
   records,errors=replay.parse_records(text);loaded=records['singles']['LOADED']
   if len(loaded)!=1 or loaded[0][1]!='ok':errors.append('bad LOADED')
   else:errors+=replay.loaded_check(plan,loaded[0][2],cut,prefix,seed,False,'cargo-transport1')
   n,mismatch=replay.compare(plan,records,after=cut,through=last)
   expected=sum(1 for t in plan['observations'] if cut<t<=last)
   if not expected or n!=expected:errors.append(f'compared {n}/{expected} observations')
   errors+=mismatch
   host=getattr(args,variant+'_host_resume').read_text()
   expected_cargo={k:x for k,x in cargo(host).items() if cut<k[1]<=last}
   actual_cargo=cargo(text)
   if not expected_cargo or expected_cargo!=actual_cargo:errors.append('cargo digest mismatch')
   saved=target/'saves/replay.sav'
   if not saved.is_file() or saved.read_bytes()!=seed:errors.append('guest modified seed SRAM')
   rows=busy(text,cut);result={'wall_seconds':wall,'observations':n,'cargo_checkpoints':len(actual_cargo),
                               'diagnostic':rows,'failures':errors}
   (args.output/(variant+'-result.json')).write_text(json.dumps(result,indent=2)+'\n')
   print(variant,'wall',round(wall,1),'observations',n,'cargo',len(actual_cargo),'failures',errors,flush=True)
   if errors:raise RuntimeError(variant+' validation failed')
   results[variant]=rows
  compare_pair(results['baseline'],results['candidate'])
  after=manifest();(args.output/'inputs-after.json').write_text(json.dumps(after,indent=2))
  if before!=after:raise RuntimeError('input changed during run')
  print('bounded post-cut diagnostic pair passed sampled parity',flush=True)

if __name__ == '__main__':
 main()
