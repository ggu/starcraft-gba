/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Nearest-enemy dispatch for the shared engine. Included by game.c. */
#if defined(SC64_Z7_SCOURGE_COMBAT) && defined(SC64_UNARMED_QUERY_SKIP)
#if defined(SC64_Z7_ARMED_TABLE_PROBE) && defined(SC64_COMBAT_FACT_TABLE_PROBE)
#include "sc64_combat_facts.h"
#endif
/* Share the expanded ground/air lookup from ROM in the larger Z7 build. */
static int __attribute__((noinline))
sc64_z7_unit_has_weapon(int type)
{
#if defined(SC64_Z7_ARMED_TABLE_PROBE) && defined(SC64_COMBAT_FACT_TABLE_PROBE)
    if ((unsigned)type < SC_RU_COUNT)
        return (sc64_fact_combat_table[type] &
                (SC64_COMBAT_GROUND_WEAPON | SC64_COMBAT_AIR_WEAPON)) != 0;
#endif
    return sc64_race_unit_damage_vs(type, 0) > 0 ||
           sc64_race_unit_damage_vs(type, 1) > 0;
}
#endif

static int GBA_HOT
nearest_enemy_typed(int x, int y, int owner, int max_distance, int attacker
                    SC64_P7_SOURCE_OWNER_PARAM)
{
    int result;
#ifdef SC64_DIAGNOSTIC_ORDERS
    u32 sc64_query_start = gba_clock();
#endif
#if defined(SC64_UNARMED_QUERY_SKIP) && !defined(SC64_RACES)
#error "SC64_UNARMED_QUERY_SKIP requires SC64_RACES"
#endif
#ifdef SC64_UNARMED_QUERY_SKIP
    /* Nothing in the ground or air weapon tables can damage a candidate.
     * Keep attacker == -1 as the historical wildcard and leave invalid IDs
     * to the ordinary checked query path. This deliberately uses damage_vs
     * for both classes so the P3 air-only Scourge is not skipped. */
    if (attacker >= 0 && attacker < SC_RU_COUNT &&
#ifdef SC64_Z7_SCOURGE_COMBAT
        !sc64_z7_unit_has_weapon(attacker)) {
#else
        sc64_race_unit_damage_vs(attacker, 0) <= 0 &&
        sc64_race_unit_damage_vs(attacker, 1) <= 0) {
#endif
        result = -1;
        goto sc64_query_done;
    }
#endif
#ifdef SC64_RACES
    if (target_lists_valid && sc64_target_index_active && max_distance > 0 &&
        x >= 0 && y >= 0 && x < WORLD_W && y < WORLD_H)
        result = sc64_target_nearest(x, y, owner, max_distance, attacker
                                     SC64_P7_SOURCE_OWNER_ARG(SC64_P7_SOURCE_OWNER_VALUE));
    else
        result = nearest_enemy_scan(x, y, owner, max_distance, attacker
                                    SC64_P7_SOURCE_OWNER_ARG(SC64_P7_SOURCE_OWNER_VALUE));
#else
    int i, best = -1, best_distance = max_distance * max_distance, d;
    (void)attacker;
    /* Only consult fog for candidates that could replace the current best. */
    if (target_lists_valid) {
        int p;
        for (p = 0; p < 5; ++p) {
            if (!owners_hostile(owner, p)) continue;
            for (i = target_unit_head[p]; i >= 0; i = target_unit_next[i]) {
                if (!u[i].alive || FAST_ABS(x - u[i].x) >= max_distance ||
                    FAST_ABS(y - u[i].y) >= max_distance) continue;
                d = dist2(x, y, u[i].x, u[i].y);
                if ((d < best_distance || (best >= 0 && d == best_distance && i < best)) &&
                    sc64_auto_target_visible(i, owner)) {
                    best_distance = d; best = i;
                }
            }
        }
        for (p = 0; p < 5; ++p) {
            if (!owners_hostile(owner, p)) continue;
            for (i = target_building_head[p]; i >= 0; i = target_building_next[i]) {
                int candidate = MAX_UNITS + i;
                if (!b[i].alive || FAST_ABS(x - b[i].x) >= max_distance ||
                    FAST_ABS(y - b[i].y) >= max_distance) continue;
                d = dist2(x, y, b[i].x, b[i].y);
                if ((d < best_distance || (best >= 0 && d == best_distance && candidate < best)) &&
                    sc64_auto_target_visible(candidate, owner)) {
                    best_distance = d; best = candidate;
                }
            }
        }
    } else {
        for (i = 0; i < MAX_UNITS; i++) {
            if (!u[i].alive || FAST_ABS(x - u[i].x) >= max_distance ||
                FAST_ABS(y - u[i].y) >= max_distance || !owners_hostile(owner, u[i].owner)) continue;
            d = dist2(x, y, u[i].x, u[i].y);
            if (d < best_distance && sc64_auto_target_visible(i, owner)) {
                best_distance = d;
                best = i;
            }
        }
        for (i = 0; i < MAX_BUILDINGS; i++) {
            if (!b[i].alive || FAST_ABS(x - b[i].x) >= max_distance ||
                FAST_ABS(y - b[i].y) >= max_distance || !owners_hostile(owner, b[i].owner)) continue;
            int candidate = MAX_UNITS + i;
            d = dist2(x, y, b[i].x, b[i].y);
            if (d < best_distance && sc64_auto_target_visible(candidate, owner)) {
                best_distance = d;
                best = candidate;
            }
        }
    }
    result = best;
#endif
#ifdef SC64_UNARMED_QUERY_SKIP
sc64_query_done:
#endif
#ifdef SC64_DIAGNOSTIC_ORDERS
    SC64_ORDER_DIAG_ACCUM(sc64_order_query_cycles, sc64_query_start);
#endif
    return result;
}

static int nearest_enemy(int x,int y,int owner,int distance) {
    return nearest_enemy_typed(x,y,owner,distance,-1 SC64_P7_SOURCE_OWNER_ARG(-1));
}
