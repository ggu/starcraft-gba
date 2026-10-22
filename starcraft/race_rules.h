#ifndef SC64_RACE_RULES_H
#define SC64_RACE_RULES_H

/* Compact authored rules for the skirmish races. Resource costs, build times,
 * HP/shields and doubled supply were checked against BWAPI's public default
 * tables (pinned d727fed68558c506163048ea889131d8cbb33915,
 * bwapi/bwapi/BWAPILIB/Source/UnitType.cpp). These are values only; no source
 * implementation or Blizzard data is included here. Supply uses BWAPI's
 * doubled internal units, matching game.c's legacy unit_supply convention. */
enum { R_TER, R_ZERG, R_PROTOSS };
enum {
    U_SCV, U_MARINE, U_FIREBAT, U_MEDIC, U_ZERGLING, U_HYDRALISK, U_RAYNOR,
    U_DRONE, U_OVERLORD, U_PROBE, U_ZEALOT, U_DRAGOON, U_SCOUT, U_TASSADAR,
    U_FENIX, U_CHRYSALIS, U_QUEEN, U_GOLIATH, U_TANK, U_WRAITH,
    U_MUTALISK, U_HUNTER_KILLER, U_REAVER, U_FENIX_DRAGOON, U_SIEGE_TANK,
    U_VULTURE, U_DROPSHIP, U_SCOURGE, SC_RU_COUNT
};
enum {
    B_COMMAND, B_BARRACKS, B_DEPOT, B_REFINERY, B_ACADEMY, B_ENG_BAY,
    B_HATCHERY, B_BUNKER, B_INFESTED_COMMAND, B_CREEP_COLONY,
    B_SPAWNING_POOL, B_HYDRALISK_DEN, B_EVOLUTION_CHAMBER,
    B_NEXUS, B_GATEWAY, B_CYBERNETICS_CORE, B_FORGE, B_ASSIMILATOR,
    B_PYLON, B_PHOTON_CANNON, B_EXTRACTOR, B_SUNKEN_COLONY, B_SPIRE,
    B_CEREBRATE, B_LAIR, B_STARGATE, B_STARPORT, B_MISSILE_TURRET,
    B_SPORE_COLONY, B_ZERG_BEACON, B_FACTORY, B_ROBOTICS_FACILITY,
    B_MATURE_CHRYSALIS, B_HIVE, B_GREATER_SPIRE, SC_RB_COUNT
};

enum {
    SC_RACE_F_WORKER = 1,
    SC_RACE_F_BUILDING = 2,
    SC_RACE_F_CONSUMES_WORKER = 4,
    SC_RACE_F_LARVA = 8,
    SC_RACE_F_PROVIDES_SUPPLY = 16,
    SC_RACE_F_REQUIRES_POWER = 32,
    SC_RACE_F_PRODUCER = 64,
    SC_RACE_F_SHIELDS = 128
};
enum {
    SC_PR_HATCHERY = 1u << 0, SC_PR_SPAWNING_POOL = 1u << 1,
    SC_PR_HYDRALISK_DEN = 1u << 2, SC_PR_NEXUS = 1u << 3,
    SC_PR_GATEWAY = 1u << 4, SC_PR_CYBERNETICS_CORE = 1u << 5,
    SC_PR_PYLON = 1u << 6, SC_PR_FORGE = 1u << 7,
    SC_PR_DEPOT = 1u << 8, SC_PR_BARRACKS = 1u << 9
};

typedef struct {
    unsigned char race, flags, producer, supply;
    unsigned short minerals, gas, build_frames, hp, shields;
    unsigned short prereq;
} Sc64RaceUnitRule;

typedef struct {
    unsigned char race, flags, worker, supply_provided;
    unsigned short minerals, gas, build_frames, hp;
    unsigned short prereq;
} Sc64RaceBuildingRule;

/* producer is a building ID for production units; Zerg units use B_HATCHERY
 * to signify larva production. Building workers are identified by their
 * faction's worker unit ID. Unknown legacy units return a neutral zero rule. */
static const Sc64RaceUnitRule sc64_race_unit_rules[SC_RU_COUNT] = {
    [U_SCV]       = { R_TER, SC_RACE_F_WORKER, B_COMMAND, 2, 50, 0, 300, 60, 0, 0 },
    [U_MARINE]    = { R_TER, 0, B_BARRACKS, 2, 50, 0, 360, 40, 0, 0 },
    [U_FIREBAT]   = { R_TER, 0, B_BARRACKS, 2, 50, 25, 360, 50, 0, 0 },
    [U_MEDIC]     = { R_TER, 0, B_BARRACKS, 2, 50, 25, 450, 60, 0, 0 },
    [U_ZERGLING]  = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 1, 50, 0, 420, 35, 0, SC_PR_SPAWNING_POOL },
    [U_HYDRALISK] = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 2, 75, 25, 420, 80, 0, SC_PR_HYDRALISK_DEN },
    [U_RAYNOR]    = { R_TER, 0, B_BARRACKS, 2, 0, 0, 0, 200, 0, 0 },
    [U_DRONE]     = { R_ZERG, SC_RACE_F_WORKER | SC_RACE_F_LARVA, B_HATCHERY, 2, 50, 0, 300, 40, 0, 0 },
    [U_OVERLORD]  = { R_ZERG, SC_RACE_F_LARVA | SC_RACE_F_PROVIDES_SUPPLY, B_HATCHERY, 0, 100, 0, 600, 200, 0, 0 },
    [U_PROBE]     = { R_PROTOSS, SC_RACE_F_WORKER, B_NEXUS, 2, 50, 0, 300, 20, 20, 0 },
    [U_ZEALOT]    = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 4, 100, 0, 600, 100, 60, SC_PR_GATEWAY },
    [U_DRAGOON]   = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 4, 125, 50, 750, 100, 80, SC_PR_CYBERNETICS_CORE },
    [U_SCOUT]     = { R_PROTOSS, SC_RACE_F_SHIELDS, B_STARGATE, 6, 275, 125, 1200, 150, 100, 0 },
    [U_TASSADAR]  = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 0, 100, 300, 1500, 80, 300, 0 },
    [U_FENIX]     = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 0, 200, 0, 1200, 240, 240, 0 },
    [U_CHRYSALIS] = { R_ZERG, 0, B_HATCHERY, 0, 0, 0, 0, 800, 0, 0 },
    [U_QUEEN]     = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 4, 100, 100, 750, 120, 0, SC_PR_SPAWNING_POOL },
    [U_GOLIATH]   = { R_TER, 0, B_FACTORY, 4, 100, 50, 600, 125, 0, 0 },
    [U_TANK]      = { R_TER, 0, B_FACTORY, 4, 150, 100, 750, 150, 0, 0 },
    [U_WRAITH]    = { R_TER, 0, B_STARPORT, 4, 150, 100, 900, 120, 0, 0 },
    [U_MUTALISK]  = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 4, 100, 100, 600, 120, 0, SC_PR_SPAWNING_POOL },
    [U_HUNTER_KILLER] = { R_ZERG, 0, B_HATCHERY, 0, 150, 50, 780, 160, 0, SC_PR_HYDRALISK_DEN },
    [U_REAVER] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_ROBOTICS_FACILITY, 8, 200, 100, 1050, 100, 80, 0 },
    [U_FENIX_DRAGOON] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 0, 300, 100, 1500, 240, 240, 0 },
    /* Siege Mode is an existing Tank transformed in place, not a factory
     * product; its source-type conversion therefore has no training cost. */
    [U_SIEGE_TANK] = { R_TER, 0, B_FACTORY, 4, 0, 0, 0, 150, 0, 0 },
    [U_VULTURE] = { R_TER, 0, B_FACTORY, 4, 75, 0, 450, 80, 0, 0 },
    /* BWAPI raw 11 Dropship. Transport behavior is not implemented here. */
    [U_DROPSHIP] = { R_TER, 0, B_STARPORT, 4, 100, 100, 750, 150, 0, 0 },
    /* BWAPI raw 47 Scourge: 25 HP, 25/75, 1 doubled supply. Its air-only
     * Suicide Scourge weapon is enabled only for the Higher Ground profile. */
    [U_SCOURGE] = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 1, 25, 75, 450, 25, 0, 0 }
};

static const Sc64RaceBuildingRule sc64_race_building_rules[SC_RB_COUNT] = {
    [B_COMMAND] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_PROVIDES_SUPPLY, U_SCV, 20, 400, 0, 1800, 1500, 0 },
    [B_BARRACKS] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER, U_SCV, 0, 150, 0, 1200, 1000, SC_PR_DEPOT },
    [B_DEPOT] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PROVIDES_SUPPLY, U_SCV, 16, 100, 0, 600, 500, 0 },
    [B_REFINERY] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 100, 0, 600, 750, 0 },
    [B_ACADEMY] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 150, 0, 1200, 600, 0 },
    [B_ENG_BAY] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 125, 0, 900, 850, 0 },
    [B_BUNKER] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 100, 0, 450, 350, 0 },
    [B_INFESTED_COMMAND] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 0, 0, 0, 1500, 0 },
    [B_HATCHERY] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_PROVIDES_SUPPLY | SC_RACE_F_CONSUMES_WORKER, U_DRONE, 2, 300, 0, 1800, 1250, 0 },
    [B_SPAWNING_POOL] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_CONSUMES_WORKER, U_DRONE, 0, 200, 0, 1200, 750, SC_PR_HATCHERY },
    [B_HYDRALISK_DEN] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_CONSUMES_WORKER, U_DRONE, 0, 100, 50, 600, 850, SC_PR_SPAWNING_POOL },
    [B_EVOLUTION_CHAMBER] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_CONSUMES_WORKER, U_DRONE, 0, 75, 0, 600, 750, SC_PR_HATCHERY },
    [B_EXTRACTOR] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_CONSUMES_WORKER, U_DRONE, 0, 50, 0, 600, 750, SC_PR_HATCHERY },
    [B_NEXUS] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_PROVIDES_SUPPLY, U_PROBE, 18, 400, 0, 1800, 750, 0 },
    [B_GATEWAY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 150, 0, 900, 500, SC_PR_PYLON },
    [B_CYBERNETICS_CORE] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 200, 0, 900, 500, SC_PR_GATEWAY | SC_PR_PYLON },
    [B_FORGE] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 150, 0, 600, 550, SC_PR_PYLON },
    [B_ASSIMILATOR] = { R_PROTOSS, SC_RACE_F_BUILDING, U_PROBE, 0, 100, 0, 600, 450, 0 },
    [B_PYLON] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_PROVIDES_SUPPLY, U_PROBE, 16, 100, 0, 450, 300, 0 },
    [B_PHOTON_CANNON] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 150, 0, 750, 100, SC_PR_FORGE | SC_PR_PYLON },
    [B_CREEP_COLONY] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_CONSUMES_WORKER, U_DRONE, 0, 75, 0, 300, 400, SC_PR_HATCHERY },
    [B_SUNKEN_COLONY] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 50, 0, 300, 300, SC_PR_SPAWNING_POOL },
    [B_SPIRE] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_CONSUMES_WORKER, U_DRONE, 0, 200, 150, 1800, 600, SC_PR_SPAWNING_POOL },
    [B_CEREBRATE] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 0, 0, 0, 1500, 0 },
    [B_LAIR] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_PROVIDES_SUPPLY, U_DRONE, 2, 150, 100, 1500, 1800, SC_PR_SPAWNING_POOL },
    [B_STARGATE] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 150, 150, 1050, 600, SC_PR_CYBERNETICS_CORE | SC_PR_PYLON },
    [B_STARPORT] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER, U_SCV, 0, 150, 100, 1050, 1300, 0 },
    [B_MISSILE_TURRET] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 75, 0, 450, 200, 0 },
    [B_SPORE_COLONY] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 50, 0, 300, 400, SC_PR_HATCHERY },
    [B_ZERG_BEACON] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 250, 0, 0, 10000, 0 },
    [B_FACTORY] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER, U_SCV, 0, 200, 100, 1200, 1250, SC_PR_BARRACKS },
    [B_ROBOTICS_FACILITY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 200, 200, 1200, 500, SC_PR_CYBERNETICS_CORE | SC_PR_PYLON },
    /* BWAPI raw 150 Mature Chrysalis: static mission building, not a unit. */
    [B_MATURE_CHRYSALIS] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 0, 0, 0, 250, 0 },
    [B_HIVE] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_PROVIDES_SUPPLY, U_DRONE, 2, 200, 150, 1800, 2500, 0 },
    [B_GREATER_SPIRE] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 100, 150, 1800, 1000, 0 }
};

static inline const Sc64RaceUnitRule *sc64_race_unit_rule(int type)
{ return type >= 0 && type < SC_RU_COUNT ? &sc64_race_unit_rules[type] : 0; }
static inline const Sc64RaceBuildingRule *sc64_race_building_rule(int type)
{ return type >= 0 && type < SC_RB_COUNT ? &sc64_race_building_rules[type] : 0; }
static inline int sc64_race_is_worker(int type)
{ const Sc64RaceUnitRule *r = sc64_race_unit_rule(type); return r && (r->flags & SC_RACE_F_WORKER); }
static inline unsigned short sc64_race_unit_supply_provided(int type)
{ return type == U_OVERLORD ? 16 : 0; }
/* A completed Hatchery, Lair, or Hive is a Zerg town hall. Lairs and Hives
 * retain the Hatchery's supply, larva, creep, and larva-unit production roles. */
static inline int sc64_race_is_zerg_townhall(int type)
{ return type == B_HATCHERY || type == B_LAIR || type == B_HIVE; }
static inline int sc64_race_producer_compatible(int building, int producer)
{ return producer == B_HATCHERY ? sc64_race_is_zerg_townhall(building) : building == producer; }

static const char *const sc64_race_names[3] = { "TERRAN", "ZERG", "PROTOSS" };
static inline const char *sc64_race_unit_name(int type)
{
    static const char *const names[SC_RU_COUNT] = {
        "SCV","MARINE","FIREBAT","MEDIC","ZERGLING","HYDRALISK","RAYNOR",
        "DRONE","OVERLORD","PROBE","ZEALOT","DRAGOON","SCOUT","TASSADAR",
        "FENIX","CHRYSALIS","QUEEN","GOLIATH","SIEGE TANK","WRAITH",
        "MUTALISK","HUNTER KILLER","REAVER","FENIX DRAGOON","SIEGE TANK",
        "VULTURE","DROPSHIP","SCOURGE"
    };
    return type >= 0 && type < SC_RU_COUNT ? names[type] : "UNKNOWN";
}
static inline const char *sc64_race_building_name(int type)
{
    static const char *const names[SC_RB_COUNT] = {
        "COMMAND CENTER","BARRACKS","SUPPLY DEPOT","REFINERY","ACADEMY","ENGINEERING BAY",
        "HATCHERY","BUNKER","INFESTED COMMAND CENTER","CREEP COLONY","SPAWNING POOL",
        "HYDRALISK DEN","EVOLUTION CHAMBER","NEXUS","GATEWAY","CYBERNETICS CORE","FORGE",
        "ASSIMILATOR","PYLON","PHOTON CANNON","EXTRACTOR","SUNKEN COLONY","SPIRE",
        "CEREBRATE","LAIR","STARGATE","STARPORT","MISSILE TURRET","SPORE COLONY",
        "ZERG BEACON","FACTORY","ROBOTICS FACILITY","MATURE CHRYSALIS",
        "HIVE","GREATER SPIRE"
    };
    return type >= 0 && type < SC_RB_COUNT ? names[type] : "UNKNOWN";
}

static inline unsigned short sc64_race_building_shields(int type)
{
    switch (type) {
    case B_NEXUS: return 750; case B_GATEWAY: return 500;
    case B_CYBERNETICS_CORE: return 500; case B_FORGE: return 550;
    case B_ASSIMILATOR: return 450; case B_PYLON: return 300;
    case B_PHOTON_CANNON: return 100; case B_STARGATE: return 600;
    default: return 0;
    }
}

/* Combat tuning uses BWAPI public default damage/range/cooldown facts and
 * retains pixel ranges and simulation-frame cooldowns. */
static inline int sc64_race_unit_damage(int type)
{
    switch (type) {
    case U_SCV: case U_DRONE: case U_PROBE: return 5;
    case U_MARINE: return 6; case U_FIREBAT: return 8;
    case U_ZERGLING: return 5; case U_HYDRALISK: return 10;
    case U_ZEALOT: return 8; case U_DRAGOON: return 20;
    case U_GOLIATH: return 12; case U_TANK: return 30;
    case U_WRAITH: return 8; case U_MUTALISK: return 9;
    case U_HUNTER_KILLER: return 20; case U_REAVER: return 100;
    case U_FENIX: return 20; case U_FENIX_DRAGOON: return 45;
    case U_TASSADAR: return 20; case U_SCOUT: return 8;
    case U_SIEGE_TANK: return 50;
    case U_VULTURE: return 20;
    default: return 0;
    }
}

static inline int sc64_race_unit_range(int type)
{
    switch (type) {
    case U_WRAITH: case U_VULTURE: return 160;
    case U_GOLIATH: return 192;
    case U_TANK: return 224;
    case U_MARINE: case U_HYDRALISK: case U_DRAGOON: case U_SCOUT:
    case U_FENIX_DRAGOON: case U_HUNTER_KILLER: return 128;
    case U_TASSADAR: return 96;
    case U_SIEGE_TANK: return 384;
    case U_MUTALISK: return 96; case U_REAVER: return 256;
    case U_FIREBAT: return 32; case U_ZEALOT: case U_FENIX: return 15;
    case U_SCV: case U_DRONE: case U_PROBE: case U_ZERGLING: return 15;
    default: return 0;
    }
}

static inline int sc64_race_unit_cooldown(int type)
{
    switch (type) {
    case U_ZERGLING: return 8; case U_MARINE: return 15;
    case U_VULTURE: return 30;
    case U_HYDRALISK: return 15; case U_FIREBAT: return 22;
    case U_ZEALOT: return 22; case U_DRAGOON: return 30;
    case U_GOLIATH: return 22; case U_TANK: return 37;
    case U_WRAITH: return 30; case U_MUTALISK: return 30;
    case U_REAVER: return 60; case U_SCOUT: return 30;
    case U_TASSADAR: return 22; case U_FENIX_DRAGOON: return 22;
    case U_SIEGE_TANK: return 75;
    default: return 15;
    }
}

static inline int sc64_race_unit_flying(int type)
{ return type == U_OVERLORD || type == U_WRAITH || type == U_MUTALISK || type == U_SCOUT || type == U_QUEEN || type == U_DROPSHIP || type == U_SCOURGE; }

/* Ground and air weapons can differ on the same unit. Damage is per hit;
 * expose the number of simultaneous blade hits so armor is applied per hit. */
static inline int sc64_race_unit_damage_vs(int type, int target_flying)
{
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    /* Pinned BWAPI Suicide_Scourge: 110 normal damage, cooldown 1, 3px
     * maximum range, air-only. game.c applies the engine's 2:1 scale. */
    if (type == U_SCOURGE) return target_flying ? 110 : 0;
#endif
    if (type == U_VULTURE && target_flying) return 0;
    if (target_flying) {
        if (type == U_GOLIATH) return 10;
        if (type == U_WRAITH) return 20;
        if (type == U_SCOUT) return 14;
    }
    return sc64_race_unit_damage(type);
}
static inline int sc64_race_unit_range_vs(int type, int target_flying)
{
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if (type == U_SCOURGE) return target_flying ? 3 : 0;
#endif
    if (type == U_VULTURE && target_flying) return 0;
    if (type == U_GOLIATH) return target_flying ? 160 : 192;
    if (type == U_TANK || type == U_SIEGE_TANK) return target_flying ? 0 : sc64_race_unit_range(type);
    if (type == U_WRAITH) return 160;
    if (type == U_SCOUT) return target_flying ? 128 : 128;
    return sc64_race_unit_range(type);
}
static inline int sc64_race_unit_cooldown_vs(int type, int target_flying)
{
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if (type == U_SCOURGE) return target_flying ? 1 : 0;
#endif
    if (type == U_WRAITH && target_flying) return 22;
    if (type == U_SCOUT && target_flying) return 22;
    return sc64_race_unit_cooldown(type);
}
static inline int sc64_race_unit_hits_vs(int type, int target_flying)
{
    return !target_flying && (type == U_ZEALOT || type == U_FENIX) ? 2 : 1;
}
static inline int sc64_race_unit_armor(int type)
{
    switch (type) {
    case U_FIREBAT: case U_GOLIATH: case U_TANK: case U_SIEGE_TANK: case U_DROPSHIP:
    case U_ZEALOT: case U_DRAGOON: return 1;
    case U_FENIX_DRAGOON: return 3;
    case U_HUNTER_KILLER: case U_FENIX: case U_TASSADAR: return 2;
    case U_MEDIC: return 1;
    default: return 0;
    }
}
static inline int sc64_race_building_armor(int type)
{
    switch (type) {
    case B_CREEP_COLONY: case B_SPORE_COLONY: case B_PYLON:
    case B_PHOTON_CANNON: case B_MISSILE_TURRET: case B_ZERG_BEACON: return 0;
    case B_SUNKEN_COLONY: return 2;
    case B_COMMAND: case B_BARRACKS: case B_DEPOT: case B_REFINERY:
    case B_ACADEMY: case B_ENG_BAY: case B_BUNKER: case B_HATCHERY:
    case B_INFESTED_COMMAND: case B_SPAWNING_POOL: case B_HYDRALISK_DEN:
    case B_EVOLUTION_CHAMBER: case B_NEXUS: case B_GATEWAY:
    case B_CYBERNETICS_CORE: case B_FORGE: case B_ASSIMILATOR:
    case B_EXTRACTOR: case B_SPIRE: case B_CEREBRATE: case B_LAIR:
    case B_STARGATE: case B_STARPORT: case B_FACTORY:
    case B_ROBOTICS_FACILITY: case B_MATURE_CHRYSALIS: case B_HIVE: case B_GREATER_SPIRE: return 1;
    default: return 0;
    }
}

static inline int sc64_race_raw_unit(int type)
{
    switch (type) {
    case U_MARINE: return 0; case U_SCV: return 7; case U_FIREBAT: return 32;
    case U_MEDIC: return 34; case U_RAYNOR: return 20; case U_ZERGLING: return 37;
    case U_HYDRALISK: return 38; case U_DRONE: return 41; case U_OVERLORD: return 42;
    case U_MUTALISK: return 43; case U_QUEEN: return 45; case U_HUNTER_KILLER: return 53;
    case U_PROBE: return 64; case U_ZEALOT: return 65; case U_DRAGOON: return 66;
    case U_SCOUT: return 70; case U_FENIX: return 77; case U_FENIX_DRAGOON: return 78;
    case U_TASSADAR: return 79; case U_REAVER: return 83; case U_GOLIATH: return 3;
    case U_TANK: return 5; case U_SIEGE_TANK: return 30;
    case U_WRAITH: return 8; case U_CHRYSALIS: return 216;
    case U_VULTURE: return 2; case U_DROPSHIP: return 11; case U_SCOURGE: return 47;
    default: return -1;
    }
}

static inline int sc64_race_raw_building(int type)
{
    switch (type) {
    case B_COMMAND: return 106; case B_BARRACKS: return 111;
    case B_DEPOT: return 109; case B_REFINERY: return 110; case B_ACADEMY: return 112;
    case B_ENG_BAY: return 122; case B_BUNKER: return 125;
    case B_INFESTED_COMMAND: return 130; case B_HATCHERY: return 131; case B_LAIR: return 132;
    case B_HYDRALISK_DEN: return 135; case B_EVOLUTION_CHAMBER: return 139;
    case B_SPIRE: return 141; case B_SPAWNING_POOL: return 142; case B_CREEP_COLONY: return 143;
    case B_SPORE_COLONY: return 144; case B_SUNKEN_COLONY: return 146; case B_EXTRACTOR: return 149;
    case B_CEREBRATE: return 151; case B_NEXUS: return 154; case B_ROBOTICS_FACILITY: return 155;
    case B_MATURE_CHRYSALIS: return 150; case B_HIVE: return 133;
    case B_GREATER_SPIRE: return 137;
    case B_PYLON: return 156; case B_ASSIMILATOR: return 157; case B_GATEWAY: return 160;
    case B_PHOTON_CANNON: return 162; case B_CYBERNETICS_CORE: return 164;
    case B_FORGE: return 166; case B_STARGATE: return 167; case B_FACTORY: return 113;
    case B_STARPORT: return 114; case B_MISSILE_TURRET: return 124; case B_ZERG_BEACON: return 194;
    default: return -1;
    }
}

/* Public BWAPI UnitType enum mapping, pinned to d727fed... UnitType.h.
 * Scenario importers can keep raw IDs in their own records while gameplay
 * uses the compact appended IDs above. */
static inline int sc64_race_unit_from_raw(int raw)
{
    switch (raw) {
    case 0: return U_MARINE; case 7: return U_SCV; case 32: return U_FIREBAT;
    case 34: return U_MEDIC; case 20: return U_RAYNOR;
#ifdef SC64_UNIFIED
    case 19: return U_RAYNOR; /* Terran tutorial's Raynor Vulture. */
#endif
    case 37: return U_ZERGLING; case 38: return U_HYDRALISK;
    case 41: return U_DRONE; case 42: return U_OVERLORD; case 45: return U_QUEEN;
    case 64: return U_PROBE; case 65: return U_ZEALOT; case 66: return U_DRAGOON;
    case 70: return U_SCOUT; case 77: return U_FENIX;
    case 78: return U_FENIX_DRAGOON; case 83: return U_REAVER;
    case 79: return U_TASSADAR;
    case 2: return U_VULTURE;
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    case 11: return U_DROPSHIP;
#endif
    case 3: return U_GOLIATH; case 5: return U_TANK; case 30: return U_SIEGE_TANK;
    case 8: return U_WRAITH; case 43: return U_MUTALISK;
    case 53: return U_HUNTER_KILLER; case 216: return U_CHRYSALIS;
    case 47: return U_SCOURGE;
    default: return -1;
    }
}

static inline int sc64_race_building_from_raw(int raw)
{
    switch (raw) {
    case 106: return B_COMMAND; case 111: return B_BARRACKS;
    case 109: return B_DEPOT; case 110: return B_REFINERY;
#ifdef SC64_UNIFIED
    case 112: return B_ACADEMY; case 125: return B_BUNKER;
    case 130: return B_INFESTED_COMMAND;
#endif
    case 122: return B_ENG_BAY;
    case 131: return B_HATCHERY; case 132: return B_LAIR;
    case 135: return B_HYDRALISK_DEN; case 139: return B_EVOLUTION_CHAMBER;
    case 141: return B_SPIRE; case 142: return B_SPAWNING_POOL;
    case 143: return B_CREEP_COLONY; case 146: return B_SUNKEN_COLONY;
    case 149: return B_EXTRACTOR; case 151: return B_CEREBRATE;
    case 154: return B_NEXUS; case 156: return B_PYLON;
    case 157: return B_ASSIMILATOR; case 160: return B_GATEWAY;
    case 162: return B_PHOTON_CANNON; case 164: return B_CYBERNETICS_CORE;
    case 113: return B_FACTORY; case 114: return B_STARPORT; case 124: return B_MISSILE_TURRET;
    case 155: return B_ROBOTICS_FACILITY;
    case 144: return B_SPORE_COLONY; case 194: return B_ZERG_BEACON;
    case 166: return B_FORGE; case 167: return B_STARGATE;
    case 150: return B_MATURE_CHRYSALIS;
    case 133: return B_HIVE; case 137: return B_GREATER_SPIRE;
    default: return -1;
    }
}

#endif
