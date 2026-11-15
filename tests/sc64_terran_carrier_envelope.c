/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_terran_carrier_envelope.h"

/* Host-study tag; no production reader or writer recognizes it. */
#define SC64_TC_FIXTURE_TAG ((SC64_TRANSPORT_SAVE_VERSION&~0x00ff0000u)|0x004d0000u)
_Static_assert(SC64_TC_FIXTURE_TAG!=SC64_TRANSPORT_SAVE_VERSION,"separate authenticated outer tag");

static u8 old_wire[RACE_IMPACT_RECORD_MAX], compressed[RACE_IMPACT_RECORD_MAX];
static u8 input[RACE_IMPACT_RECORD_MAX], reconstructed[RACE_IMPACT_RECORD_MAX];
static u8 packet[SC64_TC_PACKET_MAX];
static u16 pool_words[SC64_ARENA_WORDS];
static unsigned old_length;
static RaceLoadPlan plan;

static void setup(unsigned map)
{
    game_init();
    race_mode=1; race_human=R_TER; race_bots=1; race_skirmish_map=(int)map;
    init_state();
    assert(screen==SCREEN_PLAY);
    assert(RACE_IMPACT_ROWS==32 && RACE_IMPACT_SAVE_CAPACITY==32);
    int caster[2], target[2];
    for (int n=0;n<2;++n) {
        caster[n]=spawn_unit(ENEMY,sc64_race_unit_from_raw(12),500,500+100*n);
        target[n]=spawn_unit(SIDE,U_MARINE,640,500+100*n);
        assert(caster[n]>=0 && target[n]>=0);
        u[caster[n]].work_timer=64<<3;
        u[caster[n]].carry=200*256;
    }
    sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    for (int n=0;n<2;++n) assert(race_bc_yamato_issue(caster[n],target[n]));
    if (!sc64_battlecruiser_source_tick_due()) ++frame;
    for (int n=0;n<2;++n) assert(race_bc_yamato_tick_caster(caster[n]));
    for (unsigned i=0;i<32;++i)
        race_impact_launch(target[i&1u],1+(int)i,1,ENEMY,0,U_MARINE,20u+i);
    assert(race_impact_count==32);
    /* These retained fields are intentionally nonzero. */
    race_campaign.elapsed_frames=42;
    backwater.executed=2;
    invincible_units[MAX_UNITS-1]=1;
    invincible_buildings[MAX_BUILDINGS-1]=1;
}

static int prepare(const void *raw,u32 len,u32 tag,void *unused)
{
    (void)unused;
    unsigned n=len;
    if (tag==SC64_TC_FIXTURE_TAG) {
        n=sc64_tc_unpack(raw,len,reconstructed,sizeof(reconstructed));
        if (!n) return 0;
        raw=reconstructed;
    } else if (tag!=SC64_TRANSPORT_SAVE_VERSION || len>sizeof(race_impact_record)) {
        return 0;
    }
    if (n>sizeof(race_impact_record)) return 0;
    memcpy(&race_impact_record,raw,n);
    return race_worker_cargo_prepare_selected(&race_impact_record,n,SC64_TRANSPORT_SAVE_VERSION,&plan);
}

static void clear_journal(void)
{
    for (int i=0;i<2;++i) {
        save_test_corrupt_latest(0,SC64_TRANSPORT_SAVE_VERSION);
        save_test_corrupt_latest(0,SC64_TC_FIXTURE_TAG);
    }
}

int main(void)
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
    for (unsigned map=0;map<=SC64_SKIRMISH_IMPORTED_COUNT;++map) {
        setup(map);
        /* Run one independent journal pair per supported custom geometry. */
        clear_journal();
        assert(race_save_game());
        assert(save_read_latest(old_wire,sizeof(old_wire),SC64_TRANSPORT_SAVE_VERSION,&old_length));
        assert(old_length==SC64_TC_BASE+SC64_TC_TAIL_MAX);
        unsigned n=sc64_tc_pack(old_wire,old_length,packet,sizeof(packet),compressed,sizeof(compressed));
        assert(n==SC64_TC_WIRE_MAX && n<=16368);
        assert(sc64_tc_unpack(compressed,n,reconstructed,sizeof(reconstructed))==old_length);
        assert(!memcmp(reconstructed,old_wire,old_length));
        assert(!sc64_tc_pack(old_wire,old_length,packet,sizeof(packet),compressed,n-1));
        old_wire[SC64_TC_OMITTED]=1;
        assert(!sc64_tc_pack(old_wire,old_length,packet,sizeof(packet),compressed,sizeof(compressed)));
        old_wire[SC64_TC_OMITTED]=0;
        old_wire[SC64_TC_PREFIX]=2;
        assert(!sc64_tc_pack(old_wire,old_length,packet,sizeof(packet),compressed,sizeof(compressed)));
        old_wire[SC64_TC_PREFIX]=reconstructed[SC64_TC_PREFIX];

        const u32 tags[]={SC64_TRANSPORT_SAVE_VERSION,SC64_TC_FIXTURE_TAG};
        GbaSaveSelection predecessor, selected;
        assert(save_select_prepared(input,sizeof(input),tags,2,prepare,0,&predecessor));
        assert(predecessor.tag==SC64_TRANSPORT_SAVE_VERSION);
        memset(reconstructed,0xa5,sizeof(reconstructed));
        assert(!sc64_tc_unpack(compressed,n-1,reconstructed,sizeof(reconstructed)));
        assert(!sc64_tc_unpack(compressed,n+1,reconstructed,sizeof(reconstructed)));
        assert(!sc64_tc_unpack(compressed,n,reconstructed,old_length-1));
        for (unsigned i=0;i<sizeof(reconstructed);++i) assert(reconstructed[i]==0xa5);

        /* Malformed compact packet with a genuine journal checksum must
         * fall back to the authenticated old bank. */
        u8 saved=compressed[SC64_TC_HEADER+1];
        compressed[SC64_TC_HEADER+1]=101;
        assert(save_write_selected(compressed,n,SC64_TC_FIXTURE_TAG,&predecessor,tags,2));
        assert(save_select_prepared(input,sizeof(input),tags,2,prepare,0,&selected));
        assert(selected.tag==predecessor.tag && selected.sequence==predecessor.sequence);
        compressed[SC64_TC_HEADER+1]=saved;
        /* Structurally valid envelope, bad candidate lobby. The ordinary
         * game's detached preflight must reject it independently of the
         * compact packet checks. */
        const unsigned map_offset=offsetof(RaceSave,state)+20*sizeof(int);
        int good_map, bad_map=SC64_SKIRMISH_IMPORTED_COUNT+1;
        memcpy(&good_map,compressed+map_offset,sizeof(good_map));
        memcpy(compressed+map_offset,&bad_map,sizeof(bad_map));
        assert(sc64_tc_unpack(compressed,n,reconstructed,sizeof(reconstructed))==old_length);
        assert(save_write_selected(compressed,n,SC64_TC_FIXTURE_TAG,&predecessor,tags,2));
        assert(save_select_prepared(input,sizeof(input),tags,2,prepare,0,&selected));
        assert(selected.tag==predecessor.tag && selected.sequence==predecessor.sequence);
        memcpy(compressed+map_offset,&good_map,sizeof(good_map));
        assert(save_write_selected(compressed,n,SC64_TC_FIXTURE_TAG,&predecessor,tags,2));
        assert(save_select_prepared(input,sizeof(input),tags,2,prepare,0,&selected));
        assert(selected.tag==SC64_TC_FIXTURE_TAG && plan.impact_candidate_count==32);
        assert(race_data.battlecruiser_shots[0].state && race_data.battlecruiser_shots[1].state);
        assert(race_data.invincible_units[MAX_UNITS-1] && race_data.invincible_buildings[MAX_BUILDINGS-1]);
        printf("map %u: exact %u -> %u bytes, 100 projectiles, two Yamatos, 32 impacts\n",map,old_length,n);
    }
    /* Ordinary custom gameplay leaves the omitted mission tail canonical
     * regardless of human race or bot count. Keep all real campaign/backwater,
     * invincibility and fog bytes in the encoded payload. */
    const u8 empty[]={0,0};
    unsigned played=0;
    for (unsigned map=0;map<=SC64_SKIRMISH_IMPORTED_COUNT;++map)
        for (int human=0;human<3;++human)
            for (int bots=1;bots<=3;++bots) {
                game_init();
                race_mode=1; race_human=human; race_bots=bots; race_skirmish_map=(int)map;
                init_state();
                assert(screen==SCREEN_PLAY);
                for (unsigned n=0;n<90;++n) game_update(0,0);
                assert(screen==SCREEN_PLAY);
                clear_journal();
                assert(race_save_game());
                assert(save_read_latest(old_wire,sizeof(old_wire),SC64_TRANSPORT_SAVE_VERSION,&old_length));
                unsigned n=sc64_tc_pack(old_wire,old_length,empty,sizeof(empty),compressed,sizeof(compressed));
                assert(n && sc64_tc_unpack(compressed,n,reconstructed,sizeof(reconstructed))==old_length);
                assert(!memcmp(old_wire,reconstructed,old_length));
                ++played;
            }
    assert(played==18);
    puts("18 ordinary custom configurations retained exact payloads after 90 updates");
    printf("prefix %u, omitted %u, invincibility %u bytes, headroom %u\n",
           SC64_TC_PREFIX,SC64_TC_BASE-SC64_TC_OMITTED,SC64_TC_INV_BYTES,16368u-SC64_TC_WIRE_MAX);
}
