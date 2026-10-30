#ifndef SC64_RACE_RULES_H
#define SC64_RACE_RULES_H

#if defined(SC64_Z5_ACTORS) && !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_Z5_ACTORS requires SC64_ZP_CAMPAIGN_ACTORS"
#endif
#if defined(SC64_Z5_DOODADS) && (!defined(SC64_Z5_ACTORS) || !defined(SC64_ZP_CAMPAIGN_ACTORS))
#error "SC64_Z5_DOODADS requires SC64_Z5_ACTORS and SC64_ZP_CAMPAIGN_ACTORS"
#endif
#if defined(SC64_Z6_ACTORS) && !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_Z6_ACTORS requires SC64_ZP_CAMPAIGN_ACTORS"
#endif
#if defined(SC64_P5_ACTORS) && !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_P5_ACTORS requires SC64_ZP_CAMPAIGN_ACTORS"
#endif
#if defined(SC64_P6_ACTORS) && !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_P6_ACTORS requires SC64_ZP_CAMPAIGN_ACTORS"
#endif
#if defined(SC64_P7_ACTORS) && !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_P7_ACTORS requires SC64_ZP_CAMPAIGN_ACTORS"
#endif

#ifdef SC64_T6_ENABLED
#include "sc64_t6_actor_rules.h"
#endif

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
    U_VULTURE, U_DROPSHIP, U_SCOURGE,
#ifdef SC64_T4_RUNTIME
    /* T4-only actor IDs: props and neutral actors must not alias a combat unit. */
    U_GHOST, U_CIVILIAN, U_RHYNADON, U_RAGNASAUR, U_MAP_REVEALER, U_DATA_DISK,
    /* Raw 20 is Hero_Jim_Raynor_Marine; raw 19 remains the old Vulture actor. */
    U_RAYNOR_MARINE,
#endif
#ifdef SC64_T5_RUNTIME
    /* T5 distinguishes the raw-19 Raynor Vulture from legacy U_RAYNOR. */
    U_KERRIGAN, U_RAYNOR_VULTURE, U_SPIDER_MINE,
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    /* Appended later-campaign raw identities; earlier enum values stay fixed. */
    U_SCIENCE_VESSEL, U_BATTLECRUISER, U_NORAD_II, U_ULTRALISK, U_SHUTTLE,
    U_PSI_EMITTER,
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    /* Expanded Zerg/Protoss campaign identities; independent of Terran gates. */
    U_ZP_INFESTED_KERRIGAN, U_ZP_MAP_REVEALER, U_ZP_RAYNOR_VULTURE,
    U_ZP_HIGH_TEMPLAR, U_ZP_ULTRALISK,
#endif
#ifdef SC64_Z5_ACTORS
    /* Z5 actor IDs are separate from older partial T4 mappings. */
    U_Z5_GHOST, U_Z5_CIVILIAN, U_Z5_RHYNADON, U_Z5_BENGALAAS,
    U_Z5_RAGNASAUR, U_Z5_DATA_DISK,
#endif
#ifdef SC64_Z6_ACTORS
    /* Raw 44 Guardian is distinct from ordinary Mutalisk gameplay identity. */
    U_Z6_GUARDIAN,
#endif
#ifdef SC64_P5_ACTORS
    /* P5-only battlecruiser and Shuttle preserve their own scenario identities. */
    U_P5_SHUTTLE, U_P5_BATTLECRUISER,
#endif
#ifdef SC64_P6_ACTORS
    /* P6 scenario identities are appended; special combat remains unsupported. */
    U_P6_INFESTED_TERRAN, U_P6_HERO_DARK_TEMPLAR, U_P6_ZERATUL,
#endif
#ifdef SC64_P7_ACTORS
    /* P7-scoped Protoss identities stay separate from Terran-late aliases. */
    U_P7_ARCHON, U_P7_ARBITER, U_P7_CARRIER, U_P7_OBSERVER,
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    /* Raw 36 is a real, independently ticking Zerg Egg actor. */
    U_Z7_EGG,
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    /* Independent raw73 identity; child lifecycle is a separate feature. */
    U_P7_INTERCEPTOR,
#endif
#ifdef SC64_P8_DATA_ENABLED
    /* P8 source raw 28 is a distinct actor despite sharing raw12 source art. */
    U_P8_HYPERION,
#endif
#ifdef SC64_SKIRMISH_ROSTER
    /* Missing ordinary catalog identities are appended only in the opt-in
     * skirmish roster profile; existing campaign aliases keep their IDs. */
#ifndef SC64_TERRAN_LATE_ACTORS
    U_SK_SCIENCE_VESSEL,
#ifndef SC64_P5_ACTORS
    U_SK_BATTLECRUISER,
#endif
#endif
#ifndef SC64_Z6_ACTORS
    U_SK_GUARDIAN,
#endif
    U_SK_DEFILER, U_SK_DEVOURER, U_SK_LURKER,
#ifndef SC64_P6_ACTORS
    U_SK_INFESTED_TERRAN,
#endif
    U_SK_VALKYRIE,
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    U_SK_HIGH_TEMPLAR,
#endif
    U_SK_DARK_TEMPLAR,
#ifndef SC64_P7_ACTORS
    U_SK_OBSERVER, U_SK_CARRIER, U_SK_ARBITER, U_SK_ARCHON,
#endif
    U_SK_CORSAIR, U_SK_DARK_ARCHON,
#ifndef SC64_TERRAN_LATE_ACTORS
#ifndef SC64_P5_ACTORS
    U_SK_SHUTTLE,
#endif
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
    U_SHARED_BATTLECRUISER,
#endif
    SC_RU_COUNT
};
enum {
    B_COMMAND, B_BARRACKS, B_DEPOT, B_REFINERY, B_ACADEMY, B_ENG_BAY,
    B_HATCHERY, B_BUNKER, B_INFESTED_COMMAND, B_CREEP_COLONY,
    B_SPAWNING_POOL, B_HYDRALISK_DEN, B_EVOLUTION_CHAMBER,
    B_NEXUS, B_GATEWAY, B_CYBERNETICS_CORE, B_FORGE, B_ASSIMILATOR,
    B_PYLON, B_PHOTON_CANNON, B_EXTRACTOR, B_SUNKEN_COLONY, B_SPIRE,
    B_CEREBRATE, B_LAIR, B_STARGATE, B_STARPORT, B_MISSILE_TURRET,
    B_SPORE_COLONY, B_ZERG_BEACON, B_FACTORY, B_ROBOTICS_FACILITY,
    B_MATURE_CHRYSALIS, B_HIVE, B_GREATER_SPIRE,
#ifdef SC64_T4_RUNTIME
    B_TERRAN_BEACON,
    /* Appended T4 special-unit rules. Doors and traps use the building pool
       in this compact runtime while retaining their source raw identities. */
    B_FLOOR_MISSILE_TRAP, B_UPPER_DOOR, B_RIGHT_UPPER_DOOR,
    B_PIT_DOOR, B_RIGHT_PIT_DOOR, B_FLOOR_GUN_TRAP,
    B_WALL_FLAME_TRAP, B_RIGHT_WALL_MISSILE_TRAP,
#endif
#ifdef SC64_T5_RUNTIME
    /* Raw 115: the attached Terran Control Tower add-on. */
    B_CONTROL_TOWER,
#endif
#ifdef SC64_T6_ENABLED
    B_CRASHED_NORAD_II, B_QUEENS_NEST,
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    B_NUCLEAR_SILO, B_COVERT_OPS, B_MACHINE_SHOP, B_ARMORY,
    B_DEFILER_MOUND, B_ULTRALISK_CAVERN, B_CITADEL_ADUN,
    B_STASIS_CELL_PRISON, B_FLEET_BEACON, B_ROBOTICS_SUPPORT_BAY,
    B_SHIELD_BATTERY, B_SPECIAL_ION_CANNON,
    B_SCIENCE_FACILITY, B_PHYSICS_LAB,
#elif defined(SC64_P8_DATA_ENABLED)
    /* P8 source raw 168: the scenario's objective Stasis Cell. */
    B_STASIS_CELL_PRISON,
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    B_ZP_CONTROL_TOWER, B_ZP_MACHINE_SHOP, B_ZP_ARMORY,
    B_ZP_QUEENS_NEST, B_ZP_ULTRALISK_CAVERN, B_ZP_PROTOSS_BEACON,
#endif
#ifdef SC64_Z5_ACTORS
    B_Z5_TERRAN_BEACON,
#endif
#ifdef SC64_Z5_DOODADS
    B_Z5_FLOOR_MISSILE_TRAP, B_Z5_UPPER_DOOR, B_Z5_RIGHT_UPPER_DOOR,
    B_Z5_PIT_DOOR, B_Z5_RIGHT_PIT_DOOR, B_Z5_FLOOR_GUN_TRAP,
    B_Z5_WALL_FLAME_TRAP, B_Z5_RIGHT_WALL_FLAME_TRAP,
#endif
#ifdef SC64_P6_ACTORS
    /* Raw 210 is native UnitType, but imported as a bounded GBA prop. */
    B_P6_WALL_MISSILE_TRAP,
#endif
#ifdef SC64_P7_ACTORS
    /* P7 structures get separate identities from optional late-Terran rows. */
    B_P7_OBSERVATORY, B_P7_CITADEL_ADUN, B_P7_TEMPLAR_ARCHIVES,
    B_P7_FLEET_BEACON, B_P7_ARBITER_TRIBUNAL,
    B_P7_ROBOTICS_SUPPORT_BAY, B_P7_SHIELD_BATTERY,
#endif
#if defined(SC64_Z6_RUNTIME) && !defined(SC64_TERRAN_LATE_ACTORS)
    /* Z6 starting structures; keep earlier actor/profile enum values intact. */
    B_Z6_ROBOTICS_SUPPORT_BAY, B_Z6_SHIELD_BATTERY,
#endif
#ifdef SC64_NYDUS_ENABLED
    /* Raw 134, appended last so every disabled profile keeps its values. */
    B_NYDUS_CANAL,
#endif
#ifdef SC64_SKIRMISH_ROSTER
    /* Raw-ID mappings missing from this compile profile, appended after all
     * legacy and campaign-specific building identities. */
    B_SK_COMSAT,
#ifndef SC64_NYDUS_ENABLED
    B_SK_NYDUS,
#endif
#ifndef SC64_P7_ACTORS
#ifndef SC64_TERRAN_LATE_ACTORS
    B_SK_CITADEL_ADUN, B_SK_FLEET_BEACON,
#endif
    B_SK_TEMPLAR_ARCHIVES, B_SK_OBSERVATORY, B_SK_ARBITER_TRIBUNAL,
#endif
#ifndef SC64_TERRAN_LATE_ACTORS
    B_SK_SCIENCE_FACILITY, B_SK_NUCLEAR_SILO, B_SK_COVERT_OPS,
    B_SK_PHYSICS_LAB, B_SK_DEFILER_MOUND,
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS)
    B_SHARED_SCIENCE_FACILITY, B_SHARED_PHYSICS_LAB,
#endif
    SC_RB_COUNT
};

#ifdef SC64_SKIRMISH_ROSTER
_Static_assert(SC_RU_COUNT <= 64, "skirmish roster unit types remain within six-bit save capacity");
_Static_assert(SC_RB_COUNT <= 128, "skirmish roster building types remain within seven-bit save capacity");
#endif

#if defined(SC64_TERRAN_LATE_ACTORS) || defined(SC64_T4_RUNTIME)
static inline int sc64_race_raw_unit(int type);
static inline int sc64_race_raw_building(int type);
#endif
#ifdef SC64_T4_RUNTIME
#include "sc64_source_sight_tiles.h"
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
#include "sc64_terran_late_actor_rules.h"
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
#include "sc64_zp_actor_rules.h"
#endif
#ifdef SC64_Z6_ACTORS
#include "sc64_z6_actor_rules.h"
#endif

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
#if defined(SC64_ZP_CAMPAIGN_STORAGE) || defined(SC64_SHARED_SCOURGE_COMBAT)
    , SC_PR_SPIRE = 1u << 12, SC_PR_LAIR = 1u << 13
#endif
#ifdef SC64_T5_RUNTIME
    , SC_PR_FACTORY = 1u << 10, SC_PR_CONTROL_TOWER = 1u << 11
#endif
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
    /* Legacy Raynor is a scenario alias: its zero purchase price/time and
       doubled supply policy are retained for the old early-mission loader. */
    [U_RAYNOR]    = { R_TER, 0, B_BARRACKS, 2, 0, 0, 0,
#ifdef SC64_T4_RUNTIME
        300,
#else
        200,
#endif
        0, 0 },
    [U_DRONE]     = { R_ZERG, SC_RACE_F_WORKER | SC_RACE_F_LARVA, B_HATCHERY, 2, 50, 0, 300, 40, 0, 0 },
    [U_OVERLORD]  = { R_ZERG, SC_RACE_F_LARVA | SC_RACE_F_PROVIDES_SUPPLY, B_HATCHERY, 0, 100, 0, 600, 200, 0, 0 },
    [U_PROBE]     = { R_PROTOSS, SC_RACE_F_WORKER, B_NEXUS, 2, 50, 0, 300, 20, 20, 0 },
    [U_ZEALOT]    = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 4, 100, 0, 600, 100, 60, SC_PR_GATEWAY },
    [U_DRAGOON]   = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 4, 125, 50, 750, 100, 80, SC_PR_CYBERNETICS_CORE },
    [U_SCOUT]     = { R_PROTOSS, SC_RACE_F_SHIELDS, B_STARGATE, 6, 275, 125, 1200, 150, 100, 0 },
    [U_TASSADAR]  = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 0, 100, 300, 1500, 80, 300, 0 },
    [U_FENIX]     = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 0, 200, 0, 1200, 240, 240, 0 },
    /* Mission powerup only: the source's 1/1/1 sentinel costs are not a
       production offer, so keep the authored zero-cost, nonproducer policy. */
    [U_CHRYSALIS] = { R_ZERG, 0, B_HATCHERY, 0, 0, 0, 0, 800, 0, 0 },
    [U_QUEEN]     = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 4, 100, 100, 750, 120, 0, SC_PR_SPAWNING_POOL },
    [U_GOLIATH]   = { R_TER, 0, B_FACTORY, 4, 100, 50, 600, 125, 0, 0 },
    [U_TANK]      = { R_TER, 0, B_FACTORY, 4, 150, 100, 750, 150, 0, 0 },
    [U_WRAITH]    = { R_TER, 0, B_STARPORT, 4, 150, 100, 900, 120, 0, 0 },
    [U_MUTALISK]  = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 4, 100, 100, 600, 120, 0,
#ifdef SC64_ZP_CAMPAIGN_STORAGE
        SC_PR_SPIRE
#else
        SC_PR_SPAWNING_POOL
#endif
    },
    [U_HUNTER_KILLER] = { R_ZERG, 0, B_HATCHERY, 0, 150, 50, 780, 160, 0, SC_PR_HYDRALISK_DEN },
    [U_REAVER] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_ROBOTICS_FACILITY, 8, 200, 100, 1050, 100, 80, 0 },
    [U_FENIX_DRAGOON] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 0, 300, 100, 1500, 240, 240, 0 },
    /* Siege Mode is an existing Tank transformed in place, not a factory
     * product; its source-type conversion therefore has no training cost. */
    [U_SIEGE_TANK] = { R_TER, 0, B_FACTORY, 4, 0, 0, 0, 150, 0, 0 },
    [U_VULTURE] = { R_TER, 0, B_FACTORY, 4, 75, 0, 450, 80, 0, 0 },
    /* BWAPI raw 11 Dropship. Cargo behavior is opt-in with the T5 runtime. */
    [U_DROPSHIP] = { R_TER, 0, B_STARPORT, 4, 100, 100, 750, 150, 0,
#ifdef SC64_T5_RUNTIME
        SC_PR_CONTROL_TOWER
#else
        0
#endif
    },
    /* BWAPI raw 47 Scourge: 25 HP, 25/75, 1 doubled supply. */
    [U_SCOURGE] = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 1, 25, 75, 450, 25, 0,
#if defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
        SC_PR_SPIRE
#else
        0
#endif
    }
#ifdef SC64_T4_RUNTIME
    , [U_GHOST]        = { R_TER, 0, B_BARRACKS, 2, 25, 75, 750, 45, 0, 0 }
    /* Scenario actors retain default HP but are not normal production units. */
    , [U_CIVILIAN]     = { R_TER, 0, 0, 0, 0, 0, 0, 40, 0, 0 }
    , [U_RHYNADON]     = { R_TER, 0, 0, 0, 0, 0, 0, 60, 0, 0 }
    , [U_RAGNASAUR]    = { R_TER, 0, 0, 0, 0, 0, 0, 60, 0, 0 }
    , [U_MAP_REVEALER] = { R_TER, 0, 0, 0, 0, 0, 0, 1, 0, 0 }
    , [U_DATA_DISK]    = { R_TER, 0, 0, 0, 0, 0, 0, 800, 0, 0 }
    /* BWAPI raw 20: 200 HP, 3 armor, Hero Jim Raynor Marine. */
    /* Scenario hero: zero source supply and producer 255 make it untrainable. */
    , [U_RAYNOR_MARINE] = { R_TER, 0, 255, 0, 0, 0, 0, 200, 0, 0 }
#endif
#ifdef SC64_T5_RUNTIME
    , [U_KERRIGAN] = { R_TER, 0, 255, 0, 0, 0, 0, 250, 0, 0 }
    , [U_RAYNOR_VULTURE] = { R_TER, 0, 255, 0, 0, 0, 0, 300, 0, 0 }
    /* Deployed scenario actor, not an ordinary attack-order unit or product. */
    , [U_SPIDER_MINE] = { R_TER, 0, 255, 0, 0, 0, 0, 20, 0, 0 }
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    , [U_SCIENCE_VESSEL] = { R_TER, 0, 255, 4, 100, 225, 1200, 200, 0, 0 }
    , [U_BATTLECRUISER] = { R_TER, 0, 255, 12, 400, 300, 2000, 500, 0, 0 }
    , [U_NORAD_II] = { R_TER, 0, 255, 0, 800, 600, 4800, 700, 0, 0 }
    , [U_ULTRALISK] = { R_ZERG, 0, 255, 8, 200, 200, 900, 400, 0, 0 }
    , [U_SHUTTLE] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 4, 200, 0, 900, 80, 60, 0 }
    , [U_PSI_EMITTER] = { R_TER, 0, 255, 0, 1, 1, 1, 800, 0, 0 }
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    , [U_ZP_INFESTED_KERRIGAN] = { R_ZERG, 0, 255, 0, 0, 0, 0, 400, 0, 0 }
    , [U_ZP_MAP_REVEALER] = { R_TER, 0, 255, 0, 0, 0, 0, 1, 0, 0 }
    , [U_ZP_RAYNOR_VULTURE] = { R_TER, 0, 255, 0, 0, 0, 0, 300, 0, 0 }
    , [U_ZP_HIGH_TEMPLAR] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 0, 0, 0, 0, 40, 40, 0 }
    , [U_ZP_ULTRALISK] = { R_ZERG, 0, 255, 0, 0, 0, 0, 400, 0, 0 }
#endif
#ifdef SC64_Z5_ACTORS
    , [U_Z5_GHOST] = { R_TER, 0, 255, 0, 0, 0, 0, 45, 0, 0 }
    , [U_Z5_CIVILIAN] = { R_TER, 0, 255, 0, 0, 0, 0, 40, 0, 0 }
    , [U_Z5_RHYNADON] = { R_TER, 0, 255, 0, 0, 0, 0, 60, 0, 0 }
    , [U_Z5_BENGALAAS] = { R_TER, 0, 255, 0, 0, 0, 0, 60, 0, 0 }
    , [U_Z5_RAGNASAUR] = { R_TER, 0, 255, 0, 0, 0, 0, 60, 0, 0 }
    , [U_Z5_DATA_DISK] = { R_TER, 0, 255, 0, 0, 0, 0, 800, 0, 0 }
#endif
#ifdef SC64_Z6_ACTORS
    , [U_Z6_GUARDIAN] = { R_ZERG, 0, 255, 4, 50, 100, 600, 150, 0, 0 }
#endif
#ifdef SC64_P5_ACTORS
    , [U_P5_SHUTTLE] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 4, 200, 0, 900, 80, 60, 0 }
    , [U_P5_BATTLECRUISER] = { R_TER, 0, 255, 12, 400, 300, 2000, 500, 0, 0 }
#endif
#ifdef SC64_P8_DATA_ENABLED
    , [U_P8_HYPERION] = { R_TER, 0, 255, 0, 0, 0, 0, 850, 0, 0 }
#endif
#ifdef SC64_P6_ACTORS
    /* Scenario actors have no production path. Supply is source
       unitSupplyRequired at raw 50/74/75: 2/2/0 doubled supply. */
    , [U_P6_INFESTED_TERRAN] = { R_ZERG, 0, 255, 2, 100, 50, 600, 60, 0, 0 }
    , [U_P6_HERO_DARK_TEMPLAR] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 2, 150, 150, 750, 40, 80, 0 }
    , [U_P6_ZERATUL] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 0, 100, 300, 1500, 60, 400, 0 }
#endif
#ifdef SC64_P7_ACTORS
    /* P7 raw actors are scenario rows, never ordinary products. Keep source
       costs/vitals while the impossible producer sentinel blocks training. */
    , [U_P7_ARCHON] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 8, 0, 0, 300, 10, 350, 0 }
    , [U_P7_ARBITER] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 8, 100, 350, 2400, 200, 150, 0 }
    , [U_P7_CARRIER] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 12, 350, 250, 2100, 300, 150, 0 }
    , [U_P7_OBSERVER] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 2, 25, 75, 600, 40, 20, 0 }
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    /* Egg stats are raw36; product cost, supply and duration live on its
       target actor while it is unfinished. */
    , [U_Z7_EGG] = { R_ZERG, 0, 255, 0, 0, 0, 0, 200, 0, 0 }
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    , [U_P7_INTERCEPTOR] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 0, 25, 0, 300, 40, 40, 0 }
#endif
#ifdef SC64_SKIRMISH_ROSTER
    /* Ordinary catalog records absent from this profile's legacy raw map.
     * Values are numeric stock facts; morph-only rows retain producer 255 so
     * this mapping extension does not enable unsupported morph behavior. */
#ifndef SC64_TERRAN_LATE_ACTORS
    , [U_SK_SCIENCE_VESSEL] = { R_TER, 0, B_STARPORT, 4, 100, 225, 1200, 200, 0, 0 }
#ifndef SC64_P5_ACTORS
    , [U_SK_BATTLECRUISER] = { R_TER, 0, B_STARPORT, 12, 400, 300, 2000, 500, 0, 0 }
#endif
#endif
#ifndef SC64_Z6_ACTORS
    , [U_SK_GUARDIAN] = { R_ZERG, 0, 255, 4, 50, 100, 600, 150, 0, 0 }
#endif
    , [U_SK_DEFILER] = { R_ZERG, SC_RACE_F_LARVA, B_HATCHERY, 4, 50, 150, 750, 80, 0, 0 }
    , [U_SK_DEVOURER] = { R_ZERG, 0, 255, 4, 150, 50, 600, 250, 0, 0 }
    , [U_SK_LURKER] = { R_ZERG, 0, 255, 4, 50, 100, 600, 125, 0, 0 }
#ifndef SC64_P6_ACTORS
    , [U_SK_INFESTED_TERRAN] = { R_ZERG, 0, B_INFESTED_COMMAND, 2, 100, 50, 600, 60, 0, 0 }
#endif
    , [U_SK_VALKYRIE] = { R_TER, 0, B_STARPORT, 6, 250, 125, 750, 200, 0, 0 }
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    , [U_SK_HIGH_TEMPLAR] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 4, 50, 150, 750, 40, 40, 0 }
#endif
    , [U_SK_DARK_TEMPLAR] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_GATEWAY, 4, 125, 100, 750, 80, 40, 0 }
#ifndef SC64_P7_ACTORS
    , [U_SK_OBSERVER] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_ROBOTICS_FACILITY, 2, 25, 75, 600, 40, 20, 0 }
    , [U_SK_CARRIER] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_STARGATE, 12, 350, 250, 2100, 300, 150, 0 }
    , [U_SK_ARBITER] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_STARGATE, 8, 100, 350, 2400, 200, 150, 0 }
    , [U_SK_ARCHON] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 8, 0, 0, 300, 10, 350, 0 }
#endif
    , [U_SK_CORSAIR] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_STARGATE, 4, 150, 100, 600, 100, 80, 0 }
    , [U_SK_DARK_ARCHON] = { R_PROTOSS, SC_RACE_F_SHIELDS, 255, 8, 0, 0, 300, 25, 200, 0 }
#if !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
    , [U_SK_SHUTTLE] = { R_PROTOSS, SC_RACE_F_SHIELDS, B_ROBOTICS_FACILITY, 4, 200, 0, 900, 80, 60, 0 }
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
    , [U_SHARED_BATTLECRUISER] = { R_TER, 0, 255, 12, 400, 300, 2000, 500, 0, 0 }
#endif
};

static const Sc64RaceBuildingRule sc64_race_building_rules[SC_RB_COUNT] = {
    [B_COMMAND] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_PROVIDES_SUPPLY, U_SCV, 20, 400, 0, 1800, 1500, 0 },
    [B_BARRACKS] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER, U_SCV, 0, 150, 0, 1200, 1000, SC_PR_DEPOT },
    [B_DEPOT] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PROVIDES_SUPPLY, U_SCV, 16, 100, 0, 600, 500, 0 },
    [B_REFINERY] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 100, 0, 600, 750, 0 },
    [B_ACADEMY] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 150, 0, 1200, 600, 0 },
    [B_ENG_BAY] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 125, 0, 900, 850, 0 },
    [B_BUNKER] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 100, 0, 450, 350, 0 },
    /* Scenario building with no stock whatBuilds producer; retain no-cost UI. */
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
    [B_SPIRE] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_CONSUMES_WORKER, U_DRONE, 0, 200, 150, 1800, 600,
#ifdef SC64_ZP_CAMPAIGN_STORAGE
        SC_PR_LAIR
#else
        SC_PR_SPAWNING_POOL
#endif
    },
    [B_CEREBRATE] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 0, 0, 0, 1500, 0 },
    [B_LAIR] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_PROVIDES_SUPPLY, U_DRONE, 2, 150, 100, 1500, 1800, SC_PR_SPAWNING_POOL },
    [B_STARGATE] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 150, 150, 1050, 600, SC_PR_CYBERNETICS_CORE | SC_PR_PYLON },
    [B_STARPORT] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER, U_SCV, 0, 150, 100, 1050, 1300,
#ifdef SC64_T5_RUNTIME
        SC_PR_FACTORY
#else
        0
#endif
    },
    [B_MISSILE_TURRET] = { R_TER, SC_RACE_F_BUILDING, U_SCV, 0, 75, 0, 450, 200, 0 },
    [B_SPORE_COLONY] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 50, 0, 300, 400, SC_PR_HATCHERY },
    /* Legacy u16 fallback; sc64_race_building_max_hp returns stock 100000. */
    [B_ZERG_BEACON] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 250, 0, 0, 10000, 0 },
    [B_FACTORY] = { R_TER, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER, U_SCV, 0, 200, 100, 1200, 1250, SC_PR_BARRACKS },
    [B_ROBOTICS_FACILITY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 200, 200, 1200, 500, SC_PR_CYBERNETICS_CORE | SC_PR_PYLON },
    /* BWAPI raw 150 Mature Chrysalis: static mission building, not a unit. */
    [B_MATURE_CHRYSALIS] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 0, 0, 0, 250, 0 },
    [B_HIVE] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_PRODUCER | SC_RACE_F_PROVIDES_SUPPLY, U_DRONE, 2, 200, 150, 1800, 2500, 0 },
    [B_GREATER_SPIRE] = { R_ZERG, SC_RACE_F_BUILDING, U_DRONE, 0, 100, 150, 1800, 1000, 0 }
#ifdef SC64_T4_RUNTIME
    /* The source's 100000 HP cannot fit Sc64RaceBuildingRule.hp (u16).
       Zero is a sentinel; T4 spawn hooks must use the int accessor below. */
    /* Worker type 255 is an impossible unit ID and marks this as scenario-
       only. Generic build APIs require an exact valid worker type match. */
    , [B_TERRAN_BEACON] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    /* BWAPI's door max HP is 100000 and does not fit the compact u16 rule.
       sc64_race_building_max_hp() provides its exact int value. */
    , [B_FLOOR_MISSILE_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 50, 0 }
    , [B_UPPER_DOOR] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_RIGHT_UPPER_DOOR] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_PIT_DOOR] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_RIGHT_PIT_DOOR] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_FLOOR_GUN_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 50, 0 }
    , [B_WALL_FLAME_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 50, 0 }
    , [B_RIGHT_WALL_MISSILE_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 50, 0 }
#endif
#ifdef SC64_T5_RUNTIME
    /* BWAPI raw 115: 2x2 tiles, 500 HP, 1 armor, 50/50, 600 frames.
       This is a Starport add-on, not an SCV-built structure. */
    , [B_CONTROL_TOWER] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 500, 0 }
#endif
#ifdef SC64_T6_ENABLED
    /* The crashed ship is scenario-only, never a worker construction product. */
    , [B_CRASHED_NORAD_II] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0,
                              SC64_T6_CRASHED_NORAD_HP, 0 }
    , [B_QUEENS_NEST] = { R_ZERG, SC_RACE_F_BUILDING | SC_RACE_F_CONSUMES_WORKER,
                         U_DRONE, 0, SC64_T6_QUEENS_NEST_MINERALS,
                         SC64_T6_QUEENS_NEST_GAS, SC64_T6_QUEENS_NEST_BUILD_FRAMES,
                         SC64_T6_QUEENS_NEST_HP, 0 }
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    , [B_NUCLEAR_SILO] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 100, 100, 1200, 600, 0 }
    , [B_COVERT_OPS] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 750, 0 }
    , [B_MACHINE_SHOP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 750, 0 }
    , [B_ARMORY] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 100, 50, 1200, 750, 0 }
    , [B_DEFILER_MOUND] = { R_ZERG, SC_RACE_F_BUILDING, 255, 0, 100, 100, 900, 850, 0 }
    , [B_ULTRALISK_CAVERN] = { R_ZERG, SC_RACE_F_BUILDING, 255, 0, 150, 200, 1200, 600, 0 }
    , [B_CITADEL_ADUN] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS, 255, 0, 150, 100, 900, 450, 0 }
    , [B_STASIS_CELL_PRISON] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 150, 0, 1, 2000, 0 }
    , [B_FLEET_BEACON] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS, 255, 0, 300, 200, 900, 500, 0 }
    , [B_ROBOTICS_SUPPORT_BAY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS, 255, 0, 150, 100, 450, 450, 0 }
    , [B_SHIELD_BATTERY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS, 255, 0, 100, 0, 450, 200, 0 }
    , [B_SPECIAL_ION_CANNON] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 200, 0, 900, 2000, 0 }
    , [B_SCIENCE_FACILITY] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 100, 150, 900, 850, 0 }
    , [B_PHYSICS_LAB] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 600, 0 }
#elif defined(SC64_P8_DATA_ENABLED)
    , [B_STASIS_CELL_PRISON] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 150, 0, 1, 2000, 0 }
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    , [B_ZP_CONTROL_TOWER] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 500, 0 }
    , [B_ZP_MACHINE_SHOP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 750, 0 }
    , [B_ZP_ARMORY] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 100, 50, 1200, 750, 0 }
    , [B_ZP_QUEENS_NEST] = { R_ZERG, SC_RACE_F_BUILDING, 255, 0, 150, 100, 900, 850, 0 }
    , [B_ZP_ULTRALISK_CAVERN] = { R_ZERG, SC_RACE_F_BUILDING, 255, 0, 150, 200, 1200, 600, 0 }
    , [B_ZP_PROTOSS_BEACON] = { R_PROTOSS, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
#endif
#ifdef SC64_Z5_ACTORS
    /* 100000 HP does not fit the compact rule field; zero is sentinel. */
    , [B_Z5_TERRAN_BEACON] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
#endif
#ifdef SC64_Z5_DOODADS
    /* Scenario props are imported actors, not buildable structures. */
    , [B_Z5_FLOOR_MISSILE_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_Z5_UPPER_DOOR] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_Z5_RIGHT_UPPER_DOOR] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_Z5_PIT_DOOR] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_Z5_RIGHT_PIT_DOOR] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_Z5_FLOOR_GUN_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_Z5_WALL_FLAME_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
    , [B_Z5_RIGHT_WALL_FLAME_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 0, 0 }
#endif
#ifdef SC64_P6_ACTORS
    , [B_P6_WALL_MISSILE_TRAP] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 0, 0, 0, 50, 0 }
#endif
#ifdef SC64_P7_ACTORS
    /* P7 structures remain imported scenario actors, not worker products. */
    , [B_P7_OBSERVATORY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 50, 100, 450, 250, 0 }
    , [B_P7_CITADEL_ADUN] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 150, 100, 900, 450, 0 }
    , [B_P7_TEMPLAR_ARCHIVES] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 150, 200, 900, 500, 0 }
    , [B_P7_FLEET_BEACON] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 300, 200, 900, 500, 0 }
    , [B_P7_ARBITER_TRIBUNAL] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 200, 150, 900, 500, 0 }
    , [B_P7_ROBOTICS_SUPPORT_BAY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 150, 100, 450, 450, 0 }
#ifdef SC64_P7_BATTERY_ENABLED
    , [B_P7_SHIELD_BATTERY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, U_PROBE, 0, 100, 0, 450, 200, SC_PR_GATEWAY }
#else
    , [B_P7_SHIELD_BATTERY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 100, 0, 450, 200, 0 }
#endif
#endif
#if defined(SC64_Z6_RUNTIME) && !defined(SC64_TERRAN_LATE_ACTORS)
    , [B_Z6_ROBOTICS_SUPPORT_BAY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 150, 100, 450, 450, 0 }
    , [B_Z6_SHIELD_BATTERY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 100, 0, 450, 200, 0 }
#endif
#ifdef SC64_NYDUS_ENABLED
    /* Preplaced source endpoints only: Drone construction and BuildNydusExit
     * are not implemented, so no runtime producer is exposed. */
    , [B_NYDUS_CANAL] = { R_ZERG, SC_RACE_F_BUILDING, 255, 0, 150, 0, 600, 250, 0 }
#endif
#ifdef SC64_SKIRMISH_ROSTER
    /* Stock values are from sc64_stock_profiles.h. Worker/admission and
     * placement requirements remain in the skirmish catalog/runtime. */
    , [B_SK_COMSAT] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 500, 0 }
#ifndef SC64_NYDUS_ENABLED
    , [B_SK_NYDUS] = { R_ZERG, SC_RACE_F_BUILDING, 255, 0, 150, 0, 600, 250, 0 }
#endif
#ifndef SC64_P7_ACTORS
#ifndef SC64_TERRAN_LATE_ACTORS
    , [B_SK_CITADEL_ADUN] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 150, 100, 900, 450, 0 }
    , [B_SK_FLEET_BEACON] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 300, 200, 900, 500, 0 }
#endif
    , [B_SK_TEMPLAR_ARCHIVES] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 150, 200, 900, 500, 0 }
    , [B_SK_OBSERVATORY] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 50, 100, 450, 250, 0 }
    , [B_SK_ARBITER_TRIBUNAL] = { R_PROTOSS, SC_RACE_F_BUILDING | SC_RACE_F_SHIELDS | SC_RACE_F_REQUIRES_POWER, 255, 0, 200, 150, 900, 500, 0 }
#endif
#ifndef SC64_TERRAN_LATE_ACTORS
    , [B_SK_SCIENCE_FACILITY] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 100, 150, 900, 850, 0 }
    , [B_SK_NUCLEAR_SILO] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 100, 100, 1200, 600, 0 }
    , [B_SK_COVERT_OPS] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 750, 0 }
    , [B_SK_PHYSICS_LAB] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 600, 0 }
    , [B_SK_DEFILER_MOUND] = { R_ZERG, SC_RACE_F_BUILDING, 255, 0, 100, 100, 900, 850, 0 }
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS)
    , [B_SHARED_SCIENCE_FACILITY] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 100, 150, 900, 850, 0 }
    , [B_SHARED_PHYSICS_LAB] = { R_TER, SC_RACE_F_BUILDING, 255, 0, 50, 50, 600, 600, 0 }
#endif
};

static inline const Sc64RaceUnitRule *sc64_race_unit_rule(int type)
{ return type >= 0 && type < SC_RU_COUNT ? &sc64_race_unit_rules[type] : 0; }
static inline const Sc64RaceBuildingRule *sc64_race_building_rule(int type)
{ return type >= 0 && type < SC_RB_COUNT ? &sc64_race_building_rules[type] : 0; }
static inline int sc64_race_is_worker(int type)
{ const Sc64RaceUnitRule *r = sc64_race_unit_rule(type); return r && (r->flags & SC_RACE_F_WORKER); }
/* One larva produces two bodies for the source pair products. */
static inline int sc64_race_product_count(int type)
{
    if (type == U_ZERGLING) return 2;
#if defined(SC64_SHARED_SCOURGE_COMBAT) || defined(SC64_Z7_SCOURGE_COMBAT)
    if (type == U_SCOURGE) return 2;
#endif
    return 1;
}
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
#ifdef SC64_T4_RUNTIME
        ,"GHOST","CIVILIAN","RHYNADON","RAGNASAUR","MAP REVEALER","DATA DISK",
        "RAYNOR MARINE"
#endif
#ifdef SC64_T5_RUNTIME
        ,"KERRIGAN","RAYNOR VULTURE","SPIDER MINE"
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
        ,"SCIENCE VESSEL","BATTLECRUISER","NORAD II","ULTRALISK","SHUTTLE","PSI EMITTER"
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        ,"INFESTED KERRIGAN","MAP REVEALER","RAYNOR VULTURE","HIGH TEMPLAR","ULTRALISK"
#endif
#ifdef SC64_Z5_ACTORS
        ,"GHOST","CIVILIAN","RHYNADON","BENGALAAS","RAGNASAUR","DATA DISK"
#endif
#ifdef SC64_Z6_ACTORS
        ,"GUARDIAN"
#endif
#ifdef SC64_P5_ACTORS
        ,"SHUTTLE","BATTLECRUISER"
#endif
#ifdef SC64_P6_ACTORS
        ,"INFESTED TERRAN","HERO DARK TEMPLAR","ZERATUL"
#endif
#ifdef SC64_P7_ACTORS
        ,"ARCHON","ARBITER","CARRIER","OBSERVER"
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
        ,"EGG"
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
        ,"INTERCEPTOR"
#endif
#ifdef SC64_SKIRMISH_ROSTER
#ifndef SC64_TERRAN_LATE_ACTORS
        ,[U_SK_SCIENCE_VESSEL] = "SCIENCE VESSEL"
#ifndef SC64_P5_ACTORS
        ,[U_SK_BATTLECRUISER] = "BATTLECRUISER"
#endif
#endif
#ifndef SC64_Z6_ACTORS
        ,[U_SK_GUARDIAN] = "GUARDIAN"
#endif
        ,[U_SK_DEFILER] = "DEFILER"
        ,[U_SK_DEVOURER] = "DEVOURER"
        ,[U_SK_LURKER] = "LURKER"
#ifndef SC64_P6_ACTORS
        ,[U_SK_INFESTED_TERRAN] = "INFESTED TERRAN"
#endif
        ,[U_SK_VALKYRIE] = "VALKYRIE"
#ifndef SC64_ZP_CAMPAIGN_ACTORS
        ,[U_SK_HIGH_TEMPLAR] = "HIGH TEMPLAR"
#endif
        ,[U_SK_DARK_TEMPLAR] = "DARK TEMPLAR"
#ifndef SC64_P7_ACTORS
        ,[U_SK_OBSERVER] = "OBSERVER"
        ,[U_SK_CARRIER] = "CARRIER"
        ,[U_SK_ARBITER] = "ARBITER"
        ,[U_SK_ARCHON] = "ARCHON"
#endif
        ,[U_SK_CORSAIR] = "CORSAIR"
        ,[U_SK_DARK_ARCHON] = "DARK ARCHON"
#if !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
        ,[U_SK_SHUTTLE] = "SHUTTLE"
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
        ,[U_SHARED_BATTLECRUISER] = "BATTLECRUISER"
#endif
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
#ifdef SC64_T4_RUNTIME
        ,"TERRAN BEACON","FLOOR MISSILE TRAP","UPPER DOOR",
        "RIGHT UPPER DOOR","PIT DOOR","RIGHT PIT DOOR",
        "FLOOR GUN TRAP","WALL FLAME TRAP","RIGHT WALL MISSILE TRAP"
#endif
#ifdef SC64_T5_RUNTIME
        ,"CONTROL TOWER"
#endif
#ifdef SC64_T6_ENABLED
        ,"CRASHED NORAD II","QUEEN'S NEST"
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
        ,"NUCLEAR SILO","COVERT OPS","MACHINE SHOP","ARMORY","DEFILER MOUND",
        "ULTRALISK CAVERN","CITADEL OF ADUN","STASIS CELL PRISON","FLEET BEACON",
        "ROBOTICS SUPPORT BAY","SHIELD BATTERY","SPECIAL ION CANNON",
        "SCIENCE FACILITY","PHYSICS LAB"
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        ,"CONTROL TOWER","MACHINE SHOP","ARMORY","QUEEN'S NEST","ULTRALISK CAVERN","PROTOSS BEACON"
#endif
#ifdef SC64_Z5_ACTORS
        ,"TERRAN BEACON"
#endif
#ifdef SC64_Z5_DOODADS
        ,"FLOOR MISSILE TRAP","UPPER DOOR","RIGHT UPPER DOOR","PIT DOOR",
        "RIGHT PIT DOOR","FLOOR GUN TRAP","WALL FLAME TRAP","RIGHT WALL FLAME TRAP"
#endif
#ifdef SC64_P6_ACTORS
        ,"WALL MISSILE TRAP"
#endif
#ifdef SC64_P7_ACTORS
        ,"OBSERVATORY","CITADEL OF ADUN","TEMPLAR ARCHIVES","FLEET BEACON",
        "ARBITER TRIBUNAL","ROBOTICS SUPPORT BAY","SHIELD BATTERY"
#endif
#if defined(SC64_Z6_RUNTIME) && !defined(SC64_TERRAN_LATE_ACTORS)
        ,"ROBOTICS SUPPORT BAY","SHIELD BATTERY"
#endif
#ifdef SC64_SKIRMISH_ROSTER
        ,[B_SK_COMSAT] = "COMSAT STATION"
#ifndef SC64_NYDUS_ENABLED
        ,[B_SK_NYDUS] = "NYDUS CANAL"
#endif
#ifndef SC64_P7_ACTORS
#ifndef SC64_TERRAN_LATE_ACTORS
        ,[B_SK_CITADEL_ADUN] = "CITADEL OF ADUN"
        ,[B_SK_FLEET_BEACON] = "FLEET BEACON"
#endif
        ,[B_SK_TEMPLAR_ARCHIVES] = "TEMPLAR ARCHIVES"
        ,[B_SK_OBSERVATORY] = "OBSERVATORY"
        ,[B_SK_ARBITER_TRIBUNAL] = "ARBITER TRIBUNAL"
#endif
#ifndef SC64_TERRAN_LATE_ACTORS
        ,[B_SK_SCIENCE_FACILITY] = "SCIENCE FACILITY"
        ,[B_SK_NUCLEAR_SILO] = "NUCLEAR SILO"
        ,[B_SK_COVERT_OPS] = "COVERT OPS"
        ,[B_SK_PHYSICS_LAB] = "PHYSICS LAB"
        ,[B_SK_DEFILER_MOUND] = "DEFILER MOUND"
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS)
        ,[B_SHARED_SCIENCE_FACILITY] = "SCIENCE FACILITY"
        ,[B_SHARED_PHYSICS_LAB] = "PHYSICS LAB"
#endif
    };
    return type >= 0 && type < SC_RB_COUNT ? names[type] : "UNKNOWN";
}

static inline unsigned short sc64_race_building_shields(int type)
{
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpBuildingFact *zp = sc64_zp_building_fact(type);
    if (zp) return (unsigned short)zp->shields;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_building_fact(type);
    if (late) return (unsigned short)late->shields;
#endif
#if defined(SC64_Z6_RUNTIME) && !defined(SC64_TERRAN_LATE_ACTORS)
    if (type == B_Z6_ROBOTICS_SUPPORT_BAY) return 450;
    if (type == B_Z6_SHIELD_BATTERY) return 200;
#endif
    switch (type) {
    case B_NEXUS: return 750; case B_GATEWAY: return 500;
    case B_CYBERNETICS_CORE: return 500; case B_FORGE: return 550;
    case B_ASSIMILATOR: return 450; case B_PYLON: return 300;
    case B_PHOTON_CANNON: return 100; case B_STARGATE: return 600;
    case B_ROBOTICS_FACILITY: return 500;
    default: return 0;
    }
}

#ifdef SC64_T4_RUNTIME
/* T4-only actor policy bits. A zero result means use the pre-existing generic
   behavior; these flags are explicit exceptions for imported special actors. */
enum {
    SC64_T4_ACTOR_SELECTABLE = 1,
    SC64_T4_ACTOR_MOVABLE = 2,
    SC64_T4_ACTOR_COLLIDABLE = 4,
    SC64_T4_ACTOR_DRAWABLE = 8,
    SC64_T4_ACTOR_MEN = 16,
    SC64_T4_ACTOR_BUILDING = 32,
    SC64_T4_ACTOR_INVINCIBLE = 64,
    SC64_T4_ACTOR_HIDDEN = 128,
    SC64_T4_ACTOR_WEAPON = 256
};
static inline unsigned sc64_race_t4_actor_flags(int type)
{
#ifdef SC64_TERRAN_LATE_ACTORS
    unsigned late = sc64_terran_late_unit_actor_flags(type);
    if (late) return late;
#endif
    switch (type) {
    case U_GHOST:
    case U_CIVILIAN:
        return SC64_T4_ACTOR_SELECTABLE | SC64_T4_ACTOR_MOVABLE |
               SC64_T4_ACTOR_COLLIDABLE | SC64_T4_ACTOR_DRAWABLE |
               SC64_T4_ACTOR_MEN | (type == U_GHOST ? SC64_T4_ACTOR_WEAPON : 0);
#ifdef SC64_T5_RUNTIME
    case U_KERRIGAN: case U_RAYNOR_VULTURE:
        return SC64_T4_ACTOR_SELECTABLE | SC64_T4_ACTOR_MOVABLE |
               SC64_T4_ACTOR_COLLIDABLE | SC64_T4_ACTOR_DRAWABLE |
               SC64_T4_ACTOR_MEN | SC64_T4_ACTOR_WEAPON;
    case U_SPIDER_MINE:
        /* The autonomous arming/movement behavior is not implemented yet. */
        return SC64_T4_ACTOR_DRAWABLE;
#endif
    case U_RHYNADON: case U_RAGNASAUR:
        return SC64_T4_ACTOR_MOVABLE | SC64_T4_ACTOR_COLLIDABLE |
               SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_MEN;
    case U_MAP_REVEALER:
        return SC64_T4_ACTOR_HIDDEN;
    case U_DATA_DISK:
        return SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_INVINCIBLE;
    default:
        return 0;
    }
}
static inline unsigned sc64_race_t4_building_flags(int type)
{
#ifdef SC64_TERRAN_LATE_ACTORS
    unsigned late = sc64_terran_late_building_actor_flags(type);
    if (late) return late;
#endif
    switch (type) {
    case B_TERRAN_BEACON:
        return SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_BUILDING |
               SC64_T4_ACTOR_INVINCIBLE;
    case B_UPPER_DOOR: case B_RIGHT_UPPER_DOOR:
    case B_PIT_DOOR: case B_RIGHT_PIT_DOOR:
        /* BWAPI's raw unit flags mark all four doors Invincible. */
        return SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_BUILDING |
               SC64_T4_ACTOR_INVINCIBLE;
    case B_FLOOR_MISSILE_TRAP: case B_FLOOR_GUN_TRAP:
    case B_WALL_FLAME_TRAP: case B_RIGHT_WALL_MISSILE_TRAP:
        /* All four raw types have a source groundWeapon entry. Selectability
           and collision are handled by the scenario runtime. */
        return SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_BUILDING |
               SC64_T4_ACTOR_WEAPON;
    default: return 0;
    }
}
static inline int sc64_race_t4_is_door(int type)
{ return type >= B_UPPER_DOOR && type <= B_RIGHT_PIT_DOOR; }
static inline int sc64_race_t4_is_trap(int type)
{ return type == B_FLOOR_MISSILE_TRAP || type == B_FLOOR_GUN_TRAP ||
         type == B_WALL_FLAME_TRAP || type == B_RIGHT_WALL_MISSILE_TRAP; }
/* BWAPI stores bounding offsets in source pixels; callers applying the GBA's
   2:1 map scale should convert these raw offsets at their query boundary. */
static inline int sc64_race_t4_bounds_source(int type, int *left, int *up,
                                             int *right, int *down)
{
#ifdef SC64_TERRAN_LATE_ACTORS
    if (sc64_terran_late_actor_bounds(type, 0, left, up, right, down)) return 1;
#endif
    int l, u, r, d;
    switch (type) {
    case U_GHOST: l=7; u=10; r=7; d=11; break;
    case U_CIVILIAN: l=8; u=9; r=8; d=10; break;
    case U_RHYNADON: case U_RAGNASAUR: l=16; u=16; r=15; d=15; break;
    case U_MAP_REVEALER: l=13; u=13; r=13; d=17; break;
    case U_DATA_DISK: l=16; u=16; r=15; d=15; break;
    default: return 0;
    }
    if (left) *left=l;
    if (up) *up=u;
    if (right) *right=r;
    if (down) *down=d;
    return 1;
}
static inline int sc64_race_t4_building_bounds_source(int type, int *left,
                                                      int *up, int *right,
                                                      int *down)
{
#ifdef SC64_TERRAN_LATE_ACTORS
    if (sc64_terran_late_actor_bounds(type, 1, left, up, right, down)) return 1;
#endif
    if (type == B_UPPER_DOOR) { if(left)*left=25; if(up)*up=17; if(right)*right=44; if(down)*down=20; return 1; }
    if (type == B_RIGHT_UPPER_DOOR) { if(left)*left=44; if(up)*up=17; if(right)*right=25; if(down)*down=20; return 1; }
    if (type == B_PIT_DOOR) { if(left)*left=41; if(up)*up=17; if(right)*right=28; if(down)*down=20; return 1; }
    if (type == B_RIGHT_PIT_DOOR) { if(left)*left=28; if(up)*up=17; if(right)*right=41; if(down)*down=20; return 1; }
    if (type == B_FLOOR_MISSILE_TRAP || type == B_FLOOR_GUN_TRAP) { if(left)*left=32; if(up)*up=32; if(right)*right=31; if(down)*down=31; return 1; }
    if (type == B_WALL_FLAME_TRAP || type == B_RIGHT_WALL_MISSILE_TRAP) { if(left)*left=16; if(up)*up=16; if(right)*right=15; if(down)*down=15; return 1; }
    if (type != B_TERRAN_BEACON) return 0;
    if (left) *left=48;
    if (up) *up=32;
    if (right) *right=47;
    if (down) *down=31;
    return 1;
}
static inline int sc64_race_t4_sight_source_pixels(int type)
{
#ifdef SC64_TERRAN_LATE_ACTORS
    int late = sc64_terran_late_unit_sight_source_pixels(type);
    if (late) return late;
#endif
    switch (type) {
    case U_GHOST: return 9*32;
    case U_CIVILIAN: case U_RHYNADON: case U_RAGNASAUR: return 7*32;
    case U_MAP_REVEALER: return 10*32;
    case U_DATA_DISK: return 5*32;
    /* Every other mapped actor uses its pinned BWAPI sightRange.  Human
       automatic acquisition is gated on this sight, so it must reach the
       source weapon ranges (Goliath, Wraith, Vulture, Kerrigan, ...). */
    default: return sc64_source_sight_raw_pixels(sc64_race_raw_unit(type));
    }
}
static inline int sc64_race_t4_building_sight_source_pixels(int type)
{
#ifdef SC64_TERRAN_LATE_ACTORS
    int late = sc64_terran_late_building_sight_source_pixels(type);
    if (late) return late;
#endif
    /* BWAPI UnitType::sightRange() is tile based; Special_Terran_Beacon
       (raw 195) has sightRangeTiles[195] == 8. */
    switch (type) {
    case B_TERRAN_BEACON: return 8 * 32;
    case B_FLOOR_MISSILE_TRAP: case B_FLOOR_GUN_TRAP:
    case B_RIGHT_WALL_MISSILE_TRAP: return 6 * 32;
    case B_WALL_FLAME_TRAP: return 3 * 32;
    case B_UPPER_DOOR: case B_RIGHT_UPPER_DOOR:
    case B_PIT_DOOR: case B_RIGHT_PIT_DOOR: return 1 * 32;
    default: return sc64_source_sight_raw_pixels(sc64_race_raw_building(type));
    }
}
static inline int sc64_race_t4_unit_intrinsically_invincible(int type)
{ return type == U_DATA_DISK
#ifdef SC64_TERRAN_LATE_ACTORS
    || type == U_PSI_EMITTER
#endif
    ; }
static inline int sc64_race_t4_building_intrinsically_invincible(int type)
{ return type == B_TERRAN_BEACON || sc64_race_t4_is_door(type); }
/* C-10 Canister Rifle is Concussive, and Terran Infantry Weapons add one
   point per level. This model has no cloak/lockdown/nuke state. */
enum { SC64_RACE_DAMAGE_NORMAL, SC64_RACE_DAMAGE_CONCUSSIVE };
static inline int sc64_race_t4_weapon_damage_type(int type)
{ return type == U_GHOST
#ifdef SC64_Z5_ACTORS
      || type == U_Z5_GHOST
#endif
      ? SC64_RACE_DAMAGE_CONCUSSIVE : SC64_RACE_DAMAGE_NORMAL; }
static inline int sc64_race_t4_weapon_upgrade_bonus(int type)
{ return type == U_GHOST
#ifdef SC64_Z5_ACTORS
      || type == U_Z5_GHOST
#endif
      ? 1 : 0; }

/* BWAPI UnitSizeTypes: raw 0 Small, 1 Medium, 2 Large, 3 Independent.
   Values verified in the pinned BWAPI UnitType.cpp unitSizes::unitSize table.
   -1 means this helper has no source-size fact for that raw type. */
enum {
    SC64_T4_SIZE_UNKNOWN = -1,
    SC64_T4_SIZE_INDEPENDENT = 0,
    SC64_T4_SIZE_SMALL = 1,
    SC64_T4_SIZE_MEDIUM = 2,
    SC64_T4_SIZE_LARGE = 3
};
static inline int sc64_race_t4_source_size(int raw_type)
{
    switch (raw_type) {
    case 0: case 1: case 7: case 15: case 20: case 32: case 34: case 37:
    case 41: case 43: case 47: case 64: case 65: case 77: case 79:
    case 89: case 95:
#ifdef SC64_Z5_ACTORS
    case 90:
#endif
        return SC64_T4_SIZE_SMALL;
#ifdef SC64_T5_RUNTIME
    case 13: case 16: return SC64_T4_SIZE_SMALL;
#endif
    case 2: case 19: case 38: case 45: case 53:
        return SC64_T4_SIZE_MEDIUM;
    /* All currently supported canonical building raws are Large except the
       two special faction Beacons, whose BWAPI size is Independent. */
    case 3: case 5: case 8: case 11: case 30: case 42: case 66: case 70:
    case 78: case 83: case 106: case 109: case 110: case 111: case 112:
    case 113: case 114: case 124: case 125: case 130: case 131: case 132:
    case 133: case 135: case 137: case 139: case 141: case 142: case 143:
    case 144: case 146: case 149: case 150: case 151: case 154: case 155:
    case 156: case 157: case 160: case 162: case 164: case 166: case 167:
#ifdef SC64_Z5_DOODADS
    case 203: case 205: case 206: case 207: case 208: case 209: case 211: case 213:
#endif
        return SC64_T4_SIZE_LARGE;
    case 101: case 194: case 195: case 216: case 218:
        return SC64_T4_SIZE_INDEPENDENT;
#ifdef SC64_T6_ENABLED
    case SC64_T6_CRASHED_NORAD_RAW: case SC64_T6_QUEENS_NEST_RAW:
        return SC64_T4_SIZE_LARGE;
#endif
    default:
        return SC64_T4_SIZE_UNKNOWN;
    }
}

/* Apply armor and the Ghost's Concussive size factor to damage left after
   shields. Weapon upgrades must be applied before shield absorption, as in
   tick_combat; adding them here would count shield damage incorrectly.
   Positive integer HP uses
   floor division for Medium/Large and a minimum of 1; BW's fractional damage
   can retain fractional HP, so this bounded integer policy may differ by 1 HP.
   Damage and armor are bounded to keep this helper safe on target integer
   widths. */
static inline int sc64_race_t4_ghost_hp_damage(int raw_target, int raw_damage,
                                              int armor)
{
    if (raw_damage < 0) raw_damage = 0;
    if (raw_damage > 100000) raw_damage = 100000;
    if (armor < 0) armor = 0;
    if (armor > 100000) armor = 100000;
    int post_armor = raw_damage - armor;
    if (post_armor < 1) post_armor = 1;

    switch (sc64_race_t4_source_size(raw_target)) {
    case SC64_T4_SIZE_MEDIUM:
        post_armor /= 2;
        break;
    case SC64_T4_SIZE_LARGE:
        post_armor /= 4;
        break;
    default:
        /* Small and Independent receive full Concussive damage. Unknown raw
           types also use full damage until their source class is audited. */
        break;
    }
    return post_armor > 0 ? post_armor : 1;
}
#endif

/* Return full int HP for scenario buildings whose stock 100000 value does not
 * fit Sc64RaceBuildingRule.hp. Available in every profile so map imports and
 * building construction share one max-HP path. */
static inline int sc64_race_building_max_hp(int type)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpBuildingFact *zp = sc64_zp_building_fact(type);
    if (zp) return zp->hp;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_building_fact(type);
    if (late) return late->hp;
#endif
    if (type == B_ZERG_BEACON) return 100000;
#ifdef SC64_T4_RUNTIME
    if (type == B_TERRAN_BEACON || sc64_race_t4_is_door(type)) return 100000;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    if (type == B_ZP_PROTOSS_BEACON) return 100000;
#endif
#ifdef SC64_Z5_ACTORS
    if (type == B_Z5_TERRAN_BEACON) return 100000;
#endif
    return r ? r->hp : 0;
}

/* Combat tuning uses BWAPI public default damage/range/cooldown facts and
 * retains pixel ranges and simulation-frame cooldowns. */
static inline int sc64_race_unit_damage(int type)
{
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return 20;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return (zp->flags & SC64_ZP_ACTOR_WEAPON) ? zp->ground_damage : 0;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return late->ground_damage;
#endif
    switch (type) {
    case U_SCV: case U_DRONE: case U_PROBE: return 5;
    case U_RAYNOR:
#ifdef SC64_T4_RUNTIME
        return 30;
#else
        return 18;
#endif
#ifdef SC64_T4_RUNTIME
    case U_GHOST: return 10;
    case U_RAYNOR_MARINE: return 18;
#endif
#ifdef SC64_T5_RUNTIME
    case U_KERRIGAN: case U_RAYNOR_VULTURE: return 30;
#endif
    case U_MARINE: return 6; case U_FIREBAT: return 8;
    case U_ZERGLING: return 5; case U_HYDRALISK: return 10;
    case U_ZEALOT: return 8; case U_DRAGOON: return 20;
    case U_GOLIATH: return 12; case U_TANK: return 30;
    case U_WRAITH: return 8; case U_MUTALISK: return 9;
    case U_HUNTER_KILLER: return 20; case U_REAVER: return 100;
    case U_FENIX: return 20; case U_FENIX_DRAGOON: return 45;
    case U_TASSADAR: return 20; case U_SCOUT: return 8;
    case U_SIEGE_TANK: return 70;
    case U_VULTURE: return 20;
    default: return 0;
    }
}

static inline int sc64_race_unit_range(int type)
{
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return 256;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return (zp->flags & SC64_ZP_ACTOR_WEAPON) ? zp->ground_range : 0;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return late->ground_range;
#endif
    switch (type) {
    case U_WRAITH: case U_VULTURE: case U_RAYNOR: return 160;
#ifdef SC64_T4_RUNTIME
    case U_GHOST: return 224;
    case U_RAYNOR_MARINE: return 160;
#endif
#ifdef SC64_T5_RUNTIME
    case U_KERRIGAN: return 192;
    case U_RAYNOR_VULTURE: return 160;
#endif
    case U_GOLIATH: return 192;
    case U_TANK: return 224;
    case U_MARINE: case U_HYDRALISK: case U_DRAGOON: case U_SCOUT:
    case U_FENIX_DRAGOON: return 128;
    case U_HUNTER_KILLER: return 160;
    case U_TASSADAR: return 96;
    case U_SIEGE_TANK: return 384;
    case U_MUTALISK: return 96; case U_REAVER: return 256;
    case U_FIREBAT: return 32; case U_ZEALOT: case U_FENIX: return 15;
    case U_SCV: return 10;
    case U_DRONE: case U_PROBE: return 32;
    case U_ZERGLING: return 15;
    default: return 0;
    }
}

static inline int sc64_race_unit_cooldown(int type)
{
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return 30;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return (zp->flags & SC64_ZP_ACTOR_WEAPON) ? zp->ground_cooldown : 0;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return late->ground_cooldown;
#endif
    switch (type) {
    case U_ZERGLING: return 8; case U_MARINE: return 15;
#ifdef SC64_T4_RUNTIME
    case U_GHOST: return 22;
    case U_RAYNOR_MARINE: return 15;
    case U_RAYNOR: return 22;
#else
    case U_RAYNOR: return 15;
#endif
#ifdef SC64_T5_RUNTIME
    case U_KERRIGAN: case U_RAYNOR_VULTURE: return 22;
#endif
    case U_VULTURE: return 30;
    case U_HYDRALISK: return 15; case U_FIREBAT: return 22;
    case U_ZEALOT: return 22; case U_DRAGOON: return 30;
    case U_FENIX: return 22;
    case U_GOLIATH: return 22; case U_TANK: return 37;
    case U_WRAITH: return 30; case U_MUTALISK: return 30;
    case U_REAVER: return 60; case U_SCOUT: return 30;
    case U_TASSADAR: return 22; case U_FENIX_DRAGOON: return 22;
    case U_SIEGE_TANK: return 75;
    case U_DRONE: case U_PROBE: return 22;
    default: return 15;
    }
}

#ifdef SC64_FACT_TABLES
/* Reference oracle; sc64_fact_tables.h defines the table-backed lookup. */
#define sc64_race_unit_flying sc64_race_unit_flying_reference
#endif
static inline int sc64_race_unit_flying(int type)
{
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return 1;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return zp->flying;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return late->flying;
#endif
    return type == U_OVERLORD || type == U_WRAITH || type == U_MUTALISK || type == U_SCOUT || type == U_QUEEN || type == U_DROPSHIP || type == U_SCOURGE
#ifdef SC64_T4_RUNTIME
    || type == U_MAP_REVEALER
#endif
    ;
}

/* Weapon eligibility against aircraft is intentionally separate from
   damage: C-10 and several existing weapons have different air values. */
static inline int sc64_race_unit_damage(int type);
static inline int sc64_race_unit_can_attack_air(int type)
{
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return 0;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return (zp->flags & SC64_ZP_ACTOR_WEAPON) && zp->air_damage > 0;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return late->air_damage > 0;
#endif
    switch (type) {
    case -1: case U_MARINE: case U_HYDRALISK: case U_HUNTER_KILLER:
    case U_DRAGOON: case U_FENIX_DRAGOON: case U_GOLIATH:
    case U_WRAITH: case U_MUTALISK: case U_SCOUT:
#if (defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)) || \
    defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
    case U_SCOURGE:
#endif
#ifdef SC64_T4_RUNTIME
    case U_GHOST: case U_RAYNOR_MARINE:
#endif
#ifndef SC64_T4_RUNTIME
    case U_RAYNOR:
#endif
#ifdef SC64_T5_RUNTIME
    case U_KERRIGAN:
#endif
        return 1;
    default: return 0;
    }
}
static inline int sc64_race_unit_can_attack_target_class(int type, int flying)
{ return type < 0 || (flying ? sc64_race_unit_can_attack_air(type) :
                      sc64_race_unit_damage(type) > 0); }

/* Ground and air weapons can differ on the same unit. Damage is per hit;
 * expose the number of simultaneous blade hits so armor is applied per hit. */
static inline int sc64_race_unit_damage_vs(int type, int target_flying)
{
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return target_flying ? 0 : 20;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return (zp->flags & SC64_ZP_ACTOR_WEAPON) ?
        (target_flying ? zp->air_damage : zp->ground_damage) : 0;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return target_flying ? late->air_damage : late->ground_damage;
#endif
#ifdef SC64_T4_RUNTIME
    if (type == U_GHOST) return 10;
    if (type == U_RAYNOR && target_flying) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (type == U_KERRIGAN) return 30;
    if (type == U_RAYNOR_VULTURE) return target_flying ? 0 : 30;
#endif
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    /* Pinned BWAPI Suicide_Scourge: 110 normal damage, cooldown 1, 3px
     * maximum range, air-only. game.c applies the engine's 2:1 scale. */
    if (type == U_SCOURGE) return target_flying ? 110 : 0;
#elif defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
    /* Fixed-damage Suicide Scourge; no generic weapon-level bonus. */
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
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return target_flying ? 0 : 256;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return (zp->flags & SC64_ZP_ACTOR_WEAPON) ?
        (target_flying ? zp->air_range : zp->ground_range) : 0;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return target_flying ? late->air_range : late->ground_range;
#endif
#ifdef SC64_T4_RUNTIME
    if (type == U_GHOST) return 224;
    if (type == U_RAYNOR && target_flying) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (type == U_KERRIGAN) return 192;
    if (type == U_RAYNOR_VULTURE) return target_flying ? 0 : 160;
#endif
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if (type == U_SCOURGE) return target_flying ? 3 : 0;
#elif defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
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
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return target_flying ? 0 : 30;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return (zp->flags & SC64_ZP_ACTOR_WEAPON) ?
        (target_flying ? zp->air_cooldown : zp->ground_cooldown) : 0;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return target_flying ? late->air_cooldown : late->ground_cooldown;
#endif
#ifdef SC64_T4_RUNTIME
    if (type == U_GHOST) return 22;
    if (type == U_RAYNOR && target_flying) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (type == U_KERRIGAN || type == U_RAYNOR_VULTURE) return 22;
#endif
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if (type == U_SCOURGE) return target_flying ? 1 : 0;
#elif defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
    if (type == U_SCOURGE) return target_flying ? 1 : 0;
#endif
    if (type == U_WRAITH && target_flying) return 22;
    if (type == U_SCOUT && target_flying) return 22;
    return sc64_race_unit_cooldown(type);
}
static inline int sc64_race_unit_hits_vs(int type, int target_flying)
{
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return target_flying ? 0 : 1;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return (zp->flags & SC64_ZP_ACTOR_WEAPON) ?
        (target_flying ? zp->air_hits : zp->ground_hits) : 0;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return target_flying ? late->air_hits : late->ground_hits;
#endif
    return !target_flying && (type == U_ZEALOT || type == U_FENIX) ? 2 : 1;
}
static inline int sc64_race_unit_armor(int type)
{
#ifdef SC64_Z7_EGG_PRODUCTION
    if (type == U_Z7_EGG) return 10;
#endif
#ifdef SC64_Z6_ACTORS
    if (type == U_Z6_GUARDIAN) return 2;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *zp = sc64_zp_actor_fact(type);
    if (zp) return zp->armor;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_unit_fact(type);
    if (late) return late->armor;
#endif
    switch (type) {
    case U_RAYNOR: return 3;
#ifdef SC64_T4_RUNTIME
    case U_RAYNOR_MARINE: return 3;
#endif
#ifdef SC64_T5_RUNTIME
    case U_KERRIGAN: case U_RAYNOR_VULTURE: return 3;
#endif
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
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpBuildingFact *zp = sc64_zp_building_fact(type);
    if (zp) return zp->armor;
#endif
#ifdef SC64_P6_ACTORS
    if (type == B_P6_WALL_MISSILE_TRAP)
        return sc64_zp_building_fact(type)->armor;
#endif
#ifdef SC64_Z5_DOODADS
    if (type >= B_Z5_FLOOR_MISSILE_TRAP && type <= B_Z5_RIGHT_WALL_FLAME_TRAP)
        return sc64_zp_building_fact(type)->armor;
#endif
#ifdef SC64_Z5_ACTORS
    if (type == B_Z5_TERRAN_BEACON) return sc64_zp_building_fact(type)->armor;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late = sc64_terran_late_building_fact(type);
    if (late) return late->armor;
#endif
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
#ifdef SC64_T5_RUNTIME
    case B_CONTROL_TOWER: return 1;
#endif
#ifdef SC64_T6_ENABLED
    case B_CRASHED_NORAD_II: case B_QUEENS_NEST: return 1;
#endif
    default: return 0;
    }
}

static inline int sc64_race_raw_unit(int type)
{
    switch (type) {
    case U_MARINE: return 0; case U_SCV: return 7; case U_FIREBAT: return 32;
    case U_MEDIC: return 34;
#ifdef SC64_T4_RUNTIME
    case U_RAYNOR: return 19; case U_RAYNOR_MARINE: return 20;
#else
    case U_RAYNOR: return 20;
#endif
#ifdef SC64_T5_RUNTIME
    case U_KERRIGAN: return 16; case U_RAYNOR_VULTURE: return 19;
    case U_SPIDER_MINE: return 13;
#endif
    case U_ZERGLING: return 37;
    case U_HYDRALISK: return 38; case U_DRONE: return 41; case U_OVERLORD: return 42;
    case U_MUTALISK: return 43; case U_QUEEN: return 45; case U_HUNTER_KILLER: return 53;
    case U_PROBE: return 64; case U_ZEALOT: return 65; case U_DRAGOON: return 66;
    case U_SCOUT: return 70; case U_FENIX: return 77; case U_FENIX_DRAGOON: return 78;
    case U_TASSADAR: return 79; case U_REAVER: return 83; case U_GOLIATH: return 3;
    case U_TANK: return 5; case U_SIEGE_TANK: return 30;
    case U_WRAITH: return 8; case U_CHRYSALIS: return 216;
    case U_VULTURE: return 2; case U_DROPSHIP: return 11; case U_SCOURGE: return 47;
#ifdef SC64_T4_RUNTIME
    case U_GHOST: return 1; case U_CIVILIAN: return 15;
    case U_RHYNADON: return 89; case U_RAGNASAUR: return 95;
    case U_MAP_REVEALER: return 101; case U_DATA_DISK: return 218;
#endif
#ifdef SC64_Z5_ACTORS
    case U_Z5_GHOST: return 1; case U_Z5_CIVILIAN: return 15;
    case U_Z5_RHYNADON: return 89; case U_Z5_BENGALAAS: return 90;
    case U_Z5_RAGNASAUR: return 95; case U_Z5_DATA_DISK: return 218;
#endif
#ifdef SC64_Z6_ACTORS
    case U_Z6_GUARDIAN: return 44;
#endif
#ifdef SC64_P5_ACTORS
    case U_P5_SHUTTLE: return 69;
    case U_P5_BATTLECRUISER: return 12;
#endif
#ifdef SC64_P6_ACTORS
    case U_P6_INFESTED_TERRAN: return SC64_P6_RAW_INFESTED_TERRAN;
    case U_P6_HERO_DARK_TEMPLAR: return SC64_P6_RAW_HERO_DARK_TEMPLAR;
    case U_P6_ZERATUL: return SC64_P6_RAW_ZERATUL;
#endif
#ifdef SC64_P7_ACTORS
    case U_P7_ARCHON: return 68; case U_P7_ARBITER: return 71;
    case U_P7_CARRIER: return 72; case U_P7_OBSERVER: return 84;
#ifdef SC64_P7_PROJECTILES_ENABLED
    case U_P7_INTERCEPTOR: return 73;
#endif
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    case U_Z7_EGG: return 36;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    case U_SCIENCE_VESSEL: return 9; case U_BATTLECRUISER: return 12;
    case U_NORAD_II: return 29;
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case U_ULTRALISK: return 39;
#endif
    case U_SHUTTLE: return 69; case U_PSI_EMITTER: return 217;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    case U_ZP_INFESTED_KERRIGAN: return 51; case U_ZP_MAP_REVEALER: return 101;
    case U_ZP_RAYNOR_VULTURE: return 19; case U_ZP_HIGH_TEMPLAR: return 67;
    case U_ZP_ULTRALISK: return 39;
#endif
#ifdef SC64_P8_DATA_ENABLED
    case U_P8_HYPERION: return 28;
#endif
#ifdef SC64_SKIRMISH_ROSTER
#ifndef SC64_TERRAN_LATE_ACTORS
    case U_SK_SCIENCE_VESSEL: return 9;
#ifndef SC64_P5_ACTORS
    case U_SK_BATTLECRUISER: return 12;
#endif
#endif
#ifndef SC64_Z6_ACTORS
    case U_SK_GUARDIAN: return 44;
#endif
    case U_SK_DEFILER: return 46; case U_SK_DEVOURER: return 62;
    case U_SK_LURKER: return 103;
#ifndef SC64_P6_ACTORS
    case U_SK_INFESTED_TERRAN: return 50;
#endif
    case U_SK_VALKYRIE: return 58;
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case U_SK_HIGH_TEMPLAR: return 67;
#endif
    case U_SK_DARK_TEMPLAR: return 61;
#ifndef SC64_P7_ACTORS
    case U_SK_OBSERVER: return 84; case U_SK_CARRIER: return 72;
    case U_SK_ARBITER: return 71; case U_SK_ARCHON: return 68;
#endif
    case U_SK_CORSAIR: return 60; case U_SK_DARK_ARCHON: return 63;
#if !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
    case U_SK_SHUTTLE: return 69;
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
    case U_SHARED_BATTLECRUISER: return 12;
#endif
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
#if defined(SC64_T5_RUNTIME) && !defined(SC64_ZP_CAMPAIGN_ACTORS)
    case B_CONTROL_TOWER: return 115;
#endif
#ifdef SC64_T6_ENABLED
    case B_CRASHED_NORAD_II: return SC64_T6_CRASHED_NORAD_RAW;
    case B_QUEENS_NEST: return SC64_T6_QUEENS_NEST_RAW;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    case B_NUCLEAR_SILO: return 108; case B_COVERT_OPS: return 117;
    case B_DEFILER_MOUND: return 136;
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case B_MACHINE_SHOP: return 120; case B_ARMORY: return 123;
    case B_ULTRALISK_CAVERN: return 140;
#endif
    case B_CITADEL_ADUN: return 163;
    case B_STASIS_CELL_PRISON: return 168;
    case B_FLEET_BEACON: return 169; case B_ROBOTICS_SUPPORT_BAY: return 171;
    case B_SHIELD_BATTERY: return 172; case B_SPECIAL_ION_CANNON: return 127;
    case B_SCIENCE_FACILITY: return 116; case B_PHYSICS_LAB: return 118;
#endif
#if defined(SC64_P8_DATA_ENABLED) && !defined(SC64_TERRAN_LATE_ACTORS)
    case B_STASIS_CELL_PRISON: return 168;
#endif
#if defined(SC64_Z6_RUNTIME) && !defined(SC64_TERRAN_LATE_ACTORS)
    case B_Z6_ROBOTICS_SUPPORT_BAY: return 171;
    case B_Z6_SHIELD_BATTERY: return 172;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    case B_ZP_CONTROL_TOWER: return 115; case B_ZP_MACHINE_SHOP: return 120;
    case B_ZP_ARMORY: return 123; case B_ZP_QUEENS_NEST: return 138;
    case B_ZP_ULTRALISK_CAVERN: return 140; case B_ZP_PROTOSS_BEACON: return 196;
#endif
#ifdef SC64_Z5_ACTORS
    case B_Z5_TERRAN_BEACON: return 195;
#endif
#ifdef SC64_Z5_DOODADS
    case B_Z5_FLOOR_MISSILE_TRAP: return 203;
    case B_Z5_UPPER_DOOR: return 205; case B_Z5_RIGHT_UPPER_DOOR: return 206;
    case B_Z5_PIT_DOOR: return 207; case B_Z5_RIGHT_PIT_DOOR: return 208;
    case B_Z5_FLOOR_GUN_TRAP: return 209; case B_Z5_WALL_FLAME_TRAP: return 211;
    case B_Z5_RIGHT_WALL_FLAME_TRAP: return 213;
#endif
#ifdef SC64_T4_RUNTIME
    case B_TERRAN_BEACON: return 195;
    case B_FLOOR_MISSILE_TRAP: return 203; case B_UPPER_DOOR: return 205;
    case B_RIGHT_UPPER_DOOR: return 206; case B_PIT_DOOR: return 207;
    case B_RIGHT_PIT_DOOR: return 208; case B_FLOOR_GUN_TRAP: return 209;
    case B_WALL_FLAME_TRAP: return 211; case B_RIGHT_WALL_MISSILE_TRAP: return 212;
#endif
#ifdef SC64_P6_ACTORS
    case B_P6_WALL_MISSILE_TRAP: return SC64_P6_RAW_WALL_MISSILE_TRAP;
#endif
#ifdef SC64_P7_ACTORS
    case B_P7_OBSERVATORY: return 159; case B_P7_CITADEL_ADUN: return 163;
    case B_P7_TEMPLAR_ARCHIVES: return 165; case B_P7_FLEET_BEACON: return 169;
    case B_P7_ARBITER_TRIBUNAL: return 170;
    case B_P7_ROBOTICS_SUPPORT_BAY: return 171;
    case B_P7_SHIELD_BATTERY: return 172;
#endif
#ifdef SC64_NYDUS_ENABLED
    case B_NYDUS_CANAL: return 134;
#endif
#ifdef SC64_SKIRMISH_ROSTER
    case B_SK_COMSAT: return 107;
#ifndef SC64_NYDUS_ENABLED
    case B_SK_NYDUS: return 134;
#endif
#ifndef SC64_P7_ACTORS
#ifndef SC64_TERRAN_LATE_ACTORS
    case B_SK_CITADEL_ADUN: return 163; case B_SK_FLEET_BEACON: return 169;
#endif
    case B_SK_TEMPLAR_ARCHIVES: return 165; case B_SK_OBSERVATORY: return 159;
    case B_SK_ARBITER_TRIBUNAL: return 170;
#endif
#ifndef SC64_TERRAN_LATE_ACTORS
    case B_SK_SCIENCE_FACILITY: return 116; case B_SK_NUCLEAR_SILO: return 108;
    case B_SK_COVERT_OPS: return 117; case B_SK_PHYSICS_LAB: return 118;
    case B_SK_DEFILER_MOUND: return 136;
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS)
    case B_SHARED_SCIENCE_FACILITY: return 116;
    case B_SHARED_PHYSICS_LAB: return 118;
#endif
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
    case 34: return U_MEDIC;
#ifdef SC64_T4_RUNTIME
    case 20: return U_RAYNOR_MARINE;
#else
    case 20: return U_RAYNOR;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    case 19: return U_ZP_RAYNOR_VULTURE;
#elif defined(SC64_UNIFIED) || defined(SC64_T5_RUNTIME)
    case 19:
#ifdef SC64_T5_RUNTIME
        return U_RAYNOR_VULTURE;
#else
        return U_RAYNOR; /* Existing unified profiles' Raynor Vulture. */
#endif
#endif
    case 37: return U_ZERGLING; case 38: return U_HYDRALISK;
    case 41: return U_DRONE; case 42: return U_OVERLORD; case 45: return U_QUEEN;
    case 64: return U_PROBE; case 65: return U_ZEALOT; case 66: return U_DRAGOON;
    case 70: return U_SCOUT; case 77: return U_FENIX;
    case 78: return U_FENIX_DRAGOON; case 83: return U_REAVER;
    case 79: return U_TASSADAR;
    case 2: return U_VULTURE;
#if defined(SC64_UNIFIED) && (defined(SC64_Z3_ENABLED) || defined(SC64_T5_RUNTIME))
    case 11: return U_DROPSHIP;
#endif
#if defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_UNIFIED) && !defined(SC64_T5_RUNTIME)
    case 11: return U_DROPSHIP;
#elif defined(SC64_SKIRMISH_ROSTER) && defined(SC64_UNIFIED) && \
      !defined(SC64_Z3_ENABLED) && !defined(SC64_T5_RUNTIME)
    case 11: return U_DROPSHIP;
#endif
    case 3: return U_GOLIATH; case 5: return U_TANK; case 30: return U_SIEGE_TANK;
    case 8: return U_WRAITH; case 43: return U_MUTALISK;
    case 53: return U_HUNTER_KILLER; case 216: return U_CHRYSALIS;
    case 47: return U_SCOURGE;
#ifdef SC64_Z5_ACTORS
    case 1: return U_Z5_GHOST; case 15: return U_Z5_CIVILIAN;
    case 89: return U_Z5_RHYNADON; case 90: return U_Z5_BENGALAAS;
    case 95: return U_Z5_RAGNASAUR; case 218: return U_Z5_DATA_DISK;
#endif
#ifdef SC64_Z6_ACTORS
    case 44: return U_Z6_GUARDIAN;
#endif
#if defined(SC64_T4_RUNTIME) && !defined(SC64_Z5_ACTORS)
    case 1: return U_GHOST; case 15: return U_CIVILIAN;
    case 89: return U_RHYNADON; case 95: return U_RAGNASAUR;
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case 101: return U_MAP_REVEALER;
#endif
    case 218: return U_DATA_DISK;
#endif
#ifdef SC64_T5_RUNTIME
    case 13: return U_SPIDER_MINE; case 16: return U_KERRIGAN;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    case 51: return U_ZP_INFESTED_KERRIGAN;
    case 67: return U_ZP_HIGH_TEMPLAR;
    case 101: return U_ZP_MAP_REVEALER;
    case 39: return U_ZP_ULTRALISK;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    case 9: return U_SCIENCE_VESSEL;
#ifndef SC64_P5_ACTORS
    case 12: return U_BATTLECRUISER;
#endif
    case 29: return U_NORAD_II;
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case 39: return U_ULTRALISK;
#endif
    case 217: return U_PSI_EMITTER;
#ifndef SC64_P5_ACTORS
    case 69: return U_SHUTTLE;
#endif
#endif
#ifdef SC64_P5_ACTORS
    case 69: return U_P5_SHUTTLE;
    case 12: return U_P5_BATTLECRUISER;
#endif
#ifdef SC64_P6_ACTORS
    case SC64_P6_RAW_INFESTED_TERRAN: return U_P6_INFESTED_TERRAN;
    case SC64_P6_RAW_HERO_DARK_TEMPLAR: return U_P6_HERO_DARK_TEMPLAR;
    case SC64_P6_RAW_ZERATUL: return U_P6_ZERATUL;
#endif
#ifdef SC64_P7_ACTORS
    case 68: return U_P7_ARCHON; case 71: return U_P7_ARBITER;
    case 72: return U_P7_CARRIER; case 84: return U_P7_OBSERVER;
#ifdef SC64_P7_PROJECTILES_ENABLED
    case 73: return U_P7_INTERCEPTOR;
#endif
#endif
#ifdef SC64_P8_DATA_ENABLED
    case 28: return U_P8_HYPERION;
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    case 36: return U_Z7_EGG;
#endif
#ifdef SC64_SKIRMISH_ROSTER
#ifndef SC64_TERRAN_LATE_ACTORS
    case 9: return U_SK_SCIENCE_VESSEL;
#ifndef SC64_P5_ACTORS
    case 12: return U_SK_BATTLECRUISER;
#endif
#endif
#ifndef SC64_Z6_ACTORS
    case 44: return U_SK_GUARDIAN;
#endif
    case 46: return U_SK_DEFILER; case 62: return U_SK_DEVOURER;
    case 103: return U_SK_LURKER;
#ifndef SC64_P6_ACTORS
    case 50: return U_SK_INFESTED_TERRAN;
#endif
    case 58: return U_SK_VALKYRIE;
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case 67: return U_SK_HIGH_TEMPLAR;
#endif
    case 61: return U_SK_DARK_TEMPLAR;
#ifndef SC64_P7_ACTORS
    case 84: return U_SK_OBSERVER; case 72: return U_SK_CARRIER;
    case 71: return U_SK_ARBITER; case 68: return U_SK_ARCHON;
#endif
    case 60: return U_SK_CORSAIR; case 63: return U_SK_DARK_ARCHON;
#if !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
    case 69: return U_SK_SHUTTLE;
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P5_ACTORS)
    case 12: return U_SHARED_BATTLECRUISER;
#endif
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
#ifdef SC64_Z5_ACTORS
    case 195: return B_Z5_TERRAN_BEACON;
#endif
#ifdef SC64_T4_RUNTIME
#ifndef SC64_Z5_ACTORS
    case 195: return B_TERRAN_BEACON;
#endif
#ifndef SC64_Z5_DOODADS
    case 203: return B_FLOOR_MISSILE_TRAP; case 205: return B_UPPER_DOOR;
    case 206: return B_RIGHT_UPPER_DOOR; case 207: return B_PIT_DOOR;
    case 208: return B_RIGHT_PIT_DOOR; case 209: return B_FLOOR_GUN_TRAP;
    case 211: return B_WALL_FLAME_TRAP; case 212: return B_RIGHT_WALL_MISSILE_TRAP;
#endif
#endif
#ifdef SC64_Z5_DOODADS
    case 203: return B_Z5_FLOOR_MISSILE_TRAP; case 205: return B_Z5_UPPER_DOOR;
    case 206: return B_Z5_RIGHT_UPPER_DOOR; case 207: return B_Z5_PIT_DOOR;
    case 208: return B_Z5_RIGHT_PIT_DOOR; case 209: return B_Z5_FLOOR_GUN_TRAP;
    case 211: return B_Z5_WALL_FLAME_TRAP; case 213: return B_Z5_RIGHT_WALL_FLAME_TRAP;
#endif
#if defined(SC64_T5_RUNTIME) && !defined(SC64_ZP_CAMPAIGN_ACTORS)
    case 115: return B_CONTROL_TOWER;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    case 115: return B_ZP_CONTROL_TOWER;
    case 120: return B_ZP_MACHINE_SHOP; case 123: return B_ZP_ARMORY;
    case 138: return B_ZP_QUEENS_NEST; case 140: return B_ZP_ULTRALISK_CAVERN;
    case 196: return B_ZP_PROTOSS_BEACON;
#endif
#ifdef SC64_T6_ENABLED
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case SC64_T6_CRASHED_NORAD_RAW: return B_CRASHED_NORAD_II;
    case SC64_T6_QUEENS_NEST_RAW: return B_QUEENS_NEST;
#endif
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    case 108: return B_NUCLEAR_SILO; case 117: return B_COVERT_OPS;
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case 120: return B_MACHINE_SHOP; case 123: return B_ARMORY;
#endif
    case 136: return B_DEFILER_MOUND;
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    case 140: return B_ULTRALISK_CAVERN;
#endif
#ifndef SC64_P7_ACTORS
    case 163: return B_CITADEL_ADUN; case 169: return B_FLEET_BEACON;
    case 171: return B_ROBOTICS_SUPPORT_BAY; case 172: return B_SHIELD_BATTERY;
#endif
    case 168: return B_STASIS_CELL_PRISON;
    case 127: return B_SPECIAL_ION_CANNON;
    case 116: return B_SCIENCE_FACILITY; case 118: return B_PHYSICS_LAB;
#endif
#if defined(SC64_P8_DATA_ENABLED) && !defined(SC64_TERRAN_LATE_ACTORS)
    case 168: return B_STASIS_CELL_PRISON;
#endif
#if defined(SC64_Z6_RUNTIME) && !defined(SC64_TERRAN_LATE_ACTORS) && !defined(SC64_P7_ACTORS)
    case 171: return B_Z6_ROBOTICS_SUPPORT_BAY;
    case 172: return B_Z6_SHIELD_BATTERY;
#endif
#ifdef SC64_P7_ACTORS
    case 159: return B_P7_OBSERVATORY; case 163: return B_P7_CITADEL_ADUN;
    case 165: return B_P7_TEMPLAR_ARCHIVES; case 169: return B_P7_FLEET_BEACON;
    case 170: return B_P7_ARBITER_TRIBUNAL;
    case 171: return B_P7_ROBOTICS_SUPPORT_BAY;
    case 172: return B_P7_SHIELD_BATTERY;
#endif
#ifdef SC64_P6_ACTORS
    case SC64_P6_RAW_WALL_MISSILE_TRAP: return B_P6_WALL_MISSILE_TRAP;
#endif
#ifdef SC64_NYDUS_ENABLED
    case 134: return B_NYDUS_CANAL;
#endif
#ifdef SC64_SKIRMISH_ROSTER
    case 107: return B_SK_COMSAT;
#ifndef SC64_NYDUS_ENABLED
    case 134: return B_SK_NYDUS;
#endif
#ifndef SC64_TERRAN_LATE_ACTORS
    case 108: return B_SK_NUCLEAR_SILO;
    case 117: return B_SK_COVERT_OPS;
    case 118: return B_SK_PHYSICS_LAB;
    case 116: return B_SK_SCIENCE_FACILITY;
    case 136: return B_SK_DEFILER_MOUND;
#ifndef SC64_P7_ACTORS
    case 163: return B_SK_CITADEL_ADUN;
    case 169: return B_SK_FLEET_BEACON;
#endif
#endif
#ifndef SC64_P7_ACTORS
    case 165: return B_SK_TEMPLAR_ARCHIVES;
    case 159: return B_SK_OBSERVATORY;
    case 170: return B_SK_ARBITER_TRIBUNAL;
#endif
#endif
#if defined(SC64_BATTLECRUISER_EXPERIMENT) && !defined(SC64_SKIRMISH_ROSTER) && !defined(SC64_TERRAN_LATE_ACTORS)
    case 116: return B_SHARED_SCIENCE_FACILITY;
    case 118: return B_SHARED_PHYSICS_LAB;
#endif
    default: return -1;
    }
}

#endif
