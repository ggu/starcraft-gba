/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_N64_STOCK_OVERRIDES_H
#define SC64_N64_STOCK_OVERRIDES_H

/* Cells of sc64_stock_profiles.h that follow the StarCraft 64 rule tables
 * instead of the pinned BWAPI default tables (owner decision, 2026-11-26:
 * the ROM-derived profiles follow the N64 values). The profile tables hold
 * the N64 value; this list keeps the BWAPI value beside it so the BWAPI
 * facts check (tools/test_sc64_stock_profiles.py) still verifies every other
 * cell exactly and these cells as recorded departures. Each row is one cell
 * the ROM-table differ (tools/sc64_parity_tables.py) reported.
 * Unit top speeds are not listed yet: the N64 values change the Terran
 * campaign controllers' outcomes, so they wait for retuned controllers.
 * Rows are { table, raw row, field, BWAPI value, N64 value }; field numbers
 * are positions in the profile structs, in declaration order.
 */
enum { SC64_STOCK_OVERRIDE_UNIT = 0, SC64_STOCK_OVERRIDE_WEAPON = 1 };
enum { SC64_STOCK_U_FLAGS = 0,
       SC64_STOCK_U_GROUND_WEAPON,
       SC64_STOCK_U_AIR_WEAPON,
       SC64_STOCK_U_GROUND_HITS,
       SC64_STOCK_U_AIR_HITS,
       SC64_STOCK_U_SIZE,
       SC64_STOCK_U_ARMOR,
       SC64_STOCK_U_MINERALS,
       SC64_STOCK_U_GAS,
       SC64_STOCK_U_BUILD_FRAMES,
       SC64_STOCK_U_SUPPLY_REQUIRED,
       SC64_STOCK_U_SUPPLY_PROVIDED,
       SC64_STOCK_U_HP,
       SC64_STOCK_U_SHIELDS,
       SC64_STOCK_U_TOP_SPEED_Q10 };
enum { SC64_STOCK_W_DAMAGE = 0,
       SC64_STOCK_W_UPGRADE_BONUS,
       SC64_STOCK_W_COOLDOWN,
       SC64_STOCK_W_MIN_RANGE,
       SC64_STOCK_W_MAX_RANGE,
       SC64_STOCK_W_DAMAGE_FACTOR,
       SC64_STOCK_W_DAMAGE_CLASS };
typedef struct { unsigned short table, row, field;
                 unsigned int bwapi, n64; } Sc64StockOverride;
static const Sc64StockOverride sc64_n64_stock_overrides[] = {
    { SC64_STOCK_OVERRIDE_UNIT, 12, SC64_STOCK_U_BUILD_FRAMES, 2000, 2400 }, /* Terran_Battlecruiser build_frames */
    { SC64_STOCK_OVERRIDE_UNIT, 12, SC64_STOCK_U_SUPPLY_REQUIRED, 12, 16 }, /* Terran_Battlecruiser supply_required */
    { SC64_STOCK_OVERRIDE_UNIT, 39, SC64_STOCK_U_SUPPLY_REQUIRED, 8, 12 }, /* Zerg_Ultralisk supply_required */
    { SC64_STOCK_OVERRIDE_UNIT, 45, SC64_STOCK_U_GAS, 100, 150 }, /* Zerg_Queen gas */
    { SC64_STOCK_OVERRIDE_UNIT, 58, SC64_STOCK_U_BUILD_FRAMES, 750, 900 }, /* Terran_Valkyrie build_frames */
    { SC64_STOCK_OVERRIDE_UNIT, 65, SC64_STOCK_U_HP, 100, 80 }, /* Protoss_Zealot hp */
    { SC64_STOCK_OVERRIDE_UNIT, 65, SC64_STOCK_U_SHIELDS, 60, 80 }, /* Protoss_Zealot shields */
    { SC64_STOCK_OVERRIDE_UNIT, 66, SC64_STOCK_U_BUILD_FRAMES, 750, 600 }, /* Protoss_Dragoon build_frames */
    { SC64_STOCK_OVERRIDE_UNIT, 70, SC64_STOCK_U_MINERALS, 275, 300 }, /* Protoss_Scout minerals */
    { SC64_STOCK_OVERRIDE_UNIT, 70, SC64_STOCK_U_GAS, 125, 150 }, /* Protoss_Scout gas */
    { SC64_STOCK_OVERRIDE_UNIT, 72, SC64_STOCK_U_SUPPLY_REQUIRED, 12, 16 }, /* Protoss_Carrier supply_required */
    { SC64_STOCK_OVERRIDE_UNIT, 112, SC64_STOCK_U_MINERALS, 150, 200 }, /* Terran_Academy minerals */
    { SC64_STOCK_OVERRIDE_UNIT, 116, SC64_STOCK_U_MINERALS, 100, 150 }, /* Terran_Science_Facility minerals */
    { SC64_STOCK_OVERRIDE_UNIT, 116, SC64_STOCK_U_GAS, 150, 200 }, /* Terran_Science_Facility gas */
    { SC64_STOCK_OVERRIDE_UNIT, 116, SC64_STOCK_U_BUILD_FRAMES, 900, 1200 }, /* Terran_Science_Facility build_frames */
    { SC64_STOCK_OVERRIDE_UNIT, 124, SC64_STOCK_U_MINERALS, 75, 100 }, /* Terran_Missile_Turret minerals */
    { SC64_STOCK_OVERRIDE_UNIT, 142, SC64_STOCK_U_MINERALS, 200, 150 }, /* Zerg_Spawning_Pool minerals */
    { SC64_STOCK_OVERRIDE_UNIT, 146, SC64_STOCK_U_ARMOR, 2, 0 }, /* Zerg_Sunken_Colony armor */
    { SC64_STOCK_OVERRIDE_UNIT, 146, SC64_STOCK_U_HP, 300, 400 }, /* Zerg_Sunken_Colony hp */
    { SC64_STOCK_OVERRIDE_WEAPON, 7, SC64_STOCK_W_MAX_RANGE, 192, 160 }, /* Twin_Autocannons max_range */
    { SC64_STOCK_OVERRIDE_WEAPON, 84, SC64_STOCK_W_DAMAGE, 14, 16 }, /* Psionic_Storm damage */
    { SC64_STOCK_OVERRIDE_WEAPON, 103, SC64_STOCK_W_DAMAGE, 6, 5 }, /* Halo_Rockets damage */
};
enum { SC64_N64_STOCK_OVERRIDE_COUNT =
       (int)(sizeof sc64_n64_stock_overrides / sizeof sc64_n64_stock_overrides[0]) };
#endif
