/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_CANDIDATE_VALIDATION_H
#define SC64_GUARDIAN_CANDIDATE_VALIDATION_H
/* Include after detached unit/building decoders and stage authentication.
 * Never read live mission, race, owner, resources or SRAM from this header. */
#include "sc64_guardian_paid_building.h"

/* Source bit 0x2000 is legal on unrelated buildings (notably P6 doodads).
 * Reject only types for which it could be mistaken for a paid Hive/Spire
 * job. Raw 59 has no separate source/no-job identity in this candidate. */
static int race_guardian_source_row_valid(unsigned raw,unsigned flags)
{
    if (raw==59u) return 0;
    if ((raw==133u || raw==137u) &&
        (flags & SC64_GUARDIAN_PAID_BUILDING_MORPH)) return 0;
    return 1;
}
static int race_guardian_source_map_valid(const Sc64RaceMap *map)
{
    if (!map || (!map->units && map->unit_count)) return 0;
    for (unsigned i=0;i<map->unit_count;++i)
        if (!race_guardian_source_row_valid(map->units[i].source_type,map->units[i].flags)) return 0;
#ifdef SC64_P6_RUNTIME
    if (SC64_P6_GLOBAL_ID<SC64_RACE_MAP_COUNT && map==&sc64_race_maps[SC64_P6_GLOBAL_ID])
        for (unsigned i=0;i<SC64_P6_DOODAD_SOURCE_COUNT;++i)
            if (!sc64_p6_doodads[i].sprite_only &&
                !race_guardian_source_row_valid(sc64_p6_doodads[i].raw_type,sc64_p6_doodads[i].flags)) return 0;
#endif
    return 1;
}

static int race_guardian_candidate_puni(const unsigned char *puni,int source,int raw)
{
    if (!puni || (unsigned)source>=12u || (unsigned)raw>=228u) return 0;
    unsigned cell=(unsigned)source*228u+(unsigned)raw;
    return puni[2964u+cell] ? puni[2736u+(unsigned)raw]!=0 : puni[cell]!=0;
}
/* Ownership after a legitimate transfer may differ from raw_owner. The
 * immutable source PUNI authenticates origin; the saved owner must be an
 * active controller. P8/Terran and early Zerg mission mappings are deliberately
 * rejected until their immutable candidate permission adapters are supplied. */
static int race_guardian_candidate_origin(const RaceSave *candidate,int owner,int source,int raw)
{
    static const int owners[4]={SIDE,ENEMY,RESCUABLE,4};
    if (!candidate || (unsigned)owner>=5u || (unsigned)candidate->players[owner]>2u ||
        (unsigned)raw>=228u || (unsigned)candidate->state[0]>1u) return 0;
    if (candidate->state[0]) {
        int bots=candidate->state[4],active=0;
        if (source!=-1 || bots<1 || bots>3) return 0;
        for (int i=0;i<=bots;++i) if (owners[i]==owner) active=1;
        return active;
    }
    int mission=candidate->state[2];
    if ((unsigned)mission>=SC64_RACE_MAP_COUNT || (unsigned)source>=8u) return 0;
    const Sc64RaceMap *map=&sc64_race_maps[mission];
    /* The caller preflights this exact descriptor once per candidate. */
    if (!map->owner_races || map->owner_races[source]!=R_ZERG) return 0;
#ifdef SC64_Z7_RUNTIME
    if (mission==SC64_Z7_GLOBAL_ID)
        return race_guardian_candidate_puni(sc64_z7_source_puni,source,raw);
#endif
#ifdef SC64_Z6_RUNTIME
    if (mission==SC64_Z6_GLOBAL_ID)
        return race_guardian_candidate_puni(sc64_z6_source_puni,source,raw);
#endif
#ifdef SC64_Z5_RUNTIME
    if (mission==SC64_Z5_GLOBAL_ID)
        return race_guardian_candidate_puni(sc64_z5_source_puni,source,raw);
#endif
#ifdef SC64_Z8_RUNTIME
    if (mission==SC64_Z8_GLOBAL_ID)
        return sc64_z8_permission_available(&sc64_z8_permission_table,(unsigned)source,(unsigned)raw);
#endif
#ifdef SC64_Z9_RUNTIME
    {
        const Sc64ZlMission *m=race_zl_mission_for(mission);
        if (m) return sc64_z8_permission_available(m->permissions,(unsigned)source,(unsigned)raw);
    }
#endif
    return 0;
}

static int race_guardian_candidate_valid(const RaceSave *candidate,unsigned width,unsigned height,
                                          const Sc64StageSaveAuth *guard_auth)
{
    if (!candidate || !guard_auth || !sc64_stage_save_auth_matches(guard_auth,candidate,
        candidate->version,0u) || (unsigned)candidate->state[0]>1u) return 0;
    int guardian=(guard_auth->features & SC64_STAGE_SAVE_GUARDIAN)!=0;
    if (guardian && !candidate->state[0]) {
        int mission=candidate->state[2];
        if ((unsigned)mission>=SC64_RACE_MAP_COUNT ||
            !race_guardian_source_map_valid(&sc64_race_maps[mission])) return 0;
    }
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit q;
        if (!SC64_GUARD_DECODE(candidate,i,width,height,&q)) return 0;
        if (q.type!=U_GUARDIAN_COCOON && q.raw_type!=59) continue;
        if (!guardian || q.type!=U_GUARDIAN_COCOON || q.raw_type!=59 ||
            !race_guardian_candidate_origin(candidate,q.owner,q.raw_owner,44) ||
            q.maxhp!=200 || q.armor!=0 || q.shield!=0 || q.shield_remainder!=0 ||
            q.carry!=0 || q.flags!=0 || q.build!=-1 || q.order!=ORDER_IDLE || q.target!=44 ||
            q.work_timer<0 || q.work_timer>SC64_GUARDIAN_TICKS ||
            (q.alive ? (q.hp<1 || q.hp>200 || q.work_timer<1) : q.hp>0)) return 0;
    }
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building q;
        if (!race_save_decode_building(candidate,i,&q)) return 0;
        if ((q.type!=B_HIVE && q.type!=B_GREATER_SPIRE) ||
            !(q.flags & SC64_GUARDIAN_PAID_BUILDING_MORPH)) continue;
        int raw=sc64_race_raw_building(q.type);
        if (!guardian || !q.alive || !q.under_construction || q.raw_type!=raw ||
            q.maxhp!=sc64_race_building_max_hp(q.type) || q.hp<1 || q.hp>q.maxhp ||
            q.queue_count || q.build_progress<0 || q.build_progress>building_build_time(q.type) ||
            !race_guardian_candidate_origin(candidate,q.owner,q.raw_owner,raw)) return 0;
    }
    return 1;
}
#endif
