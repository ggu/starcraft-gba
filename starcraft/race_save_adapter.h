#ifndef SC64_RACE_SAVE_ADAPTER_H
#define SC64_RACE_SAVE_ADAPTER_H
#ifdef SC64_ZP_CAMPAIGN_STORAGE
#define race_data sc64_zp_route_save_scratch.save
#elif defined(SC64_T3_ENABLED)
#ifdef SC64_T4_SCENARIO_RUNTIME
#define race_data sc64_combined_route_save_scratch.save
#else
static RaceSave race_data;
#endif
#else
static RaceSave race_data;
#endif
/* Decode a candidate unit without touching the live world. The compact
 * late-Terran profile stores named fields; historical profiles retain their
 * exact s16 word rows and conversion behavior. */
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
static int
race_save_decode_unit(const RaceSave *candidate, unsigned slot,
                      unsigned world_width, unsigned world_height, Unit *out)
{
    if (!candidate || !out || slot >= MAX_UNITS) return 0;
#ifdef SC64_ZP_FULL_STORAGE
    return sc64_zp_unit_save_decode(out, &candidate->units[slot],
                                    world_width, world_height);
#else
    return sc64_unit_save_decode(out, &candidate->units[slot],
                                 world_width, world_height);
#endif
}
#endif
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
static int
race_save_decode_building(const RaceSave *candidate, unsigned slot, Building *out)
{
    if (!candidate || !out || slot >= MAX_BUILDINGS) return 0;
#ifdef SC64_ZP_FULL_STORAGE
    Building decoded;
    if (!sc64_building_save_decode(&decoded, &candidate->buildings[slot])) return 0;
    if (decoded.x < 0 || decoded.y < 0 || decoded.x >= 2048 || decoded.y >= 2048)
        return 0;
    *out = decoded;
    return 1;
#else
    return sc64_building_save_decode(out, &candidate->buildings[slot]);
#endif
}
#endif
#ifdef SC64_ZP_FULL_STORAGE
static int
race_save_decode_mineral(const RaceSave *candidate, unsigned slot,
                         unsigned width, unsigned height, Mineral *out)
{
    return candidate && slot < MAX_MINERALS &&
        sc64_mineral_save_decode(out, &candidate->minerals_map[slot], width, height);
}
static unsigned
race_save_unit_invincible(const RaceSave *candidate, unsigned slot)
{
    return (candidate->invincible_bits[slot >> 3] >> (slot & 7u)) & 1u;
}
static unsigned
race_save_building_invincible(const RaceSave *candidate, unsigned slot)
{
    unsigned bit = MAX_UNITS + slot;
    return (candidate->invincible_bits[bit >> 3] >> (bit & 7u)) & 1u;
}
static int
race_save_invincible_padding_valid(const RaceSave *candidate)
{
    unsigned bits = MAX_UNITS + MAX_BUILDINGS;
    unsigned used = bits & 7u;
    if (used && (candidate->invincible_bits[bits >> 3] & (u8)~((1u << used) - 1u)))
        return 0;
    return 1;
}
#define RACE_SAVE_INV_U(candidate, i) race_save_unit_invincible((candidate), (i))
#define RACE_SAVE_INV_B(candidate, i) race_save_building_invincible((candidate), (i))
#else
#define RACE_SAVE_INV_U(candidate, i) ((candidate)->invincible_units[(i)])
#define RACE_SAVE_INV_B(candidate, i) ((candidate)->invincible_buildings[(i)])
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
#include "sc64_terran_late_repair.h"
#endif
#ifdef SC64_T3_ENABLED
#ifdef SC64_T4_SCENARIO_RUNTIME
static int
race_t4_save_pack(Sc64T4SaveState *dst, const Sc64T4RuntimeState *src)
{
    Sc64T4TriggerState inactive;
    if (!dst || !src ||
        !sc64_t4_trigger_init(&inactive, &sc64_t4_scenario, 5,
                              sc64_t4_source_force(5)) ||
        memcmp(&src->player[5], &inactive, sizeof(inactive)) != 0) return 0;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->player, src->player, 5 * sizeof(dst->player[0]));
    dst->player[5] = src->player[6];
    dst->player[6] = src->player[7];
    dst->switches = src->switches;
    memcpy(dst->location30, src->location30, sizeof(dst->location30));
    dst->message_ticks = src->message_ticks;
    dst->ping_x = src->ping_x;
    dst->ping_y = src->ping_y;
    dst->ping_ticks = src->ping_ticks;
    dst->location30_moved = src->location30_moved;
    dst->message = src->message;
    dst->active_mask = src->active_mask;
    memcpy(dst->doodad_slot, src->doodad_slot, sizeof(dst->doodad_slot));
    memcpy(dst->doodad_enabled, src->doodad_enabled, sizeof(dst->doodad_enabled));
    return 1;
}

static int
race_t4_save_expand(Sc64T4RuntimeState *dst, const Sc64T4SaveState *src)
{
    Sc64T4TriggerState inactive;
    if (!dst || !src ||
        !sc64_t4_trigger_init(&inactive, &sc64_t4_scenario, 5,
                              sc64_t4_source_force(5))) return 0;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->player, src->player, 5 * sizeof(src->player[0]));
    dst->player[5] = inactive;
    dst->player[6] = src->player[5];
    dst->player[7] = src->player[6];
    dst->switches = src->switches;
    memcpy(dst->location30, src->location30, sizeof(dst->location30));
    dst->message_ticks = src->message_ticks;
    dst->ping_x = src->ping_x;
    dst->ping_y = src->ping_y;
    dst->ping_ticks = src->ping_ticks;
    dst->location30_moved = src->location30_moved;
    dst->message = src->message;
    dst->active_mask = src->active_mask;
    memcpy(dst->doodad_slot, src->doodad_slot, sizeof(dst->doodad_slot));
    memcpy(dst->doodad_enabled, src->doodad_enabled, sizeof(dst->doodad_enabled));
    return 1;
}
#endif
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
static int
race_t4_save_candidate_valid(const RaceSave *candidate, int width, int height)
{
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    const Sc64T4SaveState *state;
#else
    const Sc64T4RuntimeState *state;
#endif
    unsigned next_building = 8;
    if (!candidate) return 0;
    state = &candidate->t4;
    if (candidate->state[0] != 0 || candidate->state[2] != SC64_T4_GLOBAL_ID ||
        candidate->state[7] < 0 ||
        (u32)candidate->state[7] != candidate->campaign.elapsed_frames ||
        candidate->campaign.mission != SC64_T4_GLOBAL_ID ||
        state->active_mask != 0xdfu || state->location30_moved > 1 ||
        state->message_ticks > 90 || state->ping_ticks > 90 ||
        state->ping_x >= width * 16 || state->ping_y >= height * 16) return 0;
    if (state->message != candidate->campaign.message ||
        state->message_ticks != candidate->campaign.message_ticks ||
        (state->ping_ticks == 0 && (state->ping_x || state->ping_y)) ||
        candidate->campaign.objective_stage !=
            ((state->switches.bits[0] & 0x10u) ? 1u : 0u)) return 0;
    if (candidate->campaign.phase == SC64_RACE_PHASE_ACTIVE) {
        if (candidate->state[18] || candidate->state[19] ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (candidate->campaign.phase == SC64_RACE_PHASE_COMPLETE) {
        if (!candidate->state[18] || candidate->state[19] ||
            !(candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (candidate->campaign.phase == SC64_RACE_PHASE_FAILED) {
        if (candidate->state[18] || !candidate->state[19] ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else return 0;
    if (state->location30_moved) {
        const Sc64T4Location *original = &sc64_t4_scenario.locations[29];
        unsigned left = state->location30[0], top = state->location30[1];
        unsigned right = state->location30[2], bottom = state->location30[3];
        if (original->right <= original->left || original->bottom <= original->top ||
            left >= right || top >= bottom || right > (unsigned)width * 32u ||
            bottom > (unsigned)height * 32u ||
            right - left != original->right - original->left ||
            bottom - top != original->bottom - original->top) return 0;
    } else if (state->location30[0] || state->location30[1] ||
               state->location30[2] || state->location30[3]) return 0;
    for (unsigned player = 0; player < 8; ++player) {
        const Sc64T4TriggerState *p;
        unsigned force = sc64_t4_source_force((int)player);
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
        if (player == 5) {
            Sc64T4TriggerState initial;
            if (!sc64_t4_trigger_init(&initial, &sc64_t4_scenario, player, force) ||
                !sc64_t4_trigger_state_valid(&initial, &sc64_t4_scenario)) return 0;
            continue;
        }
        p = &state->player[player < 5 ? player : player - 1];
#else
        p = &state->player[player];
#endif
        if (!sc64_t4_trigger_state_valid(p, &sc64_t4_scenario) ||
            p->owner != player || p->force != force) return 0;
#if !defined(SC64_T3_ENABLED) || !defined(SC64_UNIFIED)
        if (player == 5) {
            Sc64T4TriggerState initial;
            if (!sc64_t4_trigger_init(&initial, &sc64_t4_scenario, player, force) ||
                memcmp(p, &initial, sizeof(initial)) != 0) return 0;
            continue;
        }
#endif
        if (p->elapsed_ticks != candidate->campaign.elapsed_frames) {
            return 0;
        }
    }
    for (unsigned d = 0; d < sc64_t4_scenario.doodad_count; ++d) {
        const Sc64T4Doodad *row = &sc64_t4_scenario.doodads[d];
        unsigned expected_slot = 255;
        int type;
        if (state->doodad_enabled[d] > 1) return 0;
        if (!(row->flags & SC64_T4_THG2_SPRITE_FLAG)) expected_slot = next_building++;
        if (state->doodad_slot[d] != expected_slot) return 0;
        if (expected_slot == 255) {
            if (state->doodad_enabled[d]) return 0;
            continue;
        }
        if (expected_slot >= MAX_BUILDINGS) return 0;
        type = sc64_race_building_from_raw(row->type);
#ifdef SC64_T6_STORAGE
        Building saved;
        if (!race_save_decode_building(candidate, expected_slot, &saved)) return 0;
        if (type < 0 ||
            saved.type != type || saved.raw_type != row->type ||
            saved.raw_owner != row->owner || saved.x != row->x / 2 ||
            saved.y != row->y / 2 ||
            RACE_SAVE_INV_B(candidate, expected_slot) !=
                (unsigned char)sc64_race_t4_building_intrinsically_invincible(type)) return 0;
#else
        if (type < 0 ||
            candidate->buildings[expected_slot][offsetof(Building,type)/sizeof(int)] != type ||
            candidate->buildings[expected_slot][offsetof(Building,raw_type)/sizeof(int)] != row->type ||
            candidate->buildings[expected_slot][offsetof(Building,raw_owner)/sizeof(int)] != row->owner ||
            candidate->buildings[expected_slot][offsetof(Building,x)/sizeof(int)] != row->x / 2 ||
            candidate->buildings[expected_slot][offsetof(Building,y)/sizeof(int)] != row->y / 2 ||
            RACE_SAVE_INV_B(candidate, expected_slot) !=
                (unsigned char)sc64_race_t4_building_intrinsically_invincible(type)) return 0;
#endif
        {
            int owner = sc64_race_t4_is_door(type) ? SC64_T4_OWNER_NEUTRAL :
                        sc64_t4_runtime_owner(row->owner);
#ifdef SC64_T6_STORAGE
            if (owner < 0 || saved.owner != owner) return 0;
#else
            if (owner < 0 || candidate->buildings[expected_slot]
                    [offsetof(Building,owner)/sizeof(int)] != owner) return 0;
#endif
        }
#ifdef SC64_T6_STORAGE
        if (sc64_race_t4_is_door(type) &&
            saved.queue_progress != 0) return 0;
        if (sc64_race_t4_is_trap(type) &&
            (saved.queue_progress < 0 || saved.queue_progress > 28)) return 0;
#else
        if (sc64_race_t4_is_door(type) &&
            candidate->buildings[expected_slot]
                [offsetof(Building,queue_progress)/sizeof(int)] != 0) return 0;
        if (sc64_race_t4_is_trap(type) &&
            (candidate->buildings[expected_slot]
                [offsetof(Building,queue_progress)/sizeof(int)] < 0 ||
             candidate->buildings[expected_slot]
                [offsetof(Building,queue_progress)/sizeof(int)] > 28)) return 0;
#endif
    }
    if (next_building != 34) return 0;
    return 1;
}
#endif
#ifdef SC64_T5_RUNTIME
#include "sc64_t5_save_adapter.h"
#include "sc64_transport_save_adapter.h"
#endif
#ifdef SC64_T6_RUNTIME
#include "sc64_t6_save_adapter.h"
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
#include "sc64_terran_late_save_adapter.h"
#endif
static int sc64_race_storage_geometry_valid(int width, int height) {
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return width >= 1 && width <= 128 && height >= 1 && height <= 128;
#elif defined(SC64_LARGE_MAPS)
    return width >= 1 && width <= 128 && height >= 1 && height <= 96;
#else
    return width >= 1 && width <= 96 && height >= 1 && height <= 96;
#endif
}
static int race_save_game(void) {
#ifdef SC64_ZP_FULL_STORAGE
    if (geyser_count < 0 || geyser_count > MAX_RACE_GEYSERS ||
        race_cols < 1 || race_cols > 128 || race_rows < 1 || race_rows > 128)
        return 0;
    for (unsigned i = 0; i < MAX_RACE_GEYSERS; ++i)
        if (geysers[i].x < 0 || geysers[i].x >= race_cols * 16 ||
            geysers[i].y < 0 || geysers[i].y >= race_rows * 16) return 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i)
        if (invincible_units[i] > 1) return 0;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i)
        if (invincible_buildings[i] > 1) return 0;
#endif
#ifdef SC64_T4_RUNTIME
    /* T4 props with intrinsic invincibility have 100000 source HP. Legacy
     * s16 rows canonicalize that value; the T6 packed codec preserves it. */
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(sc64_race_t4_building_intrinsically_invincible(b[i].type) &&
           (b[i].hp!=100000 || b[i].maxhp!=100000 ||
            b[i].raw_type!=sc64_race_raw_building(b[i].type) || b[i].under_construction ||
            b[i].queue_count || b[i].shield || !invincible_buildings[i]))return 0;
#endif
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    memset(&race_data, 0, sizeof(race_data));
#elif defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    /* The save shares storage with the transient route queue. Clear all
     * padding and optional fields before serializing this record. */
    memset(&race_data, 0, sizeof(race_data));
#endif
    race_data.version=SAVE_VERSION;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ZP_FULL_STORAGE
    for(int i=0;i<MAX_UNITS;i++)
        if(!sc64_zp_unit_save_encode(&race_data.units[i], &u[i],
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16))) return 0;
#else
    for(int i=0;i<MAX_UNITS;i++)
        if(!sc64_unit_save_encode(&race_data.units[i], &u[i],
                                  (unsigned)(race_cols * 16),
                                  (unsigned)(race_rows * 16))) return 0;
#endif
#else
    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {
        int n;memcpy(&n,(u8*)&u[i]+j*sizeof(int),sizeof(n));race_data.units[i][j]=(s16)n;
    }
#endif
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(!sc64_building_save_encode(&race_data.buildings[i], &b[i],
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16))) return 0;
#else
    for(int i=0;i<MAX_BUILDINGS;i++)for(unsigned j=0;j<RACE_BUILD_WORDS;j++) {
        int n;memcpy(&n,(u8*)&b[i]+j*sizeof(int),sizeof(n));
#ifdef SC64_T4_RUNTIME
        if(sc64_race_t4_building_intrinsically_invincible(b[i].type) &&
           (j==offsetof(Building,hp)/sizeof(int) ||
            j==offsetof(Building,maxhp)/sizeof(int)))n=0;
#endif
        race_data.buildings[i][j]=(s16)n;
    }
#endif
    memset(race_data.fog,0,sizeof(race_data.fog));
    for(int i=0;i<MAP_CELLS;i++)if(SC64_GRID_GET(explored,i))race_data.fog[i>>3]|=1u<<(i&7);
#ifdef SC64_ZP_FULL_STORAGE
    for (int i = 0; i < MAX_MINERALS; ++i)
        if (!sc64_mineral_save_encode(&race_data.minerals_map[i], &m[i],
                                      (unsigned)(race_cols * 16),
                                      (unsigned)(race_rows * 16))) return 0;
#else
    memcpy(race_data.minerals_map,m,sizeof(m));
#endif
    int state[]={race_mode,race_human,race_selected_mission,race_completed,race_bots,minerals,gas,
        frame,cx,cy,train_type,weapons_level,armor_level,weapons_research,armor_research,
        (int)seed,geyser_count,mission_objective_stage,won,lost
#ifdef SC64_IMPORTED_SKIRMISH
        ,race_skirmish_map,race_bot_races[0],race_bot_races[1],race_bot_races[2]
#endif
        };
    memcpy(race_data.state,state,sizeof(state));
    memcpy(race_data.players,race_player,sizeof(race_player));
    memcpy(race_data.ore,race_minerals,sizeof(race_minerals));
    memcpy(race_data.vespene,race_gas,sizeof(race_gas));
    memcpy(race_data.geysers,geysers,sizeof(race_data.geysers));
    race_data.campaign=race_campaign;
#ifdef SC64_UNIFIED
    race_data.backwater=backwater;
#ifdef SC64_ZP_FULL_STORAGE
    memset(race_data.invincible_bits, 0, sizeof(race_data.invincible_bits));
    for (unsigned i = 0; i < MAX_UNITS; ++i)
        if (invincible_units[i]) race_data.invincible_bits[i >> 3] |= (u8)(1u << (i & 7u));
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        unsigned bit = MAX_UNITS + i;
        if (invincible_buildings[i])
            race_data.invincible_bits[bit >> 3] |= (u8)(1u << (bit & 7u));
    }
#else
    memcpy(race_data.invincible_units,invincible_units,sizeof(invincible_units));
    memcpy(race_data.invincible_buildings,invincible_buildings,sizeof(invincible_buildings));
#endif
#endif
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    if(!race_mode&&race_selected_mission==6) {
        race_data.t3=race_t3;
        race_data.t3_buildings_killed=race_t3_buildings_killed;
    } else {
        memset(&race_data.t3,0,sizeof(race_data.t3));
        race_data.t3_buildings_killed=0;
    }
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    memset(&race_data.z3,0,sizeof(race_data.z3));race_data.z3_action_ticks=0;
    if(!race_mode&&race_selected_mission==14) {
        race_data.z3=race_z3;
        race_data.z3_action_ticks=race_z3_action_ticks;
    }
#endif
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    memset(&race_data.p3,0,sizeof(race_data.p3));race_data.p3_action_ticks=0;
    if(!race_mode&&race_selected_mission==22) {
        race_data.p3=race_p3;
        race_data.p3_action_ticks=race_p3_action_ticks;
    }
#endif
#ifdef SC64_Z4_RUNTIME
    memset(&race_data.z4, 0, sizeof(race_data.z4));
    if (race_z4_active()) {
        if (!race_z4_runtime_state_valid(&race_z4)) return 0;
        race_data.z4 = race_z4;
    }
#endif
#ifdef SC64_P4_RUNTIME
    memset(&race_data.p4, 0, sizeof(race_data.p4));
    if (race_p4_active()) {
        if (!race_p4_runtime_state_valid(&race_p4)) return 0;
        race_data.p4 = race_p4;
    }
#endif
#ifdef SC64_Z5_RUNTIME
    memset(&race_data.z5, 0, sizeof(race_data.z5));
    if (race_z5_active()) {
        if (!sc64_z5_persistent_state_valid(&race_z5) ||
            !race_z5_restore_doodads()) return 0;
        race_data.z5 = race_z5;
    }
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) {
        if (!race_terran_late_save_pack(&race_data.terran_late, &race_late,
                                        race_selected_mission,
                                        race_cols, race_rows) ||
            !race_terran_late_save_candidate_valid(&race_data,
                                                    race_cols, race_rows))
            return 0;
    } else
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active()) {
        if (!race_t6_save_pack(&race_data.t6, &race_t6) ||
            !race_t6_save_candidate_valid(&race_data, race_cols, race_rows))
            return 0;
    } else
#endif
#ifdef SC64_T5_RUNTIME
#ifndef SC64_T6_RUNTIME
    if (!race_t5_candidate_queues_available(&race_data)) return 0;
#endif
    if (race_t5_active()) {
#ifdef SC64_T6_RUNTIME
        if (!race_t5_candidate_queues_available(&race_data)) return 0;
#endif
        if (!race_t5_save_pack(&race_data.t5, &race_t5) ||
            !race_t5_save_candidate_valid(&race_data, race_cols, race_rows)) return 0;
    } else
#endif
    if (race_t4_active()) {
#ifdef SC64_T3_ENABLED
        if (!race_t4_save_pack(&race_data.t4, &race_t4)) return 0;
#else
        race_data.t4 = race_t4;
#endif
    }
    else {
#ifdef SC64_T6_RUNTIME
#ifdef SC64_TERRAN_LATE_RUNTIME
        memset(&race_data.t4, 0, sizeof(race_data.terran_late));
#else
        memset(&race_data.t4, 0, sizeof(race_data.t6));
#endif
#else
        memset(&race_data.t4, 0, sizeof(race_data.t4));
#endif
    }
    if (race_t4_active() &&
        !race_t4_save_candidate_valid(&race_data, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (!race_transport_save_candidate_valid(&race_data, race_cols, race_rows)) return 0;
#endif
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);toast(7);return 1;
}
static int race_load_game(void) {
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    Sc64T4RuntimeState expanded_t4;
    int expanded_t4_valid = 0;
#endif
#ifdef SC64_T5_RUNTIME
    Sc64T5RuntimeState expanded_t5;
    int expanded_t5_valid = 0;
#endif
#ifdef SC64_T6_RUNTIME
    Sc64T6MissionState expanded_t6;
    int expanded_t6_valid = 0;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    Sc64TerranLateMissionState expanded_terran_late;
    int expanded_terran_late_valid = 0;
#endif
    if(!save_read(&race_data,sizeof(race_data),SAVE_VERSION)||race_data.version!=SAVE_VERSION)return 0;
    const int *v=race_data.state;
    if(v[0]<0||v[0]>1||v[1]<0||v[1]>2||v[2]<0||v[2]>=SC64_RACE_MAP_COUNT||v[4]<1||v[4]>3||
       v[3]<0||v[3]>=(1<<SC64_RACE_MAP_COUNT)||v[5]<0||v[6]<0||v[7]<0||v[10]<0||v[10]>=SC_RU_COUNT||v[16]<0||v[16]>MAX_RACE_GEYSERS)return 0;
#ifdef SC64_CAMPAIGN_RACE
    if(!v[0]&&!sc64_catalog_mission_allowed(v[2],SC64_CAMPAIGN_RACE)
#ifdef SC64_T4_RUNTIME
       &&v[2]!=SC64_T4_GLOBAL_ID
#endif
#ifdef SC64_T5_RUNTIME
       &&v[2]!=SC64_T5_GLOBAL_ID
#endif
#ifdef SC64_T6_RUNTIME
       &&v[2]!=SC64_T6_GLOBAL_ID
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
       &&(v[2]<SC64_TERRAN_LATE_GLOBAL_T7||
          v[2]>SC64_TERRAN_LATE_GLOBAL_T10)
#endif
       )return 0;
#endif
#ifdef SC64_UNIFIED
    if((!sc64_catalog_mission_allowed(v[2],
#ifdef SC64_CAMPAIGN_RACE
                                   SC64_CAMPAIGN_RACE
#else
                                   -1
#endif
                                   )
#ifdef SC64_T4_RUNTIME
        &&v[2]!=SC64_T4_GLOBAL_ID
#endif
#ifdef SC64_T5_RUNTIME
        &&v[2]!=SC64_T5_GLOBAL_ID
#endif
#ifdef SC64_T6_RUNTIME
        &&v[2]!=SC64_T6_GLOBAL_ID
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
        &&(v[2]<SC64_TERRAN_LATE_GLOBAL_T7||
           v[2]>SC64_TERRAN_LATE_GLOBAL_T10)
#endif
        )||!sc64_race_maps[v[2]].width)return 0;
    if((unsigned)v[3]&~sc64_catalog_playable_mask(
#ifdef SC64_CAMPAIGN_RACE
                                               SC64_CAMPAIGN_RACE
#else
                                               -1
#endif
                                               )
#ifdef SC64_T5_RUNTIME
       & ~(1u << SC64_T5_GLOBAL_ID)
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
       & ~((1u << SC64_TERRAN_LATE_GLOBAL_T7) |
           (1u << SC64_TERRAN_LATE_GLOBAL_T8) |
           (1u << SC64_TERRAN_LATE_GLOBAL_T9) |
           (1u << SC64_TERRAN_LATE_GLOBAL_T10))
#endif
       )return 0;
#endif
    int width=v[0]?64:sc64_race_maps[v[2]].width;
    int height=v[0]?64:sc64_race_maps[v[2]].height;
#ifdef SC64_IMPORTED_SKIRMISH
    if(v[20]<0||v[20]>SC64_SKIRMISH_IMPORTED_COUNT)return 0;
    for(int i=21;i<24;i++)if(v[i]<0||v[i]>2)return 0;
    if(v[0]&&v[20]>0){width=sc64_skirmish_maps[v[20]-1].width;height=sc64_skirmish_maps[v[20]-1].height;}
#endif
    if(!sc64_race_storage_geometry_valid(width,height))return 0;
    if(v[8]<0||v[8]>=width*16||v[9]<0||v[9]>=height*16||v[11]<0||v[11]>2||
       v[12]<0||v[12]>2||v[18]<0||v[18]>1||v[19]<0||v[19]>1)return 0;
    for(int i=0;i<5;i++)if(race_data.players[i]<-1||race_data.players[i]>2||
        race_data.ore[i]<0||race_data.ore[i]>65535||race_data.vespene[i]<0||race_data.vespene[i]>65535)return 0;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#define RACE_U(i,f) candidate_unit.f
#else
#define RACE_U(i,f) race_data.units[i][offsetof(Unit,f)/sizeof(int)]
#endif
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#define RACE_B(i,f) candidate_building.f
#else
#define RACE_B(i,f) race_data.buildings[i][offsetof(Building,f)/sizeof(int)]
#endif
    for(int i=0;i<MAX_UNITS;i++) {
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
        Unit candidate_unit;
        if(!race_save_decode_unit(&race_data, (unsigned)i,
                                  (unsigned)(width * 16),
                                  (unsigned)(height * 16),
                                  &candidate_unit)) return 0;
#endif
        if(RACE_U(i,alive)<0||RACE_U(i,alive)>1)return 0;
        if(!RACE_U(i,alive))continue;
        if(RACE_U(i,type)<0||RACE_U(i,type)>=SC_RU_COUNT||RACE_U(i,owner)<0||RACE_U(i,owner)>4||
            RACE_U(i,hp)<=0||RACE_U(i,maxhp)<=0||RACE_U(i,hp)>RACE_U(i,maxhp)||
            RACE_U(i,x)<0||RACE_U(i,x)>=width*16||RACE_U(i,y)<0||RACE_U(i,y)>=height*16||
            RACE_U(i,order)<ORDER_IDLE||RACE_U(i,order)>
#ifdef SC64_T6_RUNTIME
            ORDER_REPAIR||
#elif defined(SC64_T5_RUNTIME)
            ORDER_UNLOAD||
#else
            ORDER_GAS||
#endif
            RACE_U(i,shield)<0||
            RACE_U(i,shield_remainder)<0||RACE_U(i,shield_remainder)>=1280)return 0;
        int target=RACE_U(i,target),order=RACE_U(i,order);
        if((order==ORDER_MINE&&(target<0||target>=MAX_MINERALS))||
           ((order==ORDER_GAS||order==ORDER_BUILD)&&(target<0||target>=MAX_BUILDINGS)))return 0;
#ifdef SC64_T6_RUNTIME
        if (order == ORDER_REPAIR) {
#ifdef SC64_TERRAN_LATE_RUNTIME
            if (race_data.state[0] == 0 &&
                race_data.state[2] >= SC64_TERRAN_LATE_GLOBAL_T7 &&
                race_data.state[2] <= SC64_TERRAN_LATE_GLOBAL_T10) {
                if (RACE_U(i, owner) != SIDE || RACE_U(i, type) != U_SCV ||
                    RACE_U(i, raw_type) != 7 || RACE_U(i, carry) < 0 ||
                    RACE_U(i, carry) >= SC64_T6_REPAIR_FP_ONE ||
                    RACE_U(i, work_timer) < 0 || RACE_U(i, work_timer) > 65535 ||
                    target < 0 || target >= MAX_UNITS + MAX_BUILDINGS)
                    return 0;
                if (target < MAX_UNITS) {
                    Unit repair_target;
                    if (target == i ||
                        !race_save_decode_unit(&race_data, (unsigned)target,
                                               (unsigned)(width * 16),
                                               (unsigned)(height * 16), &repair_target) ||
                        !race_late_repair_unit_type_mechanical(repair_target.type) ||
                        !race_late_repair_unit_fields_valid(repair_target.raw_type,
                            repair_target.owner, repair_target.alive, repair_target.hp,
                            repair_target.maxhp, SC64_TRANSPORT_LOADED(&repair_target)))
                        return 0;
                } else {
                    unsigned building_slot = (unsigned)(target - MAX_UNITS);
                    Building repair_target;
                    const Sc64RaceBuildingRule *rule;
                    if (!race_save_decode_building(&race_data, building_slot,
                                                   &repair_target)) return 0;
                    rule = sc64_race_building_rule(repair_target.type);
                    if (!rule || rule->race != R_TER ||
                        sc64_race_raw_building(repair_target.type) != repair_target.raw_type ||
                        !race_late_repair_building_fields_valid(repair_target.raw_type,
                            repair_target.owner, repair_target.alive,
                            !repair_target.under_construction, (repair_target.flags & 4) != 0,
                            repair_target.hp, repair_target.maxhp)) return 0;
                }
                continue;
            }
#endif
            Building target_building;
            const Sc64RaceBuildingRule *rule;
            int building_slot;
            if (race_data.state[0] != 0 ||
                race_data.state[2] != SC64_T6_GLOBAL_ID ||
                RACE_U(i, owner) != SIDE || RACE_U(i, type) != U_SCV ||
                RACE_U(i, carry) < 0 || RACE_U(i, carry) > 255 ||
                target < MAX_UNITS || target >= MAX_UNITS + MAX_BUILDINGS)
                return 0;
            building_slot = target - MAX_UNITS;
            if (!race_save_decode_building(&race_data, (unsigned)building_slot,
                                           &target_building)) return 0;
            rule = sc64_race_building_rule(target_building.type);
            if (!target_building.alive || target_building.owner != SIDE ||
                target_building.under_construction || target_building.hp < 0 ||
                target_building.maxhp <= 0 || target_building.hp >= target_building.maxhp ||
                !rule || rule->race != R_TER ||
                (rule->worker == 255 && target_building.raw_type != 126)) return 0;
        }
#endif
    }
    for(int i=0;i<MAX_BUILDINGS;i++) {
#if defined(SC64_ZP_FULL_STORAGE)
        Building candidate_building;
        if (!race_save_decode_building(&race_data, (unsigned)i,
                                       &candidate_building) ||
            candidate_building.x >= width * 16 ||
            candidate_building.y >= height * 16) return 0;
#elif defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
        Building candidate_building;
        if(!sc64_building_save_decode(&candidate_building,
                                     &race_data.buildings[i])) return 0;
#endif
        if(RACE_B(i,alive)<0||RACE_B(i,alive)>1)return 0;
#ifdef SC64_T4_RUNTIME
        if(RACE_B(i,type)>=0&&RACE_B(i,type)<SC_RB_COUNT&&
           sc64_race_t4_building_intrinsically_invincible(RACE_B(i,type))&&
#ifdef SC64_T6_STORAGE
           (RACE_B(i,hp)!=100000||RACE_B(i,maxhp)!=100000||
#else
           (RACE_B(i,hp)!=0||RACE_B(i,maxhp)!=0||
#endif
            RACE_B(i,raw_type)!=sc64_race_raw_building(RACE_B(i,type))||
            RACE_B(i,under_construction)!=0||RACE_B(i,queue_count)!=0||
            RACE_B(i,shield)!=0||RACE_SAVE_INV_B(&race_data, i)!=1))return 0;
#endif
        if(!RACE_B(i,alive))continue;
#ifdef SC64_T4_RUNTIME
        int health_ok=RACE_B(i,hp)>0&&RACE_B(i,maxhp)>0&&RACE_B(i,hp)<=RACE_B(i,maxhp);
        if(sc64_race_t4_building_intrinsically_invincible(RACE_B(i,type)))
            health_ok=1; /* The profile-specific exact prop value was checked above. */
#endif
        if(RACE_B(i,type)<0||RACE_B(i,type)>=SC_RB_COUNT||RACE_B(i,owner)<0||RACE_B(i,owner)>4||
#ifdef SC64_T4_RUNTIME
            !health_ok||
#else
            RACE_B(i,hp)<=0||RACE_B(i,maxhp)<=0||RACE_B(i,hp)>RACE_B(i,maxhp)||
#endif
            RACE_B(i,x)<0||RACE_B(i,x)>=width*16||RACE_B(i,y)<0||RACE_B(i,y)>=height*16||
            RACE_B(i,queue_count)<0||RACE_B(i,queue_count)>5||RACE_B(i,queue_head)<0||RACE_B(i,queue_head)>4||
            RACE_B(i,queue_progress)<0||RACE_B(i,build_progress)<-1||RACE_B(i,shield)<0||
            RACE_B(i,larva)<0||RACE_B(i,larva)>3||RACE_B(i,shield_remainder)<0||RACE_B(i,shield_remainder)>=1280)return 0;
        for(int j=0;j<RACE_B(i,queue_count);j++) {
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
            int type=candidate_building.queue[(RACE_B(i,queue_head)+j)%5];
#else
            int type=race_data.buildings[i][offsetof(Building,queue)/sizeof(int)+(RACE_B(i,queue_head)+j)%5];
#endif
            if(type<0||type>=SC_RU_COUNT)return 0;
        }
    }
#undef RACE_U
#undef RACE_B
#ifdef SC64_T5_RUNTIME
    if (!race_t5_candidate_queues_available(&race_data)) return 0;
    if (!race_transport_save_candidate_valid(&race_data, width, height)) return 0;
#endif
#ifdef SC64_ZP_FULL_STORAGE
    for (int i = 0; i < MAX_MINERALS; ++i) {
        Mineral decoded;
        if (!race_save_decode_mineral(&race_data, (unsigned)i,
                                      (unsigned)(width * 16),
                                      (unsigned)(height * 16), &decoded)) return 0;
    }
    for (int i = 0; i < MAX_RACE_GEYSERS; ++i)
        if (race_data.geysers[i].x < 0 || race_data.geysers[i].x >= width * 16 ||
            race_data.geysers[i].y < 0 || race_data.geysers[i].y >= height * 16) return 0;
#else
    for(int i=0;i<MAX_MINERALS;i++)if(race_data.minerals_map[i].amount<0||
        race_data.minerals_map[i].amount>65535)return 0;
#endif
#ifdef SC64_UNIFIED
    if(race_data.backwater.outcome>SC64_BACKWATER_LOST||race_data.backwater.objective>1||
       race_data.backwater.spawn_remaining>5||race_data.campaign.phase>SC64_RACE_PHASE_FAILED||
       (!v[0]&&race_data.campaign.mission!=v[2]))return 0;
#ifdef SC64_ZP_FULL_STORAGE
    if (!race_save_invincible_padding_valid(&race_data)) return 0;
#else
    for(int i=0;i<MAX_UNITS;i++)if(race_data.invincible_units[i]>1)return 0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(race_data.invincible_buildings[i]>1)return 0;
#endif
#endif
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    if(!v[0]&&v[2]==6) {
        u8 phase=race_data.campaign.phase;
        u32 t3_elapsed=race_data.t3.elapsed_ticks;
        if(!sc64_t3_state_valid(&race_data.t3))return 0;
        if(race_data.campaign.elapsed_frames!=t3_elapsed||(u32)v[7]!=t3_elapsed||
           race_data.campaign.objective_stage!=race_data.t3.objective)return 0;
        /* The optional objective can only be set after at least one building
           kill. Kills after the 120-second trigger window remain valid even
           when the alternate objective was never revealed. */
        if(race_data.t3.objective&&race_data.t3_buildings_killed==0)return 0;
        if(race_data.t3.outcome==SC64_T3_RUNNING) {
            if(v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else if(race_data.t3.outcome==SC64_T3_WON) {
            if(!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
               !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else if(race_data.t3.outcome==SC64_T3_LOST) {
            if(v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else return 0;
    }
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    if(!v[0]&&v[2]==14) {
        u8 phase=race_data.campaign.phase;
        if(!sc64_z3_state_valid(&race_data.z3)||
           race_data.campaign.elapsed_frames!=(u32)v[7]||
           race_data.campaign.objective_stage!=race_data.z3.switch7)return 0;
        u16 duration=race_z3_action_duration(&race_data.z3);
        if(duration ? (!race_data.z3_action_ticks||race_data.z3_action_ticks>duration) :
                      race_data.z3_action_ticks!=0)return 0;
        if(race_data.z3.outcome==SC64_Z3_RUNNING) {
            if(v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else if(race_data.z3.outcome==SC64_Z3_WON) {
            if(!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
               !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else {
            if(v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        }
    } else {
        if(race_data.z3.fired||race_data.z3.switch7||race_data.z3.outcome||
           race_data.z3.sequence||race_data.z3.transmission||race_data.z3_action_ticks)return 0;
    }
#endif
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if(!v[0]&&v[2]==SC64_P3_GLOBAL_ID&&
       (v[1]!=R_PROTOSS||race_data.players[SIDE]!=R_PROTOSS||
        race_data.players[ENEMY]!=R_ZERG||race_data.players[4]!=R_ZERG))return 0;
    if(!v[0]&&v[2]==22) {
        u8 phase=race_data.campaign.phase;
        if(!sc64_p3_model_state_valid(&race_data.p3)||
           race_data.campaign.elapsed_frames!=(u32)v[7]||
           race_data.campaign.objective_stage!=race_data.p3.sequence)return 0;
        u16 duration=race_p3_action_duration(&race_data.p3);
        if(duration ? (!race_data.p3_action_ticks||race_data.p3_action_ticks>duration) :
                      race_data.p3_action_ticks!=0)return 0;
        if(race_data.p3.outcome==SC64_P3_RUNNING) {
            if(v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else if(race_data.p3.outcome==SC64_P3_WON) {
            if(!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
               !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)||
               !(v[3]&(1<<22)))return 0;
        } else {
            if(v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        }
    } else {
        const u8 *p=(const u8 *)&race_data.p3;
        for(unsigned i=0;i<sizeof(race_data.p3);i++)if(p[i])return 0;
        if(race_data.p3_action_ticks)return 0;
    }
#endif
#ifdef SC64_Z4_RUNTIME
    if (!v[0] && v[2] == 15) {
        const Sc64Z4RuntimeState *z4 = &race_data.z4;
        unsigned outcome = z4->model.outcome;
        unsigned phase = race_data.campaign.phase;
        if (!race_z4_runtime_state_valid(z4) || v[1] != R_ZERG ||
            race_data.players[SIDE] != R_ZERG || race_data.players[ENEMY] != R_TER ||
            race_data.players[4] != R_TER || race_data.campaign.mission != 15 ||
            race_data.campaign.elapsed_frames != (u32)v[7] ||
            race_data.campaign.objective_stage !=
                !!(z4->model.fired & SC64_Z4_EVENT_TRIGGER_HATCH)) return 0;
        if (outcome == SC64_Z4_RUNNING) {
            if (v[18] || v[19] || phase != SC64_RACE_PHASE_ACTIVE ||
                (race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else if (outcome == SC64_Z4_WON) {
            if (!v[18] || v[19] || phase != SC64_RACE_PHASE_COMPLETE ||
                !(race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
                !(v[3] & (1 << 15))) return 0;
        } else if (v[18] || !v[19] || phase != SC64_RACE_PHASE_FAILED ||
                   (race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else {
        const u8 *bytes = (const u8 *)&race_data.z4;
        for (unsigned i = 0; i < sizeof(race_data.z4); ++i) if (bytes[i]) return 0;
    }
#endif
#ifdef SC64_P4_RUNTIME
    if (!v[0] && v[2] == SC64_P4_GLOBAL_ID) {
        const Sc64P4RuntimeState *p4 = &race_data.p4;
        unsigned outcome = p4->model.outcome;
        unsigned phase = race_data.campaign.phase;
        unsigned live = 0, queued = 0;
        for (unsigned i = 0; i < MAX_UNITS; ++i) {
            Unit candidate;
            if (!race_save_decode_unit(&race_data, i, (unsigned)(width * 16),
                                       (unsigned)(height * 16), &candidate)) return 0;
            live += candidate.alive != 0;
#ifdef SC64_P4_STORM_ENABLED
            if ((candidate.raw_type == SC64_P4_STORM_HIGH_TEMPLAR_RAW ||
                 candidate.raw_type == SC64_P4_RAW_TASSADAR) &&
                !race_p4_storm_energy_valid(&candidate)) return 0;
#endif
        }
        for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
            Building candidate;
            if (!race_save_decode_building(&race_data, i, &candidate)) return 0;
            if (candidate.alive) for (int q = 0; q < candidate.queue_count; ++q)
                queued += candidate.queue[(candidate.queue_head + q) % 5] == U_ZERGLING ? 2u : 1u;
        }
        if (!race_p4_runtime_state_valid(p4) || v[1] != R_PROTOSS ||
            race_data.players[SIDE] != R_PROTOSS || race_data.players[ENEMY] != R_ZERG ||
            race_data.players[RESCUABLE] != R_PROTOSS || race_data.players[4] != R_ZERG ||
            race_data.campaign.mission != SC64_P4_GLOBAL_ID ||
            race_data.campaign.elapsed_frames != (u32)v[7] ||
            live + queued + SC64_P4_SOURCE_CREATE_COUNT - p4->created_count > MAX_UNITS)
            return 0;
#ifdef SC64_P4_STORM_ENABLED
        for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
            const Sc64PsionicStorm *effect = &p4->storm.storm[i];
            if (effect->active && (effect->x >= width * 16 || effect->y >= height * 16))
                return 0;
        }
#endif
        if (outcome == SC64_P4_RUNNING) {
            if (v[18] || v[19] || phase != SC64_RACE_PHASE_ACTIVE ||
                (race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else if (outcome == SC64_P4_WON) {
            if (!v[18] || v[19] || phase != SC64_RACE_PHASE_COMPLETE ||
                !(race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
                !(v[3] & (1 << SC64_P4_GLOBAL_ID))) return 0;
        } else if (v[18] || !v[19] || phase != SC64_RACE_PHASE_FAILED ||
                   (race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else {
        const u8 *bytes = (const u8 *)&race_data.p4;
        for (unsigned i = 0; i < sizeof(race_data.p4); ++i) if (bytes[i]) return 0;
    }
#endif
#ifdef SC64_Z5_RUNTIME
    if (!v[0] && v[2] == SC64_Z5_GLOBAL_ID) {
        const Sc64Z5PersistentState *z5 = &race_data.z5;
        const Sc64RaceMap *map = &sc64_race_maps[SC64_Z5_GLOBAL_ID];
        unsigned live = 0, queued = 0, creates_remaining = 0;
        unsigned outcome = z5->runtime.model.outcome;
        unsigned phase = race_data.campaign.phase;
        if (!sc64_z5_persistent_state_valid(z5) || map->width != SC64_Z5_MAP_COLS ||
            map->height != SC64_Z5_MAP_ROWS || race_data.campaign.mission != SC64_Z5_GLOBAL_ID ||
            race_data.campaign.elapsed_frames != (u32)v[7] || v[1] != R_ZERG ||
            race_data.players[SIDE] != R_ZERG ||
            race_data.campaign.objective_stage != 0) return 0;
        for (unsigned i = 0; i < MAX_UNITS; ++i) {
            Unit candidate;
            if (!race_save_decode_unit(&race_data, i, width * 16u, height * 16u,
                                       &candidate)) return 0;
            live += candidate.alive != 0;
        }
        for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
            Building candidate;
            if (!race_save_decode_building(&race_data, i, &candidate)) return 0;
            if (candidate.alive) for (int q = 0; q < candidate.queue_count; ++q)
                queued += candidate.queue[(candidate.queue_head + q) % 5] == U_ZERGLING ? 2u : 1u;
        }
        for (unsigned t = 0; t < SC64_Z5_TRIGGER_COUNT; ++t) {
            const Sc64Z5SourceTrigger *trigger = &sc64_z5_source_triggers[t];
            int started = sc64_z5_mask_test(z5->runtime.model.started_lo,
                                             z5->runtime.model.started_hi, t);
            unsigned cursor = z5->runtime.model.action_cursor[t];
            if (trigger->first_action + trigger->action_count >
                SC64_Z5_SOURCE_ACTION_COUNT) return 0;
            for (unsigned a = 0; a < trigger->action_count; ++a) {
                const Sc64Z5SourceAction *action =
                    &sc64_z5_source_actions[trigger->first_action + a];
                if (action->opcode == 11 &&
                    (!started || (cursor && a >= cursor))) ++creates_remaining;
            }
        }
        if (live + queued + creates_remaining > MAX_UNITS) return 0;
        /* Every retained unit-backed THG2 row must still have exactly one
         * immutable building-pool identity in the candidate save. */
        for (unsigned row = 0; row < SC64_Z5_DOODAD_COUNT; ++row) {
            const Sc64Z5SourceDoodad *src = &sc64_z5_source_doodads[row];
            int expected_owner, type, matches = 0;
            if (!src->unit_backed) continue;
            if (src->owner >= 8 || src->type > 255) return 0;
            type = sc64_race_building_from_raw(src->type);
            if (type < 0) return 0;
            expected_owner = src->owner == map->human_slot ? SIDE :
                             map->owner_modes[src->owner];
            for (unsigned slot = 0; slot < MAX_BUILDINGS; ++slot) {
                Building candidate;
                if (!race_save_decode_building(&race_data, slot, &candidate)) return 0;
                if (candidate.alive && candidate.raw_type == src->type &&
                    candidate.raw_owner == src->owner && candidate.type == type &&
                    candidate.owner == expected_owner && candidate.x == src->x / 2 &&
                    candidate.y == src->y / 2 && candidate.flags == src->flags)
                    ++matches;
            }
            if (matches != 1) return 0;
        }
        if (outcome == SC64_Z5_RUNNING) {
            if (v[18] || v[19] || phase != SC64_RACE_PHASE_ACTIVE ||
                (race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else if (outcome == SC64_Z5_WON) {
            if (!v[18] || v[19] || phase != SC64_RACE_PHASE_COMPLETE ||
                !(race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
                !(v[3] & (1 << SC64_Z5_GLOBAL_ID))) return 0;
        } else if (outcome == SC64_Z5_LOST) {
            if (v[18] || !v[19] || phase != SC64_RACE_PHASE_FAILED ||
                (race_data.campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else return 0;
    } else {
        const u8 *bytes = (const u8 *)&race_data.z5;
        for (unsigned i = 0; i < sizeof(race_data.z5); ++i) if (bytes[i]) return 0;
    }
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (!v[0] && v[2] >= SC64_TERRAN_LATE_GLOBAL_T7 &&
        v[2] <= SC64_TERRAN_LATE_GLOBAL_T10) {
        if (!race_terran_late_save_candidate_valid(&race_data, width, height) ||
            !race_terran_late_save_expand(&expanded_terran_late,
                                          &race_data.terran_late,
                                          v[2], width, height)) return 0;
        expanded_terran_late_valid = 1;
    } else
#endif
#ifdef SC64_T6_RUNTIME
    if (!v[0] && v[2] == SC64_T6_GLOBAL_ID) {
        if (!race_t6_save_candidate_valid(&race_data, width, height) ||
            !race_t6_save_expand(&expanded_t6, &race_data.t6)) return 0;
        expanded_t6_valid = 1;
    } else
#endif
#ifdef SC64_T5_RUNTIME
    if (!v[0] && v[2] == SC64_T5_GLOBAL_ID) {
        if (!race_t5_save_candidate_valid(&race_data, width, height) ||
            !race_t5_save_expand(&expanded_t5, &race_data.t5)) return 0;
        expanded_t5_valid = 1;
    } else
#endif
    if (!v[0] && v[2] == SC64_T4_GLOBAL_ID) {
        if (!race_t4_save_candidate_valid(&race_data, width, height)) return 0;
#ifdef SC64_T3_ENABLED
        if (!race_t4_save_expand(&expanded_t4, &race_data.t4)) return 0;
        expanded_t4_valid = 1;
#endif
    } else {
        const u8 *t4 = (const u8 *)&race_data.t4;
#ifdef SC64_T6_RUNTIME
#ifdef SC64_TERRAN_LATE_RUNTIME
        for (unsigned i = 0; i < sizeof(race_data.terran_late); ++i)
#else
        for (unsigned i = 0; i < sizeof(race_data.t6); ++i)
#endif
            if (t4[i]) return 0;
#else
        for (unsigned i = 0; i < sizeof(race_data.t4); ++i)
            if (t4[i]) return 0;
#endif
    }
#endif
#ifdef SC64_T6_RUNTIME
    /* Mission states share one payload. Require all bytes beyond the selected
     * schema to be zero, including T6 records under the larger generation-18
     * union. */
    {
        unsigned used = 0;
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (!v[0] && v[2] >= SC64_TERRAN_LATE_GLOBAL_T7 &&
            v[2] <= SC64_TERRAN_LATE_GLOBAL_T10)
            used = sizeof(Sc64TerranLateMissionState);
#endif
#ifdef SC64_T6_RUNTIME
        if (!v[0] && v[2] == SC64_T6_GLOBAL_ID)
            used = sizeof(Sc64T6MissionState);
#endif
#ifdef SC64_T5_RUNTIME
        if (!v[0] && v[2] == SC64_T5_GLOBAL_ID) used = sizeof(Sc64T5SaveState);
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
        if (!v[0] && v[2] == SC64_T4_GLOBAL_ID) used = sizeof(Sc64T4SaveState);
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
        const u8 *payload = (const u8 *)&race_data.terran_late;
        for (unsigned i = used; i < sizeof(race_data.terran_late); ++i)
#else
        const u8 *payload = (const u8 *)&race_data.t6;
        for (unsigned i = used; i < sizeof(race_data.t6); ++i)
#endif
            if (payload[i]) return 0;
    }
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    if (!v[0] && v[2] == SC64_T4_GLOBAL_ID && !expanded_t4_valid) return 0;
#endif
    race_mode=v[0];race_human=v[1];race_selected_mission=v[2];race_completed=v[3];race_bots=v[4];
#ifdef SC64_IMPORTED_SKIRMISH
    race_skirmish_map=v[20];for(int i=0;i<3;i++)race_bot_races[i]=v[21+i];
#endif
    minerals=v[5];gas=v[6];frame=v[7];cx=v[8];cy=v[9];train_type=v[10];
    weapons_level=v[11];armor_level=v[12];weapons_research=v[13];armor_research=v[14];seed=(u32)v[15];
    geyser_count=v[16];mission_objective_stage=v[17];won=v[18];lost=v[19];
    race_cols=race_mode?64:sc64_race_maps[race_selected_mission].width;
    race_rows=race_mode?64:sc64_race_maps[race_selected_mission].height;
    if(race_mode)race_skirmish_geometry();
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    for(int i=0;i<MAX_UNITS;i++) {
        Unit decoded;
#ifdef SC64_ZP_FULL_STORAGE
        if(!sc64_zp_unit_save_decode(&decoded, &race_data.units[i],
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16))) return 0;
#else
        if(!sc64_unit_save_decode(&decoded, &race_data.units[i],
                                  (unsigned)(race_cols * 16),
                                  (unsigned)(race_rows * 16))) return 0;
#endif
        memcpy(&u[i], &decoded, sizeof(decoded));
    }
#else
    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {
        int n=j==offsetof(Unit,flags)/sizeof(int)?(u16)race_data.units[i][j]:race_data.units[i][j];
        memcpy((u8*)&u[i]+j*sizeof(int),&n,sizeof(n));
    }
#endif
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    for(int i=0;i<MAX_BUILDINGS;i++) {
        if(!sc64_building_save_decode(&b[i], &race_data.buildings[i])) return 0;
    }
#else
    for(int i=0;i<MAX_BUILDINGS;i++)for(unsigned j=0;j<RACE_BUILD_WORDS;j++) {
        int n=j==offsetof(Building,flags)/sizeof(int)?(u16)race_data.buildings[i][j]:race_data.buildings[i][j];
        memcpy((u8*)&b[i]+j*sizeof(int),&n,sizeof(n));
    }
#endif
#ifdef SC64_T4_RUNTIME
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(sc64_race_t4_building_intrinsically_invincible(b[i].type))
            b[i].hp=b[i].maxhp=100000;
#endif
#ifdef SC64_ZP_FULL_STORAGE
    for (int i = 0; i < MAX_MINERALS; ++i)
        if (!race_save_decode_mineral(&race_data, (unsigned)i,
                                      (unsigned)(race_cols * 16),
                                      (unsigned)(race_rows * 16), &m[i])) return 0;
#else
    memcpy(m,race_data.minerals_map,sizeof(m));
#endif
    SC64_GRID_CLEAR(explored);
    for(int i=0;i<MAP_CELLS;i++)
        if((race_data.fog[i>>3]>>(i&7))&1)SC64_GRID_SET(explored,i,1);
    memcpy(race_player,race_data.players,sizeof(race_player));
    memcpy(race_minerals,race_data.ore,sizeof(race_minerals));
    memcpy(race_gas,race_data.vespene,sizeof(race_gas));
    memcpy(geysers,race_data.geysers,sizeof(race_data.geysers));race_campaign=race_data.campaign;
#ifdef SC64_UNIFIED
    backwater=race_data.backwater;
#ifdef SC64_ZP_FULL_STORAGE
    for (unsigned i = 0; i < MAX_UNITS; ++i)
        invincible_units[i] = (u8)race_save_unit_invincible(&race_data, i);
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i)
        invincible_buildings[i] = (u8)race_save_building_invincible(&race_data, i);
#else
    memcpy(invincible_units,race_data.invincible_units,sizeof(invincible_units));
    memcpy(invincible_buildings,race_data.invincible_buildings,sizeof(invincible_buildings));
#endif
    backwater_message_ticks=0;
#endif
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    if(!race_mode&&race_selected_mission==6) {
        race_t3=race_data.t3;
        race_t3_buildings_killed=race_data.t3_buildings_killed;
    } else {
        memset(&race_t3,0,sizeof(race_t3));
        race_t3_buildings_killed=0;
    }
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    if(!race_mode&&race_selected_mission==14) {
        race_z3=race_data.z3;race_z3_action_ticks=race_data.z3_action_ticks;
    } else {
        memset(&race_z3,0,sizeof(race_z3));race_z3_action_ticks=0;
    }
#endif
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if(!race_mode&&race_selected_mission==22) {
        race_p3=race_data.p3;race_p3_action_ticks=race_data.p3_action_ticks;
    } else {
        memset(&race_p3,0,sizeof(race_p3));race_p3_action_ticks=0;
    }
#endif
#ifdef SC64_Z4_RUNTIME
    if (race_z4_active()) race_z4 = race_data.z4;
    else memset(&race_z4, 0, sizeof(race_z4));
#endif
#ifdef SC64_P4_RUNTIME
    if (race_p4_active()) race_p4 = race_data.p4;
    else memset(&race_p4, 0, sizeof(race_p4));
#endif
#ifdef SC64_Z5_RUNTIME
    if (!race_mode && race_selected_mission == SC64_Z5_GLOBAL_ID) {
        race_z5 = race_data.z5;
        if (!race_z5_rebind_loaded_world(&sc64_race_maps[SC64_Z5_GLOBAL_ID]))
            return 0;
    } else {
        memset(&race_z5, 0, sizeof(race_z5));
        memset(&race_z5_world, 0, sizeof(race_z5_world));
        memset(race_z5_doodads, 0, sizeof(race_z5_doodads));
    }
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (!race_mode && race_selected_mission == SC64_T4_GLOBAL_ID) {
#ifdef SC64_T3_ENABLED
        race_t4 = expanded_t4;
#else
        race_t4 = race_data.t4;
#endif
    } else
        memset(&race_t4, 0, sizeof(race_t4));
#endif
#ifdef SC64_T5_RUNTIME
    if (expanded_t5_valid) race_t5 = expanded_t5;
    else memset(&race_t5, 0, sizeof(race_t5));
#endif
#ifdef SC64_T6_RUNTIME
    if (expanded_t6_valid) race_t6 = expanded_t6;
    else memset(&race_t6, 0, sizeof(race_t6));
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (expanded_terran_late_valid) race_late = expanded_terran_late;
    else memset(&race_late, 0, sizeof(race_late));
#endif
    memset(tracers,0,sizeof(tracers));mode=MODE_NORMAL;menu_open=help=0;selected_building=-1;
    invalidate_path_grid();sc64_visibility_reset();update_exploration();reset_selection_gestures();
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
    screen=won||lost?SCREEN_RESULT:SCREEN_PLAY;toast(7);return 1;
}
#endif
