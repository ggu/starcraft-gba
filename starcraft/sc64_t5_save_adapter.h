#ifndef SC64_T5_SAVE_ADAPTER_H
#define SC64_T5_SAVE_ADAPTER_H

#if defined(SC64_T5_RUNTIME) && defined(SC64_T4_RUNTIME) && \
    defined(SC64_T4_SCENARIO_RUNTIME) && defined(SC64_UNIFIED)

/* Compact persistence for the four T5 source-player contexts that can run.
 * The other contexts are reconstructed from the immutable source setup. */
static unsigned
race_t5_save_source_slot(unsigned saved)
{
    static const u8 slots[4] = { 0, 2, 3, 6 };
    return slots[saved];
}

static int
race_t5_save_resource_valid(const Sc64ResourceActionState *r, u32 grant)
{
    if (!r) return 0;
    /* Source SetResources actions set both balances and both gathered totals
     * together. Once applied these trigger-owned snapshots stay unchanged;
     * ordinary harvesting/spending updates the live wallets, not this record. */
    if (grant == 0)
        return !r->current_minerals && !r->current_gas &&
               !r->total_minerals_gathered && !r->total_gas_gathered;
    return r->current_minerals == grant && r->current_gas == grant &&
           r->total_minerals_gathered == grant &&
           r->total_gas_gathered == grant;
}

static unsigned
race_t5_save_executed_action_limit(const Sc64T4TriggerState *player,
                                   unsigned trigger)
{
    if (player->flags[trigger] & SC64_T4_TR_DONE) {
        for (unsigned a = 0; a < 64; ++a)
            if (!sc64_t5_scenario.triggers[trigger].actions[a].opcode)
                return a;
        return 64;
    }
    if (!(player->flags[trigger] & SC64_T4_TR_RUNNING)) return 0;
    if (player->flags[trigger] & SC64_T4_TR_PRESERVE) return 0;
    /* The current cursor is the action being retried or a wait in progress;
     * only its strict prefix has completed and can have side effects. */
    return player->cursor[trigger];
}

static int
race_t5_save_execution_provenance_valid(const Sc64T4TriggerState *players[8],
                                        const Sc64T5AiState *ai,
                                        const Sc64ResourceActionState resources[8],
                                        const Sc64T4TriggerSwitches *switches)
{
    u8 expected_scripts[8] = { 0 };
    u8 granted[8] = { 0 };
    u8 expected_switches[32] = { 0 };
    if (!ai || !resources || !switches || !sc64_t5_ai_state_valid(ai) ||
        !sc64_t5_scenario_valid(&sc64_t5_scenario, &sc64_t5_source_setup))
        return 0;
    for (unsigned p = 0; p < 8; ++p) {
        if (!players[p]) continue;
        for (unsigned t = 0; t < sc64_t5_scenario.trigger_count; ++t) {
            const Sc64T4Trigger *tr = &sc64_t5_scenario.triggers[t];
            if (players[p]->flags[t] & SC64_T4_TR_PRESERVE) return 0;
            if (!sc64_t4_trigger_owner_enabled(
                    tr, p, (unsigned)sc64_t5_source_force(p))) continue;
            unsigned limit = race_t5_save_executed_action_limit(players[p], t);
            for (unsigned a = 0; a < limit; ++a) {
                const Sc64T4Action *action = &tr->actions[a];
                if (!action->opcode || (action->flags & 2)) continue;
                if (action->opcode == 15 || action->opcode == 16) {
                    u8 bit = sc64_t5_ai_script_bit(action->argument);
                    if (!bit || action->player != 0) return 0;
                    expected_scripts[p] |= bit;
                } else if (action->opcode == 26) {
                    if (action->player == 0 && action->argument == 300)
                        granted[SC64_T5_SOURCE_HUMAN] = 1;
                    else if (action->player == 19 && action->argument == 6000)
                        granted[SC64_T5_SOURCE_ENEMY] = 1;
                    else return 0;
                } else if (action->opcode == 13) {
                    if (action->argument > 255 || action->modifier < 4 ||
                        action->modifier > 6) return 0;
                    u8 *byte = &expected_switches[action->argument >> 3];
                    u8 bit = (u8)(1u << (action->argument & 7));
                    if (action->modifier == 4) *byte |= bit;
                    else if (action->modifier == 5) *byte &= (u8)~bit;
                    else *byte ^= bit;
                } else if (action->opcode == 3) {
                    /* No source T5 trigger uses Preserve; its history cannot
                     * be reconstructed from a cursor, so reject such state. */
                    return 0;
                }
            }
        }
    }
    for (unsigned p = 0; p < 8; ++p)
        if (ai->requested_scripts[p] != expected_scripts[p]) return 0;
    if (memcmp(switches->bits, expected_switches,
               sizeof(expected_switches)) != 0) return 0;
    for (unsigned p = 0; p < 8; ++p) {
        u32 expected = granted[p] ? (p == SC64_T5_SOURCE_HUMAN ? 300u : 6000u) : 0u;
        if (!race_t5_save_resource_valid(&resources[p], expected)) return 0;
    }
    return 1;
}

static int
race_t5_save_runtime_valid(const Sc64T5RuntimeState *runtime)
{
    Sc64T4TriggerState initial;
    const Sc64T4TriggerState *players[8] = { 0 };
    if (!runtime || runtime->active_mask != SC64_T5_ACTIVE_SOURCE_MASK) return 0;
    for (unsigned slot = 0; slot < 8; ++slot) {
        unsigned force = (unsigned)sc64_t5_source_force(slot);
        if (SC64_T5_ACTIVE_SOURCE_MASK & (1u << slot)) {
            const Sc64T4TriggerState *p = &runtime->player[slot];
            if (!sc64_t4_trigger_state_valid(p, &sc64_t5_scenario) ||
                p->owner != slot || p->force != force ||
                (slot != SC64_T5_SOURCE_HUMAN && p->outcome != 0)) return 0;
            players[slot] = p;
        } else {
            if (!sc64_t4_trigger_init(&initial, &sc64_t5_scenario, slot,
                                      force) ||
                memcmp(&runtime->player[slot], &initial,
                       sizeof(initial)) != 0) return 0;
        }
    }
    if (!race_t5_save_execution_provenance_valid(players, &runtime->ai,
                    runtime->resources, &runtime->switches)) return 0;
    if (runtime->message != 0 && runtime->message != 7 &&
        runtime->message != 8 && runtime->message != 9 &&
        runtime->message != 12) return 0;
    if (runtime->message_ticks > 750 ||
        ((!runtime->message_ticks) != (!runtime->message))) return 0;
    if (runtime->ping_ticks > 90 ||
        (runtime->ping_ticks == 0 && (runtime->ping_x || runtime->ping_y)) ||
        (runtime->ping_ticks &&
         (runtime->ping_x >= 96u * 16u || runtime->ping_y >= 96u * 16u)))
        return 0;
    return 1;
}

static int
race_t5_save_state_valid(const Sc64T5SaveState *state)
{
    const Sc64T4TriggerState *players[8] = { 0 };
    if (!state || state->active_mask != SC64_T5_ACTIVE_SOURCE_MASK) return 0;
    for (unsigned i = 0; i < 4; ++i) {
        unsigned slot = race_t5_save_source_slot(i);
        const Sc64T4TriggerState *p = &state->player[i];
        if (!sc64_t4_trigger_state_valid(p, &sc64_t5_scenario) ||
            p->owner != slot || p->force != (unsigned)sc64_t5_source_force(slot) ||
            (slot != SC64_T5_SOURCE_HUMAN && p->outcome != 0))
            return 0;
        players[slot] = p;
    }
    if (!race_t5_save_execution_provenance_valid(players, &state->ai,
                    state->resources, &state->switches)) return 0;
    if ((state->message != 0 && state->message != 7 && state->message != 8 &&
         state->message != 9 && state->message != 12) ||
        state->message_ticks > 750 ||
        ((!state->message_ticks) != (!state->message)) ||
        state->ping_ticks > 90 ||
        (!state->ping_ticks && (state->ping_x || state->ping_y)) ||
        (state->ping_ticks &&
         (state->ping_x >= 96u * 16u || state->ping_y >= 96u * 16u)))
        return 0;
    return 1;
}

static int
race_t5_save_mines_valid(const RaceSave *candidate, int width, int height)
{
#ifndef SC64_LATE_TERRAN_STORAGE
    (void)width;
    (void)height;
#endif
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
#ifdef SC64_LATE_TERRAN_STORAGE
        Unit decoded;
        if (!race_save_decode_unit(candidate, i, (unsigned)width * 16u,
                                   (unsigned)height * 16u,
                                   &decoded)) return 0;
#define T5_MINE(field) decoded.field
#else
        const s16 *row = candidate->units[i];
#define T5_MINE(field) row[offsetof(Unit, field) / sizeof(int)]
#endif
        if (!T5_MINE(alive) || T5_MINE(type) != U_SPIDER_MINE)
            continue;
        int carry = T5_MINE(carry);
        int timer = T5_MINE(work_timer);
        int target = T5_MINE(target);
        int raw = T5_MINE(raw_type);
        int raw_owner = T5_MINE(raw_owner);
        if (raw != 13 || raw_owner != SC64_T5_SOURCE_ENEMY ||
            T5_MINE(owner) != ENEMY ||
            candidate->invincible_units[i] ||
            T5_MINE(order) != ORDER_IDLE || T5_MINE(cd) != 0 ||
            carry < 0 || carry > SC64_T5_MINE_CHASING ||
            timer < 0 || timer > SC64_T5_MINE_ARM_TICKS) return 0;
        if (carry == 0) {
            if (timer || target != -1) return 0;
        } else if (carry == SC64_T5_MINE_ARMING) {
            if (timer <= 0 || target != -1) return 0;
        } else if (carry == SC64_T5_MINE_ARMED) {
            if (timer || target != -1) return 0;
        } else if (timer || target < 0 || target >= MAX_UNITS) return 0;
    }
#undef T5_MINE
    return 1;
}

static int
race_t5_save_pack(Sc64T5SaveState *dst, const Sc64T5RuntimeState *src)
{
    Sc64T5SaveState packed;
    if (!dst || !race_t5_save_runtime_valid(src)) return 0;
    memset(&packed, 0, sizeof(packed));
    for (unsigned i = 0; i < 4; ++i)
        packed.player[i] = src->player[race_t5_save_source_slot(i)];
    packed.switches = src->switches;
    memcpy(packed.resources, src->resources, sizeof(packed.resources));
    packed.ai = src->ai;
    packed.message_ticks = src->message_ticks;
    packed.ping_x = src->ping_x; packed.ping_y = src->ping_y;
    packed.ping_ticks = src->ping_ticks;
    packed.message = src->message; packed.active_mask = src->active_mask;
    if (!race_t5_save_state_valid(&packed)) return 0;
    *dst = packed;
    return 1;
}

static int
race_t5_save_expand(Sc64T5RuntimeState *dst, const Sc64T5SaveState *src)
{
    Sc64T5RuntimeState expanded;
    if (!dst || !race_t5_save_state_valid(src)) return 0;
    memset(&expanded, 0, sizeof(expanded));
    for (unsigned slot = 0; slot < 8; ++slot) {
        if (SC64_T5_ACTIVE_SOURCE_MASK & (1u << slot)) {
            unsigned saved = slot == 0 ? 0 : slot == 2 ? 1 :
                             slot == 3 ? 2 : 3;
            expanded.player[slot] = src->player[saved];
        } else if (!sc64_t4_trigger_init(&expanded.player[slot],
                                         &sc64_t5_scenario, slot,
                                         (unsigned)sc64_t5_source_force(slot))) {
            return 0;
        }
    }
    expanded.switches = src->switches;
    memcpy(expanded.resources, src->resources, sizeof(expanded.resources));
    expanded.ai = src->ai;
    expanded.message_ticks = src->message_ticks;
    expanded.ping_x = src->ping_x; expanded.ping_y = src->ping_y;
    expanded.ping_ticks = src->ping_ticks;
    expanded.message = src->message; expanded.active_mask = src->active_mask;
    if (!race_t5_save_runtime_valid(&expanded)) return 0;
    *dst = expanded;
    return 1;
}

static int
race_t5_save_candidate_valid(const RaceSave *candidate, int width, int height)
{
    const Sc64T5SaveState *state;
    if (!candidate || width != 96 || height != 96 ||
        candidate->state[0] != 0 ||
        candidate->state[2] != SC64_T5_GLOBAL_ID ||
        candidate->state[7] < 0 ||
        (u32)candidate->state[7] != candidate->campaign.elapsed_frames ||
        candidate->campaign.mission != SC64_T5_GLOBAL_ID ||
        candidate->campaign.phase > SC64_RACE_PHASE_FAILED) return 0;
    state = &candidate->t5;
    if (!race_t5_save_state_valid(state) ||
        !race_t5_save_mines_valid(candidate, width, height) ||
        state->message != candidate->campaign.message ||
        state->message_ticks != candidate->campaign.message_ticks ||
        candidate->campaign.objective_stage !=
            ((state->switches.bits[0] & 0x10u) ? 1u : 0u)) return 0;
    {
        const u8 *bytes = (const u8 *)state;
        for (unsigned i = offsetof(Sc64T5SaveState, active_mask) + 1;
             i < sizeof(*state); ++i)
            if (bytes[i]) return 0;
    }
    /* The compact member is smaller than the T4 union member. Require all
     * tail bytes to be canonical zero so the union has one encoding. */
    {
        const u8 *tail = (const u8 *)state + sizeof(*state);
        for (unsigned i = sizeof(*state); i < sizeof(candidate->t4); ++i)
            if (tail[i - sizeof(*state)]) return 0;
    }
    for (unsigned i = 0; i < 4; ++i) {
        const Sc64T4TriggerState *p = &state->player[i];
        if (p->elapsed_ticks != candidate->campaign.elapsed_frames) return 0;
    }
    if (state->ai.elapsed_ticks != candidate->campaign.elapsed_frames) return 0;
    if (candidate->state[18] < 0 || candidate->state[18] > 1 ||
        candidate->state[19] < 0 || candidate->state[19] > 1) return 0;
    if (state->player[0].outcome == 0) {
        if (candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_ACTIVE ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (state->player[0].outcome == SC64_T4_TR_WON) {
        if (!candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_COMPLETE ||
            !(candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
            !(candidate->state[3] & (1 << SC64_T5_GLOBAL_ID))) return 0;
    } else if (state->player[0].outcome == SC64_T4_TR_LOST) {
        if (candidate->state[18] || !candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_FAILED ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else return 0;
    return 1;
}

#endif /* T5 runtime save helpers */
#endif
