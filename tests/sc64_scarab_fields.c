/* SPDX-License-Identifier: GPL-3.0-or-later
 * Field-format and codec test; not a world-save or gameplay test. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int16_t s16;
#define SC64_RACES 1
enum { MAX_UNITS=240, MAX_BUILDINGS=160 };
#include "../starcraft/sc64_stock_profiles.h"
#include "../starcraft/race_rules.h"
#ifndef SC64_ZP_CAMPAIGN_ACTORS
static inline int sc64_zp_actor_default_hp(int type) { (void)type; return 0; }
static inline int sc64_zp_actor_default_shields(int type) { (void)type; return 0; }
#endif
/* SC64_REAL_UNIT_DEFINITION */
typedef struct { int x,y,hp,owner,type,alive,queue[5],queue_head,queue_count,queue_progress,build_progress,under_construction,level,shield,maxhp,raw_type,raw_owner,flags,larva,larva_timer,shield_remainder; } Building;
#include "../starcraft/sc64_scarab_fields.h"
#include "../starcraft/sc64_compact_unit_save.h"
#include "../starcraft/sc64_zp_unit_save.h"
#include "../starcraft/sc64_derived_save.h"
#include "../starcraft/sc64_p7_typed_unit_save.h"
_Static_assert(MAX_UNITS<=SC64_SCARAB_FIELD_SLOT_LIMIT,"field slot capacity");
static Unit unit(int child) {
 Unit q={0};q.x=q.y=q.tx=q.ty=q.path_x=q.path_y=10;q.type=U_REAVER;q.raw_type=83;q.raw_owner=0;q.owner=0;q.alive=1;q.hp=q.maxhp=100;q.shield=80;q.armor=sc64_race_unit_armor(U_REAVER);q.target=-1;q.build=-1;
#ifdef SC64_SCARAB_RUNTIME
 if(child){q.type=U_SCARAB;q.raw_type=85;q.hp=q.maxhp=20;q.shield=10;q.armor=sc64_race_unit_armor(U_SCARAB);}
#else
 (void)child;
#endif
 return q;
}
static void codecs(Unit q) {
 Unit out;Sc64PackedUnitSave a;Sc64ZpPackedUnitSave b;Sc64DerivedUnitSave c;Sc64P7TypedUnitSave d;
 assert(sc64_unit_save_encode(&a,&q,2048,2048));assert(sc64_unit_save_decode(&out,&a,2048,2048));assert(out.carry==q.carry && out.work_timer==q.work_timer && out.build==-1 && out.type==q.type && out.hp==q.hp && out.maxhp==q.maxhp && out.shield==q.shield && out.armor==q.armor && out.raw_type==q.raw_type && out.raw_owner==q.raw_owner);
 assert(sc64_zp_unit_save_encode(&b,&q,2048,2048));assert(sc64_zp_unit_save_decode(&out,&b,2048,2048));assert(out.carry==q.carry && out.work_timer==q.work_timer && out.build==-1 && out.type==q.type && out.hp==q.hp && out.maxhp==q.maxhp && out.shield==q.shield && out.armor==q.armor && out.raw_type==q.raw_type && out.raw_owner==q.raw_owner);
 assert(sc64_derived_unit_encode(&c,&q,2048,2048));assert(sc64_derived_unit_decode(&out,&c,2048,2048));assert(out.carry==q.carry && out.work_timer==q.work_timer && out.build==-1 && out.type==q.type && out.hp==q.hp && out.maxhp==q.maxhp && out.shield==q.shield && out.armor==q.armor && out.raw_type==q.raw_type && out.raw_owner==q.raw_owner);
 assert(sc64_p7_typed_unit_encode(&d,&q,2048,2048));assert(sc64_p7_typed_unit_decode(&out,&d,2048,2048));assert(out.carry==q.carry && out.work_timer==q.work_timer && out.build==-1 && out.type==q.type && out.hp==q.hp && out.maxhp==q.maxhp && out.shield==q.shield && out.armor==q.armor && out.raw_type==q.raw_type && out.raw_owner==q.raw_owner);
}
int main(void) {
 int carry=123,timer=456; Sc64ScarabParentFields badp={6,255,0,0}, pout={9,9,9,9};
 Sc64ScarabChildFields badc={240,0,0,0}, cout={9,9,9,9};
 assert(!sc64_scarab_parent_fields_pack(&badp,&carry,&timer) && carry==123 && timer==456);
 assert(!sc64_scarab_child_fields_pack(&badc,&carry,&timer) && carry==123 && timer==456);
 assert(!sc64_scarab_parent_fields_unpack(65536,0,&pout) && pout.queued==9);
 assert(!sc64_scarab_child_fields_unpack(1024,0,&cout) && cout.parent_slot==9);
 unsigned long parents=0,children=0;
 for(unsigned q=0;q<=5;q++)for(unsigned slot=0;slot<=255;slot++)for(unsigned cd=0;cd<=60;cd++)for(unsigned progress=0;progress<=104;progress++) {
  Sc64ScarabParentFields x={q,slot,cd,progress},y;int carry=7,timer=7;int valid=slot<240? q!=0 : slot==255 && progress==0;
  assert(sc64_scarab_parent_fields_pack(&x,&carry,&timer)==valid);if(!valid)continue;
  assert(sc64_scarab_parent_fields_unpack(carry,timer,&y));assert(x.queued==y.queued&&x.active_slot==y.active_slot&&x.cooldown==y.cooldown&&x.progress==y.progress);++parents;
  if((slot==0||slot==239||slot==255)&&(cd==0||cd==60)&&(progress==0||progress==104)){Unit u=unit(0);u.carry=carry;u.work_timer=timer;codecs(u);}
 }
 for(unsigned slot=0;slot<240;slot++)for(unsigned phase=0;phase<4;phase++)for(unsigned life=0;life<=90;life++)for(unsigned nc=0;nc<=7;nc++) {
  Sc64ScarabChildFields x={slot,phase,life,nc},y;int carry=9,timer=9;int valid=nc<=life;assert(sc64_scarab_child_fields_pack(&x,&carry,&timer)==valid);if(!valid)continue;assert(sc64_scarab_child_fields_unpack(carry,timer,&y));assert(x.parent_slot==y.parent_slot&&x.phase==y.phase&&x.life==y.life&&x.no_collision==y.no_collision);++children;
  if((slot==0||slot==239)&&(life==0||life==7||life==90)&&(nc==0||nc==7)){Unit u=unit(1);u.carry=carry;u.work_timer=timer;codecs(u);}
 }
#ifdef SC64_SCARAB_RUNTIME
 assert(U_SCARAB == SC_RU_COUNT-1 && SC_RU_COUNT <= 64);
 assert(sc64_race_raw_unit(U_SCARAB)==85 && sc64_race_unit_from_raw(85)==U_SCARAB);
 const Sc64RaceUnitRule *rule=sc64_race_unit_rule(U_SCARAB);
 assert(rule->hp==20 && rule->shields==10 && rule->supply==0 && rule->producer==255);
 assert(rule->minerals==15 && rule->gas==0 && rule->build_frames==105);
 const Sc64StockUnitProfile *stock=sc64_stock_unit_profile(85);
 assert(stock->ground_weapon==82 && stock->air_weapon==130 && stock->armor==0 && stock->top_speed_q10>0);
 puts("Scarab field packing and actual raw-85 codec rows passed");
#else
 puts("Scarab field packing and codec rows passed with surrogate identity");
#endif
 printf("parents=%lu children=%lu\n",parents,children);return 0;
}
