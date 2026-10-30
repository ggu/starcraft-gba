/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int16_t s16;
#define SC64_RACES 1
enum { MAX_UNITS=240, MAX_BUILDINGS=160 };
#include "../starcraft/sc64_stock_profiles.h"
#include "../starcraft/race_rules.h"
#ifndef SC64_ZP_CAMPAIGN_ACTORS
static inline int sc64_zp_actor_default_hp(int t) {(void)t;return 0;}
static inline int sc64_zp_actor_default_shields(int t) {(void)t;return 0;}
#endif
/* SC64_REAL_UNIT_DEFINITION */
typedef struct { int x,y,hp,owner,type,alive,queue[5],queue_head,queue_count,queue_progress,build_progress,under_construction,level,shield,maxhp,raw_type,raw_owner,flags,larva,larva_timer,shield_remainder; } Building;
#include "../starcraft/sc64_compact_unit_save.h"
#include "../starcraft/sc64_zp_unit_save.h"
#include "../starcraft/sc64_derived_save.h"
#include "../starcraft/sc64_p7_typed_unit_save.h"
#include "../starcraft/sc64_scarab_flight_bridge.h"
static Unit base(void) {Unit q={0};q.x=q.y=q.tx=q.ty=q.path_x=q.path_y=10;q.type=U_SCARAB;q.raw_type=85;q.raw_owner=0;q.alive=1;q.hp=q.maxhp=20;q.shield=10;q.armor=sc64_race_unit_armor(U_SCARAB);q.target=-1;q.build=-1;q.cd=255;q.weapon=0xe7a5;q.flags=0x55;q.path_timer=24;q.path_retry=12;q.shield_remainder=1279;return q;}
static void same(Unit a,Unit b,int derived) {if(derived)a.weapon=0;assert(!memcmp(&a,&b,sizeof(a)));}
#if !defined(SC64_LATE_TERRAN_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
#define RACE_UNIT_WORDS (sizeof(Unit)/sizeof(int))
static struct { s16 units[MAX_UNITS][RACE_UNIT_WORDS]; int state[32]; } race_data;
static Unit u[MAX_UNITS];
#endif
static void roundtrip(Unit q) {
 Unit w,z;Sc64PackedUnitSave a;Sc64ZpPackedUnitSave b;Sc64DerivedUnitSave c;Sc64P7TypedUnitSave d;
 assert(sc64_scarab_flight_to_wire(&q,&w,1,0));
 assert(sc64_unit_save_encode(&a,&w,2048,2048)&&sc64_unit_save_decode(&z,&a,2048,2048)&&sc64_scarab_flight_from_wire(&z,&z,1,0));same(q,z,0);
 assert(sc64_zp_unit_save_encode(&b,&w,2048,2048)&&sc64_zp_unit_save_decode(&z,&b,2048,2048)&&sc64_scarab_flight_from_wire(&z,&z,1,0));same(q,z,0);
 assert(sc64_derived_unit_encode(&c,&w,2048,2048)&&sc64_derived_unit_decode(&z,&c,2048,2048)&&sc64_scarab_flight_from_wire(&z,&z,1,0));same(q,z,1);
 assert(sc64_p7_typed_unit_encode(&d,&w,2048,2048)&&sc64_p7_typed_unit_decode(&z,&d,2048,2048)&&sc64_scarab_flight_from_wire(&z,&z,1,0));same(q,z,0);
 q.weapon=0x67a5; /* legacy signed-16 path cannot preserve bit 15 */
 assert(sc64_scarab_flight_to_wire(&q,&w,1,1));
#if !defined(SC64_LATE_TERRAN_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
 memset(u,0,sizeof(u));u[0]=w;memset(&race_data,0,sizeof(race_data));
 /* SC64_REAL_LEGACY_ENCODE */
 memset(u,0,sizeof(u));
 /* SC64_REAL_LEGACY_DECODE */
 z=u[0];
#else
 z=w;
#endif
 assert(sc64_scarab_flight_from_wire(&z,&z,1,1));same(q,z,0);
}
int main(void) {
 Unit q=base(),w,z,sentinel;unsigned c,k;
 for(unsigned i=0;i<SC64_SCARAB_FLIGHT_KEYS;i++){unsigned lo=i%131072u, hi=i/131072u, digit=hi%242u, flags=hi/242u; assert(flags<4u && lo+131072u*(digit+242u*flags)==i);assert(sc64_scarab_flight_unkey(i,&c));unsigned phase=(c>>8)&3u;assert(sc64_scarab_flight_key(c,phase<2?0:1,0,-1,&k)&&k==i);}
 for(unsigned i=SC64_SCARAB_FLIGHT_KEYS;i<131072u*242u*4u;i++){c=123;assert(!sc64_scarab_flight_unkey(i,&c)&&c==123);}
 static const unsigned fr[]={0,1,131071,131072,262143};
 for(unsigned ph=0;ph<4;ph++)for(unsigned p=0;p<241;p+=p==0?239:1)for(unsigned f=0;f<5;f++)for(unsigned order=0;order<5;order++) {
  unsigned slot=p==240?255:p;q=base();q.carry=(int)(slot|(ph<<8)|((ph<2?0:fr[f])<<10));q.work_timer=ph<2?0:90|(7<<7);q.order=ph<2?0:(int)order;if(ph<2&&p==240)continue;roundtrip(q);
 }
 q=base();q.carry=2<<8;q.work_timer=1;q.order=4;sentinel=base();z=sentinel;
 assert(!sc64_scarab_flight_to_wire(&q,&z,0,0)&&!memcmp(&z,&sentinel,sizeof(z)));
 assert(sc64_scarab_flight_to_wire(&q,&w,1,0));w.order=7;z=sentinel;assert(!sc64_scarab_flight_from_wire(&w,&z,1,0)&&!memcmp(&z,&sentinel,sizeof(z)));
 q.carry=1<<10;q.work_timer=0;q.order=0;assert(!sc64_scarab_flight_to_wire(&q,&z,1,0));
 w=base();w.carry=65535;w.build=239;w.order=8;w.work_timer=1024;z=sentinel;assert(!sc64_scarab_flight_from_wire(&w,&z,1,0)&&!memcmp(&z,&sentinel,sizeof(z)));
 q=base();q.carry=2<<8;q.work_timer=0;assert(!sc64_scarab_flight_to_wire(&q,&z,1,0));
 q=base();q.target=0;assert(!sc64_scarab_flight_to_wire(&q,&z,1,0));
 q=base();q.type=U_REAVER;z=sentinel;assert(!sc64_scarab_flight_to_wire(&q,&z,1,0)&&!memcmp(&z,&sentinel,sizeof(z)));
 q=base();q.type=U_REAVER;q.raw_type=83;assert(sc64_scarab_flight_to_wire(&q,&z,0,0));same(q,z,0);
 puts("Scarab bridge exhaustive key ranking and four codec roundtrips passed");
}
