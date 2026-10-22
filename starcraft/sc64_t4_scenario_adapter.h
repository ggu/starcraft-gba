#ifndef SC64_T4_SCENARIO_ADAPTER_H
#define SC64_T4_SCENARIO_ADAPTER_H

#ifdef SC64_T4_SCENARIO_RUNTIME

#ifdef SC64_T4_COUNT_CACHE
#include "sc64_t4_count_cache.h"
#endif

/* T4's immutable numeric records stay in the private generated header.
 * This driver binds them to the bounded GBA pools. Immediate door state,
 * conservative tile collision and controller eligibility are authored GBA
 * policies; native animation timing and linked-list ordering are not emulated. */
static int
race_t4_doodad_action(const Sc64T4Action *action, unsigned owner)
{
    Sc64T4QueryRect area;
    unsigned next = 0, candidate = 32;
    if (!action || owner >= 8 || action->opcode != 42 ||
        action->player != 17 || action->argument || action->flags != 0x10 ||
        !race_t4_effect_location(&race_t4, action->location, &area)) return -1;
    for (unsigned row = 0; row < sc64_t4_scenario.doodad_count; ++row) {
        const Sc64T4Doodad *source = &sc64_t4_scenario.doodads[row];
        unsigned slot = race_t4.doodad_slot[row];
        Sc64T4QueryRect actor;
        if ((source->flags & SC64_T4_THG2_SPRITE_FLAG) ||
            source->type != action->unit) continue;
        if (slot >= MAX_BUILDINGS) return -1;
        if (!b[slot].alive) continue;
        if (b[slot].raw_type != source->type ||
            b[slot].raw_owner != source->owner ||
            !race_t4_effect_building_rect(&b[slot], &actor)) return -1;
        if (!sc64_t4_query_rect_overlap(&area, &actor)) continue;
        /* Every retained T4 doodad action has one geometrically eligible
         * target. Reject ambiguity rather than invent native list ordering.
         * AllPlayers here includes neutral doors, unlike Command queries. */
        if (candidate != 32 ||
            !sc64_t4_doodad_set_state(source->flags,
                race_t4.doodad_enabled[row], action->modifier, &next)) return -1;
        candidate = row;
    }
    if (candidate == 32) return 1; /* Removed/dead targets are a no-op. */
    race_t4.doodad_enabled[candidate] = (u8)next;
    invalidate_path_grid();
    return 1;
}

static int
race_t4_init(void)
{
    memset(&race_t4, 0, sizeof(race_t4));
    memset(race_t4.doodad_slot, 255, sizeof(race_t4.doodad_slot));
    if (!race_t4_active()) return 1;
    if (sc64_t4_scenario.doodad_count != 32 ||
        sc64_t4_scenario.location_count != 255 ||
        sc64_t4_scenario.trigger_count != 38 || mission_unsupported_count)
        return 0;
    unsigned needed = 0, free_slots = 0;
    for (int slot = 0; slot < MAX_BUILDINGS; ++slot) free_slots += !b[slot].alive;
    for (unsigned row = 0; row < 32; ++row) {
        const Sc64T4Doodad *source = &sc64_t4_scenario.doodads[row];
        if (source->flags & SC64_T4_THG2_SPRITE_FLAG) continue;
        int type = sc64_race_building_from_raw(source->type);
        if (type < 0 || (!sc64_race_t4_is_door(type) &&
                         !sc64_race_t4_is_trap(type)) ||
            source->x >= (unsigned)race_cols * 32 ||
            source->y >= (unsigned)race_rows * 32) return 0;
        ++needed;
    }
    if (needed != 26 || free_slots < needed) return 0;
    for (unsigned player = 0; player < 8; ++player) {
        unsigned controller = sc64_t4_source_setup.controllers[player];
        if (controller && controller != 3 && controller != 5 && controller != 6)
            return 0;
        if (!sc64_t4_trigger_init(&race_t4.player[player], &sc64_t4_scenario,
                                  player, sc64_t4_source_force((int)player))) return 0;
        /* Rescue-passive contexts are kept independent too. This explicit
         * eligibility policy does not claim the N64's opaque slot-state test. */
        if (controller) race_t4.active_mask |= (u8)(1u << player);
    }
    if (race_t4.active_mask != 0xdf) return 0;
    for (unsigned row = 0; row < 32; ++row) {
        const Sc64T4Doodad *source = &sc64_t4_scenario.doodads[row];
        unsigned enabled;
        if (source->flags & SC64_T4_THG2_SPRITE_FLAG) continue;
        int type = sc64_race_building_from_raw(source->type);
        int owner = sc64_race_t4_is_door(type) ? NEUTRAL :
                    sc64_t4_runtime_owner(source->owner);
        if (!sc64_t4_doodad_initial_enabled(source->flags, &enabled)) return 0;
        int slot = spawn_build(owner, type, source->x / 2, source->y / 2, 0);
        if (slot < 0) return 0;
        b[slot].raw_type = source->type;
        b[slot].raw_owner = source->owner;
        b[slot].flags = 0; /* THG2 flags are not UNIT invincibility flags. */
        race_t4.doodad_slot[row] = (u8)slot;
        race_t4.doodad_enabled[row] = (u8)enabled;
    }
    invalidate_path_grid();
    return 1;
}

static int
race_t4_tick(void)
{
    static const Sc64T4TriggerOps ops = {
        race_t4_trigger_count, race_t4_trigger_action
    };
#ifdef SC64_T4_COUNT_CACHE
    Sc64T4CountCache count_cache;
#endif
    if (!race_t4_active()) return 1;
#ifdef SC64_DIAGNOSTIC_UPDATE
    u32 t4_script_start = gba_clock();
    sc64_t4_script_cycles = sc64_t4_trap_cycles = 0;
#endif
    if (race_t4.message_ticks && !--race_t4.message_ticks) race_t4.message = 0;
    if (race_t4.ping_ticks && !--race_t4.ping_ticks)
        race_t4.ping_x = race_t4.ping_y = 0;
#ifdef SC64_T4_COUNT_CACHE
    /* All source-player contexts share counts for this simulation tick. A
     * fresh stack object scopes memoized values to this point in the update. */
    sc64_t4_count_cache_init(&count_cache, &ops, &race_t4);
#endif
    /* Fresh init and SRAM load fully validate every context. The scenario is
     * immutable ROM data; the runtime step preserves those state invariants. */
    for (unsigned player = 0; player < 8; ++player)
        if ((race_t4.active_mask & (1u << player)) &&
#ifdef SC64_T4_COUNT_CACHE
            !sc64_t4_trigger_step_prevalidated(&race_t4.player[player],
                                  &sc64_t4_scenario, &race_t4.switches,
                                  &sc64_t4_count_cache_ops, &count_cache, 0)) return 0;
#else
            !sc64_t4_trigger_step_prevalidated(&race_t4.player[player],
                                  &sc64_t4_scenario, &race_t4.switches,
                                  &ops, &race_t4, 0)) return 0;
#endif
    race_campaign.objective_stage = !!(race_t4.switches.bits[0] & 16);
    race_campaign.message = race_t4.message;
    race_campaign.message_ticks = race_t4.message_ticks;
    if (race_t4.player[0].outcome == SC64_T4_TR_LOST) {
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
        lost = 1;
    } else if (race_t4.player[0].outcome == SC64_T4_TR_WON) {
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        won = 1;
        /* The frontend/completion mask remain gated until controller proof. */
    }
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_t4_script_cycles = gba_clock() - t4_script_start;
    u32 t4_trap_start = gba_clock();
#endif
    if (!race_t4.player[0].outcome) race_t4_traps_tick();
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_t4_trap_cycles = gba_clock() - t4_trap_start;
#endif
    return 1;
}

#endif
#endif
