/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The shared automatic targeting policy. Included by game.c. */
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
#if defined(SC64_TARGET_POLICY_PROBE) && !defined(SC64_P7_OBSERVER_ENABLED) && \
    !defined(SC64_P6_COMBAT_ENABLED) && !defined(SC64_T6_RUNTIME) && \
    ((defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)) || \
     defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT))
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
