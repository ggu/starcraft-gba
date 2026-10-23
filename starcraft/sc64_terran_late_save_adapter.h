#ifndef SC64_TERRAN_LATE_SAVE_ADAPTER_H
#define SC64_TERRAN_LATE_SAVE_ADAPTER_H

#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_T4_RUNTIME) && \
    defined(SC64_TERRAN_LATE_ACTORS)

#include "sc64_terran_late_world.h"

static int
race_terran_late_save_ai_action(u32 script)
{
    return sc64_terran_late_script_index(script);
}

static int
race_terran_late_save_product_available(int mission, unsigned source,
                                         int raw_type)
{
    const unsigned char *bits = 0;
    unsigned bytes = 0, available = 0;
    switch (mission) {
    case SC64_TERRAN_LATE_GLOBAL_T7:
        bits = sc64_t7_availability_bits;
        bytes = SC64_T7_AVAILABILITY_BIT_BYTES;
        break;
    case SC64_TERRAN_LATE_GLOBAL_T8:
        bits = sc64_t8_availability_bits;
        bytes = SC64_T8_AVAILABILITY_BIT_BYTES;
        break;
    case SC64_TERRAN_LATE_GLOBAL_T9:
        bits = sc64_t9_availability_bits;
        bytes = SC64_T9_AVAILABILITY_BIT_BYTES;
        break;
    case SC64_TERRAN_LATE_GLOBAL_T10:
        bits = sc64_t10_availability_bits;
        bytes = SC64_T10_AVAILABILITY_BIT_BYTES;
        break;
    default: return 0;
    }
    return source < SC64_PUNI_PLAYER_COUNT && raw_type >= 0 &&
           raw_type < (int)SC64_PUNI_UNIT_COUNT &&
           sc64_availability_bitmap_get(bits, bytes, source,
                                        (unsigned)raw_type, &available) &&
           available != 0;
}

static int
race_terran_late_save_queue_source(const Sc64T4SourceSetup *setup,
                                   int owner, int raw_owner)
{
    if (!setup) return -1;
    if (owner == SIDE) {
        int human = sc64_terran_late_source_slot_for_controller(
            setup, SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
        return raw_owner == human ? human : -1;
    }
    if (raw_owner < 0 || raw_owner >= 8 ||
        sc64_terran_late_runtime_owner_for_setup(setup,
                                                  (unsigned)raw_owner) != owner)
        return -1;
    return raw_owner;
}

static int
race_terran_late_save_queues_valid(const RaceSave *candidate, int mission)
{
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(mission);
    if (!candidate || !setup) return 0;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
#ifdef SC64_T6_STORAGE
        Building row;
        if (!race_save_decode_building(candidate, i, &row)) return 0;
        int alive = row.alive, owner = row.owner, raw_owner = row.raw_owner;
        int count = row.queue_count, head = row.queue_head;
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
        int source = race_terran_late_save_queue_source(setup, owner, raw_owner);
        for (int q = 0; q < count; ++q) {
#ifdef SC64_T6_STORAGE
            int type = row.queue[(head + q) % 5];
#else
            int type = row[offsetof(Building, queue) / sizeof(int) +
                           (head + q) % 5];
#endif
            if (type < 0 || type >= SC_RU_COUNT ||
                !race_terran_late_save_product_available(
                    mission, (unsigned)source, sc64_race_raw_unit(type)))
                return 0;
        }
    }
    return 1;
}

static int
race_terran_late_save_state_valid(const Sc64TerranLateMissionState *state,
                                  int mission, int width, int height,
                                  unsigned max_units)
{
    const Sc64T4Scenario *program =
        sc64_terran_late_scenario_for_mission(mission);
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(mission);
    if (!state || !program || !setup || width < 1 || height < 1 ||
        !sc64_terran_late_mission_state_valid(state, program, setup,
                                               (unsigned)width,
                                               (unsigned)height, max_units))
        return 0;
    for (unsigned p = 0; p < 8; ++p)
        if (state->resources[p].current_minerals > 65535u ||
            state->resources[p].current_gas > 65535u) return 0;
    for (unsigned i = offsetof(Sc64TerranLateMissionState, reserved) +
                         sizeof(state->reserved);
         i < sizeof(*state); ++i)
        if (((const u8 *)state)[i]) return 0;

    /* A mutable location may move, but its imported rectangle dimensions are
     * immutable. This rejects corrupt saves that expand a trigger region. */
    if (state->mutable_location_id != SC64_TERRAN_LATE_NO_LOCATION) {
        if (mission != SC64_TERRAN_LATE_GLOBAL_T7 ||
            state->mutable_location_id != 4) return 0;
        const Sc64T4Location *original =
            &program->locations[state->mutable_location_id - 1];
        if (original->right <= original->left ||
            original->bottom <= original->top ||
            state->mutable_location[2] - state->mutable_location[0] !=
                original->right - original->left ||
            state->mutable_location[3] - state->mutable_location[1] !=
                original->bottom - original->top) return 0;
    }

    /* Source trigger contexts retain their eight-slot identity and clocks. */
    for (unsigned p = 0; p < 8; ++p)
        if (state->triggers.player[p].elapsed_ticks !=
            state->triggers.player[0].elapsed_ticks) return 0;
    return 1;
}

static int race_terran_late_save_players_valid(
    const RaceSave *candidate, int mission, const Sc64T4SourceSetup *setup);

static int
race_terran_late_save_campaign_valid(const RaceSave *candidate,
                                     const Sc64TerranLateMissionState *state,
                                     int width, int height)
{
    int mission = candidate->state[2];
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(mission);
    int human = setup ? sc64_terran_late_source_slot_for_controller(
        setup, SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER) : -1;
    if (!setup || human < 0 || human >= 8 ||
        candidate->state[0] != 0 || candidate->state[1] != R_TER ||
        !race_terran_late_save_players_valid(candidate, mission, setup) ||
        candidate->state[7] < 0 ||
        (u32)candidate->state[7] != candidate->campaign.elapsed_frames ||
        candidate->campaign.mission != mission ||
        candidate->campaign.phase > SC64_RACE_PHASE_FAILED ||
        candidate->state[18] < 0 || candidate->state[18] > 1 ||
        candidate->state[19] < 0 || candidate->state[19] > 1 ||
        candidate->campaign.message != state->message ||
        candidate->campaign.message_ticks != state->message_ticks ||
        !race_terran_late_save_state_valid(state, mission, width, height,
                                            MAX_UNITS)) return 0;
    for (unsigned p = 0; p < 8; ++p) {
        const Sc64T4TriggerState *trigger = &state->triggers.player[p];
        if (trigger->elapsed_ticks != candidate->campaign.elapsed_frames)
            return 0;
        if ((int)p != human && trigger->outcome != 0) return 0;
    }
    const Sc64T4TriggerState *human_state = &state->triggers.player[human];
    if (human_state->outcome == 0) {
        if (candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_ACTIVE ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY))
            return 0;
    } else if (human_state->outcome == SC64_TERRAN_LATE_WON) {
        if (!candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_COMPLETE ||
            !(candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
            !(candidate->state[3] & (1 << mission))) return 0;
    } else if (human_state->outcome == SC64_TERRAN_LATE_LOST) {
        if (candidate->state[18] || !candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_FAILED ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY))
            return 0;
    } else return 0;
    return 1;
}

/* Reconstruct init_state's faction table from this candidate's mission.
 * Late missions remap source controller/force slots, so owner_modes alone is
 * not authoritative here. Keep validation candidate-local: never consult or
 * temporarily mutate the currently loaded mission globals. Unassigned
 * runtime owners retain game_init's canonical Terran value (zero). */
static int
race_terran_late_save_players_valid(const RaceSave *candidate, int mission,
                                    const Sc64T4SourceSetup *setup)
{
    const Sc64RaceMap *map;
    int expected[5] = { R_TER, R_TER, R_TER, R_TER, R_TER };
    if (!candidate || !setup || mission < 0 ||
        mission >= SC64_RACE_MAP_COUNT) return 0;
    map = &sc64_race_maps[mission];
    for (unsigned source = 0; source < 8; ++source) {
        int owner = sc64_terran_late_runtime_owner_for_setup(setup, source);
        if (owner >= 0 && owner < 5) {
            if (map->owner_races[source] >= 3) return 0;
            expected[owner] = map->owner_races[source];
        }
    }
    for (unsigned owner = 0; owner < 5; ++owner)
        if (candidate->players[owner] != expected[owner]) return 0;
    return 1;
}

static int
race_terran_late_save_emitter_valid(const RaceSave *candidate, int mission,
                                    int width, int height)
{
    const Sc64TerranLateMissionState *state = &candidate->terran_late;
    if (mission != SC64_TERRAN_LATE_GLOBAL_T7)
        return state->emitter_unit == SC64_TERRAN_LATE_NO_SLOT &&
               state->emitter_carrier == SC64_TERRAN_LATE_NO_SLOT;
    if (state->emitter_unit == SC64_TERRAN_LATE_NO_SLOT)
        return state->emitter_carrier == SC64_TERRAN_LATE_NO_SLOT;

    Unit item;
    unsigned human = (unsigned)sc64_terran_late_source_slot_for_controller(
        sc64_terran_late_setup_for_mission(mission),
        SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
    if (!race_save_decode_unit(candidate, state->emitter_unit,
                               (unsigned)width * 16u,
                               (unsigned)height * 16u, &item) ||
        !item.alive || item.raw_type != 217 || item.owner != SIDE ||
        item.raw_owner != (int)human || item.selected ||
        item.order != ORDER_IDLE || item.target != -1 ||
        !candidate->invincible_units[state->emitter_unit]) return 0;
    if (state->emitter_carrier == SC64_TERRAN_LATE_NO_SLOT) return 1;
    Unit carrier;
    if (state->emitter_carrier == state->emitter_unit ||
        !race_save_decode_unit(candidate, state->emitter_carrier,
                               (unsigned)width * 16u,
                               (unsigned)height * 16u, &carrier) ||
        !carrier.alive || carrier.owner != SIDE || carrier.type != U_SCV ||
        carrier.x != item.x || carrier.y != item.y) return 0;
    return 1;
}

static int
race_terran_late_save_flight_goals_valid(const RaceSave *candidate,
                                          int width, int height)
{
    if (!candidate || width < 1 || height < 1) return 0;
    for (unsigned i = 0; i < 96; ++i) {
        u32 goal = candidate->terran_late.flight_goals[i];
        if (!goal) continue;
        if (i >= MAX_BUILDINGS) return 0;
        Building building;
        if (!race_save_decode_building(candidate, i, &building) ||
            !building.alive || building.owner != SIDE ||
            !(building.flags & 4) || !race_late_can_lift(building.type))
            return 0;
    }
    return 1;
}

static int
race_terran_late_save_pack(Sc64TerranLateMissionState *dst,
                           const Sc64TerranLateMissionState *src,
                           int mission, int width, int height)
{
    if (!dst || !race_terran_late_save_state_valid(src, mission, width, height,
                                                    MAX_UNITS)) return 0;
    *dst = *src;
    return 1;
}

static int
race_terran_late_save_expand(Sc64TerranLateMissionState *dst,
                             const Sc64TerranLateMissionState *src,
                             int mission, int width, int height)
{
    if (!dst || !race_terran_late_save_state_valid(src, mission, width, height,
                                                    MAX_UNITS)) return 0;
    *dst = *src;
    return 1;
}

static int
race_terran_late_save_candidate_valid(const RaceSave *candidate,
                                      int width, int height)
{
    int mission;
    if (!candidate ||
        candidate->state[2] < SC64_TERRAN_LATE_GLOBAL_T7 ||
        candidate->state[2] > SC64_TERRAN_LATE_GLOBAL_T10) return 0;
    mission = candidate->state[2];
    if (sc64_race_maps[mission].width != width ||
        sc64_race_maps[mission].height != height ||
        !race_terran_late_save_campaign_valid(candidate,
                                               &candidate->terran_late,
                                               width, height) ||
        !race_terran_late_save_emitter_valid(candidate, mission,
                                              width, height) ||
        !race_terran_late_save_flight_goals_valid(candidate, width, height) ||
        !race_terran_late_save_queues_valid(candidate, mission))
        return 0;
    return 1;
}

#endif /* SC64_TERRAN_LATE_RUNTIME && SC64_T4_RUNTIME */
#endif
