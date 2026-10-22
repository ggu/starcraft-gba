#ifndef SC64_FRONTEND_H
#define SC64_FRONTEND_H

#include "gba.h"

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
} Sc64Frontend;

/* Mission IDs are a stable save/loader contract: Z1,Z2,P1,P2,T1,T2. */
static inline int sc64_frontend_mission_faction(int mission) {
#ifdef SC64_UNIFIED
    if (mission >= 4) return SC_FRONT_TERRAN;
    if (mission >= 2) return SC_FRONT_PROTOSS;
    return SC_FRONT_ZERG;
#else
    return mission >= 2 ? SC_FRONT_PROTOSS : SC_FRONT_ZERG;
#endif
}

static inline int sc64_frontend_first_mission(int faction) {
#ifdef SC64_UNIFIED
    return faction == SC_FRONT_TERRAN ? 4 :
           faction == SC_FRONT_PROTOSS ? 2 : 0;
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

static inline int sc64_front_clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
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
    f->faction = sc64_frontend_mission_faction(f->mission);
    f->race = sc64_front_clamp(race, SC_FRONT_RACE_TERRAN,
                               SC_FRONT_RACE_PROTOSS);
    f->bots = sc64_front_clamp(bots, 1, 3);
#ifdef SC64_UNIFIED
    f->completed = sc64_front_clamp(completed, -1, 5);
#else
    f->completed = sc64_front_clamp(completed, -1, 3);
#endif
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
        case SC_FRONT_MISSION: f->page = SC_FRONT_CAMPAIGN; f->row = sc64_front_campaign_row(f->faction); break;
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
#ifdef SC64_UNIFIED
            count = 3;
#else
            count = 2;
#endif
            break;
        case SC_FRONT_MISSION: count = 2; break;
        case SC_FRONT_SKIRMISH: count = 3; break;
        default: count = 1; break;
        }
        f->row = sc64_front_wrap(f->row + delta, count);
    }

    if (f->page == SC_FRONT_CAMPAIGN && (edge & (KEY_LEFT | KEY_RIGHT))) {
        f->row = sc64_front_wrap(f->row + ((edge & KEY_RIGHT) ? 1 : -1),
#ifdef SC64_UNIFIED
                                 3
#else
                                 2
#endif
                                 );
        f->faction = sc64_front_campaign_faction(f->row);
    } else if (f->page == SC_FRONT_SKIRMISH && (edge & (KEY_LEFT | KEY_RIGHT))) {
        int d = (edge & KEY_RIGHT) ? 1 : -1;
        if (f->row == 0) f->race = sc64_front_wrap(f->race + d, 3);
        else if (f->row == 1) f->bots = sc64_front_wrap(f->bots - 1 + d, 3) + 1;
    }

    if (!(edge & (KEY_A | KEY_START))) return SC_FRONT_NONE;
    switch (f->page) {
    case SC_FRONT_HOME:
        if (f->row == 0) {
            f->page = SC_FRONT_CAMPAIGN;
            f->row = sc64_front_campaign_row(f->faction);
        } else if (f->row == 1) {
            f->page = SC_FRONT_SKIRMISH;
            f->row = 0;
        } else {
            return SC_FRONT_LOAD;
        }
        break;
    case SC_FRONT_CAMPAIGN:
        f->faction = sc64_front_campaign_faction(f->row);
        f->page = SC_FRONT_MISSION;
#ifdef SC64_UNIFIED
        f->row = sc64_frontend_mission_faction(f->mission) == f->faction ?
                 f->mission - sc64_frontend_first_mission(f->faction) : 0;
#else
        f->row = f->mission / 2 == f->faction ? f->mission % 2 : 0;
#endif
        break;
    case SC_FRONT_MISSION:
        f->mission = sc64_frontend_first_mission(f->faction) + f->row;
        f->page = SC_FRONT_BRIEFING;
        break;
    case SC_FRONT_BRIEFING:
        return SC_FRONT_START_CAMPAIGN;
    case SC_FRONT_SKIRMISH:
        if (f->row == 2) return SC_FRONT_START_SKIRMISH;
        f->row = 2;
        break;
    default:
        f->page = SC_FRONT_HOME;
        f->row = 0;
        break;
    }
    return SC_FRONT_NONE;
}

#endif
