#!/usr/bin/env python3
"""Evaluate the real public race-rule helpers for a Makefile profile, without game assets.

This probes rule helpers, not game_update or full constructors. The JSON retains
helper-only eligibility so callers must audit profile-specific runtime overrides.
"""
from pathlib import Path
import argparse, hashlib, json, re, subprocess

def dump(root, variable, output, extra=()):
    root=Path(root).resolve(); output=Path(output).resolve(); output.mkdir(parents=True,exist_ok=True)
    recipe='include Makefile\n.PHONY: print-audit-flags\nprint-audit-flags:\n\t@printf \'%s\\n\' \'$('+variable+')\'\n'
    flags=subprocess.check_output(['make','--no-print-directory','-s','-f','-','print-audit-flags'],input=recipe,text=True,cwd=root).strip().split()
    if not flags: raise ValueError('empty profile flags: '+variable)
    flags=[x for x in flags if x.startswith('-D')]+list(extra)
    header=root/'starcraft/race_rules.h'
    prefix='#include <stdio.h>\n#include "'+str(root/'common/gba.h')+'"\n#include "'+str(header)+'"\n'
    pre=subprocess.check_output(['clang','-E','-P','-x','c',*flags,'-'],input=prefix,text=True,cwd=root)
    enum=next(x for x in re.findall(r'enum\s*\{(.*?)\}\s*;',pre,re.S) if 'SC_RU_COUNT' in x)
    names=list(dict.fromkeys(re.findall(r'\bU_[A-Z0-9_]+\b',enum)))
    namearray='static const char *names[SC_RU_COUNT] = {'+','.join('['+x+']="'+x+'"' for x in names)+'};\n'
    main=r'''
int main(void) {
 for (int i=0;i<SC_RU_COUNT;i++) {
  const Sc64RaceUnitRule *r=sc64_race_unit_rule(i);
  int hp=r->hp, fact_shield=-1; unsigned actor_flags=0;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
  actor_flags=sc64_zp_actor_flags(i);
#endif
#ifdef SC64_T4_RUNTIME
  if(!actor_flags) actor_flags=sc64_race_t4_actor_flags(i);
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
  const Sc64ZpActorFact *fact=sc64_zp_actor_fact(i);
  if(fact) { if(fact->hp)hp=fact->hp; fact_shield=fact->shields; }
#endif
  printf("{\"name\":\"%s\",\"enum_id\":%d,\"raw_id\":%d,\"canonical_type\":%d,\"actor_flags\":%u,\"hp\":%d,\"shields\":%d,\"fact_shields\":%d,\"armor\":%d,\"minerals\":%d,\"gas\":%d,\"build_frames\":%d,\"supply\":%d,\"supply_provided\":%d,\"flying\":%d,\"air_eligible\":%d,\"ground\":{\"damage\":%d,\"cooldown\":%d,\"range\":%d,\"hits\":%d},\"air\":{\"damage\":%d,\"cooldown\":%d,\"range\":%d,\"hits\":%d}}\n",
   names[i],i,sc64_race_raw_unit(i),sc64_race_unit_from_raw(sc64_race_raw_unit(i)),actor_flags,hp,r->shields,fact_shield,
   sc64_race_unit_armor(i),r->minerals,r->gas,r->build_frames,r->supply,
   sc64_race_unit_supply_provided(i),sc64_race_unit_flying(i),sc64_race_unit_can_attack_air(i),
   sc64_race_unit_damage_vs(i,0),sc64_race_unit_cooldown_vs(i,0),sc64_race_unit_range_vs(i,0),sc64_race_unit_hits_vs(i,0),
   sc64_race_unit_damage_vs(i,1),sc64_race_unit_cooldown_vs(i,1),sc64_race_unit_range_vs(i,1),sc64_race_unit_hits_vs(i,1));
 }
}
'''
    source=output/'dump.c';binary=output/'dump'
    source.write_text(prefix+namearray+main)
    subprocess.run(['clang','-std=c11','-O0',*flags,str(source),'-o',str(binary)],cwd=root,check=True,capture_output=True,text=True)
    rows=[json.loads(x) for x in subprocess.check_output([str(binary)],text=True).splitlines()]
    result={'engine_root':str(root),'engine_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),
            'profile_variable':variable,'defines':flags,'rules_sha256':hashlib.sha256(header.read_bytes()).hexdigest(),'units':rows}
    (output/'engine-facts.json').write_text(json.dumps(result,indent=2)+'\n')
    print(variable, len(rows),'units',str(output/'engine-facts.json'))
    return result

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('root');p.add_argument('profile');p.add_argument('output');p.add_argument('--extra',action='append',default=[])
 a=p.parse_args();dump(a.root,a.profile,a.output,a.extra)
