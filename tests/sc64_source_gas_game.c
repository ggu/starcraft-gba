/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercise the shared worker, gas-site and save logic with real product data.
 * The fixture places the worker at pickup and deposit to isolate trip yield. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static void trip(int worker, int refinery, int expected)
{
    Unit *q = &u[worker];
    int home = owner_command_center(q->owner);
    assert(home >= 0);
    q->x = b[refinery].x;
    q->y = b[refinery].y;
    q->order = ORDER_GAS;
    q->target = refinery;
    q->work_timer = 1;
    race_update_resource_worker(q);
    assert(q->carry == expected);
    int before = gas;
    q->x = b[home].x;
    q->y = b[home].y;
    race_update_resource_worker(q);
    assert(q->carry == 0 && gas == before + expected);
}

static void play(int race)
{
    game_init();
    race_mode = 1;
    race_human = race;
    race_bots = 1;
    race_skirmish_map = 1;
    race_bot_races[0] = R_TER;
    init_state();
    assert(screen == SCREEN_PLAY && sc64_gas_site_count == 4);
    int worker = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && IS_WORKER(u[i].type)) { worker = i; break; }
    assert(worker >= 0);
    Sc64GasSite site = sc64_gas_sites[0];
    assert(site.initial == 10000);
    int type = race == R_TER ? B_REFINERY : race == R_ZERG ? B_EXTRACTOR : B_ASSIMILATOR;
    int refinery = spawn_build(SIDE, type, site.x, site.y, 0);
    assert(refinery >= 0);
    trip(worker, refinery, 8);
    assert(sc64_geyser_consumed[0] == 8 && sc64_geyser_consumed[1] == 0);
    sc64_geyser_consumed[0] = site.initial - 8;
    trip(worker, refinery, 8);
    assert(sc64_geyser_consumed[0] == site.initial);
    trip(worker, refinery, 2);
    assert(race_save_game());
    sc64_geyser_consumed[0] = 0;
    assert(race_load_game());
    assert(sc64_geyser_consumed[0] == site.initial && sc64_geyser_consumed[1] == 0);
    trip(worker, refinery, 2);
    /* A replacement at the same site draws on the same reserve. */
    b[refinery].alive = 0;
    refinery = spawn_build(SIDE, type, site.x, site.y, 0);
    assert(refinery >= 0);
    trip(worker, refinery, 2);
}

static void campaign_sites(void)
{
    unsigned checked = 0;
    unsigned max_sites = 0;
    for (int mission = 0; mission < SC64_RACE_MAP_COUNT; ++mission) {
        if (sc64_catalog_mission_faction(mission) != SC64_CAMPAIGN_RACE) continue;
        const Sc64RaceMap *map = &sc64_race_maps[mission];
        race_mode = 0;
        race_selected_mission = mission;
        sc64_gas_start();
        unsigned ordinal = 0;
        for (unsigned i = 0; i < map->unit_count; ++i) {
            const Sc64ImportedUnit *unit = &map->units[i];
            if (unit->source_type != 188 && unit->source_type != 110 &&
                unit->source_type != 149 && unit->source_type != 157) continue;
            assert(ordinal < sc64_gas_site_count);
            Sc64GasSite site = sc64_gas_sites[ordinal];
            assert(site.x == unit->x_px && site.y == unit->y_px && site.initial == unit->resources);
            for (unsigned j = 0; j < ordinal; ++j)
                assert(site.x != sc64_gas_sites[j].x || site.y != sc64_gas_sites[j].y);
            sc64_geyser_consumed[ordinal] = site.initial - site.initial % 8;
            assert(sc64_gas_harvest(site.x, site.y) == 2);
            assert(sc64_geyser_consumed[ordinal] == site.initial);
            ++ordinal;
        }
        assert(ordinal == sc64_gas_site_count);
        if (ordinal > max_sites) max_sites = ordinal;
        ++checked;
    }
    assert(checked == 10);
    printf("Verified gas sites on %u campaign maps, maximum %u sites per map\n", checked, max_sites);
}

int main(void)
{
    for (int race = 0; race < 3; ++race) play(race);
    campaign_sites();
    puts("PASS: all three gas structures, full and depleted deliveries, save/load and replacement");
    return 0;
}
