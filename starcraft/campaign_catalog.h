#ifndef SC64_CAMPAIGN_CATALOG_H
#define SC64_CAMPAIGN_CATALOG_H

#ifdef SC64_T4_CAMPAIGN_ENTRY
#if !defined(SC64_T3_ENABLED) || !defined(SC64_T4_SCENARIO_RUNTIME) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || \
    SC64_CAMPAIGN_RACE != 0
#error "SC64_T4_CAMPAIGN_ENTRY requires the combined unified Terran T3/T4 profile"
#endif
#ifdef SC64_Z5_CAMPAIGN_ENTRY
#if !defined(SC64_Z4_CAMPAIGN_ENTRY) || !defined(SC64_Z4_RUNTIME) || \
    !defined(SC64_Z5_RUNTIME) || !defined(SC64_Z5_ENABLED) || \
    !defined(SC64_Z5_DATA_ENABLED) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_Z5_ACTORS) || !defined(SC64_Z5_DOODADS) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || \
    SC64_CAMPAIGN_RACE != 1
#error "SC64_Z5_CAMPAIGN_ENTRY requires the authenticated Z4/Z5 full-storage chain"
#endif
#endif
#endif
#ifdef SC64_P5_CAMPAIGN_ENTRY
#if !defined(SC64_P4_CAMPAIGN_ENTRY) || !defined(SC64_P4_STORM_ENABLED) || \
    !defined(SC64_P5_RUNTIME) || !defined(SC64_P5_ENABLED) || \
    !defined(SC64_P5_DATA_ENABLED) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_P5_ACTORS) || !defined(SC64_Z6_ACTORS) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || \
    SC64_CAMPAIGN_RACE != 2
#error "P5 campaign entry requires the authenticated P4/P5 Protoss runtime chain"
#endif
#endif
#ifdef SC64_P6_CAMPAIGN_ENTRY
#if !defined(SC64_P5_CAMPAIGN_ENTRY) || !defined(SC64_P6_RUNTIME) || \
    !defined(SC64_P6_DATA_ENABLED) || !defined(SC64_P6_ACTORS) || \
    !defined(SC64_P6_COMBAT_ENABLED) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2
#error "P6 entry requires the authenticated P1-P6 runtime, actors and combat profile"
#endif
#endif
#ifdef SC64_P7_CAMPAIGN_ENTRY
#if !defined(SC64_P6_CAMPAIGN_ENTRY) || !defined(SC64_P7_RUNTIME) || \
    !defined(SC64_P7_DATA_ENABLED) || !defined(SC64_P7_ACTORS) || \
    !defined(SC64_P7_BATTERY_ENABLED) || !defined(SC64_P7_OBSERVER_ENABLED) || \
    !defined(SC64_P7_ARCHON_SPLASH) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2
#error "P7 entry requires the authenticated P1-P7 Protoss runtime, data, actors and ability profile"
#endif
#endif
#ifdef SC64_P8_CAMPAIGN_ENTRY
#if !defined(SC64_P7_CAMPAIGN_ENTRY) || !defined(SC64_P8_RUNTIME) || \
    !defined(SC64_P8_DATA_ENABLED) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2
#error "P8 entry requires the authenticated P1-P8 Protoss runtime and data"
#endif
#endif
#ifdef SC64_Z6_CAMPAIGN_ENTRY
#if !defined(SC64_Z5_CAMPAIGN_ENTRY) || !defined(SC64_Z6_RUNTIME) || \
    !defined(SC64_Z6_DATA_ENABLED) || !defined(SC64_Z6_ACTORS) || \
    !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 1
#error "Z6 entry requires its authenticated Z1-Z6 runtime and full storage"
#endif
#endif
#ifdef SC64_Z7_CAMPAIGN_ENTRY
#if !defined(SC64_Z6_CAMPAIGN_ENTRY) || !defined(SC64_Z7_RUNTIME) || \
    !defined(SC64_Z7_DATA_ENABLED) || !defined(SC64_Z6_ACTORS) || \
    !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 1
#error "Z7 entry requires its authenticated Z1-Z7 runtime and full storage"
#endif
#endif
#ifdef SC64_Z8_CAMPAIGN_ENTRY
#if !defined(SC64_Z7_CAMPAIGN_ENTRY) || !defined(SC64_Z8_RUNTIME) || \
    !defined(SC64_Z8_DATA_ENABLED) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 1
#error "Z8 entry requires the authenticated Z1-Z8 Zerg runtime and data"
#endif
#endif
#ifdef SC64_T5_CAMPAIGN_ENTRY
#if !defined(SC64_T3_ENABLED) || !defined(SC64_T4_CAMPAIGN_ENTRY) || \
    !defined(SC64_T4_SCENARIO_RUNTIME) || !defined(SC64_T5_ENABLED) || \
    !defined(SC64_T5_RUNTIME) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0
#error "SC64_T5_CAMPAIGN_ENTRY requires the combined unified Terran T3/T4/T5 profile"
#endif
#ifdef SC64_T6_CAMPAIGN_ENTRY
#if !defined(SC64_T3_ENABLED) || !defined(SC64_T4_CAMPAIGN_ENTRY) || \
    !defined(SC64_T5_CAMPAIGN_ENTRY) || !defined(SC64_T6_RUNTIME) || \
    !defined(SC64_T6_ENABLED) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0
#error "SC64_T6_CAMPAIGN_ENTRY requires the unified Terran T1-T6 runtime chain"
#endif
#endif
#ifdef SC64_TERRAN_FULL_CAMPAIGN_ENTRY
#if !defined(SC64_T6_CAMPAIGN_ENTRY) || !defined(SC64_TERRAN_LATE_RUNTIME) || \
    !defined(SC64_TERRAN_LATE_ENABLED) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0
#error "SC64_TERRAN_FULL_CAMPAIGN_ENTRY requires unified Terran T1-T10 runtime"
#endif
#endif
#endif

/* Asset-free metadata for the ten original missions in each campaign.
 * Global IDs 0..5 are a save/loader contract and must not be reordered. */
enum {
    SC64_FACTION_TERRAN = 0,
    SC64_FACTION_ZERG = 1,
    SC64_FACTION_PROTOSS = 2,
    SC64_BASE_MISSION_COUNT = 30,
    SC64_CAMPAIGN_MISSION_COUNT = 10
};

typedef struct Sc64MissionInfo {
    int id;
    int faction;
    int chapter; /* 1-based mission number within its race campaign */
    int playable;
    const char *title;
} Sc64MissionInfo;

static const Sc64MissionInfo sc64_campaign_catalog[SC64_BASE_MISSION_COUNT] = {
    {0, SC64_FACTION_ZERG, 1, 1, "AMONG THE RUINS"},
    {1, SC64_FACTION_ZERG, 2, 1, "EGRESSION"},
    {2, SC64_FACTION_PROTOSS, 1, 1, "FIRST STRIKE"},
    {3, SC64_FACTION_PROTOSS, 2, 1, "INTO THE FLAMES"},
    {4, SC64_FACTION_TERRAN, 1, 1, "WASTELAND"},
    {5, SC64_FACTION_TERRAN, 2, 1, "BACKWATER"},
    {6, SC64_FACTION_TERRAN, 3,
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
     1,
#else
     0,
#endif
     "DESPERATE ALLIANCE"},
    {7, SC64_FACTION_TERRAN, 4,
#ifdef SC64_T4_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "THE JACOBS INSTALLATION"},
    {8, SC64_FACTION_TERRAN, 5,
#ifdef SC64_T5_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "REVOLUTION"},
    {9, SC64_FACTION_TERRAN, 6,
#ifdef SC64_T6_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "NORAD II"},
    {10, SC64_FACTION_TERRAN, 7,
#ifdef SC64_TERRAN_FULL_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "THE TRUMP CARD"},
    {11, SC64_FACTION_TERRAN, 8,
#ifdef SC64_TERRAN_FULL_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "THE BIG PUSH"},
    {12, SC64_FACTION_TERRAN, 9,
#ifdef SC64_TERRAN_FULL_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "NEW GETTYSBURG"},
    {13, SC64_FACTION_TERRAN, 10,
#ifdef SC64_TERRAN_FULL_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "THE HAMMER FALLS"},
    {14, SC64_FACTION_ZERG, 3,
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
     1,
#else
     0,
#endif
     "THE NEW DOMINION"},
    {15, SC64_FACTION_ZERG, 4,
#if defined(SC64_Z4_CAMPAIGN_ENTRY) && defined(SC64_Z4_RUNTIME) && defined(SC64_Z4_ENABLED) && defined(SC64_UNIFIED)
     1,
#else
     0,
#endif
     "AGENT OF THE SWARM"},
    {16, SC64_FACTION_ZERG, 5,
#ifdef SC64_Z5_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "THE AMERIGO"},
    {17, SC64_FACTION_ZERG, 6,
#ifdef SC64_Z6_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "THE DARK TEMPLAR"},
    {18, SC64_FACTION_ZERG, 7,
#ifdef SC64_Z7_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "THE CULLING"},
    {19, SC64_FACTION_ZERG, 8,
#ifdef SC64_Z8_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "EYE FOR AN EYE"},
    {20, SC64_FACTION_ZERG, 9, 0, "THE INVASION OF AIUR"},
    {21, SC64_FACTION_ZERG, 10, 0, "FULL CIRCLE"},
    {22, SC64_FACTION_PROTOSS, 3,
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
     1,
#else
     0,
#endif
     "HIGHER GROUND"},
    {23, SC64_FACTION_PROTOSS, 4,
#if defined(SC64_P4_CAMPAIGN_ENTRY) && defined(SC64_P4_RUNTIME) && defined(SC64_P4_ENABLED) && defined(SC64_UNIFIED)
     1,
#else
     0,
#endif
     "THE HUNT FOR TASSADAR"},
    {24, SC64_FACTION_PROTOSS, 5,
#ifdef SC64_P5_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "CHOOSING SIDES"},
    {25, SC64_FACTION_PROTOSS, 6,
#ifdef SC64_P6_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "INTO THE DARKNESS"},
    {26, SC64_FACTION_PROTOSS, 7,
#ifdef SC64_P7_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "HOMELAND"},
    {27, SC64_FACTION_PROTOSS, 8,
#ifdef SC64_P8_CAMPAIGN_ENTRY
     1,
#else
     0,
#endif
     "THE TRIAL"},
    {28, SC64_FACTION_PROTOSS, 9, 0, "SHADOW HUNTERS"},
    {29, SC64_FACTION_PROTOSS, 10, 0, "EYE OF THE STORM"}
};

static inline const Sc64MissionInfo *sc64_catalog_get(int id) {
    return id < 0 || id >= SC64_BASE_MISSION_COUNT ? 0 : &sc64_campaign_catalog[id];
}

static inline int sc64_catalog_mission_faction(int id) {
    const Sc64MissionInfo *m = sc64_catalog_get(id);
    return m ? m->faction : -1;
}

static inline int sc64_catalog_first_mission(int faction) {
    return faction == SC64_FACTION_ZERG ? 0 :
           faction == SC64_FACTION_PROTOSS ? 2 :
           faction == SC64_FACTION_TERRAN ? 4 : -1;
}

static inline int sc64_catalog_mission_at(int faction, int ordinal) {
    int first = sc64_catalog_first_mission(faction);
    if (first < 0 || ordinal < 0 || ordinal >= SC64_CAMPAIGN_MISSION_COUNT) return -1;
    if (ordinal < 2) return first + ordinal;
    if (faction == SC64_FACTION_TERRAN) return 6 + ordinal - 2;
    if (faction == SC64_FACTION_ZERG) return 14 + ordinal - 2;
    return 22 + ordinal - 2;
}

static inline int sc64_catalog_mission_count(int faction) {
    return sc64_catalog_first_mission(faction) < 0 ? 0 : SC64_CAMPAIGN_MISSION_COUNT;
}

static inline int sc64_catalog_is_playable(int id) {
    const Sc64MissionInfo *m = sc64_catalog_get(id);
    return m ? m->playable : 0;
}

static inline int sc64_catalog_mission_allowed(int id, int profile_race) {
    const Sc64MissionInfo *m = sc64_catalog_get(id);
    return m && m->playable && (profile_race < 0 || m->faction == profile_race);
}

static inline unsigned sc64_catalog_playable_mask(int profile_race) {
    unsigned mask = 0;
    for (int i = 0; i < SC64_BASE_MISSION_COUNT; ++i)
        if (sc64_catalog_mission_allowed(i, profile_race)) mask |= 1u << i;
    return mask;
}

static inline int sc64_catalog_available_next(int faction, int mission) {
    int found = 0;
    for (int i = 0; i < SC64_BASE_MISSION_COUNT; ++i) {
        if (sc64_campaign_catalog[i].faction != faction || !sc64_campaign_catalog[i].playable) continue;
        if (found) return i;
        if (i == mission) found = 1;
    }
    return -1;
}

static inline int sc64_catalog_playable_count(int faction) {
    int count = 0;
    for (int i = 0; i < SC64_BASE_MISSION_COUNT; ++i)
        if (sc64_campaign_catalog[i].faction == faction && sc64_campaign_catalog[i].playable) ++count;
    return count;
}

static inline int sc64_catalog_playable_at(int faction, int ordinal) {
    if (ordinal < 0) return -1;
    for (int i = 0; i < SC64_BASE_MISSION_COUNT; ++i)
        if (sc64_campaign_catalog[i].faction == faction && sc64_campaign_catalog[i].playable) {
            if (!ordinal--) return i;
        }
    return -1;
}

static inline int sc64_catalog_playable_ordinal(int faction, int mission) {
    int ordinal = 0;
    for (int i = 0; i < SC64_BASE_MISSION_COUNT; ++i) {
        if (sc64_campaign_catalog[i].faction != faction || !sc64_campaign_catalog[i].playable) continue;
        if (i == mission) return ordinal;
        ++ordinal;
    }
    return -1;
}

#endif
