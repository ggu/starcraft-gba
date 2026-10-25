#include <assert.h>
#include <string.h>

#include "../.local/sc64/zp-stage/generated/scenarios/sc64_z6_abilities_generated.h"
#define SC64_Z5_RUNTIME 1
#define SC64_Z5_CLOAK_ENABLED 1
#include "../starcraft/sc64_z5_cloak.h"

_Static_assert(SC64_Z6_ABILITIES_SOURCE_GLOBAL_ID == 17u, "Z6 campaign identity");
_Static_assert(SC64_Z6_ABILITIES_PERSONNEL_CLOAK_AVAILABLE == 1u &&
               SC64_Z6_ABILITIES_PERSONNEL_CLOAK_RESEARCHED == 0u,
               "Z6 cloak source facts");
_Static_assert(SC64_Z6_ABILITIES_HERO_RAW_TYPE == 51u &&
               SC64_Z6_ABILITIES_HERO_INITIAL_ENERGY_PERCENT == 100u &&
               SC64_Z6_ABILITIES_HERO_INITIAL_CLOAKED == 0u,
               "Z6 initial hero energy/state");
_Static_assert(SC64_Z6_ABILITIES_OBSERVER_RAW_TYPE == 84u &&
               SC64_Z6_ABILITIES_OBSERVER_SIGHT_TILES == 9u &&
               SC64_Z6_ABILITIES_PHOTON_CANNON_RAW_TYPE == 162u &&
               SC64_Z6_ABILITIES_PHOTON_CANNON_SIGHT_TILES == 11u,
               "Z6 detector identities and ranges");

static void test_source_start_and_toggle(void)
{
    Sc64Z5CloakState cloak={0,0};
    int energy=SC64_Z5_CLOAK_MAX_ENERGY_Q8 *
        (int)SC64_Z6_ABILITIES_HERO_INITIAL_ENERGY_PERCENT / 100;
    assert(energy==250*256);
    assert(sc64_z5_cloak_toggle(&cloak,&energy)==SC64_Z5_CLOAK_ADVANCED);
    assert(cloak.active && energy==225*256);
    assert(sc64_z5_cloak_toggle(&cloak,&energy)==SC64_Z5_CLOAK_ADVANCED);
    assert(!cloak.active && energy==225*256);
}

static void test_source_cadence_regeneration_and_save_resume(void)
{
    Sc64Z5CloakState live={0,0}, resumed;
    int energy=0, resumed_energy;
    for (unsigned i=0;i<4;i++)
        assert(sc64_z5_cloak_tick(&live,&energy,0)==SC64_Z5_CLOAK_ADVANCED);
    assert(energy==25 && live.regen_remainder==3);
    resumed=live; resumed_energy=energy;
    for (unsigned i=0;i<1000;i++) {
        int a=sc64_z5_cloak_tick(&live,&energy,0);
        int b=sc64_z5_cloak_tick(&resumed,&resumed_energy,0);
        assert(a==b && energy==resumed_energy);
        assert(memcmp(&live,&resumed,sizeof(live))==0);
    }
}

static void test_pause_and_depletion(void)
{
    Sc64Z5CloakState cloak={1,0};
    int energy=100*256;
    assert(sc64_z5_cloak_tick(&cloak,&energy,1)==SC64_Z5_CLOAK_UNCHANGED);
    assert(energy==100*256 && cloak.active);
    energy=15;
    assert(sc64_z5_cloak_tick(&cloak,&energy,0)==SC64_Z5_CLOAK_ADVANCED);
    assert(energy==7 && cloak.active);
    assert(sc64_z5_cloak_tick(&cloak,&energy,0)==SC64_Z5_CLOAK_ADVANCED);
    assert(energy==7 && !cloak.active);
}

static void test_invalid_saved_state_is_transactional(void)
{
    Sc64Z5CloakState cloak={1,1}, before=cloak;
    int energy=17*256;
    assert(!sc64_z5_cloak_state_valid(&cloak));
    assert(sc64_z5_cloak_tick(&cloak,&energy,0)==SC64_Z5_CLOAK_ERROR);
    assert(memcmp(&cloak,&before,sizeof(cloak))==0 && energy==17*256);
    cloak=(Sc64Z5CloakState){0,5}; before=cloak;
    assert(sc64_z5_cloak_toggle(&cloak,&energy)==SC64_Z5_CLOAK_ERROR);
    assert(memcmp(&cloak,&before,sizeof(cloak))==0 && energy==17*256);
}

int main(void)
{
    test_source_start_and_toggle();
    test_source_cadence_regeneration_and_save_resume();
    test_pause_and_depletion();
    test_invalid_saved_state_is_transactional();
    return 0;
}
