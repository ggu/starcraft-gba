#ifndef SC64_P7_ADAPTER_H
#define SC64_P7_ADAPTER_H

#include "sc64_p7.h"
#include "sc64_p7_scenario_generated.h"

enum {
    SC64_P7_TICKS_PER_SECOND = 30,
    SC64_P7_PRESENTATION_HOLD_TICKS = 90,
    SC64_P7_FAULT_NONE = 0,
    SC64_P7_FAULT_STATE = 1,
    SC64_P7_FAULT_CONDITION = 2,
    SC64_P7_FAULT_ACTION = 3,
    SC64_P7_FAULT_AI = 4,
    SC64_P7_FAULT_METADATA = 5,
    SC64_P7_FAULT_SWITCH = 6,
    SC64_P7_CONDITION_COMMAND = 2,
    SC64_P7_CONDITION_SWITCH = 11,
    SC64_P7_CONDITION_ELAPSED_TIME = 12,
    SC64_P7_ACTION_VICTORY = 1,
    SC64_P7_ACTION_DEFEAT = 2,
    SC64_P7_ACTION_WAIT = 4,
    SC64_P7_ACTION_TRANSMISSION = 7,
    SC64_P7_ACTION_PLAY_WAV = 8,
    SC64_P7_ACTION_DISPLAY_TEXT = 9,
    SC64_P7_ACTION_CENTER_VIEW = 10,
    SC64_P7_ACTION_CREATE_UNIT = 11,
    SC64_P7_ACTION_OBJECTIVES = 12,
    SC64_P7_ACTION_SET_SWITCH = 13,
    SC64_P7_ACTION_RUN_AI_AT_LOCATION = 16,
    SC64_P7_ACTION_SET_RESOURCES = 26,
    SC64_P7_ACTION_MOVE_LOCATION = 38,
    SC64_P7_SWITCH_MASK = 1u
};

_Static_assert(SC64_P7_SOURCE_GLOBAL_ID == SC64_P7_GLOBAL_ID,
               "P7 staged source ID mismatch");
_Static_assert(SC64_P7_SOURCE_MAP_COLS == SC64_P7_MAP_COLS &&
               SC64_P7_SOURCE_MAP_ROWS == SC64_P7_MAP_ROWS,
               "P7 staged map dimensions mismatch");
_Static_assert(SC64_P7_SOURCE_TRIGGER_COUNT == SC64_P7_TRIGGER_COUNT,
               "P7 staged trigger count mismatch");
_Static_assert(SC64_P7_SOURCE_ACTION_COUNT == SC64_P7_ACTION_TOTAL,
               "P7 staged action count mismatch");
_Static_assert(SC64_P7_SOURCE_CONDITION_COUNT == SC64_P7_CONDITION_TOTAL,
               "P7 staged condition count mismatch");

typedef struct {
    Sc64P7State model;
    u16 action_ticks[SC64_P7_TRIGGER_COUNT];
    u32 switch_bits; /* The authenticated source uses only switch 0. */
    u8 fault;
} Sc64P7RuntimeState;

_Static_assert(sizeof(Sc64P7RuntimeState) <= 48, "P7 runtime state bound");

typedef struct {
    void *context;
    u32 elapsed_frames;
    /* Resolve source Command rows against the mission world. Return 1/0 or
     * a negative value on an unknown owner, selector, or actor mapping. */
    int (*condition)(void *context, const Sc64P7RuntimeState *state,
                     const Sc64P7SourceCondition *condition);
    /* Apply non-AI action effects exactly or return failure. */
    int (*action)(void *context, const Sc64P7SourceAction *action);
    /* RunAIScriptAtLocation is opaque. A bounded authored policy must opt in. */
    int (*opaque_ai)(void *context, const Sc64P7SourceAction *action);
} Sc64P7WorldOps;

static inline u16 sc64_p7_ceil_ticks(u32 milliseconds)
{
    if (milliseconds > 1200000u) milliseconds = 1200000u;
    u32 ticks = (milliseconds * SC64_P7_TICKS_PER_SECOND + 999u) / 1000u;
    if (!ticks) ticks = 1;
    return (u16)(ticks > 36000u ? 36000u : ticks);
}

static inline int sc64_p7_source_action(unsigned trigger, unsigned index,
                                         const Sc64P7SourceAction **out)
{
    const Sc64P7SourceTrigger *t;
    const Sc64P7SourceAction *a;
    if (!out || trigger >= SC64_P7_TRIGGER_COUNT) return 0;
    t = &sc64_p7_source_triggers[trigger];
    if (index >= t->action_count || t->first_action + index >=
        SC64_P7_SOURCE_ACTION_COUNT) return 0;
    a = &sc64_p7_source_actions[t->first_action + index];
    if (a->trigger != trigger || a->index != index || !a->opcode) return 0;
    *out = a;
    return 1;
}

static inline int sc64_p7_condition_row(unsigned trigger, unsigned index,
                                         const Sc64P7SourceCondition **out)
{
    const Sc64P7SourceTrigger *t;
    const Sc64P7SourceCondition *c;
    if (!out || trigger >= SC64_P7_TRIGGER_COUNT) return 0;
    t = &sc64_p7_source_triggers[trigger];
    if (index >= t->condition_count || t->first_condition + index >=
        SC64_P7_SOURCE_CONDITION_COUNT) return 0;
    c = &sc64_p7_source_conditions[t->first_condition + index];
    if (c->trigger != trigger || c->index != index) return 0;
    *out = c;
    return 1;
}

static inline int sc64_p7_trigger_enabled(unsigned trigger, unsigned executor)
{
    if (trigger >= SC64_P7_TRIGGER_COUNT || executor >= 27u) return 0;
    return sc64_p7_source_triggers[trigger].enabled_players[executor] == 1;
}

static inline int sc64_p7_location_source(unsigned id, unsigned width,
                                           unsigned height, unsigned *left,
                                           unsigned *top, unsigned *right,
                                           unsigned *bottom)
{
    const Sc64P7SourceLocation *loc;
    if (!width || !height || width > SC64_P7_MAP_COLS ||
        height > SC64_P7_MAP_ROWS || id == 0 ||
        id > SC64_P7_SOURCE_LOCATION_COUNT || !left || !top || !right || !bottom)
        return 0;
    /* The final CHK location is the special Anywhere region. */
    if (id == SC64_P7_SOURCE_LOCATION_COUNT) {
        *left = *top = 0;
        *right = width * 32u;
        *bottom = height * 32u;
        return 1;
    }
    loc = &sc64_p7_source_locations[id-1u];
    *left=loc->left; *top=loc->top; *right=loc->right; *bottom=loc->bottom;
    return *left <= *right && *top <= *bottom &&
           *right <= width * 32u && *bottom <= height * 32u;
}

static inline int sc64_p7_location_runtime(unsigned id, unsigned width,
                                            unsigned height, int *left,
                                            int *top, int *right, int *bottom)
{
    unsigned l,t,r,b;
    if (!left || !top || !right || !bottom ||
        !sc64_p7_location_source(id,width,height,&l,&t,&r,&b)) return 0;
    *left=(int)(l/2u); *top=(int)(t/2u);
    *right=(int)(r/2u); *bottom=(int)(b/2u);
    return 1;
}

static inline u8 sc64_p7_expected_opcode(unsigned trigger, unsigned index)
{
    if (trigger == 0) return index == 0 ? 2 : 0;
    if (trigger >= 1 && trigger <= 3) {
        static const u8 rows[5] = {4,8,9,4,2};
        return index < 5 ? rows[index] : 0;
    }
    if (trigger == 4) {
        static const u8 rows[4] = {12,26,7,7};
        return index < 4 ? rows[index] : 0;
    }
    if (trigger == 5) {
        static const u8 rows[17] = {4,38,10,7,7,4,8,4,11,11,11,4,7,7,7,4,1};
        return index < 17 ? rows[index] : 0;
    }
    if (trigger == 6) {
        static const u8 rows[3] = {26,16,16};
        return index < 3 ? rows[index] : 0;
    }
    if (trigger == 7) {
        static const u8 rows[2] = {26,16};
        return index < 2 ? rows[index] : 0;
    }
    if (trigger == 8 || trigger == 9) return index == 0 ? 13 : 0;
    return 0;
}

static inline int sc64_p7_runtime_metadata_valid(void)
{
    static const u8 executor[SC64_P7_TRIGGER_COUNT] = {20,20,20,20,20,20,19,18,20,20};
    unsigned actions = 0, conditions = 0;
    for (unsigned id=1; id<=SC64_P7_SOURCE_LOCATION_COUNT; ++id) {
        unsigned l,t,r,b;
        if (!sc64_p7_location_source(id,SC64_P7_MAP_COLS,SC64_P7_MAP_ROWS,
                                    &l,&t,&r,&b)) return 0;
    }
    for (unsigned t = 0; t < SC64_P7_TRIGGER_COUNT; ++t) {
        const Sc64P7SourceTrigger *tr = &sc64_p7_source_triggers[t];
        if (tr->action_count != sc64_p7_action_count((u8)t) ||
            tr->condition_count != 1 || tr->execution_flags || tr->initial_cursor ||
            tr->first_action + tr->action_count > SC64_P7_SOURCE_ACTION_COUNT ||
            tr->first_condition + tr->condition_count > SC64_P7_SOURCE_CONDITION_COUNT)
            return 0;
        for (unsigned p = 0; p < 27; ++p)
            if (tr->enabled_players[p] != (p == executor[t] ? 1u : 0u)) return 0;
        actions += tr->action_count;
        conditions += tr->condition_count;
        for (unsigned aidx = 0; aidx < tr->action_count; ++aidx) {
            const Sc64P7SourceAction *a;
            u8 expected = sc64_p7_expected_opcode(t,aidx);
            if (!expected || !sc64_p7_source_action(t,aidx,&a) || a->opcode != expected)
                return 0;
            if (a->location && !sc64_p7_location_source(a->location,
                    SC64_P7_MAP_COLS,SC64_P7_MAP_ROWS,
                    &(unsigned){0},&(unsigned){0},&(unsigned){0},&(unsigned){0}))
                return 0;
            if (a->opcode == 3 || a->opcode == 5 || a->opcode == 6) return 0;
            if (a->opcode == SC64_P7_ACTION_SET_SWITCH &&
                (a->group1 || a->group2 != SC64_P7_SWITCH_VICTORY_GATE ||
                 a->modifier != 4 || a->flags)) return 0;
            if ((a->opcode == SC64_P7_ACTION_VICTORY ||
                 a->opcode == SC64_P7_ACTION_DEFEAT) &&
                aidx + 1u != tr->action_count) return 0;
        }
        {
            const Sc64P7SourceCondition *c;
            if (!sc64_p7_condition_row(t,0,&c) || c->location || c->mask) return 0;
            if (t <= 3 || t >= 6) {
                unsigned player = t == 0 ? 13u : t <= 3 ? 17u :
                                  t == 6 ? 19u : t == 7 ? 18u : t == 8 ? 18u : 19u;
                unsigned unit = t == 1 ? 78u : t == 2 ? 79u :
                                t == 3 ? 75u : t >= 8 ? 154u : 229u;
                unsigned amount = t == 6 || t == 7 ? 1u : 0u;
                unsigned cmp = t == 6 || t == 7 ? 0u : 1u;
                if (c->opcode != SC64_P7_CONDITION_COMMAND || c->player != player ||
                    c->unit != unit || c->amount != amount || c->comparison != cmp ||
                    c->resource || c->flags != 0x10u) return 0;
            } else if (t == 4) {
                /* The authenticated row is ElapsedTime >= 0 seconds. */
                if (c->opcode != SC64_P7_CONDITION_ELAPSED_TIME || c->player ||
                    c->amount || c->unit || c->comparison || c->resource || c->flags)
                    return 0;
            } else {
                if (c->opcode != SC64_P7_CONDITION_SWITCH || c->player ||
                    c->amount || c->unit || c->comparison != 2 ||
                    c->resource != SC64_P7_SWITCH_VICTORY_GATE || c->flags) return 0;
            }
        }
    }
    return actions == SC64_P7_ACTION_TOTAL && conditions == SC64_P7_CONDITION_TOTAL;
}

static inline u16 sc64_p7_action_duration(unsigned trigger, unsigned index)
{
    const Sc64P7SourceAction *a;
    if (!sc64_p7_source_action(trigger,index,&a)) return 0;
    switch (a->opcode) {
    case SC64_P7_ACTION_WAIT:
    case SC64_P7_ACTION_PLAY_WAV: return sc64_p7_ceil_ticks(a->time);
    case SC64_P7_ACTION_TRANSMISSION:
    case SC64_P7_ACTION_DISPLAY_TEXT: return SC64_P7_PRESENTATION_HOLD_TICKS;
    case SC64_P7_ACTION_VICTORY: case SC64_P7_ACTION_DEFEAT:
    case SC64_P7_ACTION_CENTER_VIEW: case SC64_P7_ACTION_CREATE_UNIT:
    case SC64_P7_ACTION_OBJECTIVES: case SC64_P7_ACTION_SET_SWITCH:
    case SC64_P7_ACTION_SET_RESOURCES: case SC64_P7_ACTION_MOVE_LOCATION:
    case SC64_P7_ACTION_RUN_AI_AT_LOCATION: return 1;
    default: return 0;
    }
}

static inline int sc64_p7_runtime_state_valid(const Sc64P7RuntimeState *s)
{
    if (!s || !sc64_p7_runtime_metadata_valid() ||
        !sc64_p7_state_valid(&s->model) || s->fault ||
        (s->switch_bits & ~SC64_P7_SWITCH_MASK)) return 0;
    for (unsigned i = 0; i < SC64_P7_TRIGGER_COUNT; ++i) {
        u8 cursor = s->model.action_cursor[i];
        u16 ticks = s->action_ticks[i];
        if ((cursor != 0) != (ticks != 0)) return 0;
        if (ticks && (!cursor || ticks > sc64_p7_action_duration(i,cursor-1u))) return 0;
    }
    return 1;
}

/* Private campaign fast path: immutable source rows are authenticated by
 * mission preflight/load. Keep every mutable cursor, timer and flag check. */
#ifdef SC64_P7_PREVALIDATED_TICK
static inline int sc64_p7_runtime_state_valid_prevalidated(
    const Sc64P7RuntimeState *s)
{
    if (!s || !sc64_p7_state_valid(&s->model) || s->fault ||
        (s->switch_bits & ~SC64_P7_SWITCH_MASK)) return 0;
    for (unsigned i=0;i<SC64_P7_TRIGGER_COUNT;++i) {
        u8 cursor=s->model.action_cursor[i];
        u16 ticks=s->action_ticks[i];
        if ((cursor!=0)!=(ticks!=0)) return 0;
        if (ticks && (!cursor || ticks>sc64_p7_action_duration(i,cursor-1u)))
            return 0;
    }
    return 1;
}
#endif

static inline void sc64_p7_runtime_init(Sc64P7RuntimeState *s)
{
    if (!s) return;
    sc64_p7_init(&s->model);
    for (unsigned i = 0; i < SC64_P7_TRIGGER_COUNT; ++i) s->action_ticks[i] = 0;
    s->switch_bits = 0;
    s->fault = SC64_P7_FAULT_NONE;
}

static inline int sc64_p7_condition_eval(const Sc64P7RuntimeState *s,
                                          const Sc64P7WorldOps *ops,
                                          const Sc64P7SourceCondition *c)
{
    if (c->opcode == SC64_P7_CONDITION_COMMAND)
        return ops->condition ? ops->condition(ops->context,s,c) : -1;
    if (c->opcode == SC64_P7_CONDITION_SWITCH) {
        unsigned bit;
        if (c->resource >= 32u || c->comparison != 2) return -1;
        bit = (s->switch_bits >> c->resource) & 1u;
        return bit != 0;
    }
    if (c->opcode == SC64_P7_CONDITION_ELAPSED_TIME)
        return ops->elapsed_frames / 30u >= c->amount;
    return -1;
}

static inline int sc64_p7_dispatch_action(Sc64P7RuntimeState *s,
                                           const Sc64P7WorldOps *ops,
                                           const Sc64P7ActionEvent *event)
{
    const Sc64P7SourceAction *a;
    u16 duration;
    int result;
    if (!s || !ops || !event || !sc64_p7_source_action(event->trigger,
                                                        event->action_index,&a))
        return SC64_P7_FAULT_METADATA;
    if (!sc64_p7_runtime_metadata_valid() || !sc64_p7_state_valid(&s->model) ||
        s->fault || (s->switch_bits & ~SC64_P7_SWITCH_MASK) ||
        !sc64_p7_trigger_started(&s->model,event->trigger) ||
        s->model.action_cursor[event->trigger] != (u8)(event->action_index+1u) ||
        s->action_ticks[event->trigger]) return SC64_P7_FAULT_STATE;
    duration = sc64_p7_action_duration(event->trigger,event->action_index);
    if (!duration) return SC64_P7_FAULT_ACTION;
    if (a->opcode == SC64_P7_ACTION_SET_SWITCH &&
        (a->group2 >= 32u || a->modifier != 4)) return SC64_P7_FAULT_SWITCH;
    if (a->opcode == SC64_P7_ACTION_RUN_AI_AT_LOCATION)
        result = ops->opaque_ai ? ops->opaque_ai(ops->context,a) : 0;
    else
        result = ops->action ? ops->action(ops->context,a) : 0;
    if (result != 1) return a->opcode == SC64_P7_ACTION_RUN_AI_AT_LOCATION ?
                             SC64_P7_FAULT_AI : SC64_P7_FAULT_ACTION;
    if (a->opcode == SC64_P7_ACTION_SET_SWITCH)
        s->switch_bits |= (u32)1u << a->group2;
    s->action_ticks[event->trigger] = duration;
    return SC64_P7_FAULT_NONE;
}

#ifdef SC64_P7_PREVALIDATED_TICK
static inline int sc64_p7_dispatch_action_prevalidated(
    Sc64P7RuntimeState *s,const Sc64P7WorldOps *ops,
    const Sc64P7ActionEvent *event)
{
    const Sc64P7SourceAction *a;
    u16 duration;
    int result;
    if (!s || !ops || !event || !sc64_p7_source_action(event->trigger,
                                                        event->action_index,&a))
        return SC64_P7_FAULT_METADATA;
    if (!sc64_p7_state_valid(&s->model) || s->fault ||
        (s->switch_bits&~SC64_P7_SWITCH_MASK) ||
        !sc64_p7_trigger_started(&s->model,event->trigger) ||
        s->model.action_cursor[event->trigger] != (u8)(event->action_index+1u) ||
        s->action_ticks[event->trigger]) return SC64_P7_FAULT_STATE;
    duration=sc64_p7_action_duration(event->trigger,event->action_index);
    if (!duration) return SC64_P7_FAULT_ACTION;
    if (a->opcode==SC64_P7_ACTION_SET_SWITCH &&
        (a->group2>=32u || a->modifier!=4)) return SC64_P7_FAULT_SWITCH;
    if (a->opcode==SC64_P7_ACTION_RUN_AI_AT_LOCATION)
        result=ops->opaque_ai ? ops->opaque_ai(ops->context,a) : 0;
    else result=ops->action ? ops->action(ops->context,a) : 0;
    if (result!=1) return a->opcode==SC64_P7_ACTION_RUN_AI_AT_LOCATION ?
                             SC64_P7_FAULT_AI : SC64_P7_FAULT_ACTION;
    if (a->opcode==SC64_P7_ACTION_SET_SWITCH)
        s->switch_bits |= (u32)1u << a->group2;
    s->action_ticks[event->trigger]=duration;
    return SC64_P7_FAULT_NONE;
}
#endif

/* Return 1 when an adapter tick ran, 0 when UI-paused or terminal, and a
 * negative fault code on invalid metadata, predicate, AI policy, or action. */
static inline int sc64_p7_adapter_tick(Sc64P7RuntimeState *s,
                                        const Sc64P7WorldOps *ops,
                                        int user_paused)
{
    Sc64P7Input in = {{0},0,0};
    Sc64P7Output out;
    if (!s || !sc64_p7_runtime_state_valid(s)) {
        if (s) s->fault = SC64_P7_FAULT_STATE;
        return -SC64_P7_FAULT_STATE;
    }
    if (user_paused || s->model.outcome != SC64_P7_RUNNING) return 0;
    if (!ops || !sc64_p7_runtime_metadata_valid()) {
        s->fault = SC64_P7_FAULT_METADATA;
        return -SC64_P7_FAULT_METADATA;
    }
    for (unsigned t = 0; t < SC64_P7_TRIGGER_COUNT; ++t) {
        const Sc64P7SourceCondition *c;
        int result;
        if (!sc64_p7_condition_row(t,0,&c)) {
            s->fault = SC64_P7_FAULT_METADATA;
            return -SC64_P7_FAULT_METADATA;
        }
        result = sc64_p7_condition_eval(s,ops,c);
        if (result < 0 || result > 1) {
            s->fault = SC64_P7_FAULT_CONDITION;
            return -SC64_P7_FAULT_CONDITION;
        }
        in.condition_true[t] = (u8)result;
    }
    for (unsigned t = 0; t < SC64_P7_TRIGGER_COUNT; ++t) {
        if (!s->model.action_cursor[t]) {
            if (s->action_ticks[t]) {
                s->fault = SC64_P7_FAULT_STATE;
                return -SC64_P7_FAULT_STATE;
            }
            continue;
        }
        if (!s->action_ticks[t]) {
            s->fault = SC64_P7_FAULT_STATE;
            return -SC64_P7_FAULT_STATE;
        }
        --s->action_ticks[t];
        if (!s->action_ticks[t]) in.action_complete |= sc64_p7_trigger_bit(t);
    }
    out = sc64_p7_step(&s->model,&in);
    for (u8 i = 0; i < out.action_count; ++i) {
        int fault = sc64_p7_dispatch_action(s,ops,&out.actions[i]);
        if (fault) {
            s->fault = (u8)fault;
            return -fault;
        }
    }
    if (!sc64_p7_runtime_state_valid(s)) {
        s->fault = SC64_P7_FAULT_STATE;
        return -SC64_P7_FAULT_STATE;
    }
    return 1;
}

#ifdef SC64_P7_PREVALIDATED_TICK
/* Only skips repeated immutable table scans; all mutable runtime, trigger,
 * callback result and action-state checks remain identical to the checked API. */
static int __attribute__((noinline)) sc64_p7_adapter_tick_prevalidated(Sc64P7RuntimeState *s,
    const Sc64P7WorldOps *ops,int user_paused)
{
    Sc64P7Input in={{0},0,0};
    Sc64P7Output out;
    if (!s || !sc64_p7_runtime_state_valid_prevalidated(s)) {
        if (s) s->fault=SC64_P7_FAULT_STATE;
        return -SC64_P7_FAULT_STATE;
    }
    if (user_paused || s->model.outcome!=SC64_P7_RUNNING) return 0;
    if (!ops) { s->fault=SC64_P7_FAULT_METADATA; return -SC64_P7_FAULT_METADATA; }
    for (unsigned t=0;t<SC64_P7_TRIGGER_COUNT;++t) {
        const Sc64P7SourceCondition *c;
        int result;
        if (!sc64_p7_condition_row(t,0,&c)) {
            s->fault=SC64_P7_FAULT_METADATA; return -SC64_P7_FAULT_METADATA;
        }
        result=sc64_p7_condition_eval(s,ops,c);
        if (result<0 || result>1) {
            s->fault=SC64_P7_FAULT_CONDITION; return -SC64_P7_FAULT_CONDITION;
        }
        in.condition_true[t]=(u8)result;
    }
    for (unsigned t=0;t<SC64_P7_TRIGGER_COUNT;++t) {
        if (!s->model.action_cursor[t]) {
            if (s->action_ticks[t]) { s->fault=SC64_P7_FAULT_STATE; return -s->fault; }
            continue;
        }
        if (!s->action_ticks[t]) { s->fault=SC64_P7_FAULT_STATE; return -s->fault; }
        --s->action_ticks[t];
        if (!s->action_ticks[t]) in.action_complete|=sc64_p7_trigger_bit(t);
    }
    out=sc64_p7_step(&s->model,&in);
    for (u8 i=0;i<out.action_count;++i) {
        int fault=sc64_p7_dispatch_action_prevalidated(s,ops,&out.actions[i]);
        if (fault) { s->fault=(u8)fault; return -fault; }
    }
    if (!sc64_p7_runtime_state_valid_prevalidated(s)) {
        s->fault=SC64_P7_FAULT_STATE; return -s->fault;
    }
    return 1;
}
#endif

#endif
