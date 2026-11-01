/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Existing ordinary impact behavior while the Scarab probe uses the pool. */
#define main sc64_impact_original_main
#include "sc64_impact_events_game.c"
#undef main
int main(void)
{
    test_impact_matches_presentation();test_moving_target_still_hit();
    test_dead_target_and_slot_reuse();test_attacker_death_and_ownership();
    test_simultaneous_impacts();test_shields_invincibility_and_cleanup();
    test_overflow_defers_launch();test_tower_projectiles_wait();
    test_immediate_weapons();
    puts("ordinary impact queue remains correct with Scarab marker enabled");return 0;
}
