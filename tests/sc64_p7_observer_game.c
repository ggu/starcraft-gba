/* Actual-engine fixture for the optional P6/P7 private-input profile.
 * The public pure policy fixture requires no imported game inputs. */
#include "../starcraft/game.c"

#include <assert.h>

static int add_unit(int owner, int type, int raw_type, int raw_owner,
                    int x, int y)
{
    int slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0);
    u[slot].raw_type = raw_type;
    u[slot].raw_owner = raw_owner;
    return slot;
}

int main(void)
{
    assert(sc64_p7_observer_xy_length(288,0)==288);
    assert(sc64_p7_observer_xy_length(289,0)==289);
    assert(sc64_p7_observer_xy_length(22,22)==29);
    assert(sc64_p7_observer_xy_length(100,25)==100);
    assert(sc64_p7_observer_xy_length(100,26)==102);
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    memset(visible,0,sizeof(visible));
    race_mode = 0;
    race_selected_mission = SC64_P7_GLOBAL_ID;

    /* Source owners 0 and 4 both have live ENEMY ownership. Only owner 0's
       probe and Observer may reveal/detect the human Observer at this cell. */
    add_unit(ENEMY,U_PROBE,64,0,160,160);
    int detector = add_unit(ENEMY,U_P7_OBSERVER,84,0,160,160);
    int target = spawn_unit(SIDE,U_P7_OBSERVER,160,160);
    assert(target >= 0 && u[target].raw_type==84 && u[target].raw_owner==7);
    assert(detector >= 0);
    assert(sc64_p7_observer_can_attack_from(U_MARINE,target,ENEMY,0));
    u[detector].order = ORDER_TRANSPORTED;
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,target,ENEMY,0));
    assert(nearest_enemy_typed(160,160,ENEMY,64,U_MARINE,0)==-1);
    u[detector].order = ORDER_IDLE;
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,target,ENEMY,4));

    int attacker_slot = add_unit(ENEMY,U_MARINE,0,0,160,160);
    Unit *attacker = &u[attacker_slot];
    assert(nearest_enemy_typed(attacker->x,attacker->y,ENEMY,64,
                               U_MARINE,0) == target);
    assert(nearest_enemy_typed(attacker->x,attacker->y,ENEMY,64,
                               U_MARINE,4) == -1);
    int hp_before = u[target].hp, shield_before = u[target].shield;
    tick_combat(attacker,target);
    assert(u[target].hp < hp_before || u[target].shield < shield_before);
    int hp_after_owner0_hit = u[target].hp;
    int shield_after_owner0_hit = u[target].shield;
    Unit *owner4_attacker = &u[add_unit(ENEMY,U_MARINE,0,4,160,160)];
    tick_combat(owner4_attacker,target);
    assert(u[target].hp == hp_after_owner0_hit &&
           u[target].shield == shield_after_owner0_hit);

    /* Current GBA optical sight includes alive buildings during construction.
       Such a Photon Cannon can reveal a cell, but cannot detect a cloaked
       Observer until completion. Native construction-sight differences are
       outside this current cell-circle approximation. */
    int unfinished_cannon = spawn_build(ENEMY,B_PHOTON_CANNON,1000,700,0);
    assert(unfinished_cannon >= 0);
    b[unfinished_cannon].raw_type = 162;
    b[unfinished_cannon].raw_owner = 0;
    b[unfinished_cannon].under_construction = 1;
    u[target].x = 1000; u[target].y = 700;
    assert(sc64_p7_observer_target_optically_visible(target,0,ENEMY));
    assert(!sc64_p7_observer_detector_sees(target,0,ENEMY));
    u[target].x = 160; u[target].y = 160;

#ifdef SC64_P7_ARCHON_SPLASH
    /* Cloak blocks primary acquisition for source owner 0 without a detector,
       but the separate radial path still applies collateral to that cloaked
       Observer when a visible Zealot is the primary target. */
    u[detector].order = ORDER_TRANSPORTED;
    assert(!sc64_p7_observer_detector_sees(target,0,ENEMY));
    int primary = add_unit(SIDE,U_ZEALOT,65,7,172,160);
    int archon_slot = add_unit(ENEMY,U_P7_ARCHON,68,0,160,160);
    assert(nearest_enemy_typed(160,160,ENEMY,64,U_P7_ARCHON,0)==primary);
    int collateral_hp = u[target].hp, collateral_shield = u[target].shield;
    tick_combat(&u[archon_slot],primary);
    assert(u[target].hp < collateral_hp || u[target].shield < collateral_shield);
    u[detector].order = ORDER_IDLE;
#endif

    /* Source bounding-box distance is inclusive and uses the pinned integer
       XY metric: two same-size Observers at a 318 px center delta have a 287
       px gap (inside 288); a 320 px center delta has a 289 px gap (outside). */
    add_unit(ENEMY,U_PROBE,64,0,319,160);
    u[target].x=319;
    assert(sc64_p7_observer_detector_sees(target,0,ENEMY));
    u[target].x=320;
    assert(!sc64_p7_observer_detector_sees(target,0,ENEMY));
    u[target].x=160;

    /* The source-owner unknown-provenance path deliberately falls back to
       the merged live owner, preserving legacy behavior for such products. */
    add_unit(ENEMY,U_PROBE,64,-1,160,160);
    assert(sc64_p7_observer_can_attack_from(U_MARINE,target,ENEMY,-1));

    /* A powered, completed Photon Cannon is a fixed-radius detector even
       without Observer sight. Its Pylon and Cannon must share raw owner 4. */
    int pylon = spawn_build(ENEMY,B_PYLON,240,160,0);
    int cannon = spawn_build(ENEMY,B_PHOTON_CANNON,240,160,0);
    assert(pylon >= 0 && cannon >= 0);
    b[pylon].raw_type = 156; b[pylon].raw_owner = 4;
    b[cannon].raw_type = 162; b[cannon].raw_owner = 4;
    assert(sc64_p7_observer_detector_sees(target,4,ENEMY));
    b[pylon].raw_owner = 0;
    assert(!sc64_p7_observer_detector_sees(target,4,ENEMY));
    return 0;
}
