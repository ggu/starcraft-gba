/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual-engine P7 generation-40 typed save codec fixture. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void test_p7_typed_unit_codec(void)
{
    Unit q, decoded, untouched;
    int canonical_armor;
    Sc64P7TypedUnitSave row, before;
    memset(&q, 0, sizeof(q));
    assert(sc64_p7_typed_unit_encode(&row, &q, 2048, 2048));
    assert(sc64_p7_typed_unit_decode(&decoded, &row, 2048, 2048));
    assert(memcmp(&decoded, &q, sizeof(q)) == 0);

    /* Exercise each proposed packed boundary in a valid complete Unit. */
    q.x=2047; q.y=2046; q.hp=1; q.owner=4; q.type=U_P6_ZERATUL;
    assert(sc64_p7_typed_unit_canonical(q.type,&q.maxhp,&canonical_armor,0));
    q.armor=(u8)canonical_armor;
    q.alive=1; q.selected=1; q.cd=94; q.order=ORDER_IDLE;
    q.tx=2047; q.ty=2046; q.carry=64000; q.build=239;
    q.work_timer=1688; q.weapon=0xffff; q.target=MAX_UNITS+MAX_BUILDINGS-1;
    q.path_timer=24; q.path_retry=12; q.path_x=2047; q.path_y=2046;
    q.shield=400; q.raw_type=254; q.raw_owner=14; q.flags=0x3fff;
    q.shield_remainder=1279;
    assert(sc64_p7_typed_unit_encode(&row,&q,2048,2048));
    assert(sc64_p7_typed_unit_decode(&decoded,&row,2048,2048));
    assert(memcmp(&decoded,&q,sizeof(q))==0);

    before=row;
#define REJECT_FIELD(field, value) do { Unit bad=q; bad.field=(value); \
    assert(!sc64_p7_typed_unit_encode(&row,&bad,2048,2048)); \
    assert(memcmp(&row,&before,sizeof(row))==0); } while (0)
    REJECT_FIELD(cd,256); REJECT_FIELD(work_timer,2048);
    REJECT_FIELD(build,240); REJECT_FIELD(target,400);
    REJECT_FIELD(shield,401); REJECT_FIELD(raw_type,255);
    REJECT_FIELD(raw_owner,15); REJECT_FIELD(flags,0x4000);
#undef REJECT_FIELD
    memset(&untouched,0x5a,sizeof(untouched)); decoded=untouched;
    Sc64P7TypedUnitSave badrow=before;
    badrow.bytes[27] &= 0x7fu; /* the occupied marker is the top bit */
    assert(!sc64_p7_typed_unit_decode(&decoded,&badrow,2048,2048));
    assert(memcmp(&decoded,&untouched,sizeof(decoded))==0);

    /* Exercise every canonical unit type present in the combined P1–P7
       catalog, including campaign actors outside the P7 starting roster. */
    unsigned types=0;
    for (int type=0; type<SC_RU_COUNT; ++type) {
        Unit actor; int armor,maxshield;
        memset(&actor,0,sizeof(actor));
        if (!sc64_p7_typed_unit_canonical(type,&actor.maxhp,&armor,&maxshield))
            continue;
        actor.type=type; actor.armor=(u8)armor; actor.hp=actor.maxhp;
        actor.alive=1; actor.owner=SIDE; actor.build=-1; actor.target=-1;
        actor.raw_type=-1; actor.raw_owner=-1;
        assert(sc64_p7_typed_unit_encode(&row,&actor,2048,2048));
        assert(sc64_p7_typed_unit_decode(&decoded,&row,2048,2048));
        assert(memcmp(&decoded,&actor,sizeof(actor))==0);
        ++types;
    }
    assert(types>SC_RU_COUNT/2u);
}

static void test_p7_typed_building_codec(void)
{
    Building q,decoded,untouched;
    Sc64P7TypedBuildingSave row,before;
    memset(&q,0,sizeof(q));
    assert(sc64_p7_typed_building_encode(&row,&q,2048,2048));
    assert(sc64_p7_typed_building_decode(&decoded,&row,2048,2048));
    assert(memcmp(&decoded,&q,sizeof(q))==0);
    q.x=2047; q.y=2046; q.hp=1; q.owner=4; q.type=B_PYLON;
    q.alive=1; q.maxhp=sc64_p7_typed_building_maxhp(q.type);
    q.queue[0]=U_PROBE; q.queue[1]=U_ZERGLING;
    q.queue_head=4; q.queue_count=5; q.queue_progress=65535;
    q.build_progress=65534; q.under_construction=1; q.level=127;
    q.shield=65535; q.raw_type=254; q.raw_owner=14; q.flags=0xffff;
    q.larva=3; q.larva_timer=511; q.shield_remainder=1279;
    assert(sc64_p7_typed_building_encode(&row,&q,2048,2048));
    assert(sc64_p7_typed_building_decode(&decoded,&row,2048,2048));
    assert(memcmp(&decoded,&q,sizeof(q))==0);
    before=row;
#define REJECT_BUILDING(field,value) do { Building bad=q; bad.field=(value); \
    assert(!sc64_p7_typed_building_encode(&row,&bad,2048,2048)); \
    assert(memcmp(&row,&before,sizeof(row))==0); } while(0)
    REJECT_BUILDING(raw_type,255); REJECT_BUILDING(raw_owner,15);
    REJECT_BUILDING(larva_timer,512);
#undef REJECT_BUILDING
    memset(&untouched,0x5a,sizeof(untouched)); decoded=untouched;
    Sc64P7TypedBuildingSave badrow=before;
    badrow.bytes[24] |= 0x40; /* nonzero canonical padding */
    assert(!sc64_p7_typed_building_decode(&decoded,&badrow,2048,2048));
    assert(memcmp(&decoded,&untouched,sizeof(decoded))==0);
}

static void test_skirmish_profile_save_load_matrix(void)
{
    unsigned cases = 0;
    /* The supported matrix is two map choices (synthetic and imported),
       three human races, and one through three rotated bot slots. */
    for (int map=0; map<2; ++map)
        for (int human=R_TER; human<=R_PROTOSS; ++human)
            for (int bots=1; bots<=3; ++bots) {
                race_mode=1; race_skirmish_map=map;
                race_human=human; race_bots=bots;
                race_skirmish_init();
                assert(screen==SCREEN_PLAY);
                assert(race_save_game());
                RaceSave saved=race_data;
                assert(race_load_game());
                assert(race_mode==1 && race_skirmish_map==map &&
                       race_human==human && race_bots==bots);
                assert(race_save_game());
                assert(memcmp(&saved,&race_data,sizeof(saved))==0);
                ++cases;
            }
    assert(cases==18u);
}

static void test_protoss_campaign_map_save_loads(void)
{
    unsigned cases=0;
    static const int missions[]={2,3,SC64_P3_GLOBAL_ID,SC64_P4_GLOBAL_ID,
                                 SC64_P5_GLOBAL_ID,SC64_P6_GLOBAL_ID,
                                 SC64_P7_GLOBAL_ID};
    /* Protoss P1/P2 retain the historical global IDs 2/3; P3–P7 use the
       later contiguous campaign IDs. */
    for (unsigned i=0; i<sizeof(missions)/sizeof(missions[0]); ++i) {
        int mission=missions[i];
        race_mode=0; race_selected_mission=mission;
        race_campaign_init();
        assert(screen==SCREEN_PLAY && mission_unsupported_count==0);
        assert(race_selected_mission==mission && race_save_game());
        RaceSave saved=race_data;
        assert(race_load_game() && race_save_game());
        assert(memcmp(&saved,&race_data,sizeof(saved))==0);
        ++cases;
    }
    assert(cases==7u);
}

int main(void)
{
    game_init();
    assert(SC64_RACE_SAVE_GENERATION==40u);
    assert(sizeof(Sc64P7TypedUnitSave)==28u);
    assert(sizeof(Sc64P7TypedBuildingSave)==25u);
    assert(sizeof(RaceSave)==14188u);
    test_p7_typed_unit_codec();
    test_p7_typed_building_codec();
    test_protoss_campaign_map_save_loads();
    test_skirmish_profile_save_load_matrix();
    printf("P7 typed unit/building codec, P1-P7 campaigns, and 18 skirmish setups passed (RaceSave=%u)\n", (unsigned)sizeof(RaceSave));
    return 0;
}
