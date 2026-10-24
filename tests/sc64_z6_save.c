/* Candidate-only Z6 save tests against the actual generated mission map. */
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_z6_runtime_adapter.h"
#include "../starcraft/sc64_z6_save.h"

static void init_source_world(void)
{
    const Sc64RaceMap *map = &sc64_race_maps[SC64_Z6_GLOBAL_ID];
    race_mode = 0;
    race_selected_mission = SC64_Z6_GLOBAL_ID;
    race_cols = race_rows = SC64_Z6_MAP_COLS;
    race_player[SIDE] = R_ZERG;
    race_player[ENEMY] = R_PROTOSS;
    race_minerals[ENEMY] = race_gas[ENEMY] = 0;
    supply_used = supply_cap = 0;
    selected_building = -1;
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(geysers, 0, sizeof(geysers));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    geyser_count = 0;
    unsigned mineral_slot = 0;
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        if (raw == SC64_RAW_START_LOCATION) continue;
        if (raw >= 176u && raw <= 178u) {
            assert(mineral_slot < MAX_MINERALS);
            m[mineral_slot].x = src->x_px;
            m[mineral_slot].y = src->y_px;
            m[mineral_slot].amount = src->resources ? src->resources : 1;
            m[mineral_slot].style = (int)raw;
            ++mineral_slot;
            continue;
        }
        if (raw == 188u) {
            assert(geyser_count < MAX_RACE_GEYSERS);
            geysers[geyser_count++] = (Geyser){src->x_px, src->y_px};
            continue;
        }
        int owner = src->owner == map->human_slot ? SIDE : map->owner_modes[src->owner];
        int type = sc64_race_unit_from_raw((int)raw);
        if (type >= 0) {
            int slot = spawn_unit(owner, type, src->x_px, src->y_px);
            assert(slot >= 0);
            u[slot].raw_type = (s16)raw;
            u[slot].raw_owner = src->owner;
            continue;
        }
        type = race_z6_save_building_type(raw);
        assert(type >= 0);
        int slot = spawn_build(owner, type, src->x_px, src->y_px, 0);
        assert(slot >= 0);
        b[slot].raw_type = (s16)raw;
        b[slot].raw_owner = src->owner;
    }
    memset(&race_z6, 0, sizeof(race_z6));
    sc64_z6_runtime_init(&race_z6.runtime);
    race_z6_map = map;
    assert(race_z6_save_source_map_valid(map));
}

static void pack_candidate(RaceSave *save, unsigned elapsed)
{
    memset(save, 0, sizeof(*save));
    save->version = SAVE_VERSION;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        assert(sc64_zp_unit_save_encode(&save->units[i], &u[i],
            SC64_Z6_MAP_COLS * 16u, SC64_Z6_MAP_ROWS * 16u));
        if (invincible_units[i])
            save->invincible_bits[i >> 3] |= (u8)(1u << (i & 7u));
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        assert(sc64_building_save_encode(&save->buildings[i], &b[i],
            SC64_Z6_MAP_COLS * 16u, SC64_Z6_MAP_ROWS * 16u));
        if (invincible_buildings[i]) {
            unsigned bit = MAX_UNITS + i;
            save->invincible_bits[bit >> 3] |= (u8)(1u << (bit & 7u));
        }
    }
    for (unsigned i = 0; i < MAX_MINERALS; ++i)
        assert(sc64_mineral_save_encode(&save->minerals_map[i], &m[i],
            SC64_Z6_MAP_COLS * 16u, SC64_Z6_MAP_ROWS * 16u));
    save->state[0] = race_mode;
    save->state[1] = R_ZERG;
    save->state[2] = SC64_Z6_GLOBAL_ID;
    save->state[3] = 0;
    save->state[7] = (int)elapsed;
    save->state[16] = geyser_count;
    memcpy(save->geysers, geysers, sizeof(save->geysers));
    save->state[18] = save->state[19] = 0;
    save->players[SIDE] = R_ZERG;
    save->players[ENEMY] = R_PROTOSS;
    save->players[NEUTRAL] = R_TER;
    save->campaign.mission = SC64_Z6_GLOBAL_ID;
    save->campaign.elapsed_frames = elapsed;
    save->campaign.phase = SC64_RACE_PHASE_ACTIVE;
}

static void test_actual_source_candidate_and_movement(void)
{
    RaceSave candidate;
    init_source_world();
    assert(race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) == ENEMY);
    pack_candidate(&candidate, 12);
    assert(race_z6_save_candidate_valid(&candidate, &race_z6,
                                        SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
    int slot = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_owner == 3) { slot = i; break; }
    assert(slot >= 0);
    u[slot].x += 7;
    u[slot].y += 5;
    u[slot].order = ORDER_MOVE;
    u[slot].tx = u[slot].x + 20;
    pack_candidate(&candidate, 13);
    assert(race_z6_save_candidate_valid(&candidate, &race_z6,
                                        SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
    candidate.players[ENEMY] = R_ZERG;
    assert(!race_z6_save_candidate_valid(&candidate, &race_z6,
                                         SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
}

static void test_source_death_accounting(void)
{
    RaceSave candidate;
    init_source_world();
    int slot = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_owner == 3) { slot = i; break; }
    assert(slot >= 0);
    u[slot].alive = 0;
    race_z6.deaths_by_source[3] = 1;
    pack_candidate(&candidate, 100);
    assert(race_z6_save_candidate_valid(&candidate, &race_z6,
                                        SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
    race_z6.deaths_by_source[3] = 0;
    assert(!race_z6_save_candidate_valid(&candidate, &race_z6,
                                         SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
}

static void test_source_create_and_ai_phase_limits(void)
{
    RaceSave candidate;
    init_source_world();
    const Sc64Z6SourceAction *create = 0;
    for (unsigned i = 0; i < SC64_Z6_SOURCE_ACTION_COUNT; ++i)
        if (sc64_z6_source_actions[i].trigger == 4 &&
            sc64_z6_source_actions[i].opcode == 11) {
            create = &sc64_z6_source_actions[i]; break;
        }
    assert(create && create->unit == 79);
    int created=spawn_unit(race_z6_source_owner(create->group1),
                           sc64_race_unit_from_raw(create->unit),
                           1200,900);
    assert(created>=0);
    u[created].raw_type=(s16)create->unit;
    u[created].raw_owner=(s16)create->group1;
    assert(race_z6_hallucination_mark_slot((unsigned)created));
    pack_candidate(&candidate, 200);
    assert(!race_z6_save_candidate_valid(&candidate, &race_z6,
                                        SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
    race_z6.runtime.model.started |= (u16)(1u << 4);
    race_z6.runtime.model.action_cursor[4] = 1;
    race_z6.runtime.action_ticks[4] = 1;
    assert(sc64_z6_runtime_state_valid(&race_z6.runtime));
    assert(race_z6_save_candidate_valid(&candidate, &race_z6,
                                        SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
    race_z6.ai_plan_phase = 1;
    assert(!race_z6_save_candidate_valid(&candidate, &race_z6,
                                         SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
    race_z6.ai_plan_phase = 0;
    race_z6.runtime.model.started |= (u16)(1u << 8);
    race_z6.runtime.model.action_cursor[8] = 2;
    race_z6.runtime.action_ticks[8] = 1;
    race_z6.ai_plan_ticks = SC64_Z6_AI_INTERVAL_TICKS;
    assert(race_z6_save_candidate_valid(&candidate, &race_z6,
                                        SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
    race_z6.runtime.model.action_cursor[8] = 1;
    race_z6.runtime.action_ticks[8] = 1;
    race_z6.ai_plan_ticks = 0;
    assert(race_z6_save_candidate_valid(&candidate, &race_z6,
                                        SC64_Z6_MAP_COLS, SC64_Z6_MAP_ROWS));
}

int main(void)
{
    const Sc64RaceMap *canonical = &sc64_race_maps[SC64_Z6_GLOBAL_ID];
    RaceSave candidate;
    memset(&candidate, 0, sizeof(candidate));
    race_z6_map = 0;
    assert(race_z6_source_owner(SC64_Z6_AI_OWNER_SLOT) == ENEMY);
    assert(race_z6_save_expected_owner(canonical, 1) == ENEMY);
    assert(race_z6_save_expected_owner(canonical, 3) == SIDE);
    /* Keep raw 172 unsupported until Z6 has its own explicit actor mapping.
       This is the expected fail-closed result, not a substitute actor. */
    if (sc64_race_building_from_raw(172) < 0) {
        assert(!race_z6_save_source_map_valid(canonical));
        assert(!race_z6_save_candidate_valid(&candidate, &race_z6,
                                             SC64_Z6_MAP_COLS,
                                             SC64_Z6_MAP_ROWS));
        return 0;
    }
    test_actual_source_candidate_and_movement();
    test_source_death_accounting();
    test_source_create_and_ai_phase_limits();
    return 0;
}
