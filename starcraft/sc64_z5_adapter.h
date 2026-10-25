#ifndef SC64_Z5_ADAPTER_H
#define SC64_Z5_ADAPTER_H

#include "sc64_z5.h"
#include "sc64_z5_scenario_generated.h"

enum {
    SC64_Z5_TICKS_PER_SECOND = 30,
    SC64_Z5_PRESENTATION_HOLD_TICKS = 90,
    SC64_Z5_FAULT_NONE = 0,
    SC64_Z5_FAULT_STATE = 1,
    SC64_Z5_FAULT_CONDITION = 2,
    SC64_Z5_FAULT_ACTION = 3,
    SC64_Z5_FAULT_SWITCH = 4,
    SC64_Z5_FAULT_PAUSE = 5,
    SC64_Z5_FAULT_METADATA = 6
};

_Static_assert(SC64_Z5_SOURCE_GLOBAL_ID == SC64_Z5_GLOBAL_ID,
               "Z5 staged source global ID mismatch");
_Static_assert(SC64_Z5_SOURCE_MAP_COLS == SC64_Z5_MAP_COLS &&
               SC64_Z5_SOURCE_MAP_ROWS == SC64_Z5_MAP_ROWS,
               "Z5 staged map dimensions mismatch");
_Static_assert(SC64_Z5_SOURCE_TRIGGER_COUNT == SC64_Z5_TRIGGER_COUNT,
               "Z5 staged trigger count mismatch");
_Static_assert(SC64_Z5_SOURCE_ACTION_COUNT == SC64_Z5_ACTION_TOTAL,
               "Z5 staged action count mismatch");
_Static_assert(SC64_Z5_SOURCE_CONDITION_COUNT == SC64_Z5_CONDITION_TOTAL,
               "Z5 staged condition count mismatch");

typedef struct {
    Sc64Z5State model;
    u16 action_ticks[SC64_Z5_TRIGGER_COUNT];
    u32 switch_bits[8];        /* All 256 CHK switches are distinct. */
    u8 source_paused;           /* World freeze requested by source PauseGame. */
    u8 fault;                  /* Any nonzero fault makes state unsaveable. */
} Sc64Z5RuntimeState;

_Static_assert(sizeof(Sc64Z5RuntimeState) <= 172, "Z5 runtime state bound");

typedef struct {
    void *context;
    /* Return 1 for true, 0 for false, or a negative value on resolution error.
     * This callback must resolve the numeric source selector and owner rules,
     * unit type, location geometry, comparison, and switch operands. The
     * runtime passes the private authenticated row unchanged. */
    int (*condition)(void *context, const Sc64Z5RuntimeState *state,
                     const Sc64Z5SourceCondition *condition);
    /* Return exactly 1 on success. The adapter passes the exact private row,
     * retaining every numeric operand for create/move/remove/doodad operations. */
    int (*action)(void *context, const Sc64Z5SourceAction *action);
    /* Required when source PauseGame/UnpauseGame is dispatched. */
    int (*world_pause)(void *context, int paused);
} Sc64Z5WorldOps;

static inline unsigned sc64_z5_ceil_ticks(u32 milliseconds)
{
    /* Clamp before multiplying so hostile/corrupt operands cannot wrap. */
    u32 ticks;
    if (milliseconds > 1200000u) milliseconds = 1200000u;
    ticks = (milliseconds * SC64_Z5_TICKS_PER_SECOND + 999u) / 1000u;
    if (!ticks) ticks = 1;
    return ticks > 36000u ? 36000u : (unsigned)ticks;
}

static inline int sc64_z5_source_action(unsigned trigger, unsigned index,
                                        const Sc64Z5SourceAction **out)
{
    const Sc64Z5SourceTrigger *t;
    const Sc64Z5SourceAction *a;
    if (!out || trigger >= SC64_Z5_TRIGGER_COUNT) return 0;
    t = &sc64_z5_source_triggers[trigger];
    if (index >= t->action_count || t->first_action + index >= SC64_Z5_SOURCE_ACTION_COUNT)
        return 0;
    a = &sc64_z5_source_actions[t->first_action + index];
    if (a->trigger != trigger || a->index != index || !a->opcode) return 0;
    *out = a;
    return 1;
}

static inline u16 sc64_z5_action_duration(unsigned trigger, unsigned index)
{
    const Sc64Z5SourceAction *a;
    if (!sc64_z5_source_action(trigger, index, &a)) return 0;
    switch (a->opcode) {
    case 4: return (u16)sc64_z5_ceil_ticks(a->time);  /* Wait */
    case 7: case 9: return SC64_Z5_PRESENTATION_HOLD_TICKS;
    case 8: return (u16)sc64_z5_ceil_ticks(a->time);  /* WAV duration */
    case 1: case 2: case 3: case 5: case 6: case 10: case 11:
    case 12: case 13: case 25: case 28: case 30: case 31:
    case 39: case 42: return 1;
    default: return 0;
    }
}

static inline int sc64_z5_runtime_state_valid(const Sc64Z5RuntimeState *s)
{
    if (!s || !sc64_z5_state_valid(&s->model) || s->fault || s->source_paused > 1)
        return 0;
    if ((s->switch_bits[0] & (u32)~0x17u) || s->switch_bits[1] ||
        s->switch_bits[2] || (s->switch_bits[3] & ~((u32)1u << 14)) ||
        s->switch_bits[4] || s->switch_bits[5] || s->switch_bits[6] ||
        s->switch_bits[7]) return 0;
    for (unsigned i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i) {
        u8 cursor = s->model.action_cursor[i];
        u16 ticks = s->action_ticks[i];
        if ((cursor != 0) != (ticks != 0)) return 0;
        if (ticks) {
            u16 limit = sc64_z5_action_duration(i, cursor - 1u);
            if (!limit || ticks > limit) return 0;
        }
    }
    /* Source trigger 0 emits its sole PauseGame action at source index 6.
     * Once that action has been emitted, the source world remains paused. */
    {
        int pause_reached = sc64_z5_mask_test(s->model.started_lo,
                                              s->model.started_hi, 0) &&
                            (s->model.action_cursor[0] == 0 ||
                             s->model.action_cursor[0] >= 7);
        if (!!s->source_paused != pause_reached) return 0;
    }
    return 1;
}

static inline u16 sc64_z5_runtime_init(Sc64Z5RuntimeState *s)
{
    u8 *p;
    u32 i;
    if (!s) return SC64_Z5_EVENT_NONE;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    return sc64_z5_init(&s->model);
}

static inline int sc64_z5_switch_is_set(const Sc64Z5RuntimeState *s, unsigned index)
{
    if (!s || index > 255u) return 0;
    return (int)((s->switch_bits[index >> 5] >> (index & 31u)) & 1u);
}

static inline int sc64_z5_condition_row(unsigned trigger, unsigned index,
                                         const Sc64Z5SourceCondition **out)
{
    const Sc64Z5SourceTrigger *t;
    const Sc64Z5SourceCondition *c;
    if (!out || trigger >= SC64_Z5_TRIGGER_COUNT) return 0;
    t = &sc64_z5_source_triggers[trigger];
    if (index >= t->condition_count || t->first_condition + index >=
        SC64_Z5_SOURCE_CONDITION_COUNT) return 0;
    c = &sc64_z5_source_conditions[t->first_condition + index];
    if (c->trigger != trigger || c->index != index) return 0;
    *out = c;
    return 1;
}

static inline int sc64_z5_dispatch_action(Sc64Z5RuntimeState *s,
                                           const Sc64Z5WorldOps *ops,
                                           const Sc64Z5ActionEvent *event)
{
    const Sc64Z5SourceAction *a;
    u16 duration;
    if (!s || !ops || !ops->action || !event ||
        !sc64_z5_source_action(event->trigger, event->action_index, &a))
        return SC64_Z5_FAULT_METADATA;
    duration = sc64_z5_action_duration(event->trigger, event->action_index);
    if (!duration) return SC64_Z5_FAULT_ACTION;
    if (a->opcode == 13 &&
        (a->group1 != 0 || a->group2 > 255u ||
         (a->modifier != 4 && a->modifier != 5))) return SC64_Z5_FAULT_SWITCH;
    if ((a->opcode == 5 || a->opcode == 6) && !ops->world_pause)
        return SC64_Z5_FAULT_PAUSE;
    if (ops->action(ops->context, a) != 1) return SC64_Z5_FAULT_ACTION;
    if (a->opcode == 13) {
        u32 bit = (u32)1u << (a->group2 & 31u);
        if (a->modifier == 4) s->switch_bits[a->group2 >> 5] |= bit;
        else s->switch_bits[a->group2 >> 5] &= ~bit;
    } else if (a->opcode == 5 || a->opcode == 6) {
        int pause = a->opcode == 5;
        if (ops->world_pause(ops->context, pause) != 1) return SC64_Z5_FAULT_PAUSE;
        s->source_paused = (u8)pause;
    }
    s->action_ticks[event->trigger] = duration;
    return SC64_Z5_FAULT_NONE;
}

/* One mission update. `user_paused` freezes all adapter timers and callbacks.
 * The source PauseGame flag is forwarded to the world but does not stop this
 * function, so the authored trigger sequence and mission clock continue.
 * Return 1 if updated, 0 if user-paused/terminal, or a negative fault code. */
static inline int sc64_z5_adapter_tick(Sc64Z5RuntimeState *s,
                                       const Sc64Z5WorldOps *ops,
                                       int user_paused)
{
    Sc64Z5Input in = {{0}, 0, 0, 0};
    Sc64Z5Output out;
    if (!s || !sc64_z5_runtime_state_valid(s)) {
        if (s) s->fault = SC64_Z5_FAULT_STATE;
        return -SC64_Z5_FAULT_STATE;
    }
    if (user_paused) return 0;
    /* A terminal source result freezes any other outstanding action programs. */
    if (s->model.outcome != SC64_Z5_RUNNING) return 0;
    if (!ops || !ops->condition || !ops->action) {
        s->fault = SC64_Z5_FAULT_ACTION;
        return -SC64_Z5_FAULT_ACTION;
    }
    for (unsigned t = 0; t < SC64_Z5_TRIGGER_COUNT; ++t) {
        const Sc64Z5SourceTrigger *tr = &sc64_z5_source_triggers[t];
        if (tr->action_count != sc64_z5_action_count((u8)t) ||
            tr->condition_count != sc64_z5_condition_count((u8)t)) {
            s->fault = SC64_Z5_FAULT_METADATA;
            return -SC64_Z5_FAULT_METADATA;
        }
        for (unsigned c = 0; c < tr->condition_count; ++c) {
            const Sc64Z5SourceCondition *row;
            int result;
            if (!sc64_z5_condition_row(t, c, &row)) {
                s->fault = SC64_Z5_FAULT_METADATA;
                return -SC64_Z5_FAULT_METADATA;
            }
#ifdef SC64_Z5_PRUNE_STARTED
            /* Non-preserved Z5 records are one-shot. Once started, their
             * predicate can no longer affect scheduling; still validate the
             * complete trigger and condition metadata above on every tick. */
            if (!sc64_z5_trigger_preserved((u8)t) &&
                sc64_z5_mask_test(s->model.started_lo, s->model.started_hi,
                                  (u8)t))
                continue;
#endif
            result = ops->condition(ops->context, s, row);
            if (result < 0 || result > 1) {
                s->fault = SC64_Z5_FAULT_CONDITION;
                return -SC64_Z5_FAULT_CONDITION;
            }
            if (result) in.condition_true[t] |= (u16)1u << c;
        }
    }
    for (unsigned t = 0; t < SC64_Z5_TRIGGER_COUNT; ++t) {
        if (!s->model.action_cursor[t]) {
            if (s->action_ticks[t]) {
                s->fault = SC64_Z5_FAULT_STATE;
                return -SC64_Z5_FAULT_STATE;
            }
            continue;
        }
        if (!s->action_ticks[t]) {
            s->fault = SC64_Z5_FAULT_STATE;
            return -SC64_Z5_FAULT_STATE;
        }
        --s->action_ticks[t];
        if (!s->action_ticks[t])
            sc64_z5_mask_set(&in.action_complete_lo, &in.action_complete_hi, (u8)t);
    }
    out = sc64_z5_step(&s->model, &in);
    for (u8 i = 0; i < out.action_count; ++i) {
        int fault = sc64_z5_dispatch_action(s, ops, &out.actions[i]);
        if (fault) {
            s->fault = (u8)fault;
            return -fault;
        }
    }
    if (!sc64_z5_runtime_state_valid(s)) {
        s->fault = SC64_Z5_FAULT_STATE;
        return -SC64_Z5_FAULT_STATE;
    }
    return 1;
}

#endif
