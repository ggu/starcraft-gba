#include <assert.h>
#include "../starcraft/race_rules.h"

int main(void)
{
#ifndef SC64_TERRAN_LATE_ACTORS
    /* The feature-off compact IDs remain exactly the pre-extension values. */
    _Static_assert(U_SCOURGE == 27, "legacy unit IDs changed");
    _Static_assert(B_GREATER_SPIRE == 34, "legacy building IDs changed");
#if !defined(SC64_T4_RUNTIME) && !defined(SC64_T5_RUNTIME) && !defined(SC64_T6_ENABLED)
    _Static_assert(SC_RU_COUNT == 28, "legacy unit count changed");
    _Static_assert(SC_RB_COUNT == 35, "legacy building count changed");
#endif
#ifdef SC64_T4_RUNTIME
    _Static_assert(U_RAYNOR_MARINE == 34, "T4 unit IDs changed");
    _Static_assert(B_RIGHT_WALL_MISSILE_TRAP == 43, "T4 building IDs changed");
#endif
#ifdef SC64_T5_RUNTIME
    _Static_assert(U_SPIDER_MINE == 37, "T5 unit IDs changed");
    _Static_assert(B_CONTROL_TOWER == 44, "T5 building IDs changed");
#endif
#ifdef SC64_T6_ENABLED
    _Static_assert(B_QUEENS_NEST == 46, "T6 building IDs changed");
#endif
    assert(sc64_race_unit_from_raw(9) == -1);
    assert(sc64_race_building_from_raw(127) == -1);
#else
    static const struct { int raw, type; } units[] = {
        {9,U_SCIENCE_VESSEL},{12,U_BATTLECRUISER},{29,U_NORAD_II},
        {39,U_ULTRALISK},{69,U_SHUTTLE},{217,U_PSI_EMITTER}
    };
    static const int unit_expect[][14] = {
        {200,0,1,100,225,1200,4,320,24,16,24,20,1,0},
        {500,0,3,400,300,2000,12,352,37,29,37,29,1,25},
        {700,0,4,800,600,4800,0,352,37,29,37,29,1,50},
        {400,0,1,200,200,900,8,224,19,16,18,15,0,20},
        {80,60,1,200,0,900,4,256,20,16,19,15,1,0},
        {800,0,0,1,1,1,0,160,16,16,15,15,0,0}
    };
    static const int unit_footprints[][2] = {{2,2},{2,2},{2,2},{2,2},{2,1},{1,1}};
    for (unsigned i=0; i<sizeof(units)/sizeof(units[0]); ++i) {
        const Sc64TerranLateActorFact *f = sc64_terran_late_unit_fact(units[i].type);
        assert(f && f->raw == units[i].raw);
        const int actual[] = {f->hp,f->shields,f->armor,f->minerals,f->gas,
            f->build_frames,f->supply,f->sight_px,f->left,f->up,f->right,
            f->down,f->flying,f->ground_damage};
        for (unsigned k=0; k<sizeof(actual)/sizeof(actual[0]); ++k)
            assert(actual[k] == unit_expect[i][k]);
        assert(f->footprint_width_tiles == unit_footprints[i][0]);
        assert(f->footprint_height_tiles == unit_footprints[i][1]);
        assert(sc64_race_unit_from_raw(units[i].raw) == units[i].type);
        assert(sc64_race_raw_unit(units[i].type) == units[i].raw);
        assert(sc64_race_unit_rule(units[i].type)->producer == 255);
        assert(sc64_race_unit_rule(units[i].type)->hp == f->hp);
        assert(sc64_race_unit_armor(units[i].type) == f->armor);
        assert(sc64_terran_late_unit_default_hp(units[i].type) == f->hp);
        assert(sc64_terran_late_unit_sight_source_pixels(units[i].type) == f->sight_px);
        int l=-1,u=-1,r=-1,d=-1;
        assert(sc64_terran_late_actor_bounds(units[i].type,0,&l,&u,&r,&d));
        assert(l==f->left && u==f->up && r==f->right && d==f->down);
        assert(sc64_race_unit_flying(units[i].type) == f->flying);
        assert(sc64_race_unit_damage_vs(units[i].type,0) == f->ground_damage);
        assert(sc64_race_unit_damage_vs(units[i].type,1) == f->air_damage);
        assert(sc64_race_unit_range_vs(units[i].type,0) == f->ground_range);
        assert(sc64_race_unit_range_vs(units[i].type,1) == f->air_range);
        assert(sc64_race_unit_cooldown_vs(units[i].type,0) == f->ground_cooldown);
        assert(sc64_race_unit_cooldown_vs(units[i].type,1) == f->air_cooldown);
        assert(sc64_race_unit_hits_vs(units[i].type,0) == f->ground_hits);
        assert(sc64_race_unit_hits_vs(units[i].type,1) == f->air_hits);
        assert(sc64_terran_late_unit_actor_flags(units[i].type));
    }
    assert(sc64_race_unit_damage_vs(U_BATTLECRUISER,0)==25);
    assert(sc64_race_unit_damage_vs(U_BATTLECRUISER,1)==25);
    assert(sc64_race_unit_damage_vs(U_NORAD_II,0)==50);
    assert(sc64_race_unit_damage_vs(U_ULTRALISK,1)==0);
    assert(sc64_race_unit_can_attack_air(U_BATTLECRUISER));
    assert(!sc64_race_unit_can_attack_air(U_SCIENCE_VESSEL));
    assert(sc64_race_unit_flying(U_NORAD_II) && sc64_race_raw_unit(U_NORAD_II)!=126);
    assert(sc64_race_unit_from_raw(126)==-1);
    assert(sc64_race_unit_rule(U_NORAD_II)->minerals==800);
    assert(sc64_race_unit_rule(U_NORAD_II)->gas==600);
    assert(sc64_race_unit_rule(U_PSI_EMITTER)->minerals==1);
    assert(sc64_terran_late_unit_actor_flags(U_PSI_EMITTER) & SC64_LATE_ACTOR_INVINCIBLE);
    assert(!(sc64_terran_late_unit_actor_flags(U_PSI_EMITTER) & SC64_LATE_ACTOR_WEAPON));

    static const struct { int raw,type; } buildings[] = {
        {108,B_NUCLEAR_SILO},{117,B_COVERT_OPS},{120,B_MACHINE_SHOP},
        {123,B_ARMORY},{136,B_DEFILER_MOUND},{140,B_ULTRALISK_CAVERN},
        {163,B_CITADEL_ADUN},{168,B_STASIS_CELL_PRISON},{169,B_FLEET_BEACON},
        {171,B_ROBOTICS_SUPPORT_BAY},{172,B_SHIELD_BATTERY},{127,B_SPECIAL_ION_CANNON}
    };
    static const int building_expect[][13] = {
        {600,0,1,100,100,1200,256,37,16,31,25,0,0},
        {750,0,1,50,50,600,256,47,24,28,22,0,0},
        {750,0,1,50,50,600,256,39,24,31,24,0,0},
        {750,0,1,100,50,1200,256,48,32,47,22,0,0},
        {850,0,1,100,100,900,256,48,32,48,4,0,0},
        {600,0,1,150,200,1200,256,40,32,32,31,0,0},
        {450,450,1,150,100,900,320,24,24,40,24,0,0},
        {2000,0,1,150,0,1,256,64,48,63,47,0,0},
        {500,500,1,300,200,900,320,40,32,47,24,0,0},
        {450,450,1,150,100,450,320,32,32,32,20,0,0},
        {200,200,1,100,0,450,320,32,16,32,16,0,0},
        {2000,0,1,200,0,900,256,48,32,47,31,0,0}
    };
    static const int building_footprints[][2] = {
        {2,2},{2,2},{2,2},{3,2},{4,2},{3,2},{3,2},{4,3},{3,2},{3,2},{3,2},{3,2}
    };
    for (unsigned i=0; i<sizeof(buildings)/sizeof(buildings[0]); ++i) {
        const Sc64TerranLateActorFact *f = sc64_terran_late_building_fact(buildings[i].type);
        assert(f && f->raw == buildings[i].raw);
        const int actual[] = {f->hp,f->shields,f->armor,f->minerals,f->gas,
            f->build_frames,f->sight_px,f->left,f->up,f->right,f->down,
            f->ground_damage,f->air_damage};
        for (unsigned k=0; k<sizeof(actual)/sizeof(actual[0]); ++k)
            assert(actual[k] == building_expect[i][k]);
        assert(f->footprint_width_tiles == building_footprints[i][0]);
        assert(f->footprint_height_tiles == building_footprints[i][1]);
        assert(sc64_race_building_from_raw(buildings[i].raw)==buildings[i].type);
        assert(sc64_race_raw_building(buildings[i].type)==buildings[i].raw);
        assert(sc64_race_building_rule(buildings[i].type)->worker==255);
        assert(sc64_terran_late_building_default_hp(buildings[i].type)==f->hp);
        assert(sc64_race_building_armor(buildings[i].type)==f->armor);
        assert(sc64_race_building_shields(buildings[i].type)==f->shields);
        assert(sc64_terran_late_building_sight_source_pixels(buildings[i].type)==f->sight_px);
        int l=-1,u=-1,r=-1,d=-1;
        assert(sc64_terran_late_actor_bounds(buildings[i].type,1,&l,&u,&r,&d));
        assert(l==f->left && u==f->up && r==f->right && d==f->down);
        assert(sc64_terran_late_building_actor_flags(buildings[i].type) & SC64_LATE_ACTOR_BUILDING);
    }
#ifdef SC64_T6_ENABLED
    assert(sc64_race_building_from_raw(126)==B_CRASHED_NORAD_II);
#else
    assert(sc64_race_building_from_raw(126)==-1);
#endif
    assert(sc64_race_building_from_raw(127)==B_SPECIAL_ION_CANNON);
    assert(sc64_race_building_armor(B_SPECIAL_ION_CANNON)==1);
    assert(sc64_race_unit_damage(U_BATTLECRUISER)==25);
    assert(sc64_race_unit_from_raw(127)==-1); /* Ion Cannon is never a unit. */
#endif
    return 0;
}
