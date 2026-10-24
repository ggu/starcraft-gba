#ifndef SC64_P5_ADAPTER_H
#define SC64_P5_ADAPTER_H

#include "sc64_p5.h"
#include "sc64_p5_scenario_generated.h"

enum {
    SC64_P5_TICKS_PER_SECOND = 30,
    SC64_P5_PRESENTATION_HOLD_TICKS = 90,
    SC64_P5_FAULT_NONE = 0,
    SC64_P5_FAULT_STATE = 1,
    SC64_P5_FAULT_CONDITION = 2,
    SC64_P5_FAULT_ACTION = 3,
    SC64_P5_FAULT_SWITCH = 4,
    SC64_P5_FAULT_AI = 5,
    SC64_P5_FAULT_METADATA = 6
};

_Static_assert(SC64_P5_SOURCE_GLOBAL_ID == SC64_P5_GLOBAL_ID,
               "P5 staged source global ID mismatch");
_Static_assert(SC64_P5_SOURCE_MAP_COLS == SC64_P5_MAP_COLS &&
               SC64_P5_SOURCE_MAP_ROWS == SC64_P5_MAP_ROWS,
               "P5 staged map dimensions mismatch");
_Static_assert(SC64_P5_SOURCE_TRIGGER_COUNT == SC64_P5_TRIGGER_COUNT,
               "P5 staged trigger count mismatch");
_Static_assert(SC64_P5_SOURCE_ACTION_COUNT == SC64_P5_ACTION_TOTAL,
               "P5 staged action count mismatch");
_Static_assert(SC64_P5_SOURCE_CONDITION_COUNT == SC64_P5_CONDITION_TOTAL,
               "P5 staged condition count mismatch");

typedef struct {
    Sc64P5State model;
    u16 action_ticks[SC64_P5_TRIGGER_COUNT];
    u32 switch_bits[8];
    u8 fault;
} Sc64P5RuntimeState;

_Static_assert(sizeof(Sc64P5RuntimeState) <= 72, "P5 runtime state bound");

typedef struct {
    void *context;
    /* Resolve exact numeric source predicates against the current world. */
    int (*condition)(void *context, const Sc64P5RuntimeState *state,
                     const Sc64P5SourceCondition *condition);
    /* Apply one exact source action row. Return exactly 1 on success. */
    int (*action)(void *context, const Sc64P5SourceAction *action);
    /* P5 RunAIScript and RunAIScriptAtLocation are explicitly opaque here.
     * The host adapter must supply a bounded policy for the exact numeric row;
     * this callback is not a claim to implement native Brood War AI scripts. */
    int (*opaque_ai)(void *context, const Sc64P5SourceAction *action);
} Sc64P5WorldOps;

static inline unsigned sc64_p5_ceil_ticks(u32 milliseconds)
{
    u32 ticks;
    if (milliseconds > 1200000u) milliseconds = 1200000u;
    ticks = (milliseconds * SC64_P5_TICKS_PER_SECOND + 999u) / 1000u;
    if (!ticks) ticks = 1;
    return ticks > 36000u ? 36000u : (unsigned)ticks;
}

static inline int sc64_p5_source_action(unsigned trigger, unsigned index,
                                        const Sc64P5SourceAction **out)
{
    const Sc64P5SourceTrigger *t;
    const Sc64P5SourceAction *a;
    if (!out || trigger >= SC64_P5_TRIGGER_COUNT) return 0;
    t = &sc64_p5_source_triggers[trigger];
    if (index >= t->action_count || t->first_action + index >= SC64_P5_SOURCE_ACTION_COUNT)
        return 0;
    a = &sc64_p5_source_actions[t->first_action + index];
    if (a->trigger != trigger || a->index != index || !a->opcode) return 0;
    *out = a;
    return 1;
}

static inline int sc64_p5_condition_row(unsigned trigger, unsigned index,
                                         const Sc64P5SourceCondition **out)
{
    const Sc64P5SourceTrigger *t;
    const Sc64P5SourceCondition *c;
    if (!out || trigger >= SC64_P5_TRIGGER_COUNT) return 0;
    t = &sc64_p5_source_triggers[trigger];
    if (index >= t->condition_count || t->first_condition + index >=
        SC64_P5_SOURCE_CONDITION_COUNT) return 0;
    c = &sc64_p5_source_conditions[t->first_condition + index];
    if (c->trigger != trigger || c->index != index) return 0;
    *out = c;
    return 1;
}

static inline u16 sc64_p5_action_duration(unsigned trigger, unsigned index)
{
    const Sc64P5SourceAction *a;
    if (!sc64_p5_source_action(trigger, index, &a)) return 0;
    switch (a->opcode) {
    case 4: return (u16)sc64_p5_ceil_ticks(a->time); /* Wait */
    case 7: case 9: return SC64_P5_PRESENTATION_HOLD_TICKS;
    case 8: return (u16)sc64_p5_ceil_ticks(a->time); /* WAV duration */
    case 1: case 2: case 10: case 11: case 12: case 13:
    case 15: case 16: case 22: case 26: case 43: return 1;
    default: return 0;
    }
}

static inline int sc64_p5_switch_is_set(const Sc64P5RuntimeState *s,
                                         unsigned index)
{
    if (!s || index > 255u) return 0;
    return (int)((s->switch_bits[index >> 5] >> (index & 31u)) & 1u);
}

static inline int sc64_p5_runtime_state_valid(const Sc64P5RuntimeState *s)
{
    if (!s || !sc64_p5_state_valid(&s->model) || s->fault) return 0;
    /* The staged P5 source only writes switches 0 and 4. */
    if ((s->switch_bits[0] & ~((u32)1u | ((u32)1u << 4))) ||
        s->switch_bits[1] || s->switch_bits[2] || s->switch_bits[3] ||
        s->switch_bits[4] || s->switch_bits[5] || s->switch_bits[6] ||
        s->switch_bits[7]) return 0;
    for (unsigned i = 0; i < SC64_P5_TRIGGER_COUNT; ++i) {
        u8 cursor = s->model.action_cursor[i];
        u16 ticks = s->action_ticks[i];
        if ((cursor != 0) != (ticks != 0)) return 0;
        if (ticks) {
            u16 limit = sc64_p5_action_duration(i, cursor - 1u);
            if (!limit || ticks > limit) return 0;
        }
    }
    return 1;
}

static inline u16 sc64_p5_runtime_init(Sc64P5RuntimeState *s)
{
    u8 *p;
    u32 i;
    if (!s) return SC64_P5_EVENT_NONE;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    return sc64_p5_init(&s->model);
}

static inline int sc64_p5_dispatch_action(Sc64P5RuntimeState *s,
                                           const Sc64P5WorldOps *ops,
                                           const Sc64P5ActionEvent *event)
{
    const Sc64P5SourceAction *a;
    u16 duration;
    if (!s || !ops || !event ||
        !sc64_p5_source_action(event->trigger, event->action_index, &a))
        return SC64_P5_FAULT_METADATA;
    duration = sc64_p5_action_duration(event->trigger, event->action_index);
    if (!duration) return SC64_P5_FAULT_ACTION;
    if (a->opcode == 13 &&
        (a->group1 != 0 || a->group2 > 255u ||
         (a->modifier != 4 && a->modifier != 5) ||
         (a->group2 != 0 && a->group2 != 4))) return SC64_P5_FAULT_SWITCH;
    if (a->opcode == 15 || a->opcode == 16) {
        if (!ops->opaque_ai || ops->opaque_ai(ops->context, a) != 1)
            return SC64_P5_FAULT_AI;
    } else {
        if (!ops->action || ops->action(ops->context, a) != 1)
            return SC64_P5_FAULT_ACTION;
    }
    if (a->opcode == 13) {
        u32 bit = (u32)1u << a->group2;
        if (a->modifier == 4) s->switch_bits[0] |= bit;
        else s->switch_bits[0] &= ~bit;
    }
    s->action_ticks[event->trigger] = duration;
    return SC64_P5_FAULT_NONE;
}

/* Returns 1 when advanced, 0 for user pause/terminal, or a negative fault. */
static inline int sc64_p5_adapter_tick(Sc64P5RuntimeState *s,
                                       const Sc64P5WorldOps *ops,
                                       int user_paused)
{
    Sc64P5Input in = {{0}, 0, 0};
    Sc64P5Output out;
    if (!s || !sc64_p5_runtime_state_valid(s)) {
        if (s) s->fault = SC64_P5_FAULT_STATE;
        return -SC64_P5_FAULT_STATE;
    }
    if (user_paused || s->model.outcome != SC64_P5_RUNNING) return 0;
    if (!ops || !ops->condition) {
        s->fault = SC64_P5_FAULT_CONDITION;
        return -SC64_P5_FAULT_CONDITION;
    }
    for (unsigned t = 0; t < SC64_P5_TRIGGER_COUNT; ++t) {
        const Sc64P5SourceTrigger *tr = &sc64_p5_source_triggers[t];
        if (tr->action_count != sc64_p5_action_count((u8)t) ||
            tr->condition_count != sc64_p5_condition_count((u8)t)) {
            s->fault = SC64_P5_FAULT_METADATA;
            return -SC64_P5_FAULT_METADATA;
        }
        for (unsigned c = 0; c < tr->condition_count; ++c) {
            const Sc64P5SourceCondition *row;
            int result;
            if (!sc64_p5_condition_row(t, c, &row)) {
                s->fault = SC64_P5_FAULT_METADATA;
                return -SC64_P5_FAULT_METADATA;
            }
            result = ops->condition(ops->context, s, row);
            if (result < 0 || result > 1) {
                s->fault = SC64_P5_FAULT_CONDITION;
                return -SC64_P5_FAULT_CONDITION;
            }
            if (result) in.condition_true[t] |= (u8)1u << c;
        }
    }
    for (unsigned t = 0; t < SC64_P5_TRIGGER_COUNT; ++t) {
        if (!s->model.action_cursor[t]) {
            if (s->action_ticks[t]) {
                s->fault = SC64_P5_FAULT_STATE;
                return -SC64_P5_FAULT_STATE;
            }
            continue;
        }
        if (!s->action_ticks[t]) {
            s->fault = SC64_P5_FAULT_STATE;
            return -SC64_P5_FAULT_STATE;
        }
        --s->action_ticks[t];
        if (!s->action_ticks[t]) in.action_complete |= (u16)1u << t;
    }
    out = sc64_p5_step(&s->model, &in);
    for (u8 i = 0; i < out.action_count; ++i) {
        int fault = sc64_p5_dispatch_action(s, ops, &out.actions[i]);
        if (fault) {
            s->fault = (u8)fault;
            return -fault;
        }
    }
    if (!sc64_p5_runtime_state_valid(s)) {
        s->fault = SC64_P5_FAULT_STATE;
        return -SC64_P5_FAULT_STATE;
    }
    return 1;
}

#endif
