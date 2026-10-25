#ifndef SC64_Z6_ADAPTER_H
#define SC64_Z6_ADAPTER_H

#include "sc64_z6.h"
#include "sc64_z6_scenario_generated.h"

enum {
    SC64_Z6_TICKS_PER_SECOND = 30,
    SC64_Z6_PRESENTATION_HOLD_TICKS = 90,
    SC64_Z6_FAULT_NONE = 0,
    SC64_Z6_FAULT_STATE = 1,
    SC64_Z6_FAULT_CONDITION = 2,
    SC64_Z6_FAULT_ACTION = 3,
    SC64_Z6_FAULT_SWITCH = 4,
    SC64_Z6_FAULT_METADATA = 5
};

_Static_assert(SC64_Z6_SOURCE_GLOBAL_ID == SC64_Z6_GLOBAL_ID,
               "Z6 staged source ID mismatch");
_Static_assert(SC64_Z6_SOURCE_MAP_COLS == SC64_Z6_MAP_COLS &&
               SC64_Z6_SOURCE_MAP_ROWS == SC64_Z6_MAP_ROWS,
               "Z6 staged map dimensions mismatch");
_Static_assert(SC64_Z6_SOURCE_TRIGGER_COUNT == SC64_Z6_TRIGGER_COUNT,
               "Z6 staged trigger count mismatch");
_Static_assert(SC64_Z6_SOURCE_ACTION_COUNT == SC64_Z6_ACTION_TOTAL,
               "Z6 staged action count mismatch");
_Static_assert(SC64_Z6_SOURCE_CONDITION_COUNT == SC64_Z6_CONDITION_TOTAL,
               "Z6 staged condition count mismatch");

typedef struct {
    Sc64Z6State model;
    u16 action_ticks[SC64_Z6_TRIGGER_COUNT];
    u32 switch_bits[8];
    u8 fault;
} Sc64Z6RuntimeState;

_Static_assert(sizeof(Sc64Z6RuntimeState) <= 68, "Z6 runtime state bound");

typedef struct {
    void *context;
    /* Condition callback resolves the exact authenticated numeric row against
       owner/selector, unit, location geometry, switch, and comparison rules.
       It must also respect the row's source execution-player mask (query it
       with sc64_z6_trigger_enabled). Return 1, 0, or a negative result for
       resolution failure. */
    int (*condition)(void *context, const Sc64Z6RuntimeState *state,
                     const Sc64Z6SourceCondition *condition);
    /* Action callback must perform the exact numeric action or return failure.
       It receives private metadata rows and must not claim unsupported success. */
    int (*action)(void *context, const Sc64Z6SourceAction *action);
} Sc64Z6WorldOps;

static inline unsigned sc64_z6_ceil_ticks(u32 milliseconds)
{
    u32 ticks;
    if (milliseconds > 1200000u) milliseconds = 1200000u;
    ticks = (milliseconds * SC64_Z6_TICKS_PER_SECOND + 999u) / 1000u;
    if (!ticks) ticks = 1;
    return ticks > 36000u ? 36000u : (unsigned)ticks;
}

static inline int sc64_z6_source_action(unsigned trigger, unsigned index,
                                         const Sc64Z6SourceAction **out)
{
    const Sc64Z6SourceTrigger *t;
    const Sc64Z6SourceAction *a;
    if (!out || trigger >= SC64_Z6_TRIGGER_COUNT) return 0;
    t = &sc64_z6_source_triggers[trigger];
    if (index >= t->action_count ||
        t->first_action + index >= SC64_Z6_SOURCE_ACTION_COUNT) return 0;
    a = &sc64_z6_source_actions[t->first_action + index];
    if (a->trigger != trigger || a->index != index || !a->opcode) return 0;
    *out = a;
    return 1;
}

static inline u16 sc64_z6_action_duration(unsigned trigger, unsigned index)
{
    const Sc64Z6SourceAction *a;
    if (!sc64_z6_source_action(trigger, index, &a)) return 0;
    switch (a->opcode) {
    case 4: return (u16)sc64_z6_ceil_ticks(a->time); /* Wait */
    case 7: case 9: return SC64_Z6_PRESENTATION_HOLD_TICKS;
    case 8: return (u16)sc64_z6_ceil_ticks(a->time); /* PlayWAV timing */
    case 1: case 2: case 10: case 11: case 12: case 13:
    case 16: case 23: case 26: case 38: return 1;
    default: return 0;
    }
}

static inline int sc64_z6_runtime_metadata_valid(void)
{
    unsigned t;
    for (t = 0; t < SC64_Z6_TRIGGER_COUNT; ++t) {
        const Sc64Z6SourceTrigger *tr = &sc64_z6_source_triggers[t];
        if (tr->action_count != sc64_z6_action_count((u8)t) ||
            tr->condition_count != sc64_z6_condition_count((u8)t)) return 0;
        if (tr->first_action + tr->action_count > SC64_Z6_SOURCE_ACTION_COUNT ||
            tr->first_condition + tr->condition_count > SC64_Z6_SOURCE_CONDITION_COUNT)
            return 0;
    }
    return 1;
}

static inline int sc64_z6_trigger_enabled(unsigned trigger, unsigned player_slot)
{
    if (trigger >= SC64_Z6_TRIGGER_COUNT || player_slot >= 27u) return 0;
    return sc64_z6_source_triggers[trigger].enabled_players[player_slot] == 1;
}

static inline int sc64_z6_runtime_state_valid(const Sc64Z6RuntimeState *s)
{
    unsigned i;
    if (!s || !sc64_z6_state_valid(&s->model) || s->fault ||
        (s->switch_bits[0] & ~3u) || s->switch_bits[1] || s->switch_bits[2] ||
        s->switch_bits[3] || s->switch_bits[4] || s->switch_bits[5] ||
        s->switch_bits[6] || s->switch_bits[7]) return 0;
    for (i = 0; i < SC64_Z6_TRIGGER_COUNT; ++i) {
        u8 cursor = s->model.action_cursor[i];
        u16 ticks = s->action_ticks[i];
        if ((cursor != 0) != (ticks != 0)) return 0;
        if (ticks) {
            u16 duration = sc64_z6_action_duration(i, cursor - 1u);
            if (!duration || ticks > duration) return 0;
        }
    }
    return 1;
}

/* Started records stop polling conditions only when they are source one-shot
 * rows. PreserveTrigger actions and any execution flags retain polling. */
static inline int sc64_z6_trigger_one_shot(unsigned trigger)
{
    const Sc64Z6SourceTrigger *tr;
    if (trigger >= SC64_Z6_TRIGGER_COUNT) return 0;
    tr = &sc64_z6_source_triggers[trigger];
    if (tr->execution_flags) return 0;
    for (unsigned i = 0; i < tr->action_count; ++i) {
        const Sc64Z6SourceAction *a;
        if (!sc64_z6_source_action(trigger, i, &a) || a->opcode == 3u)
            return 0;
    }
    return 1;
}

static inline u16 sc64_z6_runtime_init(Sc64Z6RuntimeState *s)
{
    u8 *p;
    u32 i;
    if (!s) return SC64_Z6_EVENT_NONE;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    return sc64_z6_init(&s->model);
}

static inline int sc64_z6_switch_is_set(const Sc64Z6RuntimeState *s,
                                         unsigned index)
{
    if (!s || index > 255u) return 0;
    return (int)((s->switch_bits[index >> 5] >> (index & 31u)) & 1u);
}

static inline int sc64_z6_condition_row(unsigned trigger, unsigned index,
                                         const Sc64Z6SourceCondition **out)
{
    const Sc64Z6SourceTrigger *t;
    const Sc64Z6SourceCondition *c;
    if (!out || trigger >= SC64_Z6_TRIGGER_COUNT) return 0;
    t = &sc64_z6_source_triggers[trigger];
    if (index >= t->condition_count ||
        t->first_condition + index >= SC64_Z6_SOURCE_CONDITION_COUNT) return 0;
    c = &sc64_z6_source_conditions[t->first_condition + index];
    if (c->trigger != trigger || c->index != index) return 0;
    *out = c;
    return 1;
}

static inline int sc64_z6_dispatch_action(Sc64Z6RuntimeState *s,
                                           const Sc64Z6WorldOps *ops,
                                           const Sc64Z6ActionEvent *event)
{
    const Sc64Z6SourceAction *a;
    u16 duration;
    if (!s || !ops || !ops->action || !event ||
        !sc64_z6_source_action(event->trigger, event->action_index, &a))
        return SC64_Z6_FAULT_METADATA;
    duration = sc64_z6_action_duration(event->trigger, event->action_index);
    if (!duration) return SC64_Z6_FAULT_ACTION;
    if (a->opcode == 13 && (a->group1 != 0 || a->group2 > 255u ||
                            (a->modifier != 4 && a->modifier != 5)))
        return SC64_Z6_FAULT_SWITCH;
    if (ops->action(ops->context, a) != 1) return SC64_Z6_FAULT_ACTION;
    if (a->opcode == 13) {
        u32 bit = (u32)1u << (a->group2 & 31u);
        if (a->modifier == 4) s->switch_bits[a->group2 >> 5] |= bit;
        else s->switch_bits[a->group2 >> 5] &= ~bit;
    }
    s->action_ticks[event->trigger] = duration;
    return SC64_Z6_FAULT_NONE;
}

/* One 30Hz mission tick. User pause freezes the scheduler; every source
 * condition/action is resolved through required callbacks and failures make
 * the state unsaveable. Return 1 updated, 0 paused/terminal, -fault on error. */
static inline int sc64_z6_adapter_tick_policy(Sc64Z6RuntimeState *s,
                                               const Sc64Z6WorldOps *ops,
                                               int user_paused,
                                               int prune_started)
{
    Sc64Z6Input in = {{0},0,0};
    Sc64Z6Output out;
    if (!s || !sc64_z6_runtime_state_valid(s)) {
        if (s) s->fault = SC64_Z6_FAULT_STATE;
        return -SC64_Z6_FAULT_STATE;
    }
    if (user_paused) return 0;
    if (s->model.outcome != SC64_Z6_RUNNING) return 0;
    if (!ops || !ops->condition || !ops->action ||
        !sc64_z6_runtime_metadata_valid()) {
        s->fault = ops ? SC64_Z6_FAULT_METADATA : SC64_Z6_FAULT_ACTION;
        return -s->fault;
    }
    for (unsigned t = 0; t < SC64_Z6_TRIGGER_COUNT; ++t) {
        const Sc64Z6SourceTrigger *tr = &sc64_z6_source_triggers[t];
        if (prune_started && (s->model.started & ((u16)1u << t)) &&
            sc64_z6_trigger_one_shot(t)) continue;
        for (unsigned c = 0; c < tr->condition_count; ++c) {
            const Sc64Z6SourceCondition *row;
            int result;
            if (!sc64_z6_condition_row(t, c, &row)) {
                s->fault = SC64_Z6_FAULT_METADATA;
                return -SC64_Z6_FAULT_METADATA;
            }
            result = ops->condition(ops->context, s, row);
            if (result < 0 || result > 1) {
                s->fault = SC64_Z6_FAULT_CONDITION;
                return -SC64_Z6_FAULT_CONDITION;
            }
            if (result) in.condition_true[t] |= (u16)1u << c;
        }
    }
    for (unsigned t = 0; t < SC64_Z6_TRIGGER_COUNT; ++t) {
        if (!s->model.action_cursor[t]) {
            if (s->action_ticks[t]) {
                s->fault = SC64_Z6_FAULT_STATE;
                return -SC64_Z6_FAULT_STATE;
            }
            continue;
        }
        if (!s->action_ticks[t]) {
            s->fault = SC64_Z6_FAULT_STATE;
            return -SC64_Z6_FAULT_STATE;
        }
        --s->action_ticks[t];
        if (!s->action_ticks[t]) in.action_complete |= (u32)1u << t;
    }
    out = sc64_z6_step(&s->model, &in);
    for (u8 i = 0; i < out.action_count; ++i) {
        int fault = sc64_z6_dispatch_action(s, ops, &out.actions[i]);
        if (fault) {
            s->fault = (u8)fault;
            return -fault;
        }
    }
    if (!sc64_z6_runtime_state_valid(s)) {
        s->fault = SC64_Z6_FAULT_STATE;
        return -SC64_Z6_FAULT_STATE;
    }
    return 1;
}

static inline int sc64_z6_adapter_tick(Sc64Z6RuntimeState *s,
                                        const Sc64Z6WorldOps *ops,
                                        int user_paused)
{
#ifdef SC64_Z6_PRUNE_STARTED
    return sc64_z6_adapter_tick_policy(s,ops,user_paused,1);
#else
    return sc64_z6_adapter_tick_policy(s,ops,user_paused,0);
#endif
}

#endif
