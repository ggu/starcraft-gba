#!/usr/bin/env python3
"""Stage authenticated BOLT image/LO entries and an index-equivalent images.tbl.

SPDX-License-Identifier: GPL-3.0-or-later
The generated filename table only maps the original indices to local filenames.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.sc64_construction_sprites import load_sprites, EXPECTED_ROM_SHA1
from tools.sc64_openbw_oracle_tables import TABLES

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--rom',type=Path,required=True)
    p.add_argument('--extractor',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();out=a.out.resolve()
    if not out.is_relative_to((ROOT/'.local').resolve()) or out.exists():
        p.error('output must be a new directory under this worktree .local')
    extractor=load_sprites(ROOT/'tools/sc64_sprites.py').load_extractor(a.extractor)
    rom=extractor.load_rom(str(a.rom))
    if hashlib.sha1(rom).hexdigest()!=EXPECTED_ROM_SHA1:p.error('ROM authentication failed')
    archive=extractor.BoltArchive(rom);entries={}
    for e in archive.entries():
        if e.path in entries:p.error('duplicate archive entry')
        entries[e.path]=e
    images=archive.read(entries[TABLES['images.dat'][0]])
    if hashlib.sha256(images).hexdigest()!=TABLES['images.dat'][1]:p.error('images.dat authentication failed')
    count=999
    if len(images)!=37962:p.error('unexpected images.dat size')
    indices={struct.unpack_from('<I',images,i*4)[0] for i in range(count)}
    for col in range(6):
        indices.update(struct.unpack_from('<I',images,14*count+4*count*col+i*4)[0] for i in range(count))
    indices.discard(0)
    if not indices or max(indices)>4096:p.error('unexpected image index range')
    blobs={};receipt={}
    for index in sorted(indices):
        flat=index-1;path=f'{13+flat//256:03X}/{flat%256:03X}'
        if path not in entries:p.error('missing BOLT image/LO entry '+path)
        blob=archive.read(entries[path]);blobs[index]=blob
        receipt[str(index)]={'bolt':path,'bytes':len(blob),'sha256':hashlib.sha256(blob).hexdigest()}
    # tbl is synthetic: each original 1-based index maps to its decimal filename.
    largest=max(indices); table=bytearray(struct.pack('<H',largest)+b'\0'*(2*largest))
    for index in range(1,largest+1):
        name=(str(index)+'.bin').encode('ascii')+b'\0'
        if len(table)>65535:p.error('filename table offset overflow')
        struct.pack_into('<H',table,2+2*(index-1),len(table));table+=name
    out.mkdir(parents=True,exist_ok=False);(out/'unit').mkdir()
    with (out/'images.tbl').open('xb') as f:f.write(table)
    for index,blob in blobs.items():
        with (out/'unit'/f'{index}.bin').open('xb') as f:f.write(blob)
    with (out/'receipt.json').open('x') as f:
        json.dump({'rom_sha1':EXPECTED_ROM_SHA1,'images_dat_sha256':TABLES['images.dat'][1],
                   'synthetic_filename_table_sha256':hashlib.sha256(table).hexdigest(),'entries':receipt},f,indent=2);f.write('\n')
    print(f'staged {len(blobs)} authenticated-ROM entries, {sum(map(len,blobs.values()))} bytes')
if __name__=='__main__':main()
