#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate audited Yamato codec preconditions and emit compile-time facts."""
import hashlib,json,pathlib,sys
p=pathlib.Path(sys.argv[1]); raw=p.read_bytes(); doc=json.loads(raw); y=doc['yamato']
ref=doc['reference']; chain=y['chain']; motion=chain['flingy_motion']; w=y['weapon_fields']
if ref['rom_sha1']!='a36d91b5aac7bb95df0d7de5f9814f5270e25c2b': raise ValueError('unexpected ROM')
if ref['table_sha256']['00C/002']!='e0bfdff3b35ff417eee003a7d85fccce4787959dcf18d14995a75a4144ddf75a': raise ValueError('flingy table changed')
if ref['table_sha256']['00C/000']!='6b6123e2cc6771d96f914fc9bb92d34da17cc3319dc02e2db0cd10b4f3d2efdc': raise ValueError('image table changed')
if ref['table_sha256']['00C/003']!='3e5ea29bf9937558d0a3a302cd85852c220cd126ec049c0da176c77739bb4a71': raise ValueError('sprite table changed')
if ref['table_sha256']['00C/007']!='764c6c1535a8ba5d3097c62c5755769ab9d1217e9ef4621e64720a8d2a22e730': raise ValueError('weapon table changed')
if ref['table_sha256']['012/002']!='f8e5f3d0fcca644470c37de65486e44b09eb30141eedccea3445afe7bc0f65bc': raise ValueError('script table changed')
if y['weapon_id']!=30 or w['bullet_type']!=0 or w['lifetime']!=255 or w['head_offset']!=0 or w['forward']!=0 or w['upward']!=0: raise ValueError('weapon facts changed')
if chain['flingy']!=158 or chain['image']!=541 or motion['top_speed_raw']!=8533 or motion['acceleration_raw']!=8533 or motion['turn_radius_raw']!=127: raise ValueError('flingy facts changed')
animations=list(y['projectile_animations'])
for image in ('542','544'): animations+=y['overlay_animations'][image]
for a in animations:
 if a.get('status') not in ('ok','no_animation'): raise ValueError('unresolved animation')
 if any(e.get('opcode')==55 for e in a.get('events',[])): raise ValueError('setflspeed invalidates speed codec')
 if a.get('control_flow_notes'): raise ValueError('unresolved control flow')
text='''/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Generated after numeric source fact/script guard; audit SHA-256 %s. */
#ifndef SC64_BC_YAMATO_GUARD_PRIVATE_H
#define SC64_BC_YAMATO_GUARD_PRIVATE_H
#define SC64_BC_YAMATO_FLINGY 158u
#define SC64_BC_YAMATO_TOP_SPEED 8533u
#define SC64_BC_YAMATO_ACCELERATION 8533u
#define SC64_BC_YAMATO_INITIAL_TURN 127u
#define SC64_BC_YAMATO_SCRIPT_HAS_SETSPEED 0u
#endif
''' % hashlib.sha256(raw).hexdigest()
pathlib.Path(sys.argv[2]).write_text(text)
