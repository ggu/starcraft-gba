/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Trigger scheduler for the authenticated Z9 and Z10 scenarios.
 *
 * This is the Z8 native-cadence scheduler (sc64_z8_triggers.h) extended with
 * the rows the two final Zerg programs add. It reuses the Z8 row types,
 * instance layout, cadence constants and pure helpers unchanged, and keeps
 * its own state and dispatch so the Z8 scheduler and its saves stay
 * byte-for-byte as they are. Additions over Z8:
 *   - Countdown (condition 1) and SetCountdownTimer (action 14): one
 *     mission-wide timer in seconds. It decrements when the elapsed-second
 *     counter advances and stops at zero. That cadence is authored from the
 *     elapsed-time clock; no native countdown routine has been traced.
 *   - Deaths (condition 15): a census callback, like Kill.
 *   - RemoveUnitAtLocation (action 25): a world callback.
 *   - RunAIScriptAtLocation (action 16) accepts the program's own FourCC list
 *     instead of Z8's three scripts.
 * Everything else, including timing, ordering and faults, is Z8's model.
 */
#ifndef SC64_ZERG_LATE_TRIGGERS_H
#define SC64_ZERG_LATE_TRIGGERS_H

#include "sc64_z8_triggers.h"

enum {
    SC64_ZL_STATE_VERSION = 0x5a4c0001,
    SC64_ZL_AI_SCRIPT_LIMIT = 8,
    /* SetCountdownTimer modifiers, as in SetResources. */
    SC64_ZL_MOD_SET = 7,
    SC64_ZL_MOD_ADD = 8,
    SC64_ZL_MOD_SUBTRACT = 9,
    /* Bounded so Add can never wrap; Z9 sets 600 seconds. */
    SC64_ZL_COUNTDOWN_LIMIT = 65535
};

typedef struct {
    Sc64Z8Program rows;
    const u32 *ai_scripts;   /* little-endian CHK group2 FourCCs */
    u8 ai_script_count;
} Sc64ZlProgram;

typedef struct {
    Sc64Z8SchedulerState core; /* core.version is SC64_ZL_STATE_VERSION */
    u32 countdown;             /* seconds */
} Sc64ZlSchedulerState;

_Static_assert(sizeof(Sc64ZlSchedulerState) == 880, "late Zerg scheduler state budget");

/* Callback contract. The first four are Sc64Z8WorldOps with the same
 * meaning; each returns exactly the values listed, and anything else, or a
 * missing callback, faults the scheduler. */
typedef struct {
    Sc64Z8WorldOps base;
    /* RunAIScriptAtLocation (16): 1 once applied, or -1. */
    int (*run_ai)(void *context, const Sc64Z8Action *action, unsigned owner,
                  const Sc64Z8Rect *rect);
    /* RemoveUnitAtLocation (25): 1 once applied, or -1. */
    int (*remove)(void *context, const Sc64Z8Action *action, unsigned owner,
                  const Sc64Z8Rect *rect);
    /* Deaths (15): 1 with *count, or -1. */
    int (*deaths)(void *context, const Sc64Z8Condition *condition,
                  unsigned owner, u32 *count);
} Sc64ZlWorldOps;

static inline int
sc64_zl_ai_script_known(const Sc64ZlProgram *p, u32 fourcc)
{
    for (unsigned i = 0; p && i < p->ai_script_count; ++i)
        if (p->ai_scripts[i] == fourcc) return 1;
    return 0;
}

static inline int
sc64_zl_condition_supported(const Sc64Z8Condition *c, unsigned location_count)
{
    if (c->flags & ~0x12u) return 0;
    if (c->flags & 2) return c->opcode && c->opcode < 24; /* never dispatched */
    switch (c->opcode) {
    case 1: case 2: case 3: case 5: case 11: case 12: case 15: case 22: case 23:
        break;
    default: return 0;
    }
    if (c->location > location_count) return 0;
    return sc64_z8_comparison_known(c->opcode, c->comparison);
}

static inline int
sc64_zl_action_supported(const Sc64ZlProgram *p, const Sc64Z8Action *a)
{
    unsigned location_count = p->rows.location_count;
    if (a->flags & ~0x16u) return 0;
    if (a->flags & 2) return a->opcode && a->opcode < 59; /* never dispatched */
    if (a->location > location_count) return 0;
    switch (a->opcode) {
    case 1: case 2: case 3: case 4: case 7: case 8: case 9: case 10:
    case 11: case 12: case 25: case 26: case 28: case 43:
        return 1;
    case 13:
        return a->group2 < 8u * SC64_Z8_SWITCH_BYTES &&
               a->modifier >= 4 && a->modifier <= 6;
    case 14:
        return a->modifier >= SC64_ZL_MOD_SET && a->modifier <= SC64_ZL_MOD_SUBTRACT &&
               a->time <= SC64_ZL_COUNTDOWN_LIMIT;
    case 16:
        return sc64_zl_ai_script_known(p, a->group2);
    case 38:
        return a->group2 <= location_count;
    default:
        return 0;
    }
}

static inline int
sc64_zl_program_valid(const Sc64ZlProgram *zp)
{
    const Sc64Z8Program *p = zp ? &zp->rows : 0;
    if (!p || !p->triggers || !p->trigger_count ||
        zp->ai_script_count > SC64_ZL_AI_SCRIPT_LIMIT ||
        (zp->ai_script_count && !zp->ai_scripts) ||
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
                !sc64_zl_condition_supported(c, p->location_count)) return 0;
            last = c->index;
        }
        last = -1;
        for (unsigned r = 0; r < t->action_count; ++r) {
            const Sc64Z8Action *a = &p->actions[t->first_action + r];
            if ((int)a->index <= last || a->index >= SC64_Z8_ACTION_SLOTS ||
                !sc64_zl_action_supported(zp, a)) return 0;
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
sc64_zl_init(Sc64ZlSchedulerState *s, const Sc64ZlProgram *zp,
             const Sc64Z8Setup *setup)
{
    Sc64Z8SchedulerState *c = s ? &s->core : 0;
    const Sc64Z8Program *p = zp ? &zp->rows : 0;
    if (!s || !sc64_zl_program_valid(zp) || !sc64_z8_setup_valid(setup, p))
        return 0;
    u8 *bytes = (u8 *)s;
    for (unsigned i = 0; i < sizeof(*s); ++i) bytes[i] = 0;
    c->version = SC64_ZL_STATE_VERSION;
    c->active_mask = setup->active_mask;
    c->frame_ms = setup->frame_ms;
    c->pass_counter = SC64_Z8_PASS_INITIAL;
    c->second_counter = SC64_Z8_SECOND_RELOAD;
    c->trigger_count = (u8)p->trigger_count;
    c->location_count = (u8)p->location_count;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o)
        c->force[o] = setup->force[o];
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        if (!(c->active_mask & (1u << o))) continue;
        for (unsigned i = 0; i < p->trigger_count; ++i) {
            if (!sc64_z8_trigger_matches(&p->triggers[i], o, c->force[o]))
                continue;
            Sc64Z8Instance *in = &c->instances[c->instance_count++];
            in->trigger = (u8)i;
            in->owner = (u8)o;
        }
    }
    for (unsigned i = 0; i < p->location_count; ++i) {
        c->locations[i].left = (s16)p->locations[i].left;
        c->locations[i].top = (s16)p->locations[i].top;
        c->locations[i].right = (s16)p->locations[i].right;
        c->locations[i].bottom = (s16)p->locations[i].bottom;
    }
    return 1;
}

/* Full persisted-state check, as sc64_z8_state_valid() plus the countdown. */
static inline int
sc64_zl_state_valid(const Sc64ZlSchedulerState *zs, const Sc64ZlProgram *zp)
{
    const Sc64Z8SchedulerState *s = zs ? &zs->core : 0;
    const Sc64Z8Program *p = zp ? &zp->rows : 0;
    if (!s || !sc64_zl_program_valid(zp) || s->version != SC64_ZL_STATE_VERSION ||
        zs->countdown > SC64_ZL_COUNTDOWN_LIMIT ||
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

/* Returns 1 (true), 0 (false) or -1 (fault). */
static inline int
sc64_zl_condition(const Sc64ZlSchedulerState *zs, const Sc64Z8Condition *c,
                  unsigned owner, const Sc64ZlWorldOps *ops, void *context)
{
    const Sc64Z8SchedulerState *s = &zs->core;
    u32 count = 0;
    const Sc64Z8Rect *rect = 0;
    switch (c->opcode) {
    case 1:
        return sc64_z8_compare(c->comparison, zs->countdown, c->amount);
    case 3:
        if (!c->location) return 0;
        rect = sc64_z8_rect(s, c->location);
        /* fall through */
    case 2: case 5:
        if (!ops || !ops->base.count ||
            ops->base.count(context, c, owner, rect, &count) != 1) return -1;
        return sc64_z8_compare(c->comparison, count, c->amount);
    case 11: {
        int set = sc64_z8_switch_get(s, c->resource);
        return c->comparison == 2 ? set : c->comparison == 3 ? !set : -1;
    }
    case 12:
        return sc64_z8_compare(c->comparison, s->elapsed_seconds, c->amount);
    case 15:
        if (!ops || !ops->deaths || ops->deaths(context, c, owner, &count) != 1)
            return -1;
        return sc64_z8_compare(c->comparison, count, c->amount);
    case 22: return 1;
    case 23: return 0;
    default: return -1;
    }
}

static inline int
sc64_zl_conditions(Sc64ZlSchedulerState *zs, const Sc64ZlProgram *zp,
                   const Sc64Z8Instance *in, const Sc64ZlWorldOps *ops,
                   void *context)
{
    const Sc64Z8Program *p = &zp->rows;
    const Sc64Z8Trigger *t = &p->triggers[in->trigger];
    unsigned expected = 0;
    for (unsigned r = 0; r < t->condition_count; ++r) {
        const Sc64Z8Condition *c = &p->conditions[t->first_condition + r];
        if (c->index != expected) return 1;
        expected = c->index + 1u;
        if (c->flags & 2) continue;
        int result = sc64_zl_condition(zs, c, in->owner, ops, context);
        if (result < 0) {
            sc64_z8_fault(&zs->core, SC64_Z8_FAULT_CONDITION, in->owner,
                          in->trigger, c->index);
            return -1;
        }
        if (!result) return 0;
    }
    return 1;
}

/* Returns 1 (advance), 0 (stall at this slot) or -1 (fault recorded). */
static inline int
sc64_zl_action(Sc64ZlSchedulerState *zs, const Sc64ZlProgram *zp,
               Sc64Z8Instance *in, const Sc64Z8Action *a,
               const Sc64ZlWorldOps *ops, void *context, u8 *pass_outcome)
{
    Sc64Z8SchedulerState *s = &zs->core;
    const Sc64Z8Program *p = &zp->rows;
    unsigned owner = in->owner;
    unsigned owner_bit = 1u << owner;
    const Sc64Z8Rect *rect = sc64_z8_rect(s, a->location);
    int r;
    switch (a->opcode) {
    case 1:
        pass_outcome[owner] = SC64_Z8_OUTCOME_VICTORY;
        return 1;
    case 2:
        if (pass_outcome[owner] != SC64_Z8_OUTCOME_VICTORY)
            pass_outcome[owner] = SC64_Z8_OUTCOME_DEFEAT;
        return 1;
    case 3:
        in->flags |= SC64_Z8_INST_PRESERVE;
        return 1;
    case 4:
        if (s->waiting_mask & owner_bit) return 0;
        if (in->flags & SC64_Z8_INST_ARMED) {
            in->flags &= (u8)~SC64_Z8_INST_ARMED;
            return 1;
        }
        sc64_z8_arm_wait(s, owner, a->time);
        in->flags |= SC64_Z8_INST_ARMED;
        return 0;
    case 7: case 10: {
        u32 duration = 0;
        if (!a->location) return 1;
        if (!ops || !ops->base.timed) break;
        r = ops->base.timed(context, SC64_Z8_TIMED_ELIGIBLE, a, owner, rect, &duration);
        if (r == 0 && !(in->flags & SC64_Z8_INST_ARMED)) return 1;
        if (r != 1) break;
        if (s->waiting_mask & owner_bit) return 0;
        if (in->flags & SC64_Z8_INST_ARMED) {
            if (ops->base.timed(context, SC64_Z8_TIMED_FINISH, a, owner, rect,
                                &duration) != 1) break;
            in->flags &= (u8)~SC64_Z8_INST_ARMED;
            return 1;
        }
        if (ops->base.timed(context, SC64_Z8_TIMED_START, a, owner, rect,
                            &duration) != 1) break;
        sc64_z8_arm_wait(s, owner, duration);
        in->flags |= SC64_Z8_INST_ARMED;
        return 0;
    }
    case 13: {
        u8 *byte = &s->switches[a->group2 >> 3];
        u8 bit = (u8)(1u << (a->group2 & 7));
        if (a->modifier == 4) *byte |= bit;
        else if (a->modifier == 5) *byte &= (u8)~bit;
        else *byte ^= bit;
        return 1;
    }
    case 14: {
        u32 value = zs->countdown;
        if (a->modifier == SC64_ZL_MOD_SET) value = a->time;
        else if (a->modifier == SC64_ZL_MOD_ADD) value += a->time;
        else if (a->modifier == SC64_ZL_MOD_SUBTRACT)
            value = value > a->time ? value - a->time : 0u;
        else break;
        zs->countdown = value > SC64_ZL_COUNTDOWN_LIMIT ? SC64_ZL_COUNTDOWN_LIMIT : value;
        return 1;
    }
    case 38: {
        s16 x = 0, y = 0;
        if (!a->location || !a->group2) return 1;
        if (!ops || !ops->base.find_unit) break;
        r = ops->base.find_unit(context, a, owner, rect, &x, &y);
        if (r == 0) {
            x = (s16)(((s32)rect->left + rect->right) / 2);
            y = (s16)(((s32)rect->top + rect->bottom) / 2);
        } else if (r != 1) break;
        sc64_z8_move_location(&s->locations[a->group2 - 1], x, y,
                              p->map_pixel_width, p->map_pixel_height);
        return 1;
    }
    case 16:
        if (!a->location) return 1;
        if (!ops || !ops->run_ai || !sc64_zl_ai_script_known(zp, a->group2) ||
            ops->run_ai(context, a, owner, rect) != 1) break;
        return 1;
    case 25:
        if (!a->location) return 1;
        if (!ops || !ops->remove || ops->remove(context, a, owner, rect) != 1) break;
        return 1;
    case 11: case 28: case 43:
        if (!a->location) return 1;
        /* fall through */
    case 8: case 9: case 12: case 26:
        if (!ops || !ops->base.action ||
            ops->base.action(context, a, owner, rect) != 1) break;
        return 1;
    default:
        break;
    }
    sc64_z8_fault(s, SC64_Z8_FAULT_ACTION, owner, in->trigger, a->index);
    return -1;
}

static inline int
sc64_zl_execute(Sc64ZlSchedulerState *zs, const Sc64ZlProgram *zp,
                Sc64Z8Instance *in, const Sc64ZlWorldOps *ops, void *context,
                u8 *pass_outcome)
{
    const Sc64Z8Program *p = &zp->rows;
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
            int result = sc64_zl_action(zs, zp, in, a, ops, context, pass_outcome);
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

static inline int
sc64_zl_pass(Sc64ZlSchedulerState *zs, const Sc64ZlProgram *zp,
             const Sc64ZlWorldOps *ops, void *context)
{
    Sc64Z8SchedulerState *s = &zs->core;
    u8 pass_outcome[SC64_Z8_OWNER_LIMIT];
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) pass_outcome[o] = 0;
    for (unsigned i = 0; i < s->instance_count; ++i) {
        Sc64Z8Instance *in = &s->instances[i];
        if (s->outcome[in->owner] || (in->flags & SC64_Z8_INST_DONE)) continue;
        if (!(in->flags & SC64_Z8_INST_EXECUTING)) {
            int match = sc64_zl_conditions(zs, zp, in, ops, context);
            if (match < 0) return 0;
            if (!match) continue;
            in->cursor = 0;
        }
        if (!sc64_zl_execute(zs, zp, in, ops, context, pass_outcome)) return 0;
    }
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o)
        if (pass_outcome[o]) s->outcome[o] = pass_outcome[o];
    if (s->passes != 0xffffffffu) ++s->passes;
    return 1;
}

static inline int
sc64_zl_preflight(const Sc64ZlSchedulerState *zs, const Sc64ZlProgram *zp)
{
    const Sc64Z8SchedulerState *s = zs ? &zs->core : 0;
    return s && zp && s->version == SC64_ZL_STATE_VERSION && !s->fault &&
           zs->countdown <= SC64_ZL_COUNTDOWN_LIMIT &&
           s->trigger_count == zp->rows.trigger_count &&
           s->location_count == zp->rows.location_count &&
           s->instance_count <= SC64_Z8_INSTANCE_LIMIT &&
           s->pass_counter <= SC64_Z8_PASS_RELOAD &&
           s->second_counter <= SC64_Z8_SECOND_RELOAD &&
           sc64_z8_frame_ms_valid(s->frame_ms);
}

/* Advance one native game frame; the Z8 cadence plus the countdown. */
static inline int
sc64_zl_tick_prevalidated(Sc64ZlSchedulerState *zs, const Sc64ZlProgram *zp,
                          const Sc64ZlWorldOps *ops, void *context, int paused)
{
    Sc64Z8SchedulerState *s = zs ? &zs->core : 0;
    if (!sc64_zl_preflight(zs, zp)) {
        if (s && !s->fault) sc64_z8_fault(s, SC64_Z8_FAULT_STATE, 0, 0, 0);
        return 0;
    }
    if (paused) return 1;
    if (s->frames != 0xffffffffu) ++s->frames;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        if (!(s->waiting_mask & (1u << o)) ||
            s->wait_ms[o] == SC64_Z8_WAIT_INDEFINITE) continue;
        if (s->frame_ms < s->wait_ms[o]) s->wait_ms[o] -= s->frame_ms;
        else sc64_z8_release_wait(s, o);
    }
    if (s->second_counter-- == 0) {
        s->second_counter = SC64_Z8_SECOND_RELOAD;
        if (s->elapsed_seconds != 0xffffffffu) ++s->elapsed_seconds;
        if (zs->countdown) --zs->countdown;
    }
    if (s->pass_counter-- != 0) return 1;
    s->pass_counter = SC64_Z8_PASS_RELOAD;
    return sc64_zl_pass(zs, zp, ops, context);
}

static inline int
sc64_zl_tick(Sc64ZlSchedulerState *zs, const Sc64ZlProgram *zp,
             const Sc64ZlWorldOps *ops, void *context, int paused)
{
    if (!sc64_zl_state_valid(zs, zp)) {
        if (zs && !zs->core.fault) sc64_z8_fault(&zs->core, SC64_Z8_FAULT_STATE, 0, 0, 0);
        return 0;
    }
    return sc64_zl_tick_prevalidated(zs, zp, ops, context, paused);
}

#endif /* SC64_ZERG_LATE_TRIGGERS_H */
