/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST constructed world only; no guest SRAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
extern void guardian_test_reset(void);

int main(void)
{
    game_init(); race_mode=1; race_human=R_ZERG; race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();
    assert(screen==SCREEN_PLAY);
    for (int i=0;i<MAX_UNITS;i++) u[i].alive=0;
    int slot=spawn_unit(SIDE,U_MUTALISK,400,400);assert(slot>=0);
    Unit *q=&u[slot];q->selected=1;q->hp=60;q->flags=0;
    minerals=500;gas=500;supply_used=race_owner_supply(SIDE);
    int supply=supply_used;assert(!race_guardian_morph_start(slot));
    int spire=spawn_build(SIDE,B_GREATER_SPIRE,200,200,0);assert(spire>=0);
    minerals=49;assert(!race_guardian_morph_start(slot));minerals=500;
    gas=99;assert(!race_guardian_morph_start(slot));gas=500;
    assert(race_guardian_morph_start(slot));assert(q->type==U_GUARDIAN_COCOON);
    assert(q->selected && q->hp==100 && q->work_timer==750);
    assert(minerals==450 && gas==400 && race_owner_supply(SIDE)==supply && supply_used==supply);
    hit_target(slot,20,U_MARINE,ENEMY);assert(q->alive && q->hp==80);
    assert(race_guardian_morph_cancel(slot));assert(q->type==U_MUTALISK && q->hp==48 && q->selected);
    assert(minerals==500 && gas==500 && race_owner_supply(SIDE)==supply && supply_used==supply);
    assert(race_guardian_morph_start(slot));assert(q->hp==80);
    for(int i=0;i<749;i++) update_orders();
    assert(q->alive && q->type==U_GUARDIAN_COCOON && q->work_timer==1);
    guardian_test_reset();assert(race_save_game());q->hp=1;
    assert(race_load_game());q=&u[slot];assert(q->hp==80 && q->work_timer==1);
    update_orders();assert(q->alive && q->type==race_guardian_product_type() && q->raw_type==44);
    assert(q->hp==60 && q->maxhp==150 && q->selected && race_owner_supply(SIDE)==supply && supply_used==supply);
    int x=q->x;assert(issue_order(ORDER_MOVE,x+24,q->y)>0);
    for(int i=0;i<40;i++) update_orders();assert(q->x>x);
    int enemy=spawn_unit(ENEMY,U_MARINE,q->x+20,q->y);assert(enemy>=0);
    int hp=u[enemy].hp;tick_combat(q,enemy);assert(u[enemy].hp<hp || race_impact_count>0);
    guardian_test_reset();assert(race_save_game());assert(race_load_game());
    q=&u[slot];q->type=U_MUTALISK;q->raw_type=43;q->maxhp=120;q->hp=120;q->armor=0;q->flags=0;
    assert(race_guardian_morph_start(slot));supply_used=race_owner_supply(SIDE);
    hit_target(slot,500,U_MARINE,ENEMY);assert(!q->alive);
    assert(!race_guardian_morph_cancel(slot));int reuse=spawn_unit(SIDE,U_MUTALISK,400,400);assert(reuse==slot);
    puts("Guardian HOST lifecycle and journal continuation passed");return 0;
}
