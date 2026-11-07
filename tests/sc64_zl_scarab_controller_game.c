/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST state, actual Z10 descriptor and Scarab lifecycle. */
#define main sc64_original_z10_main
#include "sc64_z10_game.c"
#undef main

int main(void)
{
    int parent=-1, child=-1, origin, prior_owner;
    launch_z10_from_normal_menu();
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==U_REAVER &&
        u[i].owner==ENEMY && sc64_controller_unit(i)==7) { parent=i; break; }
    assert(parent>=0);origin=u[parent].raw_owner;prior_owner=u[parent].owner;
    assert(sc64_controller_transfer_unit(parent,2));
    assert(sc64_controller_unit(parent)==2 && u[parent].raw_owner==origin && u[parent].owner==prior_owner);
    assert(sc64_controller_transfer_unit(parent,7));
    race_zl_ai_minerals[0]=61;race_zl_ai_minerals[1]=15;race_zl_ai_minerals[2]=72;minerals=83;
    assert(race_scarab_ai_restock_stage(parent));
    assert(race_zl_ai_minerals[0]==61 && race_zl_ai_minerals[1]==0 && race_zl_ai_minerals[2]==72 && minerals==83);
    assert(!sc64_controller_transfer_unit(parent,2));
    assert(sc64_controller_unit(parent)==7 && u[parent].owner==prior_owner && u[parent].raw_owner==origin);
    assert(race_zl_ai_minerals[0]==61 && race_zl_ai_minerals[1]==0 && race_zl_ai_minerals[2]==72 && minerals==83);
    assert(race_scarab_cancel(parent,0,0) && race_zl_ai_minerals[1]==15);
    assert(sc64_controller_transfer_unit(parent,2));
    assert(sc64_controller_unit(parent)==2 && u[parent].raw_owner==origin && u[parent].owner==prior_owner);
    assert(race_scarab_ai_restock_stage(parent));
    assert(race_zl_ai_minerals[0]==46 && race_zl_ai_minerals[1]==15 && race_zl_ai_minerals[2]==72 && minerals==83);
    assert(!sc64_controller_transfer_unit(parent,7));
    for (int i=0;i<107;++i) race_scarab_human_parent_step(parent);
    Sc64ScarabParent state; Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(parent,0,&state,&counts) && counts.inside==1 && !state.queued);
    for (int i=0;i<MAX_UNITS;++i) { int p; unsigned phase;
        if (race_scarab_child_lookup(i,&p,&phase) && p==parent && phase==SC64_SCARAB_CHILD_INSIDE) {child=i;break;}
    }
    assert(child>=0 && sc64_controller_unit(child)==2 && u[child].raw_owner==origin);
    assert(!sc64_controller_transfer_unit(parent,7));
    assert(race_scarab_launch_child(parent,0,child));
    assert(!sc64_controller_transfer_unit(parent,7));
    hit_target(parent,10000,U_MARINE,SIDE);
    assert(!u[parent].alive && sc64_controller_unit(parent)==-1);
    int reused=spawn_unit(ENEMY,U_REAVER,128,128);assert(reused==parent);
    assert(sc64_controller_unit(reused)==-1 && !race_scarab_ai_restock_stage(reused));
    assert(!race_save_game() && !race_load_game());
    puts("Z10 paid Scarab controller and wallet: PASS");return 0;
}
