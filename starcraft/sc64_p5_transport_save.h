#ifndef SC64_P5_TRANSPORT_SAVE_H
#define SC64_P5_TRANSPORT_SAVE_H
#include "sc64_construction_save.h"

#ifdef SC64_P5_SHUTTLE_ENABLED
#if !defined(SC64_P5_RUNTIME) || !defined(SC64_ZP_FULL_STORAGE)
#error "P5 Shuttle save validation requires P5 runtime and packed world saves"
#endif

#include "sc64_transport_rules.h"

static int race_p5_transport_passenger_provenance_valid(const Unit *q)
{
    if (!q || q->owner != SIDE) return 0;
    /* Tassadar's rescued identity stays source slot 2. All other supported
     * human passengers start in, or are produced by, source slot 7. */
    if (q->raw_type == 79) return q->raw_owner == 2;
    return q->raw_owner == 7 &&
        (q->raw_type == 64 || q->raw_type == 65 || q->raw_type == 66 ||
         q->raw_type == 67);
}

#ifdef SC64_P7_BATTERY_ENABLED
/* The shared P5 cargo validator also runs for later P7 campaign saves. Accept
 * P7's independent recharge order there after validating its Battery
 * identity. Power is transient: a Pylon can die after the actor's service
 * slot and before a pause/save; the next actor update cancels the order. */
static int race_p5_saved_p7_recharge_valid(const RaceSave *candidate,
                                           const Unit *actor)
{
    Building battery;
    const Sc64RaceUnitRule *rule;
    int target;
    if (!candidate || !actor || candidate->state[0] ||
        candidate->state[2] != SC64_P7_GLOBAL_ID || actor->order != ORDER_P7_RECHARGE ||
        actor->raw_type == 73 || actor->raw_type == 85 ||
        actor->raw_owner < 0 || actor->raw_owner >= 8 || actor->work_timer != 0 ||
        SC64_TRANSPORT_LOADED(actor)) return 0;
    rule=sc64_race_unit_rule(actor->type);
    if (!rule || rule->race != R_PROTOSS || !(rule->flags & SC_RACE_F_SHIELDS) ||
        actor->shield > rule->shields ||
        (actor->shield == rule->shields && actor->shield_remainder != 0) ||
        (sc64_game_unit_actor_flags(actor->type) &&
         !(sc64_game_unit_actor_flags(actor->type) & SC64_GAME_ACTOR_MOVABLE)))
        return 0;
    target=actor->target;
    if (target<0 || target>=MAX_BUILDINGS ||
        !race_save_decode_building(candidate,(unsigned)target,&battery)) return 0;
    return battery.alive && battery.type==B_P7_SHIELD_BATTERY &&
        battery.raw_type==172 && !battery.under_construction &&
        battery.owner==actor->owner &&
        battery.raw_owner==actor->raw_owner;
}
#endif

/* Validate P5 cargo links from a decoded candidate without touching the live
 * world. The ordinary save validator checks every other actor/world field. */
static int race_p5_transport_save_candidate_valid(const RaceSave *candidate,
                                                   int width, int height)
{
    u8 used[MAX_UNITS] = { 0 };
    if (!candidate || width < 1 || height < 1) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit actor;
        if (!race_save_decode_unit(candidate, (unsigned)i,
                                   (unsigned)width * 16u,
                                   (unsigned)height * 16u, &actor)) return 0;
        if (!actor.alive) continue;
#ifdef SC64_SHARED_BATTERY
        if (actor.order==ORDER_RECHARGE) {
            if (!race_battery_saved_recharge_valid(candidate,&actor)) return 0;
            continue;
        }
#endif
        if (actor.order < ORDER_IDLE ||
            (actor.order > ORDER_UNLOAD &&
             !SC64_PENDING_BUILD_ORDER(actor.order))) {
#ifdef SC64_P7_BATTERY_ENABLED
            if (actor.order==ORDER_P7_RECHARGE &&
                race_p5_saved_p7_recharge_valid(candidate,&actor)) continue;
#endif
            return 0;
        }
        if (SC64_PENDING_BUILD_ORDER(actor.order)) {
            if (!sc64_pending_build_save_identity_valid(candidate, actor.alive,
                    actor.owner, actor.type, actor.target, actor.tx, actor.ty,
                    width * 16, height * 16)) return 0;
            continue;
        }
        if (actor.order >= ORDER_BOARD &&
            (candidate->state[0] || candidate->state[2] != SC64_P5_GLOBAL_ID ||
             actor.owner != SIDE)) return 0;
        if (actor.order == ORDER_UNLOAD) {
            if (actor.type != U_P5_SHUTTLE || actor.raw_type != 69 ||
                actor.raw_owner != 7 ||
                actor.target != -1 || actor.work_timer < 0 ||
                actor.work_timer > SC64_TRANSPORT_UNLOAD_TICKS ||
                actor.tx < 0 || actor.tx >= width * 16 ||
                actor.ty < 0 || actor.ty >= height * 16) return 0;
        }
        if (actor.order != ORDER_BOARD && actor.order != ORDER_TRANSPORTED) continue;
        if (actor.target < 0 || actor.target >= MAX_UNITS || actor.target == i ||
            (actor.flags & 2) || sc64_race_unit_flying(actor.type) ||
            !race_p5_transport_passenger_provenance_valid(&actor)) return 0;
        Unit carrier;
        if (!race_save_decode_unit(candidate, (unsigned)actor.target,
                                   (unsigned)width * 16u,
                                   (unsigned)height * 16u, &carrier)) return 0;
        if (!carrier.alive || carrier.type != U_P5_SHUTTLE ||
            carrier.raw_type != 69 || carrier.raw_owner != 7 ||
            carrier.owner != actor.owner || carrier.owner != SIDE ||
            carrier.order == ORDER_BOARD || carrier.order == ORDER_TRANSPORTED ||
            actor.raw_type < 0 ||
            sc64_race_unit_from_raw(actor.raw_type) != actor.type) return 0;
        unsigned space;
        if (!sc64_transport_space_required_raw((unsigned)actor.raw_type, &space) ||
            space == SC64_TRANSPORT_SPACE_UNAVAILABLE) return 0;
        if (actor.order == ORDER_TRANSPORTED) {
            if (actor.selected || actor.x != carrier.x || actor.y != carrier.y ||
                actor.path_timer || actor.path_retry ||
                (unsigned)used[actor.target] + space > SC64_TRANSPORT_SHUTTLE_SPACE)
                return 0;
            used[actor.target] = (u8)(used[actor.target] + space);
        }
    }
    return 1;
}
#endif

#endif
