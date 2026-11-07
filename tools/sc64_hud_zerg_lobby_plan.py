#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Derive an explicit Z9/Z10 Protoss/Terran lobby prefix without changing the source tape.

This is a plan, not observed guest evidence. A physical replay must check the
live lobby fields at each edit and revalidate all subsequent checkpoints.
"""
import hashlib
import json
from pathlib import Path
import sys

EXPECTED='d6bb9e46f9011a8af8c32eb62942ab8d611974b5592df85e5788358900c32df5'
ORIGINAL=[128,0,1,0,128,0,128,0,32,0,64,0,1,0,1,0]
# HOME down/A; human row right (Zerg->Protoss); bots row left (2->1);
# bot row right (Protoss->Terran); up twice; A to Start, A to launch.
REPLACEMENT=[128,0,1,0,128,0,16,0,128,0,32,0,128,0,16,0,64,0,64,0,1,0,1,0]

def derive(path):
    raw=path.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=EXPECTED:raise ValueError('unexpected original tape')
    rows=[json.loads(line) for line in raw.splitlines()]
    edges=[(x['call_index'],x['held']) for x in rows if x.get('kind')=='input']
    if edges[:16]!=list(enumerate(ORIGINAL)):raise ValueError('unexpected original lobby prefix')
    adjusted=list(enumerate(REPLACEMENT))+[(tick+8,held) for tick,held in edges[16:]]
    if any(adjusted[i][0]>=adjusted[i+1][0] for i in range(len(adjusted)-1)):raise ValueError('non-monotonic derived events')
    return {'original_tape_sha256':EXPECTED,'original_lobby':ORIGINAL,'replacement_lobby':REPLACEMENT,
            'expected_lobby':{'human':2,'bots':1,'first_bot':0,'map':0},
            'gameplay_offset':8,'unverified_save_tick':3056,'transitions':adjusted,
            'scope':'derived ordinary input plan; no observed guest or gameplay proof'}
if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: sc64_hud_zerg_lobby_plan.py ORIGINAL_TAPE')
    print(json.dumps(derive(Path(sys.argv[1])),indent=2))
