#ifndef SC64_T4_TRIGGERS_H
#define SC64_T4_TRIGGERS_H

#include "sc64_trigger_types.h"

#ifdef SC64_T4_RUNTIME
/* Authored bounded runner for one source-player trigger instance. Immutable
 * CHK operands stay in private ROM tables. The caller supplies world queries
 * and atomic world effects; an unknown effect never advances its cursor.
 * This is a 30 Hz GBA policy, not an emulation of the N64 trigger scheduler.
 * Each player has independent execution state and shares the mission-wide
 * switch bank. No source player is silently merged.
 */
enum {
    SC64_T4_TRIGGER_LIMIT = 64,
    SC64_T4_TRIGGER_HZ = 30,
    SC64_T4_TRIGGER_PASS_TICKS = 30,
    SC64_T4_TR_RUNNING = 1,
    SC64_T4_TR_PRESERVE = 2,
    SC64_T4_TR_DONE = 4,
    SC64_T4_TR_WAIT = 8,
    SC64_T4_TR_WON = 1,
    SC64_T4_TR_LOST = 2,
    SC64_T4_TR_BAD_CONDITION = 1,
    SC64_T4_TR_BAD_ACTION = 2,
    SC64_T4_TR_BAD_WAIT = 3
};

typedef struct {
    u8 bits[32];
} Sc64T4TriggerSwitches;

typedef struct {
    u32 elapsed_ticks;
    u16 wait_ticks; /* Shared by this source player's trigger instances. */
    u8 cursor[SC64_T4_TRIGGER_LIMIT];
    u8 flags[SC64_T4_TRIGGER_LIMIT];
    u8 owner, force, cooldown, outcome;
    u8 active_wait;
    u8 fault, fault_trigger, fault_slot, reserved;
} Sc64T4TriggerState;
_Static_assert(sizeof(Sc64T4TriggerState) == 144, "T4 per-player trigger state budget");
_Static_assert(sizeof(Sc64T4TriggerSwitches) == 32, "T4 shared switch state budget");

typedef struct {
    /* Return 1 with the count, or 0 for an unsupported query. */
    int (*count)(void *context, const Sc64T4Condition *condition,
                 unsigned owner, u32 *count);
    /* Return 1 only after completing an atomic effect; 0 means retry later,
     * -1 means unsupported. A retry must not have partially applied effects. */
    int (*action)(void *context, const Sc64T4Action *action, unsigned owner);
} Sc64T4TriggerOps;

static int sc64_t4_trigger_wait_duration(u32 milliseconds, u16 *ticks);

static int
sc64_t4_trigger_owner_enabled(const Sc64T4Trigger *tr, unsigned owner,
                              unsigned force)
{
    if (!tr || owner >= 8 || force < 1 || force > 4) return 0;
    return tr->enabled_players[owner] || tr->enabled_players[17] ||
           tr->enabled_players[17 + force];
}

static int
sc64_t4_trigger_program_valid(const Sc64T4Scenario *program)
{
    if (!program || !program->triggers || !program->trigger_count ||
        program->trigger_count > SC64_T4_TRIGGER_LIMIT) return 0;
    for (unsigned i = 0; i < program->trigger_count; ++i) {
        const Sc64T4Trigger *tr = &program->triggers[i];
        /* Imported execution flags/cursors are retained by the importer.
         * Loading an already-running source program is not implemented. */
        if (!tr->conditions || !tr->actions || tr->execution_flags ||
            tr->initial_cursor) return 0;
        for (unsigned p = 0; p < 27; ++p)
            if (tr->enabled_players[p] > 1) return 0;
    }
    return 1;
}

static int
sc64_t4_trigger_state_valid(const Sc64T4TriggerState *s,
                            const Sc64T4Scenario *program)
{
    if (!s || !sc64_t4_trigger_program_valid(program) || s->owner >= 8 ||
        s->force < 1 || s->force > 4 || s->cooldown >= SC64_T4_TRIGGER_PASS_TICKS ||
        s->outcome > SC64_T4_TR_LOST || s->fault || s->fault_trigger ||
        s->fault_slot || s->reserved) return 0;
    if (s->wait_ticks) {
        if (s->active_wait >= program->trigger_count ||
            s->cursor[s->active_wait] >= 64 ||
            !(s->flags[s->active_wait] & SC64_T4_TR_WAIT)) return 0;
        const Sc64T4Action *a = &program->triggers[s->active_wait].actions[
            s->cursor[s->active_wait]];
        u16 maximum;
        if (a->opcode != 4 ||
            !sc64_t4_trigger_wait_duration(a->time, &maximum) ||
            s->wait_ticks > maximum) return 0;
    } else if (s->active_wait != 255) return 0;
    for (unsigned i = 0; i < SC64_T4_TRIGGER_LIMIT; ++i) {
        unsigned flags = s->flags[i];
        if (flags & ~(SC64_T4_TR_RUNNING | SC64_T4_TR_PRESERVE |
                      SC64_T4_TR_DONE | SC64_T4_TR_WAIT)) return 0;
        if (i >= program->trigger_count ||
            !sc64_t4_trigger_owner_enabled(&program->triggers[i], s->owner, s->force)) {
            if (flags || s->cursor[i]) return 0;
            continue;
        }
        if (flags & SC64_T4_TR_DONE) {
            if (flags != SC64_T4_TR_DONE || s->cursor[i]) return 0;
        } else if (!(flags & SC64_T4_TR_RUNNING)) {
            if (flags || s->cursor[i]) return 0;
        } else {
            if (s->cursor[i] >= 64) return 0;
            if (flags & SC64_T4_TR_WAIT) {
                const Sc64T4Action *a = &program->triggers[i].actions[s->cursor[i]];
                if (a->opcode != 4 || (a->flags & 2)) return 0;
            }
        }
    }
    return 1;
}

static int
sc64_t4_trigger_init(Sc64T4TriggerState *s, const Sc64T4Scenario *program,
                     unsigned owner, unsigned force)
{
    if (!s || !sc64_t4_trigger_program_valid(program) || owner >= 8 ||
        force < 1 || force > 4) return 0;
    u8 *bytes = (u8 *)s;
    for (unsigned i = 0; i < sizeof(*s); ++i) bytes[i] = 0;
    s->owner = (u8)owner; s->force = (u8)force;
    s->active_wait = 255;
    return 1;
}

static int
sc64_t4_trigger_compare(u32 count, u32 amount, unsigned comparison)
{
    switch (comparison) {
    case 0: return count >= amount;
    case 1: return count <= amount;
    case 2: return count == amount;
    default: return -1;
    }
}

static int
sc64_t4_trigger_condition(const Sc64T4TriggerState *s,
                          const Sc64T4Condition *c,
                          const Sc64T4TriggerSwitches *switches,
                          const Sc64T4TriggerOps *ops,
                          void *context)
{
    u32 count = 0;
    if (c->flags & 2) return 1; /* Disabled source condition. */
    if (c->mask || (c->flags & ~0x12u)) return -1;
    switch (c->opcode) {
    case 2: case 3: /* Command / Bring: world/type/owner/elevation queries. */
        if (!ops || !ops->count || ops->count(context, c, s->owner, &count) != 1) return -1;
        return sc64_t4_trigger_compare(count, c->amount, c->comparison);
    case 11: { /* The raw switch index is zero based, in condition byte 16. */
        if (!switches) return -1;
        int set = !!(switches->bits[c->resource >> 3] & (1u << (c->resource & 7)));
        return c->comparison == 2 ? set : c->comparison == 3 ? !set : -1;
    }
    case 12:
        return sc64_t4_trigger_compare(s->elapsed_ticks / SC64_T4_TRIGGER_HZ,
                                       c->amount, c->comparison);
    case 22: return 1;
    case 23: return 0;
    default: return -1;
    }
}

static void
sc64_t4_trigger_fault(Sc64T4TriggerState *s, unsigned fault,
                      unsigned trigger, unsigned slot)
{
    s->fault = (u8)fault; s->fault_trigger = (u8)trigger; s->fault_slot = (u8)slot;
}

static int
sc64_t4_trigger_wait_duration(u32 milliseconds, u16 *ticks)
{
    /* Avoid a 32-bit multiply overflow and reject values the state cannot
     * represent. Wait(0) yields one tick instead of spinning a preserve loop. */
    u32 seconds = milliseconds / 1000;
    if (seconds > 65535u / SC64_T4_TRIGGER_HZ) return 0;
    u32 value = seconds * SC64_T4_TRIGGER_HZ +
                (milliseconds % 1000 * SC64_T4_TRIGGER_HZ + 999) / 1000;
    if (value > 65535) return 0;
    *ticks = (u16)(value ? value : 1);
    return 1;
}

static void
sc64_t4_trigger_finish(Sc64T4TriggerState *s, unsigned i)
{
    s->flags[i] = (s->flags[i] & SC64_T4_TR_PRESERVE) ? 0 : SC64_T4_TR_DONE;
    s->cursor[i] = 0;
}

/* One nominal 30 Hz simulation tick. Pausing freezes clocks and effects.
 * Conditions are sampled in source order every 30 ticks, or when a wait
 * expires. Running actions resume without rechecking conditions. Waits share
 * this player's timer: another Wait blocks while it is running, while other
 * eligible triggers may execute their actions. A completed wait retains its
 * action marker until its source-ordered instance can resume.
 * At most 64*64 actions and 64*16 conditions are visited in one pass.
 */
static int
sc64_t4_trigger_step(Sc64T4TriggerState *s, const Sc64T4Scenario *program,
                     Sc64T4TriggerSwitches *switches,
                     const Sc64T4TriggerOps *ops, void *context, int paused)
{
    if (!switches || !sc64_t4_trigger_state_valid(s, program)) return 0;
    if (paused || s->outcome) return 1;
    if (s->elapsed_ticks != 0xffffffffu) ++s->elapsed_ticks;
    int ready = s->cooldown == 0;
    if (s->wait_ticks && !--s->wait_ticks) {
        s->active_wait = 255;
        ready = 1;
    }
    if (!ready) { --s->cooldown; return 1; }
    s->cooldown = SC64_T4_TRIGGER_PASS_TICKS - 1;
    for (unsigned i = 0; i < program->trigger_count; ++i) {
        const Sc64T4Trigger *tr = &program->triggers[i];
        if (!sc64_t4_trigger_owner_enabled(tr, s->owner, s->force) ||
            (s->flags[i] & SC64_T4_TR_DONE)) continue;
        if (!(s->flags[i] & SC64_T4_TR_RUNNING)) {
            int match = 1;
            for (unsigned slot = 0; slot < 16; ++slot) {
                const Sc64T4Condition *c = &tr->conditions[slot];
                if (!c->opcode) break;
                int result = sc64_t4_trigger_condition(s, c, switches, ops, context);
                if (result < 0) {
                    sc64_t4_trigger_fault(s, SC64_T4_TR_BAD_CONDITION, i, slot);
                    return 0;
                }
                if (!result) { match = 0; break; }
            }
            if (!match) continue;
            s->flags[i] = SC64_T4_TR_RUNNING;
        }
        for (unsigned slot = s->cursor[i]; slot < 64; ++slot) {
            const Sc64T4Action *a = &tr->actions[slot];
            s->cursor[i] = (u8)slot;
            if (a->flags & 2) {
                if (slot == 63) sc64_t4_trigger_finish(s, i);
                continue;
            }
            if (!a->opcode) { sc64_t4_trigger_finish(s, i); break; }
            if (a->opcode == 3) s->flags[i] |= SC64_T4_TR_PRESERVE;
            else if (a->opcode == 4) {
                if (s->wait_ticks) break;
                if (s->flags[i] & SC64_T4_TR_WAIT) {
                    s->flags[i] &= (u8)~SC64_T4_TR_WAIT;
                } else {
                    if (!sc64_t4_trigger_wait_duration(a->time, &s->wait_ticks)) {
                        sc64_t4_trigger_fault(s, SC64_T4_TR_BAD_WAIT, i, slot);
                        return 0;
                    }
                    s->active_wait = (u8)i;
                    s->flags[i] |= SC64_T4_TR_WAIT;
                    break;
                }
            } else if (a->opcode == 13 && a->argument < 256 &&
                       a->modifier >= 4 && a->modifier <= 6) {
                u8 *byte = &switches->bits[a->argument >> 3];
                unsigned bit = 1u << (a->argument & 7);
                if (a->modifier == 4) *byte |= (u8)bit;
                else if (a->modifier == 5) *byte &= (u8)~bit;
                else *byte ^= (u8)bit;
            } else if (a->opcode == 1 || a->opcode == 2) {
                /* Explicit GBA tie policy: Defeat dominates Victory when
                 * both terminal actions execute in the same source pass. */
                if (a->opcode == 2 || s->outcome != SC64_T4_TR_LOST)
                    s->outcome = a->opcode == 2 ? SC64_T4_TR_LOST : SC64_T4_TR_WON;
            } else {
                int result = ops && ops->action ? ops->action(context, a, s->owner) : -1;
                if (result != 0 && result != 1) {
                    sc64_t4_trigger_fault(s, SC64_T4_TR_BAD_ACTION, i, slot);
                    return 0;
                }
                if (!result) break;
            }
            if (slot == 63) sc64_t4_trigger_finish(s, i);
        }
    }
    return 1;
}
#endif /* SC64_T4_RUNTIME */
#endif
