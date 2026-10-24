/* Z7 Egg save/load tests use the real game save transaction and normal menu. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z7_EGG_PRODUCTION
#error "Compile this fixture with SC64_Z7_EGG_PRODUCTION"
#endif

static unsigned held_keys;

static void press(unsigned key)
{
    game_update((u16)key, (u16)(key & ~held_keys));
    held_keys = key;
    game_update(0, 0);
    held_keys = 0;
}

static void launch_z7(void)
{
    int ordinal;
    game_init();
    held_keys = 0;
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,
                                            SC64_Z7_GLOBAL_ID);
    assert(ordinal == 6 && race_front.page == SC_FRONT_HOME);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_Z7_GLOBAL_ID);
    press(KEY_A);
    assert(screen == SCREEN_PLAY && race_z7_active() &&
           race_campaign.mission == SC64_Z7_GLOBAL_ID);
    assert(SC64_RACE_SAVE_GENERATION == 36u);
}

static int add_source_hatchery(int owner, int raw_owner)
{
    int slot = spawn_build(owner, B_HATCHERY, owner == SIDE ? 420 : 1810,
                           owner == SIDE ? 420 : 1820, 0);
    assert(slot >= 0);
    b[slot].raw_owner = raw_owner;
    b[slot].raw_type = 131;
    b[slot].larva = 1;
    return slot;
}

static int find_egg(int owner, int product)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_Z7_EGG && u[i].owner == owner &&
            u[i].target == product) return i;
    return -1;
}

static void make_egg(int owner, int producer, int product)
{
    int slot;
    assert(race_queue_unit(owner, producer, product));
    slot = find_egg(owner, product);
    assert(slot >= 0 && u[slot].raw_type == 36 &&
           u[slot].raw_owner == (owner == SIDE ? 3 : 4));
    assert(u[slot].work_timer == sc64_z7_egg_duration(product));
}

static void encode_unit(RaceSave *save, unsigned slot, const Unit *q)
{
    assert(sc64_zp_unit_save_encode(&save->units[slot], q,
            SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u));
}

static void candidate_side_supply(const RaceSave *save, int *used, int *cap)
{
    int supply_used = 0, supply_cap = 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit q;
        assert(race_save_decode_unit(save, i, SC64_Z7_MAP_COLS * 16u,
                                     SC64_Z7_MAP_ROWS * 16u, &q));
        if (q.alive && q.owner == SIDE) {
            supply_used += unit_slot_supply(&q);
            supply_cap += sc64_race_unit_supply_provided(q.type);
        }
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building q;
        assert(race_save_decode_building(save, i, &q));
        if (q.alive && q.owner == SIDE && !q.under_construction) {
            const Sc64RaceBuildingRule *rule = sc64_race_building_rule(q.type);
            if (rule) supply_cap += rule->supply_provided;
        }
    }
    *used = supply_used;
    *cap = supply_cap;
}

static unsigned candidate_live_units(const RaceSave *save)
{
    unsigned count = 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit q;
        assert(race_save_decode_unit(save, i, SC64_Z7_MAP_COLS * 16u,
                                     SC64_Z7_MAP_ROWS * 16u, &q));
        count += q.alive != 0;
    }
    return count;
}

static void candidate_fill_human_lings(RaceSave *save, unsigned target)
{
    Unit ling = {0};
    unsigned live = candidate_live_units(save);
    const Sc64RaceUnitRule *rule = sc64_race_unit_rule(U_ZERGLING);
    assert(rule && live <= target && target <= MAX_UNITS);
    ling.type = U_ZERGLING;
    ling.owner = SIDE;
    ling.raw_type = ling.raw_owner = -1;
    ling.alive = 1;
    ling.hp = ling.maxhp = rule->hp;
    ling.armor = sc64_race_unit_armor(U_ZERGLING);
    ling.order = ORDER_IDLE;
    ling.target = -1;
    ling.build = -1;
    ling.x = ling.tx = 400;
    ling.y = ling.ty = 400;
    while (live < target) {
        int placed = 0;
        for (unsigned i = 0; i < MAX_UNITS; ++i) {
            Unit old;
            assert(race_save_decode_unit(save, i, SC64_Z7_MAP_COLS * 16u,
                                         SC64_Z7_MAP_ROWS * 16u, &old));
            if (old.alive) continue;
            assert(sc64_zp_unit_save_encode(&save->units[i], &ling,
                    SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u));
            ++live;
            placed = 1;
            break;
        }
        assert(placed);
    }
    assert(live == target);
}

static void assert_rejected_without_live_mutation(const RaceSave *bad)
{
    Unit old_units[MAX_UNITS];
    Building old_buildings[MAX_BUILDINGS];
    Sc64Z7PersistentState old_z7 = race_z7;
    Sc64RaceCampaignState old_campaign = race_campaign;
    int old_ore[5], old_gas[5], old_used = supply_used, old_cap = supply_cap;
    int old_frame = frame;
    memcpy(old_units, u, sizeof(u));
    memcpy(old_buildings, b, sizeof(b));
    memcpy(old_ore, race_minerals, sizeof(old_ore));
    memcpy(old_gas, race_gas, sizeof(old_gas));
    save_write(bad, sizeof(*bad), SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(old_units, u, sizeof(u)));
    assert(!memcmp(old_buildings, b, sizeof(b)));
    assert(!memcmp(&old_z7, &race_z7, sizeof(old_z7)));
    assert(!memcmp(&old_campaign, &race_campaign, sizeof(old_campaign)));
    assert(!memcmp(old_ore, race_minerals, sizeof(old_ore)));
    assert(!memcmp(old_gas, race_gas, sizeof(old_gas)));
    assert(supply_used == old_used && supply_cap == old_cap && frame == old_frame);
}

static void test_human_egg_roundtrip_and_corruption(void)
{
    RaceSave good, bad, unchanged;
    int producer, egg;
    launch_z7();
    producer = add_source_hatchery(SIDE, 3);
    {
        int pool = spawn_build(SIDE, B_SPAWNING_POOL, 470, 420, 0);
        assert(pool >= 0);
        b[pool].raw_owner = 3;
        b[pool].raw_type = 142;
    }
    race_minerals[SIDE] = minerals = 1000;
    race_gas[SIDE] = gas = 1000;
    make_egg(SIDE, producer, U_DRONE);
    egg = find_egg(SIDE, U_DRONE);
    assert(egg >= 0);
    b[producer].larva = 1;
    make_egg(SIDE, producer, U_ZERGLING);
    for (int i = 0; i < 17; ++i) game_update(0, 0);
    assert(u[egg].work_timer == sc64_z7_egg_duration(U_DRONE) - 17);
    assert(race_save_game());
    good = race_data;
    unchanged = good;
    assert(race_z7_save_candidate_valid(&good, &good.z7,
                                        SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    assert(!memcmp(&unchanged, &good, sizeof(good)));
    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game());
    assert(u[egg].alive && u[egg].type == U_Z7_EGG &&
           u[egg].target == U_DRONE && u[egg].work_timer ==
           sc64_z7_egg_duration(U_DRONE) - 17);
    assert(race_owner_supply(SIDE) == supply_used);

    bad = good;
    {
        Unit q;
        assert(race_save_decode_unit(&bad, (unsigned)egg,
                SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u, &q));
        q.target = U_ZERGLING + 100; /* Not an authenticated larva product. */
        encode_unit(&bad, (unsigned)egg, &q);
    }
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    unchanged = bad;
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    assert(!memcmp(&unchanged, &bad, sizeof(bad)));
    assert_rejected_without_live_mutation(&bad);

    for (int field = 0; field < 2; ++field) {
        Unit q;
        bad = good;
        assert(race_save_decode_unit(&bad, (unsigned)egg,
                SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u, &q));
        if (field == 0) ++q.armor;
        else q.shield = 1;
        encode_unit(&bad, (unsigned)egg, &q);
        assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
        assert_rejected_without_live_mutation(&bad);
    }

    bad = good;
    {
        Unit q;
        assert(race_save_decode_unit(&bad, (unsigned)egg,
                SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u, &q));
        q.work_timer = (u16)(sc64_z7_egg_duration(q.target) + 1);
        encode_unit(&bad, (unsigned)egg, &q);
    }
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    assert_rejected_without_live_mutation(&bad);

    bad = good;
    {
        Unit q;
        assert(race_save_decode_unit(&bad, (unsigned)egg,
                SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u, &q));
        q.raw_owner = 4;
        encode_unit(&bad, (unsigned)egg, &q);
    }
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    assert_rejected_without_live_mutation(&bad);

    bad = good;
    {
        int changed = 0;
        for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
            Building q;
            assert(race_save_decode_building(&bad, i, &q));
            if (!q.alive || q.owner != SIDE || q.raw_owner != 3 ||
                (q.type != B_HATCHERY && q.type != B_LAIR && q.type != B_HIVE)) continue;
            q.queue_head = 0; q.queue_count = 1; q.queue_progress = 0;
            memset(q.queue, 0, sizeof(q.queue)); q.queue[0] = U_DRONE;
            assert(sc64_building_save_encode(&bad.buildings[i], &q,
                    SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u));
            changed = 1;
            break;
        }
        assert(changed);
    }
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
}

static void test_enemy_ling_egg_and_product_bound(void)
{
    RaceSave good, bad, unchanged;
    int producer = -1, egg;
    launch_z7();
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY && b[i].raw_owner == 4 &&
            (b[i].type == B_HATCHERY || b[i].type == B_LAIR || b[i].type == B_HIVE)) {
            producer = i;
            break;
        }
    assert(producer >= 0);
    b[producer].larva = 1;
    race_minerals[ENEMY] = 1000;
    race_gas[ENEMY] = 1000;
    race_z7.runtime.model.started |= (u16)(1u << SC64_Z7_TRIGGER_AI);
    race_z7.runtime.model.action_cursor[SC64_Z7_TRIGGER_AI] = 0;
    race_z7.runtime.ai_script_mask = 15;
    race_z7.runtime.ai_plan_ticks = SC64_Z7_AI_PERIOD_TICKS;
    make_egg(ENEMY, producer, U_ZERGLING);
    egg = find_egg(ENEMY, U_ZERGLING);
    assert(egg >= 0 && race_save_game());
    good = race_data;
    unchanged = good;
    assert(race_z7_save_candidate_valid(&good, &good.z7,
                                        SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    assert(!memcmp(&unchanged, &good, sizeof(good)));

    /* The live Egg already occupies one actor slot. Filling all remaining
     * slots still leaves a valid save; its paired Ling may simply fail to
     * create the second output at hatch time without an extra reservation. */
    bad = good;
    candidate_fill_human_lings(&bad, MAX_UNITS);
    assert(candidate_live_units(&bad) == MAX_UNITS);
    assert(race_z7_save_candidate_valid(&bad, &bad.z7,
                                        SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));

    /* Losing supply providers after an Egg has started may leave the saved
     * army over cap. The validator must preserve that legitimate state. */
    bad = good;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit q;
        assert(race_save_decode_unit(&bad, i, SC64_Z7_MAP_COLS * 16u,
                                     SC64_Z7_MAP_ROWS * 16u, &q));
        if (!q.alive || q.owner != SIDE || q.raw_owner != 3 ||
            q.type != U_OVERLORD) continue;
        q.alive = 0;
        assert(sc64_zp_unit_save_encode(&bad.units[i], &q,
                SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u));
    }
    {
        int used, cap;
        candidate_side_supply(&bad, &used, &cap);
        assert(used > cap);
    }
    assert(race_z7_save_candidate_valid(&bad, &bad.z7,
                                        SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));

    bad = good;
    {
        Unit q;
        assert(race_save_decode_unit(&bad, (unsigned)egg,
                SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u, &q));
        q.target = U_DRONE;
        encode_unit(&bad, (unsigned)egg, &q);
    }
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    unchanged = bad;
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
    assert(!memcmp(&unchanged, &bad, sizeof(bad)));
    assert_rejected_without_live_mutation(&bad);

    /* At the initial AI pulse budget one live Egg is one future Ling pair.
     * A second live Egg would exceed the same bounded authored obligation. */
    bad = good;
    bad.state[7] = 0;
    bad.campaign.elapsed_frames = 0;
    {
        Unit q;
        unsigned free_slot = MAX_UNITS;
        assert(race_save_decode_unit(&good, (unsigned)egg,
                SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u, &q));
        for (unsigned i = 0; i < MAX_UNITS; ++i) {
            Unit old;
            assert(race_save_decode_unit(&good, i, SC64_Z7_MAP_COLS * 16u,
                                         SC64_Z7_MAP_ROWS * 16u, &old));
            if (!old.alive) { free_slot = i; break; }
        }
        assert(free_slot < MAX_UNITS);
        assert(sc64_zp_unit_save_encode(&bad.units[free_slot], &q,
                SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u));
    }
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));

    /* An enemy Egg is not a human objective witness. With all human actors
     * dead, a real FAILED payload is valid even though the Egg remains. */
    bad = good;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit q;
        assert(race_save_decode_unit(&bad, i, SC64_Z7_MAP_COLS * 16u,
                                     SC64_Z7_MAP_ROWS * 16u, &q));
        if (q.alive && q.owner == SIDE) {
            q.alive = 0;
            assert(sc64_zp_unit_save_encode(&bad.units[i], &q,
                    SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u));
        }
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building q;
        assert(race_save_decode_building(&bad, i, &q));
        if (q.alive && q.owner == SIDE) {
            q.alive = 0;
            assert(sc64_building_save_encode(&bad.buildings[i], &q,
                    SC64_Z7_MAP_COLS * 16u, SC64_Z7_MAP_ROWS * 16u));
        }
    }
    bad.z7.runtime.model.started |= (u16)(1u << SC64_Z7_TRIGGER_LOSS);
    bad.z7.runtime.model.action_cursor[SC64_Z7_TRIGGER_LOSS] = 0;
    bad.z7.runtime.model.outcome = SC64_Z7_LOST;
    bad.state[18] = 0;
    bad.state[19] = 1;
    bad.campaign.phase = SC64_RACE_PHASE_FAILED;
    bad.campaign.objective_flags &= ~SC64_RACE_OBJECTIVE_PRIMARY;
    assert(sc64_z7_persistent_state_valid(&bad.z7));
    assert(race_z7_save_candidate_valid(&bad, &bad.z7,
                                        SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));

    bad = good;
    bad.z7.runtime.ai_script_mask = 0;
    bad.z7.runtime.ai_plan_ticks = 0;
    bad.z7.runtime.model.started &= (u16)~(1u << SC64_Z7_TRIGGER_AI);
    assert(!race_z7_save_candidate_valid(&bad, &bad.z7,
                                         SC64_Z7_MAP_COLS, SC64_Z7_MAP_ROWS));
}

int main(void)
{
    test_human_egg_roundtrip_and_corruption();
    test_enemy_ling_egg_and_product_bound();
    puts("Z7 Egg save candidate, provenance, transactional load, and enemy Ling bounds passed");
    return 0;
}
