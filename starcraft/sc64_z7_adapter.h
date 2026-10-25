#ifndef SC64_Z7_ADAPTER_H
#define SC64_Z7_ADAPTER_H

#if defined(SC64_Z7_PREVALIDATED_TICK) && !defined(SC64_Z7_RUNTIME)
#error "SC64_Z7_PREVALIDATED_TICK requires the authenticated Z7 runtime"
#endif
#if defined(SC64_Z7_COUNT_SNAPSHOT) && !defined(SC64_Z7_PREVALIDATED_TICK)
#error "SC64_Z7_COUNT_SNAPSHOT requires SC64_Z7_PREVALIDATED_TICK"
#endif

#include "sc64_z7.h"
#include "sc64_z7_scenario_generated.h"

enum {
    SC64_Z7_PRESENTATION_HOLD_TICKS = 1,
    SC64_Z7_AI_PERIOD_TICKS = 240,
    SC64_Z7_AI_RESERVE_UNITS = 16,
    SC64_Z7_AI_WAVE_SIZE = 2,
    SC64_Z7_FAULT_NONE = 0,
    SC64_Z7_FAULT_STATE = 1,
    SC64_Z7_FAULT_CONDITION = 2,
    SC64_Z7_FAULT_ACTION = 3,
    SC64_Z7_FAULT_METADATA = 4,
    SC64_Z7_FAULT_OWNER = 5,
    SC64_Z7_FAULT_WORLD = 6
};

_Static_assert(SC64_Z7_SOURCE_GLOBAL_ID == SC64_Z7_GLOBAL_ID, "Z7 source/global id");
_Static_assert(SC64_Z7_SOURCE_MAP_COLS == SC64_Z7_MAP_COLS &&
               SC64_Z7_SOURCE_MAP_ROWS == SC64_Z7_MAP_ROWS, "Z7 map dimensions");
_Static_assert(SC64_Z7_SOURCE_TRIGGER_COUNT == SC64_Z7_TRIGGER_COUNT &&
               SC64_Z7_SOURCE_ACTION_COUNT == SC64_Z7_ACTION_TOTAL &&
               SC64_Z7_SOURCE_CONDITION_COUNT == SC64_Z7_CONDITION_TOTAL,
               "Z7 source table sizes");

typedef struct {
    Sc64Z7State model;
    u16 action_ticks[SC64_Z7_TRIGGER_COUNT];
    u8 switch10;
    u8 ai_script_mask; /* Four authenticated calls, not native AI bytecode. */
    u16 ai_plan_ticks;
    u8 ai_plan_phase;
    u8 fault;
} Sc64Z7RuntimeState;

_Static_assert(sizeof(Sc64Z7RuntimeState) <= 36, "Z7 runtime state budget");

typedef struct {
    void *context;
    u32 elapsed_frames;
    int (*condition)(void *context, const Sc64Z7RuntimeState *state,
                     const Sc64Z7SourceCondition *condition);
    int (*action)(void *context, const Sc64Z7SourceAction *action);
} Sc64Z7WorldOps;

static inline unsigned sc64_z7_ceil_ticks(u32 milliseconds)
{
    u32 ticks;
    if (milliseconds > 1200000u) milliseconds=1200000u;
    ticks=(milliseconds*SC64_Z7_TICKS_PER_SECOND+999u)/1000u;
    return ticks ? (ticks>36000u?36000u:ticks) : 1u;
}

static inline int sc64_z7_source_action(unsigned trigger,unsigned index,
                                         const Sc64Z7SourceAction **out)
{
    const Sc64Z7SourceTrigger *t; const Sc64Z7SourceAction *a;
    if (!out || trigger>=SC64_Z7_TRIGGER_COUNT) return 0;
    t=&sc64_z7_source_triggers[trigger];
    if (index>=t->action_count || t->first_action+index>=SC64_Z7_SOURCE_ACTION_COUNT) return 0;
    a=&sc64_z7_source_actions[t->first_action+index];
    if (a->trigger!=trigger || a->index!=index || !a->opcode) return 0;
    *out=a; return 1;
}

static inline int sc64_z7_trigger_executor(unsigned trigger,unsigned *executor)
{
    unsigned found=27;
    if (!executor || trigger>=SC64_Z7_TRIGGER_COUNT) return 0;
    for (unsigned i=0;i<27;++i) if (sc64_z7_source_triggers[trigger].enabled_players[i]) {
        if (found!=27) return 0;
        found=i;
    }
    if (found==27) return 0;
    *executor=found; return 1;
}

static inline int sc64_z7_condition_row(unsigned trigger,unsigned index,
                                         const Sc64Z7SourceCondition **out)
{
    const Sc64Z7SourceTrigger *t; const Sc64Z7SourceCondition *c;
    if (!out || trigger>=SC64_Z7_TRIGGER_COUNT) return 0;
    t=&sc64_z7_source_triggers[trigger];
    if (index>=t->condition_count || t->first_condition+index>=SC64_Z7_SOURCE_CONDITION_COUNT) return 0;
    c=&sc64_z7_source_conditions[t->first_condition+index];
    if (c->trigger!=trigger || c->index!=index) return 0;
    *out=c; return 1;
}

static inline u16 sc64_z7_action_duration(unsigned trigger,unsigned index)
{
    const Sc64Z7SourceAction *a;
    if (!sc64_z7_source_action(trigger,index,&a)) return 0;
    switch (a->opcode) {
    case 4: return (u16)sc64_z7_ceil_ticks(a->time);
    case 1: case 2: case 12: case 13: case 16: case 26: return 1;
    default: return 0;
    }
}

static inline int sc64_z7_runtime_metadata_valid(void)
{
    static const u8 action_counts[8]={1,3,1,5,2,1,1,1};
    static const u8 condition_op[8]={2,12,2,11,3,12,3,3};
    static const u8 executor[8]={18,18,18,19,18,18,18,18};
    static const u8 trigger_actions[8][5]={
        {2,0,0,0,0},{26,26,12,0,0},{1,0,0,0,0},
        {26,16,16,16,16},{4,13,0,0,0},{13,0,0,0,0},{13,0,0,0,0},{13,0,0,0,0}};
    static const u8 expected_force[8]={3,3,3,0,1,3,3,3};
    unsigned actions=0,conditions=0;
    for (unsigned i=0;i<8;++i) {
        const Sc64Z7SourceTrigger *t=&sc64_z7_source_triggers[i];
        if (t->execution_flags || t->initial_cursor ||
            t->action_count!=action_counts[i] || t->condition_count!=1 ||
            t->first_action+t->action_count>SC64_Z7_SOURCE_ACTION_COUNT ||
            t->first_condition+t->condition_count>SC64_Z7_SOURCE_CONDITION_COUNT)
            return 0;
        for (unsigned p=0;p<27;++p)
            if (t->enabled_players[p]!=(p==executor[i])) return 0;
        for (unsigned j=0;j<t->action_count;++j) {
            const Sc64Z7SourceAction *a;
            if (!sc64_z7_source_action(i,j,&a) || a->opcode!=trigger_actions[i][j] ||
                a->trailing[0] || a->trailing[1] || a->trailing[2]) return 0;
            if ((a->opcode==1 || a->opcode==2) &&
                (a->location || a->string_id || a->wav_id || a->time || a->group1 ||
                 a->group2 || a->unit || a->modifier || a->flags)) return 0;
            if (a->opcode==26) {
                unsigned expected_resource=(i==1)?j:2u;
                unsigned expected_amount=(i==1)?(j==0?800u:200u):8000u;
                if (a->group1!=(i==1?18u:19u) || a->group2!=expected_amount ||
                    a->unit!=expected_resource || a->modifier!=7 || a->flags) return 0;
            } else if (a->opcode==16) {
                static const u8 loc[4]={1,3,5,2};
                static const u32 script[4]={0x6137725au,0x7337725au,0x7337725au,0x6737725au};
                unsigned k=j-1u;
                if (i!=3 || a->location!=loc[k] || a->group1 || a->group2!=script[k] ||
                    a->unit || a->modifier || a->flags) return 0;
            } else if (a->opcode==4) {
                if (i!=4 || j || a->time!=10000u || a->location || a->flags) return 0;
            } else if (a->opcode==13) {
                if (a->location || a->group1 || a->group2!=10u || a->unit ||
                    a->modifier!=4u || a->flags) return 0;
            } else if (a->opcode==12) {
                if (i!=1 || a->string_id!=12u || a->location || a->flags) return 0;
            }
        }
        {
            const Sc64Z7SourceCondition *c;
            if (!sc64_z7_condition_row(i,0,&c) || c->opcode!=condition_op[i] || c->mask)
                return 0;
            if ((i==0 || i==2) && (c->location || c->resource || c->flags!=16u ||
                c->comparison!=1u || c->amount || c->unit!=(i==0?229u:231u) ||
                c->player!=(i==0?13u:19u))) return 0;
            if (i==1 || i==5) {
                unsigned amount=i==1?0u:300u;
                if (c->location || c->player || c->unit || c->comparison || c->resource ||
                    c->flags || c->amount!=amount) return 0;
            }
            if (i==3 && (c->location || c->player || c->amount || c->unit ||
                         c->comparison!=2u || c->resource!=10u || c->flags)) return 0;
            if (i==4 && (c->location!=4u || c->player!=19u || c->amount || c->unit!=231u ||
                         c->comparison!=1u || c->resource || c->flags!=16u)) return 0;
            if ((i==6 || i==7) && (c->location!=(i==6?3u:5u) || c->player!=18u ||
                c->amount!=1u || c->unit!=229u || c->comparison || c->resource ||
                c->flags!=16u)) return 0;
        }
        actions+=t->action_count; conditions+=t->condition_count;
    }
    for (unsigned i=0;i<8;++i) if (sc64_z7_source_forces[i]!=expected_force[i]) return 0;
    if (actions!=SC64_Z7_ACTION_TOTAL || conditions!=SC64_Z7_CONDITION_TOTAL) return 0;
    for (unsigned i=1;i<=5;++i) {
        const Sc64Z7SourceLocation *l=&sc64_z7_source_locations[i-1];
        if (l->left>=l->right || l->top>=l->bottom || l->right>SC64_Z7_MAP_COLS*32u ||
            l->bottom>SC64_Z7_MAP_ROWS*32u || l->flags) return 0;
    }
    return 1;
}

static inline int sc64_z7_runtime_state_valid(const Sc64Z7RuntimeState *s)
{
    static const u8 valid_ai_masks[5]={0,1,3,7,15};
    int valid_mask=0; u8 expected_mask=0;
    if (!s || !sc64_z7_state_valid(&s->model) || s->switch10>1 ||
        (s->ai_script_mask&~15u) || s->ai_plan_ticks>SC64_Z7_AI_PERIOD_TICKS ||
        s->ai_plan_phase>3u || s->fault) return 0;
    for (unsigned i=0;i<5;++i) valid_mask|=s->ai_script_mask==valid_ai_masks[i];
    if (!valid_mask) return 0;
    if (s->model.started&(1u<<SC64_Z7_TRIGGER_AI)) {
        u8 cursor=s->model.action_cursor[SC64_Z7_TRIGGER_AI];
        if (!cursor) expected_mask=15u;
        else if (cursor>=2u) expected_mask=(u8)((1u<<(cursor-1u))-1u);
    }
    if (s->ai_script_mask!=expected_mask ||
        ((s->ai_script_mask!=0)!=(s->ai_plan_ticks!=0))) return 0;
    for (unsigned i=0;i<8;++i) {
        u8 cursor=s->model.action_cursor[i]; u16 ticks=s->action_ticks[i];
        if ((cursor!=0)!=(ticks!=0)) return 0;
        if (ticks && (ticks>sc64_z7_action_duration(i,cursor-1u))) return 0;
    }
    if (!s->ai_script_mask && (s->ai_plan_ticks || s->ai_plan_phase)) return 0;
    return 1;
}

typedef struct {
    u8 source_current_player;
    u8 source_slot_active[12];
    s8 live_owner_for_source[12];
    u8 source_force_for_slot[8];
#if defined(SC64_Z7_PREVALIDATED_TICK)
    /* Runtime-only capability; never serialized and set only after bind checks. */
    u8 source_tables_prevalidated;
#endif
#ifdef SC64_Z7_COUNT_SNAPSHOT
    /* A single-tick live-world census; runtime-only and never serialized. */
    struct { u32 count[5]; u8 active; } count_snapshot;
#endif
    u32 elapsed_frames;
    u8 presentation_opcode;
    u16 last_string_id;
    u8 victory_requested,defeat_requested,objective_refresh_requested;
} Sc64Z7WorldContext;

static inline int sc64_z7_dispatch_action(Sc64Z7RuntimeState *s,
                                           const Sc64Z7WorldOps *ops,
                                           const Sc64Z7ActionEvent *event)
{
    const Sc64Z7SourceAction *a;
    u16 duration;
    if (!s || !ops || !ops->action || !event ||
        !sc64_z7_source_action(event->trigger,event->action_index,&a) ||
        s->action_ticks[event->trigger]) return SC64_Z7_FAULT_ACTION;
    if (!ops->action(ops->context,a)) return SC64_Z7_FAULT_ACTION;
    duration=sc64_z7_action_duration(event->trigger,event->action_index);
    if (!duration) return SC64_Z7_FAULT_METADATA;
    s->action_ticks[event->trigger]=duration;
    return 0;
}

/* One campaign scheduler tick. All source rows are resolved every tick;
 * user pause freezes both timers and action cursors. */
#if defined(SC64_Z7_PREVALIDATED_TICK)
static inline int sc64_z7_runtime_tick_internal(Sc64Z7RuntimeState *s,
                                                const Sc64Z7WorldOps *ops,
                                                int user_paused,
                                                int metadata_prevalidated)
{
    Sc64Z7Input input={{0},0,0};
    Sc64Z7Output out;
    if (!s || !sc64_z7_runtime_state_valid(s)) {
        if (s) s->fault=SC64_Z7_FAULT_STATE;
        return -SC64_Z7_FAULT_STATE;
    }
    if (user_paused || s->model.outcome!=SC64_Z7_RUNNING) return 0;
    if (!ops || !ops->condition || !ops->action ||
        (!metadata_prevalidated && !sc64_z7_runtime_metadata_valid())) {
        s->fault=SC64_Z7_FAULT_METADATA; return -SC64_Z7_FAULT_METADATA;
    }
    for (u8 t=0;t<SC64_Z7_TRIGGER_COUNT;++t) {
        const Sc64Z7SourceCondition *c;
        int result;
        if (metadata_prevalidated) {
            c=&sc64_z7_source_conditions[sc64_z7_source_triggers[t].first_condition];
        } else if (!sc64_z7_condition_row(t,0,&c)) {
            s->fault=SC64_Z7_FAULT_METADATA; return -SC64_Z7_FAULT_METADATA;
        }
        result=ops->condition(ops->context,s,c);
        if (result<0 || result>1) {
            s->fault=SC64_Z7_FAULT_CONDITION; return -SC64_Z7_FAULT_CONDITION;
        }
        if (result) input.condition_true[t]=1;
    }
    for (u8 t=0;t<SC64_Z7_TRIGGER_COUNT;++t) {
        if (!s->model.action_cursor[t]) {
            if (s->action_ticks[t]) { s->fault=SC64_Z7_FAULT_STATE; return -SC64_Z7_FAULT_STATE; }
            continue;
        }
        if (!s->action_ticks[t]) { s->fault=SC64_Z7_FAULT_STATE; return -SC64_Z7_FAULT_STATE; }
        if (!--s->action_ticks[t]) input.action_complete|=(u16)(1u<<t);
    }
    out=sc64_z7_step(&s->model,&input);
    for (u8 i=0;i<out.action_count;++i) {
        int fault=sc64_z7_dispatch_action(s,ops,&out.actions[i]);
        if (fault) { s->fault=(u8)fault; return -fault; }
    }
    if (s->model.outcome!=SC64_Z7_RUNNING) {
        for (unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;++i) {
            s->model.action_cursor[i]=0;
            s->action_ticks[i]=0;
        }
    }
    if (!sc64_z7_runtime_state_valid(s)) {
        s->fault=SC64_Z7_FAULT_STATE; return -SC64_Z7_FAULT_STATE;
    }
    return 1;
}

/* Public checked entrypoint for standalone clients and fixtures. */
static inline int sc64_z7_runtime_tick(Sc64Z7RuntimeState *s,
                                        const Sc64Z7WorldOps *ops,
                                        int user_paused)
{ return sc64_z7_runtime_tick_internal(s,ops,user_paused,0); }

/* Private to the authenticated game adapter after its immutable table checks. */
static inline int sc64_z7_runtime_tick_prevalidated(Sc64Z7RuntimeState *s,
                                                     const Sc64Z7WorldOps *ops,
                                                     int user_paused)
{ return sc64_z7_runtime_tick_internal(s,ops,user_paused,1); }
#else
/* Keep the default build's public checked path exactly as before the optional
 * campaign-only fast entry was introduced. */
static inline int sc64_z7_runtime_tick(Sc64Z7RuntimeState *s,
                                        const Sc64Z7WorldOps *ops,
                                        int user_paused)
{
    Sc64Z7Input input={{0},0,0};
    Sc64Z7Output out;
    if (!s || !sc64_z7_runtime_state_valid(s)) {
        if (s) s->fault=SC64_Z7_FAULT_STATE;
        return -SC64_Z7_FAULT_STATE;
    }
    if (user_paused || s->model.outcome!=SC64_Z7_RUNNING) return 0;
    if (!ops || !ops->condition || !ops->action || !sc64_z7_runtime_metadata_valid()) {
        s->fault=SC64_Z7_FAULT_METADATA; return -SC64_Z7_FAULT_METADATA;
    }
    for (u8 t=0;t<SC64_Z7_TRIGGER_COUNT;++t) {
        const Sc64Z7SourceCondition *c;
        int result;
        if (!sc64_z7_condition_row(t,0,&c)) {
            s->fault=SC64_Z7_FAULT_METADATA; return -SC64_Z7_FAULT_METADATA;
        }
        result=ops->condition(ops->context,s,c);
        if (result<0 || result>1) {
            s->fault=SC64_Z7_FAULT_CONDITION; return -SC64_Z7_FAULT_CONDITION;
        }
        if (result) input.condition_true[t]=1;
    }
    for (u8 t=0;t<SC64_Z7_TRIGGER_COUNT;++t) {
        if (!s->model.action_cursor[t]) {
            if (s->action_ticks[t]) { s->fault=SC64_Z7_FAULT_STATE; return -SC64_Z7_FAULT_STATE; }
            continue;
        }
        if (!s->action_ticks[t]) { s->fault=SC64_Z7_FAULT_STATE; return -SC64_Z7_FAULT_STATE; }
        if (!--s->action_ticks[t]) input.action_complete|=(u16)(1u<<t);
    }
    out=sc64_z7_step(&s->model,&input);
    for (u8 i=0;i<out.action_count;++i) {
        int fault=sc64_z7_dispatch_action(s,ops,&out.actions[i]);
        if (fault) { s->fault=(u8)fault; return -fault; }
    }
    if (s->model.outcome!=SC64_Z7_RUNNING) {
        for (unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;++i) {
            s->model.action_cursor[i]=0;
            s->action_ticks[i]=0;
        }
    }
    if (!sc64_z7_runtime_state_valid(s)) {
        s->fault=SC64_Z7_FAULT_STATE; return -SC64_Z7_FAULT_STATE;
    }
    return 1;
}
#endif

#endif
