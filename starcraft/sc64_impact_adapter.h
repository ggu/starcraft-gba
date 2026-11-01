/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_IMPACT_ADAPTER_H
#define SC64_IMPACT_ADAPTER_H

/* Game binding for pending projectile impacts.
 *
 * Timing reuses the imported N64 weapons/flingy/IScript bank that already
 * draws the projectile: attack release, projectile startup, distance-derived
 * flight and the Death script's domissiledmg tick. Only moving bullet types
 * (fly, follow-target, bounce) are queued; appear-at-target, source and
 * self-destruct weapons, and every attack without a source profile, keep the
 * immediate path. Resolution uses the ordinary hit path, so shields,
 * invincibility, armor and death cleanup are evaluated at impact. The packet
 * keeps its launch owner, type and damage; the attacker slot is not kept, so
 * an attacker that dies after launch still delivers its projectile.
 * See docs/IMPACT-EVENTS.md for provenance and limits. */

#ifdef SC64_SCARAB_WEAPON_PROBE
#include "sc64_scarab_impact_packet.h"
#endif
static Sc64ImpactEvent race_impact_rows[RACE_IMPACT_CAPACITY];
static u8 race_impact_count;
/* Diagnostics only; neither is saved and neither affects simulation. */
static u8 race_impact_high_water;
static u16 race_impact_deferred;

static void
race_impact_reset(void)
{
    race_impact_count = 0;
    race_impact_high_water = 0;
    race_impact_deferred = 0;
}

static void
race_impact_clear_target(int ref)
{
#ifdef SC64_SCARAB_WEAPON_PROBE
    unsigned keep=0;
    for(unsigned i=0;i<race_impact_count;i++) {
        Sc64ImpactEvent row=race_impact_rows[i];
        if(row.source_type==SC64_SCARAB_IMPACT_MARKER) {
            Sc64ScarabImpact packet;
            if(sc64_scarab_impact_unpack(&row,&packet) && packet.target==ref)
                sc64_scarab_impact_forget_target(&row);
        } else if(sc64_impact_event_target(&row)==ref) continue;
        race_impact_rows[keep++]=row;
    }
    race_impact_count=(u8)keep;
#else
    if (race_impact_count)
        (void)sc64_impact_clear_target(race_impact_rows, &race_impact_count, ref);
#endif
}

static unsigned
race_impact_death_damage_tick(const Sc64WeaponSourceAttackProfile *p)
{
    const Sc64WeaponSourceAnimation *death =
        sc64_weapon_source_animation_lookup(p->projectile_image, 1u);
    if (!death || death->event_first > SC64_WEAPON_SOURCE_EVENT_COUNT ||
        death->event_count > SC64_WEAPON_SOURCE_EVENT_COUNT - death->event_first)
        return 0;
    for (uint16_t i = 0; i < death->event_count; ++i) {
        const Sc64WeaponSourceEvent *event =
            &sc64_weapon_source_events[death->event_first + i];
        if (event->opcode == 27u) return event->source_tick; /* domissiledmg */
    }
    return 0;
}

/* End-of-update steps from launch to the impact frame, or zero for the
 * immediate path. The launching update's own step counts, so a projectile
 * whose impact is first drawn at presentation age k resolves after k + 1
 * steps, on that same update. */
static unsigned
race_impact_profile_steps(const Sc64WeaponSourceAttackProfile *p, int x0, int y0,
                          int x1, int y1)
{
    int launch_x, launch_y;
    uint32_t flight, source_tick, age;
    /* Single-target moving bullets only; attack-target-position and
     * extend-to-max-range bullets hit areas or lines the engine lacks. */
    if (!p || !p->projectile_image ||
        (p->weapon_bullet_type != SC64_SOURCE_BULLET_FLY &&
         p->weapon_bullet_type != SC64_SOURCE_BULLET_FOLLOW &&
         p->weapon_bullet_type != SC64_SOURCE_BULLET_BOUNCE)) return 0;
    sc64_weapon_source_trace_launch_origin(p,
        (uint8_t)sc64_source_effect_direction(x1 - x0, y1 - y0), x0, y0,
        &launch_x, &launch_y);
    flight = sc64_weapon_source_trace_flight_ticks(p, launch_x, launch_y, x1, y1);
    /* The renderer draws no trace for a distant projectile without motion. */
    if (!flight && sc64_weapon_source_trace_distance_raw(launch_x, launch_y, x1, y1))
        return 0;
    source_tick = sc64_weapon_source_trace_release_ticks(p) +
                  sc64_weapon_source_trace_startup_ticks(p) + flight +
                  race_impact_death_damage_tick(p);
    age = sc64_weapon_source_vfx_first_gba_age(source_tick);
    if (!age) return 0;
    /* Weapon ranges keep this far below the bound; saturating would only
     * be reached by an impossible multi-second flight. */
    return age >= SC64_IMPACT_MAX_STEPS ? SC64_IMPACT_MAX_STEPS : age + 1u;
}

/* Mirrors sc64_source_emit(): one or two source shots share the packet. */
static unsigned
race_impact_source_steps(int raw, int weapon, int air, int x0, int y0, int x1, int y1)
{
    unsigned variant, count, steps = 0;
    const Sc64WeaponSourceAttackProfile *profile;
    if (raw < 0 || raw >= 228 || weapon < 0 || weapon >= 130) return 0;
    variant = raw >= 106 ? 2u : (raw == 83 && weapon == 82) ? 1u : 0u;
    profile = sc64_weapon_source_profile_lookup((uint8_t)raw, (uint8_t)weapon,
                                                (uint8_t)air, (uint8_t)variant);
    if (!profile) return 0;
    count = profile->weapon_count == 2 ? 2u : 1u;
    for (unsigned shot = 0; shot < count; ++shot) {
        const Sc64WeaponSourceAttackProfile *p = shot == 0 ? profile :
            sc64_weapon_source_profile_lookup((uint8_t)raw, (uint8_t)weapon,
                (uint8_t)air, (uint8_t)(variant ? variant : shot));
        unsigned s = race_impact_profile_steps(p, x0, y0, x1, y1);
        if (s > steps) steps = s;
    }
    return steps;
}

static unsigned
race_impact_unit_steps(const Unit *q, int target, int air)
{
    int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type);
    Sc64StockAttack attack;
    int weapon;
    if (sc64_stock_attack_select(raw, air, &attack)) weapon = attack.weapon_id;
    else if (raw == 83 && !air) weapon = 82; /* Existing Reaver/Scarab adapter. */
    else return 0;
    return race_impact_source_steps(raw, weapon, air, q->x, q->y,
                                    target_x(target), target_y(target));
}

static unsigned
race_impact_building_steps(const Building *q, int target, int air)
{
    int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
    Sc64StockAttack attack;
    if (!sc64_stock_attack_select(raw, air, &attack)) return 0;
    return race_impact_source_steps(raw, attack.weapon_id, air, q->x, q->y,
                                    target_x(target), target_y(target));
}

static int
race_impact_full(void)
{
    if (race_impact_count < RACE_IMPACT_CAPACITY) return 0;
    if (race_impact_deferred != 0xffffu) ++race_impact_deferred;
    return 1;
}

/* Queue one weapon packet. Callers check race_impact_full() before any
 * launch side effect; the asserts below make the unrepresentable cases
 * unreachable for engine actors and stock weapons. */
static void
race_impact_launch(int target, int damage, int hits, int owner,
                   int damage_class, int source_type, unsigned steps)
{
    Sc64ImpactHit hit = { target, damage, hits, owner, damage_class, source_type };
    Sc64ImpactPushResult r = sc64_impact_push(race_impact_rows, RACE_IMPACT_CAPACITY,
                                              &race_impact_count, &hit, steps);
    if (r == SC64_IMPACT_QUEUED) {
        if (race_impact_count > race_impact_high_water)
            race_impact_high_water = race_impact_count;
        return;
    }
    /* Unreachable after race_impact_full(); never lose the packet. */
    for (int i = 0; i < hits && target_alive(target); ++i)
        hit_target_impl(target, damage, source_type, owner, damage_class);
}

static void
race_impact_resolve(const Sc64ImpactEvent *e)
{
    Sc64ImpactHit hit;
    sc64_impact_unpack(e, &hit);
    for (int i = 0; i < hit.hits && target_alive(hit.target); ++i)
        hit_target_impl(hit.target, hit.damage, hit.source_type, hit.owner,
                        hit.damage_class);
}

/* A target that dies or stops being targetable (loaded, hidden, burrowed)
 * loses its pending impacts for good, even if it reappears before arrival,
 * as the PC reference clears bullet targets when a unit is killed or hidden.
 * Survivors then advance one step; due packets resolve in launch order. */
static void __attribute__((noinline))
race_impact_tick(void)
{
#ifdef SC64_SCARAB_WEAPON_PROBE
    /* Advance all rows before resolving, then leave due rows in the shared
     * queue until taken. Death/slot cleanup can invalidate a later due
     * Scarab's explicit target without discarding its position splash. */
    unsigned keep=0;
    if(!race_impact_count)return;
    for(unsigned i=0;i<race_impact_count;i++) {
        Sc64ImpactEvent row=race_impact_rows[i];
        if(row.source_type==SC64_SCARAB_IMPACT_MARKER) {
            Sc64ScarabImpact packet;
            if(!sc64_scarab_impact_unpack(&row,&packet))continue;
            if(packet.target>=0 && !target_alive(packet.target)) sc64_scarab_impact_forget_target(&row);
            if(sc64_battlecruiser_source_tick_due() && packet.ticks)
                row.steps=(u8)(row.steps-16u);
        } else {
            if(!target_alive(sc64_impact_event_target(&row)))continue;
            if(row.steps)--row.steps;
        }
        race_impact_rows[keep++]=row;
    }
    race_impact_count=(u8)keep;
    for(;;) {
        unsigned i;
        for(i=0;i<race_impact_count;i++) {
            const Sc64ImpactEvent *row=&race_impact_rows[i];
            if(row->source_type==SC64_SCARAB_IMPACT_MARKER ? !(row->steps&0x70u) : !row->steps)break;
        }
        if(i==race_impact_count)break;
        Sc64ImpactEvent due=race_impact_rows[i];
        for(unsigned j=i+1;j<race_impact_count;j++)race_impact_rows[j-1]=race_impact_rows[j];
        --race_impact_count;
        if(due.source_type==SC64_SCARAB_IMPACT_MARKER)race_scarab_probe_bullet_resolve(&due);
        else race_impact_resolve(&due);
    }
#else
    Sc64ImpactEvent due[RACE_IMPACT_CAPACITY];
    unsigned ready, keep = 0;
    if (!race_impact_count) return;
    for (unsigned i = 0; i < race_impact_count; ++i)
        if (target_alive(sc64_impact_event_target(&race_impact_rows[i])))
            race_impact_rows[keep++] = race_impact_rows[i];
    race_impact_count = (u8)keep;
    ready = sc64_impact_step(race_impact_rows, &race_impact_count, due);
    for (unsigned i = 0; i < ready; ++i) race_impact_resolve(&due[i]);
#endif
}

/* Save: an empty queue keeps the historical RaceSave-length record. More
 * pending impacts than this profile's record holds refuse the save before
 * any SRAM write; they land within a second of play. */
static int
#if defined(SC64_SCARAB_NAMESPACE_STAGE) || defined(SC64_WORKER_CARGO_KIND)
race_impact_save_write(const GbaSaveSelection *previous)
#else
race_impact_save_write(void)
#endif
{
    u32 bytes = 0;
    if (race_impact_count > RACE_IMPACT_SAVE_CAPACITY) return 0;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    /* The fixed base stays legacy-sized. Only active Yamato rows consume
     * extension bytes; mask bits are in the already-versioned state word. */
    Sc64BcYamatoShot shots[SC64_BC_SHOT_LIMIT];
    u32 base = (u32)sizeof(RaceSave) - SC64_BC_SAVE_EXTRA_BYTES;
    u32 mask = 0, offset = base;
    memcpy(shots, race_data.battlecruiser_shots, sizeof(shots));
    for (unsigned i = 0; i < SC64_BC_SHOT_LIMIT; ++i)
        if (shots[i].state) { mask |= 1u << i; offset += sizeof(shots[i]); }
    if (race_impact_count) bytes = sc64_impact_save_bytes(race_impact_count);
    if (offset + bytes + SC64_TRANSPORT_TRAILER_BYTES > RACE_IMPACT_RECORD_MAX) return 0;
    race_data.state[28] = (race_data.state[28] & ~24u) | (mask << 3);
    offset = base;
    for (unsigned i = 0; i < SC64_BC_SHOT_LIMIT; ++i)
        if (mask & (1u << i)) {
            memcpy((u8 *)&race_impact_record + offset, &shots[i], sizeof(shots[i]));
            offset += sizeof(shots[i]);
        }
    if (race_impact_count)
        sc64_impact_encode(race_impact_rows, race_impact_count,
                           (u8 *)&race_impact_record + offset);
#if defined(SC64_SHARED_TRANSPORT) && defined(SC64_WORKER_CARGO_KIND)
    const u32 tags[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION};
    sc64_sk_transport_encode((u8 *)&race_impact_record + offset + bytes,
                             &race_skirmish_transport_candidate);
    if (previous) {
        if (!save_write_selected(&race_impact_record,offset+bytes+SC64_TRANSPORT_TRAILER_BYTES,
                                 SC64_TRANSPORT_SAVE_VERSION,previous,tags,2)) return 0;
    } else if (!save_write_first(&race_impact_record,offset+bytes+SC64_TRANSPORT_TRAILER_BYTES,
                                  SC64_TRANSPORT_SAVE_VERSION)) return 0;
#elif defined(SC64_SHARED_TRANSPORT) && !defined(SC64_SCARAB_NAMESPACE_STAGE)
    sc64_sk_transport_encode((u8 *)&race_impact_record + offset + bytes,
                             &race_skirmish_transport_candidate);
    save_write(&race_impact_record, offset + bytes + SC64_TRANSPORT_TRAILER_BYTES,
               SC64_TRANSPORT_SAVE_VERSION);
#elif defined(SC64_WORKER_CARGO_KIND)
    const u32 tags[]={SC64_LEGACY_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,SAVE_VERSION};
    if (previous) {
        if (!save_write_selected(&race_impact_record,offset+bytes,SAVE_VERSION,previous,tags,3)) return 0;
    } else if (!save_write_first(&race_impact_record,offset+bytes,SAVE_VERSION)) return 0;
#elif defined(SC64_SCARAB_NAMESPACE_STAGE)
#ifdef SC64_SHARED_TRANSPORT
    const u32 tags[]={SC64_SCARAB_TRANSPORT_SAVE_VERSION};
    sc64_sk_transport_encode((u8 *)&race_impact_record + offset + bytes,
                             &race_skirmish_transport_candidate);
    if (previous) {
        if (!save_write_selected(&race_impact_record,offset+bytes+SC64_TRANSPORT_TRAILER_BYTES,
                                 SC64_SCARAB_TRANSPORT_SAVE_VERSION,previous,tags,1)) return 0;
    } else if (!save_write_first(&race_impact_record,offset+bytes+SC64_TRANSPORT_TRAILER_BYTES,
                                  SC64_SCARAB_TRANSPORT_SAVE_VERSION)) return 0;
#else
    const u32 tags[]={SC64_LEGACY_SAVE_VERSION,SAVE_VERSION,SC64_SCARAB_SAVE_VERSION};
    if (previous) {
        if (!save_write_selected(&race_impact_record,offset+bytes,SC64_SCARAB_SAVE_VERSION,previous,tags,3)) return 0;
    } else if (!save_write_first(&race_impact_record,offset+bytes,SC64_SCARAB_SAVE_VERSION)) return 0;
#endif
#else
    save_write_migrating_feature(&race_impact_record, offset + bytes, SAVE_VERSION,
                         SC64_LEGACY_SAVE_VERSION, offsetof(RaceSave,state)+27u*sizeof(int),
                         0xffff0000u, 0x42450000u);
#endif
#else
    if (race_impact_count)
        bytes = sc64_impact_encode(race_impact_rows, race_impact_count,
                                   race_impact_record.impacts);
#ifdef SC64_SHARED_TRANSPORT
    if ((u32)sizeof(RaceSave) + bytes + SC64_TRANSPORT_TRAILER_BYTES > RACE_IMPACT_RECORD_MAX) return 0;
    sc64_sk_transport_encode((u8 *)&race_impact_record + sizeof(RaceSave) + bytes,
                             &race_skirmish_transport_candidate);
#ifdef SC64_WORKER_CARGO_KIND
    const u32 tags[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION};
    if (previous) {
        if (!save_write_selected(&race_impact_record,(u32)sizeof(RaceSave)+bytes+SC64_TRANSPORT_TRAILER_BYTES,
                                 SC64_TRANSPORT_SAVE_VERSION,previous,tags,2)) return 0;
    } else if (!save_write_first(&race_impact_record,(u32)sizeof(RaceSave)+bytes+SC64_TRANSPORT_TRAILER_BYTES,
                                  SC64_TRANSPORT_SAVE_VERSION)) return 0;
#else
    save_write(&race_impact_record, (u32)sizeof(RaceSave) + bytes + SC64_TRANSPORT_TRAILER_BYTES,
               SC64_TRANSPORT_SAVE_VERSION);
#endif
#elif defined(SC64_WORKER_CARGO_KIND)
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    const u32 tags[]={SC64_LEGACY_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,SAVE_VERSION};
#else
    const u32 tags[]={SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,SAVE_VERSION};
#endif
    if (previous) {
        if (!save_write_selected(&race_impact_record,(u32)sizeof(RaceSave)+bytes,SAVE_VERSION,previous,tags,sizeof(tags)/sizeof(tags[0]))) return 0;
    } else if (!save_write_first(&race_impact_record,(u32)sizeof(RaceSave)+bytes,SAVE_VERSION)) return 0;
#elif defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT)
    save_write_migrating_feature(&race_impact_record, (u32)sizeof(RaceSave) + bytes,
                         SAVE_VERSION, SC64_LEGACY_SAVE_VERSION,
                         offsetof(RaceSave,state)+27u*sizeof(int), 0xffff0000u, 0x42450000u);
#else
    save_write(&race_impact_record, (u32)sizeof(RaceSave) + bytes, SAVE_VERSION);
#endif
#endif
    return 1;
}

/* Decode an already-selected raw envelope in race_impact_record.  This may
 * rearrange that scratch record, but never reads SRAM or changes live state. */
static int
race_impact_save_decode_selected(u32 len, u32 selected_tag,
                                 Sc64ImpactEvent *candidate, int *count)
{
    int rows;
    *count = 0;
    if (len > (u32)sizeof(race_impact_record)) return 0;
#ifdef SC64_SHARED_TRANSPORT
#if defined(SC64_WORKER_CARGO_KIND)
    if ((selected_tag != SC64_TRANSPORT_SAVE_VERSION && selected_tag != SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION) || len < SC64_TRANSPORT_TRAILER_BYTES) return 0;
#elif defined(SC64_SCARAB_NAMESPACE_STAGE)
    if (selected_tag != SC64_SCARAB_TRANSPORT_SAVE_VERSION || len < SC64_TRANSPORT_TRAILER_BYTES) return 0;
#else
    if (selected_tag != SC64_TRANSPORT_SAVE_VERSION || len < SC64_TRANSPORT_TRAILER_BYTES) return 0;
#endif
    len -= SC64_TRANSPORT_TRAILER_BYTES;
    if (!sc64_sk_transport_decode(&race_skirmish_transport_candidate,
            (const u8 *)&race_impact_record + len, MAX_BUILDINGS)) return 0;
#if defined(SC64_WORKER_CARGO_KIND)
    selected_tag = selected_tag == SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION ?
        SC64_WORKER_CARGO_LEGACY_SAVE_VERSION : SAVE_VERSION;
#elif defined(SC64_SCARAB_NAMESPACE_STAGE)
    selected_tag = SC64_SCARAB_SAVE_VERSION;
#else
    selected_tag = SAVE_VERSION;
#endif
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    const u32 base = (u32)sizeof(RaceSave) - SC64_BC_SAVE_EXTRA_BYTES;
    Sc64BcYamatoShot shots[SC64_BC_SHOT_LIMIT] = {{0}};
    u32 offset = base, mask = 0, impact_bytes;
    int legacy = selected_tag == SC64_LEGACY_SAVE_VERSION;
    if (selected_tag != SAVE_VERSION && !legacy
#ifdef SC64_WORKER_CARGO_KIND
        && selected_tag != SC64_WORKER_CARGO_LEGACY_SAVE_VERSION
#endif
#ifdef SC64_SCARAB_NAMESPACE_STAGE
        && selected_tag != SC64_SCARAB_SAVE_VERSION
#endif
       ) return 0;
    if (len < base) return 0;
    if (!legacy) {
        if (race_data.version != selected_tag ||
            ((unsigned)race_data.state[27] & 0xffff0000u) != 0x42450000u ||
            ((unsigned)race_data.state[28] & ~31u) != 0x42440000u ||
            ((unsigned)race_data.state[28] & 7u) > 4u) return 0;
        mask = ((unsigned)race_data.state[28] >> 3) & 3u;
        for (unsigned i = 0; i < SC64_BC_SHOT_LIMIT; ++i)
            if (mask & (1u << i)) {
                if (len - offset < sizeof(shots[i])) return 0;
                memcpy(&shots[i], (u8 *)&race_impact_record + offset, sizeof(shots[i]));
                if (!shots[i].state) return 0;
                offset += sizeof(shots[i]);
            }
    } else if (race_data.version != SC64_LEGACY_SAVE_VERSION || race_data.state[27] != 0) return 0;
    impact_bytes = len - offset;
    if (impact_bytes > sizeof(race_impact_record.impacts)) return 0;
    memmove(race_impact_record.impacts, (u8 *)&race_impact_record + offset, impact_bytes);
    memcpy(race_data.battlecruiser_shots, shots, sizeof(shots));
    if (!impact_bytes) return 1;
    rows = sc64_impact_decode(race_impact_record.impacts, impact_bytes,
                              RACE_IMPACT_SAVE_CAPACITY, MAX_UNITS + MAX_BUILDINGS, 5u,
                              SC_RU_COUNT, candidate);
#else
    (void)selected_tag; /* Version validation remains in race_load_game. */
    if (len < sizeof(RaceSave)) return 0;
    if (len == sizeof(RaceSave)) return 1;
    rows = sc64_impact_decode(race_impact_record.impacts, len - (u32)sizeof(RaceSave),
                              RACE_IMPACT_SAVE_CAPACITY, MAX_UNITS + MAX_BUILDINGS, 5u,
                              SC_RU_COUNT, candidate);
#endif
    if (rows < 0) return 0;
    *count = rows;
    return 1;
}

/* Preserve the existing per-tag selection policy until semantic journal
 * migration is registered.  The decoder above is shared with that future
 * candidate preflight and operates only on the selected scratch envelope. */
static int
race_impact_save_read(Sc64ImpactEvent *candidate, int *count)
{
#if defined(SC64_SHARED_TRANSPORT) && !defined(SC64_SCARAB_NAMESPACE_STAGE)
    u32 len = 0, tag = SC64_TRANSPORT_SAVE_VERSION;
#else
    u32 len = 0, tag = SAVE_VERSION;
#endif
    *count = 0;
    if (!save_read_latest(&race_impact_record, (u32)sizeof(race_impact_record), tag, &len)) {
#if defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT) && !defined(SC64_SHARED_TRANSPORT)
        tag = SC64_LEGACY_SAVE_VERSION;
        if (!save_read_latest(&race_impact_record, (u32)sizeof(race_impact_record), tag, &len)) return 0;
#else
        return 0;
#endif
    }
    return race_impact_save_decode_selected(len, tag, candidate, count);
}

static void
race_impact_restore(const Sc64ImpactEvent *candidate, int count)
{
    race_impact_reset();
    if (count > 0) memcpy(race_impact_rows, candidate, (size_t)count * sizeof(*candidate));
    race_impact_count = (u8)count;
    race_impact_high_water = (u8)count;
}

#endif
