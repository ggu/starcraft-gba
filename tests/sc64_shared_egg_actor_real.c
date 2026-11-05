/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only real actor facts and selected HUD; no guest or journal admission. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
/* Test low-level geometry for both actor orderings. An Egg itself remains
 * immobile in gameplay; reversing this query checks symmetric bounds only. */
static void collision_case(Unit *moving,Unit *other,int dx,int dy,int radius)
{
    int cx=WORLD_W/2,cy=WORLD_H/2;
    int nx=cx+(dx>0?-1:0),ny=cy+(dy>0?-1:0);
    int sx=(dx>0)-(dx<0),sy=(dy>0)-(dy<0);
    moving->x=nx-sx;moving->y=ny-sy;moving->order=ORDER_IDLE;
    other->x=nx+dx;other->y=ny+dy;
    assert(moving->x>=0 && other->x>=0 && moving->x<WORLD_W && other->x<WORLD_W);
    assert(moving->y>=0 && other->y>=0 && moving->y<WORLD_H && other->y<WORLD_H);
    int expected=(dx*dx+dy*dy>=radius*radius);
    sc64_unit_buckets_begin();assert(sc64_unit_bucket_active);
    assert(unit_step_clear_race_fallback(moving,nx,ny)==expected);
    assert(sc64_unit_bucket_step_clear_reference(moving,nx,ny,0)==expected);
    assert(sc64_unit_bucket_step_clear_reference(moving,nx,ny,1)==expected);
#ifdef SC64_CONTACT_CACHE
    assert(sc64_contact_step_clear(moving,nx,ny,0)==expected);
    assert(sc64_contact_step_clear(moving,nx,ny,1)==expected);
#endif
    sc64_unit_buckets_end();
}
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
    /* Place both actors on opposite sides of cell boundaries, on each axis
     * and diagonal; test the strict radius boundary in both actor orders. */
    for(int i=0;i<MAX_UNITS;i++)if(i!=slot)u[i].alive=0;
    int mover=spawn_unit(SIDE,U_DRONE,WORLD_W/2,WORLD_H/2);assert(mover>=0 && mover!=slot);
    static const int offsets[][2]={{7,0},{8,0},{-7,0},{-8,0},
        {0,7},{0,8},{0,-7},{0,-8},{5,5},{6,6},{-5,-5},{-6,-6},
        {5,-5},{6,-6},{-5,5},{-6,6}};
    for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++) {
        collision_case(&u[mover],&u[slot],offsets[i][0],offsets[i][1],8);
        collision_case(&u[slot],&u[mover],offsets[i][0],offsets[i][1],8);
    }
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    /* Ordinary Terran-profile actors retain the pre-stage radius 6. */
    u[slot].type=U_DRONE;
    collision_case(&u[mover],&u[slot],5,0,6);
    collision_case(&u[mover],&u[slot],6,0,6);
    collision_case(&u[mover],&u[slot],-5,0,6);
    collision_case(&u[mover],&u[slot],-6,0,6);
#endif
    puts("Egg staged facts, selected HUD and live collision predicates passed");return 0;
}
