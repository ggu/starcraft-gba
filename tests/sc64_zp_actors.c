#include <assert.h>
#include <string.h>
#include "../starcraft/race_rules.h"

int main(void)
{
#ifndef SC64_ZP_CAMPAIGN_ACTORS
    _Static_assert(U_SCOURGE == 27 && SC_RU_COUNT == 28,
                   "Z/P actor gate changed legacy unit ids");
    _Static_assert(B_GREATER_SPIRE == 34 && SC_RB_COUNT == 35,
                   "Z/P actor gate changed legacy building ids");
    assert(sc64_race_unit_from_raw(51) == -1);
    assert(sc64_race_unit_from_raw(101) == -1);
#else
    /* Golden values transcribed independently from pinned BWAPI UnitType.cpp
       and WeaponType.cpp at d727fed68558c506163048ea889131d8cbb33915. */
    static const struct { int raw, type, hp, shields, armor, mins, gas, frames,
        sight, left, up, right, down, flying, size, damage, bonus, cooldown,
        range, hits; } unit_map[] = {
        /* Hero_Infested_Kerrigan, raw 51 */
        {51,U_ZP_INFESTED_KERRIGAN,400,0,2,200,300,1500,288,7,10,7,11,0,0,50,1,15,15,1},
        /* Special_Map_Revealer, raw 101 */
        {101,U_ZP_MAP_REVEALER,1,0,0,0,0,1,320,13,13,13,17,1,3,0,0,0,0,0},
        /* Hero_Jim_Raynor_Vulture, raw 19 */
        {19,U_ZP_RAYNOR_VULTURE,300,0,3,150,0,900,256,16,16,15,15,0,1,30,2,22,160,1},
        /* Protoss_High_Templar, raw 67 */
        {67,U_ZP_HIGH_TEMPLAR,40,40,0,50,150,750,224,12,10,11,13,0,0,0,0,0,0,0},
        /* Zerg_Ultralisk, raw 39 */
        {39,U_ZP_ULTRALISK,400,0,1,200,200,900,224,19,16,18,15,0,2,20,3,15,25,1}
    };
    for (unsigned i = 0; i < sizeof(unit_map)/sizeof(unit_map[0]); ++i) {
        assert(sc64_race_unit_from_raw(unit_map[i].raw) == unit_map[i].type);
        assert(sc64_race_raw_unit(unit_map[i].type) == unit_map[i].raw);
        const Sc64ZpActorFact *f = sc64_zp_actor_fact(unit_map[i].type);
        assert(f && f->raw == unit_map[i].raw);
        assert(f->hp==unit_map[i].hp && f->shields==unit_map[i].shields);
        assert(f->armor==unit_map[i].armor && f->minerals==unit_map[i].mins);
        assert(f->gas==unit_map[i].gas && f->build_frames==unit_map[i].frames);
        assert(f->sight_px==unit_map[i].sight);
        assert(f->left==unit_map[i].left && f->up==unit_map[i].up);
        assert(f->right==unit_map[i].right && f->down==unit_map[i].down);
        assert(f->flying==unit_map[i].flying && f->size==unit_map[i].size);
        assert(f->ground_damage==unit_map[i].damage && f->ground_bonus==unit_map[i].bonus);
        assert(f->ground_cooldown==unit_map[i].cooldown && f->ground_range==unit_map[i].range);
        assert(f->ground_hits==unit_map[i].hits);
        assert(f->air_damage==0 && f->air_bonus==0 && f->air_cooldown==0);
        assert(f->air_range==0 && f->air_hits==0);
        assert(sc64_race_unit_rule(unit_map[i].type)->producer == 255);
        assert(sc64_race_unit_rule(unit_map[i].type)->minerals == 0);
        assert(sc64_race_unit_rule(unit_map[i].type)->gas == 0);
        assert(sc64_race_unit_rule(unit_map[i].type)->build_frames == 0);
        assert(sc64_race_unit_rule(unit_map[i].type)->hp == unit_map[i].hp);
        assert(sc64_race_unit_rule(unit_map[i].type)->shields == unit_map[i].shields);
        assert(sc64_race_unit_armor(unit_map[i].type) == unit_map[i].armor);
        assert(sc64_race_unit_damage(unit_map[i].type) == unit_map[i].damage);
        assert(sc64_race_unit_range(unit_map[i].type) == unit_map[i].range);
        assert(sc64_race_unit_cooldown(unit_map[i].type) == unit_map[i].cooldown);
        assert(sc64_race_unit_flying(unit_map[i].type) == unit_map[i].flying);
        assert(sc64_race_unit_hits_vs(unit_map[i].type,0) == unit_map[i].hits);
    }
    assert(strcmp(sc64_race_unit_name(U_ZP_INFESTED_KERRIGAN), "INFESTED KERRIGAN")==0);
    assert(strcmp(sc64_race_unit_name(U_ZP_MAP_REVEALER), "MAP REVEALER")==0);
    assert(strcmp(sc64_race_unit_name(U_ZP_RAYNOR_VULTURE), "RAYNOR VULTURE")==0);
    assert(strcmp(sc64_race_unit_name(U_ZP_HIGH_TEMPLAR), "HIGH TEMPLAR")==0);
    assert(strcmp(sc64_race_unit_name(U_ZP_ULTRALISK), "ULTRALISK")==0);
    assert(sc64_race_unit_from_raw(87) == -1); /* transmission speaker only */
    assert(sc64_race_raw_unit(U_TASSADAR) == 79);
    assert(sc64_zp_actor_flags(U_ZP_MAP_REVEALER) == SC64_ZP_ACTOR_HIDDEN);
    assert(!(sc64_zp_actor_flags(U_ZP_MAP_REVEALER) &
             (SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE)));
    assert(sc64_zp_actor_fact(U_ZP_HIGH_TEMPLAR)->shields == 40);
    assert(sc64_zp_actor_fact(U_ZP_ULTRALISK)->hp == 400);
    static const struct { int raw,type,hp,armor,mins,gas,frames,sight,tw,th,l,u,r,d,size; } building_map[] = {
        {115,B_ZP_CONTROL_TOWER,500,1,50,50,600,256,2,2,47,24,28,22,2},
        {120,B_ZP_MACHINE_SHOP,750,1,50,50,600,256,2,2,39,24,31,24,2},
        {123,B_ZP_ARMORY,750,1,100,50,1200,256,3,2,48,32,47,22,2},
        {138,B_ZP_QUEENS_NEST,850,1,150,100,900,256,3,2,38,28,32,28,2},
        {140,B_ZP_ULTRALISK_CAVERN,600,1,150,200,1200,256,3,2,40,32,32,31,2},
        {196,B_ZP_PROTOSS_BEACON,100000,0,100,100,1,256,3,2,48,32,47,31,3}
    };
    for (unsigned i = 0; i < sizeof(building_map)/sizeof(building_map[0]); ++i) {
        assert(sc64_race_building_from_raw(building_map[i].raw) == building_map[i].type);
        assert(sc64_race_raw_building(building_map[i].type) == building_map[i].raw);
        const Sc64ZpBuildingFact *f = sc64_zp_building_fact(building_map[i].type);
        assert(f && f->raw==building_map[i].raw && f->hp==building_map[i].hp);
        assert(f->shields==0 && f->armor==building_map[i].armor);
        assert(f->minerals==building_map[i].mins);
        assert(f->gas==building_map[i].gas && f->build_frames==building_map[i].frames);
        assert(f->sight_px==building_map[i].sight);
        assert(f->width_tiles==building_map[i].tw && f->height_tiles==building_map[i].th);
        assert(f->left==building_map[i].l && f->up==building_map[i].u);
        assert(f->right==building_map[i].r && f->down==building_map[i].d);
        assert(f->size==building_map[i].size);
        assert(sc64_race_building_rule(building_map[i].type)->hp ==
               (building_map[i].hp > 65535 ? 0 : building_map[i].hp));
        assert(sc64_race_building_rule(building_map[i].type)->worker == 255);
    }
    /* Raw 79 Hero_Tassadar already has a canonical rule row. */
    assert(sc64_race_unit_from_raw(79)==U_TASSADAR);
    assert(sc64_race_raw_unit(U_TASSADAR)==79);
    assert(sc64_race_unit_rule(U_TASSADAR)->hp==80);
    assert(sc64_race_unit_rule(U_TASSADAR)->shields==300);
    assert(sc64_race_unit_rule(U_TASSADAR)->minerals==100);
    assert(sc64_race_unit_rule(U_TASSADAR)->gas==300);
    assert(sc64_race_unit_rule(U_TASSADAR)->build_frames==1500);
    assert(sc64_race_unit_armor(U_TASSADAR)==2);
    assert(sc64_race_unit_damage(U_TASSADAR)==20);
    assert(sc64_race_unit_range(U_TASSADAR)==96);
    assert(sc64_race_unit_cooldown(U_TASSADAR)==22);
    assert(strcmp(sc64_race_unit_name(U_TASSADAR), "TASSADAR")==0);
    assert(strcmp(sc64_race_building_name(B_ZP_PROTOSS_BEACON), "PROTOSS BEACON")==0);
    assert(sc64_zp_building_default_hp(B_ZP_PROTOSS_BEACON)==100000);
    assert(sc64_zp_building_flags(B_ZP_PROTOSS_BEACON) & SC64_ZP_ACTOR_INVINCIBLE);
#endif
    return 0;
}
