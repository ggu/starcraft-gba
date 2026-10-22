#include <assert.h>
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_p3.h"

int main(void)
{
    assert(SC64_P3_GLOBAL_ID == 22);
    assert(SC64_P3_WIDTH_TILES == 128 && SC64_P3_HEIGHT_TILES == 96);
    assert(SC64_P3_HUMAN_SLOT == 7 && SC64_P3_FORCE2_SLOT == 0 &&
           SC64_P3_FORCE3_SLOT == 6);

    /* Pinned BWAPI raw Scourge facts; its suicide collision is not simulated. */
    assert(sc64_race_unit_from_raw(47) == U_SCOURGE);
    assert(sc64_race_raw_unit(U_SCOURGE) == 47);
    assert(sc64_race_unit_rule(U_SCOURGE)->hp == 25);
    assert(sc64_race_unit_rule(U_SCOURGE)->minerals == 25);
    assert(sc64_race_unit_rule(U_SCOURGE)->gas == 75);
    assert(sc64_race_unit_rule(U_SCOURGE)->build_frames == 450);
    assert(sc64_race_unit_rule(U_SCOURGE)->supply == 1);
    assert(sc64_race_unit_flying(U_SCOURGE));
    assert(sc64_race_unit_damage_vs(U_SCOURGE, 1) == 0);

    assert(sc64_race_building_from_raw(133) == B_HIVE);
    assert(sc64_race_building_rule(B_HIVE)->hp == 2500);
    assert(sc64_race_building_armor(B_HIVE) == 1);
    assert(sc64_race_building_from_raw(137) == B_GREATER_SPIRE);
    assert(sc64_race_building_rule(B_GREATER_SPIRE)->hp == 1000);
    assert(sc64_race_building_armor(B_GREATER_SPIRE) == 1);

    assert(sc64_p3_source_conditions[0].player_group == SC64_P3_TRIGGER_CURRENT_PLAYER);
    assert(sc64_p3_source_conditions[0].unit_type == SC64_P3_RAW_ANY_UNIT);
    assert(sc64_p3_source_conditions[0].amount == 0);
    assert(sc64_p3_source_conditions[0].comparison == SC64_P3_CONDITION_AT_MOST);
    assert(sc64_p3_source_rewards[0].group1 == SC64_P3_TRIGGER_CURRENT_PLAYER);
    assert(sc64_p3_source_rewards[0].group2 == SC64_P3_HUMAN_MINERALS);
    assert(sc64_p3_source_rewards[1].group2 == SC64_P3_HUMAN_GAS);
    assert(sc64_p3_source_foes_zero_actions[0].time_ms == 2000);
    assert(sc64_p3_source_foes_zero_actions[1].opcode == SC64_P3_ACTION_TRANSMISSION);
    assert(sc64_p3_source_foes_zero_actions[2].opcode == SC64_P3_ACTION_TRANSMISSION);
    assert(sc64_p3_source_foes_zero_actions[3].time_ms == 3000);
    assert(sc64_p3_source_foes_zero_actions[4].opcode == SC64_P3_ACTION_VICTORY);
    assert(sc64_p3_source_force3_actions[0].group2 == SC64_P3_FORCE3_COMBINED_RESOURCES);
    assert(sc64_p3_source_force2_actions[0].group2 == SC64_P3_FORCE2_COMBINED_RESOURCES);
    assert(sc64_p3_source_force3_actions[1].group2 == SC64_P3_FORCE3_SCRIPT_DWORD);
    assert(sc64_p3_source_force2_actions[1].group2 == SC64_P3_FORCE2_SCRIPT_DWORD);
    return 0;
}
