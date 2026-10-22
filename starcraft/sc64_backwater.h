#ifndef SC64_BACKWATER_H
#define SC64_BACKWATER_H

#include "sc64_types.h"

/* CHK Wait fields are milliseconds. Mission update runs at 30 simulation
 * ticks per second, so round up to preserve each requested delay. */
#define SC64_BW_WAIT_TICKS(ms) (((ms) * 30u + 999u) / 1000u)

/* Persistent state for the authored Backwater CHK trigger subset. Keep this
 * small: it is copied into the existing mission save record. Rules follow
 * scenario 008/00B TRIG records 0-14 parsed by
 * references/sc64-maps/extract_sc64_maps.py @ 09b57f787a247107d1de82b8c5eab2734667ea1d. */
typedef struct {
    u16 executed;       /* One-shot trigger records, bits 0..14. */
    u16 switches;       /* CHK switches 2 and 3 used by TRIG 7..13. */
    u16 timer[15];      /* Wait actions for each one-shot trigger. */
    u8 phase[15];       /* Sequence phase for multi-action triggers. */
    u8 outcome;         /* 0 running, 1 victory, 2 defeat. */
    u8 objective;       /* 0 active, 1 completed. */
    u8 spawn_trigger;   /* Partially completed CreateUnit action, 0 if idle. */
    u8 spawn_remaining;
    u8 reserved[4];
} Sc64BackwaterState;

enum {
    SC64_BACKWATER_RUNNING = 0,
    SC64_BACKWATER_WON = 1,
    SC64_BACKWATER_LOST = 2
};

enum {
    SC64_BACKWATER_EVENT_INTRO = 1,
    SC64_BACKWATER_EVENT_RESCUE_SCV = 2,
    SC64_BACKWATER_EVENT_RAYNOR_KILLED = 3,
    SC64_BACKWATER_EVENT_FACTORY_DESTROYED = 4,
    SC64_BACKWATER_EVENT_ENDING = 5,
    SC64_BACKWATER_EVENT_CREEP_NORTH = 6,
    SC64_BACKWATER_EVENT_CREEP_SOUTH = 7,
    SC64_BACKWATER_EVENT_BURN_FACTORY = 8
};

/* Engine-owned callbacks keep unit arrays and coordinate conversion out of
 * this saveable mission script. Owners are raw CHK owner/group selectors;
 * the adapter resolves them to runtime sides and inclusive world rectangles. */
typedef struct {
    void *context;
    int (*count)(void *context, int player, int type, int location);
    int (*spawn)(void *context, int player, int type, int location, int count);
    void (*kill)(void *context, int player, int type, int location);
    void (*give)(void *context, int from, int to, int type, int location);
    void (*resources)(void *context, int ore, int gas);
    void (*invincible)(void *context, int player, int type, int location, int on);
    void (*center)(void *context, int location);
    void (*result)(void *context, int victory);
    void (*message)(void *context, int trigger, int action);
} Sc64BackwaterWorld;

/* Call once on a new run, then once per simulation frame. */
static inline void sc64_backwater_init(Sc64BackwaterState *state,
                                       const Sc64BackwaterWorld *world);
static inline void sc64_backwater_update(Sc64BackwaterState *state,
                                         const Sc64BackwaterWorld *world,
                                         u32 frame);
static inline u8 sc64_backwater_outcome(const Sc64BackwaterState *state);
static inline u8 sc64_backwater_objective(const Sc64BackwaterState *state);

/* CHK trigger selectors and raw unit ids are intentionally retained here;
 * the game adapter resolves them to runtime owners, units, and locations.
 * Standard unit ids are checked against references/bwapi/Documentation/dox/
 * unittypes.dox @ d727fed68558c506163048ea889131d8cbb33915. */
enum {
    SC64_BW_SIDE = 1, SC64_BW_RAYNOR = 2, SC64_BW_OUTPOST = 3,
    SC64_BW_ZERG = 4, SC64_BW_CURRENT_PLAYER = 13,
    SC64_BW_ALL_PLAYERS = 17, SC64_BW_FORCE_1 = 18,
    SC64_BW_FORCE_2 = 19,
    SC64_BW_ANY_UNIT = 228, SC64_BW_MEN = 229,
    SC64_BW_BUILDINGS = 230, SC64_BW_FACTORIES = 231,
    SC64_BW_RAYNOR_VULTURE = 19,
    SC64_BW_SCV = 7, SC64_BW_MARINE = 0, SC64_BW_FIREBAT = 32,
    SC64_BW_BUNKER = 125, SC64_BW_INFESTED_CC = 130,
    SC64_BW_LOC_ABANDONED_SETTLEMENT = 2,
    SC64_BW_LOC_INFESTED_BUILDING = 3,
    SC64_BW_LOC_NEAR_INFESTED_BUILDING = 4,
    SC64_BW_LOC_FIREBAT_BUNKER_2 = 6,
    SC64_BW_LOC_SCV_BUNKER = 7,
    SC64_BW_LOC_CREEP_2 = 8,
    SC64_BW_LOC_FIREBAT_BUNKER = 9,
    SC64_BW_LOC_RAYNOR = 10,
    SC64_BW_LOC_CREEP_1 = 1,
    SC64_BW_LOC_ANYWHERE = 64
};

static inline int sc64_bw_count(const Sc64BackwaterWorld *w,
                                int player, int type, int location)
{
    return w->count ? w->count(w->context, player, type, location) : 0;
}

static inline void sc64_bw_message(const Sc64BackwaterWorld *w,
                                   int trigger, int action)
{
    if (w->message) w->message(w->context, trigger, action);
}

static inline int sc64_bw_spawn(const Sc64BackwaterWorld *w, int trigger,
                                int count)
{
    int owner = SC64_BW_OUTPOST, type = SC64_BW_FIREBAT, location;
    if (trigger == 3) location = SC64_BW_LOC_FIREBAT_BUNKER_2;
    else if (trigger == 4) { location = SC64_BW_LOC_SCV_BUNKER; type = SC64_BW_SCV; }
    else if (trigger == 5) location = SC64_BW_LOC_FIREBAT_BUNKER;
    else { owner = SC64_BW_SIDE; type = 101; location = SC64_BW_LOC_INFESTED_BUILDING; }
    return w->spawn ? w->spawn(w->context, owner, type, location, count) : 0;
}

static inline void sc64_backwater_init(Sc64BackwaterState *state,
                                       const Sc64BackwaterWorld *world)
{
    u32 i;
    if (!state) return;
    for (i = 0; i < (u32)sizeof(*state); i++) ((u8 *)state)[i] = 0;
    state->executed = 1u; /* TRIG 0: objective and one-time resources. */
    if (world && world->resources) world->resources(world->context, 200, 200);
    state->objective = 0;
}

static inline void sc64_backwater_update(Sc64BackwaterState *s,
                                         const Sc64BackwaterWorld *w,
                                         u32 frame)
{
    int n;
    u32 i;
    (void)frame;
    if (!s || !w || s->outcome) return;
    for (i = 0; i < 15; i++) if (s->timer[i]) s->timer[i]--;

    /* Finish any partially placed reinforcement before accepting another. */
    if (s->spawn_trigger) {
        int made = sc64_bw_spawn(w, s->spawn_trigger, s->spawn_remaining);
        if (made > s->spawn_remaining) made = s->spawn_remaining;
        if (made > 0) s->spawn_remaining = (u8)(s->spawn_remaining - made);
        if (!s->spawn_remaining) s->spawn_trigger = 0;
    }

    /* Intro line once Raynor has entered the playable area. */
    if (!(s->executed & (1u << 1)) &&
        sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_RAYNOR_VULTURE,
                      SC64_BW_LOC_ANYWHERE) > 0) {
        s->executed |= (u16)(1u << 1);
        sc64_bw_message(w, 1, 0);
    }
    if (!(s->executed & (1u << 2)) &&
        sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_RAYNOR_VULTURE,
                      SC64_BW_LOC_ABANDONED_SETTLEMENT) > 0 &&
        sc64_bw_count(w, SC64_BW_OUTPOST, SC64_BW_FACTORIES,
                      SC64_BW_LOC_ABANDONED_SETTLEMENT) > 0) {
        s->executed |= (u16)(1u << 2);
        sc64_bw_message(w, 2, 0);
    }

    /* Rescue missions: TRIG 3/4/5. Keep partial spawn retries saveable. */
    if (!s->spawn_trigger && !(s->executed & (1u << 3)) &&
        sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_BUNKER,
                      SC64_BW_LOC_FIREBAT_BUNKER_2) > 0) {
        s->executed |= (u16)(1u << 3);
        n = sc64_bw_spawn(w, 3, 2);
        if (n < 2) { s->spawn_trigger = 3; s->spawn_remaining = (u8)(2 - n); }
    }
    if (!s->spawn_trigger && !(s->executed & (1u << 4)) &&
        sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_BUNKER,
                      SC64_BW_LOC_SCV_BUNKER) > 0) {
        s->executed |= (u16)(1u << 4);
        if (w->center) w->center(w->context, SC64_BW_LOC_SCV_BUNKER);
        s->phase[4] = 1; s->timer[4] = SC64_BW_WAIT_TICKS(200);
    }
    if (!s->spawn_trigger && !(s->executed & (1u << 5)) &&
        sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_BUNKER,
                      SC64_BW_LOC_FIREBAT_BUNKER) > 0) {
        s->executed |= (u16)(1u << 5);
        n = sc64_bw_spawn(w, 5, 3);
        if (n < 3) { s->spawn_trigger = 5; s->spawn_remaining = (u8)(3 - n); }
        s->timer[5] = SC64_BW_WAIT_TICKS(1500);
    }
    if (s->phase[4] == 1 && !s->timer[4] && !s->spawn_trigger) {
        n = sc64_bw_spawn(w, 4, 2);
        if (n < 2) { s->spawn_trigger = 4; s->spawn_remaining = (u8)(2 - n); }
        s->phase[4] = 2;
    }
    if (s->phase[4] == 2 && !s->spawn_trigger) {
        sc64_bw_message(w, 4, 5);
        s->phase[4] = 3; s->timer[4] = SC64_BW_WAIT_TICKS(2000);
    }
    if (s->phase[4] == 3 && !s->timer[4]) s->phase[4] = 0;
    if (s->timer[5] == 0 && (s->executed & (1u << 5))) s->phase[5] = 0;

    /* Raynor's survival is a separate loss condition (TRIG 6). */
    if (s->phase[6] == 0 &&
        sc64_bw_count(w, SC64_BW_ALL_PLAYERS, SC64_BW_RAYNOR_VULTURE,
                      SC64_BW_LOC_ANYWHERE) == 0) {
        s->phase[6] = 1; s->timer[6] = SC64_BW_WAIT_TICKS(1000);
    } else if (s->phase[6] == 1 && !s->timer[6]) {
        sc64_bw_message(w, 6, 2);
        s->phase[6] = 2; s->timer[6] = SC64_BW_WAIT_TICKS(500);
    } else if (s->phase[6] == 2 && !s->timer[6]) {
        s->outcome = SC64_BACKWATER_LOST;
        if (w->result) w->result(w->context, 0);
        return;
    }

    /* Creep scouting switches (TRIG 7/8) and factory discovery (TRIG 9). */
    if (!(s->switches & 1u) &&
        sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_RAYNOR_VULTURE,
                      SC64_BW_LOC_CREEP_2) > 0 &&
        sc64_bw_count(w, SC64_BW_FORCE_2, SC64_BW_FACTORIES,
                      SC64_BW_LOC_CREEP_2) > 0) {
        s->switches |= 1u; sc64_bw_message(w, 7, 1);
    }
    if (!(s->switches & 1u) &&
        sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_RAYNOR_VULTURE,
                      SC64_BW_LOC_CREEP_1) > 0 &&
        sc64_bw_count(w, SC64_BW_FORCE_2, SC64_BW_INFESTED_CC,
                      SC64_BW_LOC_CREEP_1) > 0) {
        s->switches |= 1u; sc64_bw_message(w, 8, 1);
    }
    if (!(s->switches & (1u << 2)) &&
        sc64_bw_count(w, SC64_BW_SIDE, 229, SC64_BW_LOC_INFESTED_BUILDING) > 0 &&
        sc64_bw_count(w, SC64_BW_FORCE_2, SC64_BW_INFESTED_CC,
                      SC64_BW_LOC_INFESTED_BUILDING) > 0)
        s->switches |= (1u << 2);

    /* Any human unit reaching the infested building triggers the scripted
     * factory reveal; the separate Firebat condition shares the same switch. */
    if ((s->switches & (1u << 2)) && !(s->switches & (1u << 3)) &&
        (sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_MARINE,
                       SC64_BW_LOC_INFESTED_BUILDING) > 0 ||
         sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_FIREBAT,
                       SC64_BW_LOC_INFESTED_BUILDING) > 0)) {
        int reveal_trigger = sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_MARINE,
                                            SC64_BW_LOC_INFESTED_BUILDING) > 0 ? 10 : 11;
        s->switches &= (u16)~(1u << 2);
        s->switches |= (1u << 3);
        if (w->spawn) w->spawn(w->context, SC64_BW_SIDE, 101,
                               SC64_BW_LOC_INFESTED_BUILDING, 1);
        sc64_bw_message(w, reveal_trigger, 2);
        if (w->kill) w->kill(w->context, SC64_BW_SIDE, 101,
                             SC64_BW_LOC_INFESTED_BUILDING);
    }
    if ((s->switches & (1u << 3)) &&
        sc64_bw_count(w, SC64_BW_SIDE, SC64_BW_RAYNOR_VULTURE,
                      SC64_BW_LOC_NEAR_INFESTED_BUILDING) > 0)
        sc64_bw_message(w, 12, 0);
    if (s->switches & (1u << 3)) s->switches &= (u16)~(1u << 3);

    /* The CHK's actual victory gate is the enemy Force 2 owning zero
     * Infested Command Centers. It then protects Raynor and runs the ending. */
    if (s->phase[14] == 0 &&
        sc64_bw_count(w, SC64_BW_FORCE_2, SC64_BW_INFESTED_CC,
                      SC64_BW_LOC_ANYWHERE) == 0) {
        s->phase[14] = 1; s->timer[14] = SC64_BW_WAIT_TICKS(4000);
        if (w->invincible) w->invincible(w->context, SC64_BW_CURRENT_PLAYER,
                                         SC64_BW_RAYNOR_VULTURE,
                                         SC64_BW_LOC_ANYWHERE, 1);
    } else if (s->phase[14] == 1 && !s->timer[14]) {
        if (w->center) w->center(w->context, SC64_BW_LOC_RAYNOR);
        s->phase[14] = 2; s->timer[14] = SC64_BW_WAIT_TICKS(1000);
    } else if (s->phase[14] == 2 && !s->timer[14]) {
        sc64_bw_message(w, 14, 5);
        sc64_bw_message(w, 14, 6);
        sc64_bw_message(w, 14, 7);
        sc64_bw_message(w, 14, 8);
        sc64_bw_message(w, 14, 9);
        s->phase[14] = 3; s->timer[14] = SC64_BW_WAIT_TICKS(500);
    } else if (s->phase[14] == 3 && !s->timer[14]) {
        s->outcome = SC64_BACKWATER_WON;
        s->objective = 1;
        if (w->result) w->result(w->context, 1);
    }
}

static inline u8 sc64_backwater_outcome(const Sc64BackwaterState *state)
{
    return state ? state->outcome : SC64_BACKWATER_RUNNING;
}

static inline u8 sc64_backwater_objective(const Sc64BackwaterState *state)
{
    return state ? state->objective : 0;
}

#endif
