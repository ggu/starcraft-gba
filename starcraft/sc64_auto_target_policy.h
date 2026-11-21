/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The shared automatic targeting policy. Included by game.c. */
#if defined(SC64_TARGET_POLICY_PROBE) && !defined(SC64_P7_OBSERVER_ENABLED) && \
    !defined(SC64_P6_COMBAT_ENABLED) && !defined(SC64_T6_RUNTIME) && \
    ((defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)) || \
     defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT))
#define SC64_DIRECT_TARGET_POLICY
#endif

static int __attribute__((noinline))
sc64_auto_target_candidate_allowed(int attacker, int target, int owner,
                                   int source_owner)
{
    int tx, ty;
    (void)source_owner;
    if (target >= 0 && target < MAX_UNITS && SC64_SCARAB_HIDDEN(&u[target]))
        return 0;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    /* Constructors set the source invincibility flag. Keep it a live-world
     * property so an explicit trigger override can still change it. */
    if (target >= 0 && target < MAX_UNITS && SC64_SCARAB_CHILD(&u[target]) &&
        invincible_units[target]) return 0;
#endif
    if (owner == SIDE) {
        if (target < 0) return 0;
        if (target < MAX_UNITS) {
            tx = u[target].x; ty = u[target].y;
        } else {
            int bi = target - MAX_UNITS;
            if (bi < 0 || bi >= MAX_BUILDINGS) return 0;
            tx = b[bi].x; ty = b[bi].y;
        }
        if (tx < 0 || ty < 0 || tx >= WORLD_W || ty >= WORLD_H ||
            !currently_visible(tx, ty)) return 0;
    }
#ifdef SC64_DIRECT_TARGET_POLICY
    /* In these profiles the wrappers add only the Scourge rule and the
     * Z8 visibility test. Keep the wrapper for specialized attackers. */
    if (attacker != U_SCOURGE) {
#ifdef SC64_Z8_RUNTIME
        if (owner == SIDE && race_z8_hidden_any && race_z8_unit_hidden_from_side(target))
            return 0;
#endif
        return race_can_attack(attacker, target);
    }
#endif
    return SC64_CAN_ATTACK_FROM_SOURCE(attacker, target, owner, source_owner);
}

#if defined(SC64_TARGET_QUERY_PROBE) && \
    (defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY))
#define SC64_INDEXED_TARGET_QUERY
#if defined(SC64_DIRECT_TARGET_POLICY) && !defined(SC64_TARGET_QUERY_COORDINATES_ONLY)
#define SC64_INDEXED_TARGET_WEAPONS
enum {
    SC64_TARGET_WEAPON_GROUND = 1,
    SC64_TARGET_WEAPON_AIR = 2,
    SC64_TARGET_WEAPONS_KNOWN = 4
};
static int race_can_attack_with_weapons(unsigned weapons, int target);
#endif

typedef struct {
    int attacker, owner, source_owner;
#ifdef SC64_INDEXED_TARGET_WEAPONS
    unsigned weapons;
#endif
} Sc64IndexedTargetQuery;

static inline Sc64IndexedTargetQuery
sc64_indexed_target_query(int attacker, int owner, int source_owner)
{
    Sc64IndexedTargetQuery query = {
        .attacker = attacker, .owner = owner, .source_owner = source_owner
    };
#ifdef SC64_INDEXED_TARGET_WEAPONS
    if (attacker >= 0 && attacker < SC_RU_COUNT && attacker != U_SCOURGE) {
        query.weapons = SC64_TARGET_WEAPONS_KNOWN;
        if (sc64_race_unit_damage(attacker) > 0)
            query.weapons |= SC64_TARGET_WEAPON_GROUND;
        if (sc64_race_unit_can_attack_air(attacker))
            query.weapons |= SC64_TARGET_WEAPON_AIR;
    }
#endif
    return query;
}

/* The index already validated the candidate slot and read its position
 * during this query. Visibility and target state are still read live. */
static int __attribute__((noinline))
sc64_indexed_target_candidate_allowed(const Sc64IndexedTargetQuery *query,
                                      int target, int x, int y)
{
    if (target < MAX_UNITS && SC64_SCARAB_HIDDEN(&u[target])) return 0;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    if (target < MAX_UNITS && SC64_SCARAB_CHILD(&u[target]) &&
        invincible_units[target]) return 0;
#endif
    int owner = query->owner;
    if (owner == SIDE &&
        (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H || !currently_visible(x, y)))
        return 0;
    int attacker = query->attacker;
#ifdef SC64_DIRECT_TARGET_POLICY
    if (attacker != U_SCOURGE) {
#ifdef SC64_Z8_RUNTIME
        if (owner == SIDE && race_z8_hidden_any && race_z8_unit_hidden_from_side(target))
            return 0;
#endif
#ifdef SC64_INDEXED_TARGET_WEAPONS
        if (query->weapons) return race_can_attack_with_weapons(query->weapons, target);
#endif
        return race_can_attack(attacker, target);
    }
#endif
    return SC64_CAN_ATTACK_FROM_SOURCE(attacker, target, owner, query->source_owner);
}
#endif
