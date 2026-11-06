#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Audit authenticated Z5 producer seeds and scripted ownership changes.

This prints only aggregate numeric facts; it does not write private source data.
A negative result applies to the audited starting roster and trigger actions,
not to arbitrary save contents or unobserved native behavior.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
from tools.sc64_import import unit_rows
from tools.sc64_t5_availability import effective_bits
from tools.sc64_z5_doodad_pack import decode_thg2
from tools.sc64_zp_scenario_pack import parse_chk_sections, parse_source

PRODUCER_OR_WORKER={41,131,132,133}
LARVA_PRODUCTS=(37,38,41,42,43,45,46,47)

def inspect(source: Path) -> dict:
    files=sorted(p for p in (source/'z5').rglob('*') if p.is_file())
    before={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    data=parse_source(source,'Z5')
    chk=(source/'z5'/'source.chk').read_bytes()
    if hashlib.sha256(chk).hexdigest()!=data['source_chk_sha256']:
        raise ValueError('Z5 CHK changed after authentication')
    sections=parse_chk_sections(chk)
    units=unit_rows(sections[b'UNIT'])
    doodads=decode_thg2(sections[b'THG2'],data['width'],data['height'])
    actions=data['actions'];counts=Counter(a['opcode'] for a in actions)
    initial=[(q['source_type'],q['owner_raw']) for q in units if q['source_type'] in PRODUCER_OR_WORKER]
    props=[(q['type'],q['owner']) for q in doodads if q['unit_backed'] and q['type'] in PRODUCER_OR_WORKER]
    creates=[a for a in actions if a['opcode'] in (11,44)]
    give=[a for a in actions if a['opcode']==48]
    created_seeds=[(a['trigger'],a['index'],a['unit']) for a in creates if a['unit'] in PRODUCER_OR_WORKER]
    bits=effective_bits(bytes(data['puni']))
    def allowed(source_slot,raw):
        n=source_slot*228+raw;return (bits[n>>3]>>(n&7))&1
    if any(hashlib.sha256(p.read_bytes()).hexdigest()!=h for p,h in before.items()):
        raise ValueError('Z5 source changed during census')
    return {'source_chk_sha256':data['source_chk_sha256'],'unit_rows':len(units),
            'start_markers':sum(q['source_type']==214 for q in units),
            'unit_producer_or_worker':initial,'doodad_rows':len(doodads),
            'unit_backed_doodads':sum(q['unit_backed'] for q in doodads),
            'doodad_producer_or_worker':props,'action_rows':len(actions),
            'create_with_properties':counts[11],'create_unit':counts[44],
            'give_units':len(give),'created_producer_or_worker':created_seeds,
            'created_raw_types':dict(sorted(Counter(a['unit'] for a in creates).items())),
            'source3_larva_products':{str(x):allowed(3,x) for x in LARVA_PRODUCTS},
            'source3_hatch_family':{str(x):allowed(3,x) for x in (131,132,133)}}

def main() -> None:
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True)
    args=p.parse_args();print(json.dumps(inspect(args.source.resolve()),sort_keys=True,indent=2))
if __name__=='__main__': main()
