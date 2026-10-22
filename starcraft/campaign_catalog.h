#ifndef SC64_CAMPAIGN_CATALOG_H
#define SC64_CAMPAIGN_CATALOG_H

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
    {7, SC64_FACTION_TERRAN, 4, 0, "THE JACOBS INSTALLATION"},
    {8, SC64_FACTION_TERRAN, 5, 0, "REVOLUTION"},
    {9, SC64_FACTION_TERRAN, 6, 0, "NORAD II"},
    {10, SC64_FACTION_TERRAN, 7, 0, "THE TRUMP CARD"},
    {11, SC64_FACTION_TERRAN, 8, 0, "THE BIG PUSH"},
    {12, SC64_FACTION_TERRAN, 9, 0, "NEW GETTYSBURG"},
    {13, SC64_FACTION_TERRAN, 10, 0, "THE HAMMER FALLS"},
    {14, SC64_FACTION_ZERG, 3,
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
     1,
#else
     0,
#endif
     "THE NEW DOMINION"},
    {15, SC64_FACTION_ZERG, 4, 0, "AGENT OF THE SWARM"},
    {16, SC64_FACTION_ZERG, 5, 0, "THE AMERIGO"},
    {17, SC64_FACTION_ZERG, 6, 0, "THE DARK TEMPLAR"},
    {18, SC64_FACTION_ZERG, 7, 0, "THE CULLING"},
    {19, SC64_FACTION_ZERG, 8, 0, "EYE FOR AN EYE"},
    {20, SC64_FACTION_ZERG, 9, 0, "THE INVASION OF AIUR"},
    {21, SC64_FACTION_ZERG, 10, 0, "FULL CIRCLE"},
    {22, SC64_FACTION_PROTOSS, 3,
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
     1,
#else
     0,
#endif
     "HIGHER GROUND"},
    {23, SC64_FACTION_PROTOSS, 4, 0, "THE HUNT FOR TASSADAR"},
    {24, SC64_FACTION_PROTOSS, 5, 0, "CHOOSING SIDES"},
    {25, SC64_FACTION_PROTOSS, 6, 0, "INTO THE DARKNESS"},
    {26, SC64_FACTION_PROTOSS, 7, 0, "HOMELAND"},
    {27, SC64_FACTION_PROTOSS, 8, 0, "THE TRIAL"},
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
