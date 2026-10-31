/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private, default-off Yamato experiment. Flight/impact are not wired yet. */
#ifndef SC64_BATTLECRUISER_YAMATO_H
#define SC64_BATTLECRUISER_YAMATO_H
#include "sc64_bc_range_private.h"
static int race_bc_yamato_target_valid(int target, const Unit *caster, int require_visible)
{
    if (!caster || target<0 || target>=MAX_UNITS+MAX_BUILDINGS || !target_alive(target)) return 0;
    if (target<MAX_UNITS) {
        if (SC64_TRANSPORT_LOADED(&u[target]) || (u[target].flags&16)) return 0;
#ifdef SC64_T4_RUNTIME
        if (sc64_race_t4_unit_intrinsically_invincible(u[target].type)) return 0;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
        if (invincible_units[target]) return 0;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (sc64_game_unit_actor_flags(u[target].type)&SC64_GAME_ACTOR_INVINCIBLE) return 0;
#endif
    } else {
        int slot=target-MAX_UNITS;
        if (b[slot].flags&16) return 0;
#ifdef SC64_T4_RUNTIME
        if (sc64_race_t4_building_intrinsically_invincible(b[slot].type)) return 0;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
        if (invincible_buildings[slot]) return 0;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (sc64_game_building_actor_flags(b[slot].type)&SC64_GAME_ACTOR_INVINCIBLE) return 0;
#endif
    }
#ifdef SC64_P6_COMBAT_ENABLED
    if (require_visible && race_p6_active() && (!race_p6_combat_target_detected(target,caster->owner) ||
        (target<MAX_UNITS && race_p6_combat_unit_burrowed(target)))) return 0;
#endif
#ifdef SC64_P7_OBSERVER_ENABLED
    if (require_visible && !sc64_p7_observer_can_attack_from(caster->type,target,caster->owner,caster->raw_owner)) return 0;
#endif
    if (require_visible && caster->owner==SIDE && !currently_visible(target_x(target),target_y(target))) return 0;
    return 1;
}
static int race_bc_yamato_issue(int slot,int target)
{
    Unit *q;
    int key,index;
    if (slot<0 || slot>=MAX_UNITS) return 0;
    q=&u[slot];
    if (!q->alive || sc64_race_raw_unit(q->type)!=12 || SC64_TRANSPORT_LOADED(q) ||
        (q->work_timer&7) || q->carry<150*256 || !race_bc_yamato_target_valid(target,q,1)) return 0;
    key=sc64_battlecruiser_owner_key(race_mode,race_selected_mission,q->owner,q->raw_owner);
    if (key<0 || !sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,
                                                (unsigned)key,SC64_BC_TECH_YAMATO)) return 0;
    index=sc64_bc_shot_reserve(sc64_bc_shots,(unsigned)slot,(unsigned)target);
    if (index<0) return 0;
    sc64_bc_shots[index].x_q8=q->x*256;
    sc64_bc_shots[index].y_q8=q->y*256;
    sc64_bc_shots[index].phase=(unsigned char)((unsigned)q->owner | ((unsigned)key<<3));
    q->order=ORDER_ATTACK_TARGET; q->target=target;
    q->tx=target_x(target); q->ty=target_y(target);
    q->path_timer=q->path_retry=0;
    return 1;
}
#if defined(SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT) && !defined(SC64_CV5_COVER)
#error "automatic Yamato requires authenticated cover assets"
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
static int race_bc_yamato_launch_world(int index,unsigned heading);
#endif
/* Persistent source heading follows the current target/waypoint once per
 * source tick. GBA path following and braking remain engine approximations. */
static void __attribute__((noinline)) race_bc_yamato_heading_tick(Unit *q)
{
    int dx=0,dy=0;uint8_t next,heading=(uint8_t)((unsigned)q->work_timer>>3);
    if (q->order==ORDER_ATTACK_TARGET && q->target>=0 &&
        q->target<MAX_UNITS+MAX_BUILDINGS && target_alive(q->target)) {
        dx=(target_x(q->target)-q->x)*2;dy=(target_y(q->target)-q->y)*2;
    } else if (q->order==ORDER_MOVE || q->order==ORDER_ATTACK_MOVE) {
        dx=(q->tx-q->x)*2;dy=(q->ty-q->y)*2;
    }
    if (sc64_bc_heading_toward(heading,dx,dy,&next))
        q->work_timer=((int)next<<3)|(q->work_timer&7);
}
/* Returns true while the cast owns this actor's ordinary order update. */
static int __attribute__((noinline)) race_bc_yamato_tick_caster(int slot)
{
    Unit *q=&u[slot];
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i) {
        Sc64BcYamatoShot *shot=&sc64_bc_shots[i];
        if (!shot->state || shot->state==SC64_BC_SHOT_FLIGHT || shot->caster!=(unsigned)slot) continue;
        if (!q->alive || sc64_race_raw_unit(q->type)!=12 || q->owner!=(shot->phase&7) ||
            q->order!=ORDER_ATTACK_TARGET ||
            q->target!=(shot->target==SC64_BC_SHOT_NO_TARGET?-1:(int)shot->target) ||
            (shot->state==SC64_BC_SHOT_APPROACH &&
             !race_bc_yamato_target_valid(shot->target,q,1))) {
            sc64_bc_shot_clear(shot); return 0;
        }
        if (shot->state==SC64_BC_SHOT_APPROACH) {
            int raw;
            if (shot->target<MAX_UNITS) {
                Unit *target=&u[shot->target];
                raw=target->raw_type>=0?target->raw_type:sc64_race_raw_unit(target->type);
            } else {
                Building *target=&b[shot->target-MAX_UNITS];
                raw=target->raw_type>=0?target->raw_type:sc64_race_raw_building(target->type);
            }
            int distance=raw<0?-1:sc64_bc_source_range_distance(12u,q->x,q->y,(unsigned)raw,
                                                                 target_x(shot->target),target_y(shot->target));
            if (distance<0) { sc64_bc_shot_clear(shot);q->order=ORDER_IDLE;q->target=-1;return 1; }
            if (distance>320) { move_toward(q,target_x(shot->target),target_y(shot->target)); return 1; }
            if (q->build>=0 || !sc64_battlecruiser_source_tick_due()) return 1;
            int dx=(target_x(shot->target)-q->x)*2,dy=(target_y(shot->target)-q->y)*2;
            if (dx || dy) {
                unsigned desired=sc64_yamato_direction(dx,dy);
                int delta=sc64_yamato_turn_delta((u8)((unsigned)q->work_timer>>3),(u8)desired);
                if (delta< -16 || delta>16) return 1;
            }
            if (q->carry<150*256) { sc64_bc_shot_clear(shot); q->order=ORDER_IDLE; q->target=-1; return 1; }
            q->carry-=150*256;
            /* Source unmodified weapon cooldown 15 plus random 0..3 minus 1.
             * build stores cooldown minus one, reserving -1 for ready. */
            q->build=13+(int)(sc64_bc_spell_random()&3u);
            shot->state=SC64_BC_SHOT_CHARGING; shot->age=0;
            return 1;
        }
        if (sc64_battlecruiser_source_tick_due() && shot->age<SC64_BC_YAMATO_RELEASE_SOURCE_TICK)
            ++shot->age;
        if (shot->age==SC64_BC_YAMATO_RELEASE_SOURCE_TICK && shot->target==SC64_BC_SHOT_NO_TARGET) {
            q->work_timer=(q->work_timer&~7)|6; sc64_bc_shot_clear(shot); return 1;
        }
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
        if (shot->age==SC64_BC_YAMATO_RELEASE_SOURCE_TICK) {
            if (!race_bc_yamato_launch_world(i,(unsigned)q->work_timer>>3)) {
                q->work_timer=(q->work_timer&~7)|6; sc64_bc_shot_clear(shot);
            }
            return 1;
        }
#endif
        /* Without the complete world context, leave release pending. */
        return 1;
    }
    return 0;
}
#endif
