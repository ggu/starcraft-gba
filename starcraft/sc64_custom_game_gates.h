/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Call-site gates for campaign-only predicates. Included by game.c right
 * after race_mode is declared.
 *
 * Each predicate below returns 0 whenever race_mode is nonzero (a Custom
 * Game): it has no side effects and returns nonzero only when a campaign
 * mission check holds (race_t6_active(), race_p6_active(),
 * race_late_active(); the last one is tested after read-only slot and
 * flight checks). Testing race_mode at the call site therefore gives the same
 * answer without the call; campaign missions (race_mode == 0) still call
 * the function and take exactly the path they took before. The functions
 * are declared and defined with their names in parentheses so that these
 * function-like macros do not expand there. */
#ifndef SC64_CUSTOM_GAME_GATES_H
#define SC64_CUSTOM_GAME_GATES_H

#ifdef SC64_T6_RUNTIME
#define race_t6_unit_burrowed(slot) (!race_mode && (race_t6_unit_burrowed)(slot))
#endif
#ifdef SC64_P6_COMBAT_ENABLED
#define race_p6_combat_unit_burrowed(slot) (!race_mode && (race_p6_combat_unit_burrowed)(slot))
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
#define race_late_airborne(slot) (!race_mode && (race_late_airborne)(slot))
#endif

#endif
