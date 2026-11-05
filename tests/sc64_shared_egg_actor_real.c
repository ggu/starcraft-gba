/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only real actor facts and selected HUD; no guest or journal admission. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define SC64_EGG_COLLISION_TRACE 1
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
    int x=WORLD_W/2,y=WORLD_H/2;assert(x>8 && x+8<WORLD_W && y>8 && y+8<WORLD_H);
    u[slot].x=x+7;u[slot].y=y;
    int mover=spawn_unit(SIDE,U_DRONE,x,y);assert(mover>=0 && mover!=slot);
    u[mover].order=ORDER_IDLE;
    sc64_unit_buckets_begin();assert(sc64_unit_bucket_active);
    printf("collision world=%dx%d pos=%d,%d egg=%d mover=%d cells=%u,%u heads=%u,%u next=%u,%u flags=%u,%u radius=%d,%d flying=%d,%d loaded=%d,%d fallback=%d bucket=%d\n",
           WORLD_W,WORLD_H,x,y,slot,mover,(unsigned)sc64_unit_bucket_cell[slot],
           (unsigned)sc64_unit_bucket_cell[mover],
           (unsigned)sc64_unit_bucket_head[sc64_unit_bucket_cell[slot]],
           (unsigned)sc64_unit_bucket_head[sc64_unit_bucket_cell[mover]],
           (unsigned)sc64_unit_bucket_next[slot],(unsigned)sc64_unit_bucket_next[mover],
           sc64_game_unit_actor_flags(u[slot].type),sc64_game_unit_actor_flags(u[mover].type),
           sc64_game_unit_collision_radius(u[slot].type),sc64_game_unit_collision_radius(u[mover].type),
           sc64_race_unit_flying(u[slot].type),sc64_race_unit_flying(u[mover].type),
           !!SC64_TRANSPORT_LOADED(&u[slot]),!!SC64_TRANSPORT_LOADED(&u[mover]),
           unit_step_clear_race_fallback(&u[mover],x+1,y),
           sc64_unit_bucket_step_clear_reference(&u[mover],x+1,y,0));fflush(stdout);
    printf("worker=%d order=%d idle=%d mine=%d gas=%d scarab=%d strict=%d contact=%d\n",
           !!IS_WORKER(u[mover].type),u[mover].order,ORDER_IDLE,ORDER_MINE,ORDER_GAS,
           !!SC64_SCARAB_NONCOLLIDING(&u[slot]),
           sc64_unit_bucket_step_clear_reference(&u[mover],x+1,y,1),
           sc64_contact_step_clear(&u[mover],x+1,y,0));fflush(stdout);
    assert(!unit_step_clear_race_fallback(&u[mover],x+1,y));
    assert(!sc64_unit_bucket_step_clear_reference(&u[mover],x+1,y,0));
#ifdef SC64_CONTACT_CACHE
    assert(!sc64_contact_step_clear(&u[mover],x+1,y,0));
#endif
    sc64_unit_buckets_end();
    puts("Egg staged facts, selected HUD and live collision predicates passed");return 0;
}
