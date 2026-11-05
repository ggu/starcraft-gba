/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST progression, not an ordinary-input or native parity proof.
 * Starting resources and worker positions are fixture-controlled; structures
 * must pay and finish through the existing request/arrival/build paths. */
#ifndef SC64_SCARAB_TECH_PATH_HOST_PROBE
#error "Use the explicit full-service HOST tech-path probe"
#endif
#define main sc64_skirmish_runtime_unused_main
#include "sc64_skirmish_runtime.c"
#undef main

int main(void)
{
    static const int chain[] = {160, 164, 155, 171};
    /* The shared journal supports custom games with one to three opponents;
     * construct a supported world from the outset rather than changing its
     * saved player count after the test has run. */
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;
    race_bot_races[0]=R_ZERG;init_state();
    assert(screen==SCREEN_PLAY && race_mode && race_player[SIDE]==R_PROTOSS &&
           race_bots==1 && race_player[ENEMY]==R_ZERG);
    minerals=gas=5000;
    int worker = find_worker_raw(SIDE, 64), facility = -1;
    int type = unit_raw(83);
    const Sc64SkirmishRecord *reaver = 0;
    assert(worker >= 0 && type >= 0 && menu_row_raw(171) >= 10);
    for (unsigned i = 0; i < sc64_skirmish_product_count(155); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_product_at(155, i);
        if (r->raw_id == 83) reaver = r;
    }
    assert(reaver);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) u[i].order = ORDER_IDLE;
    for (unsigned k = 0; k < sizeof(chain) / sizeof(chain[0]); ++k) {
        assert(build_with_worker(156, worker) >= 0);
        if (chain[k] == 171) {
            int m = minerals, g = gas;
            assert(facility >= 0 && !race_queue_unit(SIDE, facility, type));
            assert(minerals == m && gas == g && !b[facility].queue_count);
        }
        int slot = build_with_worker(chain[k], worker);
        assert(slot >= 0);
        if (chain[k] == 155) facility = slot;
    }
    assert(race_skirmish_product_admitted(SIDE, facility, reaver));
    int m = minerals, g = gas;
    assert(race_queue_unit(SIDE, facility, type));
    assert(minerals == m - 200 && gas == g - 100);
    for (int tick = 0; tick < unit_build_time(type) + 8 && b[facility].queue_count; ++tick)
        race_tick_buildings();
    assert(!b[facility].queue_count);
    int parent = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_race_raw_unit(u[i].type) == 83) {
            assert(parent < 0); parent = i;
        }
    assert(parent >= 0);
    Sc64ScarabParent p;
    Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(parent, 0, &p, &counts));
    assert(!counts.inside && !p.queued);
    m = minerals; g = gas;
    assert(race_scarab_enqueue(parent, 0));
    assert(minerals == m - 15 && gas == g);
    for (int tick = 0; tick < 120; ++tick) race_scarab_production_tick(parent, 0);
    assert(race_scarab_parent_read(parent, 0, &p, &counts));
    assert(counts.inside == 1 && !p.queued);
    assert(race_save_game());
    assert(race_load_game());
    assert(race_scarab_parent_read(parent, 0, &p, &counts));
    assert(counts.inside == 1 && !p.queued);
    puts("constructed paid tech progression, Reaver birth, ammunition and journal passed");
    return 0;
}
