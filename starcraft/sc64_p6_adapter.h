#ifndef SC64_P6_ADAPTER_H
#define SC64_P6_ADAPTER_H

#include "sc64_p6.h"
#include "sc64_p6_scenario_generated.h"

enum {
    SC64_P6_TICKS_PER_SECOND = 30,
    SC64_P6_PRESENTATION_HOLD_TICKS = 90,
    SC64_P6_FAULT_NONE = 0,
    SC64_P6_FAULT_STATE = 1,
    SC64_P6_FAULT_CONDITION = 2,
    SC64_P6_FAULT_ACTION = 3,
    SC64_P6_FAULT_SWITCH = 4,
    SC64_P6_FAULT_METADATA = 5,
    SC64_P6_FAULT_AI = 6,
    SC64_P6_RAW_ANY_UNIT = 229,
    SC64_P6_RAW_HERO = 79,
    SC64_P6_RAW_TEMPLAR = 75,
    SC64_P6_RAW_ANY_NONBUILDING = 230,
    SC64_P6_CONDITION_ALWAYS = 22,
    SC64_P6_CONDITION_COMMAND = 2,
    SC64_P6_CONDITION_BRING = 3,
    SC64_P6_CONDITION_SWITCH = 11,
    SC64_P6_CONDITION_ELAPSED_TIME = 12,
    SC64_P6_ACTION_VICTORY = 1,
    SC64_P6_ACTION_DEFEAT = 2,
    SC64_P6_ACTION_WAIT = 4,
    SC64_P6_ACTION_TRANSMISSION = 7,
    SC64_P6_ACTION_PLAY_WAV = 8,
    SC64_P6_ACTION_DISPLAY_TEXT = 9,
    SC64_P6_ACTION_CENTER_VIEW = 10,
    SC64_P6_ACTION_CREATE_UNIT = 11,
    SC64_P6_ACTION_OBJECTIVES = 12,
    SC64_P6_ACTION_SET_SWITCH = 13,
    SC64_P6_ACTION_RUN_AI = 15,
    SC64_P6_ACTION_KILL_UNIT = 22,
    SC64_P6_ACTION_MUTE_SPEECH = 30,
    SC64_P6_ACTION_UNMUTE_SPEECH = 31,
    SC64_P6_ACTION_SET_DOODAD = 42,
    SC64_P6_CREATE_ACTIONS = 18,
    SC64_P6_PROPERTY_COUNT = 13,
    SC64_P6_SWITCH_MASK = (1u << 0) | (1u << 1) | (1u << 3) |
                          (1u << 4) | (1u << 5)
};

_Static_assert(SC64_P6_SOURCE_GLOBAL_ID == SC64_P6_GLOBAL_ID,
               "P6 staged source ID mismatch");
_Static_assert(SC64_P6_SOURCE_MAP_COLS == SC64_P6_MAP_COLS &&
               SC64_P6_SOURCE_MAP_ROWS == SC64_P6_MAP_ROWS,
               "P6 staged map dimensions mismatch");
_Static_assert(SC64_P6_SOURCE_TRIGGER_COUNT == SC64_P6_TRIGGER_COUNT,
               "P6 staged trigger count mismatch");
_Static_assert(SC64_P6_SOURCE_ACTION_COUNT == SC64_P6_ACTION_TOTAL,
               "P6 staged action count mismatch");
_Static_assert(SC64_P6_SOURCE_CONDITION_COUNT == SC64_P6_CONDITION_TOTAL,
               "P6 staged condition count mismatch");

typedef struct {
    Sc64P6State model;
    u16 action_ticks[SC64_P6_TRIGGER_COUNT];
    u32 switch_bits;
    u8 fault;
} Sc64P6RuntimeState;

_Static_assert(sizeof(Sc64P6RuntimeState) <= 96, "P6 runtime state bound");

typedef struct {
    void *context;
    int (*condition)(void *context, const Sc64P6RuntimeState *state,
                     const Sc64P6SourceCondition *condition);
    /* All non-AI source effects are passed as their exact numeric row. The
     * callback must perform it exactly or fail. */
    int (*action)(void *context, const Sc64P6SourceAction *action);
    /* RunAIScript is opaque source data; it is never treated as source AI
     * bytecode. A bounded authored policy must explicitly accept the row. */
    int (*opaque_ai)(void *context, const Sc64P6SourceAction *action);
} Sc64P6WorldOps;

static inline unsigned sc64_p6_ceil_ticks(u32 milliseconds)
{
    u32 ticks;
    if (milliseconds > 1200000u) milliseconds = 1200000u;
    ticks = (milliseconds * SC64_P6_TICKS_PER_SECOND + 999u) / 1000u;
    if (!ticks) ticks = 1;
    return ticks > 36000u ? 36000u : ticks;
}

static inline int sc64_p6_source_action(unsigned trigger, unsigned index,
                                         const Sc64P6SourceAction **out)
{
    const Sc64P6SourceTrigger *t;
    const Sc64P6SourceAction *a;
    if (!out || trigger >= SC64_P6_TRIGGER_COUNT) return 0;
    t = &sc64_p6_source_triggers[trigger];
    if (index >= t->action_count ||
        t->first_action + index >= SC64_P6_SOURCE_ACTION_COUNT) return 0;
    a = &sc64_p6_source_actions[t->first_action + index];
    if (a->trigger != trigger || a->index != index || !a->opcode) return 0;
    *out = a;
    return 1;
}

static inline int sc64_p6_condition_row(unsigned trigger, unsigned index,
                                         const Sc64P6SourceCondition **out)
{
    const Sc64P6SourceTrigger *t;
    const Sc64P6SourceCondition *c;
    if (!out || trigger >= SC64_P6_TRIGGER_COUNT) return 0;
    t = &sc64_p6_source_triggers[trigger];
    if (index >= t->condition_count ||
        t->first_condition + index >= SC64_P6_SOURCE_CONDITION_COUNT) return 0;
    c = &sc64_p6_source_conditions[t->first_condition + index];
    if (c->trigger != trigger || c->index != index) return 0;
    *out = c;
    return 1;
}

static inline u16 sc64_p6_action_duration(unsigned trigger, unsigned index)
{
    const Sc64P6SourceAction *a;
    if (!sc64_p6_source_action(trigger, index, &a)) return 0;
    switch (a->opcode) {
    case SC64_P6_ACTION_WAIT: return (u16)sc64_p6_ceil_ticks(a->time);
    case SC64_P6_ACTION_PLAY_WAV: return (u16)sc64_p6_ceil_ticks(a->time);
    case SC64_P6_ACTION_TRANSMISSION:
    case SC64_P6_ACTION_DISPLAY_TEXT: return SC64_P6_PRESENTATION_HOLD_TICKS;
    case SC64_P6_ACTION_VICTORY: case SC64_P6_ACTION_DEFEAT:
    case SC64_P6_ACTION_CENTER_VIEW: case SC64_P6_ACTION_CREATE_UNIT:
    case SC64_P6_ACTION_OBJECTIVES: case SC64_P6_ACTION_SET_SWITCH:
    case SC64_P6_ACTION_RUN_AI: case SC64_P6_ACTION_KILL_UNIT:
    case SC64_P6_ACTION_MUTE_SPEECH: case SC64_P6_ACTION_UNMUTE_SPEECH:
    case SC64_P6_ACTION_SET_DOODAD: return 1;
    default: return 0;
    }
}

static inline int sc64_p6_trigger_enabled(unsigned trigger, unsigned executor)
{
    if (trigger >= SC64_P6_TRIGGER_COUNT || executor >= 27u) return 0;
    return sc64_p6_source_triggers[trigger].enabled_players[executor] == 1;
}

static inline int sc64_p6_location_source(unsigned id, unsigned width,
                                           unsigned height, unsigned *left,
                                           unsigned *top, unsigned *right,
                                           unsigned *bottom)
{
    const Sc64P6SourceLocation *loc;
    if (!width || !height || width > SC64_P6_MAP_COLS ||
        height > SC64_P6_MAP_ROWS || id == 0 || id > SC64_P6_SOURCE_LOCATION_COUNT ||
        !left || !top || !right || !bottom) return 0;
    /* CHK's final location is the special Anywhere region. The staged record
     * is empty, so resolve it explicitly to the map bounds. */
    if (id == SC64_P6_SOURCE_LOCATION_COUNT) {
        *left = *top = 0;
        *right = width * 32u;
        *bottom = height * 32u;
        return 1;
    }
    loc = &sc64_p6_source_locations[id - 1u];
    *left = loc->left; *top = loc->top; *right = loc->right; *bottom = loc->bottom;
    return *left <= *right && *top <= *bottom &&
           *right <= width * 32u && *bottom <= height * 32u;
}

static inline int sc64_p6_location_runtime(unsigned id, unsigned width,
                                            unsigned height, int *left,
                                            int *top, int *right, int *bottom)
{
    unsigned l,t,r,b;
    if (!left || !top || !right || !bottom ||
        !sc64_p6_location_source(id,width,height,&l,&t,&r,&b)) return 0;
    *left=(int)(l/2u); *top=(int)(t/2u);
    *right=(int)(r/2u); *bottom=(int)(b/2u);
    return 1;
}

static inline int sc64_p6_owner_matches(unsigned selector, unsigned raw_owner,
                                         unsigned current_player,
                                         const u8 active_slots[12],
                                         const u8 source_force_for_slot[8])
{
    if (!active_slots || !source_force_for_slot || raw_owner >= 12 ||
        current_player >= 8) return -1;
    if (!active_slots[raw_owner]) return 0;
    if (selector <= 11) return selector == raw_owner;
    if (selector == 12) return 0;
    if (selector == 13) return raw_owner == current_player;
    if (selector == 17) return raw_owner < 8;
    if (selector >= 18 && selector <= 21)
        return raw_owner < 8 && source_force_for_slot[raw_owner] == selector - 18u;
    return -1;
}

static inline int sc64_p6_selector_valid(unsigned selector)
{
    return selector<=11u || selector==13u || selector==17u ||
           (selector>=18u && selector<=21u);
}

static inline int sc64_p6_switch_is_set(const Sc64P6RuntimeState *s,
                                         unsigned index)
{
    return s && index < 32u ? (int)((s->switch_bits >> index) & 1u) : 0;
}

static inline int sc64_p6_source_property(unsigned id,
                                           const unsigned char **record)
{
    if (!record || id == 0 || id > SC64_P6_PROPERTY_COUNT ||
        !sc64_p6_source_property_used[id - 1u]) return 0;
    *record = sc64_p6_source_properties[id - 1u];
    return 1;
}

/* Numeric comparison values used by the staged Command/Bring rows. */
static inline int sc64_p6_count_comparison(unsigned actual,
                                            unsigned comparison,
                                            unsigned amount)
{
    if (comparison==0) return actual>=amount; /* At least */
    if (comparison==1) return actual<=amount; /* At most */
    if (comparison==10) return actual==amount; /* Exactly */
    return -1;
}

static inline int sc64_p6_switch_condition(const Sc64P6RuntimeState *s,
                                            const Sc64P6SourceCondition *c)
{
    int set;
    if (!s || !c || c->opcode!=SC64_P6_CONDITION_SWITCH || c->resource>=32u)
        return -1;
    set=sc64_p6_switch_is_set(s,c->resource);
    if (c->comparison==2) return set;      /* switch is set */
    if (c->comparison==3) return !set;     /* switch is cleared */
    return -1;
}

static inline int sc64_p6_runtime_metadata_valid(void)
{
    static const u8 action_counts[SC64_P6_TRIGGER_COUNT] = {
        1,1,2,2,2,2,2,2,5,2,2,8,8,1,7,12,5,2,3,3,7,12,3,1,3,1
    };
    static const u8 condition_counts[SC64_P6_TRIGGER_COUNT] = {
        1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,2
    };
    static const u8 executor[SC64_P6_TRIGGER_COUNT] = {
        17,17,18,18,18,18,18,18,7,18,18,18,18,19,18,7,18,18,18,18,18,18,18,17,17,17
    };
    static const u8 owner_modes[12] = {0,3,3,5,0,3,5,6,0,0,0,0};
    static const u8 owner_sides[12] = {1,1,2,0,0,2,0,2,7,7,7,4};
    static const u8 forces[20] = {3,2,0,1,3,2,3,0,9,0,7,0,6,0,5,0,0,0,0,0};
    unsigned creates=0, property_creates=0, ai_actions=0, actions=0, conditions=0;
    for (unsigned i=0;i<12;++i)
        if (sc64_p6_source_owners[i]!=owner_modes[i] ||
            sc64_p6_source_sides[i]!=owner_sides[i]) return 0;
    for (unsigned i=0;i<20;++i)
        if (sc64_p6_source_forces[i]!=forces[i]) return 0;
    for (unsigned i=0;i<64;++i) {
        if (sc64_p6_source_property_used[i] != (i < SC64_P6_PROPERTY_COUNT)) return 0;
        if (i >= SC64_P6_PROPERTY_COUNT)
            for (unsigned j=0;j<20;++j)
                if (sc64_p6_source_properties[i][j]) return 0;
    }
    for (unsigned t=0;t<SC64_P6_TRIGGER_COUNT;++t) {
        const Sc64P6SourceTrigger *tr=&sc64_p6_source_triggers[t];
        unsigned enabled=0;
        if (tr->action_count!=action_counts[t] || tr->condition_count!=condition_counts[t] ||
            tr->first_action+tr->action_count>SC64_P6_SOURCE_ACTION_COUNT ||
            tr->first_condition+tr->condition_count>SC64_P6_SOURCE_CONDITION_COUNT ||
            tr->initial_cursor || tr->execution_flags) return 0;
        for (unsigned p=0;p<27;++p) {
            if (tr->enabled_players[p] > 1) return 0;
            enabled += tr->enabled_players[p] != 0;
        }
        if (enabled!=1 || !sc64_p6_trigger_enabled(t,executor[t])) return 0;
        actions += tr->action_count; conditions += tr->condition_count;
        for (unsigned i=0;i<tr->action_count;++i) {
            const Sc64P6SourceAction *a;
            if (!sc64_p6_source_action(t,i,&a) || !sc64_p6_action_duration(t,i)) return 0;
            if (a->location && !sc64_p6_location_source(a->location,
                    SC64_P6_MAP_COLS,SC64_P6_MAP_ROWS,
                    &(unsigned){0},&(unsigned){0},&(unsigned){0},&(unsigned){0})) return 0;
            switch (a->opcode) {
            case SC64_P6_ACTION_WAIT: case SC64_P6_ACTION_TRANSMISSION:
            case SC64_P6_ACTION_PLAY_WAV: case SC64_P6_ACTION_DISPLAY_TEXT:
            case SC64_P6_ACTION_CENTER_VIEW: case SC64_P6_ACTION_OBJECTIVES:
            case SC64_P6_ACTION_MUTE_SPEECH: case SC64_P6_ACTION_UNMUTE_SPEECH:
                break;
            case SC64_P6_ACTION_VICTORY: case SC64_P6_ACTION_DEFEAT:
                if (a->location || a->group1 || a->group2 || a->unit || a->time)
                    return 0;
                break;
            case SC64_P6_ACTION_CREATE_UNIT:
                ++creates;
                if (a->modifier || !a->location || !a->group1 || !a->unit)
                    return 0;
                if (a->group2) {
                    const unsigned char *property;
                    if (a->flags!=0x18u ||
                        !sc64_p6_source_property(a->group2,&property) || !property) return 0;
                    ++property_creates;
                } else if (a->flags!=0x10u) return 0;
                if (!sc64_p6_selector_valid(a->group1)) return 0;
                break;
            case SC64_P6_ACTION_SET_SWITCH:
                if (a->group1 || a->group2 >= 32 ||
                    !(SC64_P6_SWITCH_MASK & ((u32)1u << a->group2)) ||
                    (a->modifier!=4 && a->modifier!=5) ||
                    (a->flags!=0 && a->flags!=0x04u)) return 0;
                break;
            case SC64_P6_ACTION_RUN_AI:
                /* Preserve the exact opaque script identifier for an explicit
                 * policy callback; do not claim to execute native AI. */
                if (t!=13 || i!=0 || !a->group2 || a->group1 || a->unit ||
                    a->location || a->flags) return 0;
                ++ai_actions;
                break;
            case SC64_P6_ACTION_KILL_UNIT:
                if (a->group1!=13 || a->group2 || a->unit!=101 ||
                    a->location || a->flags!=0x10u) return 0;
                break;
            case SC64_P6_ACTION_SET_DOODAD:
                if (!a->location || a->group1!=17 || a->group2 ||
                    (a->unit!=205 && a->unit!=206 && a->unit!=207 && a->unit!=208) ||
                    a->flags!=0x10u) return 0;
                break;
            default: return 0;
            }
        }
        for (unsigned i=0;i<tr->condition_count;++i) {
            const Sc64P6SourceCondition *c;
            if (!sc64_p6_condition_row(t,i,&c)) return 0;
            if (c->location && !sc64_p6_location_source(c->location,
                    SC64_P6_MAP_COLS,SC64_P6_MAP_ROWS,
                    &(unsigned){0},&(unsigned){0},&(unsigned){0},&(unsigned){0})) return 0;
            switch (c->opcode) {
            case SC64_P6_CONDITION_ALWAYS:
                if (c->location || c->player || c->amount || c->unit ||
                    c->comparison || c->resource || c->flags) return 0;
                break;
            case SC64_P6_CONDITION_COMMAND:
                if (c->location || !sc64_p6_selector_valid(c->player) ||
                    !c->unit || sc64_p6_count_comparison(0,c->comparison,c->amount)<0 ||
                    c->flags!=0x10u) return 0;
                break;
            case SC64_P6_CONDITION_BRING:
                if (!c->location || !sc64_p6_selector_valid(c->player) || !c->amount ||
                    c->comparison!=0 || c->flags!=0x10u) return 0;
                break;
            case SC64_P6_CONDITION_SWITCH:
                if (c->location || c->player || c->amount || c->unit ||
                    (c->comparison!=2 && c->comparison!=3) ||
                    (c->resource!=0 && c->resource!=1 && c->resource!=5) || c->flags) return 0;
                break;
            case SC64_P6_CONDITION_ELAPSED_TIME:
                if (c->location || c->player || c->amount || c->unit ||
                    c->comparison || c->resource || c->flags) return 0;
                break;
            default: return 0;
            }
        }
    }
    return actions==SC64_P6_ACTION_TOTAL && conditions==SC64_P6_CONDITION_TOTAL &&
           creates==SC64_P6_CREATE_ACTIONS && property_creates==13 && ai_actions==1;
}

static inline int sc64_p6_runtime_state_valid(const Sc64P6RuntimeState *s)
{
    if (!s || !sc64_p6_state_valid(&s->model) || s->fault ||
        (s->switch_bits & ~((u32)SC64_P6_SWITCH_MASK))) return 0;
    for (unsigned i=0;i<SC64_P6_TRIGGER_COUNT;++i) {
        u8 cursor=s->model.action_cursor[i];
        u16 ticks=s->action_ticks[i];
        if ((cursor!=0)!=(ticks!=0)) return 0;
        if (ticks) {
            u16 limit=sc64_p6_action_duration(i,cursor-1u);
            if (!limit || ticks>limit) return 0;
        }
    }
    return 1;
}

static inline void sc64_p6_runtime_init(Sc64P6RuntimeState *s)
{
    if (!s) return;
    sc64_p6_init(&s->model);
    for (unsigned i=0;i<SC64_P6_TRIGGER_COUNT;++i) s->action_ticks[i]=0;
    s->switch_bits=0; s->fault=SC64_P6_FAULT_NONE;
}

static inline int sc64_p6_dispatch_action(Sc64P6RuntimeState *s,
                                           const Sc64P6WorldOps *ops,
                                           const Sc64P6ActionEvent *event)
{
    const Sc64P6SourceAction *a;
    u16 duration;
    int result;
    if (!s || !ops || !event || !sc64_p6_source_action(event->trigger,event->action_index,&a))
        return SC64_P6_FAULT_METADATA;
    if (!sc64_p6_state_valid(&s->model) || s->fault ||
        (s->switch_bits & ~((u32)SC64_P6_SWITCH_MASK)) ||
        !sc64_p6_trigger_started(&s->model,event->trigger) ||
        s->model.action_cursor[event->trigger]!=(u8)(event->action_index+1u) ||
        s->action_ticks[event->trigger]) return SC64_P6_FAULT_STATE;
    duration=sc64_p6_action_duration(event->trigger,event->action_index);
    if (!duration) return SC64_P6_FAULT_ACTION;
    if (a->opcode==SC64_P6_ACTION_SET_SWITCH &&
        (a->group1 || a->group2>=32 || (a->modifier!=4 && a->modifier!=5)))
        return SC64_P6_FAULT_SWITCH;
    if (a->opcode==SC64_P6_ACTION_RUN_AI)
        result=ops->opaque_ai ? ops->opaque_ai(ops->context,a) : 0;
    else
        result=ops->action ? ops->action(ops->context,a) : 0;
    if (result!=1) return a->opcode==SC64_P6_ACTION_RUN_AI ?
                            SC64_P6_FAULT_AI : SC64_P6_FAULT_ACTION;
    if (a->opcode==SC64_P6_ACTION_SET_SWITCH) {
        u32 bit=(u32)1u<<a->group2;
        if (a->modifier==4) s->switch_bits|=bit;
        else s->switch_bits&=~bit;
    }
    s->action_ticks[event->trigger]=duration;
    return SC64_P6_FAULT_NONE;
}

static inline int sc64_p6_adapter_tick(Sc64P6RuntimeState *s,
                                        const Sc64P6WorldOps *ops,
                                        int user_paused)
{
    Sc64P6Input in={{0},0,0};
    Sc64P6Output out;
    if (!s || !sc64_p6_runtime_state_valid(s)) {
        if (s) s->fault=SC64_P6_FAULT_STATE;
        return -SC64_P6_FAULT_STATE;
    }
    if (user_paused) return 0;
    if (s->model.outcome!=SC64_P6_RUNNING) return 0;
    if (!ops || !ops->condition || !ops->action || !sc64_p6_runtime_metadata_valid()) {
        s->fault=SC64_P6_FAULT_METADATA;
        return -SC64_P6_FAULT_METADATA;
    }
    for (unsigned t=0;t<SC64_P6_TRIGGER_COUNT;++t) {
        const Sc64P6SourceTrigger *tr=&sc64_p6_source_triggers[t];
        for (unsigned i=0;i<tr->condition_count;++i) {
            const Sc64P6SourceCondition *row;
            int value;
            if (!sc64_p6_condition_row(t,i,&row)) {
                s->fault=SC64_P6_FAULT_METADATA; return -s->fault;
            }
            value=ops->condition(ops->context,s,row);
            if (value<0 || value>1) {
                s->fault=SC64_P6_FAULT_CONDITION; return -s->fault;
            }
            if (value) in.condition_true[t]|=(u16)1u<<i;
        }
    }
    for (unsigned t=0;t<SC64_P6_TRIGGER_COUNT;++t) {
        if (!s->model.action_cursor[t]) {
            if (s->action_ticks[t]) { s->fault=SC64_P6_FAULT_STATE; return -s->fault; }
            continue;
        }
        if (!s->action_ticks[t]) { s->fault=SC64_P6_FAULT_STATE; return -s->fault; }
        --s->action_ticks[t];
        if (!s->action_ticks[t]) in.action_complete|=sc64_p6_trigger_bit(t);
    }
    out=sc64_p6_step(&s->model,&in);
    for (u8 i=0;i<out.action_count;++i) {
        int fault=sc64_p6_dispatch_action(s,ops,&out.actions[i]);
        if (fault) { s->fault=(u8)fault; return -fault; }
    }
    if (!sc64_p6_runtime_state_valid(s)) {
        s->fault=SC64_P6_FAULT_STATE; return -s->fault;
    }
    return 1;
}

#endif
