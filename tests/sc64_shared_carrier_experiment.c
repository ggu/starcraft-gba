/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_CARRIER_EXPERIMENT
#error "This fixture exercises the gated shared carrier backend"
#endif
static int children(int carrier, unsigned state)
{
    int count=0;
    for (int i=0;i<MAX_UNITS;++i)
        count += u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE &&
                 u[i].build==carrier && sc64_p7_carrier_child_state(&u[i])==state;
    return count;
}
static void test_typed_rows(void)
{
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        Sc64P7TypedUnitSave row;
        Unit decoded;
        assert(sc64_p7_typed_unit_encode(&row,&u[i],race_cols*16,race_rows*16));
        assert(sc64_p7_typed_unit_decode(&decoded,&row,race_cols*16,race_rows*16));
        assert(!memcmp(&decoded,&u[i],sizeof(decoded)));
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) {
        Sc64P7TypedBuildingSave row;
        Building decoded;
        assert(sc64_p7_typed_building_encode(&row,&b[i],race_cols*16,race_rows*16));
        assert(sc64_p7_typed_building_decode(&decoded,&row,race_cols*16,race_rows*16));
        assert(!memcmp(&decoded,&b[i],sizeof(decoded)));
    }
}
static void test_catalog_rows(void)
{
    for (unsigned n=0;n<sc64_skirmish_record_count();++n) {
        const Sc64SkirmishRecord *record=sc64_skirmish_record_at(n);
        int unit=n<SC64_SKIRMISH_UNIT_COUNT;
        if (unit && record->kind!=SC64_SK_TRAIN &&
            record->kind!=SC64_SK_UNIT_MORPH && record->kind!=SC64_SK_MODE) continue;
        int type=unit ? sc64_race_unit_from_raw(record->raw_id) :
                        sc64_race_building_from_raw(record->raw_id);
        if (type<0) continue;
        for (int owner=0;owner<5;++owner) {
            if (unit) {
                int slot=spawn_unit(owner,type,400,400);
                assert(slot>=0);
                Unit *q=&u[slot], decoded;
                Sc64P7TypedUnitSave row;
                q->selected=1; q->hp=(q->maxhp+1)/2; q->shield/=2;
                if (!sc64_p7_typed_unit_encode(&row,q,race_cols*16,race_rows*16)) {
                    fprintf(stderr,"typed unit rejected raw=%u owner=%d\n",record->raw_id,owner);
                    assert(0);
                }
                assert(sc64_p7_typed_unit_decode(&decoded,&row,race_cols*16,race_rows*16));
                assert(!memcmp(q,&decoded,sizeof(decoded)));
                q->alive=0;
            } else {
                int slot=spawn_build(owner,type,400,400,0);
                assert(slot>=0);
                Building *q=&b[slot], decoded;
                Sc64P7TypedBuildingSave row;
                q->hp=(q->maxhp+1)/2; q->shield/=2;
                if (!sc64_p7_typed_building_encode(&row,q,race_cols*16,race_rows*16)) {
                    fprintf(stderr,"typed building rejected raw=%u owner=%d\n",record->raw_id,owner);
                    assert(0);
                }
                assert(sc64_p7_typed_building_decode(&decoded,&row,race_cols*16,race_rows*16));
                assert(!memcmp(q,&decoded,sizeof(decoded)));
                q->alive=0;
            }
        }
    }
}
int main(void)
{
    game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1; race_skirmish_map=0;
    race_skirmish_init(); screen=SCREEN_PLAY;
    test_typed_rows();
    test_catalog_rows();
    int carrier=spawn_unit(SIDE,SC64_CARRIER_TYPE,600,600);
    assert(carrier>=0); minerals=25;
    assert(race_p7_carrier_training_queue_owner(carrier,SIDE));
#ifndef SC64_P7_PROJECTILES_ENABLED
    assert(!race_save_game());
#endif
    for (int i=0;i<430 && screen==SCREEN_PLAY;++i) game_update(0,0);
    assert(children(carrier,SC64_P7_INTERCEPTOR_INSIDE)==1);
    int prey=spawn_unit(ENEMY,U_ZERGLING,660,600); assert(prey>=0);
    int hp=u[prey].hp, launched=0;
    for (int i=0;i<400 && screen==SCREEN_PLAY && u[prey].alive;++i) {
        game_update(0,0);
        if (children(carrier,SC64_P7_INTERCEPTOR_OUTSIDE)) launched=1;
    }
    assert(launched && (!u[prey].alive || u[prey].hp<hp));
    int doomed=spawn_unit(SIDE,SC64_CARRIER_TYPE,700,700);
    int victim=spawn_unit(ENEMY,U_ZERGLING,790,700);
    assert(doomed>=0 && victim>=0 && race_p7_carrier_training_stock(doomed,1)==1);
    for (int i=0;i<80 && !children(doomed,SC64_P7_INTERCEPTOR_OUTSIDE);++i)
        game_update(0,0);
    int child=-1;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE && u[i].build==doomed &&
            sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE) child=i;
    assert(child>=0);
    test_typed_rows();
    hit_target(doomed,10000,U_MARINE,ENEMY);
    assert(!u[doomed].alive && u[child].alive && u[child].build==-1 &&
           sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_DETACHED);
    /* Each custom-game opponent uses its own wallet and actor binding. */
    for (int owner=1; owner<5; ++owner) {
        if (owner==SIDE) continue;
        int bot=spawn_unit(owner,SC64_CARRIER_TYPE,500+owner*16,500);
        assert(bot>=0); race_minerals[owner]=25;
        assert(!race_p7_carrier_training_queue_owner(bot,SIDE));
        race_ai_plan(owner);
        assert(u[bot].carry==1);
        assert(race_minerals[owner]==0);
        assert(!race_p7_carrier_training_queue_owner(bot,owner));
        race_p7_carrier_training_source_tick();
        assert(children(bot,SC64_P7_INTERCEPTOR_TRAINING)==1);
    }
    puts("shared carrier experiment: ok"); return 0;
}
