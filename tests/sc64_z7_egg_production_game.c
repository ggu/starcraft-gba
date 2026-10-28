/* Actual-engine Z7 independent Egg production and birth fixture. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z7_EGG_PRODUCTION
#error "Compile this fixture with SC64_Z7_EGG_PRODUCTION"
#endif

static int hatchery;

static void reset_world(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(geysers, 0, sizeof(geysers));
    race_mode = 0;
    race_human = R_ZERG;
    race_selected_mission = SC64_Z7_GLOBAL_ID;
    race_player[SIDE] = R_ZERG;
    race_player[ENEMY] = R_ZERG;
    race_player[NEUTRAL] = R_ZERG;
    race_player[RESCUABLE] = R_ZERG;
    race_player[4] = R_ZERG;
    race_z7_map = &sc64_race_maps[SC64_Z7_GLOBAL_ID];
    minerals = 5000;
    gas = 5000;
    supply_used = 0;
    supply_cap = 0;
    hatchery = spawn_build(SIDE, B_HATCHERY, 160, 160, 0);
    assert(hatchery >= 0 && building_complete(hatchery));
    b[hatchery].larva = 3;
    assert(spawn_build(SIDE, B_SPAWNING_POOL, 224, 160, 0) >= 0);
    assert(spawn_build(SIDE, B_HYDRALISK_DEN, 288, 160, 0) >= 0);
    assert(spawn_unit(SIDE, U_OVERLORD, 96, 96) >= 0);
    assert(race_z7_active());
}

static int find_egg(int product)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_Z7_EGG && u[i].target == product)
            return i;
    return -1;
}

static int count_type(int type)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        n += u[i].alive && u[i].type == type;
    return n;
}

/* Exercise the same per-frame order hook used by game_update(), followed by
 * the ordinary owner supply/larva refresh. */
static void step_world(void)
{
    update_orders();
    race_tick_state();
}

static void test_independent_jobs_and_hatch(void)
{
    reset_world();
    assert(race_queue_unit(SIDE, hatchery, U_DRONE));
    assert(race_queue_unit(SIDE, hatchery, U_ZERGLING));
    assert(race_queue_unit(SIDE, hatchery, U_HYDRALISK));
    assert(b[hatchery].queue_count == 0 && b[hatchery].larva == 0);
    assert(count_type(U_Z7_EGG) == 3);
    assert(supply_used == 6); /* Drone 2 + pair of Lings 2 + Hydra 2. */

    int drone_egg = find_egg(U_DRONE);
    int ling_egg = find_egg(U_ZERGLING);
    int hydra_egg = find_egg(U_HYDRALISK);
    assert(drone_egg >= 0 && ling_egg >= 0 && hydra_egg >= 0);
    assert(u[drone_egg].raw_type == 36 && u[drone_egg].armor == 10 &&
           u[drone_egg].maxhp == 200 && u[drone_egg].work_timer == 375);
    assert(u[ling_egg].work_timer == 525 && u[hydra_egg].work_timer == 525);

    /* Egg damage scales into the morph as the source's unit-type reinit does. */
    hit_target(drone_egg, 80, U_MARINE, ENEMY);
    assert(u[drone_egg].hp == 130);
    for (int t = 0; t < 375; ++t) step_world();
    assert(count_type(U_DRONE) == 1 && count_type(U_Z7_EGG) == 2);
    assert(u[drone_egg].hp == 26 && u[drone_egg].maxhp == 40);
    assert(u[ling_egg].work_timer == 150 && u[hydra_egg].work_timer == 150);
    assert(supply_used == 6);
    for (int t = 0; t < 150; ++t) step_world();
    assert(count_type(U_ZERGLING) == 2 && count_type(U_HYDRALISK) == 1);
    assert(count_type(U_Z7_EGG) == 0 && supply_used == 6);
}

static void test_more_than_three_live_eggs(void)
{
    reset_world();
    assert(spawn_unit(SIDE, U_OVERLORD, 104, 104) >= 0);
    assert(b[hatchery].larva == 3);
    for (int n = 0; n < 3; ++n) assert(race_queue_unit(SIDE, hatchery, U_QUEEN));
    assert(count_type(U_Z7_EGG) == 3);
    for (int t = 0; t < 856; ++t) step_world();
    assert(b[hatchery].larva == 2);
    assert(race_queue_unit(SIDE, hatchery, U_QUEEN));
    assert(race_queue_unit(SIDE, hatchery, U_QUEEN));
    /* Queen eggs last longer than two larva regeneration intervals. */
    assert(count_type(U_Z7_EGG) == 5);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_Z7_EGG)
            assert(u[i].work_timer > 0 && u[i].target == U_QUEEN);
}

static void test_egg_jobs_and_larva_replenishment_during_morph(void)
{
    reset_world();
    b[hatchery].larva = 1;
    b[hatchery].larva_timer = 427;
    int old_minerals = minerals, old_gas = gas;
    assert(race_start_lair_morph(hatchery));
    assert(b[hatchery].type == B_LAIR && b[hatchery].under_construction);
    old_minerals = minerals; old_gas = gas;
    assert(race_queue_unit(SIDE, hatchery, U_DRONE));
    int egg = find_egg(U_DRONE);
    assert(egg >= 0 && u[egg].target == U_DRONE &&
           u[egg].work_timer == sc64_z7_egg_duration(U_DRONE));
    assert(minerals == old_minerals - sc64_race_unit_rule(U_DRONE)->minerals);
    assert(gas == old_gas - sc64_race_unit_rule(U_DRONE)->gas);
    assert(b[hatchery].larva == 0 && b[hatchery].queue_count == 0);
    assert(supply_used == sc64_z7_egg_reserved_supply(U_DRONE));

    /* Preserve the documented adapter cadence: an existing morphing hall
     * keeps replenishing larvae, and the paid Egg completes independently. */
    race_tick_state();
    assert(b[hatchery].larva == 1 && b[hatchery].larva_timer == 0);
    for (int t = 0; t < sc64_z7_egg_duration(U_DRONE); ++t) step_world();
    assert(count_type(U_DRONE) == 1 && count_type(U_Z7_EGG) == 0);
    assert(b[hatchery].type == B_LAIR && b[hatchery].under_construction);

    /* The live cap keeps the current timer paused while the morph continues. */
    b[hatchery].larva = 3;
    b[hatchery].larva_timer = 427;
    race_tick_state();
    assert(b[hatchery].larva == 3 && b[hatchery].larva_timer == 427);

    /* An unfinished newly constructed Hatchery is not a morphing hall. */
    reset_world();
    b[hatchery].under_construction = 1;
    b[hatchery].build_progress = 0;
    b[hatchery].larva = 2;
    b[hatchery].larva_timer = 97;
    old_minerals = minerals; old_gas = gas;
    assert(!race_queue_unit(SIDE, hatchery, U_DRONE));
    assert(!race_queue_unit(SIDE, -1, U_DRONE));
    assert(minerals == old_minerals && gas == old_gas &&
           b[hatchery].larva == 2 && count_type(U_Z7_EGG) == 0);
    race_tick_state();
    assert(b[hatchery].larva == 2 && b[hatchery].larva_timer == 97);

    /* The incomplete-hall exception is Z7 Egg-only and scenario-gated. */
    reset_world();
    b[hatchery].type = B_LAIR;
    b[hatchery].raw_type = sc64_race_raw_building(B_LAIR);
    b[hatchery].under_construction = 1;
    b[hatchery].larva = 1;
    b[hatchery].larva_timer = 81;
    race_selected_mission = 0;
    old_minerals = minerals; old_gas = gas;
    assert(!race_queue_unit(SIDE, hatchery, U_DRONE));
    race_tick_state();
    assert(minerals == old_minerals && gas == old_gas &&
           b[hatchery].larva == 1 && b[hatchery].larva_timer == 81);

    /* The same scoped exception covers the other morph target, Hive. */
    race_selected_mission = SC64_Z7_GLOBAL_ID;
    b[hatchery].type = B_HIVE;
    b[hatchery].raw_type = sc64_race_raw_building(B_HIVE);
    b[hatchery].larva = 1;
    assert(race_queue_unit(SIDE, hatchery, U_DRONE));
    assert(b[hatchery].larva == 0 && find_egg(U_DRONE) >= 0);

    /* Incomplete halls still cannot accept a non-Egg producer proxy. */
    int enemy_hall = spawn_build(ENEMY, B_LAIR, 360, 160, 1);
    assert(enemy_hall >= 0);
    b[enemy_hall].larva = 1;
    int enemy_ore = race_minerals[ENEMY], enemy_gas = race_gas[ENEMY];
    assert(!race_queue_unit(ENEMY, enemy_hall, U_DRONE));
    assert(race_minerals[ENEMY] == enemy_ore && race_gas[ENEMY] == enemy_gas &&
           b[enemy_hall].larva == 1);
}

static void test_zerg_build_prerequisites_and_extractor_during_morph(void)
{
    reset_world();
    assert(race_owner_has_prereq(SIDE, SC_PR_HATCHERY));
    assert(race_start_lair_morph(hatchery));
    assert(b[hatchery].type == B_LAIR && b[hatchery].under_construction);

    /* Native Pool/Evolution tier sees an existing Lair morph as its source
     * Hatchery, while Spire's Lair tier is not complete yet. */
    assert(race_owner_has_prereq(SIDE, SC_PR_HATCHERY));
    assert(race_can_site(SIDE, B_SPAWNING_POOL, 300, 160));
    assert(race_can_site(SIDE, B_EVOLUTION_CHAMBER, 300, 160));
    assert(!race_owner_has_prereq(SIDE, SC_PR_LAIR));
    assert(!race_can_site(SIDE, B_SPIRE, 300, 160));
    assert(race_site_creep(SIDE, 300, 160)); /* 140px from morphing hall. */
    assert(!race_site_creep(SIDE, 330, 160)); /* 170px exceeds the proxy. */

    /* A Hive morph retains source Lair tech, and therefore may satisfy Spire. */
    b[hatchery].type = B_HIVE;
    b[hatchery].raw_type = sc64_race_raw_building(B_HIVE);
    assert(race_owner_has_prereq(SIDE, SC_PR_HATCHERY));
    assert(race_owner_has_prereq(SIDE, SC_PR_LAIR));

    /* A newly built incomplete Hatchery is neither a tech prerequisite nor
     * the adapter's completed-hall creep proxy. */
    reset_world();
    b[hatchery].under_construction = 1;
    assert(!race_owner_has_prereq(SIDE, SC_PR_HATCHERY));
    assert(!race_site_creep(SIDE, 300, 160));
    assert(!race_can_site(SIDE, B_SPAWNING_POOL, 300, 160));

    /* Extractor is Drone+geyser only in Brood War. It remains paid and
     * keeps the actual geyser-placement / worker-order path during morph. */
    reset_world();
    assert(race_start_lair_morph(hatchery));
    geysers[0] = (Geyser){ 400, 400 };
    geyser_count = 1;
    int drone = spawn_unit(SIDE, U_DRONE, 96, 96);
    assert(drone >= 0);
    int before_minerals = minerals, before_gas = gas;
    assert(race_start_build(SIDE, drone, B_EXTRACTOR, 400, 400));
    assert(u[drone].order == ORDER_BUILD_PENDING && u[drone].target == B_EXTRACTOR);
    assert(minerals == before_minerals && gas == before_gas);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        assert(!b[i].alive || b[i].type != B_EXTRACTOR ||
               b[i].x != 400 || b[i].y != 400);
    /* Advance the actual pending-order service at the authored geyser site. */
    u[drone].x = 400; u[drone].y = 400;
    update_orders();
    int extractor = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_EXTRACTOR &&
            b[i].x == 400 && b[i].y == 400) { extractor = i; break; }
    assert(extractor >= 0 && b[extractor].type == B_EXTRACTOR &&
           b[extractor].under_construction && b[extractor].build_progress == 0);
    assert(!u[drone].alive && minerals == before_minerals - 50 &&
           gas == before_gas);

    /* No town hall is required for a valid Drone and geyser in active Z7. */
    reset_world();
    memset(b, 0, sizeof(b));
    memset(u, 0, sizeof(u));
    geysers[0] = (Geyser){ 400, 400 };
    geyser_count = 1;
    drone = spawn_unit(SIDE, U_DRONE, 96, 96);
    assert(drone >= 0 && !race_owner_has_prereq(SIDE, SC_PR_HATCHERY));
    before_minerals = minerals; before_gas = gas;
    assert(race_start_build(SIDE, drone, B_EXTRACTOR, 400, 400));
    assert(u[drone].order == ORDER_BUILD_PENDING && u[drone].target == B_EXTRACTOR);
    assert(minerals == before_minerals && gas == before_gas);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        assert(!b[i].alive || b[i].type != B_EXTRACTOR ||
               b[i].x != 400 || b[i].y != 400);

    /* Other missions keep the existing table prereq even with the same
     * incomplete-morph and valid-site setup. */
    reset_world();
    assert(race_start_lair_morph(hatchery));
    race_selected_mission = 0;
    geysers[0] = (Geyser){ 400, 400 };
    geyser_count = 1;
    drone = spawn_unit(SIDE, U_DRONE, 96, 96);
    assert(drone >= 0);
    before_minerals = minerals; before_gas = gas;
    assert(!race_start_build(SIDE, drone, B_EXTRACTOR, 400, 400));
    assert(minerals == before_minerals && gas == before_gas &&
           u[drone].order == ORDER_IDLE);

    /* The Z7 exception removes only the artificial town-hall prerequisite;
     * the actual build path still requires a real geyser, a free site,
     * enough minerals, and this owner's Drone worker. */
    reset_world();
    drone = spawn_unit(SIDE, U_DRONE, 96, 96);
    assert(drone >= 0);
    before_minerals = minerals; before_gas = gas;
    assert(!race_start_build(SIDE, drone, B_EXTRACTOR, 400, 400)); /* no geyser */
    assert(minerals == before_minerals && gas == before_gas &&
           u[drone].order == ORDER_IDLE && u[drone].target == -1);

    reset_world();
    geysers[0] = (Geyser){ 400, 400 };
    geyser_count = 1;
    assert(spawn_build(SIDE, B_SPAWNING_POOL, 400, 400, 0) >= 0);
    drone = spawn_unit(SIDE, U_DRONE, 96, 96);
    assert(drone >= 0);
    before_minerals = minerals; before_gas = gas;
    assert(!race_start_build(SIDE, drone, B_EXTRACTOR, 400, 400)); /* occupied */
    assert(minerals == before_minerals && gas == before_gas &&
           u[drone].order == ORDER_IDLE && u[drone].target == -1);

    reset_world();
    geysers[0] = (Geyser){ 400, 400 };
    geyser_count = 1;
    drone = spawn_unit(SIDE, U_DRONE, 96, 96);
    assert(drone >= 0);
    minerals = 49;
    before_minerals = minerals; before_gas = gas;
    assert(!race_start_build(SIDE, drone, B_EXTRACTOR, 400, 400)); /* costs 50 */
    assert(minerals == before_minerals && gas == before_gas &&
           u[drone].order == ORDER_IDLE && u[drone].target == -1);

    reset_world();
    geysers[0] = (Geyser){ 400, 400 };
    geyser_count = 1;
    drone = spawn_unit(ENEMY, U_DRONE, 96, 96);
    assert(drone >= 0);
    before_minerals = minerals; before_gas = gas;
    assert(!race_start_build(SIDE, drone, B_EXTRACTOR, 400, 400)); /* wrong owner */
    assert(minerals == before_minerals && gas == before_gas &&
           u[drone].order == ORDER_IDLE && u[drone].target == -1);

    reset_world();
    geysers[0] = (Geyser){ 400, 400 };
    geyser_count = 1;
    drone = spawn_unit(SIDE, U_ZERGLING, 96, 96);
    assert(drone >= 0);
    before_minerals = minerals; before_gas = gas;
    assert(!race_start_build(SIDE, drone, B_EXTRACTOR, 400, 400)); /* wrong type */
    assert(minerals == before_minerals && gas == before_gas &&
           u[drone].order == ORDER_IDLE && u[drone].target == -1);
}

static void test_producer_loss_and_cancel_refund(void)
{
    reset_world();
    int before_minerals = minerals;
    int before_gas = gas;
    assert(race_queue_unit(SIDE, hatchery, U_DRONE));
    int egg = find_egg(U_DRONE);
    assert(egg >= 0);
    b[hatchery].alive = 0;
    int replacement = spawn_build(SIDE, B_SPAWNING_POOL, 160, 160, 0);
    assert(replacement == hatchery); /* Producer slot reuse cannot own the Egg. */
    for (int t = 0; t < 375; ++t) step_world();
    assert(count_type(U_DRONE) == 1 && count_type(U_Z7_EGG) == 0);

    reset_world();
    before_minerals = minerals;
    before_gas = gas;
    assert(race_queue_unit(SIDE, hatchery, U_DRONE));
    egg = find_egg(U_DRONE);
    assert(egg >= 0);
    hit_target(egg, 80, U_MARINE, ENEMY);
    u[egg].selected = 1;
    assert(race_z7_egg_cancel_available());
    const char *labels[MENU_COUNT] = { 0 };
    assert(field_menu_count() == MENU_STOP + 2);
    assert(!strcmp(field_menu_base_label(MENU_STOP + 1, labels), "CANCEL EGG"));
    choose_command(MENU_STOP + 1);
    assert(!u[egg].alive && count_type(U_Z7_EGG) == 0);
    assert(minerals == before_minerals && gas == before_gas && supply_used == 0);
}

static void test_ling_second_birth_failure_is_not_retried(void)
{
    reset_world();
    b[hatchery].larva = 1;
    assert(race_queue_unit(SIDE, hatchery, U_ZERGLING));
    int egg = find_egg(U_ZERGLING);
    assert(egg >= 0 && supply_used == 2);
    while (find_unit_slot() >= 0)
        assert(spawn_unit(SIDE, U_OVERLORD, 64, 64) >= 0);
    for (int t = 0; t < 525; ++t) (void)race_z7_egg_tick_unit(egg);
    assert(u[egg].alive && u[egg].type == U_ZERGLING);
    assert(count_type(U_ZERGLING) == 1 && supply_used == 1);
    /* Free a slot after the one native birth attempt; there is no retry. */
    for (int i = MAX_UNITS - 1; i >= 0; --i)
        if (u[i].alive && u[i].type == U_OVERLORD) { u[i].alive = 0; break; }
    (void)race_z7_egg_tick_unit(egg);
    assert(count_type(U_ZERGLING) == 1 && supply_used == 1);
}

static void test_scourge_pair_birth(void)
{
    reset_world();
    assert(sc64_z7_egg_product_count(U_SCOURGE) == 2);
    assert(race_queue_unit(SIDE, hatchery, U_SCOURGE));
    int egg = find_egg(U_SCOURGE);
    assert(egg >= 0 && u[egg].work_timer == sc64_z7_egg_duration(U_SCOURGE));
    assert(supply_used == 2); /* Both one-supply Scourges reserve supply. */
    for (int t = 0; t < sc64_z7_egg_duration(U_SCOURGE); ++t) step_world();
    assert(count_type(U_SCOURGE) == 2 && count_type(U_Z7_EGG) == 0);
    assert(supply_used == 2);
}

static void test_actor_facts_and_target_priority(void)
{
    reset_world();
    const Sc64ZpActorFact *fact = sc64_zp_actor_fact(U_Z7_EGG);
    assert(fact && fact->raw == 36 && fact->hp == 200 &&
           fact->armor == 10 && fact->sight_px == 128);
    assert(fact->left == 16 && fact->up == 16 &&
           fact->right == 15 && fact->down == 15);
    assert(!(fact->flags & SC64_ZP_ACTOR_MOVABLE));
    assert(!(fact->flags & SC64_ZP_ACTOR_WEAPON));
    assert(sc64_game_unit_collision_radius(U_Z7_EGG) == 8);
    assert(sc64_zp_actor_sight_source_pixels(U_Z7_EGG) == 128);
    assert(sc64_race_unit_damage_vs(U_Z7_EGG, 0) == 0);
    assert(sc64_race_unit_damage_vs(U_Z7_EGG, 1) == 0);
    assert(sc64_race_unit_hits_vs(U_Z7_EGG, 0) == 0);
    assert(sc64_race_unit_hits_vs(U_Z7_EGG, 1) == 0);

    assert(race_queue_unit(SIDE, hatchery, U_DRONE));
    int egg = find_egg(U_DRONE);
    assert(egg >= 0);
    int old_x = u[egg].x, old_y = u[egg].y;
    int old_timer = u[egg].work_timer;
    u[egg].selected = 1;
    issue_order(ORDER_MOVE, old_x + 60, old_y + 60);
    assert(u[egg].x == old_x && u[egg].y == old_y);
    assert(u[egg].work_timer == old_timer && u[egg].target == U_DRONE);
    choose_command(MENU_STOP);
    assert(u[egg].work_timer == old_timer && u[egg].target == U_DRONE &&
           supply_used == 2);
    u[egg].selected = 0;

    /* Adult eligible targets outrank an Egg even when the Egg is closer;
       manual targeting and fallback acquisition still see the Egg. */
    race_mode = 1;
    int enemy_egg = spawn_unit(ENEMY, U_Z7_EGG, old_x + 4, old_y);
    int adult = spawn_unit(ENEMY, U_MARINE, old_x + 36, old_y);
    assert(enemy_egg >= 0 && adult >= 0);
    u[enemy_egg].target = U_DRONE;
    u[enemy_egg].work_timer = 100;
    u[enemy_egg].raw_type = 36;
    u[enemy_egg].maxhp = u[enemy_egg].hp = 200;
    u[enemy_egg].armor = 10;
    target_lists_valid = 0;
    assert(nearest_enemy_typed(old_x, old_y, SIDE, 96, U_MARINE) == adult);
    int adult_hp = u[adult].hp;
    tick_combat(&u[enemy_egg], adult);
    assert(u[adult].hp == adult_hp);
    u[adult].alive = 0;
    assert(nearest_enemy_typed(old_x, old_y, SIDE, 96, U_MARINE) == enemy_egg);
    /* Manual attack remains possible: Egg is damageable, despite having no
       weapon of its own and low automatic target priority. */
    int egg_hp = u[enemy_egg].hp;
    hit_target(enemy_egg, 20, U_MARINE, SIDE);
    assert(u[enemy_egg].hp < egg_hp);
}

int main(void)
{
    test_independent_jobs_and_hatch();
    test_more_than_three_live_eggs();
    test_egg_jobs_and_larva_replenishment_during_morph();
    test_zerg_build_prerequisites_and_extractor_during_morph();
    test_producer_loss_and_cancel_refund();
    test_ling_second_birth_failure_is_not_retried();
    test_scourge_pair_birth();
    test_actor_facts_and_target_priority();
    puts("Z7 Egg production: independent timers, supply, damage, cancellation, producer loss, paired-birth fallback, and actor rules passed");
    return 0;
}
