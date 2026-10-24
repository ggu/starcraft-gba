#ifndef SC64_P5_TRANSPORT_SAVE_H
#define SC64_P5_TRANSPORT_SAVE_H

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
        if (actor.order < ORDER_IDLE || actor.order > ORDER_UNLOAD) return 0;
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
