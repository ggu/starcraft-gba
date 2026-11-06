/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Z7_LEGACY_ROWS_PROBE_H
#define SC64_Z7_LEGACY_ROWS_PROBE_H
#if !defined(GBA_HOST) || !defined(SC64_Z7_LEGACY_ROWS_HOST_PROBE)
#error "Z7 predecessor rows are an isolated HOST probe, not a save reader"
#endif
/* Include after the game, Z7 candidate validators, legacy compact codecs,
 * transport tech and the metadata probe. These functions inspect detached
 * rows only; they cannot select, migrate, commit or admit a candidate world. */
enum { SC64_Z7_V36_UNIT_ROWS=240, SC64_Z7_V36_BUILDING_ROWS=160,
       SC64_Z7_V36_UNIT_OFFSET=4, SC64_Z7_V36_BUILDING_OFFSET=8404 };
_Static_assert(MAX_UNITS==SC64_Z7_V36_UNIT_ROWS && MAX_BUILDINGS==SC64_Z7_V36_BUILDING_ROWS,
               "Z7 predecessor row counts differ");
_Static_assert(sizeof(Sc64ZpPackedUnitSave)==35 && sizeof(Sc64PackedBuildingSave)==28,
               "Z7 predecessor row sizes differ");
static int sc64_z7_v36_row_output_disjoint(const volatile uint8_t *slot,const void *out,size_t n)
{
    uintptr_t s=(uintptr_t)slot, r=(uintptr_t)out;
    return !((r>=s && r-s<SC64_Z7_V36_SLOT_BYTES) || (s>=r && s-r<n));
}
static int sc64_z7_v36_row_context(const Sc64Z7LegacyEnvelopeProbe *meta,const Sc64RaceMap *map,
                                    const Sc64Z7LegacyMapAuthority *authority)
{
    Sc64Z7LegacyEnvelopeProbe current;
    if (!meta || !meta->slot || !sc64_z7_v36_probe_context(meta->slot,SC64_Z7_V36_SLOT_BYTES,
                                                            map,authority,&current)) return 0;
    return current.length==meta->length && current.sequence==meta->sequence &&
           current.checksum==meta->checksum;
}
static int sc64_z7_v36_probe_unit(const Sc64Z7LegacyEnvelopeProbe *meta,const Sc64RaceMap *map,
                                  const Sc64Z7LegacyMapAuthority *authority,unsigned index,Unit *out)
{
    Sc64ZpPackedUnitSave row;Unit q;
    if (!out || index>=SC64_Z7_V36_UNIT_ROWS || !meta || !meta->slot ||
        !sc64_z7_v36_row_output_disjoint(meta->slot,out,sizeof(*out)) ||
        !sc64_z7_v36_row_context(meta,map,authority)) return 0;
    const volatile uint8_t *src=meta->slot+SC64_Z7_V36_HEADER_BYTES+SC64_Z7_V36_UNIT_OFFSET+index*sizeof(row);
    for (unsigned i=0;i<sizeof(row);++i) row.bytes[i]=src[i];
    /* Feature zero is essential: the predecessor has no staged Scarab or
     * combined Guardian/Egg capability. Legacy Z7 Egg is its own old type. */
    if (!sc64_zp_unit_save_decode(&q,&row,2048,2048)) return 0;
    if (!race_z7_save_unused_unit(&q)) {
        if (q.type==U_Z7_EGG || q.raw_type==36) {
            if (!race_z7_save_egg_valid(map,&q,SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS)) return 0;
        } else if (q.raw_owner>=0 || q.raw_type>=0) {
            int raw=q.raw_type;unsigned source=(unsigned)q.raw_owner;
            if (q.raw_owner<0 || source>=8u || raw<0 || raw>=228 ||
                sc64_race_unit_from_raw(raw)!=q.type ||
                q.owner!=race_z7_save_source_owner(map,source) ||
                !sc64_race_unit_rule(q.type) ||
                sc64_race_unit_rule(q.type)->race!=map->owner_races[source]) return 0;
        } else if (q.alive && (q.raw_owner!=-1 || q.raw_type!=-1)) return 0;
        if (q.alive && q.raw_owner==-1 && q.owner!=SIDE) return 0;
    }
    *out=q;return 1;
}
static int sc64_z7_v36_probe_building(const Sc64Z7LegacyEnvelopeProbe *meta,const Sc64RaceMap *map,
                                      const Sc64Z7LegacyMapAuthority *authority,unsigned index,Building *out)
{
    Sc64PackedBuildingSave row;Building q;
    if (!out || index>=SC64_Z7_V36_BUILDING_ROWS || !meta || !meta->slot ||
        !sc64_z7_v36_row_output_disjoint(meta->slot,out,sizeof(*out)) ||
        !sc64_z7_v36_row_context(meta,map,authority)) return 0;
    const volatile uint8_t *src=meta->slot+SC64_Z7_V36_HEADER_BYTES+SC64_Z7_V36_BUILDING_OFFSET+index*sizeof(row);
    for (unsigned i=0;i<sizeof(row);++i) row.bytes[i]=src[i];
    if (!sc64_building_save_decode(&q,&row)) return 0;
    if (!race_z7_save_unused_building(&q)) {
        if (q.raw_owner<0 && q.raw_type<0) {
            if (q.alive && (q.owner!=SIDE || !race_z7_save_human_building_type(q.type))) return 0;
        } else {
            unsigned source=(unsigned)q.raw_owner;int raw=q.raw_type;
            if (q.raw_owner<0 || source>=8u || raw<0 || raw>=228 ||
                sc64_race_building_from_raw(raw)<0 ||
                q.owner!=race_z7_save_source_owner(map,source) ||
                !sc64_race_building_rule(q.type) ||
                sc64_race_building_rule(q.type)->race!=map->owner_races[source] ||
                !race_z7_save_building_type_matches_source(source,raw,q.owner,q.type)) return 0;
        }
    }
    *out=q;return 1;
}
static int sc64_z7_v36_probe_transport(const Sc64Z7LegacyEnvelopeProbe *meta,const Sc64RaceMap *map,
                                       const Sc64Z7LegacyMapAuthority *authority)
{
    uint8_t trailer[SC64_Z7_V36_TRAILER_BYTES];Sc64SkTransportTech tech;
    if (!sc64_z7_v36_row_context(meta,map,authority)) return 0;
    const volatile uint8_t *src=meta->slot+SC64_Z7_V36_HEADER_BYTES+meta->length-sizeof(trailer);
    for (unsigned i=0;i<sizeof(trailer);++i) trailer[i]=src[i];
    if (!sc64_sk_transport_decode(&tech,trailer,MAX_BUILDINGS)) return 0;
    /* This is the custom-only transport supplement. The historical campaign
     * context has no active custom owners, so all four jobs must be empty. */
    for (unsigned i=0;i<SC64_SK_TRANSPORT_OWNERS;++i)
        if (tech.owner[i].remaining || tech.owner[i].producer_plus_one || tech.owner[i].researched) return 0;
    return 1;
}
/* Bounded capacity census only; an overcommitted legacy state is reported,
 * not reclassified as corrupt. A later migration must preserve paid work and
 * prove the runtime can wait for every pending body and scripted create. */
typedef struct {
    unsigned live, queued, egg_extra, scripted, free_slots;
    int overcommitted;
} Sc64Z7LegacyCommitmentProbe;
_Static_assert(sizeof(Sc64Z7PersistentState)==34, "Z7 v36 state layout changed");
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "Z7 v36 state witness requires little-endian layout"
#endif
static int sc64_z7_v36_probe_commitments(const Sc64Z7LegacyEnvelopeProbe *meta,const Sc64RaceMap *map,
                                        const Sc64Z7LegacyMapAuthority *authority,
                                        Sc64Z7LegacyCommitmentProbe *out)
{
    Sc64Z7LegacyCommitmentProbe q={0};Sc64Z7PersistentState state;
    if (!out || !meta || !meta->slot ||
        !sc64_z7_v36_row_output_disjoint(meta->slot,out,sizeof(*out)) ||
        !sc64_z7_v36_row_context(meta,map,authority) ||
        !sc64_z7_v36_probe_transport(meta,map,authority)) return 0;
    uint8_t *dst=(uint8_t *)&state;
    const volatile uint8_t *src=meta->slot+SC64_Z7_V36_HEADER_BYTES+16116u;
    for(unsigned i=0;i<sizeof(state);++i) dst[i]=src[i];
    if (!sc64_z7_persistent_state_valid(&state)) return 0;
    q.scripted=race_z7_save_remaining_script_creates(&state);
    for(unsigned i=0;i<SC64_Z7_V36_UNIT_ROWS;++i) {
        Unit unit;
        if (!sc64_z7_v36_probe_unit(meta,map,authority,i,&unit)) return 0;
        if (!unit.alive) { ++q.free_slots; continue; }
        ++q.live;
        if (unit.type==U_Z7_EGG) {
            int count=sc64_z7_egg_product_count(unit.target);
            if (count<1 || count>2) return 0;
            q.egg_extra+=(unsigned)(count-1);
        }
    }
    for(unsigned i=0;i<SC64_Z7_V36_BUILDING_ROWS;++i) {
        Building building;
        if (!sc64_z7_v36_probe_building(meta,map,authority,i,&building)) return 0;
        if (!building.alive) continue;
        for(int j=0;j<building.queue_count;++j) {
            int type=building.queue[(building.queue_head+j)%5];
            const Sc64RaceUnitRule *unit=sc64_race_unit_rule(type);
            const Sc64RaceBuildingRule *producer=sc64_race_building_rule(building.type);
            if (!unit || !producer || unit->producer>=SC_RB_COUNT || !unit->build_frames ||
                unit->race!=producer->race ||
                !sc64_race_producer_compatible(building.type,unit->producer) ||
                (unit->race==R_ZERG && (unit->flags&SC_RACE_F_LARVA)) ||
                (building.owner==ENEMY && (building.raw_owner!=4 || type!=U_ZERGLING)) ||
                (building.owner!=SIDE && building.owner!=ENEMY)) return 0;
            q.queued+=(unsigned)sc64_race_product_count(type);
        }
    }
    q.overcommitted=(q.queued+q.egg_extra+q.scripted>q.free_slots);
    *out=q;return 1;
}
/* A detached row projection, not a RaceSave or an admissible world. It is
 * deliberately HOST-only: repeated envelope checks prioritize diagnostics
 * over guest timing. The valid flag is set only after the final recheck. */
typedef struct {
    Sc64DerivedUnitSave units[SC64_Z7_V36_UNIT_ROWS];
    Sc64DerivedBuildingSave buildings[SC64_Z7_V36_BUILDING_ROWS];
    Sc64Z7LegacyCommitmentProbe commitments;
    unsigned valid;
} Sc64Z7LegacyRowProjectionProbe;
_Static_assert(sizeof(Sc64Z7LegacyRowProjectionProbe)+
               (SC64_Z7_V36_SLOT_BYTES-SC64_Z7_V36_HEADER_BYTES)<=32768u,
               "diagnostic predecessor plus row projection exceeds route scratch");
static int sc64_z7_v36_probe_project_rows(const Sc64Z7LegacyEnvelopeProbe *meta,const Sc64RaceMap *map,
                                         const Sc64Z7LegacyMapAuthority *authority,
                                         Sc64Z7LegacyRowProjectionProbe *out)
{
    Sc64Z7LegacyCommitmentProbe commitments;
    if (!out) return 0;
    if (meta && meta->slot && !sc64_z7_v36_row_output_disjoint(meta->slot,out,sizeof(*out))) return 0;
    /* A rejected attempt must never leave a stale prior projection marked valid. */
    out->valid=0;
    if (!meta || !meta->slot ||
        !sc64_z7_v36_probe_commitments(meta,map,authority,&commitments)) return 0;
    /* Pass one establishes lossless representability before touching output. */
    for(unsigned i=0;i<SC64_Z7_V36_UNIT_ROWS;++i) {
        Unit old,roundtrip;Sc64DerivedUnitSave row;
        if (!sc64_z7_v36_probe_unit(meta,map,authority,i,&old) ||
            !sc64_derived_unit_encode_for(&row,&old,2048,2048,0) ||
            !sc64_derived_unit_decode_for(&roundtrip,&row,2048,2048,0) ||
            memcmp(&old,&roundtrip,sizeof(old))) return 0;
    }
    for(unsigned i=0;i<SC64_Z7_V36_BUILDING_ROWS;++i) {
        Building old,roundtrip;Sc64DerivedBuildingSave row;
        if (!sc64_z7_v36_probe_building(meta,map,authority,i,&old) ||
            !sc64_derived_building_encode(&row,&old,2048,2048) ||
            !sc64_derived_building_decode(&roundtrip,&row) ||
            memcmp(&old,&roundtrip,sizeof(old))) return 0;
    }
    /* Scratch may be partially written only if the source changes after pass
     * one; valid remains zero, and no live world or SRAM is ever the output. */
    memset(out,0,sizeof(*out));
    for(unsigned i=0;i<SC64_Z7_V36_UNIT_ROWS;++i) {
        Unit old;
        if (!sc64_z7_v36_probe_unit(meta,map,authority,i,&old) ||
            !sc64_derived_unit_encode_for(&out->units[i],&old,2048,2048,0)) return 0;
    }
    for(unsigned i=0;i<SC64_Z7_V36_BUILDING_ROWS;++i) {
        Building old;
        if (!sc64_z7_v36_probe_building(meta,map,authority,i,&old) ||
            !sc64_derived_building_encode(&out->buildings[i],&old,2048,2048)) return 0;
    }
    if (!sc64_z7_v36_row_context(meta,map,authority)) return 0;
    out->commitments=commitments;out->valid=1;return 1;
}
#endif
