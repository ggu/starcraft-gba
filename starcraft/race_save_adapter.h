#ifndef SC64_RACE_SAVE_ADAPTER_H
#define SC64_RACE_SAVE_ADAPTER_H
#include "sc64_construction_save.h"
#define SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX 25
#define SC64_BUILDING_DAMAGE_SAVE_MARKER 0x42443031
#ifdef SC64_SHARED_BATTERY
#define SC64_SHIELD_PHASE_SAVE_MARKER_INDEX 26
#define SC64_SHIELD_PHASE_SAVE_MARKER 0x53483132
#endif
/* state[0..23] is the historical gameplay state; the remaining words have
 * never been assigned by its serializer. Distinguish old construction rules
 * without changing the save record size or rejecting completed old worlds. */
#ifdef SC64_CONSTRUCTION_HP
#define SC64_CONSTRUCTION_SAVE_MARKER_INDEX 24
#define SC64_CONSTRUCTION_SAVE_MARKER 0x43485031
#endif
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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_decode(out, &candidate->units[slot], world_width, world_height);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_unit_decode(out, &candidate->units[slot], world_width, world_height);
#else
    return sc64_zp_unit_save_decode(out, &candidate->units[slot],
                                    world_width, world_height);
#endif
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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    if (!sc64_p7_typed_building_decode(&decoded, &candidate->buildings[slot], 2048, 2048)) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
    if (!sc64_derived_building_decode(&decoded, &candidate->buildings[slot])) return 0;
#else
    if (!sc64_building_save_decode(&decoded, &candidate->buildings[slot])) return 0;
#endif
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

/* Legacy unified SRAM rows store each Building word as s16. Preserve the
 * exact source 100000 HP for the static Zerg Beacon without changing the row
 * shape or save generation. This sentinel is valid only for raw 194 mapped
 * to B_ZERG_BEACON; all other health values remain range-checked normally. */
#define SC64_LEGACY_BEACON_HP_SENTINEL (-32768)
static int
sc64_legacy_beacon_hp_sentinel(int type, int raw_type, int hp, int maxhp)
{
    return type == B_ZERG_BEACON && raw_type == 194 &&
           hp == SC64_LEGACY_BEACON_HP_SENTINEL &&
           maxhp == SC64_LEGACY_BEACON_HP_SENTINEL;
}

#ifdef SC64_P6_RUNTIME
#include "sc64_p6_save.h"
#endif
#ifdef SC64_SHARED_BATTERY
static int race_battery_saved_recharge_valid(const RaceSave *, const Unit *);
#endif
#ifdef SC64_Z6_RUNTIME
#include "sc64_z6_save.h"
#ifdef SC64_Z6_TRANSPORT_ENABLED
#include "sc64_z6_transport_save.h"
#endif
#endif
#ifdef SC64_Z7_RUNTIME
#include "sc64_z7_save.h"
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
#ifdef SC64_SHARED_BATTERY
#include "sc64_p7_battery_rules.h"
/* Decode and validate only the candidate: recharge availability may change
 * between a unit update and save, so power and current energy are not
 * prerequisites for retaining an order. */
static int race_battery_save_unit(const RaceSave *candidate, unsigned slot,
                                  unsigned ww, unsigned wh, Unit *out)
{
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return race_save_decode_unit(candidate, slot, ww, wh, out);
#else
    if (!candidate || !out || slot >= MAX_UNITS) return 0;
    (void)ww; (void)wh;
    for (unsigned j=0; j<RACE_UNIT_WORDS; ++j) {
        int n=candidate->units[slot][j];
        memcpy((u8 *)out+j*sizeof(int), &n, sizeof(n));
    }
    return 1;
#endif
}
static int race_battery_save_encode_unit(RaceSave *candidate, unsigned slot,
                                         unsigned ww, unsigned wh, const Unit *q)
{
    if (!candidate || !q || slot>=MAX_UNITS) return 0;
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_encode(&candidate->units[slot],q,ww,wh);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_unit_encode(&candidate->units[slot],q,ww,wh);
#else
    return sc64_zp_unit_save_encode(&candidate->units[slot],q,ww,wh);
#endif
#elif defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return sc64_unit_save_encode(&candidate->units[slot],q,ww,wh);
#else
    (void)ww; (void)wh;
    for (unsigned j=0;j<RACE_UNIT_WORDS;++j) {
        int n; memcpy(&n,(const u8 *)q+j*sizeof(int),sizeof(n));
        candidate->units[slot][j]=(s16)n;
    }
    return 1;
#endif
}
static int race_battery_save_building(const RaceSave *, unsigned, Building *);
static int race_battery_save_encode_building(RaceSave *candidate, unsigned slot,
                                             unsigned ww, unsigned wh,
                                             const Building *q)
{
    if (!candidate || !q || slot>=MAX_BUILDINGS) return 0;
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_building_encode(&candidate->buildings[slot],q,ww,wh);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_building_encode(&candidate->buildings[slot],q,ww,wh);
#else
    return sc64_building_save_encode(&candidate->buildings[slot],q,ww,wh);
#endif
#elif defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return sc64_building_save_encode(&candidate->buildings[slot],q,ww,wh);
#else
    (void)ww; (void)wh;
    for (unsigned j=0;j<RACE_BUILD_WORDS;++j) {
        int n; memcpy(&n,(const u8 *)q+j*sizeof(int),sizeof(n));
        candidate->buildings[slot][j]=(s16)n;
    }
    return 1;
#endif
}
/* Legacy non-P7 shields used 1/320 point units. Convert only candidate
 * shielded actors; P7 already used 1/1280 and non-shield remainders may
 * carry unrelated regeneration state. This never touches the live world. */
static int race_battery_migrate_shield_phase(RaceSave *candidate, int width, int height)
{
    int marker, legacy_p7=0;
    if (!candidate || width<1 || height<1 || width>128 || height>128) return 0;
    marker=candidate->state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX];
    if (marker!=0 && marker!=SC64_SHIELD_PHASE_SAVE_MARKER) return 0;
    if (marker) return 1;
    /* Old slot reuse did not clear inactive queue cells. They are not
     * producer state when both count and head are zero. Normalize only the
     * decoded legacy candidate; marked saves remain strictly canonical. */
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building q; int changed=0;
        if (!race_battery_save_building(candidate,i,&q)) return 0;
        if (!q.alive || (sc64_race_raw_building(q.type)!=172 && q.raw_type!=172)) continue;
        if (q.queue_count!=0 || q.queue_head!=0) return 0;
        for (unsigned j=0;j<5;++j) if (q.queue[j]) { q.queue[j]=0; changed=1; }
        if (changed && !race_battery_save_encode_building(candidate,i,(unsigned)width*16u,
                                                           (unsigned)height*16u,&q)) return 0;
    }
#ifdef SC64_P7_BATTERY_ENABLED
    legacy_p7=!candidate->state[0] && candidate->state[2]==SC64_P7_GLOBAL_ID;
#endif
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit actor; const Sc64RaceUnitRule *rule;
        if (!race_battery_save_unit(candidate,i,(unsigned)width*16u,(unsigned)height*16u,&actor)) return 0;
        if (!actor.alive) continue;
        if (actor.order==ORDER_RECHARGE) return 0; /* Did not exist before SH12. */
        rule=sc64_race_unit_rule(actor.type);
        if (!rule || !rule->shields || legacy_p7) continue;
        if (actor.shield_remainder<0 || actor.shield_remainder>=320) return 0;
        actor.shield_remainder*=4;
        if (!race_battery_save_encode_unit(candidate,i,(unsigned)width*16u,
                                           (unsigned)height*16u,&actor)) return 0;
    }
    candidate->state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]=SC64_SHIELD_PHASE_SAVE_MARKER;
    return 1;
}
static int race_battery_save_building(const RaceSave *candidate, unsigned slot,
                                      Building *out)
{
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return race_save_decode_building(candidate, slot, out);
#else
    if (!candidate || !out || slot >= MAX_BUILDINGS) return 0;
    for (unsigned j=0; j<RACE_BUILD_WORDS; ++j) {
        int n=candidate->buildings[slot][j];
        memcpy((u8 *)out+j*sizeof(int), &n, sizeof(n));
    }
    return 1;
#endif
}
static int race_battery_saved_fields_valid(const Building *q)
{
    if (!q || !q->alive || sc64_race_raw_building(q->type)!=172 ||
        (q->raw_type!=-1 && q->raw_type!=172) || q->owner<0 || q->owner>=5 ||
        q->raw_owner < -1 || q->raw_owner >= 8 ||
        q->queue_count!=0 || q->queue_head!=0 || q->queue_progress<0 ||
        q->level<0 || !sc64_p7_battery_energy_valid((unsigned)q->queue_progress,
                                                      (unsigned)q->level)) return 0;
    for (unsigned i=0;i<5;++i) if (q->queue[i]!=0) return 0;
    return 1;
}
static int race_battery_saved_recharge_valid(const RaceSave *candidate,
                                              const Unit *actor)
{
    Building battery;
    const Sc64RaceUnitRule *rule;
    if (!candidate || !actor || !actor->alive || actor->order!=ORDER_RECHARGE ||
        actor->owner<0 || actor->owner>=5 || actor->work_timer!=0 ||
        actor->target<0 || actor->target>=MAX_BUILDINGS ||
        actor->raw_type==73 || actor->raw_type==85 ||
        actor->raw_owner < -1 || actor->raw_owner >= 8 ||
        sc64_race_raw_unit(actor->type)<0 ||
        sc64_race_raw_unit(actor->type)==73 || sc64_race_raw_unit(actor->type)==85 ||
        SC64_CARRIER_HANGAR_HIDDEN(actor) ||
        (actor->raw_type!=-1 && actor->raw_type!=sc64_race_raw_unit(actor->type)) ||
        SC64_TRANSPORT_LOADED(actor)) return 0;
#ifdef SC64_P7_RUNTIME
    if (!candidate->state[0] && candidate->state[2]==SC64_P7_GLOBAL_ID) return 0;
#endif
    rule=sc64_race_unit_rule(actor->type);
    if (!rule || rule->race!=R_PROTOSS || !(rule->flags&SC_RACE_F_SHIELDS) ||
        actor->shield<0 || actor->shield>rule->shields || actor->shield_remainder<0 ||
        actor->shield_remainder>=1280 ||
        (actor->shield==rule->shields && actor->shield_remainder!=0) ||
        SC64_BATTERY_UNIT_IMMOVABLE(actor->type) ||
        !race_battery_save_building(candidate,(unsigned)actor->target,&battery) ||
        !race_battery_saved_fields_valid(&battery) || battery.under_construction ||
        battery.owner!=actor->owner) return 0;
    /* Custom units may lack source identities. In campaign, human rescued
     * actors share SIDE; hostile actors with identities must match exactly. */
    if (!candidate->state[0] && actor->owner!=SIDE &&
        (actor->raw_owner<0 || actor->raw_owner!=battery.raw_owner)) return 0;
    return 1;
}
static int race_battery_saved_world_valid(const RaceSave *candidate, int width, int height)
{
    if (!candidate || width<1 || height<1 || width>128 || height>128) return 0;
    for (int i=0;i<MAX_BUILDINGS;++i) {
        Building q;
        if (!race_battery_save_building(candidate,(unsigned)i,&q)) return 0;
        if (q.alive && (sc64_race_raw_building(q.type)==172 || q.raw_type==172) &&
            !race_battery_saved_fields_valid(&q)) return 0;
    }
    for (int i=0;i<MAX_UNITS;++i) {
        Unit q;
        if (!race_battery_save_unit(candidate,(unsigned)i,(unsigned)width*16u,
                                    (unsigned)height*16u,&q)) return 0;
        if (q.alive && q.order==ORDER_RECHARGE &&
            !race_battery_saved_recharge_valid(candidate,&q)) return 0;
    }
    return 1;
}
#endif

#ifdef SC64_SHARED_BUNKER
/* Building cargo is validated solely against the decoded candidate. */
static int race_bunker_save_reference(const RaceSave *candidate, const Unit *actor)
{
    Building bunker;
    int slot, raw;
    if (!candidate || !actor || !actor->alive ||
        (actor->order!=ORDER_BOARD && actor->order!=ORDER_TRANSPORTED) ||
        actor->target<MAX_UNITS || actor->target>=MAX_UNITS+MAX_BUILDINGS)
        return 0;
    slot=actor->target-MAX_UNITS;
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    if (!race_save_decode_building(candidate,(unsigned)slot,&bunker)) return 0;
#else
    for (unsigned j=0;j<RACE_BUILD_WORDS;++j) {
        int n=candidate->buildings[slot][j];
        memcpy((u8 *)&bunker+j*sizeof(int),&n,sizeof(n));
    }
#endif
    raw=actor->raw_type>=0 ? actor->raw_type : sc64_race_raw_unit(actor->type);
    if (!bunker.alive || bunker.under_construction ||
        sc64_race_raw_building(bunker.type)!=125 ||
        (bunker.raw_type!=-1 && bunker.raw_type!=125) ||
        !sc64_bunker_owner_matches(actor->owner,actor->raw_owner,
             bunker.owner,bunker.raw_owner,candidate->state[0],SIDE) ||
        actor->raw_type < -1 || !sc64_bunker_passenger_raw(raw) ||
        sc64_race_unit_from_raw(raw)!=actor->type ||
        sc64_race_unit_flying(actor->type) || (actor->flags & 2)) return 0;
    if (actor->order==ORDER_TRANSPORTED &&
        (actor->selected || actor->x!=bunker.x || actor->y!=bunker.y ||
         actor->path_timer || actor->path_retry || actor->build < -1 ||
         actor->build >= MAX_UNITS+MAX_BUILDINGS || actor->build > 239)) return 0;
    return 1;
}
static int race_bunker_save_world_valid(const RaceSave *candidate, int width, int height)
{
    u8 used[MAX_BUILDINGS]={0};
    if (!candidate || width<1 || height<1 || width>128 || height>128) return 0;
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit actor;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
        if (!race_save_decode_unit(candidate,i,(unsigned)width*16u,(unsigned)height*16u,&actor)) return 0;
#else
        for (unsigned j=0;j<RACE_UNIT_WORDS;++j) {
            int n=candidate->units[i][j];
            memcpy((u8 *)&actor+j*sizeof(int),&n,sizeof(n));
        }
#endif
        if (!actor.alive || (actor.order!=ORDER_BOARD && actor.order!=ORDER_TRANSPORTED) ||
            actor.target<MAX_UNITS) continue;
        if (!race_bunker_save_reference(candidate,&actor)) return 0;
        if (actor.order==ORDER_TRANSPORTED && ++used[actor.target-MAX_UNITS]>SC64_BUNKER_CAPACITY) return 0;
    }
    return 1;
}
#endif
#ifdef SC64_T5_RUNTIME
#include "sc64_t5_save_adapter.h"
#include "sc64_transport_save_adapter.h"
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
#include "sc64_p5_transport_save.h"
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
#include "sc64_z5_consume_save.h"
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
#ifdef SC64_P5_RUNTIME
static int race_p5_saved_phase_valid(const RaceSave *candidate)
{
    const Sc64P5PersistentState *p5;
    unsigned outcome, phase;
    int started, cursor;
    if (!candidate) return 0;
    p5 = &candidate->p5;
    if (!sc64_p5_persistent_state_valid(p5)) return 0;
    outcome = p5->runtime.model.outcome;
    phase = candidate->campaign.phase;
    started = sc64_p5_trigger_mask_test(p5->runtime.model.started, 2);
    cursor = p5->runtime.model.action_cursor[2];
    if (candidate->state[0] || candidate->state[2] != SC64_P5_GLOBAL_ID ||
        candidate->state[1] != R_PROTOSS || candidate->players[SIDE] != R_PROTOSS ||
        candidate->players[RESCUABLE] != R_PROTOSS ||
        candidate->players[ENEMY] != R_ZERG || candidate->players[4] != R_ZERG ||
        candidate->players[NEUTRAL] != R_TER ||
        candidate->campaign.mission != SC64_P5_GLOBAL_ID ||
        candidate->campaign.elapsed_frames != (u32)candidate->state[7] ||
        candidate->campaign.objective_stage != (started ? 2u : 0u)) return 0;
    if (outcome == SC64_P5_RUNNING) {
        if (candidate->state[18] || candidate->state[19] ||
            phase != SC64_RACE_PHASE_ACTIVE ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (outcome == SC64_P5_WON) {
        if (!candidate->state[18] || candidate->state[19] ||
            phase != SC64_RACE_PHASE_COMPLETE ||
            !(candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
            !(candidate->state[3] & (1 << SC64_P5_GLOBAL_ID))) return 0;
    } else if (outcome == SC64_P5_LOST) {
        if (candidate->state[18] || !candidate->state[19] ||
            phase != SC64_RACE_PHASE_FAILED ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else return 0;
    (void)cursor;
    return 1;
}

static unsigned race_p5_saved_remaining_creations(const Sc64P5PersistentState *p5)
{
    const unsigned trigger = 3;
    const Sc64P5SourceTrigger *tr = &sc64_p5_source_triggers[trigger];
    unsigned remaining = 0;
    int started = sc64_p5_trigger_mask_test(p5->runtime.model.started, (u8)trigger);
    /* The source record's started bit remains set after completion, while its
     * cursor returns to zero. Do not reserve already-created actors then. */
    if (started && !p5->runtime.model.action_cursor[trigger]) return 0;
    for (unsigned i = 0; i < tr->action_count; ++i) {
        const Sc64P5SourceAction *a = &sc64_p5_source_actions[tr->first_action + i];
        if (a->opcode == SC64_P5_ACTION_CREATE_UNIT &&
            (!started || i >= p5->runtime.model.action_cursor[trigger]))
            remaining += sc64_p5_source_create_count(a);
    }
    return remaining;
}

static int race_p5_save_candidate_valid(const RaceSave *candidate,
                                        unsigned width, unsigned height)
{
    unsigned live = 0, queued = 0;
    if (!candidate || width != SC64_P5_MAP_COLS || height != SC64_P5_MAP_ROWS ||
        !race_p5_saved_phase_valid(candidate)) return 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit decoded;
        if (!race_save_decode_unit(candidate, i, width * 16u, height * 16u, &decoded))
            return 0;
        live += decoded.alive != 0;
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building decoded;
        if (!race_save_decode_building(candidate, i, &decoded)) return 0;
        if (decoded.alive) for (int q = 0; q < decoded.queue_count; ++q)
            queued += decoded.queue[(decoded.queue_head + q) % 5] == U_ZERGLING ? 2u : 1u;
    }
    return live + queued + race_p5_saved_remaining_creations(&candidate->p5) <= MAX_UNITS;
}
#endif
#ifdef SC64_P7_BATTERY_ENABLED
#include "sc64_p7_projectile_save_adapter.h"

#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
static int race_p7_carrier_training_save_valid(const RaceSave *candidate,
                                               unsigned ww,unsigned wh)
{
    /* The save occupies 16292 bytes; the candidate actor table fits in the
     * unused tail of the existing 32768-byte route scratch union. */
    Unit *actors=sc64_zp_route_save_scratch.carrier_candidate.units;
#ifndef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
    /* This generation cannot launch: reject impossible in-flight packets,
     * including a packet that names a valid but docked child as its shooter. */
    if (sc64_codec_allocated_count(candidate->interceptor_projectiles,
                                  SC64_CODEC_BYTES)!=0) return 0;
#endif
    for (unsigned i=0;i<MAX_UNITS;++i)
        if (!race_save_decode_unit(candidate,i,ww,wh,&actors[i])) return 0;
    return race_p7_carrier_training_candidate_valid(actors,MAX_UNITS);
}
#endif

/* Shared Protoss mission payloads are zero-filled before serialization. Check
 * bytes not owned by the candidate schema to reject stale larger-profile data. */
static int race_p7_union_tail_valid(const RaceSave *candidate)
{
    size_t active_size=0, union_size=0;
    if (!candidate) return 0;
#ifdef SC64_P5_RUNTIME
    union_size=sizeof(candidate->p5);
#endif
#ifdef SC64_P6_RUNTIME
    if (sizeof(candidate->p6)>union_size) union_size=sizeof(candidate->p6);
#endif
    if (sizeof(candidate->p7)>union_size) union_size=sizeof(candidate->p7);
    if (candidate->state[0]==0) {
#ifdef SC64_P5_RUNTIME
        if (candidate->state[2]==SC64_P5_GLOBAL_ID)
            active_size=sizeof(candidate->p5);
#endif
#ifdef SC64_P6_RUNTIME
        if (candidate->state[2]==SC64_P6_GLOBAL_ID)
            active_size=sizeof(candidate->p6);
#endif
        if (candidate->state[2]==SC64_P7_GLOBAL_ID)
            active_size=sizeof(candidate->p7);
    }
    if (active_size>union_size) return 0;
    const u8 *bytes=(const u8 *)&candidate->p7;
    for (size_t i=active_size;i<union_size;++i)
        if (bytes[i]) return 0;
    return 1;
}
#endif
#ifdef SC64_TRANSPORT_RUNTIME
/* Custom-game cargo uses the common Dropship runtime, independently of the
 * stricter imported-mission Shuttle and Overlord provenance policies. Inspect
 * only the candidate, before changing the live world. */
static int race_skirmish_transport_decode(const RaceSave *candidate, int slot,
                                           int width, int height, Unit *out)
{
    if (!candidate || !out || slot < 0 || slot >= MAX_UNITS) return 0;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return race_save_decode_unit(candidate, (unsigned)slot,
                                 (unsigned)width * 16u, (unsigned)height * 16u, out);
#else
    (void)width; (void)height;
    for (unsigned j=0; j<RACE_UNIT_WORDS; ++j) {
        int n=candidate->units[slot][j];
        memcpy((u8 *)out+j*sizeof(int), &n, sizeof(n));
    }
    return 1;
#endif
}

static int race_skirmish_saved_dropship(const Unit *ship)
{
    return ship && ship->alive && ship->type == U_DROPSHIP &&
        sc64_race_unit_from_raw(SC64_TRANSPORT_DROPSHIP_RAW) == U_DROPSHIP &&
        (ship->raw_type == -1 || ship->raw_type == SC64_TRANSPORT_DROPSHIP_RAW) &&
        ship->owner >= 0 && ship->owner < 5 &&
        ship->order != ORDER_BOARD && ship->order != ORDER_TRANSPORTED;
}

static int race_skirmish_transport_save_valid(const RaceSave *candidate,
                                                int width, int height)
{
    u8 used[MAX_UNITS] = {0};
    if (!candidate || candidate->state[0] != 1 || width < 1 || height < 1 ||
        width > 128 || height > 128) return 0;
    for (int i=0; i<MAX_UNITS; ++i) {
        Unit actor;
        if (!race_skirmish_transport_decode(candidate, i, width, height, &actor)) return 0;
        if (!actor.alive) continue;
        if (actor.order == ORDER_UNLOAD) {
            if (!race_skirmish_saved_dropship(&actor) || actor.target != -1 ||
                actor.work_timer < 0 || actor.work_timer > SC64_TRANSPORT_UNLOAD_TICKS ||
                actor.tx < 0 || actor.tx >= width*16 || actor.ty < 0 || actor.ty >= height*16)
                return 0;
        }
        if (actor.order != ORDER_BOARD && actor.order != ORDER_TRANSPORTED) continue;
#ifdef SC64_SHARED_BUNKER
        if (actor.target>=MAX_UNITS) continue; /* checked by candidate-wide pass */
#endif
        Unit carrier; unsigned space;
        int raw = actor.raw_type < 0 ? sc64_race_raw_unit(actor.type) : actor.raw_type;
        if (actor.target < 0 || actor.target >= MAX_UNITS || actor.target == i ||
            !race_skirmish_transport_decode(candidate, actor.target, width, height, &carrier) ||
            !race_skirmish_saved_dropship(&carrier) || actor.owner != carrier.owner ||
            (actor.flags & 2) || sc64_race_unit_flying(actor.type) ||
            raw < 0 || (sc64_race_unit_from_raw(raw) != actor.type &&
                        !(raw == 19 && actor.type == U_RAYNOR)) ||
            !sc64_transport_space_required_raw((unsigned)raw, &space) ||
            space == SC64_TRANSPORT_SPACE_UNAVAILABLE) return 0;
        if (actor.order == ORDER_TRANSPORTED) {
            if (actor.selected || actor.x != carrier.x || actor.y != carrier.y ||
                actor.path_timer || actor.path_retry ||
                (unsigned)used[actor.target] + space > SC64_TRANSPORT_DROPSHIP_SPACE) return 0;
            used[actor.target] = (u8)(used[actor.target] + space);
        }
        /* Pending board orders are not reservations: the runtime rechecks
         * space on arrival and cancels excess orders. */
    }
    return 1;
}
#endif

static int race_save_game(void) {
#ifdef SC64_Z5_CLOAK_ENABLED
    if (race_z5_active() && !race_z5_cloak_world_state_valid()) return 0;
#endif
#ifdef SC64_Z5_RUNTIME
    if (race_z5_active()) for (unsigned i = 0; i < MAX_UNITS; ++i)
        if ((u[i].type == U_ZERGLING || u[i].type == U_ZP_INFESTED_KERRIGAN) &&
            u[i].shield_remainder >= SC64_Z5_HP_REGEN_TICKS_PER_HP) return 0;
#endif
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
#if !defined(SC64_T6_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
    /* A malformed or partially-damaged beacon cannot be represented by the
     * legacy 16-bit row. Fail closed instead of truncating either HP field. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_ZERG_BEACON &&
            (b[i].raw_type != 194 ||
             !((b[i].hp == 100000 && b[i].maxhp == 100000) ||
               (b[i].maxhp == 10000 && b[i].hp > 0 && b[i].hp <= 10000)) ||
             b[i].under_construction ||
             b[i].queue_count || b[i].shield)) return 0;
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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
        if(!sc64_p7_typed_unit_encode(&race_data.units[i], &u[i], (unsigned)(race_cols * 16), (unsigned)(race_rows * 16))) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
        if(!sc64_derived_unit_encode(&race_data.units[i], &u[i],
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16))) return 0;
#else
        if(!sc64_zp_unit_save_encode(&race_data.units[i], &u[i],
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16))) return 0;
#endif
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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(!sc64_p7_typed_building_encode(&race_data.buildings[i], &b[i], (unsigned)(race_cols * 16), (unsigned)(race_rows * 16))) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(!sc64_derived_building_encode(&race_data.buildings[i], &b[i],
                                        (unsigned)(race_cols * 16),
                                        (unsigned)(race_rows * 16))) return 0;
#else
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(!sc64_building_save_encode(&race_data.buildings[i], &b[i],
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16))) return 0;
#endif
#else
    for(int i=0;i<MAX_BUILDINGS;i++)for(unsigned j=0;j<RACE_BUILD_WORDS;j++) {
        int n;memcpy(&n,(u8*)&b[i]+j*sizeof(int),sizeof(n));
#if !defined(SC64_T6_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
        if (b[i].alive && b[i].type == B_ZERG_BEACON && b[i].raw_type == 194 &&
            b[i].hp == 100000 && b[i].maxhp == 100000 &&
            (j == offsetof(Building,hp)/sizeof(int) ||
             j == offsetof(Building,maxhp)/sizeof(int)))
            n = SC64_LEGACY_BEACON_HP_SENTINEL;
#endif
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
    race_data.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX]=SC64_BUILDING_DAMAGE_SAVE_MARKER;
#ifdef SC64_SHARED_BATTERY
    race_data.state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]=SC64_SHIELD_PHASE_SAVE_MARKER;
#endif
#ifdef SC64_CONSTRUCTION_HP
    race_data.state[SC64_CONSTRUCTION_SAVE_MARKER_INDEX] = SC64_CONSTRUCTION_SAVE_MARKER;
#endif
    memcpy(race_data.players,race_player,sizeof(race_player));
#ifdef SC64_RACES
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && SC64_PENDING_BUILD_ORDER(u[i].order) &&
            !sc64_pending_build_save_identity_valid(&race_data,
                u[i].alive, u[i].owner, u[i].type, u[i].target,
                u[i].tx, u[i].ty, race_cols * 16, race_rows * 16)) return 0;
#endif
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
#ifdef SC64_P5_RUNTIME
    if (race_p5_active()) {
        if (!race_p5_runtime_state_valid()) return 0;
        race_data.p5 = race_p5;
    }
#if defined(SC64_P6_RUNTIME) || defined(SC64_P7_BATTERY_ENABLED)
    else if (
#ifdef SC64_P6_RUNTIME
               !race_p6_active() &&
#endif
#ifdef SC64_P7_BATTERY_ENABLED
               !race_p7_active() &&
#endif
               1) {
        memset(&race_data.p5, 0, sizeof(race_data.p5));
    }
#endif
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) {
        if (!race_p6_runtime_state_valid() || !race_p6_doodad_bindings_still_valid()
#ifdef SC64_P6_STORM_ENABLED
            || !race_p6_storm_world_state_valid()
#endif
            ) return 0;
        race_data.p6 = race_p6;
#if defined(SC64_P5_RUNTIME) || defined(SC64_P7_BATTERY_ENABLED)
    } else if (
#ifdef SC64_P5_RUNTIME
               !race_p5_active() &&
#endif
#ifdef SC64_P7_BATTERY_ENABLED
               !race_p7_active() &&
#endif
               1) {
#endif
        memset(&race_data.p6, 0, sizeof(race_data.p6));
    }
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (race_p7_active()) {
        if (!race_p7_runtime_state_valid() ||
            !race_p7_battery_live_world_valid()) return 0;
        race_data.p7=race_p7;
    } else if (
#ifdef SC64_P5_RUNTIME
               !race_p5_active() &&
#endif
#ifdef SC64_P6_RUNTIME
               !race_p6_active() &&
#endif
               1) {
        memset(&race_data.p7,0,sizeof(race_data.p7));
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
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) {
        if (!sc64_z6_persistent_state_valid(&race_z6)
#ifdef SC64_Z6_CLOAK_ENABLED
            || !race_z6_cloak_world_state_valid()
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
            || !race_z6_transport_state_valid()
#endif
            ) return 0;
        race_data.z6 = race_z6;
#ifdef SC64_Z5_RUNTIME
    } else if (!race_z5_active()) {
#endif
        memset(&race_data.z6, 0, sizeof(race_data.z6));
    }
#endif
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) {
        if (!sc64_z7_persistent_state_valid(&race_z7) ||
            !race_z7_live_world_valid()) return 0;
        race_data.z7 = race_z7;
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
#ifdef SC64_P5_RUNTIME
    if (race_p5_active()) {
        if (!race_p5_save_candidate_valid(&race_data, (unsigned)race_cols,
                                          (unsigned)race_rows)) return 0;
    }
#if defined(SC64_P6_RUNTIME) || defined(SC64_P7_BATTERY_ENABLED)
    else if (
#ifdef SC64_P6_RUNTIME
             !race_p6_active() &&
#endif
#ifdef SC64_P7_BATTERY_ENABLED
             !race_p7_active() &&
#endif
             1) {
#else
    else {
#endif
        const u8 *bytes = (const u8 *)&race_data.p5;
        for (unsigned i = 0; i < sizeof(race_data.p5); ++i)
            if (bytes[i]) return 0;
    }
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) {
        if (!race_p6_save_candidate_valid(&race_data, (unsigned)race_cols,
                                          (unsigned)race_rows)) return 0;
    }
#if defined(SC64_P5_RUNTIME) || defined(SC64_P7_BATTERY_ENABLED)
    else if (
#ifdef SC64_P5_RUNTIME
             !race_p5_active() &&
#endif
#ifdef SC64_P7_BATTERY_ENABLED
             !race_p7_active() &&
#endif
             1) {
#else
    else {
#endif
        const u8 *bytes = (const u8 *)&race_data.p6;
        for (unsigned i = 0; i < sizeof(race_data.p6); ++i)
            if (bytes[i]) return 0;
    }
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (race_p7_active()) {
        if (!sc64_p7_persistent_state_valid(&race_data.p7) ||
            !race_p7_battery_live_world_valid()) return 0;
    } else if (
#ifdef SC64_P5_RUNTIME
             !race_p5_active() &&
#endif
#ifdef SC64_P6_RUNTIME
             !race_p6_active() &&
#endif
             1) {
        const u8 *bytes=(const u8 *)&race_data.p7;
        for (unsigned i=0;i<sizeof(race_data.p7);++i) if (bytes[i]) return 0;
    }
#if defined(SC64_P5_RUNTIME) && defined(SC64_P6_RUNTIME)
    if (race_p7_active()) {
        const u8 *bytes=(const u8 *)&race_data.p7;
        for (unsigned i=sizeof(race_data.p7);i<sizeof(race_data.p6);++i)
            if (bytes[i]) return 0;
    }
#endif
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) {
        if (!race_z6_save_candidate_valid(&race_data, &race_data.z6,
                                          (unsigned)race_cols,
                                          (unsigned)race_rows)) return 0;
#ifdef SC64_Z5_RUNTIME
        {
            const u8 *payload = (const u8 *)&race_data.z6;
            for (unsigned i = sizeof(race_data.z6); i < sizeof(race_data.z5); ++i)
                if (payload[i]) return 0;
        }
#endif
    } else {
#ifdef SC64_Z5_RUNTIME
        if (race_z5_active()) {
            /* Z5 owns the larger shared payload. */
        } else
#endif
#ifdef SC64_Z7_RUNTIME
        if (race_z7_active()) {
            /* Z7 is checked below using its smaller shared schema. */
        } else
#endif
        {
            const u8 *bytes = (const u8 *)&race_data.z6;
            for (unsigned i = 0; i < sizeof(race_data.z6); ++i)
                if (bytes[i]) return 0;
        }
    }
#endif
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) {
        if (!race_z7_save_candidate_valid(&race_data, &race_data.z7,
                (unsigned)race_cols, (unsigned)race_rows)) return 0;
        const u8 *payload=(const u8 *)&race_data.z5;
        for (unsigned i=sizeof(race_data.z7); i<sizeof(race_data.z5); ++i)
            if (payload[i]) return 0;
    }
#endif
#ifdef SC64_SHARED_BUNKER
    if (!race_bunker_save_world_valid(&race_data, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    if (race_data.state[0] && !race_skirmish_transport_save_valid(&race_data, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_SHARED_BATTERY
    if (!race_battery_saved_world_valid(&race_data, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (!race_transport_save_candidate_valid(&race_data, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (!race_data.state[0] && !race_p5_transport_save_candidate_valid(&race_data, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (!race_data.state[0] && !race_z6_transport_save_candidate_valid(&race_data, &race_data.z6.transport, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
    if (!race_z5_consume_save_candidate_valid(&race_data, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_NYDUS_ENABLED
    if (!race_nydus_save_candidate_valid(&race_data, race_cols, race_rows)) return 0;
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (!race_p7_union_tail_valid(&race_data)) return 0;
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    if (!race_p7_projectile_encode(race_data.interceptor_projectiles) ||
        !race_p7_projectile_save_candidate_valid(&race_data,
            (unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (!race_p7_carrier_training_save_valid(&race_data,
            (unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
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
#ifdef SC64_P7_BATTERY_ENABLED
    if (!race_p7_union_tail_valid(&race_data)) return 0;
#endif
    const int *v=race_data.state;
    int building_damage_marker=v[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX];
    if (building_damage_marker && building_damage_marker!=SC64_BUILDING_DAMAGE_SAVE_MARKER) return 0;
#ifdef SC64_CONSTRUCTION_HP
    int construction_save_marker = v[SC64_CONSTRUCTION_SAVE_MARKER_INDEX];
    if (construction_save_marker != 0 &&
        construction_save_marker != SC64_CONSTRUCTION_SAVE_MARKER) return 0;
#endif
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
#ifdef SC64_P6_RUNTIME
       &&v[2]!=SC64_P6_GLOBAL_ID
#endif
#ifdef SC64_P7_BATTERY_ENABLED
       &&v[2]!=SC64_P7_GLOBAL_ID
#endif
#ifdef SC64_Z6_RUNTIME
       &&v[2]!=SC64_Z6_GLOBAL_ID
#endif
#ifdef SC64_Z7_RUNTIME
       &&v[2]!=SC64_Z7_GLOBAL_ID
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
#ifdef SC64_P6_RUNTIME
        &&v[2]!=SC64_P6_GLOBAL_ID
#endif
#ifdef SC64_P7_BATTERY_ENABLED
        &&v[2]!=SC64_P7_GLOBAL_ID
#endif
#ifdef SC64_Z6_RUNTIME
        &&v[2]!=SC64_Z6_GLOBAL_ID
#endif
#ifdef SC64_Z7_RUNTIME
        &&v[2]!=SC64_Z7_GLOBAL_ID
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
        &&(v[2]<SC64_TERRAN_LATE_GLOBAL_T7||
           v[2]>SC64_TERRAN_LATE_GLOBAL_T10)
#endif
        )||!sc64_race_maps[v[2]].width)return 0;
    if((unsigned)v[3]&~(sc64_catalog_playable_mask(
#ifdef SC64_CAMPAIGN_RACE
                                               SC64_CAMPAIGN_RACE
#else
                                               -1
#endif
                                               )
#ifdef SC64_P7_BATTERY_ENABLED
       | (1u << SC64_P7_GLOBAL_ID)
#endif
#ifdef SC64_T5_RUNTIME
       | (1u << SC64_T5_GLOBAL_ID)
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
       | ((1u << SC64_TERRAN_LATE_GLOBAL_T7) |
           (1u << SC64_TERRAN_LATE_GLOBAL_T8) |
           (1u << SC64_TERRAN_LATE_GLOBAL_T9) |
           (1u << SC64_TERRAN_LATE_GLOBAL_T10))
#endif
       ))return 0;
#endif
    int width=v[0]?64:sc64_race_maps[v[2]].width;
    int height=v[0]?64:sc64_race_maps[v[2]].height;
#ifdef SC64_IMPORTED_SKIRMISH
    if(v[20]<0||v[20]>SC64_SKIRMISH_IMPORTED_COUNT)return 0;
    for(int i=21;i<24;i++)if(v[i]<0||v[i]>2)return 0;
    if(v[0]&&v[20]>0){width=sc64_skirmish_maps[v[20]-1].width;height=sc64_skirmish_maps[v[20]-1].height;}
#endif
    if(!sc64_race_storage_geometry_valid(width,height))return 0;
#ifdef SC64_SHARED_BATTERY
    if (!race_battery_migrate_shield_phase(&race_data,width,height)) return 0;
#endif
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
            RACE_U(i,order)<ORDER_IDLE||(RACE_U(i,order)>
#ifdef SC64_NYDUS_ENABLED
            ORDER_NYDUS_ENTER && !SC64_PENDING_BUILD_ORDER(RACE_U(i,order))
#ifdef SC64_SHARED_BATTERY
            && RACE_U(i,order)!=ORDER_RECHARGE
#endif
            )||
#elif defined(SC64_P7_BATTERY_ENABLED)
            ORDER_P7_RECHARGE && !SC64_PENDING_BUILD_ORDER(RACE_U(i,order))
#ifdef SC64_SHARED_BATTERY
            && RACE_U(i,order)!=ORDER_RECHARGE
#endif
            )||
#elif defined(SC64_T6_RUNTIME)
            ORDER_REPAIR && !SC64_PENDING_BUILD_ORDER(RACE_U(i,order))
#ifdef SC64_SHARED_BATTERY
            && RACE_U(i,order)!=ORDER_RECHARGE
#endif
            )||
#elif defined(SC64_TRANSPORT_RUNTIME)
            ORDER_UNLOAD && !SC64_PENDING_BUILD_ORDER(RACE_U(i,order))
#ifdef SC64_SHARED_BATTERY
            && RACE_U(i,order)!=ORDER_RECHARGE
#endif
            )||
#elif defined(SC64_Z5_CONSUME_ENABLED)
            ORDER_Z5_CONSUME && !SC64_PENDING_BUILD_ORDER(RACE_U(i,order))
#ifdef SC64_SHARED_BATTERY
            && RACE_U(i,order)!=ORDER_RECHARGE
#endif
            )||
#else
            ORDER_GAS && !SC64_PENDING_BUILD_ORDER(RACE_U(i,order))
#ifdef SC64_SHARED_BATTERY
            && RACE_U(i,order)!=ORDER_RECHARGE
#endif
            )||
#endif
            RACE_U(i,shield)<0||
            RACE_U(i,shield_remainder)<0||RACE_U(i,shield_remainder)>=1280)return 0;
        int target=RACE_U(i,target),order=RACE_U(i,order);
        if((order==ORDER_MINE&&(target<0||target>=MAX_MINERALS))||
           ((order==ORDER_GAS||order==ORDER_BUILD)&&(target<0||target>=MAX_BUILDINGS)))return 0;
        if (SC64_PENDING_BUILD_ORDER(order) &&
            !sc64_pending_build_save_identity_valid(&race_data,
                RACE_U(i,alive), RACE_U(i,owner), RACE_U(i,type), target,
                RACE_U(i,tx), RACE_U(i,ty), width * 16, height * 16)) return 0;
#ifdef SC64_P7_BATTERY_ENABLED
        if (order==ORDER_P7_RECHARGE) {
            /* Validate persistent identities, not temporary availability.
             * Another actor may empty the Battery or kill its last Pylon
             * after this actor's slot; passive regeneration can fill shields.
             * A pause/save preserves that order until its next service tick. */
            Building target_battery;
            const Sc64RaceUnitRule *unit_rule=sc64_race_unit_rule(candidate_unit.type);
            if (race_data.state[0] != 0 ||
                race_data.state[2] != SC64_P7_GLOBAL_ID ||
                candidate_unit.raw_type==73 || candidate_unit.raw_type==85 ||
                candidate_unit.raw_owner<0 || candidate_unit.raw_owner>=8 ||
                candidate_unit.work_timer!=0 || !unit_rule ||
                unit_rule->race!=R_PROTOSS ||
                !(unit_rule->flags&SC_RACE_F_SHIELDS) ||
                candidate_unit.shield>unit_rule->shields ||
                (candidate_unit.shield==unit_rule->shields &&
                 candidate_unit.shield_remainder!=0) ||
                (sc64_game_unit_actor_flags(candidate_unit.type) &&
                 !(sc64_game_unit_actor_flags(candidate_unit.type)&SC64_GAME_ACTOR_MOVABLE)) ||
                SC64_TRANSPORT_LOADED(&candidate_unit) || target<0 ||
                target>=MAX_BUILDINGS ||
                !race_save_decode_building(&race_data,(unsigned)target,&target_battery) ||
                !target_battery.alive || target_battery.type!=B_P7_SHIELD_BATTERY ||
                target_battery.raw_type!=172 || target_battery.under_construction ||
                target_battery.owner!=candidate_unit.owner ||
                target_battery.raw_owner!=candidate_unit.raw_owner) return 0;
        }
#endif
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
#ifdef SC64_P7_BATTERY_ENABLED
    unsigned p7_battery_seen=0;
#endif
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
#ifdef SC64_CONSTRUCTION_HP
        /* The legacy clock saved no incremental HP. Its partially built
         * foundations and morphs cannot be converted without inventing lost
         * health; fail before applying any candidate state to the world. */
        if (!construction_save_marker && RACE_B(i,alive) &&
            RACE_B(i,under_construction)) return 0;
#endif
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
        int health_ok=RACE_B(i,hp)>0&&RACE_B(i,maxhp)>0&&
                      RACE_B(i,hp)<=RACE_B(i,maxhp);
        if (sc64_legacy_beacon_hp_sentinel(RACE_B(i,type),RACE_B(i,raw_type),
                                           RACE_B(i,hp),RACE_B(i,maxhp)) ||
            (RACE_B(i,type)==B_ZERG_BEACON && RACE_B(i,raw_type)==194 &&
             RACE_B(i,maxhp)==10000 && RACE_B(i,hp)>0 &&
             RACE_B(i,hp)<=10000)) {
            /* The legacy s16 codec uses this exact pair only for the
             * invulnerable stock Zerg Beacon. */
            health_ok = !RACE_B(i,under_construction) &&
                        !RACE_B(i,queue_count) && !RACE_B(i,shield);
        }
#ifdef SC64_T4_RUNTIME
        if(sc64_race_t4_building_intrinsically_invincible(RACE_B(i,type)))
            health_ok=1; /* The profile-specific exact prop value was checked above. */
#endif
        if(RACE_B(i,type)<0||RACE_B(i,type)>=SC_RB_COUNT||RACE_B(i,owner)<0||RACE_B(i,owner)>4||
            !health_ok||
            RACE_B(i,x)<0||RACE_B(i,x)>=width*16||RACE_B(i,y)<0||RACE_B(i,y)>=height*16||
            RACE_B(i,queue_count)<0||RACE_B(i,queue_count)>5||RACE_B(i,queue_head)<0||RACE_B(i,queue_head)>4||
            RACE_B(i,queue_progress)<0||RACE_B(i,build_progress)<-1||RACE_B(i,shield)<0||
            RACE_B(i,larva)<0||RACE_B(i,larva)>3||RACE_B(i,shield_remainder)<0||RACE_B(i,shield_remainder)>=1280)return 0;
#ifdef SC64_P7_BATTERY_ENABLED
        if (!race_data.state[0] && race_data.state[2]==SC64_P7_GLOBAL_ID &&
            candidate_building.type==B_P7_SHIELD_BATTERY) {
            if (candidate_building.raw_type!=172 || candidate_building.raw_owner<0 ||
                candidate_building.raw_owner>=8 ||
                (!candidate_building.under_construction && candidate_building.build_progress!=0) ||
                candidate_building.queue_count!=0 ||
                candidate_building.queue_head!=0 || candidate_building.queue_progress<0 ||
                candidate_building.queue_progress>SC64_P7_BATTERY_MAX_ENERGY_Q8 ||
                candidate_building.level<0 || candidate_building.level>=5 ||
                !sc64_p7_battery_energy_valid((unsigned)candidate_building.queue_progress,
                                              (unsigned)candidate_building.level)) return 0;
            for (int q=0;q<5;++q) if (candidate_building.queue[q]!=0) return 0;
            if ((candidate_building.raw_owner==0 || candidate_building.raw_owner==4) &&
                candidate_building.owner!=race_p7_source_owner((unsigned)candidate_building.raw_owner))
                return 0;
            if (candidate_building.raw_owner==0 || candidate_building.raw_owner==4) {
                unsigned pidx=candidate_building.raw_owner==0?0u:1u;
                const Sc64P7BatterySourceProperty *want=&sc64_p7_battery_source_properties[pidx];
                if (candidate_building.x!=(int)(want->x_source/2u) ||
                    candidate_building.y!=(int)(want->y_source/2u) ||
                    candidate_building.shield>sc64_race_building_shields(B_P7_SHIELD_BATTERY) ||
                    (p7_battery_seen&(1u<<pidx))) return 0;
                p7_battery_seen|=1u<<pidx;
            }
            if (candidate_building.raw_owner==7 && candidate_building.owner!=SIDE) return 0;
            if (candidate_building.raw_owner!=0 && candidate_building.raw_owner!=4 &&
                candidate_building.raw_owner!=7) return 0;
        }
#endif
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
#ifdef SC64_SHARED_BUNKER
    if (!race_bunker_save_world_valid(&race_data, width, height)) return 0;
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    if (race_data.state[0] && !race_skirmish_transport_save_valid(&race_data, width, height)) return 0;
#endif
#ifdef SC64_SHARED_BATTERY
    if (!race_battery_saved_world_valid(&race_data, width, height)) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (!race_t5_candidate_queues_available(&race_data)) return 0;
    if (!race_transport_save_candidate_valid(&race_data, width, height)) return 0;
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (!race_data.state[0] && !race_p5_transport_save_candidate_valid(&race_data, width, height)) return 0;
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (!race_data.state[0] && !race_z6_transport_save_candidate_valid(&race_data, &race_data.z6.transport, width, height)) return 0;
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
    if (!race_z5_consume_save_candidate_valid(&race_data, width, height)) return 0;
#endif
#ifdef SC64_NYDUS_ENABLED
    if (!race_nydus_save_candidate_valid(&race_data, width, height)) return 0;
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
#ifdef SC64_P5_RUNTIME
    if (!v[0] && v[2] == SC64_P5_GLOBAL_ID) {
        if (!race_p5_save_candidate_valid(&race_data, (unsigned)width,
                                          (unsigned)height)) return 0;
    } else {
#if defined(SC64_P6_RUNTIME)
        if (!v[0] && v[2] == SC64_P6_GLOBAL_ID) {
            /* P5 and P6 share a union; P6 owns the payload in this case. */
        } else
#endif
#if defined(SC64_P7_BATTERY_ENABLED)
        if (!v[0] && v[2] == SC64_P7_GLOBAL_ID) {
            /* P7 shares the Protoss mission payload union. */
        } else
#endif
        {
        const u8 *bytes = (const u8 *)&race_data.p5;
        for (unsigned i = 0; i < sizeof(race_data.p5); ++i)
            if (bytes[i]) return 0;
        }
    }
#if defined(SC64_P6_RUNTIME)
    if (!v[0] && v[2] == SC64_P5_GLOBAL_ID) {
        const u8 *payload = (const u8 *)&race_data.p5;
        for (unsigned i = sizeof(race_data.p5); i < sizeof(race_data.p6); ++i)
            if (payload[i]) return 0;
    }
#endif
#endif
#ifdef SC64_P6_RUNTIME
    if (!v[0] && v[2] == SC64_P6_GLOBAL_ID) {
        if (!race_p6_save_candidate_valid(&race_data, (unsigned)width,
                                           (unsigned)height)) return 0;
    } else {
#if defined(SC64_P5_RUNTIME)
        if (!v[0] && v[2] == SC64_P5_GLOBAL_ID) {
            /* P5 owns the payload in the shared union. */
        } else
#endif
#if defined(SC64_P7_BATTERY_ENABLED)
        if (!v[0] && v[2] == SC64_P7_GLOBAL_ID) {
            /* P7 owns the payload in the shared union. */
        } else
#endif
        {
            const u8 *bytes = (const u8 *)&race_data.p6;
            for (unsigned i = 0; i < sizeof(race_data.p6); ++i)
                if (bytes[i]) return 0;
        }
    }
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (!v[0] && v[2]==SC64_P7_GLOBAL_ID) {
        const Sc64P7PersistentState *p7=&race_data.p7;
        const Sc64RaceMap *map=&sc64_race_maps[SC64_P7_GLOBAL_ID];
        unsigned outcome=p7->runtime.model.outcome;
        unsigned phase=race_data.campaign.phase;
        unsigned live=0,queued=0,creates_remaining=0;
        if (!sc64_p7_persistent_state_valid(p7) ||
            !race_p7_map_shape_valid(map) ||
            race_data.campaign.mission!=SC64_P7_GLOBAL_ID ||
            race_data.campaign.elapsed_frames!=(u32)v[7]) return 0;
        for (unsigned i=0;i<MAX_UNITS;++i) {
            Unit candidate;
            if (!race_save_decode_unit(&race_data,i,(unsigned)(width*16),
                                       (unsigned)(height*16),&candidate)) return 0;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
            if (!sc64_p7_arbiter_cache_unit_valid(&candidate)) return 0;
#endif
            live+=candidate.alive!=0;
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
            if (candidate.alive && candidate.type==U_P7_CARRIER && candidate.carry>0)
                queued+=(unsigned)candidate.carry;
#endif
        }
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building candidate;
            if (!race_save_decode_building(&race_data,i,&candidate)) return 0;
            if (candidate.alive) for (int q=0;q<candidate.queue_count;++q)
                queued+=candidate.queue[(candidate.queue_head+q)%5]==U_ZERGLING?2u:1u;
        }
        if (sc64_p7_trigger_started(&p7->runtime.model,5)) {
            u8 cursor=p7->runtime.model.action_cursor[5];
            for (unsigned i=0;i<3;++i)
                if (!(p7->created_mask&(1u<<i)) && (!cursor || cursor<(u8)(9u+i)))
                    ++creates_remaining;
        } else creates_remaining=3;
        if (live+queued+creates_remaining>MAX_UNITS) return 0;
        if (outcome==SC64_P7_RUNNING) {
            if (v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
                (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else if (outcome==SC64_P7_WON) {
            if (!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
                !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)||
                !(v[3]&(1<<SC64_P7_GLOBAL_ID))) return 0;
        } else if (outcome==SC64_P7_LOST) {
            if (v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
                (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else return 0;
    } else {
        /* P5/P6 also own this union. Their nonzero mission payload is checked
         * by those profile validators; only unrelated missions require zero. */
#if defined(SC64_P5_RUNTIME)
        if (!v[0] && v[2]==SC64_P5_GLOBAL_ID) {
            /* P5 owns the shared payload. */
        } else
#endif
#if defined(SC64_P6_RUNTIME)
        if (!v[0] && v[2]==SC64_P6_GLOBAL_ID) {
            /* P6 owns the shared payload. */
        } else
#endif
        {
            const u8 *bytes=(const u8 *)&race_data.p7;
            for (unsigned i=0;i<sizeof(race_data.p7);++i)
                if (bytes[i]) return 0;
        }
    }
#if defined(SC64_P5_RUNTIME) && defined(SC64_P6_RUNTIME)
    if (!v[0] && v[2]==SC64_P7_GLOBAL_ID) {
        const u8 *bytes=(const u8 *)&race_data.p7;
        for (unsigned i=sizeof(race_data.p7);i<sizeof(race_data.p6);++i)
            if (bytes[i]) return 0;
    }
#endif
#endif
#ifdef SC64_Z5_RUNTIME
    if (!v[0] && v[2] == SC64_Z5_GLOBAL_ID) {
        const Sc64Z5PersistentState *z5 = &race_data.z5;
        const Sc64RaceMap *map = &sc64_race_maps[SC64_Z5_GLOBAL_ID];
        unsigned live = 0, queued = 0, creates_remaining = 0;
#ifdef SC64_Z5_CLOAK_ENABLED
        unsigned cloak_heroes = 0;
#endif
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
            if ((candidate.type == U_ZERGLING || candidate.type == U_ZP_INFESTED_KERRIGAN) &&
                candidate.shield_remainder >= SC64_Z5_HP_REGEN_TICKS_PER_HP) return 0;
#ifdef SC64_Z5_CLOAK_ENABLED
            if (candidate.raw_type == SC64_Z5_AUTH_KERRIGAN_RAW_TYPE &&
                candidate.raw_owner == SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER &&
                candidate.owner == SIDE) {
                if (++cloak_heroes != 1 || candidate.type != U_ZP_INFESTED_KERRIGAN ||
                    candidate.maxhp != sc64_zp_actor_default_hp(candidate.type) ||
                    candidate.carry < 0 || candidate.carry > SC64_Z5_CLOAK_MAX_ENERGY_Q8 ||
                    (!candidate.alive && (z5->cloak.active || z5->cloak.regen_remainder))) return 0;
            }
#endif
            live += candidate.alive != 0;
        }
#ifdef SC64_Z5_CLOAK_ENABLED
        if (cloak_heroes != 1) return 0;
#endif
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
                if (race_z5_prop_state_valid(&candidate, type) &&
                    candidate.raw_type == src->type &&
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
#ifdef SC64_Z6_RUNTIME
        if (!v[0] && v[2] == SC64_Z6_GLOBAL_ID) {
            /* Z5 and Z6 share this union; Z6 validation follows below. */
        } else
#endif
#ifdef SC64_Z7_RUNTIME
        if (!v[0] && v[2] == SC64_Z7_GLOBAL_ID) {
            /* Z7 shares the payload; its validation follows below. */
        } else
#endif
        {
        const u8 *bytes = (const u8 *)&race_data.z5;
        for (unsigned i = 0; i < sizeof(race_data.z5); ++i) if (bytes[i]) return 0;
        }
    }
#endif
#ifdef SC64_Z6_RUNTIME
    if (!v[0] && v[2] == SC64_Z6_GLOBAL_ID) {
        if (!race_z6_save_candidate_valid(&race_data, &race_data.z6,
                                           (unsigned)width,
                                           (unsigned)height)) return 0;
#ifdef SC64_Z5_RUNTIME
        /* Z5 is the larger member; bytes outside the Z6 schema are canonical
         * zero so old union contents cannot survive a mission change. */
        {
            const u8 *payload = (const u8 *)&race_data.z6;
            for (unsigned i = sizeof(race_data.z6); i < sizeof(race_data.z5); ++i)
                if (payload[i]) return 0;
        }
#endif
    } else {
#ifdef SC64_Z5_RUNTIME
        if (!v[0] && v[2] == SC64_Z5_GLOBAL_ID) {
            /* Z5 owns the full shared payload. */
        } else
#endif
#ifdef SC64_Z7_RUNTIME
        if (!v[0] && v[2] == SC64_Z7_GLOBAL_ID) {
            /* Z7 is validated separately using its smaller schema. */
        } else
#endif
        {
            const u8 *bytes = (const u8 *)&race_data.z6;
            for (unsigned i = 0; i < sizeof(race_data.z6); ++i)
                if (bytes[i]) return 0;
        }
    }
#endif
#ifdef SC64_Z7_RUNTIME
    if (!v[0] && v[2] == SC64_Z7_GLOBAL_ID) {
        if (!race_z7_save_candidate_valid(&race_data, &race_data.z7,
                (unsigned)width, (unsigned)height)) return 0;
        const u8 *payload=(const u8 *)&race_data.z5;
        for (unsigned i=sizeof(race_data.z7); i<sizeof(race_data.z5); ++i)
            if (payload[i]) return 0;
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
#ifdef SC64_P7_PROJECTILES_ENABLED
    if (!race_p7_projectile_save_candidate_valid(&race_data,
            (unsigned)(width*16),(unsigned)(height*16))) return 0;
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (!race_p7_carrier_training_save_valid(&race_data,
            (unsigned)(width*16),(unsigned)(height*16))) return 0;
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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
        if(!race_save_decode_unit(&race_data, (unsigned)i, (unsigned)(race_cols * 16), (unsigned)(race_rows * 16), &decoded)) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
        if(!race_save_decode_unit(&race_data, (unsigned)i,
                                  (unsigned)(race_cols * 16),
                                  (unsigned)(race_rows * 16), &decoded)) return 0;
#else
        if(!sc64_zp_unit_save_decode(&decoded, &race_data.units[i],
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16))) return 0;
#endif
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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
        if(!race_save_decode_building(&race_data, (unsigned)i, &b[i])) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
        if(!race_save_decode_building(&race_data, (unsigned)i, &b[i])) return 0;
#else
        if(!sc64_building_save_decode(&b[i], &race_data.buildings[i])) return 0;
#endif
    }
#else
    for(int i=0;i<MAX_BUILDINGS;i++)for(unsigned j=0;j<RACE_BUILD_WORDS;j++) {
        int n=j==offsetof(Building,flags)/sizeof(int)?(u16)race_data.buildings[i][j]:race_data.buildings[i][j];
        memcpy((u8*)&b[i]+j*sizeof(int),&n,sizeof(n));
    }
#if !defined(SC64_T6_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (sc64_legacy_beacon_hp_sentinel(b[i].type,b[i].raw_type,
                                           b[i].hp,b[i].maxhp))
            b[i].hp = b[i].maxhp = 100000;
#endif
#endif
/* Historical completed saves predate fractional burn and attacker storage.
     * Initialize their previously unused Terran shield word explicitly; zero
     * would otherwise falsely credit player zero for an environmental death. */
    if (!building_damage_marker)
        for (int i=0;i<MAX_BUILDINGS;++i)
            if (b[i].alive && race_building_can_burn(&b[i]))
                b[i].shield_remainder=NEUTRAL*256;
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
#ifdef SC64_P5_RUNTIME
    if (race_p5_active()) {
        race_p5 = race_data.p5;
        if (!race_p5_rebind_loaded_world(&sc64_race_maps[SC64_P5_GLOBAL_ID]))
            return 0;
    } else {
        memset(&race_p5, 0, sizeof(race_p5));
        memset(&race_p5_world, 0, sizeof(race_p5_world));
    }
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) {
        race_p6 = race_data.p6;
        if (!race_p6_rebind_loaded_world(&sc64_race_maps[SC64_P6_GLOBAL_ID]))
            return 0;
    } else {
        memset(&race_p6, 0, sizeof(race_p6));
        memset(&race_p6_world, 0, sizeof(race_p6_world));
        memset(race_p6_doodad_bindings, 0, sizeof(race_p6_doodad_bindings));
        race_p6_doodad_bindings_valid = 0;
    }
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (race_p7_active()) {
        race_p7 = race_data.p7;
        if (!race_p7_rebind_loaded_world(&sc64_race_maps[SC64_P7_GLOBAL_ID]) ||
            !race_p7_battery_live_world_valid()) return 0;
    } else {
        memset(&race_p7, 0, sizeof(race_p7));
        memset(&race_p7_world, 0, sizeof(race_p7_world));
        race_p7_map = 0;
    }
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
#ifdef SC64_Z6_RUNTIME
    if (!race_mode && race_selected_mission == SC64_Z6_GLOBAL_ID) {
        race_z6 = race_data.z6;
        if (!race_z6_rebind_loaded_world(&sc64_race_maps[SC64_Z6_GLOBAL_ID]))
            return 0;
    } else {
        memset(&race_z6, 0, sizeof(race_z6));
        memset(&race_z6_world, 0, sizeof(race_z6_world));
    }
#endif
#ifdef SC64_Z7_RUNTIME
    if (!race_mode && race_selected_mission == SC64_Z7_GLOBAL_ID) {
        race_z7 = race_data.z7;
        if (!race_z7_rebind_loaded_world(&sc64_race_maps[SC64_Z7_GLOBAL_ID]))
            return 0;
    } else {
        memset(&race_z7,0,sizeof(race_z7));
        memset(&race_z7_world,0,sizeof(race_z7_world));
        memset(&race_z7_ops,0,sizeof(race_z7_ops));
        race_z7_map=0;
    }
#endif
#ifdef SC64_NYDUS_ENABLED
    /* After mission rebinds: a mission adapter may install its source. */
    if (!race_nydus_rebind_loaded_world()) return 0;
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
#ifdef SC64_P7_PROJECTILES_ENABLED
    /* Last commit step: later mission rebind failures must not clear the old
     * projectile arena. The candidate packet already passed full preflight. */
    if (!race_p7_projectile_restore(race_data.interceptor_projectiles)) return 0;
#endif
    clear_tracers();death_vfx_reset();mode=MODE_NORMAL;menu_open=help=0;selected_building=-1;
#ifdef SC64_NEAREST_RESOURCE_DEPOT
    /* Loading restores the same serialized topology as the payload. Rebuild
     * routes, but do not treat the restore as a topology change that would
     * discard a worker's committed home-depot trip. */
    path_grid_valid=0;
#ifdef SC64_DIRECT_FIRING_POINTS
    sc64_direct_firing_cache_invalidate();
#endif
#ifdef SC64_IMPORTED_MISSION
    sc64_routes_reset();
#endif
    for (int i=0;i<MAX_UNITS;i++)
        if (u[i].alive && IS_WORKER(u[i].type) &&
            (u[i].order == ORDER_MINE || u[i].order == ORDER_GAS) &&
            u[i].build >= 0 &&
            (!race_resource_depot_valid_for(&u[i], u[i].build) ||
             !race_resource_depot_reachable(&u[i], u[i].build)))
            u[i].build=-1;
#else
    invalidate_path_grid();
#endif
    sc64_visibility_reset();update_exploration();reset_selection_gestures();
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
    screen=won||lost?SCREEN_RESULT:SCREEN_PLAY;toast(7);return 1;
}
#endif
