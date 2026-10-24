/* Actual engine lifecycle checks for optional construction HP progression. */
#include <assert.h>
#include "../starcraft/game.c"

#if !defined(SC64_CONSTRUCTION_HP) || !defined(SC64_RACES)
#error "Compile with race rules and SC64_CONSTRUCTION_HP"
#endif

static void reset_fixture(int race)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    /* Use the bounded synthetic skirmish terrain instead of an imported
       mission index; this fixture exercises construction, not map loading. */
    race_mode = 1;
    race_human = race;
    race_player[SIDE] = race;
    race_player[ENEMY] = R_TER;
    race_player[RESCUABLE] = R_ZERG;
    race_player[4] = R_PROTOSS;
    race_selected_mission = -1;
    screen = SCREEN_PLAY;
    minerals = gas = supply_used = supply_cap = 0;
}

static void test_terran_worker_pause_and_damage(void)
{
    reset_fixture(R_TER);
    int worker = spawn_unit(SIDE,U_SCV,256,256);
    int building = spawn_build(SIDE,B_BARRACKS,256,256,1);
    assert(worker >= 0 && building >= 0);
    assert(b[building].hp == sc64_construction_hp_start(b[building].maxhp));
    u[worker].order = ORDER_BUILD;
    u[worker].target = building;

    /* Far SCV: no progress and no HP gain while it walks to the foundation. */
    u[worker].x = 512; u[worker].y = 512;
    int initial_hp = b[building].hp;
    update_orders();
    assert(b[building].build_progress == 0 && b[building].hp == initial_hp);

    /* In work range the actual ORDER_BUILD branch advances both clock and HP. */
    u[worker].x = b[building].x; u[worker].y = b[building].y;
    assert(race_owner_identity(u[worker].owner) == R_TER);
    assert(u[worker].order == ORDER_BUILD && u[worker].target == building);
    assert(dist2(u[worker].x, u[worker].y, b[building].x, b[building].y) == 0);
    for (int i = 0; i < 8 && b[building].hp == initial_hp; ++i)
        update_orders();
    assert(b[building].build_progress > 0 && b[building].hp > initial_hp);
    int total = building_build_time(B_BARRACKS);
    int progress = b[building].build_progress;
    int working_hp = b[building].hp;
    u[worker].x = 512; u[worker].y = 512;
    update_orders();
    assert(b[building].build_progress == progress && b[building].hp == working_hp);
    u[worker].x = b[building].x; u[worker].y = b[building].y;
    int deficit = 25;
    b[building].hp -= deficit;
    for (int p = progress + 1; p <= total; ++p) update_orders();
    assert(!b[building].under_construction);
    assert(b[building].hp == b[building].maxhp - deficit);
    assert(!u[worker].alive || u[worker].order == ORDER_IDLE);
}

static void test_zerg_arrival_and_completion_damage(void)
{
    reset_fixture(R_ZERG);
    int building = spawn_build(SIDE,B_CREEP_COLONY,320,320,1);
    int worker = spawn_unit(SIDE,U_DRONE,320,320);
    assert(building >= 0 && worker >= 0);
    int initial_hp = b[building].hp;
    assert(initial_hp == sc64_construction_hp_start(b[building].maxhp));
    b[building].build_progress = -1;
    race_tick_buildings();
    assert(b[building].build_progress == -1 && b[building].hp == initial_hp);
    u[worker].order = ORDER_BUILD;
    u[worker].target = building;
    assert(race_builder_arrived(worker));
    assert(!u[worker].alive && b[building].build_progress == 0);
    int total = (sc64_race_building_rule(B_CREEP_COLONY)->build_frames * 5 + 3) / 4;
    for (int i = 0; i < 8 && b[building].hp == initial_hp; ++i)
        race_tick_buildings();
    assert(b[building].build_progress > 0 && b[building].hp > initial_hp);
    int progress = b[building].build_progress;
    int deficit = 17;
    b[building].hp -= deficit;
    for (int p = progress + 1; p <= total; ++p) race_tick_buildings();
    assert(!b[building].under_construction);
    assert(b[building].hp == b[building].maxhp - deficit);
}

static void test_protoss_hp_shields_and_passive_regen(void)
{
    reset_fixture(R_PROTOSS);
    int building = spawn_build(SIDE,B_GATEWAY,400,400,1);
    assert(building >= 0);
    int initial_hp = b[building].hp;
    int max_shields = sc64_race_building_shields(B_GATEWAY);
    int initial_shields = b[building].shield;
    assert(initial_hp == sc64_construction_hp_start(b[building].maxhp));
    assert(b[building].shield == sc64_construction_shield_start(max_shields));
    int total = (sc64_race_building_rule(B_GATEWAY)->build_frames * 5 + 3) / 4;
    for (int p = 1; p <= total - 2; ++p) {
        race_tick_buildings();
        race_tick_state();
        if (p == 46) {
            /* 46 ordinary recharge ticks cross the first 320/7 boundary.
               Verify recharge adds to construction while still incomplete. */
            int construction = (max_shields - initial_shields) * p / total;
            assert(b[building].under_construction);
            assert(b[building].shield == initial_shields + construction + 1);
        }
    }
    int deficit = 23;
    b[building].hp -= deficit;
    b[building].shield = (unsigned short)(max_shields - 20);
    b[building].shield_remainder = 0;
    for (int p = total - 1; p <= total; ++p) {
        race_tick_buildings();
        race_tick_state();
    }
    assert(!b[building].under_construction);
    assert(b[building].hp == b[building].maxhp - deficit);
    /* Final construction gains use the same cumulative integer schedule;
       passive converted regen remains independently active and capped. */
    int final_construction_shields =
        sc64_construction_shield_gain(max_shields,total-1,total) +
        sc64_construction_shield_gain(max_shields,total,total);
    assert(b[building].shield == max_shields - 20 +
                                      final_construction_shields);
}

#ifdef SC64_ZP_CAMPAIGN_STORAGE
static void test_lair_morph_damage_delta(void)
{
    reset_fixture(R_ZERG);
    minerals = gas = 2000;
    int hatch = spawn_build(SIDE, B_HATCHERY, 600, 600, 0);
    int pool = spawn_build(SIDE, B_SPAWNING_POOL, 680, 600, 0);
    assert(hatch >= 0 && pool >= 0 && building_complete(hatch) &&
           building_complete(pool));
    int old_max = b[hatch].maxhp;
    int damage = 200;
    b[hatch].hp = old_max - damage;
    assert(race_start_lair_morph(hatch));
    int new_max = b[hatch].maxhp;
    assert(b[hatch].type == B_LAIR && b[hatch].under_construction &&
           b[hatch].hp == old_max - damage);
    int total = (sc64_race_building_rule(B_LAIR)->build_frames * 5 + 3) / 4;
    for (int p = 1; p <= total; ++p) {
        race_tick_buildings();
        if (p < total) assert(b[hatch].hp == old_max - damage);
    }
    assert(building_complete(hatch));
    assert(b[hatch].maxhp == new_max && b[hatch].hp == new_max - damage);
}
#endif

int main(void)
{
    test_terran_worker_pause_and_damage();
    test_zerg_arrival_and_completion_damage();
    test_protoss_hp_shields_and_passive_regen();
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    test_lair_morph_damage_delta();
#endif
    return 0;
}
