#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare a separate source copy only; never compile or modify the pinned input."""
import argparse, hashlib, pathlib, shutil, subprocess
REV='0aafd85789215e84e1e43415c07d4c88461b7899'
PATCH='ff4a9a803a3cdfc55f42ae1202790833e75f426669b36ad48511d6cd3a04a368'
INPUTS={'ares/n64/cpu/cpu.cpp':'be3919fa2232299733b811ed6052bd4020ddeb3a6e8261052c3417b2dedea3c8',
        'ares/n64/cpu/memory.cpp':'55f833718501d018d7e81e089a1ca53a9891154b8952cc2ec1b5126fda632c74'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 a=argparse.ArgumentParser();a.add_argument('--source',type=pathlib.Path,required=True);a.add_argument('--output',type=pathlib.Path,required=True);args=a.parse_args()
 src=args.source.resolve();out=args.output.resolve();patch=pathlib.Path(__file__).with_name('ares-core-patch.txt')
 if out.exists() or src==out or src in out.parents:raise ValueError('fresh output outside source required')
 if subprocess.check_output(['git','-C',str(src),'rev-parse','HEAD'],text=True).strip()!=REV or subprocess.check_output(['git','-C',str(src),'status','--porcelain'],text=True).strip():raise ValueError('source revision or cleanliness')
 if sha(patch)!=PATCH or any(sha(src/k)!=v for k,v in INPUTS.items()):raise ValueError('source or patch hash')
 if shutil.disk_usage(out.parent).free<5*1024**3:raise ValueError('disk reserve')
 shutil.copytree(src,out,symlinks=True,ignore=shutil.ignore_patterns('.git'))
 for name in INPUTS:
  if (src/name).stat().st_ino==(out/name).stat().st_ino:raise ValueError('copy shares input inode')
 subprocess.run(['patch','-p1','--batch','-i',str(patch.resolve())],cwd=out,check=True,timeout=30)
 shutil.copyfile(pathlib.Path(__file__).with_name('store_hook.h'),out/'ares/n64/cpu/sc64_store_hook.h')
 if any(sha(src/k)!=v for k,v in INPUTS.items()) or sha(patch)!=PATCH:raise ValueError('input changed')
 print('source copy prepared; no compilation performed')
if __name__=='__main__':main()
