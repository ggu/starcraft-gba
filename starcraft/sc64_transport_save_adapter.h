#ifndef SC64_TRANSPORT_SAVE_ADAPTER_H
#define SC64_TRANSPORT_SAVE_ADAPTER_H
#include "sc64_construction_save.h"
#ifndef SC64_NYDUS_ORDER
/* Standalone fixtures include this header without game.c's order enum. */
#define SC64_NYDUS_ORDER(o) 0
#endif

#ifdef SC64_T5_RUNTIME
/* Validate compact cargo references before any live world state changes. */
static int race_transport_save_candidate_valid(const RaceSave *candidate,
                                               int width, int height)
{
    u8 used[MAX_UNITS] = { 0 };
    u8 attached[MAX_BUILDINGS] = { 0 };
#ifdef SC64_T6_RUNTIME
    int max_order = ORDER_UNLOAD;
#endif
    if (!candidate || width < 1 || height < 1) return 0;
#ifdef SC64_T6_RUNTIME
    if (candidate->state[0] == 0 &&
        candidate->state[2] == SC64_T6_GLOBAL_ID) max_order = ORDER_REPAIR;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (candidate->state[0] == 0 &&
        candidate->state[2] >= SC64_TERRAN_LATE_GLOBAL_T7 &&
        candidate->state[2] <= SC64_TERRAN_LATE_GLOBAL_T10) max_order = ORDER_REPAIR;
#endif
#endif
#ifdef SC64_T6_STORAGE
#define CARGO_B(i, field) saved_building.field
#else
#define CARGO_B(i, field) candidate->buildings[(i)][offsetof(Building, field) / sizeof(int)]
#endif
/* The shared custom-game validator handles cargo there; keep this
     * mission-specific actor loop unchanged for campaign candidates. The
     * T5 attachment checks below still run in both modes. */
    if (!candidate->state[0]) {
#ifdef SC64_LATE_TERRAN_STORAGE
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit actor;
        if (!race_save_decode_unit(candidate, (unsigned)i,
                                   (unsigned)width * 16u,
                                   (unsigned)height * 16u, &actor)) return 0;
        if (!actor.alive) continue;
        int order = actor.order, carrier = actor.target;
#ifdef SC64_SHARED_BATTERY
        if (order==ORDER_RECHARGE) {
            if (!race_battery_saved_recharge_valid(candidate,&actor)) return 0;
            continue;
        }
#endif
#ifdef SC64_T6_RUNTIME
        if (order < ORDER_IDLE ||
            (order > max_order && !SC64_PENDING_BUILD_ORDER(order))) return 0;
#else
        if (order < ORDER_IDLE ||
            (order > ORDER_UNLOAD && !SC64_NYDUS_ORDER(order) &&
             !SC64_PENDING_BUILD_ORDER(order))) return 0;
#endif
        if (SC64_PENDING_BUILD_ORDER(order)) {
            if (!sc64_pending_build_save_identity_valid(candidate, actor.alive,
                    actor.owner, actor.type, actor.target, actor.tx, actor.ty,
                    width * 16, height * 16)) return 0;
            continue;
        }
        if (order == ORDER_UNLOAD) {
            if (actor.type != U_DROPSHIP || actor.raw_type != 11 ||
                carrier != -1 || actor.work_timer < 0 ||
                actor.work_timer > SC64_TRANSPORT_UNLOAD_TICKS ||
                actor.tx < 0 || actor.tx >= width * 16 ||
                actor.ty < 0 || actor.ty >= height * 16) return 0;
        }
        if (order != ORDER_BOARD && order != ORDER_TRANSPORTED) continue;
        unsigned space;
        if (carrier < 0 || carrier >= MAX_UNITS || carrier == i) return 0;
        Unit ship;
        if (!race_save_decode_unit(candidate, (unsigned)carrier,
                                   (unsigned)width * 16u,
                                   (unsigned)height * 16u, &ship)) return 0;
        int raw = actor.raw_type, type = actor.type;
        if (!ship.alive || ship.type != U_DROPSHIP || ship.raw_type != 11 ||
            ship.owner != actor.owner || ship.order == ORDER_BOARD ||
            ship.order == ORDER_TRANSPORTED || (actor.flags & 2) ||
            sc64_race_unit_flying(type) ||
            (sc64_race_unit_from_raw(raw) != type &&
             !(raw == 19 && type == U_RAYNOR)) ||
            !sc64_transport_space_required_raw((unsigned)raw, &space) ||
            space == SC64_TRANSPORT_SPACE_UNAVAILABLE) return 0;
        if (order == ORDER_TRANSPORTED) {
            if (actor.selected || actor.x != ship.x || actor.y != ship.y ||
                actor.path_timer || actor.path_retry) return 0;
            if ((unsigned)used[carrier] + space > SC64_TRANSPORT_DROPSHIP_SPACE)
                return 0;
            used[carrier] = (u8)(used[carrier] + space);
        }
    }
#else
#define CARGO_U(i, field) candidate->units[(i)][offsetof(Unit, field) / sizeof(int)]
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!CARGO_U(i, alive)) continue;
        int order = CARGO_U(i, order), carrier = CARGO_U(i, target);
#ifdef SC64_SHARED_BATTERY
        if (order==ORDER_RECHARGE) {
            Unit actor;
            if (!race_battery_save_unit(candidate,(unsigned)i,(unsigned)width*16u,
                                        (unsigned)height*16u,&actor) ||
                !race_battery_saved_recharge_valid(candidate,&actor)) return 0;
            continue;
        }
#endif
#ifdef SC64_T6_RUNTIME
        if (order < ORDER_IDLE ||
            (order > max_order && !SC64_PENDING_BUILD_ORDER(order))) return 0;
#else
        if (order < ORDER_IDLE ||
            (order > ORDER_UNLOAD && !SC64_NYDUS_ORDER(order) &&
             !SC64_PENDING_BUILD_ORDER(order))) return 0;
#endif
        if (SC64_PENDING_BUILD_ORDER(order)) {
            if (!sc64_pending_build_save_identity_valid(candidate,
                    CARGO_U(i, alive), CARGO_U(i, owner), CARGO_U(i, type),
                    carrier, CARGO_U(i, tx), CARGO_U(i, ty),
                    width * 16, height * 16)) return 0;
            continue;
        }
        if (order == ORDER_UNLOAD) {
            if (CARGO_U(i, type) != U_DROPSHIP || CARGO_U(i, raw_type) != 11 ||
                carrier != -1 || CARGO_U(i, work_timer) < 0 ||
                CARGO_U(i, work_timer) > SC64_TRANSPORT_UNLOAD_TICKS ||
                CARGO_U(i, tx) < 0 || CARGO_U(i, tx) >= width * 16 ||
                CARGO_U(i, ty) < 0 || CARGO_U(i, ty) >= height * 16) return 0;
        }
        if (order != ORDER_BOARD && order != ORDER_TRANSPORTED) continue;
        unsigned space;
        int raw = CARGO_U(i, raw_type), type = CARGO_U(i, type);
        if (carrier < 0 || carrier >= MAX_UNITS || carrier == i ||
            !CARGO_U(carrier, alive) || CARGO_U(carrier, type) != U_DROPSHIP ||
            CARGO_U(carrier, raw_type) != 11 ||
            CARGO_U(carrier, owner) != CARGO_U(i, owner) ||
            CARGO_U(carrier, order) == ORDER_BOARD ||
            CARGO_U(carrier, order) == ORDER_TRANSPORTED ||
            (CARGO_U(i, flags) & 2) || sc64_race_unit_flying(type) ||
            (sc64_race_unit_from_raw(raw) != type && !(raw == 19 && type == U_RAYNOR)) ||
            !sc64_transport_space_required_raw((unsigned)raw, &space) ||
            space == SC64_TRANSPORT_SPACE_UNAVAILABLE) return 0;
        if (order == ORDER_TRANSPORTED) {
            if (CARGO_U(i, selected) ||
                CARGO_U(i, x) != CARGO_U(carrier, x) ||
                CARGO_U(i, y) != CARGO_U(carrier, y) ||
                CARGO_U(i, path_timer) || CARGO_U(i, path_retry)) return 0;
            if ((unsigned)used[carrier] + space > SC64_TRANSPORT_DROPSHIP_SPACE)
                return 0;
            used[carrier] = (u8)(used[carrier] + space);
        }
    }
#undef CARGO_U
#endif
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
#ifdef SC64_T6_STORAGE
        Building saved_building;
        if (!sc64_building_save_decode(&saved_building, &candidate->buildings[i]))
            return 0;
#endif
        if (!CARGO_B(i, alive) || CARGO_B(i, type) != B_CONTROL_TOWER) continue;
        if (candidate->state[0]) {
            /* Custom add-ons attach by geometry, not the T5 mission's level
             * slot. Their source metadata may be absent. Keep orphan towers
             * valid while rejecting duplicate or contradictory attachments. */
            if ((CARGO_B(i, raw_type) != -1 && CARGO_B(i, raw_type) != 115) ||
                CARGO_B(i, maxhp) != 500 || CARGO_B(i, level) != 0 ||
                CARGO_B(i, queue_count) ||
                (CARGO_B(i, under_construction) &&
                 (CARGO_B(i, build_progress) < 0 ||
                  CARGO_B(i, build_progress) >= building_build_time(B_CONTROL_TOWER)))) return 0;
            int parent = -1;
            for (int j=0; j<MAX_BUILDINGS; ++j) {
#ifdef SC64_T6_STORAGE
                Building other;
                if (!sc64_building_save_decode(&other, &candidate->buildings[j])) return 0;
#define SK_PARENT(f) other.f
#else
#define SK_PARENT(f) CARGO_B(j, f)
#endif
                if (SK_PARENT(alive) && SK_PARENT(type) == B_STARPORT &&
                    SK_PARENT(owner) == CARGO_B(i, owner) &&
                    SK_PARENT(x) + SC64_T5_ADDON_X_OFFSET == CARGO_B(i, x) &&
                    SK_PARENT(y) + SC64_T5_ADDON_Y_OFFSET == CARGO_B(i, y)) {
                    if ((SK_PARENT(raw_type) != -1 && SK_PARENT(raw_type) != 114) ||
                        parent >= 0) return 0;
                    parent = j;
                }
#undef SK_PARENT
            }
            if (parent >= 0) {
                if (attached[parent]) return 0;
                attached[parent] = 1;
            }
            continue;
        }
        if (CARGO_B(i, raw_type) != 115 || CARGO_B(i, maxhp) != 500 ||
            CARGO_B(i, level) < 0 || CARGO_B(i, level) > MAX_BUILDINGS ||
            CARGO_B(i, queue_count) ||
            (CARGO_B(i, under_construction) &&
             (CARGO_B(i, build_progress) < 0 || CARGO_B(i, build_progress) >= 750)))
            return 0;
        int parent = CARGO_B(i, level) - 1;
        /* An orphan stays a normal destructible building, but cannot grant
         * another Starport the attachment prerequisite. */
        if (parent < 0) continue;
#ifdef SC64_T6_STORAGE
        Building parent_building;
        if (!sc64_building_save_decode(&parent_building,
                                       &candidate->buildings[parent]) ||
            !parent_building.alive || parent_building.type != B_STARPORT ||
            parent_building.raw_type != 114 ||
            parent_building.owner != saved_building.owner ||
            saved_building.x != parent_building.x + SC64_T5_ADDON_X_OFFSET ||
            saved_building.y != parent_building.y + SC64_T5_ADDON_Y_OFFSET ||
            attached[parent]) return 0;
#else
        if (!CARGO_B(parent, alive) || CARGO_B(parent, type) != B_STARPORT ||
            CARGO_B(parent, raw_type) != 114 ||
            CARGO_B(parent, owner) != CARGO_B(i, owner) ||
            CARGO_B(i, x) != CARGO_B(parent, x) + SC64_T5_ADDON_X_OFFSET ||
            CARGO_B(i, y) != CARGO_B(parent, y) + SC64_T5_ADDON_Y_OFFSET ||
            attached[parent]) return 0;
#endif
        attached[parent] = 1;
    }
#undef CARGO_B
#undef CARGO_U
    return 1;
}
#endif
#endif
