#ifndef SC64_T6_RUNTIME_H
#define SC64_T6_RUNTIME_H

#if defined(SC64_T6_RUNTIME) && defined(SC64_T4_RUNTIME)
#include "sc64_t4_triggers.h"

enum {
    SC64_T6_TRIGGER_COUNT = 20,
    SC64_T6_CONDITION_COUNT = 31,
    SC64_T6_ACTION_COUNT = 75,
    SC64_T6_PLAYER_SLOTS = 8,
    SC64_T6_SECONDS_HZ = 30,
    SC64_T6_SOURCE_WAIT_MAX = 25000
};
_Static_assert(SC64_T6_SECONDS_HZ == SC64_T4_TRIGGER_HZ,
               "T6 inherits the T4 core's 30 Hz clock");

/* The eight independent source-slot contexts deliberately include OWNR
 * inactive slots: source Force 4 trigger instances expand there. T4's
 * bounded per-context state is reused, while Deaths is delegated by the
 * T6-gated condition case in sc64_t4_triggers.h. */
typedef struct {
    Sc64T4TriggerState player[SC64_T6_PLAYER_SLOTS];
    Sc64T4TriggerSwitches switches;
} Sc64T6RuntimeState;
_Static_assert(sizeof(Sc64T6RuntimeState) == 1184,
               "T6 trigger state budget");

/* World callbacks return 1 on completion, 0 for an atomic retry, and -1 for
 * unsupported input. A retry must not partially apply the action. */
typedef struct {
    int (*count)(void *context, const Sc64T4Condition *condition,
                 unsigned source_owner, u32 *count);
    int (*create)(void *context, const Sc64T4Action *action,
                  unsigned source_owner);
    int (*resources)(void *context, const Sc64T4Action *action,
                     unsigned source_owner);
    int (*ai)(void *context, const Sc64T4Action *action,
              unsigned source_owner);
    int (*presentation)(void *context, const Sc64T4Action *action,
                        unsigned source_owner);
} Sc64T6RuntimeOps;

static int sc64_t6_runtime_program_valid(const Sc64T4Scenario *program,
                                        const Sc64T4SourceSetup *setup);
static int sc64_t6_runtime_properties_valid(
    const Sc64T4UnitProperties *properties, unsigned count,
    const Sc64T4SourceSetup *setup);
static int sc64_t6_runtime_init(Sc64T6RuntimeState *state,
                               const Sc64T4Scenario *program,
                               const Sc64T4SourceSetup *setup);
static int sc64_t6_runtime_state_valid(const Sc64T6RuntimeState *state,
                                      const Sc64T4Scenario *program,
                                      const Sc64T4SourceSetup *setup);
static int sc64_t6_runtime_step(Sc64T6RuntimeState *state,
                                const Sc64T4Scenario *program,
                                const Sc64T4SourceSetup *setup,
                                const Sc64T6RuntimeOps *ops,
                                void *context, int paused);
static int sc64_t6_runtime_step_prevalidated(
    Sc64T6RuntimeState *state, const Sc64T4Scenario *program,
    const Sc64T4SourceSetup *setup, const Sc64T6RuntimeOps *ops,
    void *context, int paused);

static int
sc64_t6_runtime_setup_valid(const Sc64T4SourceSetup *setup)
{
    static const u8 controllers[8] = {6,5,3,0,5,0,7,0};
    static const u8 forces[8] = {0,3,1,3,2,3,1,3};
    static const u8 races[8] = {1,0,1,1,0,2,1,2};
    if (!setup) return 0;
    for (unsigned i = 0; i < 8; ++i)
        if (setup->controllers[i] != controllers[i] ||
            setup->force_by_slot[i] != forces[i] || setup->races[i] != races[i] ||
            setup->force_raw[i] != forces[i]) return 0;
    for (unsigned i = 0; i < 64; ++i)
        if (setup->property_used[i] != (i < 30)) return 0;
    return 1;
}

static int
sc64_t6_bytes_zero(const void *data, unsigned size)
{
    const u8 *bytes = data;
    if (!data) return 0;
    for (unsigned i = 0; i < size; ++i) if (bytes[i]) return 0;
    return 1;
}

static int
sc64_t6_runtime_condition_valid(const Sc64T4Condition *c)
{
    if (!c || c->mask || (c->flags != 0 && c->flags != 16)) return 0;
    switch (c->opcode) {
    case 2: /* Command */
        if (c->location != 0 || c->resource != 0 || c->flags != 16) return 0;
        if (c->player == 19 && c->unit == 229)
            return c->amount == 1 && c->comparison == 0;
        return c->amount == 0 &&
               ((c->player == 0 && c->unit == 229 && c->comparison == 1) ||
                (c->player == 17 && (c->unit == 19 || c->unit == 126) &&
                 c->comparison == 1));
    case 3: /* Bring */
        if (c->resource || c->flags != 16) return 0;
        if (c->player == 13 && c->location == 2 && c->comparison == 0)
            return (c->unit == 19 && c->amount == 1) ||
                   (c->unit == 11 && c->amount == 2);
        if (c->player == 19 && c->unit == 229 && c->location == 0)
            return c->comparison == 0 && c->amount == 1;
        if (c->player == 14 && c->unit == 229 && c->location == 3)
            return c->comparison == 0 && c->amount == 1;
        if (c->player == 0 && c->unit == 229 && c->location == 2)
            return c->comparison == 0 && c->amount == 1;
        if (c->player == 0 && c->unit == 106 && c->location == 1)
            return c->comparison == 0 && c->amount == 1;
        if (c->player == 0 && c->unit == 122 && c->location == 5)
            return (c->comparison == 0 && c->amount == 1) ||
                   (c->comparison == 1 && c->amount == 0);
        if ((c->player == 0 || c->player == 6) && c->unit == 122 &&
            c->location == 5)
            return c->comparison == 1 && c->amount == 0;
        if (c->player == 1 && c->unit == 229 && c->location == 1)
            return c->comparison == 1 && c->amount == 0;
        return 0;
    case 11: /* Switch */
        return c->player == 0 && c->unit == 0 && c->location == 0 &&
               c->amount == 0 && c->flags == 0 &&
               (c->resource == 2 || c->resource == 10) &&
               (c->comparison == 2 || c->comparison == 3);
    case 12: /* Elapsed time */
        return c->player == 0 && c->unit == 0 && c->location == 0 &&
               c->resource == 0 && c->flags == 0 &&
               (c->comparison == 0 || c->comparison == 1) &&
               (c->amount == 0 || c->amount == 180 || c->amount == 360 ||
                c->amount == 1200 || c->amount == 2700 ||
                c->amount == 7000 || c->amount == 10000);
    case 15: /* Deaths */
        return c->location == 0 && c->resource == 0 && c->unit == 231 &&
               c->amount == 1 && c->comparison == 0 && c->flags == 16 &&
               (c->player == 1 || c->player == 4);
    default: return 0;
    }
}

static int
sc64_t6_runtime_action_valid(const Sc64T4Action *a)
{
    if (!a || (a->flags & 2)) return 0;
    if (!sc64_t6_bytes_zero(a->trailing, sizeof(a->trailing))) return 0;
    switch (a->opcode) {
    case 1: case 2: /* Victory / Defeat */
        return a->location == 0 && a->string_id == 0 && a->wav_id == 0 &&
               a->time == 0 && a->player == 0 && a->argument == 0 &&
               a->unit == 0 && a->modifier == 0 && a->flags == 0;
    case 4: /* Wait */
        return a->location == 0 && a->string_id == 0 && a->wav_id == 0 &&
               a->time <= SC64_T6_SOURCE_WAIT_MAX && a->player == 0 &&
               a->argument == 0 && a->unit == 0 && a->modifier == 0 &&
               (a->flags == 0 || a->flags == 4);
    case 7: /* Transmission */
        return a->location == 2 && a->string_id != 0 && a->wav_id == 10 &&
               a->time == 0 && a->player == 0 && a->argument <= 150 &&
               a->flags == 16 && (a->unit == 19 || a->unit == 27 ||
                                   a->unit == 126) &&
               (a->modifier == 8 || a->modifier == 9);
    case 8: /* Play WAV */
        return a->location == 0 && a->string_id == 0 && a->wav_id == 547 &&
               a->time == 403 && a->player == 0 && a->flags == 0;
    case 9: /* Display text */
        return a->location == 0 && a->string_id != 0 && a->wav_id == 0 &&
               a->time == 0 && a->player == 0 && a->argument == 0 &&
               a->unit == 0 && a->modifier == 0 && a->flags == 4;
    case 10: /* Move the view */
        return a->location != 0 && a->location <= 64 && a->string_id == 0 &&
               a->wav_id == 0 && a->time == 0 && a->player == 0 &&
               a->argument == 0 && a->unit == 0 && a->modifier == 0 &&
               a->flags == 0;
    case 11: /* Create Unit With Properties */
        return a->location == 2 && a->player == 4 &&
               (a->unit == 37 || a->unit == 38) && a->modifier == 0 &&
               a->argument >= 1 && a->argument <= 30 && a->time == 0 &&
               a->string_id == 0 && a->wav_id == 0 && a->flags == 24;
    case 12: /* Display objective */
        return a->location == 0 && a->string_id != 0 && a->wav_id == 0 &&
               a->time == 0 && a->player == 0 && a->argument == 0 &&
               a->unit == 0 && a->modifier == 0 && a->flags == 0;
    case 13: /* Set Switch */
        return a->location == 0 && a->string_id == 0 && a->wav_id == 0 &&
               a->time == 0 && a->player == 0 && a->argument < 256 &&
               a->unit == 0 && a->modifier >= 4 && a->modifier <= 6 &&
               a->flags == 0;
    case 15: /* Run AI script */
    case 16: /* Run AI script at location */
        return a->string_id == 0 && a->wav_id == 0 && a->time == 0 &&
               a->unit == 0 && a->modifier == 0 && a->argument != 0 &&
               a->flags == 0 &&
               (a->opcode == 15 ? (a->location == 0) :
                                   (a->location != 0 && a->location <= 64));
    case 26: /* Set Resources */
        return a->location == 0 && a->string_id == 0 && a->wav_id == 0 &&
               a->argument != 0 &&
               a->time == 0 && a->modifier == 7 &&
               a->flags == 0 &&
               ((a->player == 0 && a->unit == 0 && a->argument == 75) ||
                ((a->player == 20 || a->player == 21) && a->unit == 2 &&
                 a->argument == 8000));
    default: return 0;
    }
}

static int
sc64_t6_runtime_program_valid(const Sc64T4Scenario *program,
                              const Sc64T4SourceSetup *setup)
{
    static const u8 expected_conditions[16] = {
        [2] = 4, [3] = 9, [11] = 9, [12] = 7, [15] = 2
    };
    static const u8 expected_actions[27] = {
        [1] = 1, [2] = 3, [4] = 7, [7] = 12, [8] = 3, [9] = 3,
        [10] = 3, [11] = 30, [12] = 1, [13] = 6, [15] = 1,
        [16] = 2, [26] = 3
    };
    unsigned conditions[16] = {0};
    unsigned actions[27] = {0};
    u32 property_refs = 0;
    if (!sc64_t4_trigger_program_valid(program) ||
        program->location_count != 64 || program->trigger_count != SC64_T6_TRIGGER_COUNT ||
        program->doodad_count != 54 || !sc64_t6_runtime_setup_valid(setup)) return 0;
    unsigned total_conditions = 0, total_actions = 0;
    for (unsigned i = 0; i < program->trigger_count; ++i) {
        const Sc64T4Trigger *tr = &program->triggers[i];
        unsigned ended = 0;
        for (unsigned p = 0; p < 27; ++p) {
            unsigned expected = (i == 0 || i == 3 || i == 4) ? p == 18 :
                (i == 6) ? p == 20 : (i == 7) ? p == 21 :
                (i == 8) ? p == 19 :
                (i == 1 || i == 2 || (i >= 5 && i <= 5) || i >= 9) ? p == 0 : 0;
            if (tr->enabled_players[p] != expected) return 0;
        }
        for (unsigned j = 0; j < 16; ++j) {
            const Sc64T4Condition *c = &tr->conditions[j];
            if (!c->opcode) {
                ended = 1;
                if (!sc64_t6_bytes_zero(c, sizeof(*c))) return 0;
                continue;
            }
            if (ended) return 0;
            if (!sc64_t6_runtime_condition_valid(c) || c->opcode >= 16) return 0;
            ++conditions[c->opcode]; ++total_conditions;
        }
        ended = 0;
        for (unsigned j = 0; j < 64; ++j) {
            const Sc64T4Action *a = &tr->actions[j];
            if (!a->opcode) {
                ended = 1;
                if (!sc64_t6_bytes_zero(a, sizeof(*a))) return 0;
                continue;
            }
            if (ended) return 0;
            if (!sc64_t6_runtime_action_valid(a) || a->opcode >= 27) return 0;
            if (a->opcode == 11) {
                u32 bit = (u32)1 << (a->argument - 1);
                if (property_refs & bit) return 0;
                property_refs |= bit;
            }
            ++actions[a->opcode]; ++total_actions;
        }
    }
    if (total_conditions != SC64_T6_CONDITION_COUNT ||
        total_actions != SC64_T6_ACTION_COUNT) return 0;
    for (unsigned i = 0; i < 16; ++i)
        if (conditions[i] != expected_conditions[i]) return 0;
    for (unsigned i = 0; i < 27; ++i)
        if (actions[i] != expected_actions[i]) return 0;
    return property_refs == (((u32)1 << 30) - 1);
}

static int
sc64_t6_runtime_properties_valid(const Sc64T4UnitProperties *properties,
                                unsigned count,
                                const Sc64T4SourceSetup *setup)
{
    if (!properties || count != 64 || !sc64_t6_runtime_setup_valid(setup)) return 0;
    for (unsigned i = 0; i < count; ++i) {
        const Sc64T4UnitProperties *p = &properties[i];
        if (i < 30) {
            if (p->valid_special != 0x1a || p->valid_fields != 2 ||
                p->owner != 0xff || p->hp_percent != 100 ||
                p->shield_percent || p->energy_percent || p->resources ||
                p->hangar_count || p->special != 2 || p->unused ||
                p->raw_record[0] != 0x1a || p->raw_record[1] != 0 ||
                p->raw_record[2] != 2 || p->raw_record[3] != 0 ||
                p->raw_record[4] != 0xff || p->raw_record[5] != 100 ||
                !sc64_t6_bytes_zero(p->raw_record + 6, 8) ||
                p->raw_record[14] != 2 || p->raw_record[15] != 0 ||
                !sc64_t6_bytes_zero(p->raw_record + 16, 4)) return 0;
        } else if (!sc64_t6_bytes_zero(p, sizeof(*p))) return 0;
    }
    return 1;
}

static int
sc64_t6_runtime_init(Sc64T6RuntimeState *state,
                     const Sc64T4Scenario *program,
                     const Sc64T4SourceSetup *setup)
{
    if (!state || !sc64_t6_runtime_program_valid(program, setup)) return 0;
    u8 *bytes = (u8 *)state;
    for (unsigned i = 0; i < sizeof(*state); ++i) bytes[i] = 0;
    for (unsigned owner = 0; owner < SC64_T6_PLAYER_SLOTS; ++owner) {
        unsigned force = setup->force_by_slot[owner] + 1u;
        if (force > 4 || !sc64_t4_trigger_init(&state->player[owner], program,
                                               owner, force)) return 0;
    }
    return 1;
}

static int
sc64_t6_runtime_state_valid(const Sc64T6RuntimeState *state,
                            const Sc64T4Scenario *program,
                            const Sc64T4SourceSetup *setup)
{
    if (!state || !sc64_t6_runtime_program_valid(program, setup)) return 0;
    for (unsigned owner = 0; owner < SC64_T6_PLAYER_SLOTS; ++owner) {
        const Sc64T4TriggerState *p = &state->player[owner];
        if (p->owner != owner || p->force != setup->force_by_slot[owner] + 1u ||
            !sc64_t4_trigger_state_valid(p, program)) return 0;
        if (owner && p->outcome) return 0; /* T6 terminal actions are slot 0. */
    }
    return 1;
}

typedef struct {
    const Sc64T6RuntimeOps *ops;
    void *context;
} Sc64T6Dispatch;

static int
sc64_t6_dispatch_count(void *opaque, const Sc64T4Condition *condition,
                       unsigned owner, u32 *count)
{
    Sc64T6Dispatch *d = opaque;
    if (!d || !d->ops || !d->ops->count || !condition ||
        (condition->opcode != 2 && condition->opcode != 3 &&
         condition->opcode != 15)) return -1;
    return d->ops->count(d->context, condition, owner, count);
}

static int
sc64_t6_dispatch_action(void *opaque, const Sc64T4Action *action,
                        unsigned owner)
{
    Sc64T6Dispatch *d = opaque;
    if (!d || !d->ops || !action || !sc64_t6_runtime_action_valid(action)) return -1;
    switch (action->opcode) {
    case 11:
        return d->ops->create ? d->ops->create(d->context, action, owner) : -1;
    case 26:
        return d->ops->resources ? d->ops->resources(d->context, action, owner) : -1;
    case 15: case 16:
        return d->ops->ai ? d->ops->ai(d->context, action, owner) : -1;
    case 7: case 8: case 9: case 10: case 12:
        return d->ops->presentation ?
            d->ops->presentation(d->context, action, owner) : -1;
    default: return -1;
    }
}

static int
sc64_t6_runtime_ops_valid(const Sc64T6RuntimeOps *ops)
{
    return ops && ops->count && ops->create && ops->resources && ops->ai &&
           ops->presentation;
}

static int
sc64_t6_runtime_step_prevalidated(Sc64T6RuntimeState *state,
                                  const Sc64T4Scenario *program,
                                  const Sc64T4SourceSetup *setup,
                                  const Sc64T6RuntimeOps *ops,
                                  void *context, int paused)
{
    /* The immutable source and full state must have passed their checked
     * validators at initialization or load. Keep only constant-size identity
     * checks here; each reused T4 context performs scalar/wait and ready-slot
     * preflights before it dereferences its execution state. */
    if (!state || !program || !setup || !program->triggers ||
        !sc64_t6_runtime_ops_valid(ops)) return 0;
    Sc64T6Dispatch dispatch = {ops, context};
    Sc64T4TriggerOps core_ops = {sc64_t6_dispatch_count,
                                 sc64_t6_dispatch_action};
    /* Slot order is deterministic. All slots execute their own context, even
     * if OWNR marks them inactive; source Force masks can enable those lists. */
    for (unsigned owner = 0; owner < SC64_T6_PLAYER_SLOTS; ++owner) {
        if (state->player[owner].owner != owner ||
            state->player[owner].force != setup->force_by_slot[owner] + 1u)
            return 0;
        if (!sc64_t4_trigger_step_prevalidated(&state->player[owner], program,
                    &state->switches, &core_ops, &dispatch, paused)) return 0;
    }
    return 1;
}

/* Checked entry point for setup/state received from outside the runtime. */
static int
sc64_t6_runtime_step(Sc64T6RuntimeState *state,
                     const Sc64T4Scenario *program,
                     const Sc64T4SourceSetup *setup,
                     const Sc64T6RuntimeOps *ops, void *context, int paused)
{
    if (!sc64_t6_runtime_state_valid(state, program, setup)) return 0;
    return sc64_t6_runtime_step_prevalidated(state, program, setup, ops,
                                              context, paused);
}

#endif /* SC64_T6_RUNTIME && SC64_T4_RUNTIME */
#endif
