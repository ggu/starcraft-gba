/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The can-attack test in tick_combat_from(). Included by game.c just
 * before that function.
 *
 * The two automatic acquisition paths in update_orders() call
 * nearest_enemy_typed(x, y, q->owner, range, q->type, q->raw_owner) and pass
 * the result straight to tick_combat_from(q, target, 1). Every path through
 * that query (the target index, the scan fallback, the Egg fallback) admits
 * a candidate only through sc64_auto_target_candidate_allowed() with those
 * arguments, which ends in the same SC64_CAN_ATTACK_FROM_SOURCE() test, and
 * nothing between the query and this test changes the state it reads. So
 * for those calls the test is already known to pass; every other caller
 * passes 0 and keeps it.
 *
 * Host builds still evaluate the test for validated calls and stop if it
 * would fail, so the host suites prove the claim on every call they make. */
#ifndef SC64_COMBAT_VALIDATED_H
#define SC64_COMBAT_VALIDATED_H

/* What the two acquisition paths pass: only race builds test can-attack. */
#ifdef SC64_RACES
#define SC64_COMBAT_QUERY_VALIDATED 1
#else
#define SC64_COMBAT_QUERY_VALIDATED 0
#endif

#ifdef SC64_RACES
#ifdef GBA_HOST
#include <stdio.h>
#include <stdlib.h>
static unsigned long sc64_combat_validated_calls, sc64_combat_checked_calls;
static void __attribute__((destructor)) sc64_combat_validated_report(void)
{
    if (getenv("SC64_COMBAT_VALIDATED_REPORT"))
        fprintf(stderr, "combat-validated: %lu validated calls proven, %lu checked\n",
                sc64_combat_validated_calls, sc64_combat_checked_calls);
}
static int
sc64_combat_target_allowed(const Unit *q, int target, int validated)
{
    int allowed = SC64_CAN_ATTACK_FROM_SOURCE(q->type, target, q->owner, q->raw_owner);
    if (validated) {
        if (!allowed) {
            fprintf(stderr, "validated combat target failed can-attack: attacker slot %d type %d "
                    "owner %d raw_owner %d target %d frame %d\n",
                    (int)(q - u), q->type, q->owner, q->raw_owner, target, frame);
            abort();
        }
        ++sc64_combat_validated_calls;
    } else {
        ++sc64_combat_checked_calls;
    }
    return allowed;
}
#define SC64_COMBAT_TARGET_ALLOWED(q, target, validated) \
    sc64_combat_target_allowed((q), (target), (validated))
#else
#define SC64_COMBAT_TARGET_ALLOWED(q, target, validated) \
    ((validated) || SC64_CAN_ATTACK_FROM_SOURCE((q)->type, (target), (q)->owner, (q)->raw_owner))
#endif
#endif

#endif
