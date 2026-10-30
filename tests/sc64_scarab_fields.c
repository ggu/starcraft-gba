/* SPDX-License-Identifier: GPL-3.0-or-later
 * Inactive field-format and codec test. Child rows use an existing canonical
 * type as a surrogate; this does not establish raw-85 actor support. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int16_t s16;
#define SC64_RACES 1
enum { MAX_UNITS=240, MAX_BUILDINGS=160 };
#include "../starcraft/sc64_stock_profiles.h"
#include "../starcraft/race_rules.h"
static inline int sc64_zp_actor_default_hp(int type) { (void)type; return 0; }
static inline int sc64_zp_actor_default_shields(int type) { (void)type; return 0; }
/* SC64_REAL_UNIT_DEFINITION */
typedef struct { int x,y,hp,owner,type,alive,queue[5],queue_head,queue_count,queue_progress,build_progress,under_construction,level,shield,maxhp,raw_type,raw_owner,flags,larva,larva_timer,shield_remainder; } Building;
#include "../starcraft/sc64_scarab_fields.h"
#include "../starcraft/sc64_compact_unit_save.h"
#include "../starcraft/sc64_zp_unit_save.h"
#include "../starcraft/sc64_derived_save.h"
#include "../starcraft/sc64_p7_typed_unit_save.h"
_Static_assert(MAX_UNITS<=SC64_SCARAB_FIELD_SLOT_LIMIT,"field slot capacity");
static Unit unit(void) {
 Unit q={0};q.x=q.y=q.tx=q.ty=q.path_x=q.path_y=10;q.type=U_REAVER;q.raw_type=83;q.raw_owner=0;q.owner=0;q.alive=1;q.hp=q.maxhp=100;q.shield=80;q.armor=sc64_race_unit_armor(U_REAVER);q.target=-1;q.build=-1;return q;
}
static void codecs(Unit q) {
 Unit out;Sc64PackedUnitSave a;Sc64ZpPackedUnitSave b;Sc64DerivedUnitSave c;Sc64P7TypedUnitSave d;
 assert(sc64_unit_save_encode(&a,&q,2048,2048));assert(sc64_unit_save_decode(&out,&a,2048,2048));assert(out.carry==q.carry && out.work_timer==q.work_timer && out.build==-1);
 assert(sc64_zp_unit_save_encode(&b,&q,2048,2048));assert(sc64_zp_unit_save_decode(&out,&b,2048,2048));assert(out.carry==q.carry && out.work_timer==q.work_timer && out.build==-1);
 assert(sc64_derived_unit_encode(&c,&q,2048,2048));assert(sc64_derived_unit_decode(&out,&c,2048,2048));assert(out.carry==q.carry && out.work_timer==q.work_timer && out.build==-1);
 assert(sc64_p7_typed_unit_encode(&d,&q,2048,2048));assert(sc64_p7_typed_unit_decode(&out,&d,2048,2048));assert(out.carry==q.carry && out.work_timer==q.work_timer && out.build==-1);
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
  if((slot==0||slot==239||slot==255)&&(cd==0||cd==60)&&(progress==0||progress==104)){Unit u=unit();u.carry=carry;u.work_timer=timer;codecs(u);}
 }
 for(unsigned slot=0;slot<240;slot++)for(unsigned phase=0;phase<4;phase++)for(unsigned life=0;life<=90;life++)for(unsigned nc=0;nc<=7;nc++) {
  Sc64ScarabChildFields x={slot,phase,life,nc},y;int carry=9,timer=9;int valid=nc<=life;assert(sc64_scarab_child_fields_pack(&x,&carry,&timer)==valid);if(!valid)continue;assert(sc64_scarab_child_fields_unpack(carry,timer,&y));assert(x.parent_slot==y.parent_slot&&x.phase==y.phase&&x.life==y.life&&x.no_collision==y.no_collision);++children;
  if((slot==0||slot==239)&&(life==0||life==7||life==90)&&(nc==0||nc==7)){Unit u=unit();u.carry=carry;u.work_timer=timer;codecs(u);}
 }
 puts("Scarab field packing and actual codec rows passed; child identity is surrogate");printf("parents=%lu children=%lu\n",parents,children);return 0;
}
