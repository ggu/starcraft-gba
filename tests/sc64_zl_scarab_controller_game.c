/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST state, actual Z10 descriptor and Scarab lifecycle. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static unsigned held_keys;
static void input(unsigned held)
{ game_update((u16)held, (u16)(held & ~held_keys)); held_keys=held; }
static void press(unsigned key) { input(key); input(0); }
static void launch_z10_from_normal_menu(void)
{
    int ordinal;
    game_init();held_keys=0;assert(race_front.page==SC_FRONT_HOME);
    ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z10_GLOBAL_ID);
    assert(ordinal==9 && sc64_catalog_playable_count(SC64_FACTION_ZERG)==10);
    press(KEY_A);assert(race_front.page==SC_FRONT_MISSION && race_front.faction==SC_FRONT_ZERG);
    for (int page=0;page<ordinal/2;++page) press(KEY_RIGHT);
    if (ordinal&1) press(KEY_DOWN);
    press(KEY_A);assert(race_front.page==SC_FRONT_BRIEFING && race_front.mission==SC64_Z10_GLOBAL_ID);
    press(KEY_START);assert(screen==SCREEN_PLAY && !race_mode && race_zl_active());
    assert(race_selected_mission==SC64_Z10_GLOBAL_ID && mission_unsupported_count==0);
}

int main(void)
{
    int parent=-1, child=-1, target=-1, origin, prior_owner;
    launch_z10_from_normal_menu();
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==U_REAVER &&
        u[i].owner==ENEMY && sc64_controller_unit(i)==0) { parent=i; break; }
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].owner==SIDE && u[i].type==U_ZERGLING && !invincible_units[i]) { target=i;break; }
    assert(parent>=0 && target>=0);origin=u[parent].raw_owner;prior_owner=u[parent].owner;
    assert(origin==0 && sc64_controller_transfer_unit(parent,7));
    assert(sc64_controller_unit(parent)==7 && u[parent].raw_owner==0 && u[parent].owner==prior_owner);
    u[parent].x=500;u[parent].y=500;u[parent].cd=0;
    u[target].x=504;u[target].y=500;
    u[target].hp=u[target].maxhp=200;u[target].shield=0;u[target].armor=0;
    { int raw,x,y,owner,hp=u[target].hp;
      assert(race_scarab_probe_target_info(target,&raw,&x,&y,&owner) && owners_hostile(prior_owner,owner));
      assert((sc64_race_unit_range_vs(u[parent].type,0)+1)/2>=4);
      tick_combat(&u[parent],target);assert(u[target].hp==hp); }
    assert(sc64_controller_transfer_unit(parent,2));
    assert(sc64_controller_unit(parent)==2 && u[parent].raw_owner==origin && u[parent].owner==prior_owner);
    assert(sc64_controller_transfer_unit(parent,7));
    race_zl_ai_minerals[0]=15;race_zl_ai_minerals[1]=15;race_zl_ai_minerals[2]=72;minerals=83;
    assert(race_scarab_ai_restock_stage(parent));
    assert(race_zl_ai_minerals[0]==15 && race_zl_ai_minerals[1]==0 && race_zl_ai_minerals[2]==72 && minerals==83);
    assert(!sc64_controller_transfer_unit(parent,2));
    assert(sc64_controller_unit(parent)==7 && u[parent].owner==prior_owner && u[parent].raw_owner==origin);
    assert(race_zl_ai_minerals[0]==15 && race_zl_ai_minerals[1]==0 && race_zl_ai_minerals[2]==72 && minerals==83);
    assert(race_scarab_cancel(parent,0,0) && race_zl_ai_minerals[1]==15);
    assert(sc64_controller_transfer_unit(parent,2));
    assert(sc64_controller_unit(parent)==2 && u[parent].raw_owner==origin && u[parent].owner==prior_owner);
    assert(race_scarab_ai_restock_stage(parent));
    assert(race_zl_ai_minerals[0]==0 && race_zl_ai_minerals[1]==15 && race_zl_ai_minerals[2]==72 && minerals==83);
    assert(!sc64_controller_transfer_unit(parent,7));
    Sc64ScarabParent state; Sc64ScarabCounts counts;
    race_scarab_human_parent_step(parent);
    assert(race_scarab_parent_read(parent,0,&state,&counts) && state.active_slot>=0);
    child=state.active_slot;
    { int owner=u[child].owner,controller=sc64_controller_unit(child);u32 m0=race_zl_ai_minerals[0],m1=race_zl_ai_minerals[1];
      assert(!sc64_controller_transfer_unit(child,7));
      assert(u[child].owner==owner && sc64_controller_unit(child)==controller && race_zl_ai_minerals[0]==m0 && race_zl_ai_minerals[1]==m1); }
    for (int i=0;i<110;++i) {
        assert(race_scarab_parent_read(parent,0,&state,&counts));
        if (counts.inside) break;
        race_scarab_human_parent_step(parent);
    }
    assert(race_scarab_parent_read(parent,0,&state,&counts) && counts.inside==1 && !state.queued);
    for (int i=0;i<MAX_UNITS;++i) { int p; unsigned phase;
        if (race_scarab_child_lookup(i,&p,&phase) && p==parent && phase==SC64_SCARAB_CHILD_INSIDE) {child=i;break;}
    }
    assert(child>=0 && sc64_controller_unit(child)==2 && u[child].raw_owner==origin);
    { int owner=u[child].owner,controller=sc64_controller_unit(child);u32 m0=race_zl_ai_minerals[0],m1=race_zl_ai_minerals[1];
      assert(!sc64_controller_transfer_unit(child,7));
      assert(u[child].owner==owner && sc64_controller_unit(child)==controller && race_zl_ai_minerals[0]==m0 && race_zl_ai_minerals[1]==m1); }
    assert(!sc64_controller_transfer_unit(parent,7));
    tick_combat(&u[parent],target);
    { int p;unsigned phase;assert(race_scarab_child_lookup(child,&p,&phase) && p==parent && phase==SC64_SCARAB_CHILD_OUTSIDE); }
    { int owner=u[child].owner,controller=sc64_controller_unit(child);u32 m0=race_zl_ai_minerals[0],m1=race_zl_ai_minerals[1];
      assert(!sc64_controller_transfer_unit(child,7));
      assert(u[child].owner==owner && sc64_controller_unit(child)==controller && race_zl_ai_minerals[0]==m0 && race_zl_ai_minerals[1]==m1); }
    assert(!sc64_controller_transfer_unit(parent,7));
    /* Advance only the real source-cadence child and impact services. The
     * authored close ground target is stationary and has 200 HP, no armor or
     * shields: the source weapon's full-radius packet is 100 damage. */
    int launched_packet=0,damage_seen=0;
    for (int i=0;i<120;++i) {
        ++frame;
        if (u[child].alive && sc64_battlecruiser_source_tick_due()) race_scarab_control_dispatch(child);
        if (race_impact_count) launched_packet=1;
        if (!launched_packet) assert(u[target].hp==200);
        race_impact_tick();
        assert(race_zl_ai_minerals[0]==0 && race_zl_ai_minerals[1]==15 && race_zl_ai_minerals[2]==72 && minerals==83);
        assert(sc64_controller_unit(parent)==2);
        if (u[target].hp<200) { damage_seen=1;break; }
    }
    fprintf(stderr,"impact packet=%d damage=%d hp=%d alive=%d childxy=%d,%d targetxy=%d,%d order=%d pending=%u carry=%d frame=%d\n",launched_packet,damage_seen,u[target].hp,u[child].alive,u[child].x,u[child].y,u[target].x,u[target].y,u[child].order,race_impact_count,u[child].carry,frame);
    assert(launched_packet && damage_seen && u[target].hp==100 && !u[child].alive);
    hit_target(parent,10000,U_MARINE,SIDE);
    assert(!u[parent].alive && sc64_controller_unit(parent)==-1);
    int reused=spawn_unit(ENEMY,U_REAVER,128,128);assert(reused==parent);
    assert(sc64_controller_unit(reused)==-1 && !race_scarab_ai_restock_stage(reused));
    assert(!race_save_game() && !race_load_game());
    puts("Z10 paid Scarab controller and wallet: PASS");return 0;
}
