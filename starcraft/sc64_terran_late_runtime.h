#ifndef SC64_TERRAN_LATE_RUNTIME_H
#define SC64_TERRAN_LATE_RUNTIME_H

#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_T4_RUNTIME)
#include "sc64_t4_triggers.h"

/* The four audited late Terran maps contain at most twelve triggers.  The
 * execution records retain the shared T4 schema, but the source condition
 * comparison values used here follow the late-map native convention. */
enum {
    SC64_TERRAN_LATE_TRIGGER_LIMIT = 12,
    SC64_TERRAN_LATE_HZ = 30,
    SC64_TERRAN_LATE_PASS_TICKS = 30,
    SC64_TERRAN_LATE_MAX_WAIT_MS = 25000,
    SC64_TERRAN_LATE_CONDITION_FAULT = 1,
    SC64_TERRAN_LATE_ACTION_FAULT = 2,
    SC64_TERRAN_LATE_WAIT_FAULT = 3,
    SC64_TERRAN_LATE_ENABLED_ALL = 17,
    SC64_TERRAN_LATE_ENABLED_FORCE1 = 18,
    SC64_TERRAN_LATE_ENABLED_FORCE4 = 21,
    SC64_TERRAN_LATE_WON = 1,
    SC64_TERRAN_LATE_LOST = 2
};

typedef struct {
    Sc64T4TriggerState player[8];
    Sc64T4TriggerSwitches switches;
} Sc64TerranLateRuntimeState;
_Static_assert(sizeof(Sc64TerranLateRuntimeState) == 1184,
               "late Terran runtime state budget");

/* World callbacks are typed numeric boundaries. Count callbacks implement
 * the mission-specific Command/Bring/Kills/Deaths operands; effect callbacks
 * implement audited world actions. Return 1 for complete, 0 for an atomic
 * retry, and -1 for unsupported data. A retry may not partially mutate. */
typedef struct {
    int (*count)(void *context, const Sc64T4Condition *condition,
                 unsigned source_owner, u32 *count);
    int (*action)(void *context, const Sc64T4Action *action,
                  unsigned source_owner);
} Sc64TerranLateRuntimeOps;

static int sc64_terran_late_program_valid(const Sc64T4Scenario *program);
static int sc64_terran_late_runtime_init(Sc64TerranLateRuntimeState *state,
                                        const Sc64T4Scenario *program,
                                        const Sc64T4SourceSetup *setup);
static int sc64_terran_late_runtime_state_valid(
    const Sc64TerranLateRuntimeState *state,
    const Sc64T4Scenario *program, const Sc64T4SourceSetup *setup);
static int sc64_terran_late_runtime_step(
    Sc64TerranLateRuntimeState *state, const Sc64T4Scenario *program,
    const Sc64T4SourceSetup *setup, const Sc64TerranLateRuntimeOps *ops,
    void *context, int paused);
static int sc64_terran_late_runtime_step_prevalidated(
    Sc64TerranLateRuntimeState *state, const Sc64T4Scenario *program,
    const Sc64T4SourceSetup *setup, const Sc64TerranLateRuntimeOps *ops,
    void *context, int paused);

static int
sc64_terran_late_enabled(const Sc64T4Trigger *trigger, unsigned owner,
                         unsigned force)
{
    if (!trigger || owner >= 8 || force < 1 || force > 4) return 0;
    return trigger->enabled_players[owner] ||
           trigger->enabled_players[SC64_TERRAN_LATE_ENABLED_ALL] ||
           trigger->enabled_players[SC64_TERRAN_LATE_ENABLED_FORCE1 + force - 1];
}

static int
sc64_terran_late_program_valid(const Sc64T4Scenario *program)
{
    if (!program || !program->triggers || !program->trigger_count ||
        program->trigger_count > SC64_TERRAN_LATE_TRIGGER_LIMIT) return 0;
    for (unsigned i = 0; i < program->trigger_count; ++i) {
        const Sc64T4Trigger *trigger = &program->triggers[i];
        if (!trigger->conditions || !trigger->actions ||
            trigger->initial_cursor || trigger->execution_flags) return 0;
        for (unsigned p = 0; p < 27; ++p) {
            if (trigger->enabled_players[p] > 1) return 0;
            if (trigger->enabled_players[p] &&
                !(p < 8 || p == SC64_TERRAN_LATE_ENABLED_ALL ||
                  (p >= SC64_TERRAN_LATE_ENABLED_FORCE1 &&
                   p <= SC64_TERRAN_LATE_ENABLED_FORCE4))) return 0;
        }
        int condition_end = 0, action_end = 0;
        for (unsigned slot = 0; slot < 16; ++slot) {
            const Sc64T4Condition *c = &trigger->conditions[slot];
            if (!c->opcode) { condition_end = 1; continue; }
            if (condition_end) return 0;
            /* Disabled source conditions are unconditional true, even when
             * their remaining numeric operands are not interpreted. */
            if (c->flags & ~0x12u) return 0;
            if (c->flags & 2) continue;
            switch (c->opcode) {
            case 2: case 3: case 5: case 15: break;
            case 11:
                if (c->location || c->player || c->amount || c->unit ||
                    c->mask || c->flags) return 0;
                if (c->comparison != 2 && c->comparison != 3) return 0;
                break;
            case 12:
                if (c->location || c->player || c->unit || c->resource ||
                    c->mask || c->flags) return 0;
                if (c->comparison != 0 && c->comparison != 1 &&
                    c->comparison != 10) return 0;
                break;
            case 22: case 23:
                if (c->location || c->player || c->amount || c->unit ||
                    c->comparison || c->resource || c->mask || c->flags)
                    return 0;
                break;
            default: return 0;
            }
            if ((c->opcode == 2 || c->opcode == 3 || c->opcode == 5 ||
                 c->opcode == 15) && c->comparison != 0 &&
                c->comparison != 1 && c->comparison != 10) return 0;
            if ((c->opcode == 2 || c->opcode == 3 || c->opcode == 5 ||
                 c->opcode == 15) && c->mask) return 0;
        }
        for (unsigned slot = 0; slot < 64; ++slot) {
            const Sc64T4Action *a = &trigger->actions[slot];
            if (a->flags & 2) continue;
            if (!a->opcode) { action_end = 1; continue; }
            if (action_end) return 0;
            switch (a->opcode) {
            case 1: case 2: case 3:
                if (a->location || a->string_id || a->wav_id || a->time ||
                    a->player || a->argument || a->unit || a->modifier ||
                    a->flags || a->trailing[0] || a->trailing[1] ||
                    a->trailing[2]) return 0;
                break;
            case 4:
                if (a->location || a->string_id || a->wav_id || a->player ||
                    a->argument || a->unit || a->modifier ||
                    (a->flags != 0 && a->flags != 4) || a->trailing[0] ||
                    a->trailing[1] || a->trailing[2] ||
                    a->time > SC64_TERRAN_LATE_MAX_WAIT_MS) return 0;
                break;
            case 13:
                if (a->location || a->string_id || a->wav_id || a->time ||
                    a->player || a->argument >= 256 || a->unit ||
                    a->modifier < 4 || a->modifier > 6 || a->flags ||
                    a->trailing[0] || a->trailing[1] || a->trailing[2])
                    return 0;
                break;
            case 7: case 8: case 9:
            case 10: case 11: case 12: case 15: case 16:
            case 26: case 38: case 43: break;
            default: return 0;
            }
        }
    }
    return 1;
}

static int
sc64_terran_late_compare(u32 count, u32 amount, unsigned comparison)
{
    switch (comparison) {
    case 0: return count >= amount;
    case 1: return count <= amount;
    case 10: return count == amount;
    default: return -1;
    }
}

static int
sc64_terran_late_condition(const Sc64T4TriggerState *player,
                           const Sc64T4Condition *condition,
                           const Sc64T4TriggerSwitches *switches,
                           const Sc64TerranLateRuntimeOps *ops, void *context)
{
    if (condition->flags & 2) return 1;
    if (condition->flags & ~0x10u) return -1;
    u32 count = 0;
    switch (condition->opcode) {
    case 2: case 3: case 5: case 15:
        if (!ops || !ops->count ||
            ops->count(context, condition, player->owner, &count) != 1)
            return -1;
        return sc64_terran_late_compare(count, condition->amount,
                                        condition->comparison);
    case 11: {
        if (!switches) return -1;
        unsigned bit = condition->resource;
        int is_set = !!(switches->bits[bit >> 3] & (1u << (bit & 7)));
        return condition->comparison == 2 ? is_set :
               condition->comparison == 3 ? !is_set : -1;
    }
    case 12:
        return sc64_terran_late_compare(
            player->elapsed_ticks / SC64_TERRAN_LATE_HZ,
            condition->amount, condition->comparison);
    case 22: return 1;
    case 23: return 0;
    default: return -1;
    }
}

static int
sc64_terran_late_wait_ticks(u32 milliseconds, u16 *result)
{
    if (!result || milliseconds > SC64_TERRAN_LATE_MAX_WAIT_MS) return 0;
    u32 seconds = milliseconds / 1000u;
    u32 ticks = seconds * SC64_TERRAN_LATE_HZ +
        (milliseconds % 1000u * SC64_TERRAN_LATE_HZ + 999u) / 1000u;
    if (!ticks) ticks = 1;
    if (ticks > 65535u) return 0;
    *result = (u16)ticks;
    return 1;
}

static void
sc64_terran_late_finish(Sc64T4TriggerState *player, unsigned index)
{
    player->flags[index] = (player->flags[index] & SC64_T4_TR_PRESERVE) ?
        0 : SC64_T4_TR_DONE;
    player->cursor[index] = 0;
}

static int
sc64_terran_late_runtime_init(Sc64TerranLateRuntimeState *state,
                              const Sc64T4Scenario *program,
                              const Sc64T4SourceSetup *setup)
{
    if (!state || !setup || !sc64_terran_late_program_valid(program)) return 0;
    for (unsigned owner = 0; owner < 8; ++owner)
        if (setup->force_by_slot[owner] > 3) return 0;
    u8 *bytes = (u8 *)state;
    for (unsigned i = 0; i < sizeof(*state); ++i) bytes[i] = 0;
    for (unsigned owner = 0; owner < 8; ++owner) {
        Sc64T4TriggerState *player = &state->player[owner];
        player->owner = (u8)owner;
        player->force = (u8)(setup->force_by_slot[owner] + 1);
        if (player->force > 4) return 0;
        player->active_wait = 255;
    }
    return 1;
}

static int
sc64_terran_late_runtime_state_valid(const Sc64TerranLateRuntimeState *state,
                                     const Sc64T4Scenario *program,
                                     const Sc64T4SourceSetup *setup)
{
    if (!state || !setup || !sc64_terran_late_program_valid(program)) return 0;
    for (unsigned owner = 0; owner < 8; ++owner) {
        const Sc64T4TriggerState *player = &state->player[owner];
        if (player->owner != owner ||
            player->force != setup->force_by_slot[owner] + 1 ||
            player->force > 4 || player->cooldown >= SC64_TERRAN_LATE_PASS_TICKS ||
            player->outcome > SC64_TERRAN_LATE_LOST || player->fault ||
            player->fault_trigger || player->fault_slot || player->reserved)
            return 0;
        if (player->wait_ticks) {
            if (player->active_wait >= program->trigger_count) return 0;
            unsigned i = player->active_wait;
            if (!(player->flags[i] & (SC64_T4_TR_RUNNING|SC64_T4_TR_WAIT)) ||
                (player->flags[i] & (SC64_T4_TR_RUNNING|SC64_T4_TR_WAIT)) !=
                    (SC64_T4_TR_RUNNING|SC64_T4_TR_WAIT) ||
                player->cursor[i] >= 64) return 0;
            const Sc64T4Action *a = &program->triggers[i].actions[player->cursor[i]];
            u16 max_ticks;
            if (a->opcode != 4 || (a->flags & 2) ||
                !sc64_terran_late_wait_ticks(a->time, &max_ticks) ||
                player->wait_ticks > max_ticks) return 0;
        } else if (player->active_wait != 255) return 0;
        for (unsigned i = 0; i < SC64_T4_TRIGGER_LIMIT; ++i) {
            unsigned flags = player->flags[i];
            if (i >= program->trigger_count ||
                !sc64_terran_late_enabled(&program->triggers[i], owner,
                                          player->force)) {
                if (flags || player->cursor[i]) return 0;
                continue;
            }
            if (flags & ~(SC64_T4_TR_RUNNING|SC64_T4_TR_PRESERVE|
                          SC64_T4_TR_DONE|SC64_T4_TR_WAIT)) return 0;
            if (flags & SC64_T4_TR_DONE) {
                if (flags != SC64_T4_TR_DONE || player->cursor[i]) return 0;
            } else if (!(flags & SC64_T4_TR_RUNNING)) {
                if (flags || player->cursor[i]) return 0;
            } else if (player->cursor[i] >= 64) return 0;
            if (!!(flags & SC64_T4_TR_WAIT) !=
                (player->wait_ticks && player->active_wait == i)) return 0;
        }
    }
    return 1;
}

static int
sc64_terran_late_runtime_preflight(const Sc64TerranLateRuntimeState *state,
                                   const Sc64T4Scenario *program,
                                   const Sc64T4SourceSetup *setup)
{
    if (!state || !program || !program->triggers ||
        !program->trigger_count ||
        program->trigger_count > SC64_TERRAN_LATE_TRIGGER_LIMIT || !setup)
        return 0;
    for (unsigned owner = 0; owner < 8; ++owner) {
        const Sc64T4TriggerState *player = &state->player[owner];
        if (setup->force_by_slot[owner] > 3 || player->owner != owner ||
            player->force != setup->force_by_slot[owner] + 1 ||
            player->cooldown >= SC64_TERRAN_LATE_PASS_TICKS ||
            player->outcome > SC64_TERRAN_LATE_LOST || player->fault ||
            player->fault_trigger || player->fault_slot || player->reserved)
            return 0;
        if (!player->wait_ticks) {
            if (player->active_wait != 255) return 0;
        } else {
            if (player->active_wait >= program->trigger_count) return 0;
            unsigned i = player->active_wait;
            if (!program->triggers[i].actions || player->cursor[i] >= 64 ||
                (player->flags[i] & ~(SC64_T4_TR_RUNNING|SC64_T4_TR_PRESERVE|
                                      SC64_T4_TR_DONE|SC64_T4_TR_WAIT)) ||
                (player->flags[i] & (SC64_T4_TR_RUNNING|SC64_T4_TR_WAIT)) !=
                    (SC64_T4_TR_RUNNING|SC64_T4_TR_WAIT) ||
                (player->flags[i] & SC64_T4_TR_DONE))
                return 0;
            const Sc64T4Action *a =
                &program->triggers[i].actions[player->cursor[i]];
            u16 maximum;
            if (a->opcode != 4 || (a->flags & 2) ||
                !sc64_terran_late_wait_ticks(a->time, &maximum) ||
                player->wait_ticks > maximum) return 0;
        }
    }
    return 1;
}

static int
sc64_terran_late_slot_valid(const Sc64T4TriggerState *player,
                            const Sc64T4Trigger *trigger, unsigned index,
                            unsigned owner, unsigned force)
{
    unsigned flags = player->flags[index];
    unsigned cursor = player->cursor[index];
    if (flags & ~(SC64_T4_TR_RUNNING|SC64_T4_TR_PRESERVE|
                  SC64_T4_TR_DONE|SC64_T4_TR_WAIT)) return 0;
    if (!sc64_terran_late_enabled(trigger, owner, force))
        return flags == 0 && cursor == 0;
    if (flags & SC64_T4_TR_DONE)
        return flags == SC64_T4_TR_DONE && cursor == 0;
    if (!(flags & SC64_T4_TR_RUNNING)) return flags == 0 && cursor == 0;
    if (cursor >= 64) return 0;
    if (flags & SC64_T4_TR_WAIT) {
        if (trigger->actions[cursor].opcode != 4) return 0;
        if (player->wait_ticks && player->active_wait != index) return 0;
        if (!player->wait_ticks && player->active_wait != 255) return 0;
    } else if (player->wait_ticks && player->active_wait == index) return 0;
    return 1;
}

static int
sc64_terran_late_runtime_step_core(Sc64TerranLateRuntimeState *state,
                                   const Sc64T4Scenario *program,
                                   const Sc64T4SourceSetup *setup,
                                   const Sc64TerranLateRuntimeOps *ops,
                                   void *context, int paused,
                                   int full_validation)
{
    if (full_validation) {
        if (!sc64_terran_late_runtime_state_valid(state, program, setup))
            return 0;
    } else if (!sc64_terran_late_runtime_preflight(state, program, setup))
        return 0;
    if (paused) return 1;
    for (unsigned owner = 0; owner < 8; ++owner) {
        Sc64T4TriggerState *player = &state->player[owner];
        if (player->outcome) continue;
        if (player->elapsed_ticks != 0xffffffffu) ++player->elapsed_ticks;
        int ready = player->cooldown == 0;
        if (player->wait_ticks && !--player->wait_ticks) {
            player->active_wait = 255;
            ready = 1;
        }
        if (!ready) { --player->cooldown; continue; }
        player->cooldown = SC64_TERRAN_LATE_PASS_TICKS - 1;
        for (unsigned ti = 0; ti < program->trigger_count; ++ti) {
            const Sc64T4Trigger *trigger = &program->triggers[ti];
            /* The prevalidated entry skips immutable program validation, but
             * mutable trigger slots are checked before every ready-pass use. */
            if (!sc64_terran_late_slot_valid(
                    player, trigger, ti, owner, player->force)) return 0;
            if (!sc64_terran_late_enabled(trigger, owner, player->force) ||
                (player->flags[ti] & SC64_T4_TR_DONE)) continue;
            if (!(player->flags[ti] & SC64_T4_TR_RUNNING)) {
                int match = 1;
                for (unsigned ci = 0; ci < 16; ++ci) {
                    const Sc64T4Condition *condition = &trigger->conditions[ci];
                    if (!condition->opcode) break;
                    int result = sc64_terran_late_condition(
                        player, condition, &state->switches, ops, context);
                    if (result < 0) {
                        player->fault = SC64_TERRAN_LATE_CONDITION_FAULT;
                        player->fault_trigger = (u8)ti;
                        player->fault_slot = (u8)ci;
                        return 0;
                    }
                    if (!result) { match = 0; break; }
                }
                if (!match) continue;
                player->flags[ti] = SC64_T4_TR_RUNNING;
            }
            for (unsigned ai = player->cursor[ti]; ai < 64; ++ai) {
                const Sc64T4Action *action = &trigger->actions[ai];
                player->cursor[ti] = (u8)ai;
                if (action->flags & 2) {
                    if (ai == 63) sc64_terran_late_finish(player, ti);
                    continue;
                }
                if (!action->opcode) { sc64_terran_late_finish(player, ti); break; }
                if (action->opcode == 3) player->flags[ti] |= SC64_T4_TR_PRESERVE;
                else if (action->opcode == 4) {
                    if (player->wait_ticks) break;
                    if (player->flags[ti] & SC64_T4_TR_WAIT) {
                        player->flags[ti] &= (u8)~SC64_T4_TR_WAIT;
                    } else {
                        if (!sc64_terran_late_wait_ticks(action->time,
                                                         &player->wait_ticks)) {
                            player->fault = SC64_TERRAN_LATE_WAIT_FAULT;
                            player->fault_trigger = (u8)ti;
                            player->fault_slot = (u8)ai;
                            return 0;
                        }
                        player->active_wait = (u8)ti;
                        player->flags[ti] |= SC64_T4_TR_WAIT;
                        break;
                    }
                } else if (action->opcode == 13) {
                    if (action->argument >= 256 || action->modifier < 4 ||
                        action->modifier > 6) {
                        player->fault = SC64_TERRAN_LATE_ACTION_FAULT;
                        player->fault_trigger = (u8)ti;
                        player->fault_slot = (u8)ai;
                        return 0;
                    }
                    u8 *byte = &state->switches.bits[action->argument >> 3];
                    unsigned bit = 1u << (action->argument & 7);
                    if (action->modifier == 4) *byte |= (u8)bit;
                    else if (action->modifier == 5) *byte &= (u8)~bit;
                    else *byte ^= (u8)bit;
                } else if (action->opcode == 1 || action->opcode == 2) {
                    if (action->opcode == 2 || player->outcome != SC64_TERRAN_LATE_LOST)
                        player->outcome = action->opcode == 2 ?
                            SC64_TERRAN_LATE_LOST : SC64_TERRAN_LATE_WON;
                } else {
                    int result = ops && ops->action ?
                        ops->action(context, action, owner) : -1;
                    if (result != 0 && result != 1) {
                        player->fault = SC64_TERRAN_LATE_ACTION_FAULT;
                        player->fault_trigger = (u8)ti;
                        player->fault_slot = (u8)ai;
                        return 0;
                    }
                    if (!result) break;
                }
                if (ai == 63) sc64_terran_late_finish(player, ti);
            }
        }
    }
    return 1;
}

static int
sc64_terran_late_runtime_step(Sc64TerranLateRuntimeState *state,
                              const Sc64T4Scenario *program,
                              const Sc64T4SourceSetup *setup,
                              const Sc64TerranLateRuntimeOps *ops,
                              void *context, int paused)
{
    return sc64_terran_late_runtime_step_core(state, program, setup, ops,
                                              context, paused, 1);
}

/* Caller must preserve the full program/state validation invariant after
 * init/load. Scalar and wait bounds are checked every frame; mutable slots
 * are checked immediately before a ready pass uses them. */
static int
sc64_terran_late_runtime_step_prevalidated(
    Sc64TerranLateRuntimeState *state, const Sc64T4Scenario *program,
    const Sc64T4SourceSetup *setup, const Sc64TerranLateRuntimeOps *ops,
    void *context, int paused)
{
    return sc64_terran_late_runtime_step_core(state, program, setup, ops,
                                              context, paused, 0);
}
#endif /* SC64_TERRAN_LATE_RUNTIME && SC64_T4_RUNTIME */
#endif
