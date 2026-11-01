/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_PROJECTILE_SAVE_ADAPTER_H
#define SC64_P7_PROJECTILE_SAVE_ADAPTER_H
#ifdef SC64_P7_PROJECTILES_ENABLED
/* Candidate-only validation. No packet is decoded into live VRAM until the
 * complete save, actor links and mission state have passed preflight. */
static int race_p7_projectile_save_candidate_valid(const RaceSave *candidate,
                                                    unsigned ww,unsigned wh)
{
    const u8 *packet;
    int count;
    if (!candidate || candidate->interceptor_padding ||
        !ww || !wh || ww>2048 || wh>2048) return 0;
    packet=candidate->interceptor_projectiles;
    if (!sc64_codec_validate_target(&race_p7_projectiles,packet,SC64_CODEC_BYTES)) return 0;
    count=sc64_codec_allocated_count(packet,SC64_CODEC_BYTES);
    if (count<0) return 0;
    /* Load preflight binds the candidate's mode (race_load_game). */
    int p7=race_protoss_services_active();
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit actor;
        if (!race_save_decode_unit(candidate,i,ww,wh,&actor)) return 0;
        if ((actor.type==U_P7_INTERCEPTOR)!=(RACE_PROTOSS_RAW(&actor)==73)) return 0;
        if (actor.alive && actor.type==U_P7_INTERCEPTOR &&
            (!p7 || RACE_PROTOSS_SLOT(&actor)<0 || RACE_PROTOSS_SLOT(&actor)>=8 ||
             actor.owner!=race_protoss_source_owner(RACE_PROTOSS_SLOT(&actor)))) return 0;
    }
    if (!p7)
        return count==0;
    for (int i=0;i<count;++i) {
        Sc64ProjectileRow row;
        if (!sc64_codec_get_row(packet,SC64_CODEC_BYTES,(u8)i,&row) ||
            row.pending_damage_event) return 0;
        if (row.state==3) continue;
        /* A launch muzzle can be 20 source pixels outside the map. Moving
         * and dying rows have already passed native map-edge clamping. */
        s32 margin=row.state==0?20*256:0;
        if (row.x_q8_source < -margin || row.y_q8_source < -margin ||
            row.x_q8_source>=(s32)(ww*512)+margin ||
            row.y_q8_source>=(s32)(wh*512)+margin) return 0;
        /* Production constructor/tick states, rather than only bit widths.
         * Projectile positions may initially include an off-map muzzle. */
        if (row.turn_rate_raw<127 || row.turn_rate_raw>383 ||
            row.order_signal_bit0 || row.hit_flags_low2 ||
            (row.movement_flags_low4&4u) || row.raw_source_owner>=8 ||
            race_protoss_source_owner(row.raw_source_owner)<0 ||
            row.destination_x_source<0 || row.destination_x_source>=(int)(ww*2) ||
            row.destination_y_source<0 || row.destination_y_source>=(int)(wh*2)) return 0;
        if (row.state==0 && (row.script_phase!=1 || row.remaining_time!=255 ||
            row.next_speed || row.turn_rate_raw!=127 || row.movement_flags_low4!=8 ||
            row.heading!=row.next_velocity_direction)) return 0;
        if (row.state==1 && (row.script_phase!=2 || row.remaining_time<0 ||
            row.remaining_time>254)) return 0;
        if (row.state==2 && (row.script_phase!=1 || row.remaining_time>254)) return 0;
        if (row.owner_ref!=255) {
            Unit shooter;
            if (!race_save_decode_unit(candidate,row.owner_ref,ww,wh,&shooter) ||
                !shooter.alive || shooter.type!=U_P7_INTERCEPTOR ||
                RACE_PROTOSS_RAW(&shooter)!=73 || RACE_PROTOSS_SLOT(&shooter)!=row.raw_source_owner ||
                shooter.owner!=race_protoss_source_owner(row.raw_source_owner)) return 0;
        }
        if (row.target_ref!=511) {
            if (row.target_ref<MAX_UNITS) {
                Unit target;
                if (!race_save_decode_unit(candidate,row.target_ref,ww,wh,&target) ||
                    !target.alive || SC64_TRANSPORT_LOADED(&target)) return 0;
            } else {
                Building target;
                if (!race_save_decode_building(candidate,row.target_ref-MAX_UNITS,&target) ||
                    !target.alive) return 0;
            }
        }
    }
    return 1;
}
#endif
#endif
