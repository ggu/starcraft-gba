#include <assert.h>
#include "../starcraft/race_skirmish.h"

int main(void)
{
    Sc64RaceState z, p;
    Sc64RaceBank bank = { 100, 0 };
    unsigned short shields = 0, remainder = 0;
    assert(sc64_race_unit_rule(U_DRONE)->minerals == 50);
    assert(sc64_race_unit_rule(U_PROBE)->race == R_PROTOSS);
    assert(sc64_race_building_rule(B_GATEWAY)->prereq & SC_PR_PYLON);
    assert(sc64_race_unit_damage_vs(U_ZEALOT, 0) == 8);
    assert(sc64_race_unit_hits_vs(U_ZEALOT, 0) == 2);
    assert(sc64_race_unit_damage_vs(U_FENIX, 0) == 20);
    assert(sc64_race_unit_hits_vs(U_FENIX, 0) == 2);
    assert(sc64_race_unit_damage_vs(U_FENIX_DRAGOON, 0) == 45);
    assert(sc64_race_unit_damage_vs(U_SCOUT, 1) == 14);
    assert(sc64_race_unit_range_vs(U_GOLIATH, 0) == 192);
    assert(sc64_race_unit_range_vs(U_GOLIATH, 1) == 160);
    assert(sc64_race_unit_armor(U_FENIX_DRAGOON) == 3);
    assert(sc64_race_unit_armor(U_GOLIATH) == 1);
    assert(sc64_race_building_armor(B_SUNKEN_COLONY) == 2);
    assert(sc64_race_building_armor(B_PYLON) == 0);
    assert(sc64_race_unit_from_raw(30) == U_SIEGE_TANK);
    assert(sc64_race_pay_unit(&bank, U_ZERGLING));
    assert(bank.minerals == 50 && bank.gas == 0);
    assert(!sc64_race_pay_unit(&bank, U_HYDRALISK)); /* No gas, no free unit. */
    assert(bank.minerals == 50 && bank.gas == 0);
    bank.minerals = 75; bank.gas = 25;
    assert(sc64_race_pay_unit(&bank, U_HYDRALISK));
    assert(bank.minerals == 0 && bank.gas == 0);
    assert(!sc64_race_pay_building(&bank, B_SPAWNING_POOL));

    sc64_race_state_init(&z, R_ZERG, 4, 12, 1, 2);
    assert(sc64_race_can_produce(&z, U_ZERGLING, 1));
    assert(sc64_race_morph(&z, U_ZERGLING, 1));
    assert(z.larva == 1 && z.supply_used == 6); /* One egg, two lings. */
    assert(sc64_race_morph(&z, U_ZERGLING, 1));
    assert(!sc64_race_morph(&z, U_ZERGLING, 1)); /* No free larva. */
    assert(sc64_race_build_consumes_worker(B_SPAWNING_POOL));
    assert(sc64_race_build_complete(&z, B_HATCHERY));
    assert(z.hatcheries == 2 && z.larva == 1);
    sc64_race_unit_died(&z, U_ZERGLING, 2);
    assert(z.supply_used == 4);
    sc64_race_larva_tick(&z);
    assert(z.larva == 1);
    for (int i = 0; i < 684; ++i) sc64_race_larva_tick(&z);
    assert(z.larva == 3);

    sc64_race_state_init(&p, R_PROTOSS, 2, 16, 0, 0);
    assert(sc64_race_morph(&p, U_ZEALOT, 1));
    assert(p.supply_used == 6);
    assert(!sc64_race_build_consumes_worker(B_GATEWAY)); /* Probe warps away then returns. */
    assert(sc64_race_build_complete(&p, B_PYLON));
    assert(p.supply_cap == 32);
    sc64_race_set_pylon_power(&p, 1);
    assert(p.powered);
    assert(sc64_race_shield_tick(&shields, 60, &remainder));
    for (int i = 1; i < 2200 && shields == 0; ++i)
        sc64_race_shield_tick(&shields, 60, &remainder);
    assert(shields > 0);
    return 0;
}
