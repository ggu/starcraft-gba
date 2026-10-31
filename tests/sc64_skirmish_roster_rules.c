/* Asset-free mapping/rule coverage for the opt-in standard roster extension.
 * This verifies identities and numeric rules only, not runtime behavior or art. */
#include <assert.h>
#include <stdio.h>

#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_skirmish_catalog.h"
#include "../starcraft/sc64_stock_profiles.h"

#ifndef SC64_SKIRMISH_ROSTER
#error "fixture must exercise the opt-in roster extension"
#endif
#ifndef SC64_TEST_EXPECT_UNITS
#error "pass the captured profile's expected unit enum count"
#endif
#ifndef SC64_TEST_EXPECT_BUILDINGS
#error "pass the captured profile's expected building enum count"
#endif
#ifndef SC64_TEST_EXPECT_UNIT_ROWS
#error "pass the all-race ordinary unit catalog row count"
#endif
#ifndef SC64_TEST_EXPECT_BUILDING_ROWS
#error "pass the all-race ordinary building catalog row count"
#endif

static int ordinary_unit_kind(unsigned kind)
{
    return kind == SC64_SK_TRAIN || kind == SC64_SK_UNIT_MORPH;
}

static int ordinary_building_kind(unsigned kind)
{
    return kind == SC64_SK_WORKER_BUILD || kind == SC64_SK_ADDON ||
        kind == SC64_SK_BUILDING_MORPH;
}

static int roster_added_unit(unsigned raw)
{
    switch (raw) {
#ifndef SC64_TERRAN_LATE_ACTORS
    case 9:
#ifndef SC64_P5_ACTORS
    case 12:
#endif
#endif
#ifndef SC64_Z6_ACTORS
    case 44:
#endif
    case 46: case 62: case 103: case 58: case 61: case 60: case 63:
#ifndef SC64_P6_ACTORS
    case 50:
#endif
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case 67:
#endif
#ifndef SC64_P7_ACTORS
    case 84: case 72: case 71: case 68:
#endif
#if !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
    case 69:
#endif
        return 1;
    default: return 0;
    }
}

static int roster_added_building(unsigned raw)
{
    switch (raw) {
    case 107: return 1;
#ifndef SC64_NYDUS_ENABLED
    case 134: return 1;
#endif
#ifndef SC64_TERRAN_LATE_ACTORS
    case 108: case 117: case 118: case 116: case 136: return 1;
#ifndef SC64_P7_ACTORS
    case 163: case 169: return 1;
#endif
#endif
#ifndef SC64_P7_ACTORS
    case 165: case 159: case 170: return 1;
#endif
    default: return 0;
    }
}

int main(void)
{
    unsigned units = 0, buildings = 0;
    fprintf(stderr, "profile enum counts units=%u buildings=%u\n",
            (unsigned)SC_RU_COUNT, (unsigned)SC_RB_COUNT);
    assert(SC_RU_COUNT == SC64_TEST_EXPECT_UNITS);
    assert(SC_RB_COUNT == SC64_TEST_EXPECT_BUILDINGS);
    assert(SC_RU_COUNT <= 64);
    assert(SC_RB_COUNT <= 128);

    for (unsigned i = 0; i < SC64_SKIRMISH_UNIT_COUNT; ++i) {
        const Sc64SkirmishRecord *record = &sc64_skirmish_unit_catalog[i];
        int type;
        if (!ordinary_unit_kind(record->kind)) continue;
        type = sc64_race_unit_from_raw(record->raw_id);
        assert(type >= 0 && type < SC_RU_COUNT);
        assert(sc64_race_raw_unit(type) == record->raw_id);
        {
            const Sc64RaceUnitRule *rule = sc64_race_unit_rule(type);
            const Sc64StockUnitProfile *stock = sc64_stock_unit_profile(record->raw_id);
            assert(rule != 0 && stock != 0);
            if (roster_added_unit(record->raw_id)) {
                assert(rule->minerals == stock->minerals);
                assert(rule->gas == stock->gas);
                assert(rule->build_frames == stock->build_frames);
                assert(rule->hp == stock->hp);
                assert(rule->shields == stock->shields);
                assert(rule->supply == stock->supply_required);
            }
        }
        assert(sc64_race_unit_name(type)[0] != '\0');
        ++units;
    }
    assert(units == SC64_TEST_EXPECT_UNIT_ROWS);

    for (unsigned i = 0; i < SC64_SKIRMISH_CATALOG_COUNT; ++i) {
        const Sc64SkirmishRecord *record = sc64_skirmish_record_at(i);
        int type;
        if (!ordinary_building_kind(record->kind)) continue;
        type = sc64_race_building_from_raw(record->raw_id);
        if (type < 0 || type >= SC_RB_COUNT)
            fprintf(stderr, "unmapped building raw=%u race=%u name=%s\n",
                    record->raw_id, record->race, record->name);
        assert(type >= 0 && type < SC_RB_COUNT);
        assert(sc64_race_raw_building(type) == record->raw_id);
        {
            const Sc64RaceBuildingRule *rule = sc64_race_building_rule(type);
            const Sc64StockUnitProfile *stock = sc64_stock_unit_profile(record->raw_id);
            assert(rule != 0 && stock != 0);
            if (roster_added_building(record->raw_id)) {
                assert(rule->minerals == stock->minerals);
                assert(rule->gas == stock->gas);
                assert(rule->build_frames == stock->build_frames);
                assert(rule->hp == stock->hp);
                assert(rule->supply_provided == stock->supply_provided);
            }
        }
        assert(sc64_race_building_name(type)[0] != '\0');
        ++buildings;
    }
    assert(buildings == SC64_TEST_EXPECT_BUILDING_ROWS);

    assert(sc64_race_unit_from_raw(58) == U_SK_VALKYRIE);
    assert(sc64_race_unit_rule(U_SK_VALKYRIE)->minerals ==
           sc64_stock_unit_profile(58)->minerals);
    assert(sc64_stock_building_profile(107) != 0);
    puts("skirmish roster mapping/rule fixture ok");
    return 0;
}
