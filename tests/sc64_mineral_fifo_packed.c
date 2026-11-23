/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free packed row and ranked mineral queue contract. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint32_t u32;
#define SC64_RACES 1
enum { SC_RU_COUNT=64, SC_RB_COUNT=64, U_OVERLORD=8, U_COCOON=64,
       MAX_UNITS=240, MAX_BUILDINGS=160, MAX_MINERALS=144 };
typedef struct { int hp; } Sc64StockUnitProfile;
static const Sc64StockUnitProfile *sc64_stock_unit_profile(int raw) { static const Sc64StockUnitProfile p={200}; return raw==59?&p:0; }
typedef struct { int hp, shields; } Sc64RaceUnitRule;
static Sc64RaceUnitRule sc64_race_unit_rules[SC_RU_COUNT];
static const Sc64RaceUnitRule *sc64_race_unit_rule(int t) { return t>=0&&t<SC_RU_COUNT?&sc64_race_unit_rules[t]:0; }
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
#include "../starcraft/sc64_mineral_fifo.h"
#include "../starcraft/sc64_mineral_fifo_rank.h"

static Unit sample(void) {
    Unit q={0}; q.x=30; q.y=40; q.tx=31; q.ty=41; q.path_x=32; q.path_y=42;
    q.hp=137; q.maxhp=200; q.type=U_OVERLORD; q.raw_type=8; q.raw_owner=3;
    q.owner=1; q.alive=1; q.selected=1; q.cd=7; q.order=3; q.build=-1;
    q.target=4; q.path_timer=3; q.path_retry=2; q.flags=1; q.shield_remainder=17;
    return q;
}

static void assert_dense_queue(const Sc64MineralFifo *fifo, unsigned patch)
{
    unsigned link = fifo->head[patch];
    for (unsigned rank = 1; rank <= MAX_UNITS; ++rank) {
        unsigned slot = 0;
        while ((slot * 97u) % MAX_UNITS + 1u != rank) ++slot;
        assert(link == slot + 1u);
        assert(fifo->patch[slot] == patch + 1u);
        link = fifo->next[slot];
    }
    assert(!link);
}

/* Decode actual wire rows, then construct the queue from those fields. */
#define CHECK_CODEC(T, ENCODE, DECODE) do { \
    Sc64MineralFifo fifo={0}; \
    for (unsigned slot=0; slot<MAX_UNITS; ++slot) { \
        unsigned rank=(slot*97u)%MAX_UNITS+1u; \
        Unit expected=sample(), wire, decoded; T packed; \
        expected.target=MAX_MINERALS-1; expected.work_timer=rank==1?45:0; \
        wire=expected; wire.carry=8+(int)rank; \
        assert(ENCODE(&packed,&wire,2048,2048)); \
        assert(DECODE(&decoded,&packed,2048,2048)); \
        assert(!memcmp(&decoded,&wire,sizeof decoded)); \
        assert(sc64_mineral_fifo_stage_rank(&fifo,slot,(unsigned)decoded.target, \
               (unsigned)(decoded.carry-8),(unsigned)decoded.work_timer,45)); \
        decoded.carry=0; assert(!memcmp(&decoded,&expected,sizeof decoded)); \
    } \
    assert(sc64_mineral_fifo_finish_ranks(&fifo)); \
    assert_dense_queue(&fifo,MAX_MINERALS-1); \
} while (0)
static void codecs(void) {
    CHECK_CODEC(Sc64PackedUnitSave,sc64_unit_save_encode,sc64_unit_save_decode);
    CHECK_CODEC(Sc64ZpPackedUnitSave,sc64_zp_unit_save_encode,sc64_zp_unit_save_decode);
    CHECK_CODEC(Sc64DerivedUnitSave,sc64_derived_unit_encode,sc64_derived_unit_decode);
    CHECK_CODEC(Sc64P7TypedUnitSave,sc64_p7_typed_unit_encode,sc64_p7_typed_unit_decode);
    /* Nonworker overloaded carry remains untouched by queue normalization. */
    Unit wire=sample(); wire.carry=137; wire.work_timer=31;
#define CHECK_UNTOUCHED(T, ENCODE, DECODE) do { T row; Unit q; \
    assert(ENCODE(&row,&wire,2048,2048)); assert(DECODE(&q,&row,2048,2048)); \
    assert(!memcmp(&q,&wire,sizeof q)); } while(0)
    CHECK_UNTOUCHED(Sc64PackedUnitSave,sc64_unit_save_encode,sc64_unit_save_decode);
    CHECK_UNTOUCHED(Sc64ZpPackedUnitSave,sc64_zp_unit_save_encode,sc64_zp_unit_save_decode);
    CHECK_UNTOUCHED(Sc64DerivedUnitSave,sc64_derived_unit_encode,sc64_derived_unit_decode);
    CHECK_UNTOUCHED(Sc64P7TypedUnitSave,sc64_p7_typed_unit_encode,sc64_p7_typed_unit_decode);
}
static void multi(void) {
    Sc64MineralFifo q={0};
    const unsigned slot[]={7,3,9,1,8,2}, patch[]={0,1,0,1,0,1}, rank[]={3,2,1,3,2,1};
    for (unsigned i=0;i<6;++i)
        assert(sc64_mineral_fifo_stage_rank(&q,slot[i],patch[i],rank[i],rank[i]==1?12:0,45));
    assert(sc64_mineral_fifo_finish_ranks(&q));
    const unsigned expected[2][3]={{9,8,7},{2,3,1}};
    for (unsigned p=0;p<2;++p) {
        unsigned link=q.head[p];
        for (unsigned i=0;i<3;++i) { assert(link==expected[p][i]+1); link=q.next[link-1]; }
        assert(!link && q.tail[p]==expected[p][2]+1);
    }
    memset(&q, 0, sizeof q);
    assert(sc64_mineral_fifo_stage_rank(&q, 0, 0, 1, 0, 45));
    assert(sc64_mineral_fifo_stage_rank(&q, 1, MAX_MINERALS-1, 1, 45, 45));
    assert(sc64_mineral_fifo_stage_rank(&q, 2, MAX_MINERALS-1, 2, 0, 45));
    assert(sc64_mineral_fifo_finish_ranks(&q));
    assert(q.head[0] == 1 && q.tail[0] == 1 && !q.next[0]);
    for (unsigned p=1; p<MAX_MINERALS-1; ++p)
        assert(!q.head[p] && !q.tail[p]);
    assert(q.head[MAX_MINERALS-1] == 2 && q.next[1] == 3);
    assert(q.tail[MAX_MINERALS-1] == 3 && !q.next[2]);
}
static void invalid(void) {
    Sc64MineralFifo q={0};
    assert(sc64_mineral_fifo_stage_rank(&q,0,0,1,0,45));
    int duplicate=sc64_mineral_fifo_stage_rank(&q,1,0,1,0,45);
    assert(!duplicate || !sc64_mineral_fifo_finish_ranks(&q));
    memset(&q,0,sizeof q);
    assert(sc64_mineral_fifo_stage_rank(&q,0,0,2,0,45));
    assert(!sc64_mineral_fifo_finish_ranks(&q));
    memset(&q,0,sizeof q);
    assert(!sc64_mineral_fifo_stage_rank(&q,0,0,2,1,45));
    assert(!sc64_mineral_fifo_stage_rank(&q,0,0,1,46,45));
    assert(!sc64_mineral_fifo_stage_rank(&q,0,0,0,0,45));
    assert(!sc64_mineral_fifo_stage_rank(&q,MAX_UNITS,0,1,0,45));
    assert(!sc64_mineral_fifo_stage_rank(&q,0,MAX_MINERALS,1,0,45));
}
int main(void) {
    sc64_race_unit_rules[U_OVERLORD].hp=200;
    codecs(); multi(); invalid();
    puts("packed mineral FIFO ranks passed"); return 0;
}
