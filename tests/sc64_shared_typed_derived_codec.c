/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Test-only stage discriminator and real codec calls; never guest SRAM. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16; typedef uint32_t u32;
#define SC64_RACES 1
#define SC64_SHARED_EGG_WIRE_HOST_STAGE 1
#define SC64_SHARED_EGG_CUSTOM_CANDIDATE 1
#define SC64_SHARED_ACTOR_FULL_HOST_STAGE 1
#define SC64_SCARAB_STABLE_ROSTER_HOST_STAGE 1
#define SC64_GUARDIAN_DISTINCT_STAGE 1
#define SC64_SHARED_EGG_TYPE 40
#define SC_RU_LEGACY_BOUND 40
typedef struct { int hp; } Sc64StockUnitProfile;
static const Sc64StockUnitProfile *sc64_stock_unit_profile(int raw) { static const Sc64StockUnitProfile cocoon={200}; static const Sc64StockUnitProfile guardian={150},scarab={20}; return raw==59?&cocoon:raw==44?&guardian:raw==85?&scarab:0; }
enum { SC_RU_COUNT=64, SC_RB_COUNT=64, U_OVERLORD=8, U_COCOON=41, U_GUARDIAN_STAGE=42, U_SCARAB=43, MAX_UNITS=240, MAX_BUILDINGS=160 };
typedef struct { int hp, shields; } Sc64RaceUnitRule;
static Sc64RaceUnitRule sc64_race_unit_rules[SC_RU_COUNT];
static const Sc64RaceUnitRule *sc64_race_unit_rule(int t) { return t>=0&&t<SC_RU_COUNT ? &sc64_race_unit_rules[t] : 0; }
static int sc64_zp_actor_default_hp(int t) { return t==U_OVERLORD?200:t==U_GUARDIAN_STAGE?150:t==U_SCARAB?20:0; }
static int sc64_zp_actor_default_shields(int t) { (void)t; return 0; }
static int sc64_race_unit_armor(int t) { return t==U_GUARDIAN_STAGE?2:0; }
static int sc64_race_building_max_hp(int t) { (void)t; return 100; }
/* Field widths match the compact Z/P campaign Unit in game.c. */
typedef struct { int hp,maxhp,carry,shield; s16 x,y,tx,ty,build,target,path_x,path_y,raw_type,raw_owner; u16 cd,work_timer,weapon,path_timer,path_retry,flags,shield_remainder; u8 owner,type,alive,selected,order,armor; } Unit;
typedef struct { int x,y,hp,owner,type,alive,queue[5],queue_head,queue_count,queue_progress,build_progress,under_construction,level,shield,maxhp,raw_type,raw_owner,flags,larva,larva_timer,shield_remainder; } Building;
#include "../starcraft/sc64_derived_save.h"
#include "../starcraft/sc64_p7_typed_unit_save.h"
#include "../starcraft/sc64_scarab_flight_bridge.h"
#include "../starcraft/sc64_scarab_waypoint_bridge.h"

int main(void) {
    sc64_race_unit_rules[U_OVERLORD].hp=200;
    sc64_race_unit_rules[U_GUARDIAN_STAGE].hp=150;
    sc64_race_unit_rules[U_SCARAB].hp=20;
    Unit u={0}, out, unchanged; u.x=u.y=u.tx=u.ty=u.path_x=u.path_y=10;
    u.type=U_SCARAB; u.raw_type=85; u.hp=u.maxhp=20; u.alive=1;
    u.owner=1; u.raw_owner=3; u.build=-1; u.target=-1;
    Sc64DerivedUnitSave d, d_before; Sc64P7TypedUnitSave t;
    memset(&d,0xa5,sizeof d); d_before=d;
    assert(!sc64_derived_unit_encode_for(&d,&u,100,100,0));
    assert(!memcmp(&d,&d_before,sizeof d));
    assert(sc64_derived_unit_encode_for(&d,&u,100,100,4));
    memset(&out,0xa5,sizeof out); unchanged=out;
    assert(!sc64_derived_unit_decode_for(&out,&d,100,100,0));
    assert(!memcmp(&out,&unchanged,sizeof out));
    assert(sc64_derived_unit_decode_for(&out,&d,100,100,4));
    assert(out.type==u.type && out.raw_type==85 && out.hp==20);
    assert(sc64_p7_typed_unit_encode_for(&t,&u,100,100,4));
    assert(!sc64_p7_typed_unit_decode_for(&out,&t,100,100,0));
    assert(sc64_p7_typed_unit_decode_for(&out,&t,100,100,4));
    u.weapon=3;
    assert(!sc64_derived_unit_encode_for(&d,&u,100,100,4));
    assert(sc64_derived_unit_encode_for(&d,&u,100,100,12));
    assert(!sc64_derived_unit_decode_for(&out,&d,100,100,4));
    assert(sc64_derived_unit_decode_for(&out,&d,100,100,12) && out.weapon==3);
    u.weapon=4; assert(!sc64_derived_unit_encode_for(&d,&u,100,100,12));
    u.weapon=0;
    u.alive=0; u.hp=0;
    assert(sc64_p7_typed_unit_encode_for(&t,&u,100,100,4));
    assert(sc64_p7_typed_unit_decode_for(&out,&t,100,100,4) && !out.alive);
    u.raw_type=36; assert(!sc64_derived_unit_encode_for(&d,&u,100,100,4));
    assert(!sc64_p7_typed_unit_encode_for(&t,&u,100,100,4));
    u.type=U_GUARDIAN_STAGE; u.raw_type=44; u.hp=u.maxhp=150; u.armor=2; u.alive=1;
    assert(sc64_derived_unit_encode_for(&d,&u,100,100,2));
    assert(!sc64_derived_unit_decode_for(&out,&d,100,100,0));
    assert(sc64_p7_typed_unit_encode_for(&t,&u,100,100,2));
    assert(!sc64_p7_typed_unit_decode_for(&out,&t,100,100,0));
    u.type=U_OVERLORD; u.raw_type=59; u.hp=u.maxhp=200; u.armor=0; u.target=44; u.work_timer=750;
    assert(sc64_p7_typed_unit_encode_for(&t,&u,100,100,2));
    assert(sc64_p7_typed_unit_decode_for(&out,&t,100,100,2));
    assert(out.raw_type==59 && out.maxhp==200 && out.armor==0);
    /* Compose both transforms and both real leaf codecs with large previous
     * source waypoints and nonzero flight motion bits. */
    Unit motion={0}, packed, unpacked;
    motion.x=motion.y=motion.tx=motion.ty=10; motion.path_x=201; motion.path_y=180;
    motion.type=U_SCARAB; motion.raw_type=85; motion.owner=1; motion.raw_owner=3;
    motion.hp=motion.maxhp=20; motion.alive=1; motion.carry=2<<8;
    motion.work_timer=10; motion.order=2; motion.build=-1; motion.target=-1;
    motion.weapon=3; motion.path_timer=4000; motion.path_retry=3000; motion.shield=7;
    assert(sc64_scarab_waypoint_to_wire(&motion,&packed,2048,2048,15,SC64_SCARAB_WAYPOINT_DERIVED));
    assert(sc64_scarab_flight_to_wire(&packed,&packed,1,0));
    assert(sc64_derived_unit_encode_for(&d,&packed,2048,2048,15));
    assert(sc64_derived_unit_decode_for(&unpacked,&d,2048,2048,15));
    assert(sc64_scarab_flight_from_wire(&unpacked,&unpacked,1,0));
    assert(sc64_scarab_waypoint_from_wire(&unpacked,&unpacked,2048,2048,15,SC64_SCARAB_WAYPOINT_DERIVED));
    assert(unpacked.path_timer==4000 && unpacked.path_retry==3000 && unpacked.weapon==3 &&
           unpacked.carry==motion.carry && unpacked.work_timer==motion.work_timer &&
           unpacked.path_x==motion.path_x && unpacked.path_y==motion.path_y && unpacked.shield==motion.shield);
    assert(sc64_scarab_waypoint_to_wire(&motion,&packed,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(sc64_scarab_flight_to_wire(&packed,&packed,1,0));
    assert(sc64_p7_typed_unit_encode_for(&t,&packed,2048,2048,15));
    assert(sc64_p7_typed_unit_decode_for(&unpacked,&t,2048,2048,15));
    assert(sc64_scarab_flight_from_wire(&unpacked,&unpacked,1,0));
    assert(sc64_scarab_waypoint_from_wire(&unpacked,&unpacked,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(unpacked.path_timer==4000 && unpacked.path_retry==3000 && unpacked.weapon==3 &&
           unpacked.carry==motion.carry && unpacked.work_timer==motion.work_timer &&
           unpacked.path_x==motion.path_x && unpacked.path_y==motion.path_y && unpacked.shield==motion.shield);
    u.raw_type=36; u.target=-1; u.work_timer=0;
    assert(sc64_derived_unit_encode(&d,&u,100,100));
    assert(sc64_derived_unit_decode(&out,&d,100,100) && out.raw_type==36);
    assert(sc64_p7_typed_unit_encode(&t,&u,100,100));
    assert(sc64_p7_typed_unit_decode(&out,&t,100,100) && out.raw_type==36);
    return 0;
}
