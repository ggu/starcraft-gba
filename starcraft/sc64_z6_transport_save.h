#ifndef SC64_Z6_TRANSPORT_SAVE_H
#define SC64_Z6_TRANSPORT_SAVE_H
#include "sc64_construction_save.h"
#ifndef SC64_NYDUS_ORDER
/* Standalone fixtures include this header without game.c's order enum. */
#define SC64_NYDUS_ORDER(o) 0
#endif

#if defined(SC64_Z6_TRANSPORT_ENABLED)
#if !defined(SC64_Z6_RUNTIME) || !defined(SC64_TRANSPORT_RUNTIME) || \
    !defined(SC64_ZP_FULL_STORAGE)
#error "Z6 transport save validation requires Z6 and packed transport storage"
#endif

#include "sc64_transport_rules.h"
#include "sc64_z6_transport_tech.h"

/* Z6's Overlord and cargo are source slot 3 actors, live on the player's side
 * after import, and must retain their source owner when transported. */
static int race_z6_transport_passenger_provenance_valid(const Unit *q)
{
    if (!q || !q->alive || q->owner != SIDE || q->raw_owner != 3 ||
        q->raw_type < 0 || sc64_race_unit_from_raw(q->raw_type) != q->type)
        return 0;
    switch (q->raw_type) {
    case 37: /* Zergling */
    case 38: /* Hydralisk */
    case 39: /* Ultralisk */
    case 41: /* Drone */
    case 51: /* Kerrigan */
        return 1;
    default:
        return 0;
    }
}

/* Candidate-only validation. `tech` must be the decoded candidate research
 * state, never the live race_z6 state, so malformed SRAM cannot borrow a
 * currently researched upgrade. Capacity includes both pending BOARD links
 * and passengers already in TRANSPORTED state. */
static int race_z6_transport_save_candidate_valid(
    const RaceSave *candidate, const Sc64Z6TransportTechState *tech,
    int width, int height)
{
    u8 used[MAX_UNITS] = { 0 };
    int is_z6;
    if (!candidate || width < 1 || height < 1 || width > 128 || height > 128)
        return 0;
    is_z6 = candidate->state[2] == SC64_Z6_GLOBAL_ID;
    /* P5 and Z5 share the mission-state union. Never interpret their bytes as
     * Z6 Ventral Sacs state; they simply cannot carry an Overlord transport. */
    if (is_z6 && (!tech || !sc64_z6_transport_tech_state_valid(tech))) return 0;

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
            (actor.order > ORDER_UNLOAD && !SC64_NYDUS_ORDER(actor.order) &&
             !SC64_PENDING_BUILD_ORDER(actor.order))) return 0;

        if (SC64_PENDING_BUILD_ORDER(actor.order)) {
            if (!sc64_pending_build_save_identity_valid(candidate, actor.alive,
                    actor.owner, actor.type, actor.target, actor.tx, actor.ty,
                    width * 16, height * 16)) return 0;
            continue;
        }

        if (actor.order != ORDER_BOARD && actor.order != ORDER_TRANSPORTED &&
            actor.order != ORDER_UNLOAD) continue;
#ifdef SC64_SHARED_BUNKER
        if ((actor.order==ORDER_BOARD || actor.order==ORDER_TRANSPORTED) &&
            actor.target>=MAX_UNITS) continue; /* candidate-wide pass */
#endif
        if (!is_z6 || candidate->state[0] != 0 ||
            candidate->state[1] != R_ZERG ||
            candidate->state[2] != SC64_Z6_GLOBAL_ID || !tech ||
            tech->researched != 1u) return 0;

        if (actor.order == ORDER_UNLOAD) {
            if (actor.type != U_OVERLORD || actor.raw_type != 42 ||
                actor.raw_owner != 3 || actor.owner != SIDE ||
                actor.target != -1 || actor.work_timer < 0 ||
                actor.work_timer > SC64_TRANSPORT_UNLOAD_TICKS ||
                actor.tx < 0 || actor.tx >= width * 16 ||
                actor.ty < 0 || actor.ty >= height * 16) return 0;
            continue;
        }

        if (actor.target < 0 || actor.target >= MAX_UNITS || actor.target == i ||
            (actor.flags & 2u) || sc64_race_unit_flying(actor.type) ||
            !race_z6_transport_passenger_provenance_valid(&actor)) return 0;

        Unit carrier;
        if (!race_save_decode_unit(candidate, (unsigned)actor.target,
                                   (unsigned)width * 16u,
                                   (unsigned)height * 16u, &carrier)) return 0;
        if (!carrier.alive || carrier.type != U_OVERLORD ||
            carrier.raw_type != 42 || carrier.raw_owner != 3 ||
            carrier.owner != SIDE || carrier.owner != actor.owner ||
            carrier.order == ORDER_BOARD ||
            carrier.order == ORDER_TRANSPORTED ||
            !tech->researched) return 0;

        unsigned space;
        if (!sc64_transport_space_required_raw((unsigned)actor.raw_type,
                                               &space) ||
            space == SC64_TRANSPORT_SPACE_UNAVAILABLE || !space) return 0;

        if (actor.order == ORDER_TRANSPORTED) {
            if (actor.selected || actor.x != carrier.x || actor.y != carrier.y ||
                actor.path_timer || actor.path_retry) return 0;
            if ((unsigned)used[actor.target] + space >
                SC64_TRANSPORT_OVERLORD_SPACE) return 0;
            used[actor.target] = (u8)(used[actor.target] + space);
        } else if (actor.order != ORDER_BOARD) {
            return 0;
        } else if ((int)actor.path_timer < 0 || actor.path_timer > 24 ||
                   (int)actor.path_retry < 0 || actor.path_retry > 12) {
            return 0;
        }
    }

    return 1;
}
#endif

#endif
