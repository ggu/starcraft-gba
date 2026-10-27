/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Isolated trigger scheduler for the authenticated Z8 scenario.
 *
 * This header is not included by the game or linked into any ROM. It
 * consumes rows shaped like the private `tools/sc64_z8_pack.py` descriptor
 * (field names match its JSON keys) and keeps every mutable trigger value in
 * one fixed-size, save-restorable state. World queries and effects go through
 * a small callback table; anything the scheduler or the caller cannot perform
 * faults explicitly instead of reporting success.
 *
 * Timing and ordering follow the native USA N64 scheduler traced at the
 * addresses cited below; see docs/Z8-TRIGGER-SCHEDULER.md for the evidence
 * and the questions that remain open. One tick is one native game frame.
 * Nothing here claims that Z8 is playable or performance-qualified.
 */
#ifndef SC64_Z8_TRIGGERS_H
#define SC64_Z8_TRIGGERS_H

#include "../common/gba.h"

enum {
    SC64_Z8_OWNER_LIMIT = 8,
    SC64_Z8_TRIGGER_LIMIT = 64,
    SC64_Z8_INSTANCE_LIMIT = 64,
    SC64_Z8_LOCATION_LIMIT = 64,
    SC64_Z8_ROW_LIMIT = 1024,
    SC64_Z8_CONDITION_SLOTS = 16,
    SC64_Z8_ACTION_SLOTS = 64,
    SC64_Z8_SWITCH_BYTES = 32,
    SC64_Z8_STATE_VERSION = 0x5a380001,

    /* 0x8008DB54 reloads the pass counter with 30 after it underflows, and
     * mission init (0x8008E07C -> 0x8008CE00) leaves it at 1, so passes run
     * on frames 2, 33, 64, ... */
    SC64_Z8_PASS_RELOAD = 30,
    SC64_Z8_PASS_INITIAL = 1,
    /* 0x8008D570/0x8008E070: elapsed seconds advance every 16 frames. */
    SC64_Z8_SECOND_RELOAD = 15,

    /* Selectors in the 27-entry enabled-player table (0x8008E094). */
    SC64_Z8_SELECT_ALL_PLAYERS = 17,
    SC64_Z8_SELECT_FORCE1 = 18,
    SC64_Z8_SELECT_FORCE4 = 21,

    /* Instance flags. 0x01/0x04/0x08 match the native instance word; ARMED
     * stands in for the native per-action bit 0x01 on the cursor action. */
    SC64_Z8_INST_EXECUTING = 0x01,
    SC64_Z8_INST_ARMED = 0x02,
    SC64_Z8_INST_PRESERVE = 0x04,
    SC64_Z8_INST_DONE = 0x08,

    SC64_Z8_OUTCOME_NONE = 0,
    SC64_Z8_OUTCOME_DEFEAT = 2,  /* 0x80080690 writes 2 */
    SC64_Z8_OUTCOME_VICTORY = 3, /* 0x8008063C writes 3 */

    SC64_Z8_FAULT_NONE = 0,
    SC64_Z8_FAULT_CONDITION = 1, /* unsupported or failed world query */
    SC64_Z8_FAULT_ACTION = 2,    /* unsupported or failed world effect */
    SC64_Z8_FAULT_STATE = 3,     /* corrupted state reached the runner */

    /* Phases of the timed presentation callback (Transmission, CenterView). */
    SC64_Z8_TIMED_ELIGIBLE = 0,
    SC64_Z8_TIMED_START = 1,
    SC64_Z8_TIMED_FINISH = 2
};

/* Indefinite timers (-1) are never decremented by 0x8008D4B8; only an
 * external completion such as a finished camera pan releases them. */
#define SC64_Z8_WAIT_INDEFINITE 0xffffffffu

/* The three distinct Z8 AI-script FourCCs, as little-endian CHK group2. */
#define SC64_Z8_AI_ZER8 0x3872655au /* "Zer8" */
#define SC64_Z8_AI_ZE8T 0x5438655au /* "Ze8T" */
#define SC64_Z8_AI_MVTE 0x6554764du /* "MvTe" */

/* Descriptor rows. `index` is the source slot (0..15 or 0..63); slots
 * absent from the compact descriptor are all-zero source slots. */
typedef struct {
    u32 location, player, amount;
    u16 unit, mask;
    u8 comparison, opcode, resource, flags, index;
} Sc64Z8Condition;

typedef struct {
    u32 location, string_id, wav_id, time, group1, group2;
    u16 unit;
    u8 opcode, modifier, flags, index;
} Sc64Z8Action;

typedef struct {
    u16 first_condition, condition_count, first_action, action_count;
    u32 execution_flags;
    u8 initial_cursor;
    u8 enabled_players[27];
} Sc64Z8Trigger;

typedef struct {
    u16 left, top, right, bottom, flags;
} Sc64Z8SourceLocation;

typedef struct {
    const Sc64Z8Trigger *triggers;
    const Sc64Z8Condition *conditions;
    const Sc64Z8Action *actions;
    const Sc64Z8SourceLocation *locations;
    u16 trigger_count, condition_count, action_count, location_count;
    u16 map_pixel_width, map_pixel_height;
} Sc64Z8Program;

/* Mission-start inputs. Instances exist only for owners active at start,
 * as 0x8008E094 walks the active-player iterator once per trigger. */
typedef struct {
    u8 active_mask;                    /* bit p: owner p takes part */
    u8 force[SC64_Z8_OWNER_LIMIT];     /* 1..4, or 0 for no force */
    u8 frame_ms;                       /* native speed-table entry */
} Sc64Z8Setup;

/* Mutable location rectangle in source pixels (native signed halfwords). */
typedef struct {
    s16 left, top, right, bottom;
} Sc64Z8Rect;

typedef struct {
    u8 trigger, owner, flags, cursor;
} Sc64Z8Instance;

typedef struct {
    u32 version;
    u32 frames;            /* saturating diagnostic counter */
    u32 passes;            /* saturating diagnostic counter */
    u32 elapsed_seconds;   /* ElapsedTime operand (0x800B9368) */
    u32 wait_ms[SC64_Z8_OWNER_LIMIT];
    u8 waiting_mask, active_mask, pass_counter, second_counter;
    u8 frame_ms, instance_count, trigger_count, location_count;
    u8 fault, fault_owner, fault_trigger, fault_slot;
    u8 force[SC64_Z8_OWNER_LIMIT];
    u8 outcome[SC64_Z8_OWNER_LIMIT];
    u8 switches[SC64_Z8_SWITCH_BYTES];
    Sc64Z8Instance instances[SC64_Z8_INSTANCE_LIMIT];
    Sc64Z8Rect locations[SC64_Z8_LOCATION_LIMIT];
} Sc64Z8SchedulerState;

_Static_assert(sizeof(Sc64Z8Condition) == 24, "Z8 condition row size");
_Static_assert(sizeof(Sc64Z8Action) == 32, "Z8 action row size");
_Static_assert(sizeof(Sc64Z8Instance) == 4, "Z8 instance size");
_Static_assert(sizeof(Sc64Z8SchedulerState) == 876, "Z8 scheduler state budget");

/* Callback contract. Each returns exactly the values listed; any other
 * value, or a missing callback, faults the scheduler. `rect` is the current
 * mutable location named by the row, or NULL when the native handler does
 * not read one. */
typedef struct {
    /* Command (2), Bring (3), Kill (5): 1 with *count, or -1. */
    int (*count)(void *context, const Sc64Z8Condition *condition,
                 unsigned owner, const Sc64Z8Rect *rect, u32 *count);
    /* Immediate effects (8, 9, 11, 12, 16, 26, 28, 43): 1 once applied, or
     * -1. Native handlers never stall these, so there is no retry result. */
    int (*action)(void *context, const Sc64Z8Action *action,
                  unsigned owner, const Sc64Z8Rect *rect);
    /* Transmission (7), CenterView (10). ELIGIBLE: 1, 0 (complete with no
     * effect) or -1. START: 1 with *duration_ms, which may be
     * SC64_Z8_WAIT_INDEFINITE, or -1. FINISH: 1 or -1. */
    int (*timed)(void *context, int phase, const Sc64Z8Action *action,
                 unsigned owner, const Sc64Z8Rect *rect, u32 *duration_ms);
    /* MoveLocation (38) anchor: the first actor matching unit/group1 in the
     * source location. 1 with source-pixel x and y, 0 for none, or -1. */
    int (*find_unit)(void *context, const Sc64Z8Action *action,
                     unsigned owner, const Sc64Z8Rect *rect, s16 *x, s16 *y);
} Sc64Z8WorldOps;

/* Native game-speed table at 0x80001424: milliseconds per frame. */
static inline int
sc64_z8_frame_ms_valid(unsigned ms)
{
    return ms == 167 || ms == 111 || ms == 83 || ms == 67 || ms == 56 ||
           ms == 48 || ms == 42;
}

static inline int
sc64_z8_ai_script_known(u32 fourcc)
{
    return fourcc == SC64_Z8_AI_ZER8 || fourcc == SC64_Z8_AI_ZE8T ||
           fourcc == SC64_Z8_AI_MVTE;
}

/* CreateUnitwithProperties count: 0x800819EC turns zero into one. */
static inline unsigned
sc64_z8_create_count(const Sc64Z8Action *a)
{
    return a->modifier ? a->modifier : 1u;
}

/* Command and Bring count completed actors unless the comparison is AtMost
 * (0x80086438/0x80086484 pass comparison != 1 to the census). */
static inline int
sc64_z8_condition_counts_completed(const Sc64Z8Condition *c)
{
    return c->comparison != 1;
}

static inline int
sc64_z8_selector_matches(unsigned selector, unsigned owner, unsigned force)
{
    if (selector < SC64_Z8_OWNER_LIMIT) return selector == owner;
    if (selector == SC64_Z8_SELECT_ALL_PLAYERS) return 1;
    if (selector >= SC64_Z8_SELECT_FORCE1 && selector <= SC64_Z8_SELECT_FORCE4)
        return force && selector - SC64_Z8_SELECT_FORCE1 + 1 == force;
    return 0;
}

/* Count how many instances one trigger would give one owner. */
static inline unsigned
sc64_z8_trigger_matches(const Sc64Z8Trigger *t, unsigned owner, unsigned force)
{
    unsigned n = 0;
    for (unsigned sel = 0; sel < 27; ++sel)
        if (t->enabled_players[sel] && sc64_z8_selector_matches(sel, owner, force))
            ++n;
    return n;
}

static inline int
sc64_z8_comparison_known(unsigned opcode, unsigned comparison)
{
    /* 0x800862F0 knows AtLeast 0, AtMost 1 and Exactly 10; 0x800863C4 knows
     * Set 2 and Cleared 3. Natively anything else is silently false. */
    if (opcode == 11) return comparison == 2 || comparison == 3;
    if (opcode == 22 || opcode == 23) return 1;
    return comparison == 0 || comparison == 1 || comparison == 10;
}

static inline int
sc64_z8_condition_supported(const Sc64Z8Condition *c, unsigned location_count)
{
    if (c->flags & ~0x12u) return 0;
    if (c->flags & 2) return c->opcode && c->opcode < 24; /* never dispatched */
    switch (c->opcode) {
    case 2: case 3: case 5: case 11: case 12: case 22: case 23: break;
    default: return 0;
    }
    if (c->location > location_count) return 0;
    return sc64_z8_comparison_known(c->opcode, c->comparison);
}

static inline int
sc64_z8_action_supported(const Sc64Z8Action *a, unsigned location_count)
{
    if (a->flags & ~0x16u) return 0;
    if (a->flags & 2) return a->opcode && a->opcode < 59; /* never dispatched */
    if (a->location > location_count) return 0;
    switch (a->opcode) {
    case 1: case 2: case 3: case 4: case 7: case 8: case 9: case 10:
    case 11: case 12: case 26: case 28: case 43:
        return 1;
    case 13: /* 4 set, 5 clear, 6 toggle; 11 needs the native RNG stream. */
        return a->group2 < 8u * SC64_Z8_SWITCH_BYTES &&
               a->modifier >= 4 && a->modifier <= 6;
    case 16:
        return sc64_z8_ai_script_known(a->group2);
    case 38:
        return a->group2 <= location_count;
    default:
        return 0;
    }
}

static inline int
sc64_z8_program_valid(const Sc64Z8Program *p)
{
    if (!p || !p->triggers || !p->trigger_count ||
        p->trigger_count > SC64_Z8_TRIGGER_LIMIT ||
        p->condition_count > SC64_Z8_ROW_LIMIT ||
        p->action_count > SC64_Z8_ROW_LIMIT ||
        p->location_count > SC64_Z8_LOCATION_LIMIT ||
        (p->condition_count && !p->conditions) ||
        (p->action_count && !p->actions) ||
        (p->location_count && !p->locations) ||
        !p->map_pixel_width || !p->map_pixel_height ||
        p->map_pixel_width > 0x7fff || p->map_pixel_height > 0x7fff) return 0;
    unsigned next_condition = 0, next_action = 0;
    for (unsigned i = 0; i < p->trigger_count; ++i) {
        const Sc64Z8Trigger *t = &p->triggers[i];
        /* Instantiation copies these into every instance; resuming a source
         * program that starts mid-execution is not traced. */
        if (t->execution_flags || t->initial_cursor) return 0;
        if (t->first_condition != next_condition ||
            t->first_action != next_action ||
            t->condition_count > SC64_Z8_CONDITION_SLOTS ||
            t->action_count > SC64_Z8_ACTION_SLOTS) return 0;
        next_condition += t->condition_count;
        next_action += t->action_count;
        if (next_condition > p->condition_count ||
            next_action > p->action_count) return 0;
        for (unsigned sel = 0; sel < 27; ++sel) {
            if (t->enabled_players[sel] > 1) return 0;
            if (t->enabled_players[sel] && !(sel < SC64_Z8_OWNER_LIMIT ||
                sel == SC64_Z8_SELECT_ALL_PLAYERS ||
                (sel >= SC64_Z8_SELECT_FORCE1 && sel <= SC64_Z8_SELECT_FORCE4)))
                return 0;
        }
        int last = -1;
        for (unsigned r = 0; r < t->condition_count; ++r) {
            const Sc64Z8Condition *c = &p->conditions[t->first_condition + r];
            if ((int)c->index <= last || c->index >= SC64_Z8_CONDITION_SLOTS ||
                !sc64_z8_condition_supported(c, p->location_count)) return 0;
            last = c->index;
        }
        last = -1;
        for (unsigned r = 0; r < t->action_count; ++r) {
            const Sc64Z8Action *a = &p->actions[t->first_action + r];
            if ((int)a->index <= last || a->index >= SC64_Z8_ACTION_SLOTS ||
                !sc64_z8_action_supported(a, p->location_count)) return 0;
            last = a->index;
        }
    }
    if (next_condition != p->condition_count || next_action != p->action_count)
        return 0;
    for (unsigned i = 0; i < p->location_count; ++i) {
        const Sc64Z8SourceLocation *l = &p->locations[i];
        if (l->left > l->right || l->top > l->bottom ||
            l->right > p->map_pixel_width || l->bottom > p->map_pixel_height)
            return 0;
    }
    return 1;
}

static inline int
sc64_z8_setup_valid(const Sc64Z8Setup *setup, const Sc64Z8Program *p)
{
    if (!setup || !setup->active_mask ||
        !sc64_z8_frame_ms_valid(setup->frame_ms)) return 0;
    unsigned instances = 0;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        if (setup->force[o] > 4) return 0;
        if (!(setup->active_mask & (1u << o))) continue;
        for (unsigned i = 0; i < p->trigger_count; ++i) {
            unsigned n = sc64_z8_trigger_matches(&p->triggers[i], o,
                                                 setup->force[o]);
            /* Natively each matching selector appends its own instance;
             * duplicate instances are not needed by Z8 and are rejected. */
            if (n > 1) return 0;
            instances += n;
        }
    }
    return instances && instances <= SC64_Z8_INSTANCE_LIMIT;
}

static inline int
sc64_z8_init(Sc64Z8SchedulerState *s, const Sc64Z8Program *p,
             const Sc64Z8Setup *setup)
{
    if (!s || !sc64_z8_program_valid(p) || !sc64_z8_setup_valid(setup, p))
        return 0;
    u8 *bytes = (u8 *)s;
    for (unsigned i = 0; i < sizeof(*s); ++i) bytes[i] = 0;
    s->version = SC64_Z8_STATE_VERSION;
    s->active_mask = setup->active_mask;
    s->frame_ms = setup->frame_ms;
    s->pass_counter = SC64_Z8_PASS_INITIAL;
    s->second_counter = SC64_Z8_SECOND_RELOAD;
    s->trigger_count = (u8)p->trigger_count;
    s->location_count = (u8)p->location_count;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o)
        s->force[o] = setup->force[o];
    /* Per-owner lists in ascending owner order, each in trigger order. */
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        if (!(s->active_mask & (1u << o))) continue;
        for (unsigned i = 0; i < p->trigger_count; ++i) {
            if (!sc64_z8_trigger_matches(&p->triggers[i], o, s->force[o]))
                continue;
            Sc64Z8Instance *in = &s->instances[s->instance_count++];
            in->trigger = (u8)i;
            in->owner = (u8)o;
        }
    }
    for (unsigned i = 0; i < p->location_count; ++i) {
        s->locations[i].left = (s16)p->locations[i].left;
        s->locations[i].top = (s16)p->locations[i].top;
        s->locations[i].right = (s16)p->locations[i].right;
        s->locations[i].bottom = (s16)p->locations[i].bottom;
    }
    return 1;
}

static inline const Sc64Z8Action *
sc64_z8_action_at(const Sc64Z8Program *p, const Sc64Z8Trigger *t,
                  unsigned slot)
{
    for (unsigned r = 0; r < t->action_count; ++r) {
        const Sc64Z8Action *a = &p->actions[t->first_action + r];
        if (a->index == slot) return a;
        if (a->index > slot) break;
    }
    return 0;
}

static inline int
sc64_z8_action_timed(unsigned opcode)
{
    return opcode == 4 || opcode == 7 || opcode == 10;
}

/* Full persisted-state check for restore and the checked tick. A valid
 * state can only rest at a timed action: every other handler completes. */
static inline int
sc64_z8_state_valid(const Sc64Z8SchedulerState *s, const Sc64Z8Program *p)
{
    if (!s || !sc64_z8_program_valid(p) || s->version != SC64_Z8_STATE_VERSION ||
        s->trigger_count != p->trigger_count ||
        s->location_count != p->location_count ||
        s->fault || s->fault_owner || s->fault_trigger || s->fault_slot ||
        s->pass_counter > SC64_Z8_PASS_RELOAD ||
        s->second_counter > SC64_Z8_SECOND_RELOAD ||
        !sc64_z8_frame_ms_valid(s->frame_ms)) return 0;
    Sc64Z8Setup setup;
    setup.active_mask = s->active_mask;
    setup.frame_ms = s->frame_ms;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) setup.force[o] = s->force[o];
    if (!sc64_z8_setup_valid(&setup, p)) return 0;
    unsigned n = 0, armed_mask = 0, instance_owners = 0;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        if (!(s->active_mask & (1u << o))) continue;
        for (unsigned i = 0; i < p->trigger_count; ++i) {
            if (!sc64_z8_trigger_matches(&p->triggers[i], o, s->force[o]))
                continue;
            if (n >= s->instance_count) return 0;
            const Sc64Z8Instance *in = &s->instances[n++];
            if (in->trigger != i || in->owner != o) return 0;
            instance_owners |= 1u << o;
            unsigned f = in->flags;
            if (f & ~0x0fu) return 0;
            if (f & SC64_Z8_INST_DONE) {
                if (f != SC64_Z8_INST_DONE || in->cursor) return 0;
            } else if (!(f & SC64_Z8_INST_EXECUTING)) {
                if ((f & SC64_Z8_INST_ARMED) || in->cursor) return 0;
            } else {
                const Sc64Z8Action *a = in->cursor < SC64_Z8_ACTION_SLOTS ?
                    sc64_z8_action_at(p, &p->triggers[i], in->cursor) : 0;
                if (!a || (a->flags & 2) || !sc64_z8_action_timed(a->opcode))
                    return 0;
                if (f & SC64_Z8_INST_ARMED) armed_mask |= 1u << o;
            }
        }
    }
    if (n != s->instance_count) return 0;
    for (unsigned i = n; i < SC64_Z8_INSTANCE_LIMIT; ++i) {
        const Sc64Z8Instance *in = &s->instances[i];
        if (in->trigger || in->owner || in->flags || in->cursor) return 0;
    }
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        unsigned bit = 1u << o;
        if (s->outcome[o] != SC64_Z8_OUTCOME_NONE &&
            s->outcome[o] != SC64_Z8_OUTCOME_DEFEAT &&
            s->outcome[o] != SC64_Z8_OUTCOME_VICTORY) return 0;
        if (s->outcome[o] && !(instance_owners & bit)) return 0;
        /* Only an instance arms a wait, and it stays armed until it resumes
         * after the wait has ended. */
        if (s->waiting_mask & bit) {
            if (!(armed_mask & bit)) return 0;
        } else if (s->wait_ms[o]) return 0;
    }
    for (unsigned i = 0; i < SC64_Z8_LOCATION_LIMIT; ++i) {
        const Sc64Z8Rect *r = &s->locations[i];
        if (i >= p->location_count) {
            if (r->left || r->top || r->right || r->bottom) return 0;
        } else if (r->left > r->right || r->top > r->bottom) return 0;
    }
    return 1;
}

/* Copy a saved image only after it passes the full validator; `out` is not
 * touched on rejection. */
static inline int
sc64_z8_restore(Sc64Z8SchedulerState *out, const Sc64Z8SchedulerState *saved,
                const Sc64Z8Program *p)
{
    if (!out || !sc64_z8_state_valid(saved, p)) return 0;
    *out = *saved;
    return 1;
}

static inline void
sc64_z8_fault(Sc64Z8SchedulerState *s, unsigned fault, unsigned owner,
              unsigned trigger, unsigned slot)
{
    s->fault = (u8)fault;
    s->fault_owner = (u8)owner;
    s->fault_trigger = (u8)trigger;
    s->fault_slot = (u8)slot;
}

/* 0x8008CDB4 arms an owner's shared wait. */
static inline void
sc64_z8_arm_wait(Sc64Z8SchedulerState *s, unsigned owner, u32 ms)
{
    s->waiting_mask |= (u8)(1u << owner);
    s->wait_ms[owner] = ms;
}

/* 0x8008CCF8: end an owner's wait and make the pass counter 1, so the next
 * pass runs on the following frame. Unlike the native routine, this refuses
 * an owner that is not waiting so a stray completion cannot shift cadence. */
static inline int
sc64_z8_release_wait(Sc64Z8SchedulerState *s, unsigned owner)
{
    if (!s || owner >= SC64_Z8_OWNER_LIMIT || s->fault ||
        !(s->waiting_mask & (1u << owner))) return 0;
    s->waiting_mask &= (u8)~(1u << owner);
    s->wait_ms[owner] = 0;
    s->pass_counter = SC64_Z8_PASS_INITIAL;
    return 1;
}

static inline int
sc64_z8_compare(unsigned comparison, u32 value, u32 amount)
{
    switch (comparison) {
    case 0: return value >= amount;
    case 1: return value <= amount;
    case 10: return value == amount;
    default: return -1;
    }
}

static inline const Sc64Z8Rect *
sc64_z8_rect(const Sc64Z8SchedulerState *s, u32 location)
{
    return location ? &s->locations[location - 1] : 0;
}

static inline int
sc64_z8_switch_get(const Sc64Z8SchedulerState *s, unsigned index)
{
    return (s->switches[index >> 3] >> (index & 7)) & 1;
}

/* Returns 1 (true), 0 (false) or -1 (fault). */
static inline int
sc64_z8_condition(const Sc64Z8SchedulerState *s, const Sc64Z8Condition *c,
                  unsigned owner, const Sc64Z8WorldOps *ops, void *context)
{
    u32 count = 0;
    const Sc64Z8Rect *rect = 0;
    switch (c->opcode) {
    case 3:
        /* 0x80086484 is false for location 0 before it counts anything. */
        if (!c->location) return 0;
        rect = sc64_z8_rect(s, c->location);
        /* fall through */
    case 2: case 5:
        if (!ops || !ops->count ||
            ops->count(context, c, owner, rect, &count) != 1) return -1;
        return sc64_z8_compare(c->comparison, count, c->amount);
    case 11: {
        int set = sc64_z8_switch_get(s, c->resource);
        return c->comparison == 2 ? set : c->comparison == 3 ? !set : -1;
    }
    case 12:
        return sc64_z8_compare(c->comparison, s->elapsed_seconds, c->amount);
    case 22: return 1;
    case 23: return 0;
    default: return -1;
    }
}

static inline int
sc64_z8_conditions(Sc64Z8SchedulerState *s, const Sc64Z8Program *p,
                   const Sc64Z8Instance *in, const Sc64Z8WorldOps *ops,
                   void *context)
{
    const Sc64Z8Trigger *t = &p->triggers[in->trigger];
    unsigned expected = 0;
    /* 0x8008D038: source-slot order, disabled rows skipped, the first
     * all-zero slot ends the list as a pass, the first false row fails. */
    for (unsigned r = 0; r < t->condition_count; ++r) {
        const Sc64Z8Condition *c = &p->conditions[t->first_condition + r];
        if (c->index != expected) return 1;
        expected = c->index + 1u;
        if (c->flags & 2) continue;
        int result = sc64_z8_condition(s, c, in->owner, ops, context);
        if (result < 0) {
            sc64_z8_fault(s, SC64_Z8_FAULT_CONDITION, in->owner, in->trigger,
                          c->index);
            return -1;
        }
        if (!result) return 0;
    }
    return 1;
}

/* 0x800840EC: recentre the destination on the anchor, then clamp each axis
 * once using the native order and 16-bit stores. */
static inline void
sc64_z8_move_location(Sc64Z8Rect *dst, s32 x, s32 y, s32 width, s32 height)
{
    s32 dx = x - ((s32)dst->left + dst->right) / 2;
    s32 dy = y - ((s32)dst->top + dst->bottom) / 2;
    s32 left = (s16)(u16)(dst->left + dx), right = (s16)(u16)(dst->right + dx);
    s32 top = (s16)(u16)(dst->top + dy), bottom = (s16)(u16)(dst->bottom + dy);
    if (left < 0) { right = (s16)(u16)(right - left); left = 0; }
    if (top < 0) { bottom = (s16)(u16)(bottom - top); top = 0; }
    if (right >= width) {
        left = (s16)(u16)(left - 1 - (right - width));
        right = width - 1;
    }
    if (bottom >= height) {
        top = (s16)(u16)(top - 1 - (bottom - height));
        bottom = height - 1;
    }
    dst->left = (s16)left; dst->right = (s16)right;
    dst->top = (s16)top; dst->bottom = (s16)bottom;
}

/* Returns 1 (advance), 0 (stall at this slot) or -1 (fault recorded). */
static inline int
sc64_z8_action(Sc64Z8SchedulerState *s, const Sc64Z8Program *p,
               Sc64Z8Instance *in, const Sc64Z8Action *a,
               const Sc64Z8WorldOps *ops, void *context, u8 *pass_outcome)
{
    unsigned owner = in->owner;
    unsigned owner_bit = 1u << owner;
    const Sc64Z8Rect *rect = sc64_z8_rect(s, a->location);
    int r;
    switch (a->opcode) {
    case 1: /* 0x8008063C: Victory overrides Defeat in the same pass. */
        pass_outcome[owner] = SC64_Z8_OUTCOME_VICTORY;
        return 1;
    case 2: /* 0x80080690: Defeat yields to an earlier Victory. */
        if (pass_outcome[owner] != SC64_Z8_OUTCOME_VICTORY)
            pass_outcome[owner] = SC64_Z8_OUTCOME_DEFEAT;
        return 1;
    case 3: /* 0x8008071C */
        in->flags |= SC64_Z8_INST_PRESERVE;
        return 1;
    case 4: /* 0x8008073C */
        if (s->waiting_mask & owner_bit) return 0;
        if (in->flags & SC64_Z8_INST_ARMED) {
            in->flags &= (u8)~SC64_Z8_INST_ARMED;
            return 1;
        }
        sc64_z8_arm_wait(s, owner, a->time);
        in->flags |= SC64_Z8_INST_ARMED;
        return 0;
    case 7: case 10: { /* 0x80080DA8 Transmission, 0x80081444 CenterView */
        u32 duration = 0;
        if (!a->location) return 1;
        if (!ops || !ops->timed) break;
        r = ops->timed(context, SC64_Z8_TIMED_ELIGIBLE, a, owner, rect, &duration);
        /* Natively an ineligible armed action keeps its per-action bit for a
         * later preserved run; that transition is not modelled. */
        if (r == 0 && !(in->flags & SC64_Z8_INST_ARMED)) return 1;
        if (r != 1) break;
        if (s->waiting_mask & owner_bit) return 0;
        if (in->flags & SC64_Z8_INST_ARMED) {
            if (ops->timed(context, SC64_Z8_TIMED_FINISH, a, owner, rect,
                           &duration) != 1) break;
            in->flags &= (u8)~SC64_Z8_INST_ARMED;
            return 1;
        }
        if (ops->timed(context, SC64_Z8_TIMED_START, a, owner, rect,
                       &duration) != 1) break;
        sc64_z8_arm_wait(s, owner, duration);
        in->flags |= SC64_Z8_INST_ARMED;
        return 0;
    }
    case 13: { /* 0x80081ADC: one mission-wide 256-switch bank. */
        u8 *byte = &s->switches[a->group2 >> 3];
        u8 bit = (u8)(1u << (a->group2 & 7));
        if (a->modifier == 4) *byte |= bit;
        else if (a->modifier == 5) *byte &= (u8)~bit;
        else *byte ^= bit;
        return 1;
    }
    case 38: { /* 0x800840EC */
        s16 x = 0, y = 0;
        if (!a->location || !a->group2) return 1;
        if (!ops || !ops->find_unit) break;
        r = ops->find_unit(context, a, owner, rect, &x, &y);
        if (r == 0) {
            x = (s16)(((s32)rect->left + rect->right) / 2);
            y = (s16)(((s32)rect->top + rect->bottom) / 2);
        } else if (r != 1) break;
        sc64_z8_move_location(&s->locations[a->group2 - 1], x, y,
                              p->map_pixel_width, p->map_pixel_height);
        return 1;
    }
    case 11: case 16: case 28: case 43:
        /* These handlers return success before touching the world when the
         * location byte is zero (0x80080620). */
        if (!a->location) return 1;
        /* fall through */
    case 8: case 9: case 12: case 26:
        if (!ops || !ops->action ||
            ops->action(context, a, owner, rect) != 1) break;
        return 1;
    default:
        break;
    }
    sc64_z8_fault(s, SC64_Z8_FAULT_ACTION, owner, in->trigger, a->index);
    return -1;
}

/* 0x8008D108: run from the cursor until an action stalls or the list ends.
 * Completion resets a preserved instance for re-evaluation in a later pass
 * and retires any other instance. */
static inline int
sc64_z8_execute(Sc64Z8SchedulerState *s, const Sc64Z8Program *p,
                Sc64Z8Instance *in, const Sc64Z8WorldOps *ops, void *context,
                u8 *pass_outcome)
{
    const Sc64Z8Trigger *t = &p->triggers[in->trigger];
    unsigned cursor = in->cursor, r = 0;
    in->flags |= SC64_Z8_INST_EXECUTING;
    while (r < t->action_count && p->actions[t->first_action + r].index < cursor)
        ++r;
    while (cursor < SC64_Z8_ACTION_SLOTS) {
        const Sc64Z8Action *a = r < t->action_count ?
            &p->actions[t->first_action + r] : 0;
        if (!a || a->index != cursor) { cursor = SC64_Z8_ACTION_SLOTS; break; }
        if (!(a->flags & 2)) {
            int result = sc64_z8_action(s, p, in, a, ops, context, pass_outcome);
            if (result < 0) return 0;
            if (!result) break;
        }
        ++cursor; ++r;
    }
    if (cursor < SC64_Z8_ACTION_SLOTS) {
        in->cursor = (u8)cursor;
    } else if (in->flags & SC64_Z8_INST_PRESERVE) {
        in->flags &= (u8)~(SC64_Z8_INST_EXECUTING | SC64_Z8_INST_ARMED);
        in->cursor = 0;
    } else {
        in->flags = SC64_Z8_INST_DONE;
        in->cursor = 0;
    }
    return 1;
}

/* One trigger pass (0x8008DAEC -> 0x8008D240 per owner). Owners run in
 * ascending slot order and each owner's instances in trigger order; every
 * effect is visible to all later instances in the same pass. */
static inline int
sc64_z8_pass(Sc64Z8SchedulerState *s, const Sc64Z8Program *p,
             const Sc64Z8WorldOps *ops, void *context)
{
    u8 pass_outcome[SC64_Z8_OWNER_LIMIT];
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) pass_outcome[o] = 0;
    for (unsigned i = 0; i < s->instance_count; ++i) {
        Sc64Z8Instance *in = &s->instances[i];
        if (s->outcome[in->owner] || (in->flags & SC64_Z8_INST_DONE)) continue;
        if (!(in->flags & SC64_Z8_INST_EXECUTING)) {
            int match = sc64_z8_conditions(s, p, in, ops, context);
            if (match < 0) return 0;
            if (!match) continue;
            in->cursor = 0;
        }
        if (!sc64_z8_execute(s, p, in, ops, context, pass_outcome)) return 0;
    }
    /* The native post-pass resolution (0x8008D768) also ends the mission
     * for other players; the caller owns that decision. */
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o)
        if (pass_outcome[o]) s->outcome[o] = pass_outcome[o];
    if (s->passes != 0xffffffffu) ++s->passes;
    return 1;
}

/* Cheap per-tick checks for a state that passed sc64_z8_state_valid() at
 * init or restore and has since been changed only by this scheduler. */
static inline int
sc64_z8_preflight(const Sc64Z8SchedulerState *s, const Sc64Z8Program *p)
{
    return s && p && s->version == SC64_Z8_STATE_VERSION && !s->fault &&
           s->trigger_count == p->trigger_count &&
           s->location_count == p->location_count &&
           s->instance_count <= SC64_Z8_INSTANCE_LIMIT &&
           s->pass_counter <= SC64_Z8_PASS_RELOAD &&
           s->second_counter <= SC64_Z8_SECOND_RELOAD &&
           sc64_z8_frame_ms_valid(s->frame_ms);
}

/* Advance one native game frame. A paused frame changes nothing. Returns 1,
 * or 0 after a fault (which persists until init or restore). */
static inline int
sc64_z8_tick_prevalidated(Sc64Z8SchedulerState *s, const Sc64Z8Program *p,
                          const Sc64Z8WorldOps *ops, void *context, int paused)
{
    if (!sc64_z8_preflight(s, p)) {
        if (s && !s->fault) sc64_z8_fault(s, SC64_Z8_FAULT_STATE, 0, 0, 0);
        return 0;
    }
    if (paused) return 1;
    if (s->frames != 0xffffffffu) ++s->frames;
    /* 0x8008D4B8: a timer no larger than the frame step expires. */
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        if (!(s->waiting_mask & (1u << o)) ||
            s->wait_ms[o] == SC64_Z8_WAIT_INDEFINITE) continue;
        if (s->frame_ms < s->wait_ms[o]) s->wait_ms[o] -= s->frame_ms;
        else sc64_z8_release_wait(s, o);
    }
    if (s->second_counter-- == 0) {
        s->second_counter = SC64_Z8_SECOND_RELOAD;
        if (s->elapsed_seconds != 0xffffffffu) ++s->elapsed_seconds;
    }
    /* 0x8008DB30: the pass runs when the counter underflows. */
    if (s->pass_counter-- != 0) return 1;
    s->pass_counter = SC64_Z8_PASS_RELOAD;
    return sc64_z8_pass(s, p, ops, context);
}

/* Checked entry point for fixtures and untrusted state. */
static inline int
sc64_z8_tick(Sc64Z8SchedulerState *s, const Sc64Z8Program *p,
             const Sc64Z8WorldOps *ops, void *context, int paused)
{
    if (!sc64_z8_state_valid(s, p)) {
        if (s && !s->fault) sc64_z8_fault(s, SC64_Z8_FAULT_STATE, 0, 0, 0);
        return 0;
    }
    return sc64_z8_tick_prevalidated(s, p, ops, context, paused);
}

#endif /* SC64_Z8_TRIGGERS_H */
