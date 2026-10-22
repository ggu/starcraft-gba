#ifndef SC64_TRANSPORT_SAVE_ADAPTER_H
#define SC64_TRANSPORT_SAVE_ADAPTER_H

#ifdef SC64_T5_RUNTIME
/* Validate compact cargo references before any live world state changes. */
static int race_transport_save_candidate_valid(const RaceSave *candidate,
                                               int width, int height)
{
    u8 used[MAX_UNITS] = { 0 };
    u8 attached[MAX_BUILDINGS] = { 0 };
    if (!candidate || width < 1 || height < 1) return 0;
#define CARGO_U(i, field) candidate->units[(i)][offsetof(Unit, field) / sizeof(int)]
#define CARGO_B(i, field) candidate->buildings[(i)][offsetof(Building, field) / sizeof(int)]
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!CARGO_U(i, alive)) continue;
        int order = CARGO_U(i, order), carrier = CARGO_U(i, target);
        if (order < ORDER_IDLE || order > ORDER_UNLOAD) return 0;
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
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!CARGO_B(i, alive) || CARGO_B(i, type) != B_CONTROL_TOWER) continue;
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
        if (!CARGO_B(parent, alive) || CARGO_B(parent, type) != B_STARPORT ||
            CARGO_B(parent, raw_type) != 114 ||
            CARGO_B(parent, owner) != CARGO_B(i, owner) ||
            CARGO_B(i, x) != CARGO_B(parent, x) + SC64_T5_ADDON_X_OFFSET ||
            CARGO_B(i, y) != CARGO_B(parent, y) + SC64_T5_ADDON_Y_OFFSET ||
            attached[parent]) return 0;
        attached[parent] = 1;
    }
#undef CARGO_U
#undef CARGO_B
    return 1;
}
#endif
#endif
