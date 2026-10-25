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
    assert(sc64_race_unit_cooldown_vs(U_FENIX, 0) == 22);
    assert(sc64_race_unit_damage_vs(U_FENIX_DRAGOON, 0) == 45);
    assert(sc64_race_unit_damage(U_SIEGE_TANK) == 70);
    assert(sc64_race_unit_range(U_HUNTER_KILLER) == 160);
    assert(sc64_race_unit_range(U_SCV) == 10);
    assert(sc64_race_unit_range(U_DRONE) == 32);
    assert(sc64_race_unit_range(U_PROBE) == 32);
    assert(sc64_race_unit_cooldown(U_DRONE) == 22);
    assert(sc64_race_unit_cooldown(U_PROBE) == 22);
    assert(sc64_race_unit_hits_vs(U_GOLIATH, 1) == 1);
    assert(sc64_race_unit_hits_vs(U_SCOUT, 1) == 1);
    assert(sc64_race_unit_damage_vs(U_SCOUT, 1) == 14);
    assert(sc64_race_unit_range_vs(U_GOLIATH, 0) == 192);
    assert(sc64_race_unit_range_vs(U_GOLIATH, 1) == 160);
    assert(sc64_race_unit_armor(U_FENIX_DRAGOON) == 3);
    assert(sc64_race_unit_armor(U_GOLIATH) == 1);
    /* BWAPI Terran Vulture: 75/0, 80 HP, 2 supply, factory, 450 frames. */
    assert(U_VULTURE < U_DROPSHIP && U_DROPSHIP == U_SCOURGE - 1);
    assert(sc64_race_unit_rule(U_VULTURE)->race == R_TER);
    assert(sc64_race_unit_rule(U_VULTURE)->producer == B_FACTORY);
    assert(sc64_race_unit_rule(U_VULTURE)->minerals == 75);
    assert(sc64_race_unit_rule(U_VULTURE)->gas == 0);
    assert(sc64_race_unit_rule(U_VULTURE)->build_frames == 450);
    assert(sc64_race_unit_rule(U_VULTURE)->hp == 80);
    assert(sc64_race_unit_rule(U_VULTURE)->supply == 4);
    assert(sc64_race_unit_armor(U_VULTURE) == 0);
    assert(sc64_race_unit_damage_vs(U_VULTURE, 0) == 20);
    assert(sc64_race_unit_damage_vs(U_VULTURE, 1) == 0);
    assert(sc64_race_unit_range_vs(U_VULTURE, 0) == 160);
    assert(sc64_race_unit_range_vs(U_VULTURE, 1) == 0);
    assert(sc64_race_unit_cooldown_vs(U_VULTURE, 0) == 30);
    assert(sc64_race_raw_unit(U_VULTURE) == 2);
    assert(sc64_race_unit_from_raw(2) == U_VULTURE);
    /* The non-T4 compact ruleset retains its legacy Raynor alias. */
    assert(sc64_race_unit_from_raw(20) == U_RAYNOR);
    assert(sc64_race_raw_unit(U_RAYNOR) == 20);
#ifdef SC64_UNIFIED
    assert(sc64_race_unit_from_raw(19) == U_RAYNOR);
#endif
    /* Pinned BWAPI raw 11: Dropship is a distinct flying, unarmed unit.
     * Transport/cargo behavior is deliberately outside this rules contract. */
    assert(sc64_race_unit_rule(U_DROPSHIP)->race == R_TER);
    assert(sc64_race_unit_rule(U_DROPSHIP)->producer == B_STARPORT);
    assert(sc64_race_unit_rule(U_DROPSHIP)->minerals == 100);
    assert(sc64_race_unit_rule(U_DROPSHIP)->gas == 100);
    assert(sc64_race_unit_rule(U_DROPSHIP)->build_frames == 750);
    assert(sc64_race_unit_rule(U_DROPSHIP)->hp == 150);
    assert(sc64_race_unit_rule(U_DROPSHIP)->supply == 4);
    assert(sc64_race_unit_armor(U_DROPSHIP) == 1);
    assert(sc64_race_unit_flying(U_DROPSHIP));
    assert(sc64_race_unit_damage_vs(U_DROPSHIP, 0) == 0);
    assert(sc64_race_unit_damage_vs(U_DROPSHIP, 1) == 0);
    assert(sc64_race_unit_range_vs(U_DROPSHIP, 0) == 0);
    assert(sc64_race_unit_range_vs(U_DROPSHIP, 1) == 0);
    assert(sc64_race_raw_unit(U_DROPSHIP) == 11);
    assert(U_SCOURGE == SC_RU_COUNT - 1 && U_DROPSHIP == U_SCOURGE - 1);
    assert(sc64_race_unit_from_raw(47) == U_SCOURGE);
    assert(sc64_race_raw_unit(U_SCOURGE) == 47);
    assert(sc64_race_unit_rule(U_SCOURGE)->race == R_ZERG);
    assert(sc64_race_unit_rule(U_SCOURGE)->minerals == 25);
    assert(sc64_race_unit_rule(U_SCOURGE)->gas == 75);
    assert(sc64_race_unit_rule(U_SCOURGE)->build_frames == 450);
    assert(sc64_race_unit_rule(U_SCOURGE)->hp == 25);
    assert(sc64_race_unit_rule(U_SCOURGE)->supply == 1);
    assert(sc64_race_unit_flying(U_SCOURGE));
    assert(sc64_race_unit_damage_vs(U_SCOURGE, 1) == 0); /* Suicide attack not simulated. */
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    assert(sc64_race_unit_from_raw(11) == U_DROPSHIP);
#else
    assert(sc64_race_unit_from_raw(11) == -1);
#endif
    assert(sc64_race_raw_building(B_ENG_BAY) == 122);
    assert(sc64_race_building_from_raw(122) == B_ENG_BAY);
    assert(sc64_race_building_from_raw(118) == -1);
    /* Raw 150 is the Mature Chrysalis building; raw 216 remains the Young Chrysalis unit. */
    assert(sc64_race_unit_from_raw(150) == -1);
    assert(sc64_race_building_from_raw(150) == B_MATURE_CHRYSALIS);
    assert(sc64_race_raw_building(B_MATURE_CHRYSALIS) == 150);
    assert(sc64_race_building_rule(B_MATURE_CHRYSALIS)->flags & SC_RACE_F_BUILDING);
    assert(sc64_race_building_rule(B_MATURE_CHRYSALIS)->hp == 250);
    assert(sc64_race_building_armor(B_MATURE_CHRYSALIS) == 1);
    assert(sc64_race_unit_from_raw(216) == U_CHRYSALIS);
    assert(sc64_race_unit_rule(U_CHRYSALIS)->hp == 800);
    assert(sc64_race_building_from_raw(133) == B_HIVE);
    assert(sc64_race_raw_building(B_HIVE) == 133);
    assert(sc64_race_building_rule(B_HIVE)->hp == 2500);
    assert(sc64_race_building_armor(B_HIVE) == 1);
    for (int n = 0; n < 3; ++n) {
        static const int halls[] = { B_HATCHERY, B_LAIR, B_HIVE };
        int hall = halls[n];
        assert(sc64_race_is_zerg_townhall(hall));
        assert(sc64_race_producer_compatible(hall, B_HATCHERY));
        assert(sc64_race_building_rule(hall)->supply_provided == 2);
        assert(sc64_race_building_rule(hall)->flags & SC_RACE_F_PRODUCER);
    }
    assert(!sc64_race_is_zerg_townhall(B_GREATER_SPIRE));
    assert(!sc64_race_producer_compatible(B_NEXUS, B_HATCHERY));
    assert(sc64_race_building_from_raw(137) == B_GREATER_SPIRE);
    assert(sc64_race_raw_building(B_GREATER_SPIRE) == 137);
    assert(sc64_race_building_rule(B_GREATER_SPIRE)->hp == 1000);
    assert(sc64_race_building_armor(B_GREATER_SPIRE) == 1);
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
