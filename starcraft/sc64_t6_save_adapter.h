#ifndef SC64_T6_SAVE_ADAPTER_H
#define SC64_T6_SAVE_ADAPTER_H

#if defined(SC64_T6_RUNTIME) && defined(SC64_T4_RUNTIME) && defined(SC64_T6_ENABLED)
#include "sc64_t6_scenario_adapter.h"
#include "sc64_t6_availability_adapter.h"

static unsigned
race_t6_save_action_limit(const Sc64T4TriggerState *player, unsigned trigger)
{
    if (player->flags[trigger] & SC64_T4_TR_DONE) {
        for (unsigned a = 0; a < 64; ++a)
            if (!sc64_t6_scenario.triggers[trigger].actions[a].opcode) return a;
        return 64;
    }
    if (!(player->flags[trigger] & SC64_T4_TR_RUNNING) ||
        (player->flags[trigger] & SC64_T4_TR_PRESERVE)) return 0;
    return player->cursor[trigger];
}

static u8
race_t6_save_ai_bit(u32 script)
{
    if (script == 0x75637352u) return SC64_T6_AI_RSCU;
    if (script == 0x36726554u) return SC64_T6_AI_T6A;
    if (script == 0x62366554u) return SC64_T6_AI_T6B;
    return 0;
}

static int
race_t6_save_ai_owner_valid(u8 bit, unsigned owner)
{
    if (bit == SC64_T6_AI_RSCU) return owner == 2 || owner == 6;
    if (bit == SC64_T6_AI_T6A) return owner == 4;
    if (bit == SC64_T6_AI_T6B)
        return owner == 1 || owner == 3 || owner == 5 || owner == 7;
    return 0;
}

static int
race_t6_save_provenance_valid(const Sc64T6MissionState *state)
{
    Sc64ResourceActionState expected_resources[8] = {{0}};
    u8 expected_ai[8] = {0};
    u8 expected_switches[32] = {0};
    for (unsigned p = 0; p < 8; ++p) {
        unsigned force = sc64_t6_source_setup.force_by_slot[p] + 1u;
        for (unsigned t = 0; t < sc64_t6_scenario.trigger_count; ++t) {
            const Sc64T4Trigger *trigger = &sc64_t6_scenario.triggers[t];
            if (!sc64_t4_trigger_owner_enabled(trigger, p, force)) continue;
            unsigned limit = race_t6_save_action_limit(&state->triggers.player[p], t);
            for (unsigned a = 0; a < limit; ++a) {
                const Sc64T4Action *action = &trigger->actions[a];
                if (!action->opcode) break;
                if (action->opcode == 15 || action->opcode == 16) {
                    u8 bit = race_t6_save_ai_bit(action->argument);
                    if (!race_t6_save_ai_owner_valid(bit, p)) return 0;
                    expected_ai[p] |= bit;
                } else if (action->opcode == 26) {
                    for (unsigned target = 0; target < 8; ++target) {
                        int selected = action->player == target;
                        if (action->player == 20 || action->player == 21)
                            selected = sc64_t6_source_setup.force_by_slot[target] ==
                                       action->player - 18;
                        if (selected &&
                            !sc64_resource_action_apply_one_player(
                                &expected_resources[target], action->unit,
                                action->modifier, action->argument)) return 0;
                    }
                } else if (action->opcode == 13) {
                    unsigned index = action->argument;
                    if (index >= 256) return 0;
                    u8 *byte = &expected_switches[index >> 3];
                    u8 bit = (u8)(1u << (index & 7));
                    if (action->modifier == 4) *byte |= bit;
                    else if (action->modifier == 5) *byte &= (u8)~bit;
                    else if (action->modifier == 6) *byte ^= bit;
                    else return 0;
                }
            }
        }
    }
    for (unsigned p = 0; p < 8; ++p)
        if (state->ai_requests[p] != expected_ai[p] ||
            memcmp(&state->resources[p], &expected_resources[p],
                   sizeof(expected_resources[p])) != 0) return 0;
    return memcmp(state->triggers.switches.bits, expected_switches,
                  sizeof(expected_switches)) == 0;
}

/* T6 stores all eight source-player contexts. Unlike the compact T4/T5
 * formats, no context is reconstructed or folded into another source slot. */
static int
race_t6_save_state_valid(const Sc64T6MissionState *state)
{
    if (!state || !sc64_t6_scenario_state_valid(&state->triggers)) return 0;
    if (state->message != 0 && state->message != 7 && state->message != 8 &&
        state->message != 9 && state->message != 12) return 0;
    if (state->message_ticks > 90 ||
        ((!state->message_ticks) != (!state->message)) ||
        state->ping_ticks || state->ping_x || state->ping_y ||
        state->reserved[0] || state->reserved[1])
        return 0;
    if (state->rescue_mask & (u8)~0x44u) return 0;
    for (unsigned p = 0; p < 8; ++p) {
        const Sc64ResourceActionState *r = &state->resources[p];
        if (r->current_minerals > 65535u || r->current_gas > 65535u ||
            (state->ai_requests[p] & (u8)~(SC64_T6_AI_RSCU |
                                           SC64_T6_AI_T6A |
                                           SC64_T6_AI_T6B))) return 0;
    }
    /* 160 is byte-aligned; this also makes the encoding canonical if the
     * actor limit changes in a later experiment. */
    for (unsigned i = SC64_T6_UNIT_LIMIT; i < sizeof(state->burrowed) * 8u; ++i)
        if (sc64_t6_burrowed_get(state, i)) return 0;
    return race_t6_save_provenance_valid(state);
}

static int
race_t6_save_pack(Sc64T6MissionState *dst, const Sc64T6MissionState *src)
{
    if (!dst || !race_t6_save_state_valid(src)) return 0;
    *dst = *src;
    return 1;
}

static int
race_t6_save_expand(Sc64T6MissionState *dst, const Sc64T6MissionState *src)
{
    if (!dst || !race_t6_save_state_valid(src)) return 0;
    *dst = *src;
    return 1;
}

static int
race_t6_save_candidate_queues_available(const RaceSave *candidate)
{
    if (!candidate || candidate->state[0] != 0 ||
        candidate->state[2] != SC64_T6_GLOBAL_ID) return 1;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
#ifdef SC64_T6_STORAGE
        Building decoded;
        if (!sc64_building_save_decode(&decoded, &candidate->buildings[i])) return 0;
        const Building *row = &decoded;
        int alive = row->alive, owner = row->owner, raw_owner = row->raw_owner;
        int count = row->queue_count, head = row->queue_head;
#else
        const s16 *row = candidate->buildings[i];
        int alive = row[offsetof(Building, alive) / sizeof(int)];
        int owner = row[offsetof(Building, owner) / sizeof(int)];
        int raw_owner = row[offsetof(Building, raw_owner) / sizeof(int)];
        int count = row[offsetof(Building, queue_count) / sizeof(int)];
        int head = row[offsetof(Building, queue_head) / sizeof(int)];
#endif
        if (!alive) continue;
        if (count < 0 || count > 5 || head < 0 || head > 4) return 0;
        int source = race_t6_product_source_slot(owner, raw_owner);
        for (int q = 0; q < count; ++q) {
#ifdef SC64_T6_STORAGE
            int type = row->queue[(head + q) % 5];
#else
            int type = row[offsetof(Building, queue) / sizeof(int) +
                           (head + q) % 5];
#endif
            if (type < 0 || type >= SC_RU_COUNT ||
                !race_t6_candidate_product_available(source,
                    sc64_race_raw_unit(type))) return 0;
        }
    }
    return 1;
}

static int
race_t6_save_burrow_valid(const RaceSave *candidate,
                          const Sc64T6MissionState *state,
                          int width, int height)
{
#ifndef SC64_LATE_TERRAN_STORAGE
    (void)width; (void)height;
#endif
    for (unsigned i = 0; i < SC64_T6_UNIT_LIMIT; ++i) {
#ifdef SC64_LATE_TERRAN_STORAGE
        Unit decoded;
        if (!race_save_decode_unit(candidate, i, (unsigned)width * 16u,
                                   (unsigned)height * 16u, &decoded)) return 0;
#define T6_BURROW(field) decoded.field
#else
        const s16 *row = candidate->units[i];
#define T6_BURROW(field) row[offsetof(Unit, field) / sizeof(int)]
#endif
        int alive = T6_BURROW(alive);
        int raw = T6_BURROW(raw_type);
        int flags = T6_BURROW(flags);
        int burrowed = sc64_t6_burrowed_get(state, i);
        int expected = alive && (raw == 37 || raw == 38) && (flags & 2);
        if (burrowed != expected) return 0;
    }
#undef T6_BURROW
    return 1;
}

static int
race_t6_save_candidate_valid(const RaceSave *candidate, int width, int height)
{
    const Sc64T6MissionState *state;
    if (!candidate || width != 96 || height != 96 ||
        candidate->state[0] != 0 || candidate->state[2] != SC64_T6_GLOBAL_ID ||
        candidate->state[7] < 0 ||
        (u32)candidate->state[7] != candidate->campaign.elapsed_frames ||
        candidate->campaign.mission != SC64_T6_GLOBAL_ID ||
        candidate->campaign.phase > SC64_RACE_PHASE_FAILED ||
        candidate->state[18] < 0 || candidate->state[18] > 1 ||
        candidate->state[19] < 0 || candidate->state[19] > 1) return 0;
    state = &candidate->t6;
    if (!race_t6_save_state_valid(state) ||
        state->message != candidate->campaign.message ||
        state->message_ticks != candidate->campaign.message_ticks ||
        !race_t6_save_burrow_valid(candidate, state, width, height)) return 0;
    for (unsigned p = 0; p < 8; ++p)
        if (state->triggers.player[p].elapsed_ticks !=
            candidate->campaign.elapsed_frames) return 0;
    if (state->triggers.player[0].outcome == 0) {
        if (candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_ACTIVE ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY))
            return 0;
    } else if (state->triggers.player[0].outcome == SC64_T4_TR_WON) {
        if (!candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_COMPLETE ||
            !(candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
            !(candidate->state[3] & (1 << SC64_T6_GLOBAL_ID))) return 0;
    } else if (state->triggers.player[0].outcome == SC64_T4_TR_LOST) {
        if (candidate->state[18] || !candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_FAILED ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY))
            return 0;
    } else return 0;
    if (!race_t6_save_candidate_queues_available(candidate)) return 0;
    return 1;
}

#endif /* SC64_T6_RUNTIME && SC64_T4_RUNTIME && SC64_T6_ENABLED */
#endif
