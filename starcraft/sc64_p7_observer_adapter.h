#ifndef SC64_P7_OBSERVER_ADAPTER_H
#define SC64_P7_OBSERVER_ADAPTER_H

#ifdef SC64_P7_OBSERVER_ENABLED
#include "sc64_p7_observer_rules.h"
#include "sc64_p7_geometry.h"

/* Owner-specific optical visibility query. The world maintains a single
 * SIDE grid, so only source slot 7 can reuse it. Other P7 owners are evaluated
 * on demand using the same cell-center circles as sc64_visibility.h. */
static int sc64_p7_observer_source_matches(int raw_owner, int live_owner,
                                           int source_raw_owner,
                                           int source_live_owner)
{
    /* A known querying slot must never absorb another source player's
       unknown or collapsed live-owner records. Unknown query provenance alone
       triggers the documented legacy live-owner fallback. */
    if (source_raw_owner >= 0 && source_raw_owner < 8)
        return raw_owner == source_raw_owner;
    return live_owner == source_live_owner;
}

static int sc64_p7_observer_sight_radius_runtime(int type, int building)
{
    int source_pixels = building ?
        sc64_zp_building_sight_source_pixels(type) :
        sc64_zp_actor_sight_source_pixels(type);
    if (source_pixels > 0) return source_pixels / 2;
    if (building)
        return (type == B_COMMAND || sc64_race_is_zerg_townhall(type) ||
                type == B_NEXUS) ? 112 : 64;
    return IS_WORKER(type) ? 56 : 72;
}

static int sc64_p7_observer_owner_sees_cell(int source_owner, int live_owner,
                                           int cell)
{
    int col = cell % MAP_COLS, row = cell / MAP_COLS;
    int cx = col * 16 + 8, cy = row * 16 + 8;
    if (source_owner == 7 || (source_owner < 0 && live_owner == SIDE))
        return SC64_GRID_GET(visible, cell);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        sc64_p7_observer_source_matches(u[i].raw_owner, u[i].owner,
                                        source_owner, live_owner)) {
        int radius = sc64_p7_observer_sight_radius_runtime(u[i].type, 0);
        int dx = cx - u[i].x, dy = cy - u[i].y;
        if (dx * dx + dy * dy <= radius * radius) return 1;
    }
    /* Match the current GBA sight grid: alive buildings contribute optical
     * sight while under construction. Native construction-sight differences
     * are not modeled by this approximation. */
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        sc64_p7_observer_source_matches(b[i].raw_owner, b[i].owner,
                                        source_owner, live_owner)) {
        int radius = sc64_p7_observer_sight_radius_runtime(b[i].type, 1);
        int dx = cx - b[i].x, dy = cy - b[i].y;
        if (dx * dx + dy * dy <= radius * radius) return 1;
    }
    return 0;
}

static int sc64_p7_observer_target_optically_visible(int target,
                                                      int source_owner,
                                                      int live_owner)
{
    int x, y, cell;
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS) return 0;
    if (target < MAX_UNITS) {
        if (!u[target].alive) return 0;
        x = u[target].x; y = u[target].y;
    } else {
        int i = target - MAX_UNITS;
        if (!b[i].alive) return 0;
        x = b[i].x; y = b[i].y;
    }
    int col = x / 16, row = y / 16;
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) return 0;
    cell = col + row * MAP_COLS;
    return sc64_p7_observer_owner_sees_cell(source_owner, live_owner, cell);
}

static int sc64_p7_observer_target_raw_type(int target)
{
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS) return -1;
    return target < MAX_UNITS ? u[target].raw_type :
           b[target - MAX_UNITS].raw_type;
}

static int sc64_p7_observer_pylon_powers(int cannon, int source_owner,
                                          int live_owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) &&
        b[i].type == B_PYLON &&
        sc64_p7_observer_source_matches(b[i].raw_owner, b[i].owner,
                                        source_owner, live_owner) &&
        dist2(b[cannon].x, b[cannon].y, b[i].x, b[i].y) <= 144 * 144)
        return 1;
    return 0;
}

static unsigned sc64_p7_observer_xy_length(unsigned x, unsigned y)
{
    unsigned major = x > y ? x : y, minor = x > y ? y : x;
    /* The runtime uses the pinned integer distance metric, approximated with
       shifts so this stays deterministic and cheap on the ARM target. */
    if (major / 4u < minor)
        major = major - major / 16u + minor * 3u / 8u - major / 64u +
                minor * 3u / 256u;
    return major;
}

static int sc64_p7_observer_rect_distance(unsigned raw_a, int x_a, int y_a,
                                          int building_a, unsigned raw_b,
                                          int x_b, int y_b, int building_b)
{
    int al, at, ar, ab, bl, bt, br, bb;
    if (!sc64_p7_actor_rect_source(raw_a, x_a, y_a, building_a,
                                  &al, &at, &ar, &ab) ||
        !sc64_p7_actor_rect_source(raw_b, x_b, y_b, building_b,
                                  &bl, &bt, &br, &bb)) return -1;
    /* Half-open helper rectangles correspond to inclusive source rectangles.
       Source rect_difference measures the gap between inclusive edge pixels. */
    int dx = al >= br ? al - (br - 1) : bl >= ar ? bl - (ar - 1) : 0;
    int dy = at >= bb ? at - (bb - 1) : bt >= ab ? bt - (ab - 1) : 0;
    return (int)sc64_p7_observer_xy_length((unsigned)dx,(unsigned)dy);
}

static int sc64_p7_observer_detector_sees(int target, int source_owner,
                                         int live_owner)
{
    int target_raw = sc64_p7_observer_target_raw_type(target);
    if (target_raw < 0 || !sc64_p7_observer_target_optically_visible(
            target, source_owner, live_owner)) return 0;
    unsigned raw_target = (unsigned)target_raw;
    int target_x = target < MAX_UNITS ? u[target].x : b[target-MAX_UNITS].x;
    int target_y = target < MAX_UNITS ? u[target].y : b[target-MAX_UNITS].y;
    int target_building = target >= MAX_UNITS;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        u[i].raw_type == SC64_P7_OBSERVER_RAW &&
        sc64_p7_observer_source_matches(u[i].raw_owner, u[i].owner,
                                        source_owner, live_owner)) {
        Sc64P7DetectorState state = {1, 1, 0, 1, 0};
        int range = sc64_p7_detector_range_source_pixels(
            (unsigned)u[i].raw_type, 1, 0);
        int distance = sc64_p7_observer_rect_distance((unsigned)u[i].raw_type,
            u[i].x, u[i].y, 0, raw_target, target_x, target_y,
            target_building);
        if (sc64_p7_detector_usable(&state, 0) && distance >= 0 &&
            distance <= range) return 1;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        b[i].raw_type == SC64_P7_CANNON_RAW && building_complete(i) &&
        sc64_p7_observer_pylon_powers(i, source_owner, live_owner) &&
        sc64_p7_observer_source_matches(b[i].raw_owner, b[i].owner,
                                        source_owner, live_owner)) {
        Sc64P7DetectorState state = {1, 1, 0, 1, 0};
        int range = sc64_p7_detector_range_source_pixels(
            (unsigned)b[i].raw_type, 1, 0);
        int distance = sc64_p7_observer_rect_distance((unsigned)b[i].raw_type,
            b[i].x, b[i].y, 1, raw_target, target_x, target_y,
            target_building);
        if (sc64_p7_detector_usable(&state, 1) && distance >= 0 &&
            distance <= range) return 1;
    }
    return 0;
}

static int sc64_p7_observer_can_attack_from(int attacker, int target,
                                            int live_owner,
                                            int source_owner)
{
    if (!SC64_CAN_ATTACK_FROM(attacker, target, live_owner)) return 0;
    if (!race_p7_active() || source_owner >= 8 ||
        !sc64_p7_observer_is_cloaked(
            (unsigned)sc64_p7_observer_target_raw_type(target))) return 1;
    return sc64_p7_observer_target_allowed(
        (unsigned)sc64_p7_observer_target_raw_type(target),
        sc64_p7_observer_target_optically_visible(target, source_owner,
                                                   live_owner),
        sc64_p7_observer_detector_sees(target, source_owner, live_owner));
}

#endif /* SC64_P7_OBSERVER_ENABLED */
#endif /* SC64_P7_OBSERVER_ADAPTER_H */
