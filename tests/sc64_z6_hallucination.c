#include <assert.h>
#include <limits.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z6_runtime_adapter.h"
#include "../starcraft/sc64_z6_hallucination.h"
#include "../starcraft/sc64_zp_unit_save.h"

static unsigned spawn_source_tassadar(int owner, int raw_owner)
{
    int slot = spawn_unit(owner, U_TASSADAR, 300, 400);
    assert(slot >= 0);
    u[slot].raw_type = SC64_Z6_HALLUCINATION_SOURCE_RAW;
    u[slot].raw_owner = raw_owner;
    return (unsigned)slot;
}

static void init_test(void)
{
    race_mode = 0;
    race_selected_mission = SC64_Z6_GLOBAL_ID;
    race_cols = race_rows = SC64_Z6_MAP_COLS;
    race_z6_map = &sc64_race_maps[SC64_Z6_GLOBAL_ID];
    memset(u, 0, sizeof(u));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(&race_z6, 0, sizeof(race_z6));
    supply_used = 0;
    sc64_z6_runtime_init(&race_z6.runtime);
}

static void test_mark_identity_and_damage_stages(void)
{
    init_test();
    unsigned slot = spawn_source_tassadar(ENEMY, 2);
    assert(race_z6_hallucination_mark_slot(slot));
    assert(u[slot].flags & SC64_Z6_HALLUCINATION_FLAG);
    assert(u[slot].work_timer == SC64_Z6_HALLUCINATION_LIFETIME_TICKS);
    assert(race_z6_hallucination_actor(&u[slot]));
    assert(race_z6_hallucination_world_state_valid());
    assert(race_z6_hallucination_unit_state_valid(&u[slot], ENEMY));

    /* Source doubles the incoming weapon amount before shields and armor. */
    u[slot].shield = 7;
    u[slot].armor = 3;
    int incoming = race_z6_hallucination_incoming_damage((int)slot, 20);
    assert(incoming == 40);
    int after_shields = incoming - u[slot].shield;
    assert(after_shields == 33);
    int after_armor = after_shields - u[slot].armor;
    assert(after_armor == 30);
    assert(race_z6_hallucination_incoming_damage((int)slot, INT_MAX) == INT_MAX);
    assert(race_z6_hallucination_incoming_damage(-1, 20) == 20);
    assert(!race_z6_hallucination_can_deal_damage((int)slot));
    assert(race_z6_hallucination_can_deal_damage(-1));

    /* A different raw owner cannot acquire the source property. */
    unsigned invalid = spawn_source_tassadar(ENEMY, 1);
    assert(!race_z6_hallucination_mark_slot(invalid));
    assert(!(u[invalid].flags & SC64_Z6_HALLUCINATION_FLAG));
}

static void test_combat_death_cleanup_and_candidate_rows(void)
{
    init_test();
    unsigned slot = spawn_source_tassadar(ENEMY, 2);
    assert(race_z6_hallucination_mark_slot(slot));
    assert(!race_z6_hallucination_unit_state_valid(&u[slot], SIDE));
    u[slot].hp = 1;
    u[slot].armor = 0;
    hit_target((int)slot, 20000, U_MARINE, SIDE);
    assert(!u[slot].alive);
    /* The actual hit path owns source accounting and marker cleanup. */
    assert(!(u[slot].flags & SC64_Z6_HALLUCINATION_FLAG));
    assert(u[slot].work_timer == 0);
    assert(race_z6_hallucination_unit_state_valid(&u[slot], ENEMY));
    assert(race_z6_hallucination_world_state_valid());
    assert(race_z6_hallucination_tick(0)); /* Dead, cleaned row cannot fault. */
    assert(race_z6.deaths_by_source[2] == 1);

    /* Candidate-only validation rejects a forged raw79 row that drops the
       source marker while alive, or keeps it after ordinary death. */
    Unit forged = u[slot];
    forged.alive = 1;
    forged.work_timer = 1;
    assert(!race_z6_hallucination_unit_state_valid(&forged, ENEMY));
    forged.flags |= SC64_Z6_HALLUCINATION_FLAG;
    assert(race_z6_hallucination_unit_state_valid(&forged, ENEMY));
    forged.work_timer = SC64_Z6_HALLUCINATION_LIFETIME_TICKS + 1;
    assert(!race_z6_hallucination_unit_state_valid(&forged, ENEMY));
    forged.work_timer = 1;
    forged.alive = 0;
    assert(!race_z6_hallucination_unit_state_valid(&forged, ENEMY));
    forged.flags &= ~SC64_Z6_HALLUCINATION_FLAG;
    forged.work_timer = 0;
    assert(race_z6_hallucination_unit_state_valid(&forged, ENEMY));
}

static void test_actual_weapon_animation_does_no_damage(void)
{
    init_test();
    unsigned attacker = spawn_source_tassadar(ENEMY, 2);
    assert(race_z6_hallucination_mark_slot(attacker));
    int victim = spawn_unit(SIDE, U_ZEALOT, 310, 400);
    assert(victim >= 0);
    int hp = u[victim].hp, shields = u[victim].shield;
    assert(!u[attacker].cd);
    tick_combat(&u[attacker], victim);
    assert(u[attacker].cd > 0);
    assert(u[victim].hp == hp && u[victim].shield == shields);
}

static void test_lifetime_pause_expiration_and_death_accounting(void)
{
    init_test();
    unsigned slot = spawn_source_tassadar(ENEMY, 2);
    assert(race_z6_hallucination_mark_slot(slot));
    assert(race_z6_hallucination_tick(1));
    assert(u[slot].work_timer == SC64_Z6_HALLUCINATION_LIFETIME_TICKS);
    race_selected_mission = 16;
    assert(race_z6_hallucination_tick(0));
    assert(u[slot].work_timer == SC64_Z6_HALLUCINATION_LIFETIME_TICKS);
    race_selected_mission = SC64_Z6_GLOBAL_ID;
    for (unsigned i = 1; i < SC64_Z6_HALLUCINATION_LIFETIME_TICKS; ++i) {
        assert(race_z6_hallucination_tick(0));
        assert(u[slot].alive);
        assert(u[slot].work_timer ==
               SC64_Z6_HALLUCINATION_LIFETIME_TICKS - i);
    }
    assert(race_z6_hallucination_tick(0));
    assert(!u[slot].alive);
    assert(!(u[slot].flags & SC64_Z6_HALLUCINATION_FLAG));
    assert(u[slot].work_timer == 0);
    assert(race_z6.deaths_by_source[2] == 1);
    assert(supply_used == 0); /* P2 source owner is ENEMY, not SIDE. */
    assert(race_z6_hallucination_tick(0));
    assert(race_z6.deaths_by_source[2] == 1); /* Expiry is accounted once. */
}

static void test_timer_roundtrip_and_corrupt_state_rejection(void)
{
    init_test();
    unsigned slot = spawn_source_tassadar(ENEMY, 2);
    assert(race_z6_hallucination_mark_slot(slot));
    Unit before, after;
    Sc64ZpPackedUnitSave encoded;
    memset(&before, 0, sizeof(before));
    memset(&after, 0, sizeof(after));
    assert(sc64_zp_unit_save_encode(&encoded, &u[slot],
                                    SC64_Z6_MAP_COLS * 16u,
                                    SC64_Z6_MAP_ROWS * 16u));
    assert(sc64_zp_unit_save_decode(&after, &encoded,
                                    SC64_Z6_MAP_COLS * 16u,
                                    SC64_Z6_MAP_ROWS * 16u));
    assert(after.work_timer == SC64_Z6_HALLUCINATION_LIFETIME_TICKS);
    assert(after.flags & SC64_Z6_HALLUCINATION_FLAG);
    assert(after.raw_type == SC64_Z6_HALLUCINATION_SOURCE_RAW);
    assert(after.raw_owner == SC64_Z6_HALLUCINATION_SOURCE_OWNER);
    u[slot].work_timer = 0;
    assert(!race_z6_hallucination_world_state_valid());
}

int main(void)
{
    test_mark_identity_and_damage_stages();
    test_combat_death_cleanup_and_candidate_rows();
    test_actual_weapon_animation_does_no_damage();
    test_lifetime_pause_expiration_and_death_accounting();
    test_timer_roundtrip_and_corrupt_state_rejection();
    return 0;
}
