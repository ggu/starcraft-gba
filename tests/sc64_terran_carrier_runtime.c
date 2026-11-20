/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

_Static_assert(U_P7_CARRIER == 44 && U_P7_INTERCEPTOR == 45,
               "append after the standard Terran roster");

static u8 wire[RACE_IMPACT_RECORD_MAX], ordinary[RACE_IMPACT_RECORD_MAX];
static const u32 tags[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,
                        SC64_TRANSPORT_SAVE_VERSION,SC64_TERRAN_CARRIER_TAG};

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
    assert(save_read_latest(wire,sizeof(wire),SC64_TERRAN_CARRIER_TAG,&wire_size));
    unsigned ordinary_size=sc64_tc_unpack_packet(wire,wire_size,ordinary,sizeof(ordinary),0,0);
    assert(ordinary_size);
    clear_journal();
    assert(save_write_first(ordinary,ordinary_size,SC64_TRANSPORT_SAVE_VERSION));
    GbaSaveSelection prior, compact, selected;
    RaceLoadPlan plan;
    assert(race_worker_cargo_select(&plan,&prior) && prior.tag==SC64_TRANSPORT_SAVE_VERSION);
    int pylon=spawn_build(SIDE,B_PYLON,200,200,0);
    int gate=spawn_build(SIDE,B_STARGATE,248,200,0);
    int beacon=spawn_build(SIDE,B_FLEET_BEACON,200,248,0);
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
    race_protoss_generic_clock[0]=4;
    race_p7_carrier_training_frame_tick();
    assert(u[carrier].carry==1 && find_actor(U_P7_INTERCEPTOR)<0);
    race_protoss_generic_clock[0]=0;
    race_p7_carrier_training_frame_tick();
    for (unsigned n=0;n<300;++n) race_p7_carrier_training_frame_tick();
    int child=find_actor(U_P7_INTERCEPTOR);
    assert(child>=0 && sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_INSIDE);
    int target=spawn_unit(ENEMY,U_MARINE,u[carrier].x+100,u[carrier].y);
    assert(target>=0);
    sc64_visibility_reset();update_exploration();
    u[carrier].order=ORDER_ATTACK_TARGET;u[carrier].target=target;
    assert(race_p7_interceptor_flight_carrier_holds(carrier));
    for (unsigned n=0;n<180 && sc64_pool_active_head(&race_p7_projectiles)==SC64_END;++n) {
        race_protoss_generic_clock[0]=(u8)((unsigned)frame%5u);
        race_p7_carrier_training_frame_tick();
        race_p7_carrier_training_end_frame();
        ++frame;
    }
    assert(sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_OUTSIDE);
    assert(sc64_pool_active_head(&race_p7_projectiles)!=SC64_END);
    static u16 shot_bank[SC64_ARENA_WORDS];
    memcpy(shot_bank,race_p7_projectile_host_words,sizeof(shot_bank));
    int hp=u[target].hp;
    assert(race_save_game());
    assert(race_worker_cargo_select(&plan,&compact));
    assert(compact.tag==SC64_TERRAN_CARRIER_TAG && compact.slot!=prior.slot);
    assert(save_read_latest(wire,sizeof(wire),SC64_TERRAN_CARRIER_TAG,&wire_size));
    u[target].hp=1;
    race_p7_projectile_reset();
    assert(race_load_game());
    assert(u[target].hp==hp && u[child].alive && u[child].type==U_P7_INTERCEPTOR);
    assert(!memcmp(shot_bank,race_p7_projectile_host_words,sizeof(shot_bank)));
    for (unsigned n=0;n<180 && u[target].alive && u[target].hp==hp;++n)
        race_p7_projectile_source_tick();
    assert(!u[target].alive || u[target].hp<hp);
    assert(!race_p7_projectile_fault);

    /* A last detached Interceptor still runs without a live Carrier. */
    hit_target(carrier,100000,U_MARINE,ENEMY);
    assert(!u[carrier].alive && u[child].alive);
    assert(sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_DETACHED);
    int lifetime=u[child].carry;
    race_protoss_generic_clock[0]=4;
    race_p7_carrier_training_frame_tick();
    assert(u[child].carry==lifetime);
    race_protoss_generic_clock[0]=0;
    race_p7_carrier_training_frame_tick();
    assert(u[child].carry==lifetime-1);

    /* Preserve the real packet shape but name an unoccupied shooter slot.
     * Structural validation still passes; the candidate-world check must fail. */
    memcpy(race_p7_projectile_host_words,shot_bank,sizeof(shot_bank));
    u8 shot=sc64_pool_active_head(&race_p7_projectiles);
    Sc64ProjectileRow row;
    assert(sc64_pool_get_row(&race_p7_projectiles,shot,&row));
    assert(!u[MAX_UNITS-1].alive);
    row.owner_ref=MAX_UNITS-1;
    assert(sc64_pool_set_row(&race_p7_projectiles,shot,&row));
    unsigned packet_size=race_p7_projectile_encode(wire+SC64_TC_HEADER);
    assert(packet_size==sc64_tc_get16(wire+SC64_TC_COUNTS));
    assert(sc64_compact_codec_validate(wire+SC64_TC_HEADER,packet_size,SC64_POOL_CAPACITY,0,0));
    assert(save_write_selected(wire,wire_size,SC64_TERRAN_CARRIER_TAG,&prior,tags,3));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==prior.tag && selected.slot==prior.slot);
    /* A new, genuinely checksummed bank with an impossible active count
     * must be discarded before the ordinary bank is published. */
    wire[SC64_TC_HEADER+1]=101;
    assert(save_write_selected(wire,wire_size,SC64_TERRAN_CARRIER_TAG,&prior,tags,3));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==prior.tag && selected.slot==prior.slot);
    assert(race_load_game());
    assert(find_actor(U_P7_CARRIER)<0 && find_actor(U_P7_INTERCEPTOR)<0);
    assert(sc64_pool_active_head(&race_p7_projectiles)==SC64_END);

    clear_journal();
    game_init();
    const int campaign=sc64_frontend_first_mission(R_TER);
    race_mode=0; race_human=R_TER; race_selected_mission=campaign;
    init_state();
    assert(screen==SCREEN_PLAY);
    assert(race_save_game());
    assert(!save_read_latest(wire,sizeof(wire),SC64_TERRAN_CARRIER_TAG,&wire_size));
    assert(save_read_latest(wire,sizeof(wire),SC64_TRANSPORT_SAVE_VERSION,&wire_size));
    assert(race_load_game());
    assert(!race_mode && race_selected_mission==campaign);
    puts("Terran custom paid Carrier and Interceptor, compact live shot save/load and damage passed");
}
