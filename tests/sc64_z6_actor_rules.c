#include <assert.h>
#include "race_rules.h"

#ifndef SC64_Z6_ACTORS
#ifdef SC64_ZP_CAMPAIGN_ACTORS
_Static_assert(SC_RU_COUNT == 33,
               "Z6 actor IDs must not alter the ZP-only ABI when disabled");
#else
_Static_assert(SC_RU_COUNT == 28,
               "the base race enum stays unchanged without campaign gates");
#endif
int main(void)
{
    assert(sc64_race_unit_from_raw(44) == -1);
    return 0;
}
#else
#ifdef SC64_T4_RUNTIME
_Static_assert(SC_RU_COUNT == 47,
               "T4 actor IDs coexist with Z5 and the appended Guardian");
#else
_Static_assert(SC_RU_COUNT == 40,
               "Z6 appends one actor after the existing Z5 IDs");
#endif
_Static_assert(U_Z6_GUARDIAN == SC_RU_COUNT - 1,
               "Guardian appends to the selected actor set");
int main(void)
{
    const Sc64Z6ActorFact *guardian = sc64_z6_actor_fact(44);
    assert(guardian && guardian->raw == 44);
    assert(guardian->hp == 150 && guardian->shields == 0 && guardian->armor == 2);
    assert(guardian->minerals == 50 && guardian->gas == 100);
    assert(guardian->build_frames == 600 && guardian->sight_px == 352);
    assert(guardian->width_tiles == 2 && guardian->height_tiles == 2);
    assert(guardian->left == 22 && guardian->up == 22 &&
           guardian->right == 21 && guardian->down == 21);
    assert(guardian->size == 2);
    assert(guardian->ground_damage_type == SC64_Z6_DAMAGE_NORMAL);
    assert(guardian->ground_upgrade == SC64_Z6_UPGRADE_ZERG_FLYER_ATTACKS);
    assert(guardian->ground_damage == 20 && guardian->ground_bonus == 2);
    assert(guardian->ground_cooldown == 30 && guardian->ground_range == 256 &&
           guardian->ground_hits == 1);
    assert(guardian->air_damage == 0 && guardian->air_hits == 0);
    assert((guardian->flags & (SC64_Z6_SOURCE_FLYER|
                               SC64_Z6_SOURCE_REGENERATES_HP|
                               SC64_Z6_SOURCE_ORGANIC|
                               SC64_Z6_SOURCE_MORPH|
                               SC64_Z6_SOURCE_HUGE|
                               SC64_Z6_SOURCE_GROUND_ONLY)) ==
           (SC64_Z6_SOURCE_FLYER|SC64_Z6_SOURCE_REGENERATES_HP|
            SC64_Z6_SOURCE_ORGANIC|SC64_Z6_SOURCE_MORPH|
            SC64_Z6_SOURCE_HUGE|SC64_Z6_SOURCE_GROUND_ONLY));
    assert(sc64_z6_actor_radius_px(guardian) == 11);
    assert(sc64_z6_actor_fact(43) == 0);
    assert(sc64_race_unit_from_raw(44) == U_Z6_GUARDIAN);
    assert(sc64_race_raw_unit(U_Z6_GUARDIAN) == 44);
    assert(sc64_race_unit_rule(U_Z6_GUARDIAN)->hp == 150);
    assert(sc64_race_unit_flying(U_Z6_GUARDIAN));
    assert(sc64_race_unit_damage(U_Z6_GUARDIAN) == 20);
    assert(sc64_race_unit_range(U_Z6_GUARDIAN) == 256);
    assert(sc64_race_unit_cooldown(U_Z6_GUARDIAN) == 30);
    assert(sc64_race_unit_damage_vs(U_Z6_GUARDIAN, 0) == 20);
    assert(sc64_race_unit_damage_vs(U_Z6_GUARDIAN, 1) == 0);
    assert(sc64_race_unit_range_vs(U_Z6_GUARDIAN, 1) == 0);
    assert(sc64_race_unit_cooldown_vs(U_Z6_GUARDIAN, 0) == 30);
    assert(sc64_race_unit_hits_vs(U_Z6_GUARDIAN, 0) == 1);
    assert(sc64_race_unit_hits_vs(U_Z6_GUARDIAN, 1) == 0);
    assert(sc64_race_unit_armor(U_Z6_GUARDIAN) == 2);
    assert(!sc64_race_unit_can_attack_air(U_Z6_GUARDIAN));
    return 0;
}
#endif
