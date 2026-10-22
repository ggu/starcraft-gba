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
    test_t5_hero_combat_damage();
    puts("T5 mine and hero actual-engine fixtures passed (not gameplay proof)");
    return 0;
}
