/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only real actor facts and selected HUD; no guest or journal admission. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
int main(void)
{
    int type=SC64_SHARED_EGG_TYPE;
    assert(!strcmp(sc64_race_unit_name(type),"EGG"));
    assert(sc64_race_raw_unit(type)==36);
    assert(sc64_race_unit_rule(type)->hp==200 && sc64_race_unit_armor(type)==10);
    assert(!sc64_race_unit_flying(type));
    assert(sc64_game_unit_actor_flags(type)==(SC64_GAME_ACTOR_SELECTABLE|
        SC64_GAME_ACTOR_COLLIDABLE|SC64_GAME_ACTOR_DRAWABLE|SC64_GAME_ACTOR_MEN));
    assert(sc64_game_unit_collision_radius(type)==8);
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();
    assert(screen==SCREEN_PLAY);
    int slot=spawn_unit(SIDE,type,50,50);assert(slot>=0);
    u[slot].raw_type=36;u[slot].target=U_DRONE;u[slot].work_timer=1;
    for(int i=0;i<MAX_UNITS;i++)u[i].selected=(i==slot);
    selected_building=-1;menu_open=0;mode=MODE_NORMAL;box_state=0;
    assert(selected_count()==1 && !strcmp(unit_name(type),"EGG"));
    game_render();game_render();
    /* Compare the actual full-world, bucket, and cached predicates at a
     * distance that the source Egg's radius 8 blocks but radius 6 misses. */
    for(int i=0;i<MAX_UNITS;i++)if(i!=slot)u[i].alive=0;
    u[slot].x=507;u[slot].y=500;
    int mover=spawn_unit(SIDE,U_DRONE,500,500);assert(mover>=0 && mover!=slot);
    u[mover].order=ORDER_IDLE;
    sc64_unit_buckets_begin();
    assert(!unit_step_clear_race_fallback(&u[mover],501,500));
    assert(!sc64_unit_bucket_step_clear_reference(&u[mover],501,500,0));
#ifdef SC64_CONTACT_CACHE
    assert(!sc64_contact_step_clear(&u[mover],501,500,0));
#endif
    sc64_unit_buckets_end();
    puts("Egg staged facts, selected HUD and live collision predicates passed");return 0;
}
