/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_dark_swarm_cast_probe.h"
int main(void)
{
    unsigned energy=100u*256u; Sc64DarkSwarmEffect effect={0};
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;
    race_skirmish_init();
    int caster=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);
    assert(caster>=0 && sc64_race_raw_unit(u[caster].type)==46);
    assert(SC64_DEFILER_ACTOR(caster).live && SC64_DEFILER_ACTOR(caster).heading==104 &&
           SC64_DEFILER_ACTOR(caster).energy_q8==50u*256u);
    /* Until a complete schema exists, the actual save path must refuse a
     * live experimental caster instead of writing incomplete state. */
    assert(!race_save_game());
    /* Exercise ordinary pad menu navigation and point selection. */
    screen=SCREEN_PLAY; race_mode=1; race_human=R_ZERG; selected_building=-1;
    u[caster].selected=1; SC64_DEFILER_ACTOR(caster).energy_q8=100u*256u;
    game_update(KEY_B,KEY_B); assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=field_menu_count_base();++n) {
        game_update(0,0); game_update(KEY_DOWN,KEY_DOWN);
    }
    assert(menu_cursor==field_menu_count_base());
    game_update(0,0); game_update(KEY_A,KEY_A);
    assert(!menu_open && mode==MODE_DARK_SWARM_STAGE && !SC64_DEFILER_ACTOR(caster).intent);
    cx=400;cy=350;game_update(0,0);game_update(KEY_A,KEY_A);
    assert(mode==MODE_NORMAL && SC64_DEFILER_ACTOR(caster).intent &&
           SC64_DEFILER_ACTOR(caster).target_x==400 && SC64_DEFILER_ACTOR(caster).target_y==350 &&
           SC64_DEFILER_ACTOR(caster).energy_q8==100u*256u);
    /* STOP replaces an unlaunched point order without spending energy. */
    game_update(0,0); game_update(KEY_B,KEY_B); assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=MENU_STOP;++n) {
        game_update(0,0); game_update(KEY_DOWN,KEY_DOWN);
    }
    assert(menu_cursor==MENU_STOP);
    game_update(0,0); game_update(KEY_A,KEY_A);
    assert(!SC64_DEFILER_ACTOR(caster).intent && SC64_DEFILER_ACTOR(caster).energy_q8==100u*256u);
    SC64_DEFILER_ACTOR(caster).intent=1;
    assert(issue_order(ORDER_MOVE,420,350)==1);
    assert(!SC64_DEFILER_ACTOR(caster).intent && SC64_DEFILER_ACTOR(caster).energy_q8==100u*256u);
    game_update(0,0);game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=field_menu_count_base();++n) {
        game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);
    }
    assert(menu_cursor==field_menu_count_base());
    game_update(0,0);game_update(KEY_A,KEY_A);assert(mode==MODE_DARK_SWARM_STAGE);
    game_update(0,0);game_update(KEY_B,KEY_B);
    assert(mode==MODE_NORMAL && !SC64_DEFILER_ACTOR(caster).intent);
    menu_open=0;SC64_DEFILER_ACTOR(caster).energy_q8=99u*256u;
    game_update(0,0);game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=field_menu_count_base();++n) {
        game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);
    }
    game_update(0,0);game_update(KEY_A,KEY_A);
    assert(mode==MODE_NORMAL && !menu_open && !SC64_DEFILER_ACTOR(caster).intent);
    int other=spawn_unit(SIDE,U_ZERGLING,330,320);assert(other>=0);
    u[other].selected=1;assert(sc64_defiler_stage_selected()<0);
    u[other].selected=0;u[caster].owner=ENEMY;assert(sc64_defiler_stage_selected()<0);
    u[caster].owner=SIDE;
#ifdef SC64_TRANSPORT_RUNTIME
    u[caster].order=ORDER_TRANSPORTED;assert(sc64_defiler_stage_selected()<0);
    u[caster].order=ORDER_IDLE;
#endif
    u[caster].selected=0;
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    /* Probe the compact row only; real saves still need explicit semantic
     * validation, versioning and effect/order state. */
    {
        Sc64P7TypedUnitSave row; Unit restored;
        u[caster].carry=50*256;
        assert(sc64_p7_typed_unit_encode(&row,&u[caster],2048,2048));
        assert(sc64_p7_typed_unit_decode(&restored,&row,2048,2048));
        assert(restored.type==U_SHARED_DEFILER && restored.carry==50*256);
    }
#endif
    assert(!sc64_dark_swarm_probe_begin(&u[caster],1,&energy));
    assert(energy==100u*256u);
    assert(sc64_dark_swarm_probe_begin(&u[caster],0,&energy) && energy==0);
    assert(!sc64_dark_swarm_probe_begin(&u[caster],0,&energy));
    assert(!effect.remaining); /* animation start is not projectile hit */
    assert(!sc64_dark_swarm_probe_place(0,0,0,2048,2048));
    assert(energy==0); /* allocation failure cannot refund energy */
    assert(sc64_dark_swarm_probe_place(&effect,0,2047,2048,2048));
    assert(effect.x==80 && effect.y==1936 && effect.remaining==900);
    /* Feed the actual shared source-cadence predicate, not every game update. */
    int source_ticks=0,expired=0;
    race_mode=1;
    for (frame=0;frame<1125;frame++) if(sc64_battlecruiser_source_tick_due()) {
        ++source_ticks; expired+=sc64_dark_swarm_source_tick(&effect);
        if(source_ticks<900) assert(!expired && effect.remaining==900-source_ticks);
    }
    assert(source_ticks==900 && expired==1 && !effect.remaining);
    puts("Dark Swarm actual-actor cast/placement probe passed");
    return 0;
}
