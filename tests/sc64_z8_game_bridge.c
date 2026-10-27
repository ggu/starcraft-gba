/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual engine-pool binding fixture. It exercises spawn_unit/spawn_build,
 * raw-source identity, pinned raw geometry, invincibility bytes and wallets. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void reset_pools(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    supply_used = 0;
    unit_tap = 0;
}

static void assert_unit_extent(unsigned raw, unsigned slot)
{
    Sc64Z8ActorView actor;
    int l, t, r, d;
    assert(sc64_z8_game_actor_at(0, slot, &actor) == 1);
    assert(actor.alive && actor.type_known && !actor.building);
    assert(actor.raw_type == raw);
    if (!sc64_p7_actor_rect_source(raw, u[slot].x, u[slot].y, 0,
                                   &l, &t, &r, &d))
        assert(sc64_z7_world_actor_rect(raw, u[slot].x, u[slot].y,
                                        &l, &t, &r, &d));
    assert(actor.left == u[slot].x * 2 - l);
    assert(actor.top == u[slot].y * 2 - t);
    assert(actor.right == r - u[slot].x * 2 - 1);
    assert(actor.bottom == d - u[slot].y * 2 - 1);
    assert(actor.invincible == &invincible_units[slot]);
}

static void assert_building_extent(unsigned raw, unsigned slot)
{
    Sc64Z8ActorView actor;
    int l, t, r, d;
    assert(sc64_z8_game_actor_at(0, MAX_UNITS + slot, &actor) == 1);
    assert(actor.alive && actor.type_known && actor.building);
    assert(actor.raw_type == raw);
    if (sc64_game_building_bounds_source(b[slot].type, &l, &t, &r, &d)) {
        assert(actor.left == l && actor.top == t &&
               actor.right == r && actor.bottom == d);
    } else {
        if (!sc64_p7_actor_rect_source(raw, b[slot].x, b[slot].y, 1,
                                       &l, &t, &r, &d))
            assert(sc64_z7_world_building_rect(raw, b[slot].x, b[slot].y,
                                                &l, &t, &r, &d));
        assert(actor.left == b[slot].x * 2 - l);
        assert(actor.top == b[slot].y * 2 - t);
        assert(actor.right == r - b[slot].x * 2 - 1);
        assert(actor.bottom == d - b[slot].y * 2 - 1);
    }
    assert(actor.invincible == &invincible_buildings[slot]);
    assert(actor.completed == !b[slot].under_construction);
}

int main(void)
{
    /* Native Z8 descriptor actor kinds after resource/start-marker filtering.
     * In particular 64/65/66/70 require the pinned base-unit fallback. */
    static const u8 unit_raw[] = {
        37, 38, 39, 41, 42, 51, 64, 65, 66, 67, 70, 83, 84
    };
    /* Source structures include both ZP/P6/P7 facts and standard Protoss
     * structures whose exact dimensions come from the P7 raw helper. */
    static const u8 building_raw[] = {
        131, 134, 143, 154, 155, 156, 159, 160, 162, 163, 164,
        165, 166, 167, 171, 194
    };
    unsigned unit_slots[sizeof(unit_raw)], building_slots[sizeof(building_raw)];
    unsigned unit_count = 0, building_count = 0;
    Sc64Z8WorldView world = {0};
    Sc64Z8Rect world_rect = {0, 0, 4095, 4095};
    u32 minerals[SC64_Z8_OWNER_LIMIT] = {0};
    u32 gas[SC64_Z8_OWNER_LIMIT] = {0};

    reset_pools();
    assert(sc64_race_unit_from_raw(214) < 0); /* Start Location is not a unit. */
    for (unsigned i = 0; i < sizeof(unit_raw); ++i) {
        int type = sc64_race_unit_from_raw(unit_raw[i]);
        int slot;
        assert(type >= 0);
        slot = spawn_unit(SIDE, type, 100 + (int)i * 3, 120 + (int)i * 2);
        assert(slot >= 0);
        u[slot].raw_type = unit_raw[i];
        u[slot].raw_owner = (s16)(i % 8u);
        u[slot].alive = 1;
        unit_slots[unit_count++] = (unsigned)slot;
    }
    for (unsigned i = 0; i < sizeof(building_raw); ++i) {
        int type = sc64_race_building_from_raw(building_raw[i]);
        int slot;
        assert(type >= 0);
        slot = spawn_build(SIDE, type, 220 + (int)i * 4, 220 + (int)i * 3,
                           (i & 1u) != 0);
        assert(slot >= 0);
        b[slot].raw_type = building_raw[i];
        b[slot].raw_owner = (s16)(i % 8u);
        b[slot].alive = 1;
        building_slots[building_count++] = (unsigned)slot;
    }

    sc64_z8_game_bind_actor_pool(&world);
    world.active_mask = 0xffu;
    for (unsigned i = 0; i < unit_count; ++i)
        assert_unit_extent(unit_raw[i], unit_slots[i]);
    for (unsigned i = 0; i < building_count; ++i)
        assert_building_extent(building_raw[i], building_slots[i]);

    /* Owner 11 is a valid native non-player actor; All Players excludes it
     * instead of faulting. Dead capacity slots are also readable. */
    int neutral_type = sc64_race_unit_from_raw(67);
    int neutral = spawn_unit(NEUTRAL, neutral_type, 140, 140);
    assert(neutral >= 0);
    u[neutral].raw_type = 67;
    u[neutral].raw_owner = 11;
    Sc64Z8Condition all_players = {
        .player = SC64_Z8_SELECT_ALL_PLAYERS, .unit = 67, .opcode = 2
    };
    u32 count = 99;
    assert(sc64_z8_world_count(&world, &all_players, 0, 0, &count) == 1);
    assert(count == 1); /* The owner-1 raw-67 row counts; owner 11 does not. */
    Sc64Z8ActorView dead;
    assert(sc64_z8_game_actor_at(0, MAX_UNITS - 1u, &dead) == 1 && !dead.alive);

#ifdef SC64_Z7_EGG_PRODUCTION
    /* Pending Z7 Eggs are alive but not complete; after the production tick
     * changes the same slot to its product type, Command counts it complete. */
    int egg_type = sc64_race_unit_from_raw(36);
    assert(egg_type == U_Z7_EGG);
    int egg = spawn_unit(SIDE, egg_type, 180, 180);
    assert(egg >= 0);
    u[egg].raw_type = 36; u[egg].raw_owner = 0; u[egg].alive = 1;
    u[egg].target = U_DRONE; u[egg].work_timer = 10;
    Sc64Z8ActorView egg_view;
    assert(sc64_z8_game_actor_at(0, (unsigned)egg, &egg_view) == 1);
    assert(egg_view.alive && !egg_view.completed);
    Sc64Z8Condition completed_egg = {
        .player = 0, .unit = 36, .opcode = 2, .comparison = 0
    };
    count = 99;
    assert(sc64_z8_world_count(&world, &completed_egg, 0, 0, &count) == 1);
    assert(count == 0);
    completed_egg.comparison = 1; /* AtMost includes in-progress actors. */
    assert(sc64_z8_world_count(&world, &completed_egg, 0, 0, &count) == 1);
    assert(count == 1);
    u[egg].type = U_DRONE; u[egg].raw_type = 41; u[egg].work_timer = 0;
    assert(sc64_z8_game_actor_at(0, (unsigned)egg, &egg_view) == 1);
    assert(egg_view.completed && egg_view.raw_type == 41);
#endif

#ifdef SC64_TRANSPORT_RUNTIME
    /* Cargo remains in Command census, but cannot satisfy a location-based
     * Bring condition. This follows the existing game world-provider rule. */
    unsigned cargo_slot = unit_slots[0];
    u[cargo_slot].order = ORDER_TRANSPORTED;
    Sc64Z8Condition cargo = {
        .player = u[cargo_slot].raw_owner, .unit = unit_raw[0],
        .opcode = 2, .comparison = 1
    };
    count = 0;
    assert(sc64_z8_world_count(&world, &cargo, 0, 0, &count) == 1);
    assert(count == 1);
    cargo.opcode = 3;
    assert(sc64_z8_world_count(&world, &cargo, 0, &world_rect, &count) == 1);
    assert(count == 0);
#endif

    /* Opcode 43 updates the actual per-slot invincibility byte, not flags. */
    unsigned selected_slot = unit_slots[6]; /* raw Probe 64 */
    u[selected_slot].raw_owner = 2;
    Sc64Z8Action invincible = {
        .group1 = 2, .unit = 64, .opcode = 43, .modifier = 4, .location = 1
    };
    assert(sc64_z8_world_action(&world, &invincible, 0, &world_rect) == 1);
    assert(invincible_units[selected_slot] == 1);
    assert(u[selected_slot].flags == 0);
    invincible.modifier = 5;
    assert(sc64_z8_world_action(&world, &invincible, 0, &world_rect) == 1);
    assert(invincible_units[selected_slot] == 0);

    /* Source owners 1 and 2 can share a live engine side but retain separate
     * current-resource slots when source actions execute. */
    minerals[1] = 10; minerals[2] = 100;
    gas[1] = 20; gas[2] = 200;
    sc64_z8_game_bind_source_wallets(&world, minerals, gas);
    Sc64Z8Action resource = {
        .group1 = 1, .group2 = 7, .unit = SC64_RESOURCE_MINERALS,
        .opcode = 26, .modifier = SC64_RESOURCE_MOD_ADD
    };
    assert(sc64_z8_world_action(&world, &resource, 0, 0) == 1);
    resource.group1 = 2; resource.group2 = 11;
    assert(sc64_z8_world_action(&world, &resource, 0, 0) == 1);
    assert(minerals[1] == 17 && minerals[2] == 111);
    assert(gas[1] == 20 && gas[2] == 200);

    puts("sc64_z8_game_bridge: actual pools, native extents, invincibility and source wallets passed");
    return 0;
}
