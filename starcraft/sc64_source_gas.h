/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SOURCE_GAS_H
#define SC64_SOURCE_GAS_H

/* Save ordinals follow the source UNIT order. An occupied refinery and
 * the bare geyser revealed on its destruction share the same account. */
static u16 sc64_geyser_consumed[SC64_FIELD_GEYSER_LIMIT];

static int sc64_gas_source_site(int mode, int map_id, int mission,
                                unsigned ordinal, Sc64GasSite *site)
{
    if (mode == 1) {
        if (map_id == 0) {
            /* Outpost is the authored map. Its four gas sites each start
             * with 5,000 gas; only their positions depend on live geometry. */
            if (ordinal >= 4) return 0;
            *site = (Sc64GasSite){geysers[ordinal].x, geysers[ordinal].y, 5000};
            return 1;
        }
        if (map_id < 1 || map_id > SC64_SKIRMISH_IMPORTED_COUNT) return 0;
        const Sc64SkirmishMap *map = &sc64_skirmish_maps[map_id - 1];
        if (ordinal >= map->geyser_count) return 0;
        const Sc64SkirmishGeyser *src = &map->geysers[ordinal];
        if (src->amount > 65535u) return 0;
        *site = (Sc64GasSite){src->x, src->y, src->amount};
        return 1;
    }
    if (mode != 0 || (unsigned)mission >= SC64_RACE_MAP_COUNT) return 0;
    const Sc64RaceMap *map = &sc64_race_maps[mission];
    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *src = &map->units[i];
        switch (src->source_type) {
        case 110: case 149: case 157: case 188:
            if (ordinal-- == 0) {
                *site = (Sc64GasSite){src->x_px, src->y_px, src->resources};
                return 1;
            }
            break;
        }
    }
    return 0;
}

static unsigned sc64_gas_site_capacity(int mode, int map_id, int mission,
                                       unsigned ordinal)
{
    /* Save preflight can inspect a different map from the active world. */
    if (mode == 1 && map_id == 0) return ordinal < 4 ? 5000u : 0;
    Sc64GasSite site;
    return sc64_gas_source_site(mode, map_id, mission, ordinal, &site) ? site.initial : 0;
}

static void sc64_gas_rebuild_sites(void)
{
    sc64_gas_site_count = 0;
    while (sc64_gas_site_count < SC64_FIELD_GEYSER_LIMIT &&
           sc64_gas_source_site(race_mode, race_skirmish_map, race_selected_mission,
                                sc64_gas_site_count, &sc64_gas_sites[sc64_gas_site_count]))
        ++sc64_gas_site_count;
}

static void sc64_gas_start(void)
{
    memset(sc64_geyser_consumed, 0, sizeof(sc64_geyser_consumed));
    sc64_gas_rebuild_sites();
}

static int sc64_gas_harvest(int x, int y)
{
    for (int i = 0; i < sc64_gas_site_count; ++i) {
        const Sc64GasSite *site = &sc64_gas_sites[i];
        if (site->x != x || site->y != y) continue;
        if (site->initial - sc64_geyser_consumed[i] >= 8) {
            sc64_geyser_consumed[i] += 8;
            return 8;
        }
        /* The N64 exhausts the final 0..7 units and grants two gas,
         * including every subsequent trip to the depleted site. */
        sc64_geyser_consumed[i] = site->initial;
        return 2;
    }
    /* A scripted or synthetic refinery may have no source resource row. */
    return 8;
}
#endif
