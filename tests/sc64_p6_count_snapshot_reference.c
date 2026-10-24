#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_p6_adapter.h"

enum { MAX_UNITS = 240, MAX_BUILDINGS = 160, SIDE = 0, RESCUABLE = 3,
       ENEMY = 1, NEUTRAL = 2 };
typedef struct {
    int x,y,type,owner,raw_type,raw_owner,alive,selected,target,order;
} Unit;
typedef struct { int x,y,type,owner,raw_type,raw_owner,alive; } Building;
extern Unit u[MAX_UNITS];
extern Building b[MAX_BUILDINGS];
extern u8 invincible_units[MAX_UNITS], invincible_buildings[MAX_BUILDINGS];
extern int sc64_race_raw_unit(int type);
extern int sc64_race_raw_building(int type);
static int is_men(void *context, unsigned raw)
{ (void)context; return raw != 101u && raw != 218u; }
#include "../starcraft/sc64_p6_world_adapter.h"

int sc64_p6_reference_count(const Sc64P6WorldContext *context,
                            unsigned selector, unsigned wanted,
                            unsigned location, u32 *count)
{ return sc64_p6_world_count(context, selector, wanted, location, count); }

typedef struct {
    Sc64P6WorldContext *world;
    u8 *actions;
    unsigned *action_count;
} ReferenceOpsContext;

static int reference_condition(void *opaque, const Sc64P6RuntimeState *state,
                               const Sc64P6SourceCondition *condition)
{
    ReferenceOpsContext *context = opaque;
    return sc64_p6_world_condition(context->world, state, condition);
}

static int reference_action(void *opaque, const Sc64P6SourceAction *action)
{
    ReferenceOpsContext *context = opaque;
    unsigned n = (*context->action_count)++;
    if (!action || n >= SC64_P6_ACTION_TOTAL) return 0;
    context->actions[n * 3u] = action->trigger;
    context->actions[n * 3u + 1u] = action->index;
    context->actions[n * 3u + 2u] = action->opcode;
    return 1;
}

static int reference_ai(void *opaque, const Sc64P6SourceAction *action)
{ return reference_action(opaque, action); }

int sc64_p6_reference_tick(Sc64P6RuntimeState *state,
                           Sc64P6WorldContext *world, int paused,
                           u8 *actions, unsigned *action_count)
{
    ReferenceOpsContext context = {world, actions, action_count};
    Sc64P6WorldOps ops = {&context, reference_condition,
                          reference_action, reference_ai};
    return sc64_p6_adapter_tick_policy(state, &ops, paused, 1, 1);
}
