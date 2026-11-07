/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Test-only stage discriminator and real codec calls; never guest SRAM. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint8_t u8; typedef uint32_t u32;
#define SC64_RACES 1
typedef struct { int hp; } Sc64StockUnitProfile;
static const Sc64StockUnitProfile *sc64_stock_unit_profile(int raw) { static const Sc64StockUnitProfile cocoon={200}; return raw==59?&cocoon:0; }
enum { SC_RU_COUNT=64, SC_RB_COUNT=64, U_OVERLORD=8, U_COCOON=64, MAX_UNITS=240, MAX_BUILDINGS=160 };
typedef struct { int hp, shields; } Sc64RaceUnitRule;
static Sc64RaceUnitRule sc64_race_unit_rules[SC_RU_COUNT];
static const Sc64RaceUnitRule *sc64_race_unit_rule(int t) { return t>=0&&t<SC_RU_COUNT ? &sc64_race_unit_rules[t] : 0; }
static int sc64_zp_actor_default_hp(int t) { return t==U_OVERLORD?200:0; }
static int sc64_zp_actor_default_shields(int t) { (void)t; return 0; }
static int sc64_race_unit_armor(int t) { (void)t; return 0; }
static int sc64_race_building_max_hp(int t) { (void)t; return 100; }
typedef struct { int x,y,hp,maxhp,owner,type,alive,selected,cd,order,tx,ty,carry,build,work_timer,armor,weapon,target,path_timer,path_retry,path_x,path_y,shield,raw_type,raw_owner,flags,shield_remainder; } Unit;
typedef struct { int x,y,hp,owner,type,alive,queue[5],queue_head,queue_count,queue_progress,build_progress,under_construction,level,shield,maxhp,raw_type,raw_owner,flags,larva,larva_timer,shield_remainder; } Building;
#include "../starcraft/sc64_compact_unit_save.h"
#include "../starcraft/sc64_zp_unit_save.h"
#include "../starcraft/sc64_derived_save.h"
#include "../starcraft/sc64_p7_typed_unit_save.h"

static int normalize(const Unit *in, Unit *out, int stage)
{
    if (!in || !out) return 0;
    if (in->type != U_COCOON) { if (in->raw_type == 59) return 0; *out=*in; return 1; }
    if (!stage || in->raw_type!=59 || in->hp<1 || in->hp>200 || in->maxhp!=200 ||
        in->target!=44 || in->work_timer<1 || in->work_timer>750 || in->build!=-1 ||
        in->carry || in->flags || in->order || in->armor || in->shield || !in->alive) return 0;
    *out=*in; out->type=U_OVERLORD; return 1;
}
static int restore(const Unit *in, Unit *out, int stage)
{
    if (!in || !out) return 0;
    if (in->raw_type!=59) { *out=*in; return 1; }
    if (!stage || in->type!=U_OVERLORD) return 0;
    Unit tmp=*in, check; tmp.type=U_COCOON;
    if (!normalize(&tmp,&check,stage)) return 0;
    *out=tmp; return 1;
}
#define ROUND(codec,enc,dec) do { codec bytes; Unit wire, decoded, output; \
 assert(normalize(&source,&wire,1)); assert(enc(&bytes,&wire,2048,2048)); \
 assert(dec(&decoded,&bytes,2048,2048)); assert(restore(&decoded,&output,1)); \
 assert(!memcmp(&source,&output,sizeof(Unit))); assert(!restore(&decoded,&output,0)); \
 assert(!enc(&bytes,&source,2048,2048)); } while(0)
int main(void)
{
    sc64_race_unit_rules[U_OVERLORD].hp=200;
    Unit source={0}; source.x=30;source.y=40;source.tx=30;source.ty=40;
    source.path_x=30;source.path_y=40;source.hp=200;source.maxhp=200;
    source.type=U_COCOON;source.raw_type=59;source.raw_owner=3;source.owner=1;
    source.alive=1;source.selected=1;source.build=-1;source.target=44;source.work_timer=750;
    for(int i=0;i<2;i++) {
        if(i){source.hp=37;source.work_timer=1;source.selected=0;source.owner=4;}
        ROUND(Sc64PackedUnitSave,sc64_unit_save_encode,sc64_unit_save_decode);
        ROUND(Sc64ZpPackedUnitSave,sc64_zp_unit_save_encode,sc64_zp_unit_save_decode);
        ROUND(Sc64DerivedUnitSave,sc64_derived_unit_encode,sc64_derived_unit_decode);
        ROUND(Sc64P7TypedUnitSave,sc64_p7_typed_unit_encode,sc64_p7_typed_unit_decode);
    }
    Unit w; assert(!normalize(&source,&w,0));
#define BAD(field,value) do { Unit b=source;b.field=value;assert(!normalize(&b,&w,1)); } while(0)
    BAD(raw_type,44); BAD(target,43); BAD(work_timer,0); BAD(work_timer,751);
    BAD(build,0); BAD(carry,1); BAD(flags,1); BAD(order,1); BAD(maxhp,150);
    BAD(hp,0); BAD(hp,201); BAD(type,U_OVERLORD);
#undef BAD
    return 0;
}
