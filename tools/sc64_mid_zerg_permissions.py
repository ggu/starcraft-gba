#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare private, authenticated Z5/Z6 source-PUNI permissions."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
from tools.sc64_t5_availability import effective_bits
from tools.sc64_zp_scenario_pack import parse_source

ROOT = Path(__file__).resolve().parents[1]
MISSIONS = (('Z5', 16), ('Z6', 17))

def render(rows: list[dict]) -> str:
    if [r['mission'] for r in rows] != [16, 17]:
        raise ValueError('expected Z5 and Z6 stable identities')
    if any(len(r['owners']) != 12 or len(r['races']) != 12 or len(r['bits']) != 342 for r in rows):
        raise ValueError('invalid permission dimensions')
    lines = ['/* Private owned-ROM derivative; do not distribute. */',
             '#ifndef SC64_MID_ZERG_PERMISSIONS_GENERATED_H',
             '#define SC64_MID_ZERG_PERMISSIONS_GENERATED_H',
             'typedef struct { unsigned char mission, owner_kind[12], source_race[12], bits[342]; } Sc64MidZergPermission;',
             'static const Sc64MidZergPermission sc64_mid_zerg_permissions[2] = {']
    for r in rows:
        lines += ['  { '+str(r['mission'])+', {'+','.join(map(str,r['owners']))+'},',
                  '    {'+','.join(map(str,r['races']))+'}, {']
        for i in range(0,342,16): lines.append('    '+','.join(f'0x{x:02x}' for x in r['bits'][i:i+16])+',')
        lines.append('  } },')
    return '\n'.join(lines+['};','#endif',''])

def run(source: Path, output: Path) -> dict:
    output=output.resolve()
    if not output.is_relative_to((ROOT/'.local').resolve()) or output.exists():
        raise ValueError('output must be a new directory under this checkout .local')
    rows=[]; identities=[]
    for name,mission in MISSIONS:
        d=parse_source(source,name)
        if d['global_catalog_id'] != mission: raise ValueError('mission identity mismatch')
        rows.append({'mission':mission,'owners':d['owners'],'races':d['sides'],
                     'bits':effective_bits(bytes(d['puni']))})
        identities.append({'mission':name,'id':mission,'path':d['source_map_path'],
                           'source_chk_sha256':d['source_chk_sha256']})
    header=render(rows).encode('ascii');output.mkdir(parents=True)
    (output/'sc64_mid_zerg_permissions_generated.h').write_bytes(header)
    receipt={'missions':identities,'header_sha256':hashlib.sha256(header).hexdigest()}
    (output/'manifest.json').write_text(json.dumps(receipt,indent=2)+'\n')
    return receipt

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    a=p.parse_args();print(json.dumps(run(a.source,a.out),sort_keys=True))
if __name__=='__main__': main()
