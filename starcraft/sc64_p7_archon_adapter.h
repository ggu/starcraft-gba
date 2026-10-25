#ifndef SC64_P7_ARCHON_ADAPTER_H
#define SC64_P7_ARCHON_ADAPTER_H

#ifdef SC64_P7_ARCHON_SPLASH
#include "sc64_p7_archon_rules.h"
#include "sc64_p7_geometry.h"

/* This Archon path does not implement native burrow detection/target rules.
 * SC64_CAN_ATTACK_FROM follows the current engine's visibility policy, which
 * can reject even an otherwise splash-eligible P6 burrowed actor. The P7 map
 * census has no burrowed actors; do not infer native burrow parity from this
 * adapter. */

static unsigned
race_p7_archon_xy_length(int dx,int dy)
{
    unsigned x=dx<0?0u-(unsigned)dx:(unsigned)dx;
    unsigned y=dy<0?0u-(unsigned)dy:(unsigned)dy;
    if (x<y) { unsigned t=x; x=y; y=t; }
    /* Pinned OpenBW xy_length integer approximation. */
    if (x/4u<y)
        x=x-x/16u+y*3u/8u-x/64u+y*3u/256u;
    return x;
}

static int
race_p7_archon_distance_to_actor(int raw,int x,int y,int building,
                                  int impact_x,int impact_y,unsigned *distance)
{
    int l,t,r,b,px=impact_x,py=impact_y;
    if (!distance || !sc64_p7_actor_rect_source(raw,x,y,building,&l,&t,&r,&b) ||
        l>=r || t>=b) return 0;
    /* Helper returns half-open pixel bounds. Native rect.to is inclusive. */
    if (impact_x<l) impact_x=l;
    else if (impact_x>=r) impact_x=r-1;
    if (impact_y<t) impact_y=t;
    else if (impact_y>=b) impact_y=b-1;
    *distance=race_p7_archon_xy_length(px-impact_x,py-impact_y);
    return 1;
}

static int
race_p7_archon_distance_between(int raw_a,int x_a,int y_a,int building_a,
                                 int raw_b,int x_b,int y_b,int building_b,
                                 unsigned *distance)
{
    int al,at,ar,ab,bl,bt,br,bb,dx=0,dy=0;
    if (!distance ||
        !sc64_p7_actor_rect_source(raw_a,x_a,y_a,building_a,&al,&at,&ar,&ab) ||
        !sc64_p7_actor_rect_source(raw_b,x_b,y_b,building_b,&bl,&bt,&br,&bb))
        return 0;
    /* Convert each half-open rectangle back to OpenBW's inclusive `to` edge,
     * then use its exact rect_difference axis rules before xy_length. */
    if (al>br-1) dx=al-(br-1);
    else if (bl>ar-1) dx=bl-(ar-1);
    if (at>bb-1) dy=at-(bb-1);
    else if (bt>ab-1) dy=bt-(ab-1);
    *distance=race_p7_archon_xy_length(dx,dy);
    return 1;
}

static int
race_p7_archon_damage_one(int target,unsigned packet_q8,int source_owner)
{
    Sc64P7ArchonDamageTarget model;
    Sc64P7ArchonDamageResult result;
    int hp,shield,armor,shield_damage,hp_damage,adjusted;
    if (target<0 || target>=MAX_UNITS+MAX_BUILDINGS) return 0;
    if (target<MAX_UNITS) {
        Unit *q=&u[target];
        if (!q->alive || invincible_units[target] || SC64_TRANSPORT_LOADED(q)) return 0;
        hp=q->hp; shield=q->shield;
        armor=q->armor+(q->owner==SIDE?armor_level:0);
    } else {
        int slot=target-MAX_UNITS;
        Building *q=&b[slot];
        if (!q->alive || invincible_buildings[slot]) return 0;
        hp=q->hp; shield=q->shield; armor=sc64_race_building_armor(q->type);
    }
    if (hp<0 || shield<0 || armor<0) return 0;
    model.hp_q8=(u32)hp*256u;
    model.shields_q8=(u32)shield*256u;
    model.armor_q8=(u32)armor*256u;
    model.has_shields=(u8)(shield>0);
    model.invincible=0;
    if (!sc64_p7_archon_normal_damage(&model,packet_q8,&result)) return 0;
    shield_damage=(int)(result.shield_damage_q8>>8);
    hp_damage=(int)(result.hp_damage_q8>>8);
    /* The gameplay arrays store integer HP/shields. Follow the established
     * P6 adapter policy: truncate each Q8 channel, but preserve any nonzero
     * fractional HP hit as one integer point. `hit_target` then applies its
     * normal shield transfer, armor and death bookkeeping exactly once. */
    if (result.hp_damage_q8 && !hp_damage) hp_damage=1;
    if (hp_damage>hp) hp_damage=hp;
    if (!shield_damage && !hp_damage) return 0;
    adjusted=hp_damage ? shield_damage+hp_damage+armor : shield_damage;
    if (adjusted<1) adjusted=1;
    hit_target(target,adjusted,U_P7_ARCHON,source_owner);
    return 1;
}

/* P7 source players 0 and 4 can both map to one live ENEMY slot. Enemy_Splash
 * owner exclusion is about source-player identity when provenance is known;
 * for entities without source provenance, retain the runtime-owner fallback. */
static int
race_p7_archon_same_owner(int source_owner,int source_raw_owner,
                           int target_owner,int target_raw_owner)
{
    if (source_raw_owner>=0 && source_raw_owner<8 &&
        target_raw_owner>=0 && target_raw_owner<8)
        return source_raw_owner==target_raw_owner;
    return source_owner==target_owner;
}

static int
race_p7_archon_splash_attack(Unit *attacker,int primary)
{
    int primary_raw,primary_building;
    unsigned attack_distance;
    if (!attacker || attacker->type!=U_P7_ARCHON) return 0;
    if (primary<0 || primary>=MAX_UNITS+MAX_BUILDINGS || !target_alive(primary))
        return 1; /* Consume an invalid authored Archon shot; never direct-hit. */
    primary_building=primary>=MAX_UNITS;
    primary_raw=primary_building ?
        (b[primary-MAX_UNITS].raw_type>=0?b[primary-MAX_UNITS].raw_type:
         sc64_race_raw_building(b[primary-MAX_UNITS].type)) :
        (u[primary].raw_type>=0?u[primary].raw_type:sc64_race_raw_unit(u[primary].type));
    if (primary_raw<0 || !race_p7_archon_distance_between(
            68,attacker->x,attacker->y,0,primary_raw,target_x(primary),
            target_y(primary),primary_building,&attack_distance)) return 1;
    if (attack_distance>SC64_P7_ARCHON_RANGE_SOURCE_PX) {
        move_toward(attacker,target_x(primary),target_y(primary));
        return 1;
    }
    if (attacker->cd) return 1;
    int impact_x=target_x(primary)*2,impact_y=target_y(primary)*2;
    unsigned upgrade=attacker->owner==SIDE && weapons_level>0 ?
                     (unsigned)weapons_level : 0u;
    unsigned slots=0;
    for (int i=0;i<MAX_UNITS;++i) {
        Unit *candidate=&u[i];
        int raw,idx=i;
        unsigned distance;
        Sc64P7ArchonPacket packet;
        if (!candidate->alive || candidate==attacker ||
            SC64_TRANSPORT_LOADED(candidate)) continue;
        raw=candidate->raw_type>=0?candidate->raw_type:
            sc64_race_raw_unit(candidate->type);
        if (raw<0 || !race_p7_archon_distance_to_actor(raw,candidate->x,
                    candidate->y,0,impact_x,impact_y,&distance) ||
            distance>SC64_P7_ARCHON_OUTER_SOURCE_PX) continue;
        if (idx!=primary && race_p7_archon_same_owner(attacker->owner,
                attacker->raw_owner,candidate->owner,candidate->raw_owner))
            continue;
        if (!SC64_CAN_ATTACK_FROM(-1,idx,attacker->owner)) continue;
        if (!sc64_p7_archon_packet((unsigned)upgrade,distance,&packet) ||
            !packet.tier) continue;
        if (race_p7_archon_damage_one(idx,packet.packet_damage_q8,
                                      attacker->owner)) ++slots;
    }
    for (int i=0;i<MAX_BUILDINGS;++i) {
        Building *candidate=&b[i];
        int idx=MAX_UNITS+i;
        int raw;
        unsigned distance;
        Sc64P7ArchonPacket packet;
        if (!candidate->alive) continue;
        raw=candidate->raw_type>=0?candidate->raw_type:
            sc64_race_raw_building(candidate->type);
        if (raw<0 || !race_p7_archon_distance_to_actor(raw,candidate->x,
                    candidate->y,1,impact_x,impact_y,&distance) ||
            distance>SC64_P7_ARCHON_OUTER_SOURCE_PX) continue;
        if (idx!=primary && race_p7_archon_same_owner(attacker->owner,
                attacker->raw_owner,candidate->owner,candidate->raw_owner))
            continue;
        if (!SC64_CAN_ATTACK_FROM(-1,idx,attacker->owner)) continue;
        if (!sc64_p7_archon_packet((unsigned)upgrade,distance,&packet) ||
            !packet.tier) continue;
        if (race_p7_archon_damage_one(idx,packet.packet_damage_q8,
                                      attacker->owner)) ++slots;
    }
    if (slots) {
        int color=sc64_weapon_vfx_unit_profile(attacker->type,0);
        add_tracer(attacker->x,attacker->y,target_x(primary),target_y(primary),color);
    }
    attacker->cd=SC64_P7_ARCHON_COOLDOWN_GBA_TICKS;
    return 1;
}
#endif /* SC64_P7_ARCHON_SPLASH */

#endif /* SC64_P7_ARCHON_ADAPTER_H */
