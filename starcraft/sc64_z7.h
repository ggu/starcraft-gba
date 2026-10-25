#ifndef SC64_Z7_H
#define SC64_Z7_H

#include "../common/gba.h"

/* Numeric scheduler for the authenticated Z7 trigger table. Opaque Zr7* AI
 * scripts are dispatched by a separately bounded, authored policy. */
enum {
    SC64_Z7_GLOBAL_ID = 18,
    SC64_Z7_MAP_COLS = 128,
    SC64_Z7_MAP_ROWS = 128,
    SC64_Z7_TRIGGER_COUNT = 8,
    SC64_Z7_ACTION_TOTAL = 15,
    SC64_Z7_CONDITION_TOTAL = 8,
    SC64_Z7_TICKS_PER_SECOND = 30,
    SC64_Z7_TRIGGER_LOSS = 0,
    SC64_Z7_TRIGGER_SETUP = 1,
    SC64_Z7_TRIGGER_VICTORY = 2,
    SC64_Z7_TRIGGER_AI = 3,
    SC64_Z7_TRIGGER_INITIAL_SWITCH = 4,
    SC64_Z7_TRIGGER_TIMER_SWITCH = 5,
    SC64_Z7_TRIGGER_FORCE0_REGION3 = 6,
    SC64_Z7_TRIGGER_FORCE0_REGION5 = 7,
    SC64_Z7_RUNNING = 0,
    SC64_Z7_WON = 1,
    SC64_Z7_LOST = 2,
    SC64_Z7_EVENT_ACTION = 1u << 0,
    SC64_Z7_EVENT_WON = 1u << 1,
    SC64_Z7_EVENT_LOST = 1u << 2
};

static inline u8 sc64_z7_action_count(u8 trigger)
{
    static const u8 counts[SC64_Z7_TRIGGER_COUNT] = {1,3,1,5,2,1,1,1};
    return trigger < SC64_Z7_TRIGGER_COUNT ? counts[trigger] : 0;
}

static inline u8 sc64_z7_condition_count(u8 trigger)
{ return trigger < SC64_Z7_TRIGGER_COUNT ? 1u : 0u; }

typedef struct {
    u16 condition_true[SC64_Z7_TRIGGER_COUNT];
    u16 action_complete;
    u8 paused;
} Sc64Z7Input;

typedef struct {
    u16 started;
    u8 outcome;
    /* Zero is idle/done. Otherwise this is source action index + 1. */
    u8 action_cursor[SC64_Z7_TRIGGER_COUNT];
} Sc64Z7State;

_Static_assert(sizeof(Sc64Z7State) == 12, "Z7 model state size");

typedef struct { u8 trigger, action_index; } Sc64Z7ActionEvent;
typedef struct {
    u8 events, action_count;
    Sc64Z7ActionEvent actions[SC64_Z7_TRIGGER_COUNT];
} Sc64Z7Output;

static inline u16 sc64_z7_init(Sc64Z7State *s)
{
    if (!s) return 0;
    for (unsigned i=0;i<sizeof(*s);++i) ((u8 *)s)[i]=0;
    return 0;
}

static inline int sc64_z7_state_valid(const Sc64Z7State *s)
{
    unsigned terminal=0, terminal_id=255;
    if (!s || s->outcome>SC64_Z7_LOST || (s->started & (u16)~0xffu)) return 0;
    for (unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;++i) {
        if (s->action_cursor[i]>sc64_z7_action_count((u8)i) ||
            (s->action_cursor[i] && !(s->started & (1u<<i)))) return 0;
        if (i==SC64_Z7_TRIGGER_LOSS || i==SC64_Z7_TRIGGER_VICTORY) {
            if ((s->started & (1u<<i)) && !s->action_cursor[i]) {
                ++terminal; terminal_id=i;
            }
        }
    }
    if (s->outcome==SC64_Z7_RUNNING && terminal) return 0;
    if (s->outcome==SC64_Z7_WON && (terminal!=1 || terminal_id!=SC64_Z7_TRIGGER_VICTORY)) return 0;
    if (s->outcome==SC64_Z7_LOST && (terminal!=1 || terminal_id!=SC64_Z7_TRIGGER_LOSS)) return 0;
    return 1;
}

static inline int sc64_z7_input_valid(const Sc64Z7Input *in)
{
    if (!in || in->paused>1 || (in->action_complete & (u16)~0xffu)) return 0;
    for (unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;++i)
        if (in->condition_true[i] & (u16)~1u) return 0;
    return 1;
}

static inline int sc64_z7_input_state_valid(const Sc64Z7State *s,const Sc64Z7Input *in)
{
    if (!sc64_z7_state_valid(s) || !sc64_z7_input_valid(in)) return 0;
    for (unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;++i)
        if ((in->action_complete&(1u<<i)) && !s->action_cursor[i]) return 0;
    return 1;
}

static inline int sc64_z7_conditions_met(const Sc64Z7Input *in,u8 trigger)
{ return in && trigger<SC64_Z7_TRIGGER_COUNT && (in->condition_true[trigger]&1u); }

static inline void sc64_z7_emit(Sc64Z7Output *out,u8 trigger,u8 action)
{
    if (out->action_count>=SC64_Z7_TRIGGER_COUNT) return;
    out->actions[out->action_count++]=(Sc64Z7ActionEvent){trigger,action};
    out->events|=SC64_Z7_EVENT_ACTION;
}

/* Active programs complete in source order before newly-ready triggers. */
static inline Sc64Z7Output sc64_z7_step(Sc64Z7State *s,const Sc64Z7Input *in)
{
    Sc64Z7Output out={0,0,{{0,0}}};
    u8 was_active[SC64_Z7_TRIGGER_COUNT];
    if (!sc64_z7_input_state_valid(s,in) || in->paused || s->outcome!=SC64_Z7_RUNNING) return out;
    for (unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;++i) was_active[i]=s->action_cursor[i]!=0;
    for (u8 i=0;i<SC64_Z7_TRIGGER_COUNT;++i) {
        u8 cursor=s->action_cursor[i];
        if (s->outcome!=SC64_Z7_RUNNING) break;
        if (!cursor || !(in->action_complete&(1u<<i))) continue;
        if (cursor<sc64_z7_action_count(i)) {
            s->action_cursor[i]=(u8)(cursor+1u);
            sc64_z7_emit(&out,i,cursor);
        } else {
            s->action_cursor[i]=0;
            if (i==SC64_Z7_TRIGGER_VICTORY) { s->outcome=SC64_Z7_WON; out.events|=SC64_Z7_EVENT_WON; }
            else if (i==SC64_Z7_TRIGGER_LOSS) { s->outcome=SC64_Z7_LOST; out.events|=SC64_Z7_EVENT_LOST; }
        }
    }
    if (s->outcome==SC64_Z7_RUNNING) for (u8 i=0;i<SC64_Z7_TRIGGER_COUNT;++i) {
        if (!sc64_z7_conditions_met(in,i) || was_active[i] || s->action_cursor[i] ||
            (s->started&(1u<<i))) continue;
        s->started|=(u16)(1u<<i); s->action_cursor[i]=1; sc64_z7_emit(&out,i,0);
    }
    return out;
}

#endif
