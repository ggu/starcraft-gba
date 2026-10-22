#ifndef SC64_FRONTEND_H
#define SC64_FRONTEND_H

#include "gba.h"
#include "campaign_catalog.h"

#ifndef SC64_MAP_OUTPOST
#define SC64_MAP_OUTPOST 0
#endif
#ifndef SC64_MAP_BLOOD_BATH
#define SC64_MAP_BLOOD_BATH 1
#endif
#ifdef SC64_IMPORTED_SKIRMISH
#define SC64_FRONT_MAP_COUNT 2
#else
#define SC64_FRONT_MAP_COUNT 1
#endif
#ifndef SC64_MAP_COUNT
#define SC64_MAP_COUNT SC64_FRONT_MAP_COUNT
#endif

/* Small, deterministic input model for the optional StarCraft 64 frontend.
 * Rendering is deliberately left to the caller. `held` is the current
 * held-key mask (using KEY_* from common/gba.h); actions fire on key edges.
 */
enum {
    SC_FRONT_HOME = 0,
    SC_FRONT_CAMPAIGN,
    SC_FRONT_MISSION,
    SC_FRONT_BRIEFING,
    SC_FRONT_SKIRMISH
};

enum {
    SC_FRONT_NONE = 0,
    SC_FRONT_START_CAMPAIGN,
    SC_FRONT_START_SKIRMISH,
    SC_FRONT_LOAD
};

enum {
#ifdef SC64_UNIFIED
    SC_FRONT_TERRAN = 0,
    SC_FRONT_ZERG = 1,
    SC_FRONT_PROTOSS = 2
#else
    SC_FRONT_ZERG = 0,
    SC_FRONT_PROTOSS = 1
#endif
};

enum {
    SC_FRONT_RACE_TERRAN = 0,
    SC_FRONT_RACE_ZERG = 1,
    SC_FRONT_RACE_PROTOSS = 2
};

typedef struct Sc64Frontend {
    unsigned ticks;
    unsigned prev_buttons;
    int page;
    int row;
    int faction;       /* Campaign race; unified builds use Terran/Zerg/Protoss IDs */
    int mission;       /* Global campaign mission index */
    int race;          /* Skirmish race: Terran 0, Zerg 1, Protoss 2 */
    int bots;          /* Number of opponents, clamped to 1-3 */
    int completed;     /* Highest completed global campaign mission, or -1 */
    int map;           /* Appended to preserve existing field offsets */
    int bot_races[3];  /* Per-opponent race choices, retained as count changes */
    int mission_page;
} Sc64Frontend;

/* Mission IDs are a stable save/loader contract: Z1,Z2,P1,P2,T1,T2. */
static inline int sc64_frontend_mission_faction(int mission) {
#ifdef SC64_UNIFIED
    int faction = sc64_catalog_mission_faction(mission);
    return faction == SC64_FACTION_TERRAN ? SC_FRONT_TERRAN :
           faction == SC64_FACTION_PROTOSS ? SC_FRONT_PROTOSS : SC_FRONT_ZERG;
#else
    return mission >= 2 ? SC_FRONT_PROTOSS : SC_FRONT_ZERG;
#endif
}

static inline int sc64_frontend_first_mission(int faction) {
#ifdef SC64_UNIFIED
    return sc64_catalog_first_mission(faction == SC_FRONT_TERRAN ? SC64_FACTION_TERRAN :
           faction == SC_FRONT_PROTOSS ? SC64_FACTION_PROTOSS : SC64_FACTION_ZERG);
#else
    return faction == SC_FRONT_PROTOSS ? 2 : 0;
#endif
}

static inline int sc64_front_campaign_row(int faction) {
#ifdef SC64_UNIFIED
    /* The presentation order favors the original Terran identity. */
    return faction == SC_FRONT_TERRAN ? 0 :
           faction == SC_FRONT_ZERG ? 1 : 2;
#else
    return faction;
#endif
}

static inline int sc64_front_campaign_faction(int row) {
#ifdef SC64_UNIFIED
    return row == 0 ? SC_FRONT_TERRAN :
           row == 1 ? SC_FRONT_ZERG : SC_FRONT_PROTOSS;
#else
    return row;
#endif
}

static inline int sc64_front_campaign_count(void) {
#ifdef SC64_CAMPAIGN_RACE
    return 1;
#elif defined(SC64_UNIFIED)
    return 3;
#else
    return 2;
#endif
}

static inline int sc64_front_campaign_faction_at(int row) {
#ifdef SC64_CAMPAIGN_RACE
#if SC64_CAMPAIGN_RACE == 0
    (void)row;
    return SC_FRONT_TERRAN;
#elif SC64_CAMPAIGN_RACE == 1
    (void)row;
    return SC_FRONT_ZERG;
#elif SC64_CAMPAIGN_RACE == 2
    (void)row;
    return SC_FRONT_PROTOSS;
#else
#error "SC64_CAMPAIGN_RACE must be 0 (Terran), 1 (Zerg), or 2 (Protoss)"
#endif
#else
    (void)row;
    return sc64_front_campaign_faction(row);
#endif
}

static inline int sc64_front_clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline int sc64_front_catalog_faction(int faction) {
#ifdef SC64_UNIFIED
    return faction == SC_FRONT_TERRAN ? SC64_FACTION_TERRAN :
           faction == SC_FRONT_PROTOSS ? SC64_FACTION_PROTOSS : SC64_FACTION_ZERG;
#else
    return faction == SC_FRONT_PROTOSS ? SC64_FACTION_PROTOSS : SC64_FACTION_ZERG;
#endif
}

static inline int sc64_front_mission_rows(const Sc64Frontend *f) {
    int remaining;
    if (!f) return 1;
    remaining = sc64_catalog_playable_count(sc64_front_catalog_faction(f->faction)) -
                f->mission_page * 2;
    return remaining > 1 ? 2 : 1;
}

static inline void sc64_frontend_reset(Sc64Frontend *f, int mission,
                                        int race, int bots, int completed) {
    if (!f) return;
    f->ticks = 0;
    f->prev_buttons = 0;
    f->page = SC_FRONT_HOME;
    f->row = 0;
#ifdef SC64_UNIFIED
    f->mission = sc64_front_clamp(mission, 0, 5);
#else
    f->mission = sc64_front_clamp(mission, 0, 3);
#endif
#ifdef SC64_CAMPAIGN_RACE
    f->faction = sc64_front_campaign_faction_at(0);
    f->mission = sc64_catalog_playable_at(sc64_front_catalog_faction(f->faction), 0);
#else
    f->faction = sc64_frontend_mission_faction(f->mission);
#endif
    f->race = sc64_front_clamp(race, SC_FRONT_RACE_TERRAN,
                               SC_FRONT_RACE_PROTOSS);
    f->bots = sc64_front_clamp(bots, 1, 3);
#ifdef SC64_UNIFIED
    f->completed = sc64_front_clamp(completed, -1, 5);
#else
    f->completed = sc64_front_clamp(completed, -1, 3);
#endif
    f->map = SC64_MAP_OUTPOST;
    for (int i = 0; i < 3; ++i) {
        int ai = (f->race + i + 1) % 3;
        if (ai == f->race) ai = (ai + 1) % 3;
        f->bot_races[i] = ai;
    }
    f->mission_page = 0;
}

static inline int sc64_front_wrap(int v, int count) {
    if (v < 0) return count - 1;
    if (v >= count) return 0;
    return v;
}

/* Pass the currently-held GBA key mask. Navigation and confirmation are
 * edge-triggered. L quick-loads from every page; B backs out, and on HOME
 * B retains the historical quick-load shortcut. */
static inline int sc64_frontend_update(Sc64Frontend *f, unsigned held) {
    unsigned edge;
    if (!f) return SC_FRONT_NONE;
    f->ticks++;
    edge = held & ~f->prev_buttons;
    f->prev_buttons = held;

    if (edge & KEY_L) return SC_FRONT_LOAD;

    if (edge & KEY_B) {
        switch (f->page) {
        case SC_FRONT_HOME: return SC_FRONT_LOAD;
        case SC_FRONT_CAMPAIGN: f->page = SC_FRONT_HOME; f->row = 0; break;
        case SC_FRONT_MISSION:
            f->page = SC_FRONT_CAMPAIGN;
#ifdef SC64_CAMPAIGN_RACE
            f->row = 0;
#else
            f->row = sc64_front_campaign_row(f->faction);
#endif
            break;
        case SC_FRONT_BRIEFING: f->page = SC_FRONT_MISSION; f->row = f->mission % 2; break;
        case SC_FRONT_SKIRMISH: f->page = SC_FRONT_HOME; f->row = 1; break;
        default: f->page = SC_FRONT_HOME; f->row = 0; break;
        }
        return SC_FRONT_NONE;
    }

    if (edge & (KEY_UP | KEY_DOWN)) {
        int delta = (edge & KEY_DOWN) ? 1 : -1;
        int count = 1;
        switch (f->page) {
        case SC_FRONT_HOME: count = 3; break;
        case SC_FRONT_CAMPAIGN:
            count = sc64_front_campaign_count();
            break;
        case SC_FRONT_MISSION: count = sc64_front_mission_rows(f); break;
        case SC_FRONT_SKIRMISH:
#ifdef SC64_IMPORTED_SKIRMISH
            count = 4 + f->bots;
#else
            count = 3;
#endif
            break;
        default: count = 1; break;
        }
        f->row = sc64_front_wrap(f->row + delta, count);
    }

    if (f->page == SC_FRONT_MISSION && (edge & (KEY_LEFT | KEY_RIGHT))) {
        int total = sc64_catalog_playable_count(sc64_front_catalog_faction(f->faction));
        int pages = (total + 1) / 2;
        if (pages > 1) f->mission_page = sc64_front_wrap(f->mission_page +
            ((edge & KEY_RIGHT) ? 1 : -1), pages);
        f->row = 0;
    } else if (f->page == SC_FRONT_CAMPAIGN && (edge & (KEY_LEFT | KEY_RIGHT))) {
        f->row = sc64_front_wrap(f->row + ((edge & KEY_RIGHT) ? 1 : -1),
                                 sc64_front_campaign_count()
                                 );
        f->faction = sc64_front_campaign_faction_at(f->row);
    } else if (f->page == SC_FRONT_SKIRMISH && (edge & (KEY_LEFT | KEY_RIGHT))) {
        int d = (edge & KEY_RIGHT) ? 1 : -1;
#ifdef SC64_IMPORTED_SKIRMISH
        if (f->row == 0) {
            f->map = sc64_front_wrap(f->map + d, SC64_FRONT_MAP_COUNT);
        } else if (f->row == 1) f->race = sc64_front_wrap(f->race + d, 3);
        else if (f->row == 2) {
            f->bots = sc64_front_wrap(f->bots - 1 + d, 3) + 1;
            f->row = sc64_front_clamp(f->row, 0, 3 + f->bots);
        } else if (f->row >= 3 && f->row < 3 + f->bots)
            f->bot_races[f->row - 3] = sc64_front_wrap(f->bot_races[f->row - 3] + d, 3);
#else
        if (f->row == 0) f->race = sc64_front_wrap(f->race + d, 3);
        else if (f->row == 1) f->bots = sc64_front_wrap(f->bots - 1 + d, 3) + 1;
#endif
    }

    if (!(edge & (KEY_A | KEY_START))) return SC_FRONT_NONE;
    switch (f->page) {
    case SC_FRONT_HOME:
        if (f->row == 0) {
#ifdef SC64_CAMPAIGN_RACE
            f->faction = sc64_front_campaign_faction_at(0);
            f->page = SC_FRONT_MISSION;
            f->row = 0;
#else
            f->page = SC_FRONT_CAMPAIGN;
            f->row = sc64_front_campaign_row(f->faction);
#endif
        } else if (f->row == 1) {
            f->page = SC_FRONT_SKIRMISH;
            f->row = 0;
        } else {
            return SC_FRONT_LOAD;
        }
        break;
    case SC_FRONT_CAMPAIGN:
        f->faction = sc64_front_campaign_faction_at(f->row);
        f->page = SC_FRONT_MISSION;
#ifdef SC64_UNIFIED
        f->mission_page = 0;
        f->row = sc64_frontend_mission_faction(f->mission) == f->faction ?
                 (sc64_catalog_playable_at(sc64_front_catalog_faction(f->faction), 1) == f->mission ? 1 : 0) : 0;
#else
        f->row = f->mission / 2 == f->faction ? f->mission % 2 : 0;
#endif
        break;
    case SC_FRONT_MISSION:
        f->mission = sc64_catalog_playable_at(sc64_front_catalog_faction(f->faction),
                                               f->mission_page * 2 + f->row);
        if (f->mission < 0) return SC_FRONT_NONE;
        f->page = SC_FRONT_BRIEFING;
        break;
    case SC_FRONT_BRIEFING:
        return SC_FRONT_START_CAMPAIGN;
    case SC_FRONT_SKIRMISH:
#ifdef SC64_IMPORTED_SKIRMISH
        if (f->row == 3 + f->bots) return SC_FRONT_START_SKIRMISH;
        f->row = 3 + f->bots;
#else
        if (f->row == 2) return SC_FRONT_START_SKIRMISH;
        f->row = 2;
#endif
        break;
    default:
        f->page = SC_FRONT_HOME;
        f->row = 0;
        break;
    }
    return SC_FRONT_NONE;
}

#endif
