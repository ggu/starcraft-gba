#include <assert.h>
#include <stdio.h>

#include "starcraft/sc64_t4_world.h"

int main(void)
{
    static const int runtime_owner[8] = {0, 3, 3, 1, 4, 2, 4, 3};
    static const int force[8] = {1, 3, 3, 2, 4, 4, 4, 3};
    int i;

    for (i = 0; i < 8; ++i) {
        assert(sc64_t4_runtime_owner(i) == runtime_owner[i]);
        assert(sc64_t4_source_force(i) == force[i]);
        assert(sc64_t4_force_contains(force[i], i));
        assert(sc64_t4_force_hostile_to_human(force[i]) ==
               (force[i] == SC64_T4_FORCE2 || force[i] == SC64_T4_FORCE4));
    }
    assert(sc64_t4_runtime_owner(11) == SC64_T4_OWNER_NEUTRAL);
    assert(sc64_t4_runtime_owner(8) == -1);
    assert(sc64_t4_runtime_owner(-1) == -1);
    assert(sc64_t4_source_force(-1) == SC64_T4_FORCE_NONE);
    assert(sc64_t4_source_force(8) == SC64_T4_FORCE_NONE);
    assert(sc64_t4_force_contains(SC64_T4_FORCE3, 7));
    assert(!sc64_t4_force_contains(SC64_T4_FORCE2, 7));
    assert(sc64_t4_current_source_owner(SC64_T4_OWNER_SIDE, 1) == 0);
    assert(sc64_t4_force_contains(SC64_T4_FORCE1,
           sc64_t4_current_source_owner(SC64_T4_OWNER_SIDE, 1)));
    assert(!sc64_t4_force_contains(SC64_T4_FORCE3,
           sc64_t4_current_source_owner(SC64_T4_OWNER_SIDE, 1)));
    assert(sc64_t4_current_source_owner(SC64_T4_OWNER_RESCUABLE, 1) == 1);
    assert(sc64_t4_force_contains(SC64_T4_FORCE3,
           sc64_t4_current_source_owner(SC64_T4_OWNER_RESCUABLE, 1)));
    assert(sc64_t4_current_source_owner(SC64_T4_OWNER_ENEMY, 4) == -1);
    assert(sc64_t4_current_source_owner(SC64_T4_OWNER_FORCE4, 4) == 4);
    assert(sc64_t4_current_source_owner(SC64_T4_OWNER_FORCE4, 6) == 6);
    assert(sc64_t4_current_source_owner(SC64_T4_OWNER_NEUTRAL, 11) == 11);
    assert(sc64_t4_current_source_owner(SC64_T4_OWNER_SIDE, 11) == -1);
    assert(sc64_t4_current_source_owner(SC64_T4_OWNER_SIDE, 3) == -1);

    assert(sc64_t4_owner_hostile_to_human(SC64_T4_OWNER_ENEMY));
    assert(sc64_t4_owner_hostile_to_human(SC64_T4_OWNER_FORCE4));
    assert(!sc64_t4_owner_hostile_to_human(SC64_T4_OWNER_RESCUABLE));
    assert(!sc64_t4_owner_hostile_to_human(SC64_T4_OWNER_NEUTRAL));
    assert(!sc64_t4_owner_hostile_to_human(SC64_T4_OWNER_SIDE));

    for (i = 0; i <= SC64_T4_OWNER_FORCE4; ++i) {
        int j;
        for (j = 0; j <= SC64_T4_OWNER_FORCE4; ++j) {
            int active_i = i == SC64_T4_OWNER_SIDE ||
                           i == SC64_T4_OWNER_ENEMY ||
                           i == SC64_T4_OWNER_FORCE4;
            int active_j = j == SC64_T4_OWNER_SIDE ||
                           j == SC64_T4_OWNER_ENEMY ||
                           j == SC64_T4_OWNER_FORCE4;
            assert(sc64_t4_owners_hostile(i, j) ==
                   (active_i && active_j && i != j));
        }
    }
    assert(sc64_t4_owners_hostile(SC64_T4_OWNER_ENEMY,
                                  SC64_T4_OWNER_FORCE4));
    assert(!sc64_t4_owners_hostile(SC64_T4_OWNER_FORCE4,
                                   SC64_T4_OWNER_FORCE4));
    assert(!sc64_t4_owners_hostile(-1, SC64_T4_OWNER_SIDE));
    assert(!sc64_t4_owners_hostile(SC64_T4_OWNER_SIDE, 5));
    assert(sc64_t4_rescue_candidate_present(SC64_T4_OWNER_SIDE, 1));
    assert(!sc64_t4_rescue_candidate_present(SC64_T4_OWNER_SIDE, 0));
    assert(!sc64_t4_rescue_candidate_present(SC64_T4_OWNER_RESCUABLE, 1));

    /* GPTP's source-pixel rectangular +/-64 query, inclusive approximation.
       Candidate ownership is live controller state, not raw provenance. */
    assert(sc64_t4_rescue_contact(1, 1, 100, 100, 0, 164, 36));
    assert(sc64_t4_rescue_contact(1, 2, 100, 100, 0, 36, 164));
    assert(sc64_t4_rescue_contact(1, 7, 100, 100, 0, 100, 100));
    /* A slot-1 unit that was already rescued is live SIDE and can qualify;
       its preserved raw_owner does not affect the candidate test. */
    assert(sc64_t4_rescue_candidate_present(SC64_T4_OWNER_SIDE, 1));
    assert(sc64_t4_rescue_contact(1, 2, 100, 100,
                                  SC64_T4_OWNER_SIDE, 164, 100));
    assert(!sc64_t4_rescue_contact(0, 1, 100, 100, 0, 100, 100));
    assert(!sc64_t4_rescue_contact(1, 3, 100, 100, 0, 100, 100));
    assert(!sc64_t4_rescue_contact(1, 1, 100, 100,
                                   SC64_T4_OWNER_ENEMY, 100, 100));
    assert(!sc64_t4_rescue_contact(1, 1, 100, 100, 0, 165, 100));
    assert(!sc64_t4_rescue_contact(1, 1, 100, 100, 0, 100, 165));

    /* Only completed, grounded resource depots trigger owner-wide transfer. */
    assert(sc64_t4_rescue_depot_group(1, 1, 1));
    assert(!sc64_t4_rescue_depot_group(0, 1, 1));
    assert(!sc64_t4_rescue_depot_group(1, 0, 1));
    assert(!sc64_t4_rescue_depot_group(1, 1, 0));
    assert(sc64_t4_rescue_transfer_candidate(1, 1, 2, 2, 0));
    assert(!sc64_t4_rescue_transfer_candidate(1, 0, 2, 2, 0));
    assert(!sc64_t4_rescue_transfer_candidate(1, 1, 1, 2, 0));
    assert(sc64_t4_rescue_transfer_candidate(0, 1, 7, 7, 1));
    assert(!sc64_t4_rescue_transfer_candidate(0, 1, 7, 7, 0));
    assert(!sc64_t4_rescue_transfer_candidate(0, 1, 2, 7, 1));
    assert(!sc64_t4_rescue_transfer_candidate(1, 1, 2, 8, 0));

    puts("T4 source owner, force, rescue range, and transfer policy pass");
    return 0;
}
