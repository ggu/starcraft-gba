#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authenticate P8 CHK and derive its private PUNI availability bitmap."""
import argparse, hashlib, json, pathlib, struct, tempfile, os
from sc64_t5_availability import effective_bits
CHK='e2257c48a119bfde3da075e08628b312857e3aad9222c47678fc29d57c578cd9'
PUNI='41892d465558f723f06b9d442cc6fb657d1efcd629b33b1ba5a11036909bb527'
def digest(data): return hashlib.sha256(data).hexdigest()
def extract(chk, manifest):
    expected={'schema':1,'mission_id':'P8','source_map_path':'008/025','source_rom_sha1':'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b','source_chk_sha256':CHK,'extractor':'references/sc64-maps/extract_sc64_maps.py','extractor_commit':'09b57f787a247107d1de82b8c5eab2734667ea1d'}
    if any(manifest.get(k)!=v for k,v in expected.items()) or manifest.get('source_chk_bytes')!=len(chk) or digest(chk)!=CHK: raise ValueError('P8 source authentication failed')
    records=[]; pos=0
    while pos<len(chk):
        if pos+8>len(chk): raise ValueError('truncated CHK header')
        tag=chk[pos:pos+4]; size=struct.unpack_from('<I',chk,pos+4)[0]; pos+=8
        if pos+size>len(chk): raise ValueError('truncated CHK payload')
        if tag==b'PUNI': records.append(chk[pos:pos+size])
        pos+=size
    if len(records)!=1 or len(records[0])!=5700 or digest(records[0])!=PUNI: raise ValueError('P8 PUNI authentication failed')
    return records[0]
def render(puni):
    bits=effective_bits(puni)
    return '/* Private generated source data; do not commit. */\n#ifndef SC64_P8_AVAILABILITY_GENERATED_H\n#define SC64_P8_AVAILABILITY_GENERATED_H\n#define SC64_P8_AVAILABILITY_BIT_BYTES %du\nstatic const unsigned char sc64_p8_availability_bits[] = {%s};\n#endif\n' % (len(bits), ','.join(map(str,bits)))
def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=pathlib.Path,required=True);p.add_argument('--manifest',type=pathlib.Path,required=True);p.add_argument('--out',type=pathlib.Path,required=True);a=p.parse_args()
    data=extract(a.source.read_bytes(),json.loads(a.manifest.read_text()));out=a.out.resolve(); root=pathlib.Path.cwd().resolve()
    if not out.is_relative_to(root/'.local') or a.out.is_symlink() or out in (a.source.resolve(),a.manifest.resolve()):
        raise ValueError('output must be an unaliased file inside ignored .local')
    out.parent.mkdir(parents=True,exist_ok=True)
    tmp=None
    try:
        with tempfile.NamedTemporaryFile(mode='w',dir=out.parent,prefix='.p8-puni-',delete=False) as f:
            tmp=pathlib.Path(f.name); f.write(render(data))
        os.replace(tmp,out)
    finally:
        if tmp is not None and tmp.exists(): tmp.unlink()
if __name__=='__main__':main()
