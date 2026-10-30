#!/usr/bin/env python3
"""Reauthenticate all staged image and DAT inputs, then run the OpenBW probe.

SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse,hashlib,json,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.sc64_construction_sprites import load_sprites,EXPECTED_ROM_SHA1
from tools.sc64_openbw_oracle_tables import TABLES

def main():
 p=argparse.ArgumentParser()
 for name in ('rom','extractor','tables','images','probe'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();extractor=load_sprites(ROOT/'tools/sc64_sprites.py').load_extractor(a.extractor)
 rom=extractor.load_rom(str(a.rom))
 if hashlib.sha1(rom).hexdigest()!=EXPECTED_ROM_SHA1:p.error('ROM authentication failed')
 archive=extractor.BoltArchive(rom);entries={e.path:e for e in archive.entries()}
 for name,(_,digest) in TABLES.items():
  if hashlib.sha256((a.tables/name).read_bytes()).hexdigest()!=digest:p.error('bad table '+name)
 images=(a.tables/'images.dat').read_bytes();n=999
 indices={struct.unpack_from('<I',images,i*4)[0] for i in range(n)}
 for col in range(6):indices.update(struct.unpack_from('<I',images,14*n+4*n*col+i*4)[0] for i in range(n))
 indices.discard(0);largest=max(indices);table=bytearray(struct.pack('<H',largest)+b'\0'*(2*largest))
 for i in range(1,largest+1):
  struct.pack_into('<H',table,2+2*(i-1),len(table));table+=(str(i)+'.bin').encode('ascii')+b'\0'
 if (a.images/'images.tbl').read_bytes()!=table:p.error('filename table mismatch')
 for i in indices:
  flat=i-1;path=f'{13+flat//256:03X}/{flat%256:03X}'
  if path not in entries or (a.images/'unit'/f'{i}.bin').read_bytes()!=archive.read(entries[path]):p.error('image entry mismatch: '+path)
 subprocess.run([str(a.probe.resolve()),str(a.tables.resolve()),str(a.images.resolve())],check=True)
if __name__=='__main__':main()
