/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private prototype: seeded infrastructure, real admission and paid queue.
 * This is not an ordinary-menu or save/load qualification. */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void)
{
    start_race(R_TER);
    int starport = fixture_building_raw(SIDE, 114, 500, 500);
    int second = fixture_building_raw(SIDE, 114, 700, 500);
    int tower = fixture_building_raw(SIDE, 115, 748, 508);
    int lab = fixture_building_raw(SIDE, 118, 900, 500);
    int sf = fixture_building_raw(SIDE, 116, 852, 492);
    int type = unit_raw(12);
    assert(starport >= 0 && second >= 0 && tower >= 0 && lab >= 0 && sf >= 0 && type >= 0);
    assert(!race_queue_unit(SIDE, starport, type)); /* tower belongs to second */
    b[tower].x = 548; b[tower].y = 508;
    b[lab].under_construction = 1;
    assert(!race_queue_unit(SIDE, starport, type));
    b[lab].under_construction = 0; b[lab].owner = ENEMY;
    assert(!race_queue_unit(SIDE, starport, type));
    b[lab].owner = SIDE; b[sf].alive = 0; /* completed Lab survives parent */
    int before_m = minerals, before_g = gas;
    assert(race_queue_unit(SIDE, starport, type));
    assert(minerals == before_m - 400 && gas == before_g - 300);
    assert(b[starport].queue_count == 1 && b[starport].queue[b[starport].queue_head] == type);
    for (int tick = 0; tick < 2510 && b[starport].queue_count; ++tick)
        race_tick_buildings();
    assert(b[starport].queue_count == 0);
    int cruiser = find_worker_raw(SIDE, 12);
    assert(cruiser >= 0 && u[cruiser].hp == 500 && u[cruiser].armor == 3);
    assert(sc64_race_unit_flying(type));
    assert(sc64_race_unit_damage_vs(type, 0) == 25);
    assert(sc64_race_unit_damage_vs(type, 1) == 25);
    int original_x = u[cruiser].x;
    for (int step = 0; step < 20; ++step)
        move_toward(&u[cruiser], original_x + 96, u[cruiser].y);
    assert(u[cruiser].x > original_x);
    int ground = spawn_unit(ENEMY, U_MARINE, u[cruiser].x + 32, u[cruiser].y);
    int air = spawn_unit(ENEMY, U_WRAITH, u[cruiser].x + 40, u[cruiser].y);
    assert(ground >= 0 && air >= 0);
    int hp = u[ground].hp;
    u[cruiser].cd = 0;
    tick_combat(&u[cruiser], ground);
    assert(u[ground].hp < hp && u[cruiser].cd > 0);
    hp = u[air].hp;
    u[cruiser].cd = 0;
    tick_combat(&u[cruiser], air);
    assert(u[air].hp < hp && u[cruiser].cd > 0);
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    assert(u[cruiser].carry == 50 * 256);
    u[cruiser].selected = 1;
    stop_selected();
    assert(u[cruiser].carry == 50 * 256);
    for (int t = 0; t < 5; ++t) { ++frame; race_tick_state(); }
    assert(u[cruiser].carry == 50 * 256 + 32);
#endif
    /* Independent normal worker construction followed by the paid add-on. */
    start_race(R_TER);
    starport = fixture_building_raw(SIDE, 114, 500, 500);
    int worker = find_worker_raw(SIDE, 7);
    assert(starport >= 0 && worker >= 0);
    sf = build_with_worker(116, worker);
    assert(sf >= 0 && building_complete(sf));
    selected_building = sf;
    const Sc64SkirmishRecord *lab_record = build_record(R_TER, 118);
    assert(lab_record && race_skirmish_record_available(lab_record));
    before_m = minerals; before_g = gas;
    assert(race_skirmish_choose_record(lab_record));
    assert(minerals == before_m - 50 && gas == before_g - 50);
    lab = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].under_construction &&
            sc64_race_raw_building(b[i].type) == 118) lab = i;
    assert(lab >= 0);
    for (int tick = 0; tick < 760 && !building_complete(lab); ++tick)
        race_tick_buildings();
    assert(building_complete(lab));
    puts("experimental Battlecruiser paid queue, completion, flying and lasers pass");
    return 0;
}
