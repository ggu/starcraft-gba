/* Actual-engine coverage for the default-off direct AttackUnit order. */
#include "../starcraft/game.c"
#include <assert.h>

static void reveal_box(int x0, int y0, int x1, int y1, int on)
{
    int x, y;
    for (y = y0 / 16; y <= y1 / 16; ++y)
        for (x = x0 / 16; x <= x1 / 16; ++x)
            if (x >= 0 && y >= 0 && x < MAP_COLS && y < MAP_ROWS)
                SC64_GRID_SET(visible, x + y * MAP_COLS, on);
}

static int add_enemy_unit(int type, int x, int y)
{
    int slot = spawn_unit(ENEMY, type, x, y);
    assert(slot >= 0);
    return slot;
}

int main(void)
{
    int attacker, near_target, far_target, building, last_x, last_y;
    int near_hp, far_hp;
    Sc64DirectTargetState state = {1, 1, 1, 0, 0, 1};
    assert(sc64_direct_target_valid(state));
    assert(sc64_direct_target_can_damage(state));
    state.visible = 0;
    assert(sc64_direct_target_valid(state));
    assert(!sc64_direct_target_can_damage(state));
    state.visible = 1; state.loaded = 1;
    assert(!sc64_direct_target_valid(state));
    state.loaded = 0; state.invincible = 1;
    assert(!sc64_direct_target_valid(state));

    gba_init();
    game_init();
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = R_TER;
    race_player[ENEMY] = R_ZERG;
    race_player[RESCUABLE] = R_PROTOSS;
    race_player[4] = R_TER;
    race_mode = 1;
    assert(!owners_hostile(SIDE, NEUTRAL));
    assert(owners_hostile(SIDE, RESCUABLE) && owners_hostile(SIDE, 4));
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
#endif
    SC64_GRID_CLEAR(visible);
    race_mode = 1;
    race_human = R_TER;
    attacker = spawn_unit(SIDE, U_MARINE, 100, 100);
    near_target = add_enemy_unit(U_ZERGLING, 116, 100);
    far_target = add_enemy_unit(U_ZERGLING, 150, 100);
    assert(attacker >= 0);
    u[attacker].selected = 1;
    reveal_box(80, 80, 220, 140, 1);

    /* A click on the farther hostile keeps focus even when another enemy is
       nearer. The persistent packed order fields are the only state used. */
    mode = MODE_ATTACK; cx = u[far_target].x; cy = u[far_target].y;
    near_hp = u[near_target].hp;
    far_hp = u[far_target].hp;
    interact();
    assert(u[attacker].order == ORDER_ATTACK_TARGET &&
           u[attacker].target == far_target);
    update_orders();
    assert(u[attacker].order == ORDER_ATTACK_TARGET &&
           u[attacker].target == far_target);
    assert(u[far_target].hp < far_hp && u[near_target].hp == near_hp);

    /* A moving, visible target updates its last-seen position while preserving
       identity through approach and cooldown ticks. */
    u[far_target].x = 184;
    u[far_target].y = 104;
    update_orders();
    assert(u[attacker].target == far_target && u[attacker].tx == 184 &&
           u[attacker].ty == 104);
    u[attacker].cd = 3;
    update_orders();
    assert(u[attacker].target == far_target && u[attacker].cd == 2);

    /* Fogged targets do not leak their current position into movement or
       combat; only the saved last-visible coordinate is approached. */
    last_x = u[attacker].tx; last_y = u[attacker].ty;
    reveal_box(160, 80, 220, 140, 0);
    u[far_target].x = 220; u[far_target].y = 100;
    update_orders();
    assert(u[attacker].target == far_target && u[attacker].tx == last_x &&
           u[attacker].ty == last_y);
    assert(u[attacker].order == ORDER_ATTACK_TARGET);
    /* Arriving at the stored last-seen point expires a target that remains
       outside current vision. Hidden live coordinates stay irrelevant. */
    u[attacker].x = last_x;
    u[attacker].y = last_y;
    sc64_direct_attack_tick_unit(attacker);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);
    u[far_target].alive = 0;

    /* A hostile building is also a combined-namespace direct target. */
    reveal_box(80, 80, 220, 140, 1);
    building = spawn_build(ENEMY, B_BARRACKS, 200, 100, 0);
    assert(building >= 0);
    mode = MODE_ATTACK; cx = b[building].x; cy = b[building].y;
    interact();
    assert(u[attacker].order == ORDER_ATTACK_TARGET &&
           u[attacker].target == MAX_UNITS + building);

    /* A ground click remains AttackMove. */
    mode = MODE_ATTACK; cx = 300; cy = 300;
    interact();
    assert(u[attacker].order == ORDER_ATTACK_MOVE && u[attacker].target == -1);

    /* SCVs are ground-capable in the race rules and receive direct orders. */
    u[attacker].selected = 0;
    int worker = spawn_unit(SIDE, U_SCV, 100, 120);
    assert(worker >= 0);
    u[worker].selected = 1;
    mode = MODE_ATTACK; cx = u[near_target].x; cy = u[near_target].y;
    interact();
    assert(u[worker].order == ORDER_ATTACK_TARGET &&
           u[worker].target == near_target);

    /* A true weaponless actor cannot receive a direct combat order. */
    u[worker].alive = 0;
    int noncombatant = spawn_unit(SIDE, U_OVERLORD, 100, 120);
    assert(noncombatant >= 0);
    u[noncombatant].selected = 1;
    mode = MODE_ATTACK; cx = u[near_target].x; cy = u[near_target].y;
    interact();
    assert(u[noncombatant].order != ORDER_ATTACK_TARGET);
    u[noncombatant].order = ORDER_ATTACK_TARGET;
    u[noncombatant].target = near_target;
    sc64_direct_attack_tick_unit(noncombatant);
    assert(u[noncombatant].order == ORDER_IDLE &&
           u[noncombatant].target == -1);

    /* Other skirmish owners remain hostile candidates; owner 4 is not
       collapsed into the ordinary ENEMY slot by the direct-order scan. */
    int faction4 = spawn_unit(4, U_ZERGLING, 150, 140);
    assert(faction4 >= 0);
    u[attacker].selected = 1;
    mode = MODE_ATTACK; cx = u[faction4].x; cy = u[faction4].y;
    interact();
    assert(u[attacker].order == ORDER_ATTACK_TARGET &&
           u[attacker].target == faction4);
    int faction3 = spawn_unit(RESCUABLE, U_ZERGLING, 170, 140);
    assert(faction3 >= 0);
    mode = MODE_ATTACK; cx = u[faction3].x; cy = u[faction3].y;
    interact();
    assert(u[attacker].order == ORDER_ATTACK_TARGET &&
           u[attacker].target == faction3);

    /* Intrinsic invulnerability and hidden source actors are never retained
       as focus targets. */
    u[attacker].target = faction4;
    u[attacker].order = ORDER_ATTACK_TARGET;
    u[faction4].flags |= 16;
    sc64_direct_attack_tick_unit(attacker);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);
    u[faction4].flags &= (u16)~16u;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    invincible_units[faction4] = 1;
    u[attacker].target = faction4;
    u[attacker].order = ORDER_ATTACK_TARGET;
    sc64_direct_attack_tick_unit(attacker);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);
    invincible_units[faction4] = 0;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    int hidden_type = sc64_race_unit_from_raw(101);
    if (hidden_type >= 0) {
        int hidden = spawn_unit(ENEMY, hidden_type, 150, 140);
        assert(hidden >= 0);
        u[attacker].order = ORDER_ATTACK_TARGET;
        u[attacker].target = hidden;
        sc64_direct_attack_tick_unit(attacker);
        assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);
    }
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    u[attacker].order = ORDER_ATTACK_TARGET;
    u[attacker].target = faction4;
    u[faction4].order = ORDER_TRANSPORTED;
    sc64_direct_attack_tick_unit(attacker);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);
    u[faction4].order = ORDER_IDLE;
#endif

    /* A capable ground-only Zergling can focus ground targets but cannot
       acquire a hostile flying Overlord through the direct-order path. */
    int ground_only = spawn_unit(SIDE, U_ZERGLING, 100, 120);
    int flying_target = spawn_unit(ENEMY, U_OVERLORD, 300, 160);
    assert(ground_only >= 0 && flying_target >= 0);
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    u[ground_only].selected = 1;
    reveal_box(280, 140, 320, 180, 1);
    mode = MODE_ATTACK; cx = u[flying_target].x; cy = u[flying_target].y;
    interact();
    assert(u[ground_only].order != ORDER_ATTACK_TARGET);
    mode = MODE_ATTACK; cx = u[near_target].x; cy = u[near_target].y;
    interact();
    assert(u[ground_only].order == ORDER_ATTACK_TARGET &&
           u[ground_only].target == near_target);

    /* The existing packed ZP row already carries order and target exactly. */
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    {
        Sc64ZpPackedUnitSave row;
        Unit decoded;
        u[attacker].order = ORDER_ATTACK_TARGET;
        u[attacker].target = MAX_UNITS + building;
        assert(sc64_zp_unit_save_encode(&row, &u[attacker], WORLD_W, WORLD_H));
        assert(sc64_zp_unit_save_decode(&decoded, &row, WORLD_W, WORLD_H));
        assert(decoded.order == ORDER_ATTACK_TARGET &&
               decoded.target == MAX_UNITS + building);
    }
#endif
    b[building].flags |= 16;
    sc64_direct_attack_tick_unit(attacker);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);
    b[building].flags &= (u16)~16u;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    invincible_buildings[building] = 1;
    u[attacker].order = ORDER_ATTACK_TARGET;
    u[attacker].target = MAX_UNITS + building;
    sc64_direct_attack_tick_unit(attacker);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);
    invincible_buildings[building] = 0;
#endif

    /* Dead target slots are cleared before reuse, so stale focus cannot
       silently transfer to a new actor occupying the same slot. */
    u[attacker].selected = 1;
    u[attacker].order = ORDER_ATTACK_TARGET;
    u[attacker].target = near_target;
    u[near_target].alive = 0;
    int replacement = spawn_unit(ENEMY, U_ZERGLING, 116, 100);
    assert(replacement == near_target);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);

    /* Reusing a building slot also invalidates the combined target ID. */
    u[attacker].order = ORDER_ATTACK_TARGET;
    u[attacker].target = MAX_UNITS + building;
    b[building].alive = 0;
    int replacement_building = spawn_build(ENEMY, B_BARRACKS, 200, 100, 0);
    assert(replacement_building == building);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);

    return 0;
}
