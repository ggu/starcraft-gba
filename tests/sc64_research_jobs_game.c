/* SPDX-License-Identifier: GPL-3.0-or-later
 * Constructed host integration: actual Building ABI and journal level type.
 * The stage is not connected to menus, live update or save/load. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"
#ifndef SC64_EXPERIMENTAL_RESEARCH_JOBS
#error "requires host research job integration stage"
#endif

int main(void)
{
    Building q, saved;
    Sc64ProtossResearchLevels levels;
    int ore=1000, local_gas=1000;
    memset(&q,0,sizeof q); memset(&levels,0,sizeof levels);
    q.alive=1; q.owner=0; q.type=B_FORGE;
    saved=q;
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_SHIELDS,&levels,&ore,&ore,
        3,1,1,1,0,0,0,0,0));
    assert(!memcmp(&q,&saved,sizeof q) && ore==1000);
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_GROUND_WEAPONS,&levels,&ore,&local_gas,
        3,1,0,1,0,1,0,0,0)); /* unpowered */
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_GROUND_WEAPONS,&levels,&ore,&local_gas,
        3,1,1,0,0,1,0,0,0)); /* missing exact prerequisite */
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_GROUND_WEAPONS,&levels,&ore,&local_gas,
        3,1,1,1,0,1,0,1,0)); /* old generic weapon job still pending */
    assert(!memcmp(&q,&saved,sizeof q) && ore==1000 && local_gas==1000);
    assert(sc64_pr_job_begin_custom(&q,0,SC64_PR_GROUND_WEAPONS,&levels,&ore,&local_gas,
        3,1,1,1,0,1,0,0,0));
    assert(ore==850 && local_gas==850 && q.queue[1]==2 && q.queue_progress==5600);
    assert(sc64_pr_job_valid_custom(&q,0,&levels,1,0,0,0));
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,0,0,0)==0 && q.queue_progress==5600);
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,1,0,1)==-1 && q.queue_progress==5600);
    q.owner=1;
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,0,0,1)==-1 && q.queue_progress==5600);
    q.owner=0; q.queue_progress=1;
    assert(sc64_pr_job_tick_custom(&q,0,&levels,1,0,0,0,1)==1);
    assert(levels.level[0][SC64_PR_GROUND_WEAPONS]==2 && !q.level && !q.queue_progress);
    assert(sc64_pr_effective_level(SC64_PR_AIR_WEAPONS,0,1,0)==1);
    assert(sc64_pr_effective_level(SC64_PR_SHIELDS,0,1,2)==0);
    q.type=B_CYBERNETICS_CORE; saved=q;
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_SHIELDS,&levels,&ore,&local_gas,
        3,1,1,1,0,1,0,0,0)); /* shields require Forge */
    assert(!memcmp(&q,&saved,sizeof q));
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_AIR_WEAPONS,&levels,&ore,&local_gas,
        1,1,1,1,0,1,0,0,0)); /* already at legacy floor / cap */
    assert(!sc64_pr_job_begin_custom(&q,0,SC64_PR_AIR_WEAPONS,&levels,&ore,&local_gas,
        3,1,1,1,1,1,0,0,0)); /* duplicate */
    assert(!memcmp(&q,&saved,sizeof q));
    q.owner=1; saved=q;
    assert(!sc64_pr_job_begin_custom(&q,1,SC64_PR_AIR_WEAPONS,&levels,&ore,&local_gas,
        3,1,1,1,0,1,0,0,0)); /* human legacy floor must not affect an AI */
    assert(!memcmp(&q,&saved,sizeof q));
    /* Actual custom roster and producer admission, not caller-supplied flags. */
    race_mode=1; race_bots=1; race_player[1]=R_PROTOSS;
    memset(&sc64_experimental_research_levels,0,sizeof sc64_experimental_research_levels);
    memset(b,0,sizeof b); memset(sc64_pr_paid,0,sizeof sc64_pr_paid);
    b[0].alive=1; b[0].owner=1; b[0].type=B_CYBERNETICS_CORE;
    b[0].raw_type=-1; b[0].x=100; b[0].y=100;
    b[1].alive=1; b[1].owner=1; b[1].type=B_PYLON;
    b[1].raw_type=-1; b[1].x=100; b[1].y=100;
    race_minerals[1]=race_gas[1]=1000;
    assert(sc64_pr_stage_begin_slot(0,SC64_PR_AIR_WEAPONS));
    assert(race_minerals[1]==900 && race_gas[1]==900 && sc64_pr_paid[0].live);
    b[2]=b[0]; b[2].level=0; b[2].queue[0]=b[2].queue[1]=0; b[2].queue_progress=0;
    saved=b[2]; assert(!sc64_pr_stage_begin_slot(2,SC64_PR_AIR_WEAPONS));
    assert(!memcmp(&b[2],&saved,sizeof saved) && race_minerals[1]==900 && race_gas[1]==900);
    static u8 previous_sram[32768];
    memset(host_save,0xa5,sizeof host_save);
    memcpy(previous_sram,host_save,sizeof host_save);
    assert(!save_game() && !race_save_game());
    assert(!memcmp(previous_sram,host_save,sizeof host_save));
    assert(!load_game() && !race_load_game());
    assert(!memcmp(previous_sram,host_save,sizeof host_save));
    assert(!sc64_pr_stage_transfer_allowed(0,SIDE));
    /* A later level grant does not alter the original paid refund. */
    sc64_experimental_research_levels.level[1][SC64_PR_AIR_WEAPONS]=1;
    assert(sc64_pr_stage_cancel_slot(0,0));
    assert(race_minerals[1]==1000 && race_gas[1]==1000);
    assert(!b[0].level && !b[0].queue_progress && !sc64_pr_paid[0].live);
    assert(!sc64_pr_stage_cancel_slot(0,0));
    assert(race_minerals[1]==1000 && race_gas[1]==1000);
    /* Level two air research requires a completed same-owner Fleet Beacon. */
    saved=b[0]; assert(!sc64_pr_stage_begin_slot(0,SC64_PR_AIR_WEAPONS));
    assert(!memcmp(&b[0],&saved,sizeof saved));
    sc64_experimental_research_levels.level[1][SC64_PR_AIR_WEAPONS]=0;
    assert(sc64_pr_stage_begin_slot(0,SC64_PR_AIR_WEAPONS));
    int before_ore=race_minerals[1], before_gas=race_gas[1];
    b[0].owner=SIDE; saved=b[0];
    destroy_building(0,SIDE);
    assert(!memcmp(&b[0],&saved,sizeof saved));
    assert(race_minerals[1]==before_ore && race_gas[1]==before_gas);
    b[0].owner=1; race_minerals[1]=65535; saved=b[0];
    destroy_building(0,SIDE);
    assert(!memcmp(&b[0],&saved,sizeof saved));
    assert(race_minerals[1]==65535 && race_gas[1]==before_gas);
    race_minerals[1]=before_ore; destroy_building(0,SIDE);
    assert(!b[0].alive && !b[0].level && !b[0].queue_progress && !sc64_pr_paid[0].live);
    assert(race_minerals[1]==before_ore+75 && race_gas[1]==before_gas+75);
    destroy_building(0,SIDE);
    assert(race_minerals[1]==before_ore+75 && race_gas[1]==before_gas+75);

    /* Human wallet uses signed-int range; shields require an owned Core. */
    race_human=R_PROTOSS; race_player[SIDE]=R_PROTOSS; minerals=gas=1000;
    b[3].alive=1; b[3].owner=SIDE; b[3].type=B_FORGE; b[3].raw_type=-1;
    b[3].x=b[3].y=200;
    b[4].alive=1; b[4].owner=SIDE; b[4].type=B_PYLON; b[4].raw_type=-1;
    b[4].x=b[4].y=200;
    assert(sc64_pr_stage_begin_slot(3,SC64_PR_SHIELDS)); /* level 1 needs no Core */
    assert(sc64_pr_stage_cancel_slot(3,0) && minerals==1000 && gas==1000);
    sc64_experimental_research_levels.level[SIDE][SC64_PR_SHIELDS]=1;
    assert(!sc64_pr_stage_begin_slot(3,SC64_PR_SHIELDS)); /* level 2 needs Core */
    b[5].alive=1; b[5].owner=SIDE; b[5].type=B_CYBERNETICS_CORE; b[5].raw_type=-1;
    assert(sc64_pr_stage_begin_slot(3,SC64_PR_SHIELDS));
    minerals=INT_MAX; int human_gas=gas; saved=b[3];
    assert(!sc64_pr_stage_cancel_slot(3,0));
    assert(!memcmp(&b[3],&saved,sizeof saved) && minerals==INT_MAX && gas==human_gas);
    minerals=70000; assert(sc64_pr_stage_cancel_slot(3,0));
    assert(minerals==70300 && gas==human_gas+300);
    b[0].level=1; b[0].queue_progress=1; b[0].queue[0]=1; b[0].queue[1]=1;
    saved=b[0]; assert(spawn_build(SIDE,B_PYLON,100,100,0)==-1);
    assert(!memcmp(&b[0],&saved,sizeof saved));

    /* Fresh constructed custom state, then actual B/A menu input and update. */
    gba_init(); game_init(); race_mode=1; race_human=R_PROTOSS;
    race_bots=1; race_bot_races[0]=R_PROTOSS; race_skirmish_map=0; init_state();
    int pylon=spawn_build(SIDE,B_PYLON,240,240,0);
    int forge=spawn_build(SIDE,B_FORGE,240,240,0);
    assert(pylon>=0 && forge>=0);
    for(int i=0;i<MAX_UNITS;i++) u[i].selected=0;
    selected_building=forge; minerals=gas=1000; menu_open=0;
#ifdef SC_FIELD_FEEDBACK
    menu_last=0;
#endif
    game_update(KEY_B,KEY_B); assert(menu_open && field_menu_count()==4);
    assert(!strcmp(race_menu_label(0),"GROUND WEAPONS"));
    game_update(0,0); game_update(KEY_A,KEY_A);
    assert(!menu_open && sc64_pr_paid[forge].live && minerals==900 && gas==900);
    int paid_ore=minerals, paid_gas=gas; start_research(0);
    assert(!weapons_research && minerals==paid_ore && gas==paid_gas);
    unsigned remaining=b[forge].queue_progress;
    b[forge].raw_type=1; sc64_pr_stage_tick();
    assert(b[forge].queue_progress==remaining && sc64_pr_paid[forge].live);
    b[forge].raw_type=-1; b[pylon].alive=0;
    game_update(0,0); assert(b[forge].queue_progress==remaining);
    b[pylon].alive=1; b[forge].queue_progress=1;
    game_update(0,0);
    assert(!sc64_pr_paid[forge].live && !b[forge].queue_progress &&
           sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]==1);
    /* Selected-weapon family, legacy floors, owner isolation and range. */
    weapons_level=1; armor_level=1;
    sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_WEAPONS]=2;
    sc64_experimental_research_levels.level[SIDE][SC64_PR_AIR_WEAPONS]=3;
    sc64_experimental_research_levels.level[SIDE][SC64_PR_GROUND_ARMOR]=2;
    assert(sc64_pr_stage_weapon_level(SIDE,66)==2); /* Dragoon, both targets */
    assert(sc64_pr_stage_weapon_level(SIDE,73)==3); /* Scout air family */
    assert(sc64_pr_stage_weapon_level(SIDE,62)==1); /* Probe 60 remains legacy */
    assert(sc64_pr_stage_weapon_level(1,66)==0); /* owner does not borrow SIDE */
    Sc64StockAttack attack;
    assert(sc64_stock_attack_select(66,0,&attack) && attack.weapon_id==66);
    assert(sc64_stock_attack_damage_per_impact(&attack,sc64_pr_stage_weapon_level(SIDE,attack.weapon_id)) >
           sc64_stock_attack_damage_per_impact(&attack,weapons_level));
    Unit target={0}; target.alive=1; target.owner=SIDE; target.type=U_ZEALOT; target.raw_type=65;
    assert(sc64_pr_stage_unit_armor_level(&target)==2);
    target.owner=1; assert(sc64_pr_stage_unit_armor_level(&target)==0);
    target.owner=SIDE; target.type=U_DRAGOON; target.raw_type=66;
    assert(sc64_pr_stage_dragoon_bonus(&target)==0);
    sc64_experimental_research_levels.level[SIDE][SC64_PR_SINGULARITY]=1;
    assert(sc64_pr_stage_dragoon_bonus(&target)==32);
    target.raw_type=17; assert(sc64_pr_stage_dragoon_bonus(&target)==0);
    memset(&u[0],0,sizeof u[0]); u[0].alive=1; u[0].owner=SIDE;
    u[0].type=U_ZEALOT; u[0].raw_type=65; u[0].hp=100; u[0].shield=0;
    hit_target_impl_ex(0,10,U_MARINE,ENEMY,SC64_STOCK_DMG_NORMAL,0);
    assert(u[0].hp==92); /* one typed armor bonus, never added to legacy */
    puts("research job staged admission, lifecycle, menu and save refusal: ok");
    return 0;
}
