/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_skirmish_menu.h"

static const unsigned char basic_raw[SC64_SK_RACE_COUNT][5] = {
    { 111, 109, 110, 112, 122 },
    { 142, 131, 149, 135, 139 },
    { 160, 156, 157, 164, 166 }
};
static const char *const basic_names[SC64_SK_RACE_COUNT][5] = {
    { "BARRACKS", "SUPPLY DEPOT", "REFINERY", "ACADEMY", "ENGINEERING BAY" },
    { "SPAWNING POOL", "HATCHERY", "EXTRACTOR", "HYDRALISK DEN", "EVOLUTION CHAMBER" },
    { "GATEWAY", "PYLON", "ASSIMILATOR", "CYBERNETICS CORE", "FORGE" }
};

static const Sc64SkirmishRecord *find_build(unsigned race, unsigned raw)
{
    unsigned i;
    for (i = 0; i < sc64_skirmish_build_count(race); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_build_at(race, i);
        if (r->raw_id == raw) return r;
    }
    return 0;
}

static int reject_one(const Sc64SkirmishRecord *r, void *context)
{
    unsigned rejected = *(const unsigned *)context;
    return r->raw_id != rejected;
}

static int reject_all(const Sc64SkirmishRecord *r, void *context)
{
    (void)r;
    (void)context;
    return 0;
}

static void check_menu_race(unsigned race)
{
    unsigned i, expected = 10;
    Sc64SkirmishMenuRow row;
    assert(sc64_skirmish_menu_count(race) >= 10);
    assert(sc64_skirmish_menu_at(race, 0, &row));
    assert(row.kind == SC64_SK_MENU_MOVE && !row.record && !strcmp(row.label, "MOVE"));
    assert(sc64_skirmish_menu_at(race, 1, &row));
    assert(row.kind == SC64_SK_MENU_ATTACK && !row.record && !strcmp(row.label, "ATTACK"));
    for (i = 0; i < 5; ++i) {
        assert(sc64_skirmish_menu_at(race, i + 2, &row));
        assert(row.kind == SC64_SK_MENU_BUILD);
        assert(row.record && row.record->raw_id == basic_raw[race][i]);
        assert(!strcmp(row.label, basic_names[race][i]));
        assert(row.record->kind == SC64_SK_WORKER_BUILD);
    }
    assert(sc64_skirmish_menu_at(race, 7, &row));
    assert(row.kind == SC64_SK_MENU_WEAPONS && !row.record);
    assert(sc64_skirmish_menu_at(race, 8, &row));
    assert(row.kind == SC64_SK_MENU_ARMOR && !row.record);
    assert(sc64_skirmish_menu_at(race, 9, &row));
    assert(row.kind == SC64_SK_MENU_STOP && !row.record);

    for (i = 0; i < sc64_skirmish_build_count(race); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_build_at(race, i);
        if (!sc64_skirmish_menu_is_basic(race, r->raw_id)) ++expected;
    }
    assert(sc64_skirmish_menu_count(race) == expected);
    for (i = 10; i < expected; ++i) {
        unsigned j, ordinal = 10;
        const Sc64SkirmishRecord *expected_record = 0;
        assert(sc64_skirmish_menu_at(race, i, &row));
        /* Independently derive the appended sequence from the authored catalog. */
        for (j = 0; j < sc64_skirmish_build_count(race); ++j) {
            const Sc64SkirmishRecord *r = sc64_skirmish_build_at(race, j);
            if (sc64_skirmish_menu_is_basic(race, r->raw_id)) continue;
            if (ordinal++ == i) { expected_record = r; break; }
        }
        assert(expected_record && row.record == expected_record);
        /* 17 six-pixel cells leave room for both three-digit costs before
         * the minimap starts at x=178 in the command-icon HUD. */
        assert(row.label && strlen(row.label) <= 17);
        if (row.record->raw_id == 120) assert(!strcmp(row.label, "MACHINE SHOP"));
        if (row.record->raw_id == 132) assert(!strcmp(row.label, "MORPH LAIR"));
        if (expected_record->kind == SC64_SK_ADDON)
            assert(row.kind == SC64_SK_MENU_ADDON);
        else if (expected_record->kind == SC64_SK_BUILDING_MORPH)
            assert(row.kind == SC64_SK_MENU_MORPH);
        else
            assert(row.kind == SC64_SK_MENU_BUILD);
    }
    assert(!sc64_skirmish_menu_at(race, expected, &row));
}

static void check_command_center_row(void)
{
    unsigned i;
    Sc64SkirmishMenuRow row;
    for (i = 0; i < sc64_skirmish_menu_count(SC64_SK_TERRAN); ++i) {
        assert(sc64_skirmish_menu_at(SC64_SK_TERRAN, i, &row));
        if (row.record && row.record->raw_id == 106) {
            assert(row.record->kind == SC64_SK_WORKER_BUILD);
            assert(!strcmp(row.label, "Command Center"));
            return;
        }
    }
    assert(!"Terran menu must include the Command Center build option");
}

static void check_product_cycle(unsigned producer, const unsigned *ids,
                                unsigned count)
{
    unsigned i, current = SC64_SK_NONE;
    for (i = 0; i < count; ++i) {
        int next = sc64_skirmish_menu_next_product(producer, current, 0, 0);
        assert(next == (int)ids[i]);
        current = (unsigned)next;
    }
    assert(sc64_skirmish_menu_next_product(producer, current, 0, 0) == (int)ids[0]);
    assert(sc64_skirmish_menu_next_product(producer, 252, 0, 0) == (int)ids[0]);
}

int main(void)
{
    static const unsigned barracks[] = { 0, 1, 32, 34 };
    static const unsigned command_center[] = { 7 };
    static const unsigned factory[] = { 2, 3, 5 };
    static const unsigned stargate[] = { 70, 60, 72, 71 };
    static const unsigned gateway[] = { 65, 66, 67, 61 };
    static const unsigned robotics[] = { 69, 84, 83 };
    static const unsigned larva[] = { 41, 42, 37, 38, 45, 43, 46, 47, 39 };
    unsigned i, locked = 5;
    Sc64SkirmishMenuRow row;

    for (i = 0; i < SC64_SK_RACE_COUNT; ++i) check_menu_race(i);
    check_command_center_row();
    assert(sc64_skirmish_menu_count(SC64_SK_RACE_COUNT) == 0);
    assert(!sc64_skirmish_menu_at(SC64_SK_RACE_COUNT, 0, &row));
    assert(!sc64_skirmish_menu_at(SC64_SK_TERRAN, 0, 0));
    assert(!sc64_skirmish_menu_record(SC64_SK_TERRAN, 7)); /* SCV is a product, not a build option. */
    assert(!sc64_skirmish_menu_record(SC64_SK_ZERG, 106)); /* Never resolve another race's row. */
    assert(!find_build(SC64_SK_TERRAN, 30)); /* Siege mode is not a building row. */

    check_product_cycle(111, barracks, sizeof(barracks) / sizeof(barracks[0]));
    check_product_cycle(106, command_center, sizeof(command_center) / sizeof(command_center[0]));
    check_product_cycle(113, factory, sizeof(factory) / sizeof(factory[0]));
    check_product_cycle(167, stargate, sizeof(stargate) / sizeof(stargate[0]));
    check_product_cycle(160, gateway, sizeof(gateway) / sizeof(gateway[0]));
    check_product_cycle(155, robotics, sizeof(robotics) / sizeof(robotics[0]));
    check_product_cycle(35, larva, sizeof(larva) / sizeof(larva[0]));
    locked = 5;
    assert(sc64_skirmish_menu_next_product(113, 3, reject_one, &locked) == 2);
    locked = 3;
    assert(sc64_skirmish_menu_next_product(113, 2, reject_one, &locked) == 5);
    locked = 2;
    assert(sc64_skirmish_menu_next_product(113, 5, reject_one, &locked) == 3);
    assert(sc64_skirmish_menu_next_product(113, 5, reject_all, 0) == SC64_SK_NONE);
    assert(sc64_skirmish_menu_next_product(255, SC64_SK_NONE, 0, 0) == SC64_SK_NONE);

    puts("sc64 skirmish menu: ok");
    return 0;
}
