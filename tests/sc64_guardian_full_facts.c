/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Generated in a private, stage-specific directory; no guest qualification. */
#include <assert.h>
#include <stdio.h>
#include "race_rules.h"
#include "sc64_game_actor_facts.h"
int main(void)
{
    assert(SC64_FACT_UNIT_COUNT==SC_RU_COUNT);
    for (int t=0;t<SC_RU_COUNT;++t) {
        assert(sc64_game_unit_actor_flags(t)==sc64_game_unit_actor_flags_reference(t));
        assert(sc64_game_unit_collision_radius(t)==sc64_game_unit_collision_radius_reference(t));
        assert(sc64_race_unit_flying(t)==sc64_race_unit_flying_reference(t));
    }
#ifdef SC64_FACT_SIGHT_TABLES
    assert(sc64_fact_sight_radius_runtime(U_GUARDIAN_COCOON,0)==64);
#if defined(SC64_SHARED_EGG_WIRE_HOST_STAGE) && !defined(SC64_Z7_EGG_PRODUCTION)
    assert(sc64_fact_sight_radius_runtime(U_SHARED_EGG_STAGE,0)==64);
#endif
#ifdef SC64_GUARDIAN_DISTINCT_STAGE
    assert(sc64_fact_sight_radius_runtime(U_GUARDIAN_STAGE,0)==176);
#endif
#endif /* SC64_FACT_SIGHT_TABLES */
#ifdef SC64_SHARED_ACTOR_FULL_HOST_STAGE
    assert(sc64_race_raw_unit(U_GUARDIAN_COCOON)==59);
    assert(sc64_race_raw_unit(U_SCARAB)==85);
#ifdef SC64_Z7_EGG_PRODUCTION
    assert(U_GUARDIAN_COCOON==SC_RU_LEGACY_COUNT);
#else
    assert(U_SHARED_EGG_STAGE==SC_RU_LEGACY_COUNT);
    assert(U_GUARDIAN_COCOON==U_SHARED_EGG_STAGE+1);
#endif
#ifdef SC64_GUARDIAN_DISTINCT_STAGE
    assert(sc64_race_raw_unit(U_GUARDIAN_STAGE)==44);
    assert(U_GUARDIAN_STAGE==U_GUARDIAN_COCOON+1);
    assert(U_SCARAB==U_GUARDIAN_STAGE+1);
#else
    assert(U_SCARAB==U_GUARDIAN_COCOON+1);
#endif
#endif
    puts("Guardian and Egg full fact table HOST stage passed");
    return 0;
}
