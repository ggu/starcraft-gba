/* Actual engine integration fixtures for T5 mines and hero combat. Direct
 * world setup here is test scaffolding, not a gameplay/controller proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void start_t5(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active() && race_cols == 96 && race_rows == 96);
}

static void clear_world_fixture(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    target_lists_valid = 0;
    sc64_target_index_end();
    sc64_unit_buckets_end();
    supply_used = 0;
    weapons_level = armor_level = 0;
}

static Unit *unit_fixture(int slot, int type, int raw, int owner,
                          int raw_owner, int x, int y, int hp)
{
    Unit *q = &u[slot];
    memset(q, 0, sizeof(*q));
    q->alive = 1; q->type = type; q->raw_type = raw;
    q->owner = owner; q->raw_owner = raw_owner;
    q->x = x; q->y = y; q->hp = q->maxhp = hp;
    q->target = -1;
    return q;
}

/* Original full-scan oracle: apply the eligibility predicate before range
 * filtering, then use ascending-slot order and strict distance improvement. */
static int reference_mine_find_target(const Unit *mine)
{
    int selected = -1;
    const int radius = SC64_T5_MINE_ACQUIRE_SOURCE_PX;
    int best = 2 * radius * radius + 1;
    int mx, my;
    if (!mine) return -1;
    mx = mine->x * 2; my = mine->y * 2;
    for (int i = 0; i < MAX_UNITS; ++i) {
        int dx, dy, d;
        if (!sc64_t5_mine_unit_targetable(mine, &u[i])) continue;
        dx = u[i].x * 2 - mx; dy = u[i].y * 2 - my;
        if (dx < -radius || dx > radius || dy < -radius || dy > radius) continue;
        d = dx * dx + dy * dy;
        if (d < best) { best = d; selected = i; }
    }
    return selected;
}

static unsigned test_rng = 0x64c0ffeeu;
static unsigned test_random(void)
{
    test_rng ^= test_rng << 13;
    test_rng ^= test_rng >> 17;
    test_rng ^= test_rng << 5;
    return test_rng;
}

static void test_mine_acquisition_matches_full_scan(void)
{
    static const int types[] = { U_SCV, U_MARINE, U_VULTURE,
        U_RAYNOR_VULTURE, U_WRAITH, U_KERRIGAN, U_SPIDER_MINE };
    static const int raws[] = { 7, 0, 2, 19, 8, 16, 13 };
    static const int owners[] = { SIDE, ENEMY, RESCUABLE, NEUTRAL };
    Unit *mine;
    start_t5(); clear_world_fixture();
    mine = unit_fixture(0, U_SPIDER_MINE, 13, ENEMY, 6, 800, 800, 20);

    /* Inclusive square edges and strict tie policy, including square corners
     * whose squared distance exceeds radius squared. */
    unit_fixture(1, U_SCV, 7, SIDE, 0, 848, 800, 50);  /* +96 */
    unit_fixture(2, U_SCV, 7, SIDE, 0, 752, 800, 50);  /* -96 */
    unit_fixture(3, U_SCV, 7, SIDE, 0, 848, 848, 50);  /* corner */
    assert(sc64_t5_mine_find_target(mine) == reference_mine_find_target(mine));
    assert(sc64_t5_mine_find_target(mine) == 1);
    u[1].alive = 0; u[2].alive = 0;
    assert(sc64_t5_mine_find_target(mine) == 3);
    u[3].x = 849; /* one GBA pixel puts the corner outside (+98 source px). */
    assert(sc64_t5_mine_find_target(mine) == -1);
    assert(sc64_t5_mine_find_target(mine) == reference_mine_find_target(mine));

    /* Randomized bounded rosters exercise cheap range rejection before every
     * expensive eligibility branch, while comparing the selected slot with
     * the pre-optimization full-scan oracle. */
    for (int trial = 0; trial < 1200; ++trial) {
        clear_world_fixture();
        mine = unit_fixture(0, U_SPIDER_MINE, 13, ENEMY, 6,
                            400 + (int)(test_random() % 400),
                            400 + (int)(test_random() % 400), 20);
        for (int i = 1; i < MAX_UNITS; ++i) {
            unsigned r = test_random();
            int kind = (int)(r % (sizeof(types) / sizeof(types[0])));
            int dx = (int)(test_random() % 301) - 150;
            int dy = (int)(test_random() % 301) - 150;
            Unit *q = unit_fixture(i, types[kind], raws[kind],
                    owners[test_random() % (sizeof(owners) / sizeof(owners[0]))],
                    (int)(test_random() % 12), mine->x + dx / 2,
                    mine->y + dy / 2, 20 + (int)(test_random() % 200));
            q->alive = (test_random() & 3u) != 0;
            q->flags = (test_random() & 7u) == 0 ? 16 : 0;
            q->order = (test_random() & 15u) == 0 ? ORDER_TRANSPORTED : ORDER_IDLE;
            invincible_units[i] = (test_random() & 15u) == 0;
        }
        assert(sc64_t5_mine_find_target(mine) == reference_mine_find_target(mine));
    }
}

static void test_update_orders_preserves_mine_state(void)
{
    start_t5(); clear_world_fixture();
    Unit *mine = unit_fixture(0, U_SPIDER_MINE, 13, ENEMY, 6, 120, 120, 20);
    mine->carry = SC64_T5_MINE_CHASING;
    mine->work_timer = 29;
    mine->target = 7;
    mine->cd = 11;
    update_orders();
    assert(mine->alive && mine->carry == SC64_T5_MINE_CHASING);
    assert(mine->work_timer == 29 && mine->target == 7 && mine->cd == 11);
}

static void test_mine_arm_chase_explosive_splash(void)
{
    start_t5(); clear_world_fixture();
    Unit *mine = unit_fixture(0, U_SPIDER_MINE, 13, ENEMY, 6, 100, 100, 20);
    Unit *marine = unit_fixture(1, U_MARINE, 0, SIDE, 0, 100, 100, 100);
    Unit *friendly_vulture = unit_fixture(2, U_VULTURE, 2, ENEMY, 6,
                                           136, 100, 100);
    Unit *flying = unit_fixture(3, U_WRAITH, 8, SIDE, 0, 100, 100, 100);
    Building *building = &b[0];
    building->alive = 1; building->type = B_BARRACKS;
    building->raw_type = 111; building->owner = SIDE;
    building->hp = building->maxhp = 500;
    building->x = 100; building->y = 100;
    marine->shield = 10; marine->armor = 2;
    marine->carry = 0;
    friendly_vulture->armor = 0;

    /* Initial state, 75-tick converted arm delay, acquire, then contact. */
    for (int tick = 0; tick < SC64_T5_MINE_ARM_TICKS; ++tick)
        race_t5_mines_tick();
    assert(mine->alive && mine->carry == SC64_T5_MINE_ARMED);
    assert(mine->work_timer == 0 && mine->target == -1);
    race_t5_mines_tick();
    assert(mine->carry == SC64_T5_MINE_CHASING && mine->target == 1);
    race_t5_mines_tick();
    assert(!mine->alive);

    /* Actual hit_target path: shield first, then armor, then Explosive size. */
    assert(marine->shield == 0 && marine->hp == 44); /* (125-10-2)/2 */
    /* Source radial splash includes allies; Vulture is Medium and 56 source
       pixels from its bounding box, so 125/2 then *3/4 = 46 integer HP. */
    assert(friendly_vulture->hp == 54);
    assert(flying->hp == 100);
    assert(building->hp == 500); /* Spider Mines' NoBuild ground weapon. */
}

static void test_t5_hero_combat_damage(void)
{
    start_t5(); clear_world_fixture();
    weapons_level = 1;
    Unit *raynor = unit_fixture(0, U_RAYNOR_VULTURE, 19, SIDE, 0,
                                100, 100, 300);
    Unit *marine = unit_fixture(1, U_SCV, 7, ENEMY, 6, 110, 100, 100);
    tick_combat(raynor, 1);
    assert(marine->hp == 68); /* 30 base + 2 hero vehicle-weapon upgrade. */
    assert(raynor->cd == 28);

    clear_world_fixture();
    weapons_level = 1;
    Unit *kerrigan = unit_fixture(0, U_KERRIGAN, 16, SIDE, 0,
                                  100, 100, 250);
    Unit *vulture = unit_fixture(1, U_VULTURE, 2, ENEMY, 6, 110, 100, 100);
    vulture->shield = 5; vulture->armor = 2;
    tick_combat(kerrigan, 1);
    assert(vulture->shield == 0 && vulture->hp == 88);
    assert(kerrigan->cd == 28);
}

int main(void)
{
    test_update_orders_preserves_mine_state();
    test_mine_arm_chase_explosive_splash();
    test_mine_acquisition_matches_full_scan();
    test_t5_hero_combat_damage();
    puts("T5 mine and hero actual-engine fixtures passed (not gameplay proof)");
    return 0;
}
