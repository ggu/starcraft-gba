/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real-engine phase/eligibility test; intentionally constructed states, not
 * a normal-input acquisition, flight or campaign qualification. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_scarab_unit_adapter.h"

static void set_phase(int child, int parent, unsigned phase, unsigned life, unsigned nc)
{
    assert(race_scarab_child_set(child,(unsigned)parent,phase,life,nc));
}
int main(void)
{
    gba_init(); game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1; init_state();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); minerals=100;
    int parent=spawn_unit(SIDE,U_REAVER,128,128);
    assert(parent>=0 && race_scarab_parent_init(parent));
    assert(race_scarab_enqueue(parent,0));
    assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent p; Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(parent,0,&p,&counts)); int child=p.active_slot;
    /* A hidden child's stored coordinates need not follow its moving parent. */
    u[child].x=u[child].tx=512; u[child].y=u[child].ty=512;
    int mover=spawn_unit(ENEMY,U_MARINE,530,512); assert(mover>=0);
    /* A terrain-valid empty footprint is an independent placement baseline. */
    int px=-1,py=-1; u[child].alive=0; u[mover].alive=0;
    for(int y=64;y<WORLD_H-32 && px<0;y+=32) for(int x=64;x<WORLD_W-32;x+=32)
        if(can_place_except_worker(B_PYLON,x,y,-1)){px=x;py=y;break;}
    assert(px>=0); u[child].alive=1; u[mover].alive=1;
    for(unsigned phase=0;phase<2;phase++) {
        set_phase(child,parent,phase,0,0);
        sc64_unit_buckets_begin();
        assert(SC64_SCARAB_HIDDEN(&u[child]) && SC64_SCARAB_NONCOLLIDING(&u[child]));
        assert(!target_alive(child));
        sc64_target_index_begin();
        assert(sc64_target_index_active);
        for(int owner=0;owner<5;owner++) for(int cell=0;cell<SC64_TARGET_CELL_COUNT;cell++)
            for(int e=sc64_target_head[owner][cell];e!=SC64_TARGET_NONE;e=sc64_target_next[e])
                assert(e!=child);
        assert(unit_step_clear(&u[mover],512,512));
        u[mover].alive=0; u[child].x=px; u[child].y=py;
        assert(can_place_except_worker(B_PYLON,px,py,-1));
#ifdef SC64_P3_PRODUCTION_EXIT
        Sc64P3ProductionExitContext exit_ctx={0}; exit_ctx.producer_type=U_MARINE;
        assert(!sc64_p3_exit_unit_blocked(&exit_ctx,px,py,0));
#endif
        u[mover].alive=1; u[child].x=u[child].y=512;
        sc64_unit_bucket_active=0;
        assert(unit_step_clear(&u[mover],512,512));
        cx=cy=512; assert(selection_target()!=child);
        select_visible_type(U_SCARAB); assert(!u[child].selected);
        box_anchor_x=box_anchor_y=511; box_end_x=box_end_y=513;
        select_box_units(); assert(!u[child].selected);
        cycle_selection(); assert(!u[child].selected);
        /* Rendering must not emit even a fallback or health/shield bar. */
        camx=480; camy=480; gfx_begin(C_BLACK);
        static u8 before[240*160] __attribute__((aligned(4)));
        static u8 after[240*160] __attribute__((aligned(4)));
        assert(gfx_read_band(0,160,before)); draw_unit(&u[child]);
        assert(gfx_read_band(0,160,after)); assert(!memcmp(before,after,sizeof(before)));
        sc64_update_exploration(); assert(!currently_visible(512,512));
        int hp=u[child].hp,shield=u[child].shield;
        invincible_units[child]=0; hit_target(child,100,U_MARINE,ENEMY);
        assert(u[child].hp==hp && u[child].shield==shield);
        invincible_units[child]=1;
    }
    /* Start from active indexes, then launch without fixture-side resets. */
    sc64_unit_buckets_begin(); sc64_target_index_begin(); target_lists_valid=1;
    set_phase(child,parent,SC64_SCARAB_CHILD_OUTSIDE,90,7);
    assert(!target_lists_valid && !sc64_target_index_active && !sc64_unit_bucket_active);
    assert(!SC64_SCARAB_HIDDEN(&u[child]) && SC64_SCARAB_NONCOLLIDING(&u[child]));
    assert(unit_step_clear(&u[mover],512,512));
    assert(unit_step_clear(&u[child],530,512));
    sc64_unit_buckets_begin(); sc64_target_index_begin(); target_lists_valid=1;
    set_phase(child,parent,SC64_SCARAB_CHILD_OUTSIDE,83,0);
    assert(!target_lists_valid && !sc64_target_index_active && !sc64_unit_bucket_active);
    assert(!SC64_SCARAB_NONCOLLIDING(&u[child]));
    assert(!unit_step_clear(&u[mover],512,512));
    sc64_unit_bucket_active=0; assert(!unit_step_clear(&u[mover],512,512));
    memset(visible,0xff,sizeof(visible));
    assert(!sc64_auto_target_candidate_allowed(U_MARINE,child,ENEMY,-1));
    int hp=u[child].hp,shield=u[child].shield;
    hit_target(child,1,U_MARINE,ENEMY);
    assert(u[child].hp==hp && u[child].shield==shield);
    cx=cy=512; assert(selection_target()!=child);
    /* Scripted invincibility override is a separate live flag, not a hard
     * type ban: visible outside actors can become vulnerable explicitly. */
    invincible_units[child]=0;
    assert(sc64_auto_target_candidate_allowed(U_MARINE,child,ENEMY,-1));
    hit_target(child,1,U_MARINE,ENEMY);
    assert(u[child].shield<shield || u[child].hp<hp);
    /* No-parent outside bodies keep world participation, without a stale link. */
    set_phase(child,SC64_SCARAB_FIELD_NO_SLOT,SC64_SCARAB_CHILD_OUTSIDE,82,0);
    int none=99; unsigned phase;
    assert(race_scarab_child_lookup(child,&none,&phase) && none==-1);
    assert(!SC64_SCARAB_HIDDEN(&u[child]) && !SC64_SCARAB_NONCOLLIDING(&u[child]));
    puts("Scarab world presence: hidden phases, selection, collision, vision and live invincibility pass");
    return 0;
}
