/* The crashed objective actor must not acquire flying-hero behavior. */
#include <assert.h>
#include <string.h>
#include "../starcraft/race_rules.h"

int main(void)
{
    assert(sc64_race_unit_from_raw(126) == -1);
    assert(sc64_race_unit_from_raw(138) == -1);
    assert(sc64_race_unit_from_raw(29) == -1);
#ifdef SC64_T6_ENABLED
    const Sc64T6BuildingFact *ship = sc64_t6_building_fact(126);
    const Sc64T6BuildingFact *nest = sc64_t6_building_fact(138);
    assert(ship && nest && ship != nest);
    assert(!ship->flying && !ship->ground_weapon && !ship->air_weapon);
    assert(ship->hp == 700 && ship->armor == 1 && ship->sight_pixels == 320);
    assert(ship->tile_width == 3 && ship->tile_height == 2);
    assert(ship->left == 48 && ship->right == 47 && ship->up == 32 && ship->down == 31);
    assert(nest->hp == 850 && nest->armor == 1 && nest->sight_pixels == 256);
    assert(nest->minerals == 150 && nest->gas == 100 && nest->build_frames == 900);
    assert(nest->tile_width == 3 && nest->tile_height == 2);
    assert(nest->left == 38 && nest->right == 32 && nest->up == 28 && nest->down == 28);
    assert(sc64_t6_building_fact(29) == 0);
    assert(sc64_t6_building_fact(255) == 0);
    assert(sc64_t6_building_fact(65535) == 0);
    assert(sc64_race_building_from_raw(126) == B_CRASHED_NORAD_II);
    assert(sc64_race_building_from_raw(138) == B_QUEENS_NEST);
    assert(sc64_race_raw_building(B_CRASHED_NORAD_II) == 126);
    assert(sc64_race_raw_building(B_QUEENS_NEST) == 138);
    assert(sc64_race_building_armor(B_CRASHED_NORAD_II) == 1);
    assert(sc64_race_building_armor(B_QUEENS_NEST) == 1);
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(B_CRASHED_NORAD_II);
    assert(rule && rule->hp == 700 && rule->worker == 255);
    assert(!(rule->flags & SC_RACE_F_PRODUCER) && !rule->build_frames);
    rule = sc64_race_building_rule(B_QUEENS_NEST);
    assert(rule && rule->hp == 850 && rule->race == R_ZERG && rule->worker == U_DRONE);
    assert(rule->flags & SC_RACE_F_CONSUMES_WORKER);
    assert(strcmp(sc64_race_building_name(B_CRASHED_NORAD_II), "CRASHED NORAD II") == 0);
#else
    assert(sc64_race_building_from_raw(126) == -1);
    assert(sc64_race_building_from_raw(138) == -1);
#endif
    /* Existing identities stay unchanged with the additional data enabled. */
    assert(sc64_race_raw_building(B_TERRAN_BEACON) == 195);
    assert(sc64_race_raw_building(B_CONTROL_TOWER) == 115);
    assert(sc64_race_unit_from_raw(19) == U_RAYNOR_VULTURE);
    assert(sc64_race_unit_from_raw(11) == U_DROPSHIP);
    return 0;
}
