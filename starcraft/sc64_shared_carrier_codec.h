/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHARED_CARRIER_CODEC_H
#define SC64_SHARED_CARRIER_CODEC_H

#if defined(SC64_SHARED_CARRIER_EXPERIMENT) && !defined(SC64_P7_PROJECTILES_ENABLED)
/* Candidate-only codec. These helpers neither publish a save nor modify the
 * live world. The caller supplies bounded scratch for decoded actor rows. */
static int sc64_shared_carrier_overlap(const void *a, size_t an,
                                       const void *b, size_t bn)
{
    uintptr_t ap=(uintptr_t)a, bp=(uintptr_t)b;
    if (!a || !b || an>UINTPTR_MAX-ap || bn>UINTPTR_MAX-bp) return 1;
    return ap<bp+bn && bp<ap+an;
}

/* Relocate a checksummed predecessor envelope in scratch only. The legacy
 * actor arrays start at the same offset, but a larger custom union may move
 * every subsequent field and any variable-length sidecar. This does not
 * validate or commit the candidate and must run only after tag/CRC checks. */
static int sc64_shared_carrier_expand_legacy(void *buffer, size_t length,
                                             size_t capacity, size_t *expanded)
{
    const size_t old_tail=offsetof(RaceSave,buildings)+
        sizeof(((RaceSave *)0)->buildings);
    const size_t new_tail=offsetof(RaceSave,minerals_map);
    const size_t prefix=sizeof(((RaceSave *)0)->minerals_map)+sizeof(((RaceSave *)0)->fog);
    const size_t old_state=(old_tail+prefix+3u)&~(size_t)3u;
    const size_t new_state=offsetof(RaceSave,state);
    size_t delta;
    if (!buffer || !expanded || (old_tail&3u) || old_tail>new_tail ||
        offsetof(RaceSave,fog)!=new_tail+sizeof(((RaceSave *)0)->minerals_map) ||
        new_state<new_tail+prefix || old_state>new_state || length<old_state ||
        length>capacity || sc64_shared_carrier_overlap(buffer,capacity,expanded,sizeof(*expanded))) return 0;
    delta=new_state-old_state;
    if (delta>capacity-length) return 0;
    for (size_t i=old_tail+prefix;i<old_state;++i)
        if (((const u8 *)buffer)[i]) return 0;
    memmove((u8 *)buffer+new_state,(u8 *)buffer+old_state,length-old_state);
    memmove((u8 *)buffer+new_tail,(u8 *)buffer+old_tail,prefix);
    memset((u8 *)buffer+old_tail,0,new_tail-old_tail);
    memset((u8 *)buffer+new_tail+prefix,0,new_state-new_tail-prefix);
    *expanded=length+delta;
    return 1;
}
/* The inverse is used only after a complete legacy-format candidate and its
 * variable sidecars have been constructed in the expanded scratch layout. */
static int sc64_shared_carrier_compact_legacy(void *buffer, size_t length,
                                              size_t capacity, size_t *compacted)
{
    const size_t old_tail=offsetof(RaceSave,buildings)+sizeof(((RaceSave *)0)->buildings);
    const size_t new_tail=offsetof(RaceSave,minerals_map);
    const size_t prefix=sizeof(((RaceSave *)0)->minerals_map)+sizeof(((RaceSave *)0)->fog);
    const size_t old_state=(old_tail+prefix+3u)&~(size_t)3u;
    const size_t new_state=offsetof(RaceSave,state);
    if (!buffer || !compacted || (old_tail&3u) || old_tail>new_tail ||
        offsetof(RaceSave,fog)!=new_tail+sizeof(((RaceSave *)0)->minerals_map) ||
        new_state<new_tail+prefix || old_state>new_state || length<new_state ||
        length>capacity || sc64_shared_carrier_overlap(buffer,capacity,compacted,sizeof(*compacted))) return 0;
    memmove((u8 *)buffer+old_tail,(u8 *)buffer+new_tail,prefix);
    memset((u8 *)buffer+old_tail+prefix,0,old_state-old_tail-prefix);
    memmove((u8 *)buffer+old_state,(u8 *)buffer+new_state,length-new_state);
    *compacted=length-(new_state-old_state);
    return 1;
}
static int sc64_shared_carrier_candidate_encode(RaceSave *dst,
                                                const Unit *actors,
                                                const Building *buildings,
                                                const Sc64Arena *projectiles,
                                                unsigned ww, unsigned wh)
{
    if (!dst || !actors || !buildings || !projectiles || !projectiles->words ||
        projectiles->word_count<SC64_ARENA_WORDS ||
        sc64_shared_carrier_overlap(dst,sizeof(*dst),actors,sizeof(*actors)*MAX_UNITS) ||
        sc64_shared_carrier_overlap(dst,sizeof(*dst),buildings,sizeof(*buildings)*MAX_BUILDINGS) ||
        sc64_shared_carrier_overlap(dst,sizeof(*dst),(const void *)projectiles->words,
                                    SC64_ARENA_WORDS*sizeof(u16))) return 0;
    memset(&dst->units,0,offsetof(RaceSave,minerals_map)-offsetof(RaceSave,units));
    for (unsigned i=0;i<MAX_UNITS;++i)
        if (!sc64_p7_typed_unit_encode(&dst->carrier_custom.units[i],
                                       &actors[i],ww,wh)) return 0;
    for (unsigned i=0;i<MAX_BUILDINGS;++i)
        if (!sc64_p7_typed_building_encode(&dst->carrier_custom.buildings[i],
                                           &buildings[i],ww,wh)) return 0;
    return sc64_codec_encode(projectiles,dst->carrier_custom.projectiles,
                             SC64_CODEC_BYTES);
}

static int sc64_shared_carrier_candidate_valid(const RaceSave *src,
                                               Unit *scratch, unsigned count,
                                               unsigned ww, unsigned wh)
{
    const u8 *packet;
    signed char old_binding;
    int old_mission, valid, allocated;
    if (!src || !scratch || count!=MAX_UNITS || !ww || !wh ||
        ww>2048 || wh>2048 ||
        sc64_shared_carrier_overlap(src,sizeof(*src),scratch,sizeof(*scratch)*MAX_UNITS) ||
        sc64_shared_carrier_overlap(scratch,sizeof(*scratch)*MAX_UNITS,u,sizeof(u)) ||
        sc64_shared_carrier_overlap(scratch,sizeof(*scratch)*MAX_UNITS,b,sizeof(b)) ||
        sc64_shared_carrier_overlap(scratch,sizeof(*scratch)*MAX_UNITS,
            (const void *)race_p7_projectiles.words,SC64_ARENA_WORDS*sizeof(u16))) return 0;
    {
        const u8 *region=(const u8 *)&src->units;
        size_t span=offsetof(RaceSave,minerals_map)-offsetof(RaceSave,units);
        for (size_t i=sizeof(src->carrier_custom);i<span;++i)
            if (region[i]) return 0;
    }
    packet=src->carrier_custom.projectiles;
    if (!sc64_codec_validate(packet,SC64_CODEC_BYTES)) return 0;
    allocated=sc64_codec_allocated_count(packet,SC64_CODEC_BYTES);
    if (allocated<0) return 0;
    for (unsigned i=0;i<MAX_UNITS;++i)
        if (!sc64_p7_typed_unit_decode(&scratch[i],&src->carrier_custom.units[i],
                                       ww,wh)) return 0;
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building decoded;
        if (!sc64_p7_typed_building_decode(&decoded,
                    &src->carrier_custom.buildings[i],ww,wh)) return 0;
    }
    /* Candidate owners are generic even if the live world is a campaign. */
    old_binding=race_protoss_binding_override;
    old_mission=race_protoss_override_mission;
    race_protoss_bind_candidate(1,0);
    valid=race_p7_carrier_training_candidate_valid(scratch,MAX_UNITS);
    race_protoss_binding_override=old_binding;
    race_protoss_override_mission=old_mission;
    if (!valid) return 0;
    for (int i=0;i<allocated;++i) {
        Sc64ProjectileRow row;
        if (!sc64_codec_get_row(packet,SC64_CODEC_BYTES,(u8)i,&row) ||
            row.pending_damage_event) return 0;
        if (row.state==3) continue;
        s32 margin=row.state==0 ? 20*256 : 0;
        if (row.x_q8_source < -margin || row.y_q8_source < -margin ||
            row.x_q8_source >= (s32)(ww*512)+margin ||
            row.y_q8_source >= (s32)(wh*512)+margin ||
            row.turn_rate_raw<127 || row.turn_rate_raw>383 ||
            row.order_signal_bit0 || !sc64_interceptor_save_hit_flags_supported(row.hit_flags_low2) ||
            (row.movement_flags_low4&4u) || row.raw_source_owner>=8 ||
            race_protoss_generic_source_owner(row.raw_source_owner)<0 ||
            row.destination_x_source<0 || row.destination_x_source>=(int)(ww*2) ||
            row.destination_y_source<0 || row.destination_y_source>=(int)(wh*2)) return 0;
        if (row.state==0 && (row.script_phase!=1 || row.remaining_time!=255 ||
            row.next_speed || row.turn_rate_raw!=127 || row.movement_flags_low4!=8 ||
            row.heading!=row.next_velocity_direction)) return 0;
        if (row.state==1 && (row.script_phase!=2 || row.remaining_time<0 ||
            row.remaining_time>254)) return 0;
        if (row.state==2 && (row.script_phase!=1 || row.remaining_time>254)) return 0;
        if (row.owner_ref!=255) {
            const Unit *shooter;
            if (row.owner_ref>=MAX_UNITS) return 0;
            shooter=&scratch[row.owner_ref];
            if (!shooter->alive || shooter->type!=SC64_INTERCEPTOR_TYPE ||
                race_protoss_unit_raw(shooter)!=73 ||
                race_protoss_generic_slot(shooter->owner)!=row.raw_source_owner ||
                shooter->owner!=race_protoss_generic_source_owner(row.raw_source_owner)) return 0;
        }
        if (row.target_ref!=511) {
            if (row.target_ref<MAX_UNITS) {
                const Unit *target=&scratch[row.target_ref];
                if (!target->alive || SC64_TRANSPORT_LOADED(target)) return 0;
            } else {
                Building target;
                unsigned slot=row.target_ref-MAX_UNITS;
                if (slot>=MAX_BUILDINGS ||
                    !sc64_p7_typed_building_decode(&target,
                        &src->carrier_custom.buildings[slot],ww,wh) || !target.alive) return 0;
            }
        }
    }
    return 1;
}
#endif
#endif
