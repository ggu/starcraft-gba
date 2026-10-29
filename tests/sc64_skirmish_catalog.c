/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_skirmish_catalog.h"

static const Sc64SkirmishRecord *find(unsigned id) {
    unsigned i;
    for (i = 0; i < sc64_skirmish_record_count(); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_record_at(i);
        if (r && r->raw_id == id) return r;
    }
    return 0;
}

static void expect(unsigned id, unsigned kind, unsigned producer,
                   unsigned parent, unsigned req1, unsigned req2) {
    const Sc64SkirmishRecord *r = find(id);
    assert(r);
    assert(r->kind == kind);
    assert(r->producer_raw == producer);
    assert(r->parent_raw == parent);
    assert(r->prereq1_raw == req1);
    assert(r->prereq2_raw == req2);
}

typedef struct { unsigned char id, producer; } ProducerFact;
/* Transcribed independently from the pinned UnitType-pinned.cpp whatBuilds
 * facts for every normal train product. Non-train transitions are tested
 * separately because BWAPI's whatBuilds identity alone does not describe
 * their mode/morph/subunit lifecycle. */
static const ProducerFact expected_train_producers[] = {
    {7,106},{0,111},{1,111},{32,111},{34,111},
    {2,113},{3,113},{5,113},
    {8,114},{9,114},{11,114},{12,114},{58,114},
    {41,35},{42,35},{37,35},{38,35},{45,35},{43,35},{46,35},{47,35},{39,35},
    {50,130},
    {64,154},{65,160},{66,160},{67,160},{61,160},
    {69,155},{84,155},{83,155},
    {70,167},{60,167},{72,167},{71,167}
};
typedef struct {
    unsigned char id, kind, producer, parent, req1, req2;
} BuildFact;
/* Required-unit/whatBuilds facts for every construction, addon, and structure
 * morph row, reviewed against the pinned metadata prerequisite table. */
static const BuildFact expected_build_facts[] = {
    {106,SC64_SK_WORKER_BUILD,7,255,255,255},
    {109,SC64_SK_WORKER_BUILD,7,255,255,255},
    {110,SC64_SK_WORKER_BUILD,7,255,255,255},
    {111,SC64_SK_WORKER_BUILD,7,255,106,255},
    {112,SC64_SK_WORKER_BUILD,7,255,111,255},
    {113,SC64_SK_WORKER_BUILD,7,255,111,255},
    {114,SC64_SK_WORKER_BUILD,7,255,113,255},
    {116,SC64_SK_WORKER_BUILD,7,255,114,255},
    {122,SC64_SK_WORKER_BUILD,7,255,106,255},
    {123,SC64_SK_WORKER_BUILD,7,255,113,255},
    {124,SC64_SK_WORKER_BUILD,7,255,122,255},
    {125,SC64_SK_WORKER_BUILD,7,255,111,255},
    {107,SC64_SK_ADDON,106,106,112,255},
    {108,SC64_SK_ADDON,106,106,116,117},
    {115,SC64_SK_ADDON,114,114,255,255},
    {117,SC64_SK_ADDON,116,116,255,255},
    {118,SC64_SK_ADDON,116,116,255,255},
    {120,SC64_SK_ADDON,113,113,255,255},
    {131,SC64_SK_WORKER_BUILD,41,255,255,255},
    {132,SC64_SK_BUILDING_MORPH,131,131,142,255},
    {133,SC64_SK_BUILDING_MORPH,132,132,138,255},
    {142,SC64_SK_WORKER_BUILD,41,255,131,255},
    {135,SC64_SK_WORKER_BUILD,41,255,142,255},
    {139,SC64_SK_WORKER_BUILD,41,255,131,255},
    {149,SC64_SK_WORKER_BUILD,41,255,255,255},
    {138,SC64_SK_WORKER_BUILD,41,255,132,255},
    {141,SC64_SK_WORKER_BUILD,41,255,132,255},
    {137,SC64_SK_BUILDING_MORPH,141,141,133,255},
    {136,SC64_SK_WORKER_BUILD,41,255,133,255},
    {140,SC64_SK_WORKER_BUILD,41,255,133,255},
    {134,SC64_SK_WORKER_BUILD,41,255,133,255},
    {143,SC64_SK_WORKER_BUILD,41,255,131,255},
    {146,SC64_SK_BUILDING_MORPH,143,143,142,255},
    {144,SC64_SK_BUILDING_MORPH,143,143,139,255},
    /* Queen infestation is a state/ability transition, not Drone construction. */
    {130,SC64_SK_BUILDING_MORPH,45,106,255,255},
    {154,SC64_SK_WORKER_BUILD,64,255,255,255},
    {156,SC64_SK_WORKER_BUILD,64,255,255,255},
    {157,SC64_SK_WORKER_BUILD,64,255,255,255},
    {160,SC64_SK_WORKER_BUILD,64,255,154,255},
    {166,SC64_SK_WORKER_BUILD,64,255,154,255},
    {164,SC64_SK_WORKER_BUILD,64,255,160,255},
    {163,SC64_SK_WORKER_BUILD,64,255,164,255},
    {165,SC64_SK_WORKER_BUILD,64,255,163,255},
    {155,SC64_SK_WORKER_BUILD,64,255,164,255},
    {171,SC64_SK_WORKER_BUILD,64,255,155,255},
    {159,SC64_SK_WORKER_BUILD,64,255,155,255},
    {167,SC64_SK_WORKER_BUILD,64,255,164,255},
    {169,SC64_SK_WORKER_BUILD,64,255,167,255},
    {170,SC64_SK_WORKER_BUILD,64,255,165,167},
    {162,SC64_SK_WORKER_BUILD,64,255,166,255},
    {172,SC64_SK_WORKER_BUILD,64,255,160,255}
};

int main(void) {
    unsigned i, race, train = 0, morph = 0, subunit = 0, ammo = 0;
    unsigned terran_build = 0, zerg_build = 0, protoss_build = 0;
    unsigned char seen[228] = {0};
    static const unsigned standard_unit_ids[] = {
        7,0,1,2,3,5,8,9,11,12,32,34,58,
        41,42,37,38,45,43,46,47,39,50,44,62,103,
        64,65,66,67,61,69,84,83,70,60,72,71,68,63
    };

    assert(sc64_skirmish_record_at(sc64_skirmish_record_count()) == 0);
    assert(sc64_skirmish_build_at(SC64_SK_TERRAN, sc64_skirmish_build_count(SC64_SK_TERRAN)) == 0);
    assert(sc64_skirmish_product_at(113, sc64_skirmish_product_count(113)) == 0);
    for (race = 0; race < SC64_SK_RACE_COUNT; ++race) {
        assert(sc64_skirmish_build_count(race) > 0);
        for (i = 0; i < sc64_skirmish_build_count(race); ++i) {
            const Sc64SkirmishRecord *r = sc64_skirmish_build_at(race, i);
            assert(r && r->race == race);
            assert(r->kind == SC64_SK_WORKER_BUILD || r->kind == SC64_SK_ADDON ||
                   r->kind == SC64_SK_BUILDING_MORPH);
            if (race == SC64_SK_TERRAN) ++terran_build;
            if (race == SC64_SK_ZERG) ++zerg_build;
            if (race == SC64_SK_PROTOSS) ++protoss_build;
        }
    }
    assert(terran_build >= 18 && zerg_build >= 17 && protoss_build >= 16);

    for (i = 0; i < sc64_skirmish_record_count(); ++i) {
        const Sc64SkirmishRecord *r = sc64_skirmish_record_at(i);
        assert(r && r->name && r->name[0]);
        assert(r->input_count > 0);
        assert(r->raw_id < sizeof(seen));
        assert(!seen[r->raw_id]);
        seen[r->raw_id] = 1;
        assert(r->race < SC64_SK_RACE_COUNT);
        if (r->kind == SC64_SK_TRAIN) {
            unsigned j, found_in_view = 0;
            ++train;
            for (j = 0; j < sc64_skirmish_product_count(r->producer_raw); ++j)
                if (sc64_skirmish_product_at(r->producer_raw, j) == r) found_in_view = 1;
            assert(found_in_view);
        } else if (r->kind == SC64_SK_UNIT_MORPH) ++morph;
        else if (r->kind == SC64_SK_SUBUNIT) ++subunit;
        else if (r->kind == SC64_SK_AMMO) ++ammo;
    }
    assert(train >= 35 && morph >= 5 && subunit >= 3 && ammo >= 2);
    assert(train == sizeof(expected_train_producers) / sizeof(expected_train_producers[0]));
    for (i = 0; i < sizeof(expected_train_producers) / sizeof(expected_train_producers[0]); ++i) {
        const Sc64SkirmishRecord *r = find(expected_train_producers[i].id);
        assert(r && r->kind == SC64_SK_TRAIN);
        assert(r->producer_raw == expected_train_producers[i].producer);
    }
    assert(terran_build + zerg_build + protoss_build ==
           sizeof(expected_build_facts) / sizeof(expected_build_facts[0]));
    for (i = 0; i < sizeof(expected_build_facts) / sizeof(expected_build_facts[0]); ++i) {
        const BuildFact *f = &expected_build_facts[i];
        expect(f->id, f->kind, f->producer, f->parent, f->req1, f->req2);
    }
    for (i = 0; i < sizeof(standard_unit_ids) / sizeof(standard_unit_ids[0]); ++i)
        assert(find(standard_unit_ids[i]));

    /* The normal Tank is a Factory product; Siege Mode is a separate
     * transition, with its Machine Shop unlock preserved. */
    expect(5, SC64_SK_TRAIN, 113, 255, 120, 255);
    expect(30, SC64_SK_MODE, 113, 5, 120, 255);
    expect(3, SC64_SK_TRAIN, 113, 255, 123, 255);
    expect(72, SC64_SK_TRAIN, 167, 255, 169, 255);
    expect(44, SC64_SK_UNIT_MORPH, 43, 43, 137, 255);
    expect(62, SC64_SK_UNIT_MORPH, 43, 43, 137, 255);
    expect(103, SC64_SK_UNIT_MORPH, 38, 38, 135, 255);
    assert(find(103)->tech_key == SC64_SK_TECH_LURKER_ASPECT);
    expect(68, SC64_SK_UNIT_MORPH, 67, 67, 255, 255);
    expect(63, SC64_SK_UNIT_MORPH, 61, 61, 255, 255);
    expect(73, SC64_SK_AMMO, 72, 72, 255, 255);
    expect(85, SC64_SK_AMMO, 83, 83, 255, 255);
    expect(14, SC64_SK_AMMO, 108, 108, 255, 255);
    expect(35, SC64_SK_SUBUNIT, 131, 131, 255, 255);
    expect(4, SC64_SK_SUBUNIT, 3, 3, 255, 255);
    expect(6, SC64_SK_SUBUNIT, 5, 5, 255, 255);
    expect(31, SC64_SK_SUBUNIT, 30, 30, 255, 255);
    assert(find(68)->kind != SC64_SK_TRAIN && find(63)->kind != SC64_SK_TRAIN);
    assert(find(68)->input_count == 2 && find(63)->input_count == 2);
    assert(sc64_skirmish_product_count(167) >= 4);
    assert(sc64_skirmish_product_count(113) >= 3);
    assert(sc64_skirmish_product_count(160) >= 4);

    /* Exclude obvious heroes, doodads, resources, and map objects. */
    assert(!find(10) && !find(16) && !find(48) && !find(74));
    assert(!find(176) && !find(188) && !find(203));
    assert(!sc64_skirmish_build_at(99, 0));
    puts("sc64 skirmish catalog: ok");
    return 0;
}
