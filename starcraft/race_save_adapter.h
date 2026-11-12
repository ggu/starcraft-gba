#ifndef SC64_RACE_SAVE_ADAPTER_H
#define SC64_RACE_SAVE_ADAPTER_H
#include "sc64_construction_save.h"
#if defined(SC64_Q8_FLASH_ALL_CUSTOM_HOST) || defined(SC64_Q8_FLASH_ALL_CUSTOM_ARM_PROBE)
#include "sc64_q8_flash_custom_roster.h"
#endif
#if defined(SC64_WORKER_CARGO_KIND) && (!defined(SC64_IMPACT_EVENTS) || defined(SC64_SCARAB_NAMESPACE_STAGE))
#error "worker cargo requires the impact journal and does not support the Scarab draft"
#endif
#ifdef SC64_SAVE_UI_CONTEXT
#include "sc64_save_ui_context.h"
_Static_assert(MAX_BUILDINGS <= 254, "UI save index exceeds one-byte encoding");
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
#if !defined(SC64_RACES) || !defined(SC64_WORKER_CARGO_KIND) || !defined(SC64_IMPACT_EVENTS) || !defined(SC64_SHARED_TRANSPORT) || defined(SC64_Z7_DERIVED_SAVE) || defined(SC64_GUARDIAN_COCOON_STAGE) || defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE) || defined(SC64_SCARAB_NAMESPACE_STAGE)
#error "experimental research journal requires normal shared transport profiles"
#endif
#define SC64_RESEARCH_TRANSPORT_SAVE_VERSION SC64_SAVE_TAG_RESEARCH(SC64_TRANSPORT_SAVE_VERSION)
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
#define SC64_COMPOUND_FLASH_TAG SC64_SAVE_TAG_COMPOUND_FLASH(SC64_TRANSPORT_SAVE_VERSION)
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS
/* Resolve the candidate's immutable mission roles without consulting or
 * changing the currently running mission. The ordinary load preflight still
 * authenticates the rest of the map and actor state before publication. */
static int race_research_campaign_candidate_rule(const RaceSave *candidate,
    unsigned family, Sc64PrCampaignRule *rule)
{
    int8_t roles[8];
    Sc64PrCampaignPolicyView view=sc64_pr_campaign_generated_view();
    if (!candidate || candidate->state[0]!=0 || candidate->state[1]!=R_PROTOSS ||
        candidate->state[2]<0 || candidate->state[2]>=SC64_RACE_MAP_COUNT) return 0;
    int mission=candidate->state[2];
    const Sc64RaceMap *map=&sc64_race_maps[mission];
    for (unsigned source=0;source<8;++source) {
        int owner=map->owner_modes[source];
#ifdef SC64_P3_RUNTIME
        if (mission==SC64_P3_GLOBAL_ID) {
            owner=source==SC64_P3_HUMAN_SLOT ? SIDE :
                  source==SC64_P3_FORCE2_SLOT ? ENEMY :
                  source==SC64_P3_FORCE3_SLOT ? 4 : NEUTRAL;
        }
#endif
#ifdef SC64_P4_RUNTIME
        if (mission==SC64_RACE_MISSION_P4) owner=race_p4_source_owner(source);
#endif
#ifdef SC64_P5_RUNTIME
        if (mission==SC64_RACE_MISSION_P5) owner=race_p5_source_owner(source);
#endif
#ifdef SC64_P6_RUNTIME
        if (mission==SC64_P6_GLOBAL_ID) owner=race_p6_source_owner(source);
#endif
        roles[source]=(int8_t)(owner<0 ? NEUTRAL : owner);
    }
    return sc64_pr_campaign_rule(&view,0,map->mission_id,map->human_slot,roles,
                                 SIDE,family,rule);
}
#endif

#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
#define SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION SC64_SAVE_TAG_RESEARCH_JOBS(SC64_TRANSPORT_SAVE_VERSION)
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
#define SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION SC64_SAVE_TAG_CAMPAIGN_JOBS(SC64_TRANSPORT_SAVE_VERSION)
_Static_assert(SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION != SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION &&
               SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION != SC64_RESEARCH_TRANSPORT_SAVE_VERSION &&
               SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION != SC64_TRANSPORT_SAVE_VERSION,
               "campaign research journal tag collision");
#endif
_Static_assert(SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION != SC64_RESEARCH_TRANSPORT_SAVE_VERSION &&
               SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION != SC64_TRANSPORT_SAVE_VERSION &&
               SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION != SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,
               "active research journal tag collision");
#endif
_Static_assert(SC64_SAVE_TAG_BASE_VALID(SC64_TRANSPORT_SAVE_VERSION),
               "transport tag leaves the historical 0x43 namespace");
_Static_assert(SC64_RESEARCH_TRANSPORT_SAVE_VERSION != SC64_TRANSPORT_SAVE_VERSION &&
               SC64_RESEARCH_TRANSPORT_SAVE_VERSION != SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,
               "research journal tag collision");
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
#define SC64_PR_ROW_WIDTH 25u
#define SC64_PR_USED_BITS 3u
#else
#define SC64_PR_ROW_WIDTH 28u
#define SC64_PR_USED_BITS 1u
#endif
#endif
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
#define race_impact_record sc64_zp_route_save_scratch.impact_record
#elif defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
#define race_data sc64_combined_route_save_scratch.save
#define race_impact_record sc64_combined_route_save_scratch.impact_record
#elif defined(SC64_IMPACT_EVENTS)
/* The record's first member is the RaceSave; pending impacts follow it. */
static RaceImpactRecord race_impact_record;
#define race_data race_impact_record.save
#else
static RaceSave race_data;
#endif
#ifdef SC64_GUARDIAN_COCOON_STAGE
#include "sc64_guardian_wire.h"
#include "sc64_stage_save_auth.h"
#endif
#ifdef SC64_SCARAB_NAMESPACE_STAGE
#include "sc64_scarab_flight_bridge.h"
/* Set only from the authenticated selected tag, before candidate decoding. */
static int race_scarab_stage_wire;
#endif
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
#include "sc64_stage_save_auth.h"
#define race_save_decode_unit race_save_decode_unit_raw_stage
#endif
#if defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) || defined(SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
#include "sc64_shared_egg_wire_host.h"
#endif
/* Decode a candidate unit without touching the live world. The compact
 * late-Terran profile stores named fields; historical profiles retain their
 * exact s16 word rows and conversion behavior. */
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE) || defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT) || defined(SC64_SCARAB_RUNTIME) || defined(SC64_WORKER_CARGO_KIND)
static int
race_save_decode_unit(const RaceSave *candidate, unsigned slot,
                      unsigned world_width, unsigned world_height, Unit *out)
{
    if (!candidate || !out || slot >= MAX_UNITS) return 0;
#if !defined(SC64_LATE_TERRAN_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
    (void)world_width; (void)world_height;
    memset(out,0,sizeof(*out));
    for (unsigned j=0;j<RACE_UNIT_WORDS;++j) {
        int n=j==offsetof(Unit,flags)/sizeof(int) ? (u16)candidate->units[slot][j] : candidate->units[slot][j];
        memcpy((u8 *)out+j*sizeof(int),&n,sizeof(n));
    }
    if (out->alive && sc64_race_raw_unit(out->type)==12 &&
        ((unsigned)candidate->state[27] & 0xffff0000u)==0x42450000u)
        out->carry=(u16)candidate->units[slot][offsetof(Unit,carry)/sizeof(int)];
    return 1;
#elif defined(SC64_ZP_FULL_STORAGE) && !defined(SC64_SCARAB_NAMESPACE_STAGE) && !defined(SC64_GUARDIAN_COCOON_STAGE)
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_decode(out, &candidate->units[slot], world_width, world_height);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_unit_decode(out, &candidate->units[slot], world_width, world_height);
#else
    return sc64_zp_unit_save_decode(out, &candidate->units[slot], world_width, world_height);
#endif
#else
    Unit wire;
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    if (!sc64_p7_typed_unit_decode(&wire, &candidate->units[slot], world_width, world_height)) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
    if (!sc64_derived_unit_decode(&wire, &candidate->units[slot], world_width, world_height)) return 0;
#else
    if (!sc64_zp_unit_save_decode(&wire, &candidate->units[slot], world_width, world_height)) return 0;
#endif
#else
    if (!sc64_unit_save_decode(&wire, &candidate->units[slot], world_width, world_height)) return 0;
#endif
#ifdef SC64_SCARAB_NAMESPACE_STAGE
    if (wire.type==U_SCARAB || wire.raw_type==85) {
        if (!race_scarab_stage_wire || !sc64_scarab_flight_from_wire(&wire,out,1,0) ||
            (((unsigned)out->carry>>8)&3u)>1u) return 0;
        return 1;
    }
#endif
#ifdef SC64_GUARDIAN_COCOON_STAGE
    return sc64_guardian_wire_decode(&wire,out,0);
#else
    *out=wire; return 1;
#endif
#endif
}
#ifdef SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE
#include "sc64_scarab_flight_bridge.h"
/* Token binds this decoding to the already-validated detached candidate. */
static int race_compound_scarab_decode(const RaceSave *candidate,unsigned slot,
                                       unsigned width,unsigned height,Unit *out,
                                       const Sc64StageSaveAuth *auth)
{
    Unit wire,decoded;unsigned x,y;
    if (!candidate || !out || !sc64_stage_save_auth_matches(auth,candidate,
            candidate->version,0) || (auth->features & ~SC64_STAGE_SAVE_SCARAB) ||
        !auth->flight || auth->flight_count!=MAX_UNITS || slot>=MAX_UNITS ||
        !race_save_decode_unit(candidate,slot,width,height,&wire)) return 0;
    const u8 *points=auth->flight+slot*4u;
    x=(unsigned)points[0]|((unsigned)points[1]<<8);
    y=(unsigned)points[2]|((unsigned)points[3]<<8);
    if (wire.type!=U_SCARAB && wire.raw_type!=85) {
        if(x || y)return 0;
        *out=wire;return 1;
    }
    if (!(auth->features & SC64_STAGE_SAVE_SCARAB) ||
        wire.path_timer || wire.path_retry ||
        !sc64_scarab_flight_from_wire(&wire,&decoded,1,0)) return 0;
    if(decoded.alive && (((unsigned)decoded.carry>>8)&3u)>=2u) {
        if(x>32767 || y>32767)return 0;
        decoded.path_timer=(u16)x;decoded.path_retry=(u16)y;
    } else if(x || y)return 0;
    *out=decoded;return 1;
}
#endif
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
#undef race_save_decode_unit
/* The ordinary decoder never admits the appended Egg identity. Existing
 * Z7's Egg ID is legacy and remains subject to its own mission validator. */
static int race_save_decode_unit(const RaceSave *candidate,unsigned slot,
                                 unsigned width,unsigned height,Unit *out)
{
    Unit decoded;
    if (!out || !race_save_decode_unit_raw_stage(candidate,slot,width,height,&decoded)) return 0;
#ifndef SC64_Z7_EGG_PRODUCTION
    if (decoded.type==U_SHARED_EGG_STAGE) return 0;
#endif
    *out=decoded;return 1;
}
/* Dedicated primitive for a detached, already authenticated candidate. It
 * is deliberately not wired into journal selection or publication yet. */
static int race_save_decode_unit_authenticated(const RaceSave *candidate,unsigned slot,
                                                unsigned width,unsigned height,Unit *out,
                                                const Sc64StageSaveAuth *auth)
{
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
    return race_guardian_decode_authenticated(candidate,slot,width,height,out,auth);
#else
    Unit decoded;
    if (!candidate || !out || slot>=MAX_UNITS ||
        !sc64_stage_save_auth_matches(auth,candidate,candidate->version,0)) return 0;
    if (auth->features & SC64_STAGE_SAVE_EGG) {
#if defined(SC64_ZP_FULL_STORAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
        if (!sc64_p7_typed_unit_decode_for(&decoded,&candidate->units[slot],width,height,1)) return 0;
#elif defined(SC64_ZP_FULL_STORAGE) && defined(SC64_Z7_DERIVED_SAVE)
        if (!sc64_derived_unit_decode(&decoded,&candidate->units[slot],width,height)) return 0;
#elif defined(SC64_ZP_FULL_STORAGE)
        if (!sc64_zp_unit_save_decode_for(&decoded,&candidate->units[slot],width,height,1)) return 0;
#elif defined(SC64_LATE_TERRAN_STORAGE)
        if (!sc64_unit_save_decode_for(&decoded,&candidate->units[slot],width,height,1)) return 0;
#else
        if (!race_save_decode_unit_raw_stage(candidate,slot,width,height,&decoded)) return 0;
#endif
    } else if (!race_save_decode_unit_raw_stage(candidate,slot,width,height,&decoded)) return 0;
    if (decoded.type==SC64_SHARED_EGG_TYPE &&
        (!(auth->features & SC64_STAGE_SAVE_EGG) || (decoded.alive && decoded.raw_type!=36))) {
#ifdef SC64_Z7_EGG_PRODUCTION
        if (candidate->state[0] || candidate->state[2]!=SC64_Z7_GLOBAL_ID) return 0;
#else
        return 0;
#endif
    }
    *out=decoded;return 1;
#endif
}
#endif
#endif
#ifdef SC64_GUARDIAN_COCOON_STAGE
/* Explicit detached-candidate decoder. This is not yet wired into journal
 * selection: the caller must supply a token minted after slot authentication. */
static int race_guardian_decode_authenticated(const RaceSave *candidate,unsigned slot,
                                               unsigned width,unsigned height,Unit *out,
                                               const Sc64StageSaveAuth *auth)
{
    Unit wire,decoded;
    if (!candidate || !out || slot>=MAX_UNITS ||
        !sc64_stage_save_auth_matches(auth,candidate,candidate->version,0)) return 0;
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
    int egg_stage=(auth->features & SC64_STAGE_SAVE_EGG)!=0;
#else
    int egg_stage=0;
#endif
    (void)egg_stage;
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
    if (!sc64_p7_typed_unit_decode_for(&wire,&candidate->units[slot],width,height,egg_stage)) return 0;
#else
    if (!sc64_p7_typed_unit_decode(&wire,&candidate->units[slot],width,height)) return 0;
#endif
#elif defined(SC64_Z7_DERIVED_SAVE)
    if (!sc64_derived_unit_decode(&wire,&candidate->units[slot],width,height)) return 0;
#else
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
    if (!sc64_zp_unit_save_decode_for(&wire,&candidate->units[slot],width,height,egg_stage)) return 0;
#else
    if (!sc64_zp_unit_save_decode(&wire,&candidate->units[slot],width,height)) return 0;
#endif
#endif
#else
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
    if (!sc64_unit_save_decode_for(&wire,&candidate->units[slot],width,height,egg_stage)) return 0;
#else
    if (!sc64_unit_save_decode(&wire,&candidate->units[slot],width,height)) return 0;
#endif
#endif
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
#ifndef SC64_Z7_EGG_PRODUCTION
    if (wire.type==SC64_SHARED_EGG_TYPE && !egg_stage) return 0;
#endif
    if (wire.type==SC64_SHARED_EGG_TYPE && wire.alive && wire.raw_type!=36) return 0;
#endif
    if (!sc64_guardian_wire_decode(&wire,&decoded,
                                    (auth->features & SC64_STAGE_SAVE_GUARDIAN)!=0)) return 0;
    *out=decoded;return 1;
}
#endif

#if defined(SC64_SCARAB_NAMESPACE_STAGE) && defined(SC64_ZP_FULL_STORAGE)
/* Codec-specific row writer; semantic conversion is performed by the caller. */
static int race_scarab_zp_encode(RaceSave *candidate, unsigned slot,
                                  const Unit *unit, unsigned width, unsigned height)
{
    if (!candidate || !unit || slot >= MAX_UNITS) return 0;
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_encode(&candidate->units[slot],unit,width,height);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_unit_encode(&candidate->units[slot],unit,width,height);
#else
    return sc64_zp_unit_save_encode(&candidate->units[slot],unit,width,height);
#endif
}
#endif
#ifdef SC64_SCARAB_RUNTIME
#include "sc64_scarab_fields.h"
#include "sc64_scarab_lifecycle.h"
static int race_scarab_save_child_fields(const Unit *unit,Sc64ScarabChildFields *out)
{
#ifdef SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE
    Unit wire;
    /* Packed motion uses the upper carry bits; validate the full logical
     * flight before extracting the ordinary parent/phase fields. */
    if (!sc64_scarab_flight_to_wire(unit,&wire,1,0)) return 0;
    return sc64_scarab_child_fields_unpack((int)((unsigned)unit->carry&1023u),
                                            unit->work_timer,out);
#else
    return sc64_scarab_child_fields_unpack(unit->carry,unit->work_timer,out);
#endif
}
/* Read candidate rows only; never alias or mutate the live world. This is a
 * bounded graph check for the inactive Scarab prototype, not a save migration. */
static int
race_scarab_save_candidate_valid(const RaceSave *candidate, unsigned width, unsigned height SC64_GCTX_DECL)
{
    if (!candidate || !width || !height) return 0;
    for (unsigned i=0; i<MAX_UNITS; ++i) {
        Unit q;
        if (!SC64_GUARD_DECODE(candidate,i,width,height,&q)) return 0;
        if (!q.alive) continue;
        if ((q.type==U_SCARAB) != (q.raw_type==85)) return 0;
        if (q.type==U_SCARAB) {
            Sc64ScarabChildFields child;
            if ((unsigned)q.owner>=5 || q.raw_owner < -1 || q.raw_owner > 255 || q.selected ||
                q.build!=-1 || q.maxhp!=20 || q.hp<=0 || q.hp>20 ||
                q.shield<0 || q.shield>10 ||
                !race_scarab_save_child_fields(&q,&child)) return 0;
            if (child.phase<2 && (child.life || child.no_collision ||
                                  q.order!=ORDER_IDLE || q.target!=-1)) return 0;
            if (child.phase==2 &&
#ifdef SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE
                !(q.order==SC64_SCARAB_FSM_FRESH_RELEASE && !child.life && !child.no_collision) &&
#endif
                !sc64_scarab_flight_valid(
                &(Sc64ScarabFlight){(uint8_t)child.life,(uint8_t)child.no_collision})) return 0;
            if (child.parent_slot!=SC64_SCARAB_FIELD_NO_SLOT) {
                Unit parent;
                if (child.parent_slot>=MAX_UNITS || child.parent_slot==i ||
                    !SC64_GUARD_DECODE(candidate,child.parent_slot,width,height,&parent) ||
                    !parent.alive || sc64_race_raw_unit(parent.type)!=83 ||
                    (parent.raw_type!=-1 && parent.raw_type!=83) ||
                    parent.owner!=q.owner || parent.raw_owner!=q.raw_owner) return 0;
            }
        }
        if (sc64_race_raw_unit(q.type)==83) {
            Sc64ScarabParentFields f;
            Sc64ScarabCounts counts={0,0}; int training=-1;
            if ((q.raw_type!=-1 && q.raw_type!=83) || (unsigned)q.owner>=5 ||
                q.raw_owner < -1 || q.raw_owner > 255 ||
                !sc64_scarab_parent_fields_unpack(q.carry,q.work_timer,&f)) return 0;
            for (unsigned j=0; j<MAX_UNITS; ++j) {
                Unit child; Sc64ScarabChildFields cf;
                if (!SC64_GUARD_DECODE(candidate,j,width,height,&child)) return 0;
                if (!child.alive || child.type!=U_SCARAB) continue;
                if (!race_scarab_save_child_fields(&child,&cf)) return 0;
                if (cf.parent_slot!=i) continue;
                if (cf.phase==0) {
                    unsigned hp;
                    if (training>=0 || !sc64_scarab_training_hp_q8(f.progress,&hp) ||
                        child.hp!=(int)(hp>>8)) return 0;
                    training=(int)j;
                } else if (cf.phase==1) {
                    if (child.hp!=20) return 0;
                    ++counts.inside;
                } else ++counts.outside;
            }
            Sc64ScarabParent p={f.active_slot==SC64_SCARAB_FIELD_NO_SLOT ? -1 : (int)f.active_slot,
                                (uint16_t)f.progress,(uint8_t)f.queued,(uint8_t)f.cooldown};
            /* Above-current-capacity worlds may arise after ownership changes;
             * the lifecycle helper permits them but refuses new admission. */
            if (training!=p.active_slot || !sc64_scarab_parent_valid(&p,counts,1)) return 0;
        }
    }
    return 1;
}
#endif
#ifdef SC64_WORKER_CARGO_KIND
#include "sc64_worker_cargo_save.h"
#endif
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE) || defined(SC64_BATTLECRUISER_RESEARCH_EXPERIMENT) || defined(SC64_BATTLECRUISER_YAMATO_EXPERIMENT) || defined(SC64_SHARED_TRANSPORT)
static int
race_save_decode_building(const RaceSave *candidate, unsigned slot, Building *out)
{
    if (!candidate || !out || slot >= MAX_BUILDINGS) return 0;
#if !defined(SC64_T6_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
    memset(out,0,sizeof(*out));
    for (unsigned j=0;j<RACE_BUILD_WORDS;++j) {
        int n=j==offsetof(Building,flags)/sizeof(int) ? (u16)candidate->buildings[slot][j] : candidate->buildings[slot][j];
        memcpy((u8 *)out+j*sizeof(int),&n,sizeof(n));
    }
    return 1;
#elif defined(SC64_ZP_FULL_STORAGE)
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
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
/* Candidate ownership is derived only from saved custom players or the
 * immutable descriptor named by the saved Z8/Z9 mission. */
static int race_shared_egg_custom_owner(int owner,int raw_owner,void *context)
{
    const RaceSave *candidate=context;
    static const int owners[4]={SIDE,ENEMY,RESCUABLE,4};
    if (!candidate || candidate->state[0]!=1 || candidate->state[4]<1 ||
        candidate->state[4]>3 || raw_owner!=-1 || owner<0 || owner>=5 ||
        candidate->players[owner]!=R_ZERG) return 0;
    for (int i=0;i<=candidate->state[4];++i) if (owners[i]==owner) return 1;
    return 0;
}
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
static unsigned race_z6_save_remaining_creates(const Sc64Z6PersistentState *state);
#endif
static int race_shared_egg_candidate_owner(int owner,int raw_owner,void *context)
{
    const RaceSave *candidate=context;
    if (!candidate) return 0;
    if (candidate->state[0]) return race_shared_egg_custom_owner(owner,raw_owner,context);
    int mission_id=candidate->state[2];
    if (owner<0 || owner>=5 || raw_owner<0 || raw_owner>=8 ||
        candidate->players[owner]!=R_ZERG || mission_id<0 || mission_id>=SC64_RACE_MAP_COUNT) return 0;
    const Sc64RaceMap *map=&sc64_race_maps[mission_id];
    if (map->owner_races[raw_owner]!=R_ZERG) return 0;
#ifdef SC64_SHARED_EGG_Z1_HOST_STAGE
    if (mission_id==SC64_RACE_MISSION_Z1) {
        int mapped=(unsigned)raw_owner==map->human_slot ? SIDE : map->owner_modes[raw_owner];
        return mapped==owner;
    }
#endif
#ifdef SC64_SHARED_EGG_Z2_HOST_STAGE
    if (mission_id==SC64_RACE_MISSION_Z2) {
        int mapped=(unsigned)raw_owner==map->human_slot ? SIDE : map->owner_modes[raw_owner];
        return mapped==owner;
    }
#endif
#ifdef SC64_SHARED_EGG_Z3_HOST_STAGE
    if (mission_id==SC64_RACE_MISSION_Z3) {
        int mapped=(unsigned)raw_owner==map->human_slot ? SIDE : map->owner_modes[raw_owner];
        return mapped==owner;
    }
#endif
#ifdef SC64_SHARED_EGG_Z4_HOST_STAGE
    if (mission_id==SC64_RACE_MISSION_Z4)
        return race_z4_source_owner((unsigned)raw_owner)==owner;
#endif
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
    if (mission_id==SC64_Z6_GLOBAL_ID) {
        int mapped=(unsigned)raw_owner==map->human_slot ? SIDE : map->owner_modes[raw_owner];
        return mapped==owner;
    }
#endif
    if (mission_id==SC64_Z8_GLOBAL_ID) return race_z8_source_owner((unsigned)raw_owner)==owner;
    if (mission_id==SC64_Z9_GLOBAL_ID
#ifdef SC64_Z10_RUNTIME
        || mission_id==SC64_Z10_GLOBAL_ID
#endif
       ) {
        const Sc64ZlMission *mission=race_zl_mission_for(mission_id);
        return mission && mission->live_owner[raw_owner]==owner;
    }
    return 0;
}
static int race_shared_egg_candidate_valid(const RaceSave *candidate,
                                           unsigned width,unsigned height SC64_GCTX_DECL)
{
    unsigned free_slots=0,committed=0;
    static const int owners[4]={SIDE,ENEMY,RESCUABLE,4};
    if (!candidate || candidate->state[0]<0 || candidate->state[0]>1) return 0;
    if (candidate->state[0]) {
        if (candidate->state[1]<0 || candidate->state[1]>2 || candidate->state[4]<1 ||
            candidate->state[4]>3 || candidate->players[SIDE]!=candidate->state[1]) return 0;
        for (int i=0;i<candidate->state[4];++i) {
            int selected=candidate->state[21+i];
            if (selected<0 || selected>2 || candidate->players[owners[i+1]]!=selected) return 0;
        }
    } else if (
#ifdef SC64_SHARED_EGG_Z1_HOST_STAGE
               candidate->state[2]!=SC64_RACE_MISSION_Z1 &&
#endif
#ifdef SC64_SHARED_EGG_Z2_HOST_STAGE
               candidate->state[2]!=SC64_RACE_MISSION_Z2 &&
#endif
#ifdef SC64_SHARED_EGG_Z3_HOST_STAGE
               candidate->state[2]!=SC64_RACE_MISSION_Z3 &&
#endif
#ifdef SC64_SHARED_EGG_Z4_HOST_STAGE
               candidate->state[2]!=SC64_RACE_MISSION_Z4 &&
#endif
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
               candidate->state[2]!=SC64_Z6_GLOBAL_ID &&
#endif
               candidate->state[2]!=SC64_Z8_GLOBAL_ID && candidate->state[2]!=SC64_Z9_GLOBAL_ID
#ifdef SC64_Z10_RUNTIME
               && candidate->state[2]!=SC64_Z10_GLOBAL_ID
#endif
              ) return 0;
    for (unsigned i=0;i<MAX_UNITS;i++) {
        Unit q;
        if (!SC64_GUARD_DECODE(candidate,i,width,height,&q)) return 0;
        if (!q.alive) { ++free_slots; if(q.type==U_Z7_EGG)return 0; continue; }
        if (q.type!=U_Z7_EGG) continue;
        if (!sc64_shared_egg_wire_valid(&q,1,race_shared_egg_candidate_owner,(void *)candidate)) return 0;
        int raw=sc64_race_raw_unit(q.target);
        if (raw<0 || sc64_race_unit_from_raw(raw)!=q.target) return 0;
#ifdef SC64_SHARED_EGG_Z2_HOST_STAGE
        /* Only tagged active Eggs use this admission rule. Old-tag actors
         * remain subject to their original decoder and are not reclassified. */
        if (!candidate->state[0] && candidate->state[2]==SC64_RACE_MISSION_Z2 &&
            !sc64_early_zerg_permission_available(SC64_RACE_MISSION_Z2,
                                                   (unsigned)q.raw_owner,(unsigned)raw)) return 0;
#endif
#ifdef SC64_SHARED_EGG_Z3_HOST_STAGE
        if (!candidate->state[0] && candidate->state[2]==SC64_RACE_MISSION_Z3 &&
            !sc64_early_zerg_permission_available(SC64_RACE_MISSION_Z3,
                                                   (unsigned)q.raw_owner,(unsigned)raw)) return 0;
#endif
#ifdef SC64_SHARED_EGG_Z4_HOST_STAGE
        if (!candidate->state[0] && candidate->state[2]==SC64_RACE_MISSION_Z4 &&
            !sc64_early_zerg_permission_available(SC64_RACE_MISSION_Z4,
                                                   (unsigned)q.raw_owner,(unsigned)raw)) return 0;
#endif
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
        if (!candidate->state[0] && candidate->state[2]==SC64_Z6_GLOBAL_ID &&
            !sc64_mid_zerg_permission_available(SC64_Z6_GLOBAL_ID,
                                                 (unsigned)q.raw_owner,(unsigned)raw)) return 0;
#endif
        if (candidate->state[0]) {
            int admitted=0;
            for (unsigned j=0;j<SC64_SKIRMISH_UNIT_COUNT;j++) {
                const Sc64SkirmishRecord *product=&sc64_skirmish_unit_catalog[j];
                if (product->raw_id==raw && product->kind==SC64_SK_TRAIN && product->race==R_ZERG)
                    admitted=1;
            }
            if (!admitted) return 0;
        }
        /* A paid Egg may outlive its producer and prerequisites. */
        committed+=(unsigned)(sc64_z7_egg_product_count(q.target)-1);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;i++) {
        Building q;
        if (!race_save_decode_building(candidate,i,&q)) return 0;
        if (!q.alive) continue;
        if (q.queue_count<0 || q.queue_count>5 || q.queue_head<0 || q.queue_head>=5) return 0;
        for (int j=0;j<q.queue_count;j++) {
            int type=q.queue[(q.queue_head+j)%5];
            if (!sc64_race_unit_rule(type)) return 0;
            committed+=(unsigned)sc64_race_product_count(type);
        }
    }
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
    if (!candidate->state[0] && candidate->state[2]==SC64_Z6_GLOBAL_ID)
        committed+=race_z6_save_remaining_creates(&candidate->z6);
#endif
    return committed<=free_slots;
}
/* Preserve the narrow HOST test helper while mission admission expands. */
static int race_shared_egg_custom_candidate_valid(const RaceSave *candidate,
                                                   unsigned width,unsigned height)
{
    return candidate && candidate->state[0]==1 &&
           race_shared_egg_candidate_valid(candidate,width,height SC64_GCTX_ARG);
}

#endif
#ifdef SC64_SHARED_TRANSPORT
/* Validate only candidate data, before installing any world state. */
static int race_sk_transport_candidate_valid(const RaceSave *candidate)
{
    static const int owners[SC64_SK_TRANSPORT_OWNERS] = {0, 1, 3, 4};
    if (!candidate) return 0;
    for (int i = 0; i < SC64_SK_TRANSPORT_OWNERS; ++i) {
        const Sc64SkTransportJob *job = &race_skirmish_transport_candidate.owner[i];
        int active = candidate->state[0] && (i == 0 || i <= candidate->state[4]);
        if (!sc64_sk_transport_job_valid(job, MAX_BUILDINGS)) return 0;
        if ((!active || candidate->players[owners[i]] != R_ZERG) &&
            (job->researched || job->remaining || job->producer_plus_one)) return 0;
        if (job->remaining) {
            Building q;
            int slot = (int)job->producer_plus_one - 1;
            if (!race_save_decode_building(candidate, (unsigned)slot, &q) ||
                !q.alive || q.hp <= 0 || q.under_construction || q.owner != owners[i] ||
                (q.type != B_LAIR && q.type != B_HIVE) ||
                (q.raw_type >= 0 && q.raw_type != sc64_race_raw_building(q.type))) return 0;
            for (int k = 0; k < i; ++k)
                if (race_skirmish_transport_candidate.owner[k].producer_plus_one ==
                    job->producer_plus_one) return 0;
        }
    }
    return 1;
}
#endif
#if defined(SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
/* The custom candidate owner identity comes entirely
 * from saved player state; ordinary legacy actors retain their raw owners. */
static int race_combined_egg_owner(int owner,int raw_owner,void *context)
{
    const RaceSave *candidate=context;static const int owners[4]={SIDE,ENEMY,RESCUABLE,4};
    if (!candidate || candidate->state[0]!=1 || candidate->state[4]<1 ||
        candidate->state[4]>3 || raw_owner!=-1 || owner<0 || owner>=5 ||
        candidate->players[owner]!=R_ZERG) return 0;
    for (int i=0;i<=candidate->state[4];i++) if (owners[i]==owner) return 1;
    return 0;
}
static int race_combined_egg_candidate_valid(const RaceSave *candidate,unsigned width,
                                               unsigned height SC64_GCTX_DECL)
{
    unsigned free_slots=0,committed=0;static const int owners[4]={SIDE,ENEMY,RESCUABLE,4};
    if (!candidate || candidate->state[0]!=1 || candidate->state[1]<0 || candidate->state[1]>2 ||
        candidate->state[4]<1 || candidate->state[4]>3 || candidate->players[SIDE]!=candidate->state[1]) return 0;
    for (int i=0;i<candidate->state[4];i++) {
        int selected=candidate->state[21+i];
        if (selected<0 || selected>2 || candidate->players[owners[i+1]]!=selected) return 0;
    }
    for (unsigned i=0;i<MAX_UNITS;i++) {
        Unit q;if (!SC64_GUARD_DECODE(candidate,i,width,height,&q)) return 0;
        if (!q.alive) { ++free_slots; if(q.type==SC64_SHARED_EGG_TYPE)return 0; continue; }
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        /* The current training child already occupies a live slot; `carry`
         * is only the additional paid queue, matching the existing service. */
        if (q.type==U_P7_CARRIER && RACE_PROTOSS_RAW(&q)==SC64_P7_CARRIER_RAW) {
            if (q.carry<0 || q.carry>=SC64_P7_CARRIER_QUEUE_LIMIT) return 0;
            committed+=(unsigned)q.carry;
        }
#endif
        if (q.type!=SC64_SHARED_EGG_TYPE) continue;
        if (!sc64_shared_egg_wire_valid(&q,1,race_combined_egg_owner,(void *)candidate)) return 0;
        int raw=sc64_race_raw_unit(q.target),admitted=0;
        if (raw<0 || sc64_race_unit_from_raw(raw)!=q.target) return 0;
        for (unsigned j=0;j<SC64_SKIRMISH_UNIT_COUNT;j++) {
            const Sc64SkirmishRecord *product=&sc64_skirmish_unit_catalog[j];
            if (product->raw_id==raw && product->kind==SC64_SK_TRAIN && product->race==R_ZERG) admitted=1;
        }
        if (!admitted) return 0;
        committed+=(unsigned)(sc64_race_product_count(q.target)-1);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;i++) {
        Building q;if (!race_save_decode_building(candidate,i,&q)) return 0;
        if (!q.alive) continue;
        if (q.queue_count<0 || q.queue_count>5 || q.queue_head<0 || q.queue_head>=5) return 0;
        for (int j=0;j<q.queue_count;j++) {
            int type=q.queue[(q.queue_head+j)%5];if (!sc64_race_unit_rule(type)) return 0;
            committed+=(unsigned)sc64_race_product_count(type);
        }
    }
    return committed<=free_slots;
}
#endif

#ifdef SC64_Z8_RUNTIME
/* Z8's post-load world binding requires source identities to agree with
 * live owners and runtime types. Check the decoded candidate before any
 * live pool is overwritten, using the same rules as the rebind. */
static int
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
race_z8_save_candidate_world_valid_for(const RaceSave *candidate, int width, int height, int egg_authenticated SC64_GCTX_DECL)
#else
race_z8_save_candidate_world_valid(const RaceSave *candidate, int width, int height SC64_GCTX_DECL)
#endif
{
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
    (void)egg_authenticated;
#endif
    const Sc64RaceMap *map = &sc64_race_maps[SC64_Z8_GLOBAL_ID];
    Sc64NydusSourcePair pairs[SC64_NYDUS_MAX_LINKS];
    unsigned pair_count = 0, nexus_count = 0;
    if (!candidate || width <= 0 || height <= 0 || !race_z8_map_shape_valid(map) ||
        width != map->width || height != map->height ||
        !sc64_nydus_source_pairs(sc64_z8_program_nydus, SC64_Z8_PROGRAM_NYDUS_COUNT,
                                  pairs, &pair_count)) return 0;
    for (unsigned i = 0; i < map->unit_count; ++i)
        nexus_count += map->units[i].source_type == SC64_Z8_RAW_NEXUS &&
                       map->units[i].owner != SC64_Z8_HUMAN_SLOT;
    if (nexus_count != 2u) return 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit unit;
        if (!SC64_GUARD_DECODE(candidate, i, (unsigned)width * 16u,
                                   (unsigned)height * 16u, &unit)) return 0;
        if (!unit.alive) continue;
        if (unit.raw_owner < 0 || unit.raw_owner >= SC64_Z8_OWNER_LIMIT ||
            race_z8_source_owner((unsigned)unit.raw_owner) != unit.owner ||
            unit.raw_type < 0 || (sc64_race_unit_from_raw(unit.raw_type) != unit.type
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
             && !(egg_authenticated && unit.type==U_Z7_EGG && unit.raw_type==36)
#endif
             ))
            return 0;
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building building;
        if (!race_save_decode_building(candidate, i, &building)) return 0;
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
        if (!race_colony_morph_state_valid_for(&building,SIDE,SC64_Z8_HUMAN_SLOT)) return 0;
#endif
        if (!building.alive) continue;
        if (building.raw_owner < 0 || building.raw_owner >= SC64_Z8_OWNER_LIMIT ||
            race_z8_source_owner((unsigned)building.raw_owner) != building.owner ||
            building.raw_type < 0 || sc64_race_building_from_raw(building.raw_type) < 0)
            return 0;
    }
    return 1;
}
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
static int race_z8_save_candidate_world_valid(const RaceSave *candidate,int width,int height SC64_GCTX_DECL)
{ return race_z8_save_candidate_world_valid_for(candidate,width,height,0 SC64_GCTX_ARG); }
#endif
#endif

#ifndef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
#define race_z8_save_candidate_world_valid_for(c,w,h,a) race_z8_save_candidate_world_valid(c,w,h SC64_GCTX_ARG)
#define race_z8_save_unpack_for(c,s,a,e) race_z8_save_unpack(c,s,a)
#endif

#ifdef SC64_Z9_RUNTIME
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
/* Staged preflight binds source ownership and scheduler data to the saved
 * mission descriptor without changing the live selected mission. */
static int race_zl_candidate_map_valid(const Sc64RaceMap *map,const Sc64ZlMission *mission)
{
    if (!mission || !map || mission->global_id>=SC64_RACE_MAP_COUNT ||
        map!=&sc64_race_maps[mission->global_id] || map->width!=mission->map_cols ||
        map->height!=mission->map_rows || map->human_slot!=SC64_ZL_HUMAN_SLOT ||
        map->unit_count!=mission->map_units || !map->owner_modes || !map->units ||
        !race_zl_text_equal(map->mission_id,mission->mission_id)) return 0;
    for (unsigned i=0;i<8;i++) if(map->owner_modes[i]!=mission->owner_modes[i]) return 0;
    return 1;
}
static int race_zl_unpack_candidate_for(const Sc64ZlPersistentState *in,
                                         Sc64ZlSchedulerState *scratch,
                                         const Sc64ZlMission *mission,int egg_stage)
{
    Sc64Z8Setup setup;Sc64ZlAiState ai;
    if (!in || !scratch || !mission) return 0;
    race_zl_setup(mission,&setup);memcpy(&ai,in->ai,sizeof(ai));
    if (!sc64_zl_ai_state_valid(&ai) ||
        (in->crystal_carrier!=SC64_ZL_NO_CARRIER && in->crystal_carrier>=MAX_UNITS)) return 0;
    return sc64_zl_state_unpack_for(0,in,mission->program,&setup,scratch,egg_stage);
}
#endif
/* Z9/Z10 rebinding requires source identities to agree with live owners and
 * runtime types; check the decoded candidate before any live pool changes. */
static int
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
race_zl_save_candidate_world_valid_for(const RaceSave *candidate, int width, int height, int egg_authenticated SC64_GCTX_DECL)
#else
race_zl_save_candidate_world_valid(const RaceSave *candidate, int width, int height SC64_GCTX_DECL)
#endif
{
    const Sc64RaceMap *map;
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
    (void)egg_authenticated;
#endif
    if (!candidate || width <= 0 || height <= 0 || candidate->state[2] < 0 ||
        candidate->state[2] >= SC64_RACE_MAP_COUNT)
        return 0;
    map = &sc64_race_maps[candidate->state[2]];
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
    const Sc64ZlMission *mission=race_zl_mission_for(candidate->state[2]);
    if (!race_zl_candidate_map_valid(map,mission) || width!=map->width || height!=map->height) return 0;
#define SC64_ZL_CANDIDATE_OWNER(slot) (mission->live_owner[(slot)])
#else
    if (!race_zl_map_shape_valid(map) || width != map->width || height != map->height) return 0;
#define SC64_ZL_CANDIDATE_OWNER(slot) race_zl_source_owner(slot)
#endif
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit unit;
        if (!SC64_GUARD_DECODE(candidate, i, (unsigned)width * 16u,
                                   (unsigned)height * 16u, &unit)) return 0;
        if (!unit.alive) continue;
        if (unit.raw_owner < 0 || unit.raw_owner >= SC64_Z8_OWNER_LIMIT ||
            SC64_ZL_CANDIDATE_OWNER((unsigned)unit.raw_owner) != unit.owner ||
            unit.raw_type < 0 || (sc64_race_unit_from_raw(unit.raw_type) != unit.type
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
             && !(egg_authenticated && unit.type==U_Z7_EGG && unit.raw_type==36)
#endif
             ))
            return 0;
    }
    /* Rebind checks this link after live pools are installed. Resolve it
     * against the candidate first so a bad link cannot partially load. */
    if (candidate->zl.crystal_carrier != SC64_ZL_NO_CARRIER) {
        Unit carrier;
        unsigned slot = candidate->zl.crystal_carrier;
        if (slot >= MAX_UNITS ||
            !SC64_GUARD_DECODE(candidate, slot, (unsigned)width * 16u,
                                   (unsigned)height * 16u, &carrier) ||
            !carrier.alive || carrier.owner != SIDE ||
            carrier.raw_type != SC64_ZL_RAW_DRONE) return 0;
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building building;
        if (!race_save_decode_building(candidate, i, &building)) return 0;
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
        if (!race_colony_morph_state_valid_for(&building,SIDE,SC64_ZL_HUMAN_SLOT)) return 0;
#endif
        if (!building.alive) continue;
        if (building.raw_owner < 0 || building.raw_owner >= SC64_Z8_OWNER_LIMIT ||
            SC64_ZL_CANDIDATE_OWNER((unsigned)building.raw_owner) != building.owner ||
            building.raw_type < 0 || sc64_race_building_from_raw(building.raw_type) < 0)
            return 0;
    }
    return 1;
}
#undef SC64_ZL_CANDIDATE_OWNER
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
static int race_zl_save_candidate_world_valid(const RaceSave *candidate,int width,int height SC64_GCTX_DECL)
{ return race_zl_save_candidate_world_valid_for(candidate,width,height,0 SC64_GCTX_ARG); }
#endif

#ifndef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
#define race_zl_save_candidate_world_valid_for(c,w,h,a) race_zl_save_candidate_world_valid(c,w,h SC64_GCTX_ARG)
#define race_zl_save_unpack_for(c,s,a,e) race_zl_save_unpack(c,s,a)
#endif

/* The Z9/Z10 record is smaller than the shared Zerg payload; the rest must
 * stay canonical zero. */
static int race_zl_save_payload_tail_clear(const RaceSave *candidate)
{
    const u8 *payload = (const u8 *)&candidate->z5;
    for (unsigned i = sizeof(Sc64ZlPersistentState); i < sizeof(candidate->z5); ++i)
        if (payload[i]) return 0;
    return 1;
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
                                  unsigned ww, unsigned wh, Unit *out SC64_GCTX_DECL)
{
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return SC64_GUARD_DECODE(candidate, slot, ww, wh, out);
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
static int race_battery_migrate_shield_phase(RaceSave *candidate, int width, int height SC64_GCTX_DECL)
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
        if (!race_battery_save_unit(candidate,i,(unsigned)width*16u,(unsigned)height*16u,&actor SC64_GCTX_ARG)) return 0;
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
static int race_battery_saved_world_valid(const RaceSave *candidate, int width, int height SC64_GCTX_DECL)
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
                                    (unsigned)height*16u,&q SC64_GCTX_ARG)) return 0;
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
static int race_bunker_save_world_valid(const RaceSave *candidate, int width, int height SC64_GCTX_DECL)
{
    u8 used[MAX_BUILDINGS]={0};
    if (!candidate || width<1 || height<1 || width>128 || height>128) return 0;
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit actor;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
        if (!SC64_GUARD_DECODE(candidate,i,(unsigned)width*16u,(unsigned)height*16u,&actor)) return 0;
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
                                        unsigned width, unsigned height SC64_GCTX_DECL)
{
    unsigned live = 0, queued = 0;
    if (!candidate || width != SC64_P5_MAP_COLS || height != SC64_P5_MAP_ROWS ||
        !race_p5_saved_phase_valid(candidate)) return 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit decoded;
        if (!SC64_GUARD_DECODE(candidate, i, width * 16u, height * 16u, &decoded))
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
                                               unsigned ww,unsigned wh SC64_GCTX_DECL)
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
        if (!SC64_GUARD_DECODE(candidate,i,ww,wh,&actors[i])) return 0;
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
#ifdef SC64_P8_RUNTIME
    if (sizeof(candidate->p8)>union_size) union_size=sizeof(candidate->p8);
#endif
#ifdef SC64_P9_RUNTIME
    if (sizeof(candidate->p9)>union_size) union_size=sizeof(candidate->p9);
#endif
#ifdef SC64_P10_RUNTIME
    if (sizeof(candidate->p10)>union_size) union_size=sizeof(candidate->p10);
#endif
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
#ifdef SC64_P8_RUNTIME
        if (candidate->state[2]==SC64_P8_GLOBAL_ID)
            active_size=sizeof(candidate->p8);
#endif
#ifdef SC64_P9_RUNTIME
        if (candidate->state[2]==SC64_P9_GLOBAL_ID)
            active_size=sizeof(candidate->p9);
#endif
#ifdef SC64_P10_RUNTIME
        if (candidate->state[2]==SC64_P10_GLOBAL_ID)
            active_size=sizeof(candidate->p10);
#endif
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
                                           int width, int height, Unit *out SC64_GCTX_DECL)
{
    if (!candidate || !out || slot < 0 || slot >= MAX_UNITS) return 0;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return SC64_GUARD_DECODE(candidate, (unsigned)slot,
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

static int race_skirmish_saved_transport(const RaceSave *candidate, const Unit *ship)
{
    (void)candidate;
    if (!ship || !ship->alive || (unsigned)ship->owner >= 5u ||
        ship->order == ORDER_BOARD || ship->order == ORDER_TRANSPORTED) return 0;
    int raw = ship->raw_type >= 0 ? ship->raw_type : sc64_race_raw_unit(ship->type);
    if (raw < 0 || sc64_race_unit_from_raw(raw) != ship->type) return 0;
#ifdef SC64_SHARED_TRANSPORT
    if (sc64_transport_unconditional_carrier_raw((unsigned)raw)) return 1;
#else
    if (raw == SC64_TRANSPORT_DROPSHIP_RAW) return ship->type == U_DROPSHIP;
#endif
#ifdef SC64_SHARED_TRANSPORT
    if (raw == SC64_TRANSPORT_OVERLORD_RAW && ship->type == U_OVERLORD) {
        if (candidate->state[0]) {
            int owner = sc64_sk_transport_owner_index(ship->owner);
            return owner >= 0 && race_skirmish_transport_candidate.owner[owner].researched;
        }
#ifdef SC64_Z6_TRANSPORT_ENABLED
        return candidate->state[2] == SC64_Z6_GLOBAL_ID &&
            candidate->z6.transport.researched && ship->owner == SIDE && ship->raw_owner == 3;
#else
        return 0;
#endif
    }
#endif
    return 0;
}

static int race_skirmish_transport_save_valid(const RaceSave *candidate,
                                                int width, int height SC64_GCTX_DECL)
{
    u8 used[MAX_UNITS] = {0};
    if (!candidate || width < 1 || height < 1 ||
        width > 128 || height > 128) return 0;
    for (int i=0; i<MAX_UNITS; ++i) {
        Unit actor;
        if (!race_skirmish_transport_decode(candidate, i, width, height, &actor SC64_GCTX_ARG)) return 0;
        if (!actor.alive) continue;
        if (actor.order == ORDER_UNLOAD) {
            if (!race_skirmish_saved_transport(candidate, &actor) || actor.target != -1 ||
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
            !race_skirmish_transport_decode(candidate, actor.target, width, height, &carrier SC64_GCTX_ARG) ||
            !race_skirmish_saved_transport(candidate, &carrier) || actor.owner != carrier.owner ||
            (actor.flags & 2) || sc64_race_unit_flying(actor.type) ||
            raw < 0 || (sc64_race_unit_from_raw(raw) != actor.type &&
                        !(raw == 19 && actor.type == U_RAYNOR)) ||
#ifdef SC64_SHARED_TRANSPORT
            !sc64_transport_shared_space_required_raw((unsigned)raw, &space) ||
#else
            !sc64_transport_space_required_raw((unsigned)raw, &space) ||
#endif
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

#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
static int race_bc_shots_candidate_valid(const RaceSave *candidate,unsigned ww,unsigned wh SC64_GCTX_DECL)
{
    if (!candidate || !ww || !wh) return 0;
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit actor;
        if (!SC64_GUARD_DECODE(candidate,i,ww,wh,&actor)) return 0;
        if (!actor.alive || sc64_race_raw_unit(actor.type)!=12) continue;
        if ((unsigned)actor.work_timer>2047u || ((unsigned)actor.work_timer&7u)>6u ||
            actor.build< -1 || actor.build>31 ||
            ((actor.work_timer&7) && actor.order!=ORDER_ATTACK_TARGET) ||
            (actor.order!=ORDER_IDLE && actor.order!=ORDER_MOVE &&
             actor.order!=ORDER_ATTACK_MOVE && actor.order!=ORDER_ATTACK_TARGET)) return 0;
    }
    for (unsigned i=0;i<SC64_BC_SHOT_LIMIT;++i) {
        const Sc64BcYamatoShot *shot=&candidate->battlecruiser_shots[i];
        static const Sc64BcYamatoShot zero={0};
        if (!shot->state) { if (memcmp(shot,&zero,sizeof(zero))) return 0; continue; }
        if (shot->state>SC64_BC_SHOT_FLIGHT ||
            (shot->target>=MAX_UNITS+MAX_BUILDINGS &&
             !((shot->state==SC64_BC_SHOT_FLIGHT || shot->state==SC64_BC_SHOT_CHARGING) &&
               shot->target==SC64_BC_SHOT_NO_TARGET)) ||
            (shot->phase&0x80u) ||
            ((shot->phase&0x40u) && shot->state!=SC64_BC_SHOT_FLIGHT) ||
            (shot->phase&7u)>4u) return 0;
        if (shot->state==SC64_BC_SHOT_FLIGHT) {
            Sc64BcMotionFields motion;
            if (!sc64_bc_motion_unpack(shot,&motion) || motion.turn_rate<127u ||
                motion.turn_rate>383u || motion.x_q8>=ww*512u || motion.y_q8>=wh*512u ||
                motion.destination_x< -30 || motion.destination_y< -30 ||
                motion.destination_x>=(int)(ww*2u)+30 || motion.destination_y>=(int)(wh*2u)+30) return 0;
        } else if (shot->x_q8<0 || shot->y_q8<0 ||
                   (unsigned)shot->x_q8>=ww*256u || (unsigned)shot->y_q8>=wh*256u ||
                   shot->motion!=0 ||
                   (shot->state==SC64_BC_SHOT_CHARGING && shot->age>SC64_BC_YAMATO_RELEASE_SOURCE_TICK)) return 0;
        if (shot->target==SC64_BC_SHOT_NO_TARGET) {
            /* A launched missile continues visually after target removal. */
        } else if (shot->target<MAX_UNITS) {
            Unit target;
            if (!SC64_GUARD_DECODE(candidate,shot->target,ww,wh,&target) || !target.alive) return 0;
        } else {
            Building target;
            if (!race_save_decode_building(candidate,shot->target-MAX_UNITS,&target) || !target.alive) return 0;
        }
        if (shot->caster!=SC64_BC_SHOT_NO_CASTER) {
            Unit caster;
            if (shot->caster>=MAX_UNITS ||
                !SC64_GUARD_DECODE(candidate,shot->caster,ww,wh,&caster) ||
                !caster.alive || sc64_race_raw_unit(caster.type)!=12) return 0;
            if (shot->state!=SC64_BC_SHOT_FLIGHT &&
                (caster.owner!=(shot->phase&7) ||
                 sc64_battlecruiser_owner_key(candidate->state[0],candidate->state[2],caster.owner,caster.raw_owner)!=(shot->phase>>3))) return 0;
            if (shot->state!=SC64_BC_SHOT_FLIGHT &&
                (caster.order!=ORDER_ATTACK_TARGET || caster.target!=
                 (shot->target==SC64_BC_SHOT_NO_TARGET?-1:(int)shot->target))) return 0;
        } else if (shot->state!=SC64_BC_SHOT_FLIGHT) return 0;
        for (unsigned j=0;j<i;++j)
            if (candidate->battlecruiser_shots[j].state && shot->state!=SC64_BC_SHOT_FLIGHT &&
                candidate->battlecruiser_shots[j].state!=SC64_BC_SHOT_FLIGHT &&
                shot->caster!=SC64_BC_SHOT_NO_CASTER &&
                candidate->battlecruiser_shots[j].caster==shot->caster) return 0;
    }
    return 1;
}
#endif

#ifdef SC64_WORKER_CARGO_KIND
static int race_worker_cargo_previous(GbaSaveSelection *selection);
#endif
#ifdef SC64_SCARAB_NAMESPACE_STAGE
static int race_scarab_stage_previous(GbaSaveSelection *selection);
/* Dead rows are semantically empty. Validate the actor's identity/flight
 * fields first, then canonicalize only the serialization temporary. Live
 * outside children remain unsupported and are never transformed. */
static int race_scarab_save_wire(const Unit *live,Unit *wire)
{
    if(!live || !wire || !sc64_scarab_flight_to_wire(live,wire,1,0)) return 0;
    if((live->type==U_SCARAB || live->raw_type==85) &&
       (((unsigned)live->carry>>8)&3u)>1u) {
        if(live->alive)return 0;
        memset(wire,0,sizeof(*wire));
    }
    return 1;
}
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
/* Decode only into one bounded row. Context is from the detached candidate;
 * an old tag never gains the new job interpretation. `paid_out`, when used,
 * is detached HOST-only scratch and is published only after selection. */
static int race_research_jobs_candidate_scan(const RaceSave *candidate,
    const Sc64ProtossResearchLevels *levels, int jobs_tag)
{
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
    if (candidate && candidate->state[0]==0) {
        unsigned seen=0;
        if (jobs_tag==1 || candidate->state[11]<0 || candidate->state[11]>2 ||
            candidate->state[12]<0 || candidate->state[12]>2 ||
            !sc64_pr_levels_valid(levels,SC64_PR_CAMPAIGN_OWNERS)) return 0;
        if (candidate->state[1]!=R_PROTOSS) {
            if ((candidate->state[1]!=R_TER && candidate->state[1]!=R_ZERG) ||
                candidate->state[2]<0 || candidate->state[2]>=SC64_RACE_MAP_COUNT) return 0;
            const Sc64RaceMap *map=&sc64_race_maps[candidate->state[2]];
            if (!map->mission_id || map->human_slot>=8 ||
                map->mission_id[0]!=(candidate->state[1]==R_TER ? 'T' : 'Z') ||
                map->owner_races[map->human_slot]!=candidate->state[1]) return 0;
            for (unsigned owner=0;owner<SC64_PR_CAMPAIGN_OWNERS;++owner)
                for (unsigned family=0;family<SC64_PR_FAMILY_COUNT;++family)
                    if (levels->level[owner][family]) return 0;
            for (unsigned i=0;i<MAX_BUILDINGS;++i) {
                Building q;
                if (!race_save_decode_building(candidate,i,&q)) return 0;
                if ((q.type==B_FORGE || q.type==B_CYBERNETICS_CORE) &&
                    (q.level || q.queue_progress || q.queue_count)) return 0;
            }
            return 1;
        }
        for (unsigned family=0;family<SC64_PR_FAMILY_COUNT;++family) {
            Sc64PrCampaignRule rule;
            if (!race_research_campaign_candidate_rule(candidate,family,&rule) ||
                levels->level[SIDE][family]<rule.initial ||
                levels->level[SIDE][family]>rule.maximum) return 0;
            for (unsigned owner=1;owner<SC64_PR_CAMPAIGN_OWNERS;++owner)
                if (levels->level[owner][family]) return 0;
        }
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building q; Sc64PrCampaignRule rule; Sc64ProtossResearchQuote source, frozen;
            if (!race_save_decode_building(candidate,i,&q)) return 0;
            if (q.type!=B_FORGE && q.type!=B_CYBERNETICS_CORE) continue;
            if (!q.level && !q.queue_progress && !q.queue_count) continue;
            if (jobs_tag!=2 || q.owner!=SIDE || q.level<1 || q.level>SC64_PR_FAMILY_COUNT ||
                (q.raw_type>=0 && q.raw_type!=sc64_race_raw_building(q.type))) return 0;
            unsigned family=(unsigned)q.level-1;
            if ((seen & (1u<<family)) ||
                !race_research_campaign_candidate_rule(candidate,family,&rule) ||
                !rule.cost_supported || q.queue[1]<1 || q.queue[1]>rule.maximum ||
                !sc64_pr_campaign_quote(&rule,family,(unsigned)q.queue[1]-1,&source) ||
                !sc64_pr_quote_v45(family,(unsigned)q.queue[1]-1,&frozen) ||
                source.mineral!=frozen.mineral || source.gas!=frozen.gas || source.ticks!=frozen.ticks ||
                !sc64_pr_job_valid_custom(&q,SIDE,levels,(unsigned)candidate->state[11],
                   (unsigned)candidate->state[12],candidate->state[13]!=0,candidate->state[14]!=0)) return 0;
            seen |= 1u<<family;
        }
        return 1;
    }
    if (jobs_tag==2) return 0;
#endif
    Sc64PrCandidateScan scan;
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    /* A campaign record never carries research: it must use the pre-research
     * tag and hold no levels. Its Forge/Core rows are not reinterpreted. */
    if (candidate && candidate->state[0]==0) return !jobs_tag;
#endif
    if (!candidate || !levels || candidate->state[0]!=1 ||
        candidate->state[4]<1 || candidate->state[4]>3 ||
        candidate->state[11]<0 || candidate->state[12]<0 ||
        !sc64_pr_candidate_scan_init(&scan,levels,candidate->players,(unsigned)candidate->state[4],
            (unsigned)candidate->state[11],(unsigned)candidate->state[12],
            candidate->state[13]!=0,candidate->state[14]!=0)) return 0;
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building q;
        if (!race_save_decode_building(candidate,i,&q)) return 0;
        if (q.type!=B_FORGE && q.type!=B_CYBERNETICS_CORE) continue;
        if (!jobs_tag) {
            /* Legacy rows may not be reinterpreted or silently discarded. */
            if (q.level || q.queue_progress || q.queue_count) return 0;
            continue;
        }
        if (!sc64_pr_candidate_scan_row(&scan,&q)) return 0;

    }
    return 1;
}
#endif

static int race_save_game(void) {
#ifdef SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
    Sc64StageSaveAuth compound_writer_auth;
    u8 compound_writer_flight[MAX_UNITS*4u]={0};
    const Sc64StageSaveAuth *guard_auth=&compound_writer_auth;
#else
    const Sc64StageSaveAuth *guard_auth=0; /* Until a compound writer is admitted. */
    return 0;
#endif
#endif
#if defined(SC64_CAMPAIGN_CONTROLLER_HOST_STAGE) && !defined(SC64_CONTROLLER_FLASH_P1_HOST_STAGE) && !defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
    return 0; /* no sidecar wire format yet, for any candidate */
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
    if (sc64_pr_stage_fault || (race_mode!=1
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
        && race_mode!=0
#endif
#ifdef SC64_RESEARCH_SRAM_PRODUCT
        && race_mode!=0 /* campaign saves keep the pre-research tag and layout */
#endif
       )) return 0;
#endif
#if defined(SC64_EXPERIMENTAL_RESEARCH_JOBS) && !defined(SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL)
    /* This HOST-only stage has no authenticated active-job wire format.
     * Refuse all writes, including direct calls that bypass save_game(). */
    return 0;
#endif
#if defined(SC64_SCARAB_AI_RESTOCK_HOST_STAGE) && !defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
    return 0; /* HOST payer witnesses have no authenticated journal. */
#endif
#ifdef SC64_SHARED_EGG_HOST_STAGE
    /* No new Egg save format is admitted by this HOST-only experiment.
     * Include dead Egg identities: kill_unit retains actor identity. Raw 36
     * alone is not an Egg discriminator for an ordinary legacy actor. */
    if (!race_z7_active() && race_shared_egg_context()
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
        && !(race_mode || race_z8_active() || race_zl_active()
#ifdef SC64_SHARED_EGG_Z1_HOST_STAGE
             || race_selected_mission==SC64_RACE_MISSION_Z1
#endif
#ifdef SC64_SHARED_EGG_Z2_HOST_STAGE
             || race_selected_mission==SC64_RACE_MISSION_Z2
#endif
#ifdef SC64_SHARED_EGG_Z3_HOST_STAGE
             || race_selected_mission==SC64_RACE_MISSION_Z3
#endif
#ifdef SC64_SHARED_EGG_Z4_HOST_STAGE
             || race_selected_mission==SC64_RACE_MISSION_Z4
#endif
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
             || race_selected_mission==SC64_Z6_GLOBAL_ID
#endif
             )
#endif
       )
        for (int i=0;i<MAX_UNITS;++i)
            if (u[i].type==U_Z7_EGG) return 0;
#endif
#ifdef SC64_WORKER_CARGO_KIND
#if defined(SC64_Q8_FLASH_HOST_STAGE) || defined(SC64_Q8_FLASH_ARM_PROBE)
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
    if (race_mode!=0 || race_selected_mission!=SC64_Z10_GLOBAL_ID || race_human!=R_ZERG || !race_q8_flash_begin()) return 0;
#elif defined(SC64_CONTROLLER_FLASH_P1_HOST_STAGE)
    if (race_mode!=0 || race_selected_mission!=SC64_RACE_MISSION_P1 || race_human!=R_PROTOSS || !race_q8_flash_begin()) return 0;
#else
    if (race_mode!=1 ||
#if defined(SC64_Q8_FLASH_ALL_CUSTOM_HOST) || defined(SC64_Q8_FLASH_ALL_CUSTOM_ARM_PROBE)
        !sc64_q8_flash_custom_roster_valid(race_human,race_bots,race_player,race_bot_races) ||
#else
        race_human!=R_PROTOSS ||
#endif
        !race_q8_flash_begin()) return 0;
#endif
    GbaSaveSelection cargo_previous={0};
    int cargo_have_previous=0;
#else
    GbaSaveSelection cargo_previous;
    int cargo_have_previous=race_worker_cargo_previous(&cargo_previous);
#endif
#endif
#ifdef SC64_P8_RUNTIME
    if (!race_p8_live_products_valid()) return 0;
#endif
#ifdef SC64_SCARAB_WEAPON_PROBE
    if (race_scarab_probe_bullets_pending()) return 0;
#endif
#ifdef SC64_SCARAB_NAMESPACE_STAGE
    GbaSaveSelection previous;
    int have_previous=race_scarab_stage_previous(&previous);
    if (race_mode!=1 || race_skirmish_map<0 || race_skirmish_map>SC64_SKIRMISH_IMPORTED_COUNT ||
        !sc64_race_storage_geometry_valid(race_cols,race_rows)) return 0;
    if (race_skirmish_map==0 ? (race_cols!=64 || race_rows!=64) :
        (race_cols!=sc64_skirmish_maps[race_skirmish_map-1].width ||
         race_rows!=sc64_skirmish_maps[race_skirmish_map-1].height)) return 0;
    for (int i=0;i<MAX_UNITS;i++) if (u[i].alive) {
        Sc64ScarabParentFields f;
        if (u[i].type==U_SCARAB || u[i].raw_type==85) {
            Unit wire;
            if (!sc64_scarab_flight_to_wire(&u[i],&wire,1,0) ||
                (((unsigned)u[i].carry>>8)&3u)>1u) return 0;
        }
        if (sc64_race_raw_unit(u[i].type)==83 &&
            ((u[i].raw_type!=-1 && u[i].raw_type!=83) ||
             !sc64_scarab_parent_fields_unpack(u[i].carry,u[i].work_timer,&f))) return 0;
    }
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    if (sc64_bc_pause_offset>4) return 0;
#ifndef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
    race_bc_yamato_sync_references();
#endif
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && sc64_race_raw_unit(u[i].type)==12) {
        if ((unsigned)u[i].work_timer>2047u || ((unsigned)u[i].work_timer&7u)>6u ||
            u[i].build< -1 || u[i].build>31 ||
            ((u[i].work_timer&7) && u[i].order!=ORDER_ATTACK_TARGET) ||
            (u[i].order!=ORDER_IDLE && u[i].order!=ORDER_MOVE &&
             u[i].order!=ORDER_ATTACK_MOVE && u[i].order!=ORDER_ATTACK_TARGET)) return 0;
    }
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    for (int i=0;i<MAX_BUILDINGS;++i) {
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (race_late_active()) break; /* legacy campaign uses its mission codec */
#endif
        if (!race_bc_facility_fields_valid(&b[i],(unsigned)WORLD_W,(unsigned)WORLD_H)) return 0;
    }
#endif
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
    if (!race_bc_research_live_valid()) return 0;
#endif
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && sc64_race_raw_unit(u[i].type) == 12) {
        int key = sc64_battlecruiser_owner_key(race_mode, race_selected_mission, u[i].owner, u[i].raw_owner);
        int reactor;
        if (key < 0 || (u[i].raw_type != -1 && u[i].raw_type != 12)) return 0;
        reactor = sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,
                                              (unsigned)key, SC64_BC_TECH_REACTOR);
        if (u[i].carry < 0 || u[i].carry > sc64_battlecruiser_energy_cap(reactor)) return 0;
    }
#endif
#ifdef SC64_WORKER_CARGO_KIND
    for (unsigned i=0;i<MAX_UNITS;++i)
        if (!sc64_worker_cargo_state_valid(&u[i])) return 0;
#endif
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
    race_data.version=
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
        race_mode ? SC64_GUARDIAN_EGG_SAVE_VERSION : SC64_GUARDIAN_SAVE_VERSION;
#elif defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
        race_mode ? SC64_SHARED_EGG_SAVE_VERSION : SAVE_VERSION;
#elif defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE)
        (race_mode || race_z8_active() || race_zl_active()
#ifdef SC64_SHARED_EGG_Z1_HOST_STAGE
         || race_selected_mission==SC64_RACE_MISSION_Z1
#endif
#ifdef SC64_SHARED_EGG_Z2_HOST_STAGE
         || race_selected_mission==SC64_RACE_MISSION_Z2
#endif
#ifdef SC64_SHARED_EGG_Z3_HOST_STAGE
         || race_selected_mission==SC64_RACE_MISSION_Z3
#endif
#ifdef SC64_SHARED_EGG_Z4_HOST_STAGE
         || race_selected_mission==SC64_RACE_MISSION_Z4
#endif
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
         || race_selected_mission==SC64_Z6_GLOBAL_ID
#endif
         ) ?
        SC64_SHARED_EGG_SAVE_VERSION : SAVE_VERSION;
#elif defined(SC64_GUARDIAN_COCOON_STAGE)
        SC64_GUARDIAN_SAVE_VERSION;
#elif defined(SC64_SCARAB_NAMESPACE_STAGE)
        SC64_SCARAB_SAVE_VERSION;
#else
        SAVE_VERSION;
#endif
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
    const Sc64StageTagPair compound_writer_pair={SC64_COMPOUND_FLASH_TAG,SAVE_VERSION,SC64_STAGE_SAVE_SCARAB};
    if (!sc64_stage_save_authenticate(&compound_writer_auth,&race_data,SC64_COMPOUND_FLASH_TAG,
                                      race_data.version,&compound_writer_pair,1)) return 0;
    compound_writer_auth.flight=compound_writer_flight;
    compound_writer_auth.flight_count=MAX_UNITS;
#endif
#ifdef SC64_GUARDIAN_COCOON_STAGE
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
    const Sc64StageTagPair writer_pair=race_mode ?
        (Sc64StageTagPair){SC64_GUARDIAN_EGG_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_EGG_SAVE_VERSION,
                           SC64_STAGE_SAVE_GUARDIAN|SC64_STAGE_SAVE_EGG} :
        (Sc64StageTagPair){SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_SAVE_VERSION,SC64_STAGE_SAVE_GUARDIAN};
#else
    const Sc64StageTagPair writer_pair={SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,
        SC64_GUARDIAN_SAVE_VERSION,SC64_STAGE_SAVE_GUARDIAN};
#endif
    Sc64StageSaveAuth writer_auth;
    if (!sc64_stage_save_authenticate(&writer_auth,&race_data,writer_pair.outer,
                                      race_data.version,&writer_pair,1)) return 0;
    const Sc64StageSaveAuth *guard_auth=&writer_auth;
#endif
#ifdef SC64_SHARED_EGG_CUSTOM_CANDIDATE
    const Sc64StageTagPair writer_pair = race_mode ?
        (Sc64StageTagPair){SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_SAVE_VERSION,SC64_STAGE_SAVE_EGG} :
        (Sc64StageTagPair){SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION,0};
    Sc64StageSaveAuth writer_auth;
    if (!sc64_stage_save_authenticate(&writer_auth,&race_data,writer_pair.outer,
                                      race_data.version,&writer_pair,1)) return 0;
    const Sc64StageSaveAuth *guard_auth=&writer_auth;
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    memcpy(race_data.battlecruiser_shots,sc64_bc_shots,sizeof(sc64_bc_shots));
#endif
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_GUARDIAN_COCOON_STAGE
    for (int i=0;i<MAX_UNITS;i++) {
        Unit wire;
        if (!sc64_guardian_wire_encode(&u[i],&wire,1)) return 0;
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
        if (race_mode && wire.type==SC64_SHARED_EGG_TYPE && !wire.alive) memset(&wire,0,sizeof(wire));
#endif
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
        if (!sc64_p7_typed_unit_encode_for(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16),race_mode)) return 0;
#else
        if (!sc64_p7_typed_unit_encode(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
#endif
#elif defined(SC64_Z7_DERIVED_SAVE)
        if (!sc64_derived_unit_encode(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
#else
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
        if (!sc64_zp_unit_save_encode_for(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16),race_mode)) return 0;
#else
        if (!sc64_zp_unit_save_encode(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
#endif
#endif
    }
#elif defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    for (int i=0;i<MAX_UNITS;i++) {
        Unit wire=u[i];
        if (race_mode && wire.type==SC64_SHARED_EGG_TYPE && !wire.alive) memset(&wire,0,sizeof(wire));
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
        if (!sc64_p7_typed_unit_encode_for(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16),race_mode)) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
        if (!sc64_derived_unit_encode(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
#else
        if (!sc64_zp_unit_save_encode_for(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16),race_mode)) return 0;
#endif
    }
#elif defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE)
    for (int i=0;i<MAX_UNITS;i++) {
        Unit wire=u[i];
        /* Dead staged Eggs are empty slots, not identities to resurrect. */
        if (race_data.version==SC64_SHARED_EGG_SAVE_VERSION && wire.type==U_Z7_EGG && wire.alive==0)
            memset(&wire,0,sizeof(wire));
        if (!sc64_zp_unit_save_encode(&race_data.units[i],&wire,
                                     (unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
    }
#elif defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
    for (int i=0;i<MAX_UNITS;i++) {
        Unit wire;
        if (!u[i].alive && (u[i].type==U_SCARAB || u[i].raw_type==85)) {
            /* Dead Scarabs are inert retired slots, not staged identities. */
            memset(&wire,0,sizeof wire);
        } else if (!sc64_scarab_flight_to_wire(&u[i],&wire,1,0)) return 0;
        if (u[i].alive && u[i].type==U_SCARAB && u[i].raw_type==85) {
            unsigned phase=((unsigned)u[i].carry>>8)&3u;
            unsigned x=u[i].path_timer,y=u[i].path_retry;
            if (phase>=2) {
                if(x>32767 || y>32767)return 0;
                compound_writer_flight[i*4u]=(u8)x;compound_writer_flight[i*4u+1]=(u8)(x>>8);
                compound_writer_flight[i*4u+2]=(u8)y;compound_writer_flight[i*4u+3]=(u8)(y>>8);
            } else if(x || y)return 0;
            wire.path_timer=0;wire.path_retry=0;
        }
        if (
            !sc64_zp_unit_save_encode(&race_data.units[i],&wire,
                                      (unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
    }
#elif defined(SC64_SCARAB_NAMESPACE_STAGE)
    for (int i=0;i<MAX_UNITS;i++) {
        Unit wire;
        if (!race_scarab_save_wire(&u[i],&wire) ||
            !race_scarab_zp_encode(&race_data,(unsigned)i,&wire,
                                   (unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
    }
#else
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
#endif
#else
    for(int i=0;i<MAX_UNITS;i++)
#ifdef SC64_GUARDIAN_COCOON_STAGE
    {
        Unit wire;
        if (!sc64_guardian_wire_encode(&u[i],&wire,1)) return 0;
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
        if (race_mode && wire.type==SC64_SHARED_EGG_TYPE && !wire.alive) memset(&wire,0,sizeof(wire));
        if (!sc64_unit_save_encode_for(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16),race_mode)) return 0;
#else
        if (!sc64_unit_save_encode(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
#endif
    }
#elif defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    {
        Unit wire=u[i];
        if (race_mode && wire.type==SC64_SHARED_EGG_TYPE && !wire.alive) memset(&wire,0,sizeof(wire));
        if (!sc64_unit_save_encode_for(&race_data.units[i],&wire,(unsigned)(race_cols*16),(unsigned)(race_rows*16),race_mode)) return 0;
    }
#elif defined(SC64_SCARAB_NAMESPACE_STAGE)
    {
        Unit wire;
        if (!race_scarab_save_wire(&u[i],&wire) ||
            !sc64_unit_save_encode(&race_data.units[i],&wire,
                                   (unsigned)(race_cols*16),(unsigned)(race_rows*16))) return 0;
    }
#else
        if(!sc64_unit_save_encode(&race_data.units[i], &u[i],
                                  (unsigned)(race_cols * 16),
                                  (unsigned)(race_rows * 16))) return 0;
#endif
#endif
#else
    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {
        int n;memcpy(&n,(u8*)&u[i]+j*sizeof(int),sizeof(n));
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
        if (u[i].alive && sc64_race_raw_unit(u[i].type)==12 &&
            j==offsetof(Unit,carry)/sizeof(int) && n>=32768) n-=65536;
#endif
        race_data.units[i][j]=(s16)n;
    }
#endif
#ifdef SC64_SCARAB_NAMESPACE_STAGE
    race_scarab_stage_wire=1;
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
#ifdef SC64_SAVE_UI_CONTEXT
    {
        int selected = selected_building;
        if (selected < 0 || selected >= MAX_BUILDINGS || !b[selected].alive) selected = -1;
#ifdef SC_FIELD_FEEDBACK
        unsigned last = menu_last;
#else
        unsigned last = 0;
#endif
        race_data.state[SC64_SAVE_UI_STATE_INDEX] = (int)sc64_save_ui_encode(
            selected, MAX_BUILDINGS, menu_cursor, last);
    }
#endif
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    race_data.state[27] = 0x42450000 | sc64_battlecruiser_tech_bits;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    race_data.state[28] = 0x42440000 | sc64_bc_pause_offset;
    memcpy(&race_data.state[29],&sc64_bc_spell_rng,sizeof(sc64_bc_spell_rng));
#endif
#endif
    race_data.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX]=SC64_BUILDING_DAMAGE_SAVE_MARKER;
#ifdef SC64_SHARED_BATTERY
    race_data.state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]=SC64_SHIELD_PHASE_SAVE_MARKER;
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (race_mode) {
        if (!race_p7_battery_live_world_valid()) return 0;
        race_data.state[SC64_PROTOSS_CLOCK_STATE_INDEX] =
            race_protoss_clock_word(race_protoss_generic_clock);
    }
#endif
#ifdef SC64_CONSTRUCTION_HP
    race_data.state[SC64_CONSTRUCTION_SAVE_MARKER_INDEX] = SC64_CONSTRUCTION_SAVE_MARKER;
#endif
#ifdef SC64_SCARAB_RUNTIME
    if (!race_scarab_save_candidate_valid(&race_data,(unsigned)race_cols*16u,
                                           (unsigned)race_rows*16u SC64_GCTX_ARG)) return 0;
#endif
    memcpy(race_data.players,race_player,sizeof(race_player));
#ifdef SC64_SHARED_TRANSPORT
    if (race_mode) race_skirmish_transport_candidate = race_skirmish_transport_tech;
    else memset(&race_skirmish_transport_candidate, 0, sizeof(race_skirmish_transport_candidate));
    if (!race_sk_transport_candidate_valid(&race_data)) return 0;
#endif
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
#ifdef SC64_P8_RUNTIME
               !race_p8_active() &&
#endif
#ifdef SC64_P9_RUNTIME
               !race_p9_active() &&
#endif
#ifdef SC64_P10_RUNTIME
               !race_p10_active() &&
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
#ifdef SC64_P8_RUNTIME
               !race_p8_active() &&
#endif
#ifdef SC64_P9_RUNTIME
               !race_p9_active() &&
#endif
#ifdef SC64_P10_RUNTIME
               !race_p10_active() &&
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
#ifdef SC64_P8_RUNTIME
               !race_p8_active() &&
#endif
#ifdef SC64_P9_RUNTIME
               !race_p9_active() &&
#endif
#ifdef SC64_P10_RUNTIME
               !race_p10_active() &&
#endif
               1) {
        memset(&race_data.p7,0,sizeof(race_data.p7));
    }
#endif
#ifdef SC64_P8_RUNTIME
    if (race_p8_active()) {
        if (!race_p8_runtime_state_valid() ||
            !race_p7_battery_live_world_valid()) return 0;
        /* Clear the whole shared union so its tail stays zero. */
        memset(&race_data.p6,0,sizeof(race_data.p6));
        race_data.p8=race_p8;
    }
#endif
#ifdef SC64_P9_RUNTIME
    if (race_p9_active()) {
        if (!race_p9_runtime_state_valid() ||
            !race_p7_battery_live_world_valid()) return 0;
        memset(&race_data.p6,0,sizeof(race_data.p6));
        race_data.p9=race_p9;
    }
#endif
#ifdef SC64_P10_RUNTIME
    if (race_p10_active()) {
        if (!race_p10_runtime_state_valid() ||
            !race_p7_battery_live_world_valid()) return 0;
        memset(&race_data.p6,0,sizeof(race_data.p6));
        race_data.p10=race_p10;
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
#ifdef SC64_Z8_RUNTIME
    if (race_z8_active() && !race_z8_save_pack(&race_data.z8)) return 0;
#endif
#ifdef SC64_Z9_RUNTIME
    if (race_zl_active() && !race_zl_save_pack(&race_data.zl)) return 0;
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) {
        if (!race_terran_late_save_pack(&race_data.terran_late, &race_late,
                                        race_selected_mission,
                                        race_cols, race_rows) ||
            !race_terran_late_save_candidate_valid(&race_data,
                                                    race_cols, race_rows SC64_GCTX_ARG))
            return 0;
    } else
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active()) {
        if (!race_t6_save_pack(&race_data.t6, &race_t6) ||
            !race_t6_save_candidate_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG))
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
            !race_t5_save_candidate_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
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
                                          (unsigned)race_rows SC64_GCTX_ARG)) return 0;
    }
#if defined(SC64_P6_RUNTIME) || defined(SC64_P7_BATTERY_ENABLED)
    else if (
#ifdef SC64_P6_RUNTIME
             !race_p6_active() &&
#endif
#ifdef SC64_P7_BATTERY_ENABLED
             !race_p7_active() &&
#endif
#ifdef SC64_P8_RUNTIME
             !race_p8_active() &&
#endif
#ifdef SC64_P9_RUNTIME
             !race_p9_active() &&
#endif
#ifdef SC64_P10_RUNTIME
             !race_p10_active() &&
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
                                          (unsigned)race_rows SC64_GCTX_ARG)) return 0;
    }
#if defined(SC64_P5_RUNTIME) || defined(SC64_P7_BATTERY_ENABLED)
    else if (
#ifdef SC64_P5_RUNTIME
             !race_p5_active() &&
#endif
#ifdef SC64_P7_BATTERY_ENABLED
             !race_p7_active() &&
#endif
#ifdef SC64_P8_RUNTIME
             !race_p8_active() &&
#endif
#ifdef SC64_P9_RUNTIME
             !race_p9_active() &&
#endif
#ifdef SC64_P10_RUNTIME
             !race_p10_active() &&
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
#ifdef SC64_P8_RUNTIME
             !race_p8_active() &&
#endif
#ifdef SC64_P9_RUNTIME
             !race_p9_active() &&
#endif
#ifdef SC64_P10_RUNTIME
             !race_p10_active() &&
#endif
             1) {
        const u8 *bytes=(const u8 *)&race_data.p7;
        for (unsigned i=0;i<sizeof(race_data.p7);++i) if (bytes[i]) return 0;
    }
#ifdef SC64_P8_RUNTIME
    if (race_p8_active() && !sc64_p8_persistent_state_valid(&race_data.p8))
        return 0;
#endif
#ifdef SC64_P9_RUNTIME
    if (race_p9_active() && !sc64_p9_persistent_state_valid(&race_data.p9))
        return 0;
#endif
#ifdef SC64_P10_RUNTIME
    if (race_p10_active() && !sc64_p10_persistent_state_valid(&race_data.p10))
        return 0;
#endif
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
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
        if (!race_z6_save_candidate_valid_for(&race_data, &race_data.z6,
                                          (unsigned)race_cols,(unsigned)race_rows,
                                          race_data.version SC64_GCTX_ARG)) return 0;
#else
        if (!race_z6_save_candidate_valid(&race_data, &race_data.z6,
                                          (unsigned)race_cols,
                                          (unsigned)race_rows SC64_GCTX_ARG)) return 0;
#endif
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
#ifdef SC64_Z8_RUNTIME
        if (race_z8_active()) {
            /* Z8 fills the shared payload; it is checked below. */
        } else
#endif
#ifdef SC64_Z9_RUNTIME
        if (race_zl_active()) {
            /* Z9/Z10 share the payload; checked below. */
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
                (unsigned)race_cols, (unsigned)race_rows SC64_GCTX_ARG)) return 0;
        const u8 *payload=(const u8 *)&race_data.z5;
        for (unsigned i=sizeof(race_data.z7); i<sizeof(race_data.z5); ++i)
            if (payload[i]) return 0;
    }
#endif
#ifdef SC64_SHARED_BUNKER
    if (!race_bunker_save_world_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_TRANSPORT_RUNTIME
#ifdef SC64_SHARED_TRANSPORT
    if (!race_skirmish_transport_save_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#else
    if (race_data.state[0] && !race_skirmish_transport_save_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#endif
#endif
#ifdef SC64_SHARED_BATTERY
    if (!race_battery_saved_world_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_Z8_RUNTIME
    if (race_z8_active() &&
        !race_z8_save_unpack_for(&race_data.z8,
                             &sc64_zp_route_save_scratch.z8_candidate.scheduler, 0,
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
                             race_data.version==SC64_SHARED_EGG_SAVE_VERSION
#else
                             0
#endif
                             ))
        return 0;
#endif
#ifdef SC64_Z9_RUNTIME
    if (race_zl_active() &&
        (!race_zl_save_payload_tail_clear(&race_data) ||
         !race_zl_save_unpack_for(&race_data.zl,
                              &sc64_zp_route_save_scratch.zl_candidate.scheduler, 0,
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
                              race_data.version==SC64_SHARED_EGG_SAVE_VERSION
#else
                              0
#endif
                              )))
        return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (!race_transport_save_candidate_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (!race_data.state[0] && !race_p5_transport_save_candidate_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (!race_data.state[0] && !race_z6_transport_save_candidate_valid(&race_data, &race_data.z6.transport, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
    if (!race_z5_consume_save_candidate_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_NYDUS_ENABLED
    if (!race_nydus_save_candidate_valid(&race_data, race_cols, race_rows SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (!race_p7_union_tail_valid(&race_data)) return 0;
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    if (!race_p7_projectile_encode(race_data.interceptor_projectiles) ||
        !race_p7_projectile_save_candidate_valid(&race_data,
            (unsigned)(race_cols*16),(unsigned)(race_rows*16) SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (!race_p7_carrier_training_save_valid(&race_data,
            (unsigned)(race_cols*16),(unsigned)(race_rows*16) SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    if (!race_bc_shots_candidate_valid(&race_data,(unsigned)WORLD_W,(unsigned)WORLD_H SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_SCARAB_NAMESPACE_STAGE
    if (!race_scarab_save_candidate_valid(&race_data,(unsigned)race_cols*16u,(unsigned)race_rows*16u SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
    if (race_data.version==SC64_SHARED_EGG_SAVE_VERSION && !race_shared_egg_candidate_valid(&race_data,
                                    (unsigned)race_cols*16u,(unsigned)race_rows*16u SC64_GCTX_ARG)) return 0;
#endif
#if defined(SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    if (race_mode && !race_combined_egg_candidate_valid(&race_data,
            (unsigned)race_cols*16u,(unsigned)race_rows*16u SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    if (race_mode==1) {
#endif
    if (!race_research_jobs_candidate_scan(&race_data,&sc64_experimental_research_levels,
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
                                            race_mode ? 1 : 2
#else
                                            1
#endif
                                           )) return 0;
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building decoded;
        if (!race_save_decode_building(&race_data,i,&decoded)) return 0;
        if (sc64_pr_stage_job_marked(&b[i])) {
            Sc64PrPaidReceipt paid;
            if (!sc64_pr_stage_receipt(i,&paid) || !sc64_pr_stage_owner_active(paid.payer) ||
                !sc64_pr_job_receipt_valid(&decoded,paid.payer,&paid)) return 0;
        }
    }
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    }
#endif
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    if (race_mode==1)
#endif
    if ((race_mode!=0 && race_mode!=1) ||
        !sc64_pr_padding_write((u8 *)race_data.buildings, MAX_BUILDINGS, SC64_PR_ROW_WIDTH,
                               SC64_PR_USED_BITS, race_mode ? 5u : 8u,
                               &sc64_experimental_research_levels)) return 0;
#endif
#ifdef SC64_IMPACT_EVENTS
    /* This runs after validators that share the scratch tail. */
#ifdef SC64_WORKER_CARGO_KIND
    if (!race_impact_save_write(cargo_have_previous ? &cargo_previous : 0)) return 0;
#elif defined(SC64_SCARAB_NAMESPACE_STAGE)
    if (!race_impact_save_write(have_previous ? &previous : 0)) return 0;
#else
    if (!race_impact_save_write()) return 0;
#endif
#elif defined(SC64_SHARED_TRANSPORT)
    {
        u8 trailer[16];
        sc64_sk_transport_encode(trailer, &race_skirmish_transport_candidate);
        if (!save_write_parts(&race_data, sizeof(race_data), trailer, sizeof(trailer),
                              SC64_TRANSPORT_SAVE_VERSION)) return 0;
    }
#elif defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT)
    save_write_migrating_feature(&race_data,sizeof(race_data),SAVE_VERSION,
                         SC64_LEGACY_SAVE_VERSION, offsetof(RaceSave,state)+27u*sizeof(int),
                         0xffff0000u, 0x42450000u);
#else
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);
#endif
#ifdef SC64_RANGE_WORK
    /* Match the load boundary after publication: these navigation caches are
     * not serialized. Preserve committed worker depot assignments, which are
     * serialized and do not represent a topology change. */
    path_grid_valid = 0;
#ifdef SC64_DIRECT_FIRING_POINTS
    sc64_direct_firing_cache_invalidate();
#endif
    sc64_routes_reset();
#endif
    toast(7);return 1;
}
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
_Static_assert(sizeof(race_data.buildings[0]) == SC64_PR_ROW_WIDTH, "unsupported research building row");
_Static_assert(SC64_PR_PAD_TUPLE_VALID(MAX_BUILDINGS,SC64_PR_ROW_WIDTH,SC64_PR_USED_BITS),
               "unsupported research journal building layout");
#endif
/* Candidate-only load outputs live outside the route/save scratch union. */
typedef struct {
    int building_damage_marker;
#ifdef SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE
    Sc64StageSaveAuth compound_auth;
    u8 compound_flight[MAX_UNITS*4u];
#endif
#if defined(SC64_Q8_FLASH_HOST_STAGE) || defined(SC64_Q8_FLASH_ARM_PROBE)
    u8 q8_debt[MAX_UNITS+MAX_BUILDINGS];
#if defined(SC64_CONTROLLER_FLASH_P1_HOST_STAGE) || defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
    u8 controller_units[MAX_UNITS], controller_buildings[MAX_BUILDINGS];
#endif
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
    u8 scarab_payers[MAX_UNITS];
#endif
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
    Sc64ProtossResearchLevels research_levels;
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
    u8 research_jobs_tag;
#endif
#endif
#ifdef SC64_GUARDIAN_COCOON_STAGE
    Sc64StageSaveAuth guardian_auth;
#endif
#if defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    Sc64StageSaveAuth egg_auth;
#endif
#ifdef SC64_WORKER_CARGO_KIND
    int legacy_worker_cargo;
#endif
    union {
        int unused;
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
        Sc64T4RuntimeState t4;
#endif
#ifdef SC64_T5_RUNTIME
        Sc64T5RuntimeState t5;
#endif
#ifdef SC64_T6_RUNTIME
        Sc64T6MissionState t6;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
        Sc64TerranLateMissionState terran_late;
#endif
    } expanded;
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    int expanded_t4_valid;
#endif
#ifdef SC64_T5_RUNTIME
    int expanded_t5_valid;
#endif
#ifdef SC64_T6_RUNTIME
    int expanded_t6_valid;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    int expanded_terran_late_valid;
#endif
#ifdef SC64_IMPACT_EVENTS
    Sc64ImpactEvent impact_candidate[RACE_IMPACT_SAVE_CAPACITY ? RACE_IMPACT_SAVE_CAPACITY : 1];
    int impact_candidate_count;
#endif
} RaceLoadPlan;
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
typedef struct {RaceLoadPlan *plan;const u8 *flight;} Sc64CfPrepareContext;
#endif

/* May modify candidate/route scratch, never the live world or SRAM. */
static int race_load_preflight_candidate(RaceLoadPlan *plan) {
#ifdef SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE
    const Sc64StageSaveAuth *guard_auth=&plan->compound_auth;
#elif defined(SC64_GUARDIAN_COCOON_STAGE)
    const Sc64StageSaveAuth *guard_auth=&plan->guardian_auth;
#elif defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    const Sc64StageSaveAuth *guard_auth=&plan->egg_auth;
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (!race_p7_union_tail_valid(&race_data)) return 0;
#endif
    const int *v=race_data.state;
#ifdef SC64_SHARED_TRANSPORT
    if (!race_sk_transport_candidate_valid(&race_data)) return 0;
#endif
#ifdef SC64_SAVE_UI_CONTEXT
    if (!sc64_save_ui_valid((unsigned)v[SC64_SAVE_UI_STATE_INDEX], MAX_BUILDINGS)) return 0;
#endif
#if defined(SC64_P7_OBSERVER_ENABLED) || defined(SC64_P7_BATTERY_ENABLED)
    race_protoss_bind_candidate(v[0], v[2]);
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    /* Only the generic binding keeps its service clock in the state words. */
    if (!race_protoss_clock_word_valid(v[SC64_PROTOSS_CLOCK_STATE_INDEX]) ||
        (v[SC64_PROTOSS_CLOCK_STATE_INDEX] && !v[0])) return 0;
#endif
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
#ifdef SC64_P8_RUNTIME
       &&v[2]!=SC64_P8_GLOBAL_ID
#endif
#ifdef SC64_P9_RUNTIME
       &&v[2]!=SC64_P9_GLOBAL_ID
#endif
#ifdef SC64_P10_RUNTIME
       &&v[2]!=SC64_P10_GLOBAL_ID
#endif
#ifdef SC64_Z6_RUNTIME
       &&v[2]!=SC64_Z6_GLOBAL_ID
#endif
#ifdef SC64_Z7_RUNTIME
       &&v[2]!=SC64_Z7_GLOBAL_ID
#endif
#ifdef SC64_Z8_RUNTIME
       &&v[2]!=SC64_Z8_GLOBAL_ID
#endif
#ifdef SC64_Z9_RUNTIME
       &&v[2]!=SC64_Z9_GLOBAL_ID
#endif
#ifdef SC64_Z10_RUNTIME
       &&v[2]!=SC64_Z10_GLOBAL_ID
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
#ifdef SC64_P8_RUNTIME
        &&v[2]!=SC64_P8_GLOBAL_ID
#endif
#ifdef SC64_P9_RUNTIME
        &&v[2]!=SC64_P9_GLOBAL_ID
#endif
#ifdef SC64_P10_RUNTIME
        &&v[2]!=SC64_P10_GLOBAL_ID
#endif
#ifdef SC64_Z6_RUNTIME
        &&v[2]!=SC64_Z6_GLOBAL_ID
#endif
#ifdef SC64_Z7_RUNTIME
        &&v[2]!=SC64_Z7_GLOBAL_ID
#endif
#ifdef SC64_Z8_RUNTIME
        &&v[2]!=SC64_Z8_GLOBAL_ID
#endif
#ifdef SC64_Z9_RUNTIME
        &&v[2]!=SC64_Z9_GLOBAL_ID
#endif
#ifdef SC64_Z10_RUNTIME
        &&v[2]!=SC64_Z10_GLOBAL_ID
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
#ifdef SC64_P8_RUNTIME
       | (1u << SC64_P8_GLOBAL_ID)
#endif
#ifdef SC64_P9_RUNTIME
       | (1u << SC64_P9_GLOBAL_ID)
#endif
#ifdef SC64_P10_RUNTIME
       | (1u << SC64_P10_GLOBAL_ID)
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
    if (!race_battery_migrate_shield_phase(&race_data,width,height SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_WORKER_CARGO_KIND
    if (!race_save_worker_cargo_prepare(&race_data,(unsigned)width*16u,
                                        (unsigned)height*16u,plan->legacy_worker_cargo SC64_GCTX_ARG)) return 0;
#if !defined(SC64_GUARDIAN_COCOON_STAGE) && !defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE) && !defined(SC64_SHARED_EGG_LATE_SERVICE)
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
    if (!(plan->egg_auth.features & SC64_STAGE_SAVE_EGG))
#endif
        race_data.version = SAVE_VERSION;
#endif
#endif
#if (defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE) || defined(SC64_SHARED_EGG_LATE_SERVICE)) && (defined(SC64_Z6_RUNTIME) || defined(SC64_Z7_RUNTIME))
    /* The selected token remains bound to the unmodified inner version. Only
     * after cargo migration succeeds may this exact old authenticated pair
     * be interpreted as the predecessor of the current campaign generation. */
    u32 campaign_expected_version=SAVE_VERSION;
    if (plan->legacy_worker_cargo && plan->egg_auth.features==0 &&
        plan->egg_auth.outer==SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION &&
        sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,
                                    SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,0) &&
        race_data.version==SC64_WORKER_CARGO_LEGACY_SAVE_VERSION)
        campaign_expected_version=SC64_WORKER_CARGO_LEGACY_SAVE_VERSION;
#ifdef SC64_SHARED_EGG_Z6_HOST_STAGE
    if (!v[0] && v[2]==SC64_Z6_GLOBAL_ID &&
        sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,
                                     SC64_SHARED_EGG_SAVE_VERSION,SC64_STAGE_SAVE_EGG))
        campaign_expected_version=SC64_SHARED_EGG_SAVE_VERSION;
#endif
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
        if(!SC64_GUARD_DECODE(&race_data, (unsigned)i,
                                  (unsigned)(width * 16),
                                  (unsigned)(height * 16),
                                  &candidate_unit)) return 0;
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
        /* U_Z7_EGG is a legacy type only in the Z7 campaign. Every custom or
         * later-mission instance needs its selected candidate's Egg token,
         * regardless of whether the staged live producer is compiled. */
        if ((v[0] || v[2]!=SC64_Z7_GLOBAL_ID) && RACE_U(i,type)==U_Z7_EGG) {
#if defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
            if (!sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,
                                     race_data.version,SC64_STAGE_SAVE_EGG)) return 0;
#elif defined(SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE)
            if (!sc64_stage_save_auth_matches(&plan->guardian_auth,&race_data,
                                     race_data.version,SC64_STAGE_SAVE_EGG)) return 0;
#else
            return 0;
#endif
        }
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
#ifdef SC64_P8_RUNTIME
        if (v[0]==0 && v[2]==SC64_P8_GLOBAL_ID && SC64_PENDING_BUILD_ORDER(order) &&
            !race_p8_source_product_available(RACE_U(i,owner),RACE_U(i,raw_owner),
                sc64_race_raw_building(target))) return 0;
#endif
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
            /* The candidate's mode is bound for this preflight. */
            if (!race_protoss_services_active() ||
                RACE_PROTOSS_RAW(&candidate_unit)==73 || RACE_PROTOSS_RAW(&candidate_unit)==85 ||
                RACE_PROTOSS_SLOT(&candidate_unit)<0 || RACE_PROTOSS_SLOT(&candidate_unit)>=8 ||
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
                RACE_PROTOSS_RAW(&target_battery)!=172 || target_battery.under_construction ||
                target_battery.owner!=candidate_unit.owner ||
                RACE_PROTOSS_SLOT(&target_battery)!=RACE_PROTOSS_SLOT(&candidate_unit)) return 0;
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
                        !SC64_GUARD_DECODE(&race_data, (unsigned)target,
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
        else if (candidate_building.type==B_P7_SHIELD_BATTERY &&
                 race_protoss_service_binding()==SC64_PROTOSS_BIND_GENERIC &&
                 !race_protoss_generic_battery_candidate_valid(&candidate_building))
            return 0;
#ifdef SC64_PROTOSS_LATER_BINDINGS
        else if (candidate_building.type==B_P7_SHIELD_BATTERY &&
                 race_protoss_service_binding()==SC64_PROTOSS_BIND_LATER &&
                 !race_protoss_later_battery_candidate_valid(race_data.state[2],
                                                             &candidate_building))
            return 0;
#endif
#endif
        for(int j=0;j<RACE_B(i,queue_count);j++) {
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
            int type=candidate_building.queue[(RACE_B(i,queue_head)+j)%5];
#else
            int type=race_data.buildings[i][offsetof(Building,queue)/sizeof(int)+(RACE_B(i,queue_head)+j)%5];
#endif
            if(type<0||type>=SC_RU_COUNT)return 0;
#ifdef SC64_P8_RUNTIME
            if (v[0]==0 && v[2]==SC64_P8_GLOBAL_ID &&
                !race_p8_source_product_available(candidate_building.owner,candidate_building.raw_owner,
                    sc64_race_raw_unit(type))) return 0;
#endif
        }
    }
#undef RACE_U
#undef RACE_B
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
    if ((plan->egg_auth.features & SC64_STAGE_SAVE_EGG) &&
        (!sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG) ||
         !race_shared_egg_candidate_valid(&race_data,(unsigned)width*16u,(unsigned)height*16u SC64_GCTX_ARG))) return 0;
#endif
#ifdef SC64_SHARED_EGG_CUSTOM_CANDIDATE
    if ((plan->egg_auth.features & SC64_STAGE_SAVE_EGG) &&
        (!sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG) ||
         !race_combined_egg_candidate_valid(&race_data,(unsigned)width*16u,(unsigned)height*16u SC64_GCTX_ARG))) return 0;
#endif
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
    if ((plan->guardian_auth.features & SC64_STAGE_SAVE_EGG) &&
        !race_combined_egg_candidate_valid(&race_data,(unsigned)width*16u,(unsigned)height*16u SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_SHARED_BUNKER
    if (!race_bunker_save_world_valid(&race_data, width, height SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_TRANSPORT_RUNTIME
#ifdef SC64_SHARED_TRANSPORT
    if (!race_skirmish_transport_save_valid(&race_data, width, height SC64_GCTX_ARG)) return 0;
#else
    if (race_data.state[0] && !race_skirmish_transport_save_valid(&race_data, width, height SC64_GCTX_ARG)) return 0;
#endif
#endif
#ifdef SC64_SHARED_BATTERY
    if (!race_battery_saved_world_valid(&race_data, width, height SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (!race_t5_candidate_queues_available(&race_data)) return 0;
    if (!race_transport_save_candidate_valid(&race_data, width, height SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (!race_data.state[0] && !race_p5_transport_save_candidate_valid(&race_data, width, height SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (!race_data.state[0] && !race_z6_transport_save_candidate_valid(&race_data, &race_data.z6.transport, width, height SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
    if (!race_z5_consume_save_candidate_valid(&race_data, width, height SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_NYDUS_ENABLED
    if (!race_nydus_save_candidate_valid(&race_data, width, height SC64_GCTX_ARG)) return 0;
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
#ifdef SC64_SHARED_EGG_Z4_HOST_STAGE
        unsigned pending=z4->faults & 0x7f000000u;
        if (pending && race_data.version!=SC64_SHARED_EGG_SAVE_VERSION) return 0;
#endif
        if (!race_z4_runtime_state_valid(z4) || v[1] != R_ZERG ||
            race_data.players[SIDE] != R_ZERG || race_data.players[ENEMY] != R_TER ||
            race_data.players[4] != R_TER || race_data.campaign.mission != 15 ||
            race_data.campaign.elapsed_frames != (u32)v[7] ||
            race_data.campaign.objective_stage !=
                !!(z4->model.fired & SC64_Z4_EVENT_TRIGGER_HATCH)) return 0;
        if (outcome == SC64_Z4_RUNNING
#ifdef SC64_SHARED_EGG_Z4_HOST_STAGE
            || pending
#endif
           ) {
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
            if (!SC64_GUARD_DECODE(&race_data, i, (unsigned)(width * 16),
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
                                          (unsigned)height SC64_GCTX_ARG)) return 0;
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
#ifdef SC64_P8_RUNTIME
        if (!v[0] && v[2] == SC64_P8_GLOBAL_ID) {
            /* P8 shares the Protoss mission payload union. */
        } else
#endif
#ifdef SC64_P9_RUNTIME
        if (!v[0] && v[2] == SC64_P9_GLOBAL_ID) {
            /* P9 shares the Protoss mission payload union. */
        } else
#endif
#ifdef SC64_P10_RUNTIME
        if (!v[0] && v[2] == SC64_P10_GLOBAL_ID) {
            /* P10 shares the Protoss mission payload union. */
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
                                           (unsigned)height SC64_GCTX_ARG)) return 0;
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
#ifdef SC64_P8_RUNTIME
        if (!v[0] && v[2] == SC64_P8_GLOBAL_ID) {
            /* P8 owns the payload in the shared union. */
        } else
#endif
#ifdef SC64_P9_RUNTIME
        if (!v[0] && v[2] == SC64_P9_GLOBAL_ID) {
            /* P9 owns the payload in the shared union. */
        } else
#endif
#ifdef SC64_P10_RUNTIME
        if (!v[0] && v[2] == SC64_P10_GLOBAL_ID) {
            /* P10 owns the payload in the shared union. */
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
            if (!SC64_GUARD_DECODE(&race_data,i,(unsigned)(width*16),
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
#ifdef SC64_P8_RUNTIME
        if (!v[0] && v[2]==SC64_P8_GLOBAL_ID) {
            /* P8 owns the shared payload. */
        } else
#endif
#ifdef SC64_P9_RUNTIME
        if (!v[0] && v[2]==SC64_P9_GLOBAL_ID) {
            /* P9 owns the shared payload. */
        } else
#endif
#ifdef SC64_P10_RUNTIME
        if (!v[0] && v[2]==SC64_P10_GLOBAL_ID) {
            /* P10 owns the shared payload. */
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
#ifdef SC64_P8_RUNTIME
    if (!v[0] && v[2]==SC64_P8_GLOBAL_ID) {
        const Sc64P8PersistentState *p8=&race_data.p8;
        unsigned outcome=p8->runtime[SC64_P8_RUNNER_HUMAN].outcome;
        unsigned phase=race_data.campaign.phase;
        if (!sc64_p8_persistent_state_valid(p8) ||
            !race_p8_map_shape_valid(&sc64_race_maps[SC64_P8_GLOBAL_ID]) ||
            race_data.campaign.mission!=SC64_P8_GLOBAL_ID ||
            race_data.campaign.elapsed_frames!=(u32)v[7] ||
            p8->runtime[SC64_P8_RUNNER_DEFENDERS].outcome!=SC64_P8_OUTCOME_RUNNING ||
            p8->runtime[SC64_P8_RUNNER_CONCLAVE].outcome!=SC64_P8_OUTCOME_RUNNING)
            return 0;
        /* The shared census requires a source identity on every live actor. */
        for (unsigned i=0;i<MAX_UNITS;++i) {
            Unit candidate;
            if (!SC64_GUARD_DECODE(&race_data,i,(unsigned)(width*16),
                                       (unsigned)(height*16),&candidate)) return 0;
            if (candidate.alive && (candidate.raw_owner<0 || candidate.raw_owner>=8 ||
                sc64_race_unit_from_raw(candidate.raw_type)<0)) return 0;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
            if (!sc64_p7_arbiter_cache_unit_valid(&candidate)) return 0;
#endif
        }
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building candidate;
            if (!race_save_decode_building(&race_data,i,&candidate)) return 0;
            if (candidate.alive && (candidate.raw_owner<0 || candidate.raw_owner>=8 ||
                sc64_race_building_from_raw(candidate.raw_type)<0)) return 0;
        }
        if (outcome==SC64_P8_OUTCOME_RUNNING) {
            if (v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
                (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else if (outcome==SC64_P8_OUTCOME_WON) {
            if (!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
                !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)||
                !(v[3]&(1<<SC64_P8_GLOBAL_ID))) return 0;
        } else if (outcome==SC64_P8_OUTCOME_LOST) {
            if (v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
                (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else return 0;
    }
#endif
#ifdef SC64_P9_RUNTIME
    if (!v[0] && v[2]==SC64_P9_GLOBAL_ID) {
        const Sc64P9PersistentState *p9=&race_data.p9;
        unsigned outcome=p9->runner[SC64_P9_RUNNER_HUMAN].outcome;
        unsigned phase=race_data.campaign.phase;
        if (!sc64_p9_source_state_valid(p9) ||
            !race_p9_map_shape_valid(&sc64_race_maps[SC64_P9_GLOBAL_ID]) ||
            race_data.campaign.mission!=SC64_P9_GLOBAL_ID ||
            race_data.campaign.elapsed_frames!=(u32)v[7]) return 0;
        for (unsigned r=1;r<SC64_P9_RUNNER_COUNT;++r)
            if (p9->runner[r].outcome!=SC64_P9_OUTCOME_RUNNING) return 0;
        /* The shared census requires a source identity on every live actor. */
        for (unsigned i=0;i<MAX_UNITS;++i) {
            Unit candidate;
            if (!SC64_GUARD_DECODE(&race_data,i,(unsigned)(width*16),
                                       (unsigned)(height*16),&candidate)) return 0;
            if (candidate.alive && (candidate.raw_owner<0 || candidate.raw_owner>=8 ||
                sc64_race_unit_from_raw(candidate.raw_type)<0)) return 0;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
            if (!sc64_p7_arbiter_cache_unit_valid(&candidate)) return 0;
#endif
        }
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building candidate;
            if (!race_save_decode_building(&race_data,i,&candidate)) return 0;
            if (candidate.alive && (candidate.raw_owner<0 || candidate.raw_owner>=8 ||
                sc64_race_building_from_raw(candidate.raw_type)<0)) return 0;
        }
        if (outcome==SC64_P9_OUTCOME_RUNNING) {
            if (v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
                (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else if (outcome==SC64_P9_OUTCOME_WON) {
            if (!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
                !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)||
                !(v[3]&(1<<SC64_P9_GLOBAL_ID))) return 0;
        } else if (outcome==SC64_P9_OUTCOME_LOST) {
            if (v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
                (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else return 0;
    }
#endif
#ifdef SC64_P10_RUNTIME
    if (!v[0] && v[2]==SC64_P10_GLOBAL_ID) {
        const Sc64P10PersistentState *p10=&race_data.p10;
        unsigned outcome=p10->runner[SC64_P10_RUNNER_HUMAN].outcome;
        unsigned phase=race_data.campaign.phase;
        if (!sc64_p10_source_state_valid(p10) ||
            !race_p10_map_shape_valid(&sc64_race_maps[SC64_P10_GLOBAL_ID]) ||
            race_data.campaign.mission!=SC64_P10_GLOBAL_ID ||
            race_data.campaign.elapsed_frames!=(u32)v[7]) return 0;
        for (unsigned r=1;r<SC64_P10_RUNNER_COUNT;++r)
            if (p10->runner[r].outcome!=SC64_P10_OUTCOME_RUNNING) return 0;
        /* The shared census requires a source identity on every live actor. */
        for (unsigned i=0;i<MAX_UNITS;++i) {
            Unit candidate;
            if (!SC64_GUARD_DECODE(&race_data,i,(unsigned)(width*16),
                                       (unsigned)(height*16),&candidate)) return 0;
            if (candidate.alive && (candidate.raw_owner<0 || candidate.raw_owner>=8 ||
                sc64_race_unit_from_raw(candidate.raw_type)<0)) return 0;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
            if (!sc64_p7_arbiter_cache_unit_valid(&candidate)) return 0;
#endif
        }
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building candidate;
            if (!race_save_decode_building(&race_data,i,&candidate)) return 0;
            if (candidate.alive && (candidate.raw_owner<0 || candidate.raw_owner>=8 ||
                sc64_race_building_from_raw(candidate.raw_type)<0)) return 0;
        }
        if (outcome==SC64_P10_OUTCOME_RUNNING) {
            if (v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
                (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else if (outcome==SC64_P10_OUTCOME_WON) {
            if (!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
                !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)||
                !(v[3]&(1<<SC64_P10_GLOBAL_ID))) return 0;
        } else if (outcome==SC64_P10_OUTCOME_LOST) {
            if (v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
                (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
        } else return 0;
    }
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
            if (!SC64_GUARD_DECODE(&race_data, i, width * 16u, height * 16u,
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
#ifdef SC64_Z8_RUNTIME
        if (!v[0] && v[2] == SC64_Z8_GLOBAL_ID) {
            /* Z8 fills the payload; its validation follows below. */
        } else
#endif
#ifdef SC64_Z9_RUNTIME
        if (!v[0] && race_zl_mission_for(v[2])) {
            /* Z9/Z10 share the payload; validation follows below. */
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
#if defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE) || defined(SC64_SHARED_EGG_LATE_SERVICE)
        if (!race_z6_save_candidate_valid_for(&race_data, &race_data.z6,
                                           (unsigned)width,
                                           (unsigned)height, campaign_expected_version SC64_GCTX_ARG)) return 0;
#else
        if (!race_z6_save_candidate_valid(&race_data, &race_data.z6,
                                           (unsigned)width,
                                           (unsigned)height SC64_GCTX_ARG)) return 0;
#endif
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
#ifdef SC64_Z8_RUNTIME
        if (!v[0] && v[2] == SC64_Z8_GLOBAL_ID) {
            /* Z8 is validated separately below. */
        } else
#endif
#ifdef SC64_Z9_RUNTIME
        if (!v[0] && race_zl_mission_for(v[2])) {
            /* Z9/Z10 are validated separately below. */
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
#if defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE) || defined(SC64_SHARED_EGG_LATE_SERVICE)
        if (!race_z7_save_candidate_valid_for(&race_data, &race_data.z7,
                (unsigned)width, (unsigned)height, campaign_expected_version SC64_GCTX_ARG)) return 0;
#else
        if (!race_z7_save_candidate_valid(&race_data, &race_data.z7,
                (unsigned)width, (unsigned)height SC64_GCTX_ARG)) return 0;
#endif
        const u8 *payload=(const u8 *)&race_data.z5;
        for (unsigned i=sizeof(race_data.z7); i<sizeof(race_data.z5); ++i)
            if (payload[i]) return 0;
    }
#endif
#ifdef SC64_Z8_RUNTIME
    if (!v[0] && v[2] == SC64_Z8_GLOBAL_ID &&
        (!race_z8_save_candidate_world_valid_for(&race_data, width, height,
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
         sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG)
#else
         0
#endif
          SC64_GCTX_ARG) ||
         !race_z8_save_unpack_for(&race_data.z8,
                              &sc64_zp_route_save_scratch.z8_candidate.scheduler, 0,
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
                              sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG)
#else
                              0
#endif
                              )))
        return 0;
#endif
#ifdef SC64_Z9_RUNTIME
    if (!v[0] && race_zl_mission_for(v[2])) {
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
        const Sc64ZlMission *mission=race_zl_mission_for(v[2]);
        if (!race_zl_save_candidate_world_valid_for(&race_data,width,height,
            sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG) SC64_GCTX_ARG) ||
            !race_zl_save_payload_tail_clear(&race_data) ||
            !race_zl_unpack_candidate_for(&race_data.zl,
                &sc64_zp_route_save_scratch.zl_candidate.scheduler,mission,
                sc64_stage_save_auth_matches(&plan->egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG))) return 0;
#else
        /* Legacy validation temporarily binds its selected mission. */
        int saved_mission = race_selected_mission, ok;
        race_selected_mission = v[2];
        ok = race_zl_save_candidate_world_valid_for(&race_data, width, height, 0 SC64_GCTX_ARG) &&
             race_zl_save_payload_tail_clear(&race_data) &&
             race_zl_save_unpack(&race_data.zl,
                                 &sc64_zp_route_save_scratch.zl_candidate.scheduler, 0);
        race_selected_mission = saved_mission;
        if (!ok) return 0;
#endif
    }
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (!v[0] && v[2] >= SC64_TERRAN_LATE_GLOBAL_T7 &&
        v[2] <= SC64_TERRAN_LATE_GLOBAL_T10) {
        if (!race_terran_late_save_candidate_valid(&race_data, width, height SC64_GCTX_ARG) ||
            !race_terran_late_save_expand(&plan->expanded.terran_late,
                                          &race_data.terran_late,
                                          v[2], width, height)) return 0;
        plan->expanded_terran_late_valid = 1;
    } else
#endif
#ifdef SC64_T6_RUNTIME
    if (!v[0] && v[2] == SC64_T6_GLOBAL_ID) {
        if (!race_t6_save_candidate_valid(&race_data, width, height SC64_GCTX_ARG) ||
            !race_t6_save_expand(&plan->expanded.t6, &race_data.t6)) return 0;
        plan->expanded_t6_valid = 1;
    } else
#endif
#ifdef SC64_T5_RUNTIME
    if (!v[0] && v[2] == SC64_T5_GLOBAL_ID) {
        if (!race_t5_save_candidate_valid(&race_data, width, height SC64_GCTX_ARG) ||
            !race_t5_save_expand(&plan->expanded.t5, &race_data.t5)) return 0;
        plan->expanded_t5_valid = 1;
    } else
#endif
    if (!v[0] && v[2] == SC64_T4_GLOBAL_ID) {
        if (!race_t4_save_candidate_valid(&race_data, width, height)) return 0;
#ifdef SC64_T3_ENABLED
        if (!race_t4_save_expand(&plan->expanded.t4, &race_data.t4)) return 0;
        plan->expanded_t4_valid = 1;
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
    if (!v[0] && v[2] == SC64_T4_GLOBAL_ID && !plan->expanded_t4_valid) return 0;
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    if (!race_p7_projectile_save_candidate_valid(&race_data,
            (unsigned)(width*16),(unsigned)(height*16) SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (!race_p7_carrier_training_save_valid(&race_data,
            (unsigned)(width*16),(unsigned)(height*16) SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    {
        unsigned marker = (unsigned)v[27];
        unsigned bits = marker & 65535u;
        if (marker && (marker & 0xffff0000u) != 0x42450000u) return 0;
        for (unsigned i = 0; i < MAX_UNITS; ++i) {
            Unit candidate;
            int key, reactor;
            if (!SC64_GUARD_DECODE(&race_data, i, (unsigned)(width * 16),
                                       (unsigned)(height * 16), &candidate)) return 0;
            if (!candidate.alive || sc64_race_raw_unit(candidate.type) != 12) continue;
            /* Legacy raw12 energy had no defined encoding; reject atomically. */
            if (!marker || (candidate.raw_type != -1 && candidate.raw_type != 12)) return 0;
            key = sc64_battlecruiser_owner_key(v[0], v[2], candidate.owner, candidate.raw_owner);
            if (key < 0) return 0;
            reactor = sc64_battlecruiser_tech_has(bits, (unsigned)key, SC64_BC_TECH_REACTOR);
            if (candidate.carry < 0 || candidate.carry > sc64_battlecruiser_energy_cap(reactor)) return 0;
        }
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building q;
#ifdef SC64_TERRAN_LATE_RUNTIME
            if (!v[0] && v[2]>=10 && v[2]<=13) break;
#endif
            if (!race_save_decode_building(&race_data,i,&q)) return 0;
            if (q.alive && sc64_race_raw_building(q.type)==116 && (q.flags&4) && !marker) return 0;
            if (!race_bc_facility_fields_valid(&q,(unsigned)(width*16),(unsigned)(height*16))) return 0;
        }
#endif
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
        unsigned pending = 0;
        for (unsigned i=0; i<MAX_BUILDINGS; ++i) {
            Building lab;
            int key;
            Sc64BattlecruiserResearch research;
            if (!race_save_decode_building(&race_data,i,&lab)) return 0;
            if (!lab.alive || sc64_race_raw_building(lab.type) != 118) continue;
            if (lab.raw_type != -1 && lab.raw_type != 118) return 0;
            if (lab.level < 0 || lab.queue_progress < 0 || lab.queue_count) return 0;
            research.kind = (unsigned)lab.level;
            research.remaining = (unsigned)lab.queue_progress;
            if (!sc64_battlecruiser_research_valid(&research)) return 0;
            if (!research.kind) continue;
            if (!marker || lab.under_construction) return 0;
            key = sc64_battlecruiser_owner_key(v[0], v[2],lab.owner,lab.raw_owner);
            if (key < 0 || sc64_battlecruiser_tech_has(bits,(unsigned)key,research.kind) ||
                sc64_battlecruiser_tech_has(pending,(unsigned)key,research.kind)) return 0;
            (void)sc64_battlecruiser_tech_complete(&pending,(unsigned)key,research.kind);
        }
#endif
    }
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    if (!race_bc_shots_candidate_valid(&race_data,(unsigned)width*16u,(unsigned)height*16u SC64_GCTX_ARG)) return 0;
#endif
#ifdef SC64_SCARAB_RUNTIME
    if (!race_scarab_save_candidate_valid(&race_data,(unsigned)width*16u,
                                           (unsigned)height*16u SC64_GCTX_ARG)) return 0;
#endif
    plan->building_damage_marker = building_damage_marker;
    return 1;
}

static int race_load_preflight(RaceLoadPlan *plan) {
#if defined(SC64_P7_OBSERVER_ENABLED) || defined(SC64_P7_BATTERY_ENABLED)
    signed char prior_binding = race_protoss_binding_override;
    int prior_mission = race_protoss_override_mission;
#endif
    int ok = race_load_preflight_candidate(plan);
#if defined(SC64_P7_OBSERVER_ENABLED) || defined(SC64_P7_BATTERY_ENABLED)
    race_protoss_binding_override = prior_binding;
    race_protoss_override_mission = prior_mission;
#endif
    return ok;
}

#ifdef SC64_WORKER_CARGO_KIND
/* Select and validate both generations together before publication. The
 * callback only mutates the shared candidate and its load plan. */
static int race_worker_cargo_prepare_selected(const void *raw,u32 len,u32 tag,void *context) {
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
    Sc64CfPrepareContext *compound=context;
    RaceLoadPlan *plan=compound?compound->plan:0;
    if(!plan || !compound->flight)return 0;
#else
    RaceLoadPlan *plan=context;
#endif
#if defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) || defined(SC64_GUARDIAN_COCOON_STAGE)
    if (!plan) return 0;
    memset(plan,0,sizeof(*plan));
    if (raw != (const void *)&race_impact_record) return 0;
#else
    if (raw != (const void *)&race_impact_record) return 0;
    memset(plan,0,sizeof(*plan));
#endif
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
    /* Only the compound reader calls this stage, after validating its full
     * outer checksum and fixed header. Bind the token to this scratch record. */
    if (tag!=SC64_COMPOUND_FLASH_TAG) return 0;
    const Sc64StageTagPair compound_pair={SC64_COMPOUND_FLASH_TAG,SAVE_VERSION,SC64_STAGE_SAVE_SCARAB};
    if (!sc64_stage_save_authenticate(&plan->compound_auth,&race_data,tag,race_data.version,
                                      &compound_pair,1)) return 0;
    memcpy(plan->compound_flight,compound->flight,sizeof plan->compound_flight);
    plan->compound_auth.flight=plan->compound_flight;
    plan->compound_auth.flight_count=MAX_UNITS;
    tag=SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION;
#endif
#if defined(SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE)
    const Sc64StageTagPair combined_tags[]={
        {SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,0},
        {SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION,0},
        {SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_SAVE_VERSION,SC64_STAGE_SAVE_GUARDIAN},
        {SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_SAVE_VERSION,SC64_STAGE_SAVE_EGG},
        {SC64_GUARDIAN_EGG_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_EGG_SAVE_VERSION,SC64_STAGE_SAVE_GUARDIAN|SC64_STAGE_SAVE_EGG}
    };
    if (!sc64_stage_save_authenticate(&plan->guardian_auth,&race_data,tag,race_data.version,
             combined_tags,sizeof(combined_tags)/sizeof(combined_tags[0]))) return 0;
    plan->legacy_worker_cargo=(tag==SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION);
    u32 impact_tag=plan->guardian_auth.features ? SC64_TRANSPORT_SAVE_VERSION : tag;
    if (!race_impact_save_decode_selected_guardian(len,impact_tag,plan->impact_candidate,
                                          &plan->impact_candidate_count,&plan->guardian_auth)) return 0;
#elif defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    const Sc64StageTagPair egg_tags[]={
        {SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,0},
        {SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION,0},
        {SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_SAVE_VERSION,SC64_STAGE_SAVE_EGG}
    };
    /* save_select_prepared has already validated this slot's checksum. Bind
     * the unchanged inner version to this exact outer tag before decoding. */
    if (!sc64_stage_save_authenticate(&plan->egg_auth,&race_data,tag,race_data.version,
                                      egg_tags,sizeof(egg_tags)/sizeof(egg_tags[0]))) return 0;
    plan->legacy_worker_cargo=(tag==SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION);
    u32 impact_tag=(plan->egg_auth.features & SC64_STAGE_SAVE_EGG) ?
                    SC64_TRANSPORT_SAVE_VERSION : tag;
#ifdef SC64_SHARED_EGG_CUSTOM_CANDIDATE
    if (!race_impact_save_decode_selected_guardian(len,impact_tag,plan->impact_candidate,
                                          &plan->impact_candidate_count,&plan->egg_auth)) return 0;
#else
    if (!race_impact_save_decode_selected(len,impact_tag,plan->impact_candidate,
                                          &plan->impact_candidate_count)) return 0;
#endif
#elif defined(SC64_GUARDIAN_COCOON_STAGE)
    const Sc64StageTagPair guardian_tags[]={
        {SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,0},
        {SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION,0},
        {SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_SAVE_VERSION,SC64_STAGE_SAVE_GUARDIAN}
    };
    if (!sc64_stage_save_authenticate(&plan->guardian_auth,&race_data,tag,race_data.version,
             guardian_tags,sizeof(guardian_tags)/sizeof(guardian_tags[0]))) return 0;
    plan->legacy_worker_cargo=(tag==SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION);
    u32 impact_tag=(plan->guardian_auth.features & SC64_STAGE_SAVE_GUARDIAN) ?
                    SC64_TRANSPORT_SAVE_VERSION : tag;
    if (!race_impact_save_decode_selected_guardian(len,impact_tag,plan->impact_candidate,
                                          &plan->impact_candidate_count,&plan->guardian_auth)) return 0;
#else
#ifdef SC64_SHARED_TRANSPORT
    if (tag != SC64_TRANSPORT_SAVE_VERSION && tag != SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
        && tag != SC64_RESEARCH_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
        && tag != SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
        && tag != SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION
#endif
#endif
#endif
       ) return 0;
    plan->legacy_worker_cargo=(tag==SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION);
#else
    if (tag != SAVE_VERSION && tag != SC64_WORKER_CARGO_LEGACY_SAVE_VERSION
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
        && tag != SC64_LEGACY_SAVE_VERSION
#endif
       ) return 0;
    plan->legacy_worker_cargo=(tag!=SAVE_VERSION);
#endif
    if (!race_impact_save_decode_selected(len,
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
        (tag==SC64_RESEARCH_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
         || tag==SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
         || tag==SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION
#endif
#endif
        ) ? SC64_TRANSPORT_SAVE_VERSION :
#endif
        tag,plan->impact_candidate,&plan->impact_candidate_count)) return 0;
#if defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT) && !defined(SC64_SHARED_TRANSPORT)
    if (race_data.version != tag) return 0;
#else
    if (race_data.version != (plan->legacy_worker_cargo ? SC64_WORKER_CARGO_LEGACY_SAVE_VERSION : SAVE_VERSION)) return 0;
#endif
#endif /* SC64_SHARED_EGG_JOURNAL_HOST_STAGE */
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
    if (tag==SC64_RESEARCH_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
        || tag==SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
        || tag==SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION
#endif
#endif
       ) {
        if ((race_data.state[0]!=0 && race_data.state[0]!=1)
#ifdef SC64_RESEARCH_SRAM_PRODUCT
            || race_data.state[0]!=1 /* only custom games write a research tag */
#endif
            || !sc64_pr_padding_read((u8 *)race_data.buildings,MAX_BUILDINGS,SC64_PR_ROW_WIDTH,
                                  SC64_PR_USED_BITS,race_data.state[0] ? 5u : 8u,
                                  &plan->research_levels)) return 0;
    }
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
    plan->research_jobs_tag=(u8)(tag==SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION);
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
    if (tag==SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION) plan->research_jobs_tag=2;
    /* Old campaign records did not initialize typed levels. Preserve any
     * old typed levels while restoring the authenticated scenario floor. */
    if (race_data.state[0]==0 && race_data.state[1]==R_PROTOSS && plan->research_jobs_tag==0) {
        for (unsigned family=0;family<SC64_PR_FAMILY_COUNT;++family) {
            Sc64PrCampaignRule rule;
            if (!race_research_campaign_candidate_rule(&race_data,family,&rule)) return 0;
            if (plan->research_levels.level[SIDE][family]<rule.initial)
                plan->research_levels.level[SIDE][family]=rule.initial;
        }
    }
#endif
    if (!race_research_jobs_candidate_scan(&race_data,&plan->research_levels,
                                           plan->research_jobs_tag)) return 0;
#endif
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
#if !defined(SC64_SHARED_TRANSPORT)
    if (tag==SC64_LEGACY_SAVE_VERSION) {
        if (race_data.state[27] != 0) return 0;
    } else
#endif
    if (((unsigned)race_data.state[27] & 0xffff0000u) != 0x42450000u) return 0;
#endif
#ifdef SC64_DIAGNOSTIC_SAVE_VALIDATE
    u32 start=gba_clock();
    int accepted=race_load_preflight(plan);
    sc64_diagnostic_save_validate(1,-1,gba_clock()-start,tag,len,accepted);
    return accepted;
#else
    return race_load_preflight(plan);
#endif
}
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
#include "sc64_compound_flash_host_adapter.h"
#elif defined(SC64_Q8_FLASH_HOST_STAGE) || defined(SC64_Q8_FLASH_ARM_PROBE)
#include "sc64_q8_flash_host_adapter.h"
#endif
static int race_worker_cargo_select(RaceLoadPlan *plan,GbaSaveSelection *selection) {
#ifdef SC64_SHARED_TRANSPORT
#ifdef SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
    const u32 tags[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION,
        SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,
        SC64_GUARDIAN_EGG_TRANSPORT_SAVE_VERSION};
#elif defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    const u32 tags[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION};
#elif defined(SC64_GUARDIAN_COCOON_STAGE)
    const u32 tags[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_TRANSPORT_SAVE_VERSION};
#else
    const u32 tags[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
        ,SC64_RESEARCH_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
        ,SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION
#ifdef SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL
        ,SC64_RESEARCH_CAMPAIGN_JOBS_TRANSPORT_SAVE_VERSION
#endif
#endif
#endif
    };
#endif
#elif defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT)
    const u32 tags[]={SC64_LEGACY_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,SAVE_VERSION};
#else
    const u32 tags[]={SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,SAVE_VERSION};
#endif
#if defined(SC64_GUARDIAN_COCOON_STAGE) || defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    int selected=save_select_prepared(&race_impact_record,sizeof(race_impact_record),tags,sizeof(tags)/sizeof(tags[0]),
                                      race_worker_cargo_prepare_selected,plan,selection);
    if (!selected) memset(plan,0,sizeof(*plan));
    return selected;
#else
    return save_select_prepared(&race_impact_record,sizeof(race_impact_record),tags,sizeof(tags)/sizeof(tags[0]),
                                race_worker_cargo_prepare_selected,plan,selection);
#endif
}
static int race_worker_cargo_previous(GbaSaveSelection *selection) {
    RaceLoadPlan plan;
    return race_worker_cargo_select(&plan,selection);
}
#endif

#ifdef SC64_SCARAB_NAMESPACE_STAGE
/* Stage scope: supported skirmish geometries and the Terran compact codec.
 * The tag, never field values, determines the parent representation. */
static int race_scarab_stage_prepare(const void *raw,u32 len,u32 tag,void *context) {
    RaceLoadPlan *plan=context;
    if (raw != (const void *)&race_impact_record) return 0;
    memset(plan,0,sizeof(*plan));
    if (!race_impact_save_decode_selected(len,tag,plan->impact_candidate,
                                          &plan->impact_candidate_count)) return 0;
#ifdef SC64_SHARED_TRANSPORT
    if (tag != SC64_SCARAB_TRANSPORT_SAVE_VERSION) return 0;
    tag = SC64_SCARAB_SAVE_VERSION;
#endif
    race_scarab_stage_wire=(tag==SC64_SCARAB_SAVE_VERSION);
    int map=race_data.state[20], width=64, height=64;
    if (race_data.state[0]!=1 || map<0 || map>SC64_SKIRMISH_IMPORTED_COUNT) return 0;
    if (map) { width=sc64_skirmish_maps[map-1].width; height=sc64_skirmish_maps[map-1].height; }
    if (!sc64_race_storage_geometry_valid(width,height)) return 0;
    for (unsigned i=0;i<MAX_UNITS;i++) {
        Unit q;
        if (!SC64_GUARD_DECODE(&race_data,i,(unsigned)width*16u,(unsigned)height*16u,&q)) return 0;
        if (!q.alive) continue;
        if ((q.type==U_SCARAB || q.raw_type==85) && !race_scarab_stage_wire) return 0;
        if (sc64_race_raw_unit(q.type)==83) {
            int c,t;
            if (q.raw_type!=-1 && q.raw_type!=83) return 0;
            if (!sc64_scarab_parent_convert(tag==SC64_SCARAB_SAVE_VERSION ?
                    SC64_SCARAB_PARENT_PACKED : SC64_SCARAB_PARENT_LEGACY,
                    83,q.carry,q.work_timer,&c,&t)) return 0;
            q.carry=c; q.work_timer=t;
#ifdef SC64_ZP_FULL_STORAGE
            if (!race_scarab_zp_encode(&race_data,i,&q,(unsigned)width*16u,(unsigned)height*16u)) return 0;
#else
            if (!sc64_unit_save_encode(&race_data.units[i],&q,(unsigned)width*16u,(unsigned)height*16u)) return 0;
#endif
        }
    }
    /* A checked new envelope has the existing BC payload layout. Mission
     * validators still expect the existing internal version token. */
    if (tag==SC64_SCARAB_SAVE_VERSION) race_data.version=SAVE_VERSION;
    return race_load_preflight(plan);
}
static int race_scarab_stage_select(RaceLoadPlan *plan,GbaSaveSelection *selection) {
#ifdef SC64_SHARED_TRANSPORT
    const u32 tags[]={SC64_SCARAB_TRANSPORT_SAVE_VERSION};
    const unsigned tag_count=1;
#else
    const u32 tags[]={SC64_LEGACY_SAVE_VERSION,SAVE_VERSION,SC64_SCARAB_SAVE_VERSION};
    const unsigned tag_count=3;
#endif
    if (!save_select_validated(&race_impact_record,sizeof(race_impact_record),tags,tag_count,
                               race_scarab_stage_prepare,plan,selection)) return 0;
    return race_scarab_stage_prepare(&race_impact_record,selection->length,
                                     selection->tag,plan);
}
static int race_scarab_stage_previous(GbaSaveSelection *selection) {
    RaceLoadPlan plan;
    return race_scarab_stage_select(&plan,selection);
}
#endif
static int race_load_game(void) {
#if defined(SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE) && !defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
    return 0; /* No compound envelope reader has been admitted yet. */
#endif
#if defined(SC64_CAMPAIGN_CONTROLLER_HOST_STAGE) && !defined(SC64_CONTROLLER_FLASH_P1_HOST_STAGE) && !defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
    return 0; /* title state cannot identify an incoming campaign candidate */
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
    if (sc64_pr_stage_fault) return 0;
#endif
#if defined(SC64_EXPERIMENTAL_RESEARCH_JOBS) && !defined(SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL)
    /* No old or research-journal candidate may publish ambiguous staged
     * building fields until active-job decoding is separately validated. */
    return 0;
#endif
#if defined(SC64_SCARAB_AI_RESTOCK_HOST_STAGE) && !defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
    return 0; /* No candidate can restore the HOST payer witnesses. */
#endif
    RaceLoadPlan plan = {0};
#ifdef SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE
    const Sc64StageSaveAuth *guard_auth=&plan.compound_auth;
#elif defined(SC64_GUARDIAN_COCOON_STAGE)
    const Sc64StageSaveAuth *guard_auth=&plan.guardian_auth;
#elif defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE)
    const Sc64StageSaveAuth *guard_auth=&plan.egg_auth;
#endif
#ifdef SC64_WORKER_CARGO_KIND
#if defined(SC64_Q8_FLASH_HOST_STAGE) || defined(SC64_Q8_FLASH_ARM_PROBE)
    if (!race_q8_flash_select(&plan)) return 0;
#else
    GbaSaveSelection selected;
    if (!race_worker_cargo_select(&plan,&selected)) return 0;
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL
    if (!race_research_jobs_candidate_scan(&race_data,&plan.research_levels,
                                           plan.research_jobs_tag)) return 0;
#endif
#endif
#elif defined(SC64_SCARAB_NAMESPACE_STAGE)
    GbaSaveSelection selected;
    if (!race_scarab_stage_select(&plan,&selected)) return 0;
#else
#ifdef SC64_IMPACT_EVENTS
    /* Decode both queues before validators reuse the scratch tail. */
    if (!race_impact_save_read(plan.impact_candidate,&plan.impact_candidate_count)) return 0;
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    if (race_data.version == SAVE_VERSION) {
        if (((unsigned)race_data.state[27] & 0xffff0000u) != 0x42450000u
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
            || ((unsigned)race_data.state[28] & ~31u) != 0x42440000u ||
               ((unsigned)race_data.state[28] & 7u)>4u
#endif
            ) return 0;
    } else if (race_data.version != SC64_LEGACY_SAVE_VERSION || race_data.state[27] != 0) return 0;
#else
    if (race_data.version != SAVE_VERSION) return 0;
#endif
#elif defined(SC64_SHARED_TRANSPORT)
    {
        u8 trailer[16];
        if (!save_read_parts(&race_data, sizeof(race_data), trailer, sizeof(trailer),
                             SC64_TRANSPORT_SAVE_VERSION) ||
            race_data.version != SAVE_VERSION ||
            !sc64_sk_transport_decode(&race_skirmish_transport_candidate, trailer, MAX_BUILDINGS))
            return 0;
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
        if (((unsigned)race_data.state[27] & 0xffff0000u) != 0x42450000u
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
            || ((unsigned)race_data.state[28] & ~31u) != 0x42440000u ||
               ((unsigned)race_data.state[28] & 7u) > 4u
#endif
           ) return 0;
#endif
    }
#elif defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT)
    if (save_read(&race_data,sizeof(race_data),SAVE_VERSION)) {
        if (race_data.version != SAVE_VERSION ||
            ((unsigned)race_data.state[27] & 0xffff0000u) != 0x42450000u
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
            || ((unsigned)race_data.state[28] & ~31u) != 0x42440000u ||
               ((unsigned)race_data.state[28] & 7u)>4u
#endif
            ) return 0;
    } else {
        memset(&race_data,0,sizeof(race_data));
        if (!save_read(&race_data,sizeof(race_data)-SC64_BC_SAVE_EXTRA_BYTES,SC64_LEGACY_SAVE_VERSION) ||
            race_data.version != SC64_LEGACY_SAVE_VERSION || race_data.state[27] != 0) return 0;
    }
#else
    if(!save_read(&race_data,sizeof(race_data),SAVE_VERSION)||race_data.version!=SAVE_VERSION)return 0;
#endif
    if (!race_load_preflight(&plan)) return 0;
#endif
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOURNAL
    sc64_experimental_research_levels=plan.research_levels;
#endif
    const int *v = race_data.state;
    race_mode=v[0];race_human=v[1];race_selected_mission=v[2];race_completed=v[3];race_bots=v[4];
#ifdef SC64_SHARED_TRANSPORT
    race_skirmish_transport_tech = race_skirmish_transport_candidate;
#endif
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    sc64_battlecruiser_tech_bits = (u16)((unsigned)v[27] & 65535u);
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    memcpy(sc64_bc_shots,race_data.battlecruiser_shots,sizeof(sc64_bc_shots));
    sc64_bc_pause_offset=(u8)((unsigned)v[28]&7u);
    if ((unsigned)v[28]==0) sc64_bc_spell_rng=42u;
    else memcpy(&sc64_bc_spell_rng,&v[29],sizeof(sc64_bc_spell_rng));
#endif
#endif
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
        if(!SC64_GUARD_DECODE(&race_data, (unsigned)i, (unsigned)(race_cols * 16), (unsigned)(race_rows * 16), &decoded)) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
        if(!SC64_GUARD_DECODE(&race_data, (unsigned)i,
                                  (unsigned)(race_cols * 16),
                                  (unsigned)(race_rows * 16), &decoded)) return 0;
#else
#if defined(SC64_SCARAB_NAMESPACE_STAGE) || defined(SC64_GUARDIAN_COCOON_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE) || defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
        if(!SC64_GUARD_DECODE(&race_data,(unsigned)i,(unsigned)(race_cols*16),
                                  (unsigned)(race_rows*16),&decoded)) return 0;
#else
        if(!sc64_zp_unit_save_decode(&decoded, &race_data.units[i],
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16))) return 0;
#endif
#endif
#else
#if defined(SC64_SCARAB_NAMESPACE_STAGE) || defined(SC64_GUARDIAN_COCOON_STAGE) || defined(SC64_SHARED_EGG_CUSTOM_CANDIDATE) || defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
        if(!SC64_GUARD_DECODE(&race_data,(unsigned)i,(unsigned)(race_cols*16),
                                  (unsigned)(race_rows*16),&decoded)) return 0;
#else
        if(!sc64_unit_save_decode(&decoded, &race_data.units[i],
                                  (unsigned)(race_cols * 16),
                                  (unsigned)(race_rows * 16))) return 0;
#endif
#endif
        memcpy(&u[i], &decoded, sizeof(decoded));
    }
#else
    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {
        int n=j==offsetof(Unit,flags)/sizeof(int)?(u16)race_data.units[i][j]:race_data.units[i][j];
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
        if (j==offsetof(Unit,carry)/sizeof(int) &&
            sc64_race_raw_unit(race_data.units[i][offsetof(Unit,type)/sizeof(int)])==12 &&
            ((unsigned)race_data.state[27]&0xffff0000u)==0x42450000u)
            n=(u16)race_data.units[i][j];
#endif
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
    if (!plan.building_damage_marker)
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
    /* Custom games: restore the shared service clock. Saves from before the
     * shared services carry no cache word; those actors start with an empty
     * one. */
    if (race_mode) {
        race_protoss_clock_from_word(race_protoss_generic_clock,
                                     v[SC64_PROTOSS_CLOCK_STATE_INDEX]);
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
        sc64_p7_arbiter_cache_due=0;
        sc64_p7_arbiter_cache_initialized=
            race_protoss_generic_clock[1] || race_protoss_generic_clock[2];
        for (int i=0;i<MAX_UNITS;++i)
            if (!sc64_p7_arbiter_cache_empty_unit(&u[i]) &&
                !SC64_P7_ARBITER_CACHE_VALID(u[i].weapon))
                u[i].weapon=(u16)(1u<<13);
#endif
        if (!race_p7_battery_live_world_valid()) return 0;
    }
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
#ifdef SC64_P8_RUNTIME
    if (race_p8_active()) {
        race_p8 = race_data.p8;
        if (!race_p8_rebind_loaded_world(&sc64_race_maps[SC64_P8_GLOBAL_ID]))
            return 0;
    } else {
        memset(&race_p8, 0, sizeof(race_p8));
        race_p8_map = 0;
    }
#endif
#ifdef SC64_P9_RUNTIME
    if (race_p9_active()) {
        race_p9 = race_data.p9;
        if (!race_p9_rebind_loaded_world(&sc64_race_maps[SC64_P9_GLOBAL_ID]))
            return 0;
    } else {
        memset(&race_p9, 0, sizeof(race_p9));
        race_p9_map = 0;
    }
#endif
#ifdef SC64_P10_RUNTIME
    if (race_p10_active()) {
        race_p10 = race_data.p10;
        if (!race_p10_rebind_loaded_world(&sc64_race_maps[SC64_P10_GLOBAL_ID]))
            return 0;
    } else {
        memset(&race_p10, 0, sizeof(race_p10));
        race_p10_map = 0;
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
#ifdef SC64_Z8_RUNTIME
    if (race_z8_active()) {
        if (!race_z8_save_unpack_for(&race_data.z8,
                                 &sc64_zp_route_save_scratch.z8_candidate.scheduler, 1,
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
                                 sc64_stage_save_auth_matches(&plan.egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG)
#else
                                 0
#endif
                                 ) ||
            !race_z8_rebind_loaded_world(&sc64_race_maps[SC64_Z8_GLOBAL_ID]))
            return 0;
    } else if (!race_z8_runtime_init(0)) {
        return 0;
    }
#endif
#ifdef SC64_Z9_RUNTIME
    if (race_zl_active()) {
        if (!race_zl_save_unpack_for(&race_data.zl,
                                 &sc64_zp_route_save_scratch.zl_candidate.scheduler, 1,
#ifdef SC64_SHARED_EGG_JOURNAL_HOST_STAGE
                                 sc64_stage_save_auth_matches(&plan.egg_auth,&race_data,race_data.version,SC64_STAGE_SAVE_EGG)
#else
                                 0
#endif
                                 ) ||
            !race_zl_rebind_loaded_world(&sc64_race_maps[race_selected_mission]))
            return 0;
    } else if (!race_zl_runtime_init(0)) {
        return 0;
    }
#endif
#ifdef SC64_NYDUS_ENABLED
    /* After mission rebinds: a mission adapter may install its source. */
    if (!race_nydus_rebind_loaded_world()) return 0;
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (!race_mode && race_selected_mission == SC64_T4_GLOBAL_ID) {
#ifdef SC64_T3_ENABLED
        race_t4 = plan.expanded.t4;
#else
        race_t4 = race_data.t4;
#endif
    } else
        memset(&race_t4, 0, sizeof(race_t4));
#endif
#ifdef SC64_T5_RUNTIME
    if (plan.expanded_t5_valid) race_t5 = plan.expanded.t5;
    else memset(&race_t5, 0, sizeof(race_t5));
#endif
#ifdef SC64_T6_RUNTIME
    if (plan.expanded_t6_valid) race_t6 = plan.expanded.t6;
    else memset(&race_t6, 0, sizeof(race_t6));
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (plan.expanded_terran_late_valid) race_late = plan.expanded.terran_late;
    else memset(&race_late, 0, sizeof(race_late));
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    /* Last commit step: later mission rebind failures must not clear the old
     * projectile arena. The candidate packet already passed full preflight. */
    if (!race_p7_projectile_restore(race_data.interceptor_projectiles)) return 0;
#endif
    clear_tracers();death_vfx_reset();mode=MODE_NORMAL;menu_open=help=0;selected_building=-1;
#ifdef SC64_SAVE_UI_CONTEXT
    if (v[SC64_SAVE_UI_STATE_INDEX]) {
        unsigned ui = (unsigned)v[SC64_SAVE_UI_STATE_INDEX];
        unsigned selected = ui & 255u;
        if (selected && b[selected - 1u].alive) selected_building = (int)selected - 1;
        menu_cursor = (int)((ui >> 8) & 255u);
#ifdef SC_FIELD_FEEDBACK
        menu_last = (u8)((ui >> 16) & 255u);
#endif
    }
#endif
#ifdef SC64_IMPACT_EVENTS
    race_impact_restore(plan.impact_candidate,plan.impact_candidate_count);
#endif
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
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_capacity(SIDE);
#if defined(SC64_Q8_FLASH_HOST_STAGE) || defined(SC64_Q8_FLASH_ARM_PROBE)
    memcpy(sc64_pr_hp_debt,plan.q8_debt,sizeof sc64_pr_hp_debt);
#if defined(SC64_CONTROLLER_FLASH_P1_HOST_STAGE) || defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
    memcpy(sc64_current_unit,plan.controller_units,sizeof sc64_current_unit);
    memcpy(sc64_current_building,plan.controller_buildings,sizeof sc64_current_building);
#endif
#ifdef SC64_COMPOUND_FLASH_Z10_HOST_STAGE
    memcpy(sc64_scarab_stage_payer,plan.scarab_payers,sizeof sc64_scarab_stage_payer);
#endif
#endif
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    sc64_pr_stage_resync();
#endif
    screen=won||lost?SCREEN_RESULT:SCREEN_PLAY;toast(7);return 1;
}
#endif
