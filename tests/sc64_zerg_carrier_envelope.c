/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ZC_LAYOUT_ONLY
#include <assert.h>
#include <stdio.h>
#include <string.h>
#endif
#include "../starcraft/game.c"
#include "../starcraft/sc64_zerg_carrier_envelope.h"

#ifdef SC64_ZC_LAYOUT_ONLY
const unsigned sc64_zc_layout_receipt[]={
    SC64_ZC_HEADER,SC64_ZC_PACKET_MAX,SC64_ZC_WIRE_MAX,SC64_ZC_LOGICAL_MAX,SC64_ZC_WORK_BYTES
};
unsigned sc64_zc_arm_pack(u8 *work,unsigned bytes,unsigned packet_length)
{
    return sc64_zc_pack(work,bytes,work+SC64_ZC_PACKET_WORK,packet_length,
                        work,SC64_ZC_WORK_BYTES);
}
unsigned sc64_zc_arm_unpack(u8 *work,unsigned bytes,unsigned *packet_length)
{
    return sc64_zc_unpack_in_place(work,bytes,SC64_ZC_WORK_BYTES,packet_length);
}
#else

#define SC64_ZC_TEST_TAG ((SC64_TRANSPORT_SAVE_VERSION&~0x00ff0000u)|0x004e0000u)
_Static_assert(RACE_IMPACT_SAVE_CAPACITY == 8 && RACE_IMPACT_CAPACITY == 32,
    "the envelope must preserve both legacy and full live impact capacities");
_Static_assert(SC64_ZC_TEST_TAG != SC64_TRANSPORT_SAVE_VERSION,"distinct journal tag");

static u8 old_wire[SC64_ZC_LOGICAL_MAX], rebuilt[SC64_ZC_LOGICAL_MAX];
static u8 encoded[RACE_IMPACT_RECORD_MAX], selection[RACE_IMPACT_RECORD_MAX];
static u8 packet[SC64_ZC_PACKET_MAX], restored_packet[SC64_ZC_PACKET_MAX];
static u8 scratch[SC64_ZC_WORK_BYTES+4];
static u16 pool_words[SC64_ARENA_WORDS], restore_words[SC64_ARENA_WORDS];
static RaceLoadPlan plan;
static int impact_count;
static const u32 tags[]={SC64_TRANSPORT_SAVE_VERSION,SC64_ZC_TEST_TAG};

static void clear_journal(void)
{
    for (int i=0;i<2;++i) {
        save_test_corrupt_latest(0,tags[0]);
        save_test_corrupt_latest(0,tags[1]);
    }
}

static void setup(int map,int human,int bots)
{
    game_init();
    race_mode=1; race_human=human; race_bots=bots; race_skirmish_map=map;
    race_bot_races[0]=R_TER; race_bot_races[1]=R_PROTOSS; race_bot_races[2]=R_ZERG;
    init_state();
    assert(screen==SCREEN_PLAY && race_cols==64 && race_rows==64);
}

/* This uses the real predecessor preflight for the base, actor slots and
 * transport. The storage study checks the full extended impact block
 * separately because the older preflight only has an eight-row buffer. */
static int prepare(const void *raw,u32 len,u32 tag,void *ctx)
{
    (void)ctx;
    unsigned selected_len=len;
    impact_count=0;
    if (tag==SC64_ZC_TEST_TAG) {
        selected_len=sc64_zc_unpack(raw,len,rebuilt,sizeof(rebuilt));
        if (!selected_len) return 0;
        unsigned impacts=selected_len-sizeof(RaceSave)-SC64_TRANSPORT_TRAILER_BYTES;
        Sc64ImpactEvent rows[RACE_IMPACT_ROWS];
        if (impacts) {
            impact_count=sc64_impact_decode(rebuilt+sizeof(RaceSave),impacts,RACE_IMPACT_ROWS,
                MAX_UNITS+MAX_BUILDINGS,5,SC_RU_COUNT,rows);
            if (impact_count<1) return 0;
        }
        if (impact_count>RACE_IMPACT_SAVE_CAPACITY) {
            for (int i=0;i<impact_count;++i) {
                unsigned target=(unsigned)sc64_impact_event_target(&rows[i]);
                if (target<MAX_UNITS) {
                    Unit unit;
                    Sc64ZpPackedUnitSave row;
                    memcpy(&row,rebuilt+offsetof(RaceSave,units)+target*sizeof(row),sizeof(row));
                    if (!sc64_zp_unit_save_decode(&unit,&row,1024,1024) ||
                        !unit.alive || SC64_TRANSPORT_LOADED(&unit)) return 0;
                } else {
                    Building building;
                    Sc64PackedBuildingSave row;
                    memcpy(&row,rebuilt+offsetof(RaceSave,buildings)+
                           (target-MAX_UNITS)*sizeof(row),sizeof(row));
                    if (!sc64_building_save_decode(&building,&row) || !building.alive) return 0;
                }
            }
            memmove(rebuilt+sizeof(RaceSave),rebuilt+selected_len-SC64_TRANSPORT_TRAILER_BYTES,
                    SC64_TRANSPORT_TRAILER_BYTES);
            selected_len=sizeof(RaceSave)+SC64_TRANSPORT_TRAILER_BYTES;
        }
        raw=rebuilt;
    } else if (tag!=SC64_TRANSPORT_SAVE_VERSION) return 0;
    if (selected_len>sizeof(race_impact_record)) return 0;
    memcpy(&race_impact_record,raw,selected_len);
    return race_worker_cargo_prepare_selected(&race_impact_record,selected_len,tags[0],&plan);
}

static void fill_packet(void)
{
    Sc64Arena arena={pool_words,SC64_ARENA_WORDS};
    assert(sc64_pool_init(&arena));
    for (unsigned i=0;i<SC64_POOL_CAPACITY;++i) {
        u8 id,native;
        assert(sc64_compact_pool_acquire(&arena,SC64_POOL_CAPACITY,&id,&native));
        Sc64ProjectileRow shot={.target_ref=511,.owner_ref=255,.x_q8_source=(int)(i*512u),
                                 .next_speed=531,.turn_rate_raw=127,.state=1};
        assert(sc64_pool_set_row(&arena,id,&shot));
    }
    assert(sc64_compact_codec_max_bytes(SC64_POOL_CAPACITY)==sizeof(packet));
    assert(sc64_compact_codec_encode(&arena,SC64_POOL_CAPACITY,packet,sizeof(packet))==sizeof(packet));
}

static void roundtrip_scratch(unsigned logical,unsigned wire,const u8 *p,unsigned packet_bytes)
{
    memset(scratch,0x49,sizeof(scratch));
    memcpy(scratch,old_wire,logical);
    memcpy(scratch+SC64_ZC_PACKET_WORK,p,packet_bytes);
    assert(sc64_zc_pack(scratch,logical,scratch+SC64_ZC_PACKET_WORK,packet_bytes,
                        scratch,SC64_ZC_WORK_BYTES)==wire);
    assert(!memcmp(scratch,encoded,wire));
    unsigned opened=0x7fff;
    assert(!sc64_zc_unpack_in_place(scratch,wire,SC64_ZC_WORK_BYTES-1,&opened));
    assert(opened==0x7fff && !memcmp(scratch,encoded,wire));
    assert(sc64_zc_unpack_in_place(scratch,wire,SC64_ZC_WORK_BYTES,&opened)==logical);
    assert(opened==packet_bytes && !memcmp(scratch+SC64_ZC_PACKET_WORK,p,packet_bytes));
    assert(!memcmp(scratch,old_wire,logical));
    for (unsigned i=SC64_ZC_WORK_BYTES;i<sizeof(scratch);++i) assert(scratch[i]==0x49);
}

/* All slots retain arbitrary admitted bit patterns, including zero rows and
 * every packed bit phase. Test separately from semantic game preflight. */
static void bit_layout(void)
{
    u32 r=0x4d52aa63;
    for (unsigned i=0;i<sizeof(RaceSave);++i) {
        r^=r<<13; r^=r>>17; r^=r<<5;
        old_wire[i]=(u8)r;
    }
    for (unsigned i=0;i<MAX_UNITS+MAX_BUILDINGS;++i) {
        int building=i>=MAX_UNITS;
        unsigned row_bits=building ? SC64_ZC_BUILDING_BITS : SC64_ZC_UNIT_BITS;
        unsigned owner=building ? SC64_ZC_BUILDING_OWNER : SC64_ZC_UNIT_OWNER;
        unsigned slot=building ? i-MAX_UNITS : i;
        u8 *rbytes=old_wire+(building ? offsetof(RaceSave,buildings) : offsetof(RaceSave,units))+
                   slot*row_bits/8;
        for (unsigned bit=0;bit<row_bits;++bit)
            if (bit==10 || bit==21 || (bit>=owner+4 && bit<owner+9) ||
                (building && bit>=SC64_ZC_BUILDING_USED))
                rbytes[bit/8] &= (u8)~(1u<<(bit%8));
    }
    memset(old_wire+offsetof(RaceSave,z4),0,sizeof(RaceSave)-offsetof(RaceSave,z4));
    memset(old_wire+offsetof(RaceSave,fog)+512,0,1536);
    u32 version=SAVE_VERSION;
    int custom=1,map=0;
    memcpy(old_wire,&version,sizeof(version));
    memcpy(old_wire+offsetof(RaceSave,state),&custom,sizeof(custom));
    memcpy(old_wire+offsetof(RaceSave,state)+20*sizeof(int),&map,sizeof(map));
    Sc64SkTransportTech no_transport={0};
    sc64_sk_transport_encode(old_wire+sizeof(RaceSave),&no_transport);
    unsigned n=sc64_zc_pack(old_wire,sizeof(RaceSave)+SC64_TRANSPORT_TRAILER_BYTES,packet,sizeof(packet),
                            encoded,sizeof(encoded));
    assert(n && sc64_zc_unpack(encoded,n,rebuilt,sizeof(rebuilt))==
           sizeof(RaceSave)+SC64_TRANSPORT_TRAILER_BYTES);
    assert(!memcmp(old_wire,rebuilt,sizeof(RaceSave)+SC64_TRANSPORT_TRAILER_BYTES));
    roundtrip_scratch(sizeof(RaceSave)+SC64_TRANSPORT_TRAILER_BYTES,n,packet,sizeof(packet));
}

static void saturated(unsigned map)
{
    clear_journal();
    setup((int)map,R_ZERG,1);
    int a=spawn_unit(SIDE,U_DRONE,111,152), bslot=spawn_build(ENEMY,B_BARRACKS,600,601,0);
    assert(a>=0 && bslot>=0);
    for (unsigned i=0;i<RACE_IMPACT_SAVE_CAPACITY;++i)
        race_impact_launch(i%2?a:MAX_UNITS+bslot,1+(int)i,1,ENEMY,0,U_MARINE,20u+i);
    assert(race_save_game());
    u32 legacy_size;
    assert(save_read_latest(old_wire,sizeof(old_wire),tags[0],&legacy_size));
    assert(legacy_size==RACE_IMPACT_RECORD_MAX);
    unsigned n=sc64_zc_pack(old_wire,legacy_size,packet,sizeof(packet),encoded,sizeof(encoded));
    assert(n && sc64_zc_unpack(encoded,n,rebuilt,sizeof(rebuilt))==legacy_size);
    assert(!memcmp(rebuilt,old_wire,legacy_size));

    u8 transport[SC64_TRANSPORT_TRAILER_BYTES];
    memcpy(transport,old_wire+legacy_size-sizeof(transport),sizeof(transport));
    for (unsigned i=RACE_IMPACT_SAVE_CAPACITY;i<RACE_IMPACT_ROWS;++i)
        race_impact_launch(i%2?a:MAX_UNITS+bslot,1+(int)i,1,ENEMY,0,U_MARINE,20u+i);
    assert(race_impact_count==RACE_IMPACT_ROWS && !race_save_game());
    Sc64ImpactEvent rows[RACE_IMPACT_ROWS];
    for (unsigned i=0;i<RACE_IMPACT_ROWS;++i) rows[i]=race_impact_rows[i];
    unsigned bytes=sc64_impact_encode(rows,RACE_IMPACT_ROWS,old_wire+sizeof(RaceSave));
    assert(bytes+sizeof(transport)==SC64_ZC_TAIL_MAX);
    memcpy(old_wire+sizeof(RaceSave)+bytes,transport,sizeof(transport));
    n=sc64_zc_pack(old_wire,sizeof(old_wire),packet,sizeof(packet),encoded,sizeof(encoded));
    assert(n==SC64_ZC_WIRE_MAX);
    assert(sc64_zc_unpack(encoded,n,rebuilt,sizeof(rebuilt))==sizeof(old_wire));
    assert(!memcmp(old_wire,rebuilt,sizeof(rebuilt)));
    roundtrip_scratch(sizeof(old_wire),n,packet,sizeof(packet));
    Sc64Arena restored={restore_words,SC64_ARENA_WORDS};
    assert(sc64_compact_codec_decode(&restored,encoded+SC64_ZC_HEADER,sizeof(packet),SC64_POOL_CAPACITY));
    assert(sc64_compact_codec_encode(&restored,SC64_POOL_CAPACITY,restored_packet,sizeof(restored_packet))==sizeof(packet));
    assert(!memcmp(restored_packet,packet,sizeof(packet)));

    /* Failure leaves the destination untouched, including unsupported
     * geometry/owner bits and mission/fog data that may not be discarded. */
    u8 sentinel[RACE_IMPACT_RECORD_MAX];
    memset(sentinel,0x3e,sizeof(sentinel));
    const unsigned rejected[]={ (unsigned)offsetof(RaceSave, fog)+512,
                                (unsigned)offsetof(RaceSave, z4),
                                (unsigned)offsetof(RaceSave,units)+1,
                                (unsigned)offsetof(RaceSave,units)+(SC64_ZC_UNIT_OWNER+4)/8,
                                (unsigned)offsetof(RaceSave,buildings)+27 };
    const unsigned bits[]={1,1,4,16,128};
    for (unsigned i=0;i<sizeof(rejected)/sizeof(*rejected);++i) {
        assert(!(old_wire[rejected[i]]&bits[i]));
        old_wire[rejected[i]]|=(u8)bits[i];
        assert(!sc64_zc_pack(old_wire,sizeof(old_wire),packet,sizeof(packet),sentinel,sizeof(sentinel)));
        old_wire[rejected[i]]^=(u8)bits[i];
    }
    assert(!sc64_zc_pack(old_wire,sizeof(old_wire),packet,sizeof(packet),sentinel,n-1));
    for (unsigned i=0;i<sizeof(sentinel);++i) assert(sentinel[i]==0x3e);
    memset(rebuilt,0xa5,sizeof(rebuilt));
    assert(!sc64_zc_unpack(encoded,n-1,rebuilt,sizeof(rebuilt)));
    assert(!sc64_zc_unpack(encoded,n+1,rebuilt,sizeof(rebuilt)));
    assert(!sc64_zc_unpack(encoded,n,rebuilt,sizeof(rebuilt)-1));
    for (unsigned i=0;i<sizeof(rebuilt);++i) assert(rebuilt[i]==0xa5);

    GbaSaveSelection predecessor, selected;
    assert(save_select_prepared(selection,sizeof(selection),tags,2,prepare,0,&predecessor));
    assert(predecessor.tag==tags[0]);
    u8 *shot_count=encoded+SC64_ZC_HEADER+1;
    *shot_count=101;
    assert(save_write_selected(encoded,n,tags[1],&predecessor,tags,2));
    assert(save_select_prepared(selection,sizeof(selection),tags,2,prepare,0,&selected));
    assert(selected.tag==predecessor.tag && selected.sequence==predecessor.sequence);
    *shot_count=100;

    int bots=4, saved;
    u8 *bots_at=encoded+SC64_ZC_MISC+4*sizeof(int);
    memcpy(&saved,bots_at,sizeof(saved));
    memcpy(bots_at,&bots,sizeof(bots));
    assert(sc64_zc_unpack(encoded,n,rebuilt,sizeof(rebuilt)));
    assert(save_write_selected(encoded,n,tags[1],&predecessor,tags,2));
    assert(save_select_prepared(selection,sizeof(selection),tags,2,prepare,0,&selected));
    assert(selected.tag==predecessor.tag && selected.sequence==predecessor.sequence);
    memcpy(bots_at,&saved,sizeof(saved));
    assert(save_write_selected(encoded,n,tags[1],&predecessor,tags,2));
    assert(save_select_prepared(selection,sizeof(selection),tags,2,prepare,0,&selected));
    assert(selected.tag==tags[1] && impact_count==32);
    printf("map %u: %u logical bytes -> %u journal, 100 shots, 32 impacts\n",map,(unsigned)sizeof(old_wire),n);
}

int main(void)
{
    fill_packet();
    bit_layout();
    for (unsigned map=0;map<=SC64_SKIRMISH_IMPORTED_COUNT;++map) saturated(map);
    const u8 empty[]={0,0};
    unsigned played=0;
    for (int map=0;map<=SC64_SKIRMISH_IMPORTED_COUNT;++map)
        for (int human=0;human<3;++human)
            for (int bots=1;bots<=3;++bots) {
                clear_journal();
                setup(map,human,bots);
                for (unsigned i=0;i<90;++i) game_update(0,0);
                assert(screen==SCREEN_PLAY && race_save_game());
                u32 size;
                assert(save_read_latest(old_wire,sizeof(old_wire),tags[0],&size));
                unsigned n=sc64_zc_pack(old_wire,size,empty,sizeof(empty),encoded,sizeof(encoded));
                assert(n && sc64_zc_unpack(encoded,n,rebuilt,sizeof(rebuilt))==size);
                assert(!memcmp(old_wire,rebuilt,size));
                roundtrip_scratch(size,n,empty,sizeof(empty));
                ++played;
            }
    assert(played==18);
    puts("18 ordinary custom configurations kept exact predecessor bytes");
}

#endif
