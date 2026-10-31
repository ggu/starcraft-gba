/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SKIRMISH_CATALOG_H
#define SC64_SKIRMISH_CATALOG_H

/* Raw UnitType IDs and relationships for the ordinary StarCraft/Brood War
 * skirmish roster. This is reference/capability data, not a claim that this
 * project currently implements every row. IDs and prerequisite relationships
 * were checked against the locally prepared pinned BWAPI UnitType metadata
 * (.local/sc64/UnitType-pinned.cpp; the project's stock-profile numeric table is another
 * local check). Public product requirements were cross-checked against the
 * Blizzard StarCraft Compendium at https://classic.battle.net/scc/ (for
 * example https://classic.battle.net/scc/protoss/units/carrier.shtml:
 * Stargate, Fleet Beacon). Only authored numeric
 * facts and labels are recorded here; no BWAPI implementation is copied.
 * Heroes, critters, resources, campaign actors, and doodads are excluded.
 *
 * Product/build lists are bounded constant ROM tables. A raw ID is used
 * instead of profile-conditional engine enums so one catalog describes all
 * races independently of campaign compilation flags. SC64_SK_* values are
 * record kinds, not runtime implementation guarantees.
 */
enum {
    SC64_SK_TERRAN = 0, SC64_SK_ZERG = 1, SC64_SK_PROTOSS = 2,
    SC64_SK_RACE_COUNT = 3,
    SC64_SK_NONE = 255,
    SC64_SK_WORKER_BUILD = 1,
    SC64_SK_ADDON = 2,
    SC64_SK_BUILDING_MORPH = 3,
    SC64_SK_TRAIN = 4,
    SC64_SK_UNIT_MORPH = 5,
    SC64_SK_MODE = 6,
    SC64_SK_SUBUNIT = 7,
    SC64_SK_AMMO = 8,
    /* Capability key, not a UnitType raw ID. */
    SC64_SK_TECH_LURKER_ASPECT = 1
};

typedef struct {
    unsigned char raw_id, race, kind, producer_raw, parent_raw, input_count;
    unsigned char prereq1_raw, prereq2_raw, tech_key;
    const char *name;
} Sc64SkirmishRecord;

#define SC64_SK_ROW(id, race_, kind_, producer_, parent_, req1_, req2_, tech_, name_) \
    { (id), (race_), (kind_), (producer_), (parent_), 1, (req1_), (req2_), (tech_), (name_) }
#define SC64_SK_B(id,r,prod,req1,req2,name) SC64_SK_ROW(id,r,SC64_SK_WORKER_BUILD,prod,SC64_SK_NONE,req1,req2,SC64_SK_NONE,name)
#define SC64_SK_A(id,r,prod,parent,req1,req2,name) SC64_SK_ROW(id,r,SC64_SK_ADDON,prod,parent,req1,req2,SC64_SK_NONE,name)
#define SC64_SK_M(id,r,parent,req1,req2,name) SC64_SK_ROW(id,r,SC64_SK_BUILDING_MORPH,parent,parent,req1,req2,SC64_SK_NONE,name)
#define SC64_SK_T(id,r,prod,req1,req2,name) SC64_SK_ROW(id,r,SC64_SK_TRAIN,prod,SC64_SK_NONE,req1,req2,SC64_SK_NONE,name)
#define SC64_SK_U(id,r,parent,req1,req2,tech,name) SC64_SK_ROW(id,r,SC64_SK_UNIT_MORPH,parent,parent,req1,req2,tech,name)
#define SC64_SK_S(id,r,parent,name) SC64_SK_ROW(id,r,SC64_SK_SUBUNIT,parent,parent,SC64_SK_NONE,SC64_SK_NONE,SC64_SK_NONE,name)
#define SC64_SK_Q(id,r,parent,name) SC64_SK_ROW(id,r,SC64_SK_AMMO,parent,parent,SC64_SK_NONE,SC64_SK_NONE,SC64_SK_NONE,name)
#define SC64_SK_MERGE(id,r,parent,name) \
    { (id), (r), SC64_SK_UNIT_MORPH, (parent), (parent), 2, SC64_SK_NONE, SC64_SK_NONE, SC64_SK_NONE, (name) }

/* Raw UnitType values are the stable stock IDs. */
static const Sc64SkirmishRecord sc64_skirmish_unit_catalog[] = {
    SC64_SK_T(7,SC64_SK_TERRAN,106,255,255,"SCV"),
    SC64_SK_T(0,SC64_SK_TERRAN,111,255,255,"Marine"),
    SC64_SK_T(1,SC64_SK_TERRAN,111,112,117,"Ghost"),
    SC64_SK_T(32,SC64_SK_TERRAN,111,112,255,"Firebat"),
    SC64_SK_T(34,SC64_SK_TERRAN,111,112,255,"Medic"),
    SC64_SK_T(2,SC64_SK_TERRAN,113,255,255,"Vulture"),
    SC64_SK_T(3,SC64_SK_TERRAN,113,123,255,"Goliath"),
    SC64_SK_T(5,SC64_SK_TERRAN,113,120,255,"Siege Tank (Tank Mode)"),
    SC64_SK_T(8,SC64_SK_TERRAN,114,255,255,"Wraith"),
    SC64_SK_T(9,SC64_SK_TERRAN,114,115,116,"Science Vessel"),
    SC64_SK_T(11,SC64_SK_TERRAN,114,115,255,"Dropship"),
    SC64_SK_T(12,SC64_SK_TERRAN,114,115,118,"Battlecruiser"),
    SC64_SK_T(58,SC64_SK_TERRAN,114,115,123,"Valkyrie"),
    SC64_SK_S(4,SC64_SK_TERRAN,3,"Goliath turret subunit"),
    SC64_SK_S(6,SC64_SK_TERRAN,5,"Tank Mode turret subunit"),
    SC64_SK_S(31,SC64_SK_TERRAN,30,"Siege Mode turret subunit"),
    SC64_SK_ROW(30,SC64_SK_TERRAN,SC64_SK_MODE,113,5,120,255,255,"Siege Tank (Siege Mode)"),
    SC64_SK_ROW(13,SC64_SK_TERRAN,SC64_SK_SUBUNIT,2,2,255,255,255,"Spider Mine (deployed by Vulture)"),
    SC64_SK_Q(14,SC64_SK_TERRAN,108,"Nuclear Missile (Silo ammunition)"),
    SC64_SK_T(41,SC64_SK_ZERG,35,255,255,"Drone"),
    SC64_SK_T(42,SC64_SK_ZERG,35,255,255,"Overlord"),
    SC64_SK_T(37,SC64_SK_ZERG,35,142,255,"Zergling"),
    SC64_SK_T(38,SC64_SK_ZERG,35,135,255,"Hydralisk"),
    SC64_SK_T(45,SC64_SK_ZERG,35,138,255,"Queen"),
    SC64_SK_T(43,SC64_SK_ZERG,35,141,255,"Mutalisk"),
    SC64_SK_T(46,SC64_SK_ZERG,35,136,255,"Defiler"),
    SC64_SK_T(47,SC64_SK_ZERG,35,141,255,"Scourge"),
    SC64_SK_T(39,SC64_SK_ZERG,35,140,255,"Ultralisk"),
    SC64_SK_T(50,SC64_SK_ZERG,130,255,255,"Infested Terran"),
    SC64_SK_U(44,SC64_SK_ZERG,43,137,255,255,"Guardian (Mutalisk morph)"),
    SC64_SK_U(62,SC64_SK_ZERG,43,137,255,255,"Devourer (Mutalisk morph)"),
    SC64_SK_U(103,SC64_SK_ZERG,38,135,255,SC64_SK_TECH_LURKER_ASPECT,"Lurker (Hydralisk morph)"),
    SC64_SK_ROW(97,SC64_SK_ZERG,SC64_SK_SUBUNIT,38,38,255,255,SC64_SK_TECH_LURKER_ASPECT,"Lurker Egg"),
    SC64_SK_S(35,SC64_SK_ZERG,131,"Zerg Larva"),
    SC64_SK_S(36,SC64_SK_ZERG,35,"Zerg Egg"),
    SC64_SK_S(59,SC64_SK_ZERG,43,"Zerg Cocoon"),
    SC64_SK_S(40,SC64_SK_ZERG,45,"Broodling (Queen spell subunit)"),
    SC64_SK_T(64,SC64_SK_PROTOSS,154,255,255,"Probe"),
    SC64_SK_T(65,SC64_SK_PROTOSS,160,255,255,"Zealot"),
    SC64_SK_T(66,SC64_SK_PROTOSS,160,164,255,"Dragoon"),
    SC64_SK_T(67,SC64_SK_PROTOSS,160,165,255,"High Templar"),
    SC64_SK_T(61,SC64_SK_PROTOSS,160,165,255,"Dark Templar"),
    SC64_SK_T(69,SC64_SK_PROTOSS,155,255,255,"Shuttle"),
    SC64_SK_T(84,SC64_SK_PROTOSS,155,159,255,"Observer"),
    SC64_SK_T(83,SC64_SK_PROTOSS,155,171,255,"Reaver"),
    SC64_SK_T(70,SC64_SK_PROTOSS,167,255,255,"Scout"),
    SC64_SK_T(60,SC64_SK_PROTOSS,167,255,255,"Corsair"),
    SC64_SK_T(72,SC64_SK_PROTOSS,167,169,255,"Carrier"),
    SC64_SK_T(71,SC64_SK_PROTOSS,167,170,255,"Arbiter"),
    SC64_SK_MERGE(68,SC64_SK_PROTOSS,67,"Archon (two High Templars merged)"),
    SC64_SK_MERGE(63,SC64_SK_PROTOSS,61,"Dark Archon (two Dark Templars merged)"),
    SC64_SK_Q(73,SC64_SK_PROTOSS,72,"Interceptor (Carrier-produced subunit)"),
    SC64_SK_Q(85,SC64_SK_PROTOSS,83,"Scarab (Reaver-produced ammunition)"),
};
static const Sc64SkirmishRecord sc64_skirmish_build_terran[] = {
    SC64_SK_B(106,SC64_SK_TERRAN,7,255,255,"Command Center"),
    SC64_SK_B(109,SC64_SK_TERRAN,7,255,255,"Supply Depot"),
    SC64_SK_B(110,SC64_SK_TERRAN,7,255,255,"Refinery"),
    SC64_SK_B(111,SC64_SK_TERRAN,7,106,255,"Barracks"),
    SC64_SK_B(112,SC64_SK_TERRAN,7,111,255,"Academy"),
    SC64_SK_B(113,SC64_SK_TERRAN,7,111,255,"Factory"),
    SC64_SK_B(114,SC64_SK_TERRAN,7,113,255,"Starport"),
    SC64_SK_B(116,SC64_SK_TERRAN,7,114,255,"Science Facility"),
    SC64_SK_B(122,SC64_SK_TERRAN,7,106,255,"Engineering Bay"),
    SC64_SK_B(123,SC64_SK_TERRAN,7,113,255,"Armory"),
    SC64_SK_B(124,SC64_SK_TERRAN,7,122,255,"Missile Turret"),
    SC64_SK_B(125,SC64_SK_TERRAN,7,111,255,"Bunker"),
    SC64_SK_A(107,SC64_SK_TERRAN,106,106,112,255,"Comsat Station addon"),
    SC64_SK_A(108,SC64_SK_TERRAN,106,106,116,117,"Nuclear Silo addon"),
    SC64_SK_A(115,SC64_SK_TERRAN,114,114,255,255,"Control Tower addon"),
    SC64_SK_A(117,SC64_SK_TERRAN,116,116,255,255,"Covert Ops addon"),
    SC64_SK_A(118,SC64_SK_TERRAN,116,116,255,255,"Physics Lab addon"),
    SC64_SK_A(120,SC64_SK_TERRAN,113,113,255,255,"Machine Shop addon"),
};
static const Sc64SkirmishRecord sc64_skirmish_build_zerg[] = {
    SC64_SK_B(131,SC64_SK_ZERG,41,255,255,"Hatchery"),
    SC64_SK_M(132,SC64_SK_ZERG,131,142,255,"Lair (Hatchery morph)"),
    SC64_SK_M(133,SC64_SK_ZERG,132,138,255,"Hive (Lair morph)"),
    SC64_SK_B(142,SC64_SK_ZERG,41,131,255,"Spawning Pool"),
    SC64_SK_B(135,SC64_SK_ZERG,41,142,255,"Hydralisk Den"),
    SC64_SK_B(139,SC64_SK_ZERG,41,131,255,"Evolution Chamber"),
    SC64_SK_B(149,SC64_SK_ZERG,41,255,255,"Extractor"),
    SC64_SK_B(138,SC64_SK_ZERG,41,132,255,"Queens Nest"),
    SC64_SK_B(141,SC64_SK_ZERG,41,132,255,"Spire"),
    SC64_SK_M(137,SC64_SK_ZERG,141,133,255,"Greater Spire (Spire morph)"),
    SC64_SK_B(136,SC64_SK_ZERG,41,133,255,"Defiler Mound"),
    SC64_SK_B(140,SC64_SK_ZERG,41,133,255,"Ultralisk Cavern"),
    SC64_SK_B(134,SC64_SK_ZERG,41,133,255,"Nydus Canal"),
    SC64_SK_B(143,SC64_SK_ZERG,41,131,255,"Creep Colony"),
    SC64_SK_M(146,SC64_SK_ZERG,143,142,255,"Sunken Colony (Creep Colony morph)"),
    SC64_SK_M(144,SC64_SK_ZERG,143,139,255,"Spore Colony (Creep Colony morph)"),
    SC64_SK_ROW(130,SC64_SK_ZERG,SC64_SK_BUILDING_MORPH,45,106,255,255,255,"Infested Command Center (Queen infestation)"),
};
static const Sc64SkirmishRecord sc64_skirmish_build_protoss[] = {
    SC64_SK_B(154,SC64_SK_PROTOSS,64,255,255,"Nexus"),
    SC64_SK_B(156,SC64_SK_PROTOSS,64,255,255,"Pylon"),
    SC64_SK_B(157,SC64_SK_PROTOSS,64,255,255,"Assimilator"),
    SC64_SK_B(160,SC64_SK_PROTOSS,64,154,255,"Gateway"),
    SC64_SK_B(166,SC64_SK_PROTOSS,64,154,255,"Forge"),
    SC64_SK_B(164,SC64_SK_PROTOSS,64,160,255,"Cybernetics Core"),
    SC64_SK_B(163,SC64_SK_PROTOSS,64,164,255,"Citadel of Adun"),
    SC64_SK_B(165,SC64_SK_PROTOSS,64,163,255,"Templar Archives"),
    SC64_SK_B(155,SC64_SK_PROTOSS,64,164,255,"Robotics Facility"),
    SC64_SK_B(171,SC64_SK_PROTOSS,64,155,255,"Robotics Support Bay"),
    SC64_SK_B(159,SC64_SK_PROTOSS,64,155,255,"Observatory"),
    SC64_SK_B(167,SC64_SK_PROTOSS,64,164,255,"Stargate"),
    SC64_SK_B(169,SC64_SK_PROTOSS,64,167,255,"Fleet Beacon"),
    SC64_SK_B(170,SC64_SK_PROTOSS,64,165,167,"Arbiter Tribunal"),
    SC64_SK_B(162,SC64_SK_PROTOSS,64,166,255,"Photon Cannon"),
    SC64_SK_B(172,SC64_SK_PROTOSS,64,160,255,"Shield Battery"),
};

enum {
    SC64_SKIRMISH_UNIT_COUNT = sizeof(sc64_skirmish_unit_catalog) / sizeof(sc64_skirmish_unit_catalog[0]),
    SC64_SKIRMISH_TERRAN_BUILD_COUNT = sizeof(sc64_skirmish_build_terran) / sizeof(sc64_skirmish_build_terran[0]),
    SC64_SKIRMISH_ZERG_BUILD_COUNT = sizeof(sc64_skirmish_build_zerg) / sizeof(sc64_skirmish_build_zerg[0]),
    SC64_SKIRMISH_PROTOSS_BUILD_COUNT = sizeof(sc64_skirmish_build_protoss) / sizeof(sc64_skirmish_build_protoss[0]),
    SC64_SKIRMISH_CATALOG_COUNT = SC64_SKIRMISH_UNIT_COUNT + SC64_SKIRMISH_TERRAN_BUILD_COUNT +
        SC64_SKIRMISH_ZERG_BUILD_COUNT + SC64_SKIRMISH_PROTOSS_BUILD_COUNT
};

static inline unsigned sc64_skirmish_record_count(void) { return SC64_SKIRMISH_CATALOG_COUNT; }
static inline const Sc64SkirmishRecord *sc64_skirmish_record_at(unsigned i) {
    if (i < SC64_SKIRMISH_UNIT_COUNT) return &sc64_skirmish_unit_catalog[i];
    i -= SC64_SKIRMISH_UNIT_COUNT;
    if (i < SC64_SKIRMISH_TERRAN_BUILD_COUNT) return &sc64_skirmish_build_terran[i];
    i -= SC64_SKIRMISH_TERRAN_BUILD_COUNT;
    if (i < SC64_SKIRMISH_ZERG_BUILD_COUNT) return &sc64_skirmish_build_zerg[i];
    i -= SC64_SKIRMISH_ZERG_BUILD_COUNT;
    return i < SC64_SKIRMISH_PROTOSS_BUILD_COUNT ? &sc64_skirmish_build_protoss[i] :
        (const Sc64SkirmishRecord *)0;
}
static inline unsigned sc64_skirmish_build_count(unsigned race) {
    switch (race) {
    case SC64_SK_TERRAN: return SC64_SKIRMISH_TERRAN_BUILD_COUNT;
    case SC64_SK_ZERG: return SC64_SKIRMISH_ZERG_BUILD_COUNT;
    case SC64_SK_PROTOSS: return SC64_SKIRMISH_PROTOSS_BUILD_COUNT;
    default: return 0;
    }
}
static inline const Sc64SkirmishRecord *sc64_skirmish_build_at(unsigned race, unsigned index) {
    switch (race) {
    case SC64_SK_TERRAN: return index < SC64_SKIRMISH_TERRAN_BUILD_COUNT ? &sc64_skirmish_build_terran[index] : 0;
    case SC64_SK_ZERG: return index < SC64_SKIRMISH_ZERG_BUILD_COUNT ? &sc64_skirmish_build_zerg[index] : 0;
    case SC64_SK_PROTOSS: return index < SC64_SKIRMISH_PROTOSS_BUILD_COUNT ? &sc64_skirmish_build_protoss[index] : 0;
    default: return 0;
    }
}
static inline unsigned sc64_skirmish_product_count(unsigned producer_raw) {
    unsigned i, n = 0;
    for (i = 0; i < SC64_SKIRMISH_UNIT_COUNT; ++i)
        if (sc64_skirmish_unit_catalog[i].producer_raw == producer_raw &&
            sc64_skirmish_unit_catalog[i].kind == SC64_SK_TRAIN) ++n;
    return n;
}
static inline const Sc64SkirmishRecord *sc64_skirmish_product_at(unsigned producer_raw, unsigned index) {
    unsigned i;
    for (i = 0; i < SC64_SKIRMISH_UNIT_COUNT; ++i) {
        const Sc64SkirmishRecord *r = &sc64_skirmish_unit_catalog[i];
        if (r->producer_raw == producer_raw && r->kind == SC64_SK_TRAIN && !index--) return r;
    }
    return (const Sc64SkirmishRecord *)0;
}

#undef SC64_SK_ROW
#undef SC64_SK_B
#undef SC64_SK_A
#undef SC64_SK_M
#undef SC64_SK_T
#undef SC64_SK_U
#undef SC64_SK_S
#undef SC64_SK_Q
#undef SC64_SK_MERGE
#endif
