/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

_Static_assert(U_P7_CARRIER == 45 && U_P7_INTERCEPTOR == 49,
               "keep the Z10 Carrier identity and append only raw73");

static u8 wire[RACE_IMPACT_RECORD_MAX], ordinary[RACE_IMPACT_RECORD_MAX];
static const u32 tags[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,
                        SC64_TRANSPORT_SAVE_VERSION,SC64_ZERG_CARRIER_TAG};

static void clear_journal(void)
{
    for (unsigned i=0;i<2;++i)
        for (unsigned tag=0;tag<sizeof(tags)/sizeof(tags[0]);++tag)
            save_test_corrupt_latest(0,tags[tag]);
}

static int find_actor(int type)
{
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==type) return i;
    return -1;
}

int main(void)
{
    game_init();
    race_mode=1; race_human=R_PROTOSS; race_bots=1; race_skirmish_map=0;
    race_bot_races[0]=R_TER;
    init_state();
    assert(screen==SCREEN_PLAY);
    assert(sc64_p8_unit_sprite_for_raw(72) && sc64_p8_unit_sprite_for_raw(73));
    assert(sc64_unit_facing_frames(72) && sc64_unit_facing_frames(73));
    /* Establish an ordinary predecessor from exactly the reconstructed game
     * bytes. Assert that the real probe writer migrates it to a separate bank. */
    unsigned wire_size;
    assert(race_save_game());
    assert(save_read_latest(wire,sizeof(wire),SC64_ZERG_CARRIER_TAG,&wire_size));
    unsigned ordinary_size=sc64_zc_unpack(wire,wire_size,ordinary,sizeof(ordinary));
    assert(ordinary_size);
    clear_journal();
    assert(save_write_first(ordinary,ordinary_size,SC64_TRANSPORT_SAVE_VERSION));
    GbaSaveSelection prior, compact, selected;
    RaceLoadPlan plan;
    assert(race_worker_cargo_select(&plan,&prior) && prior.tag==SC64_TRANSPORT_SAVE_VERSION);
    int pylon=spawn_build(SIDE,B_PYLON,200,200,0);
    int gate=spawn_build(SIDE,B_STARGATE,248,200,0);
    int beacon=spawn_build(SIDE,sc64_race_building_from_raw(169),200,248,0);
    assert(pylon>=0 && gate>=0 && beacon>=0);
    minerals=1000;gas=1000;supply_cap=100;
    assert(race_queue_unit(SIDE,gate,U_P7_CARRIER) && minerals==650 && gas==750);
    for (unsigned n=0;n<2625;++n) race_tick_buildings();
    int carrier=find_actor(U_P7_CARRIER);
    assert(carrier>=0 && RACE_PROTOSS_RAW(&u[carrier])==72);
    u[carrier].selected=1;
    assert(race_carrier_command_base()<field_menu_count());
    choose_command(race_carrier_command_base());
    assert(u[carrier].carry==1 && minerals==625);
    race_p7_carrier_training_source_tick();
    for (unsigned n=0;n<300;++n) race_p7_carrier_training_source_tick();
    int child=find_actor(U_P7_INTERCEPTOR);
    assert(child>=0 && sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_INSIDE);
    int target=spawn_unit(ENEMY,U_MARINE,u[carrier].x+100,u[carrier].y);
    assert(target>=0);
    sc64_visibility_reset();update_exploration();
    u[carrier].order=ORDER_ATTACK_TARGET;u[carrier].target=target;
    assert(race_p7_interceptor_flight_carrier_holds(carrier));
    for (unsigned n=0;n<180 && sc64_pool_active_head(&race_p7_projectiles)==SC64_END;++n) {
        race_p7_interceptor_flight_source_tick();
        race_p7_interceptor_flight_frame_move();
        race_p7_carrier_training_end_frame();
        ++frame;
    }
    assert(sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_OUTSIDE);
    assert(sc64_pool_active_head(&race_p7_projectiles)!=SC64_END);
    static u16 shot_bank[SC64_ARENA_WORDS];
    memcpy(shot_bank,race_p7_projectile_host_words,sizeof(shot_bank));
    int hp=u[target].hp;
    for (unsigned n=0;n<RACE_IMPACT_ROWS;++n)
        race_impact_launch(target,1+(int)n,1,ENEMY,0,U_MARINE,20u+n);
    assert(race_impact_count==RACE_IMPACT_ROWS);
    Sc64ImpactEvent pending[RACE_IMPACT_ROWS];
    memcpy(pending,race_impact_rows,sizeof(pending));
    assert(race_save_game());
    assert(race_worker_cargo_select(&plan,&compact));
    assert(compact.tag==SC64_ZERG_CARRIER_TAG && compact.slot!=prior.slot);
    assert(save_read_latest(wire,sizeof(wire),SC64_ZERG_CARRIER_TAG,&wire_size));
    u[target].hp=1;
    race_p7_projectile_reset();
    assert(race_load_game());
    assert(u[target].hp==hp && u[child].alive && u[child].type==U_P7_INTERCEPTOR);
    assert(!memcmp(shot_bank,race_p7_projectile_host_words,sizeof(shot_bank)));
    assert(race_impact_count==RACE_IMPACT_ROWS && !memcmp(race_impact_rows,pending,sizeof(pending)));
    for (unsigned n=0;n<180 && u[target].alive && u[target].hp==hp;++n)
        race_p7_projectile_source_tick();
    assert(!u[target].alive || u[target].hp<hp);
    assert(!race_p7_projectile_fault);
    /* Preserve the real packet shape but name an unoccupied shooter slot.
     * Structural validation still passes; the candidate-world check must fail. */
    memcpy(race_p7_projectile_host_words,shot_bank,sizeof(shot_bank));
    u8 shot=sc64_pool_active_head(&race_p7_projectiles);
    Sc64ProjectileRow row;
    assert(sc64_pool_get_row(&race_p7_projectiles,shot,&row));
    assert(!u[MAX_UNITS-1].alive);
    row.owner_ref=MAX_UNITS-1;
    assert(sc64_pool_set_row(&race_p7_projectiles,shot,&row));
    unsigned packet_size=race_p7_projectile_encode(wire+SC64_ZC_HEADER);
    assert(packet_size==sc64_zc_get16(wire+SC64_ZC_COUNTS));
    assert(sc64_compact_codec_validate(wire+SC64_ZC_HEADER,packet_size,SC64_POOL_CAPACITY,0,0));
    assert(save_write_selected(wire,wire_size,SC64_ZERG_CARRIER_TAG,&prior,tags,3));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==prior.tag && selected.slot==prior.slot);
    /* A new, genuinely checksummed bank with an impossible active count
     * must be discarded before the ordinary bank is published. */
    wire[SC64_ZC_HEADER+1]=101;
    assert(save_write_selected(wire,wire_size,SC64_ZERG_CARRIER_TAG,&prior,tags,3));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==prior.tag && selected.slot==prior.slot);
    assert(race_load_game());
    assert(find_actor(U_P7_CARRIER)<0 && find_actor(U_P7_INTERCEPTOR)<0);
    assert(sc64_pool_active_head(&race_p7_projectiles)==SC64_END);

    clear_journal();
    game_init();
    const int campaign=sc64_frontend_first_mission(R_ZERG);
    race_mode=0; race_human=R_ZERG; race_selected_mission=campaign;
    init_state();
    assert(screen==SCREEN_PLAY);
    assert(race_save_game());
    assert(!save_read_latest(wire,sizeof(wire),SC64_ZERG_CARRIER_TAG,&wire_size));
    assert(save_read_latest(wire,sizeof(wire),SC64_TRANSPORT_SAVE_VERSION,&wire_size));
    assert(race_load_game());
    assert(!race_mode && race_selected_mission==campaign);
    puts("Z10 custom paid Carrier and Interceptor, compact live shot save/load and damage passed");
}
