#ifndef SC64_FRONTEND_H
#define SC64_FRONTEND_H

#include "gba.h"
#include "campaign_catalog.h"
#include "sc64_briefing.h"

#ifdef SC64_T4_REPLAY_ENTRY
#if !defined(SC64_T4_SCENARIO_RUNTIME)
#error "SC64_T4_REPLAY_ENTRY requires SC64_T4_SCENARIO_RUNTIME"
#endif
#if !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0
#error "SC64_T4_REPLAY_ENTRY requires the unified Terran campaign profile"
#endif
#ifndef SC64_T4_GLOBAL_ID7
#define SC64_T4_GLOBAL_ID7 7
#endif
#endif

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

#ifdef SC64_SHOWCASE_ENABLED
#define SC64_FRONT_SHOWCASE_COUNT 8
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
#ifdef SC64_SHOWCASE_ENABLED
    , SC_FRONT_SHOWCASE
#endif
};

enum {
    SC_FRONT_NONE = 0,
    SC_FRONT_START_CAMPAIGN,
    SC_FRONT_START_SKIRMISH,
    SC_FRONT_LOAD
#ifdef SC64_SHOWCASE_ENABLED
    , SC_FRONT_START_SHOWCASE
#endif
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
    /* Briefing fields are appended so existing users retain their offsets. */
    unsigned briefing_page;
    unsigned briefing_pages;
    unsigned briefing_page_tick;
    unsigned briefing_reveal_length;
    int briefing_revealed;
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

/* Keep briefing playback independent from rendering and generated campaign
 * headers. Configure it from the currently-selected generated page before
 * drawing; page_count is clamped to a small fixed bound and zero means one
 * page, which preserves the asset-free frontend's historical behavior. The
 * reveal length counts bytes in the prewrapped page text, including newlines. */
static inline void sc64_frontend_briefing_configure(Sc64Frontend *f,
                                                     unsigned page_count,
                                                     unsigned reveal_length) {
    if (!f) return;
    if (page_count == 0) page_count = 1;
    if (page_count > SC64_BRIEFING_MAX_PAGES)
        page_count = SC64_BRIEFING_MAX_PAGES;
    if (reveal_length > SC64_BRIEFING_MAX_TEXT_BYTES)
        reveal_length = SC64_BRIEFING_MAX_TEXT_BYTES;
    f->briefing_pages = page_count;
    if (f->briefing_page >= page_count) f->briefing_page = page_count - 1;
    f->briefing_reveal_length = reveal_length;
}

static inline void sc64_frontend_briefing_reset_page(Sc64Frontend *f) {
    if (!f) return;
    f->briefing_page = 0;
    f->briefing_pages = 1;
    f->briefing_page_tick = f->ticks;
    f->briefing_reveal_length = 0;
    f->briefing_revealed = 0;
}

/* Text reveals at two bytes per frontend tick. Manual reveal wins immediately;
 * unsigned subtraction also keeps the age calculation valid across tick wrap. */
static inline unsigned sc64_frontend_briefing_visible(const Sc64Frontend *f) {
    unsigned elapsed;
    if (!f) return 0;
    if (f->briefing_revealed) return f->briefing_reveal_length;
    elapsed = f->ticks - f->briefing_page_tick;
    if (elapsed >= (f->briefing_reveal_length + 1u) / 2u)
        return f->briefing_reveal_length;
    return elapsed * 2u;
}

static inline int sc64_frontend_briefing_is_revealed(const Sc64Frontend *f) {
    return !f || sc64_frontend_briefing_visible(f) >= f->briefing_reveal_length;
}

#ifdef SC64_SHOWCASE_ENABLED
static inline const char *sc64_front_showcase_name(int scene) {
    static const char *const names[SC64_FRONT_SHOWCASE_COUNT] = {
        "CARRIER APPROACH", "SIEGE LINE", "TERRAN LAST STAND",
        "ZERG LAST STAND", "PROTOSS LAST STAND", "PROTOSS CROSS FIRE",
        "THREE WAY BATTLE", "BASE UNDER SIEGE"
    };
    return names[sc64_front_clamp(scene, 0, SC64_FRONT_SHOWCASE_COUNT - 1)];
}
#endif

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

static inline int sc64_front_mission_ordinal(const Sc64Frontend *f) {
    return f ? sc64_catalog_playable_ordinal(sc64_front_catalog_faction(f->faction),
                                              f->mission) : -1;
}

static inline void sc64_frontend_reset(Sc64Frontend *f, int mission,
                                        int race, int bots, int completed) {
    if (!f) return;
    f->ticks = 0;
    f->prev_buttons = 0;
    f->page = SC_FRONT_HOME;
    f->row = 0;
#ifdef SC64_UNIFIED
#ifdef SC64_T4_REPLAY_ENTRY
    f->mission = mission == SC64_T4_GLOBAL_ID7 ? mission : sc64_front_clamp(mission, 0, 5);
#else
#if defined(SC64_T3_ENABLED) || defined(SC64_Z3_ENABLED) || \
    defined(SC64_T6_CAMPAIGN_ENTRY) || defined(SC64_TERRAN_FULL_CAMPAIGN_ENTRY) || \
    (defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED))
    f->mission = mission;
#else
    f->mission = sc64_front_clamp(mission, 0, 5);
#endif
#endif
#else
    f->mission = sc64_front_clamp(mission, 0, 3);
#endif
#ifdef SC64_CAMPAIGN_RACE
    f->faction = sc64_front_campaign_faction_at(0);
#ifdef SC64_T4_REPLAY_ENTRY
    if (f->mission != SC64_T4_GLOBAL_ID7 &&
        !sc64_catalog_mission_allowed(f->mission, sc64_front_catalog_faction(f->faction)))
        f->mission = sc64_catalog_playable_at(sc64_front_catalog_faction(f->faction), 0);
#else
    if (!sc64_catalog_mission_allowed(f->mission, sc64_front_catalog_faction(f->faction)))
        f->mission = sc64_catalog_playable_at(sc64_front_catalog_faction(f->faction), 0);
#endif
#else
    f->faction = sc64_frontend_mission_faction(f->mission);
#ifdef SC64_T4_REPLAY_ENTRY
    if (f->mission != SC64_T4_GLOBAL_ID7 &&
        !sc64_catalog_mission_allowed(f->mission, sc64_front_catalog_faction(f->faction)))
        f->mission = sc64_catalog_playable_at(sc64_front_catalog_faction(f->faction), 0);
#else
    if (!sc64_catalog_mission_allowed(f->mission, sc64_front_catalog_faction(f->faction)))
        f->mission = sc64_catalog_playable_at(sc64_front_catalog_faction(f->faction), 0);
#endif
#endif
    f->race = sc64_front_clamp(race, SC_FRONT_RACE_TERRAN,
                               SC_FRONT_RACE_PROTOSS);
    f->bots = sc64_front_clamp(bots, 1, 3);
#ifdef SC64_UNIFIED
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    f->completed = sc64_front_clamp(completed, -1, 22);
#elif defined(SC64_Z3_ENABLED)
    f->completed = sc64_front_clamp(completed, -1, 14);
#elif defined(SC64_TERRAN_FULL_CAMPAIGN_ENTRY)
    f->completed = sc64_front_clamp(completed, -1, 13);
#elif defined(SC64_T6_CAMPAIGN_ENTRY)
    f->completed = sc64_front_clamp(completed, -1, 9);
#elif defined(SC64_T5_CAMPAIGN_ENTRY)
    f->completed = sc64_front_clamp(completed, -1, 8);
#elif defined(SC64_T4_CAMPAIGN_ENTRY)
    f->completed = sc64_front_clamp(completed, -1, 7);
#elif defined(SC64_T3_ENABLED)
    f->completed = sc64_front_clamp(completed, -1, 6);
#else
    f->completed = sc64_front_clamp(completed, -1, 5);
#endif
#else
    f->completed = sc64_front_clamp(completed, -1, 3);
#endif
    f->map = SC64_MAP_OUTPOST;
    f->briefing_page = 0;
    f->briefing_pages = 1;
    f->briefing_page_tick = 0;
    f->briefing_reveal_length = 0;
    f->briefing_revealed = 0;
    for (int i = 0; i < 3; ++i) {
        int ai = (f->race + i + 1) % 3;
        if (ai == f->race) ai = (ai + 1) % 3;
        f->bot_races[i] = ai;
    }
    int mission_ordinal = sc64_front_mission_ordinal(f);
    f->mission_page = mission_ordinal < 0 ? 0 : mission_ordinal / 2;
    f->row = 0;
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

#ifdef SC64_T4_REPLAY_ENTRY
    /* Internal scenario replay route: both controls must be held together.
     * This bypasses the playable list without changing catalog metadata. */
    if (f->page == SC_FRONT_HOME && (held & (KEY_START | KEY_SELECT)) ==
        (KEY_START | KEY_SELECT) && (edge & (KEY_START | KEY_SELECT))) {
        f->faction = SC_FRONT_TERRAN;
        f->mission = SC64_T4_GLOBAL_ID7;
        f->mission_page = 0;
        f->row = 0;
        f->page = SC_FRONT_BRIEFING;
        sc64_frontend_briefing_reset_page(f);
        return SC_FRONT_NONE;
    }
#endif

#ifdef SC64_SHOWCASE_ENABLED
    /* Hidden filming menu. Requiring the shoulder buttons to remain held
     * while SELECT is newly pressed keeps ordinary HOME controls untouched. */
    if (f->page == SC_FRONT_HOME &&
        (held & (KEY_L | KEY_R)) == (KEY_L | KEY_R) && (edge & KEY_SELECT)) {
        f->page = SC_FRONT_SHOWCASE;
        f->row = 0;
        return SC_FRONT_NONE;
    }
#endif

#ifdef SC64_SHOWCASE_ENABLED
    if (edge & KEY_L) {
        if (!(f->page == SC_FRONT_SHOWCASE ||
              (f->page == SC_FRONT_HOME && (held & KEY_R))))
            return SC_FRONT_LOAD;
    }
#else
    if (edge & KEY_L) return SC_FRONT_LOAD;
#endif

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
        case SC_FRONT_BRIEFING:
#ifdef SC64_T4_REPLAY_ENTRY
            if (f->mission == SC64_T4_GLOBAL_ID7) {
                f->page = SC_FRONT_HOME;
                f->row = 0;
                break;
            }
#endif
            f->page = SC_FRONT_MISSION;
            sc64_frontend_briefing_reset_page(f);
            {
                int ordinal = sc64_front_mission_ordinal(f);
                f->mission_page = ordinal < 0 ? 0 : ordinal / 2;
                f->row = ordinal < 0 ? 0 : ordinal % 2;
            }
            break;
        case SC_FRONT_SKIRMISH: f->page = SC_FRONT_HOME; f->row = 1; break;
#ifdef SC64_SHOWCASE_ENABLED
        case SC_FRONT_SHOWCASE: f->page = SC_FRONT_HOME; f->row = 0; break;
#endif
        default: f->page = SC_FRONT_HOME; f->row = 0; break;
        }
        return SC_FRONT_NONE;
    }

    if (f->page == SC_FRONT_BRIEFING && (edge & (KEY_LEFT | KEY_RIGHT))) {
        if (edge & KEY_RIGHT) {
            if (f->briefing_page + 1 < f->briefing_pages) {
                ++f->briefing_page;
                f->briefing_page_tick = f->ticks;
                f->briefing_revealed = 0;
            }
        } else if (f->briefing_page > 0) {
            --f->briefing_page;
            f->briefing_page_tick = f->ticks;
            f->briefing_revealed = 0;
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
#ifdef SC64_SHOWCASE_ENABLED
        case SC_FRONT_SHOWCASE: count = SC64_FRONT_SHOWCASE_COUNT; break;
#endif
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
            {
                int ordinal = sc64_front_mission_ordinal(f);
                f->mission_page = ordinal < 0 ? 0 : ordinal / 2;
                f->row = ordinal < 0 ? 0 : ordinal % 2;
            }
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
        {
            int ordinal = sc64_catalog_playable_ordinal(sc64_front_catalog_faction(f->faction),
                                                         f->mission);
            f->row = ordinal >= 0 ? ordinal % 2 : 0;
            f->mission_page = ordinal >= 0 ? ordinal / 2 : 0;
        }
#else
        f->row = f->mission / 2 == f->faction ? f->mission % 2 : 0;
#endif
        break;
    case SC_FRONT_MISSION:
        f->mission = sc64_catalog_playable_at(sc64_front_catalog_faction(f->faction),
                                               f->mission_page * 2 + f->row);
        if (f->mission < 0) return SC_FRONT_NONE;
        f->page = SC_FRONT_BRIEFING;
        sc64_frontend_briefing_reset_page(f);
        break;
    case SC_FRONT_BRIEFING:
        if (edge & KEY_START) return SC_FRONT_START_CAMPAIGN;
        if (!sc64_frontend_briefing_is_revealed(f)) {
            f->briefing_revealed = 1;
            return SC_FRONT_NONE;
        }
        if (f->briefing_page + 1 < f->briefing_pages) {
            ++f->briefing_page;
            f->briefing_page_tick = f->ticks;
            f->briefing_revealed = 0;
            break;
        }
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
#ifdef SC64_SHOWCASE_ENABLED
    case SC_FRONT_SHOWCASE:
        return SC_FRONT_START_SHOWCASE;
#endif
    default:
        f->page = SC_FRONT_HOME;
        f->row = 0;
        break;
    }
    return SC_FRONT_NONE;
}

#endif
