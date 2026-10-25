#include "../starcraft/game.c"

#include <assert.h>

static void clear_combat_fixture(void)
{
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    memset(tracers,0,sizeof(tracers));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    race_mode=1; race_selected_mission=0; weapons_level=1; armor_level=0;
}

static int add_raw_unit(int owner,int raw,int x,int y,int hp)
{
    int type=sc64_race_unit_from_raw(raw);
    assert(type>=0);
    int slot=spawn_unit(owner,type,x,y);
    assert(slot>=0);
    u[slot].raw_type=(s16)raw;
    u[slot].hp=u[slot].maxhp=hp;
    u[slot].armor=0;
    u[slot].shield=0;
    return slot;
}

static void test_actual_splash_targeting_and_damage(void)
{
    clear_combat_fixture();
    int archon=add_raw_unit(SIDE,68,100,100,350);
    int primary=add_raw_unit(ENEMY,64,130,100,100);
    int middle=add_raw_unit(ENEMY,64,143,100,100);
    int outer=add_raw_unit(ENEMY,64,150,100,100);
    int outside=add_raw_unit(ENEMY,64,151,100,100);
    int ally=add_raw_unit(SIDE,64,132,100,100);
    int allied_other_owner=add_raw_unit(RESCUABLE,64,136,100,100);
    int loaded=add_raw_unit(ENEMY,64,131,100,100);
    int invincible=add_raw_unit(ENEMY,64,133,100,100);
    int array_invincible=add_raw_unit(ENEMY,64,134,100,100);
    u[primary].shield=2;
    u[loaded].order=ORDER_TRANSPORTED;
    u[invincible].flags|=16u;
    u[array_invincible].shield=5;
    invincible_units[array_invincible]=1;
    race_mode=0;
    assert(!owners_hostile(SIDE,RESCUABLE));
    tick_combat(&u[archon],primary);
    assert(tracers[0].life>0 &&
        sc64_weapon_vfx_style(tracers[0].color)==SC64_WEAPON_VFX_STYLE_ARC);
    assert(u[primary].hp==69 && u[primary].shield==0); /* 33, Normal */
    assert(u[middle].hp==84); /* 16.5 Q8 packet -> integer GBA HP */
    assert(u[outer].hp==92);  /* 8.25 Q8 packet -> integer GBA HP */
    assert(u[outside].hp==100);
    assert(u[ally].hp==100);
    assert(u[allied_other_owner].hp==67); /* Enemy_Splash filters owner ID only. */
    assert(u[loaded].hp==100);
    assert(u[invincible].hp==100);
    assert(u[array_invincible].hp==100 && u[array_invincible].shield==5);
    assert(u[archon].hp==350 && u[archon].cd==SC64_P7_ARCHON_COOLDOWN_GBA_TICKS);
}

static void test_enemy_splash_primary_ally_exception(void)
{
    clear_combat_fixture();
    int archon=add_raw_unit(SIDE,68,100,100,350);
    int allied_primary=add_raw_unit(SIDE,64,130,100,100);
    int same_owner_neighbor=add_raw_unit(SIDE,64,132,100,100);
    tick_combat(&u[archon],allied_primary);
    assert(u[allied_primary].hp==67); /* Explicit primary is the exception. */
    assert(u[same_owner_neighbor].hp==100);
    assert(u[archon].hp==350);
}

static void test_enemy_splash_uses_source_owner_when_collapsed(void)
{
    clear_combat_fixture();
    /* Source owners 0 and 4 are both represented by runtime ENEMY in this
     * fixture. Enemy_Splash must exclude only same-source-owner collateral. */
    int archon=add_raw_unit(ENEMY,68,100,100,350);
    int primary=add_raw_unit(SIDE,64,130,100,100);
    int same_source_owner=add_raw_unit(ENEMY,64,132,100,100);
    int distinct_source_owner=add_raw_unit(ENEMY,64,134,100,100);
    int building=spawn_build(ENEMY,B_NEXUS,136,100,0);
    assert(building>=0);
    u[archon].raw_owner=0;
    u[primary].raw_owner=7;
    u[same_source_owner].raw_owner=0;
    u[distinct_source_owner].raw_owner=4;
    b[building].raw_type=154;
    b[building].raw_owner=4;
    b[building].hp=b[building].maxhp=100;
    b[building].shield=0;
    race_mode=0;

    assert(SC64_CAN_ATTACK_FROM(U_P7_ARCHON,primary,u[archon].owner));
    tick_combat(&u[archon],primary);
    assert(u[primary].hp==70); /* Base 30 damage; ENEMY has no SIDE upgrade. */
    assert(u[same_source_owner].hp==100); /* Same source owner is excluded. */
    assert(u[distinct_source_owner].hp==70); /* Distinct source owner is hit. */
    assert(b[building].hp==71); /* Same-owner rule applies; Nexus armor is 1. */
}

static void test_fractional_shield_overflow_engine_quantization(void)
{
    clear_combat_fixture();
    int archon=add_raw_unit(SIDE,68,100,100,350);
    int primary=add_raw_unit(ENEMY,64,130,100,100);
    int one_armor=add_raw_unit(ENEMY,64,140,100,100);
    int two_armor=add_raw_unit(ENEMY,64,140,100,100);
    unsigned distance=0;
    assert(race_p7_archon_distance_to_actor(64,140,100,0,260,200,&distance));
    assert(distance>=4 && distance<=SC64_P7_ARCHON_MIDDLE_SOURCE_PX);
    u[one_armor].shield=15;
    u[one_armor].armor=1;
    u[two_armor].shield=15;
    u[two_armor].armor=2;
    tick_combat(&u[archon],primary);
    /* At source distance 10, the upgraded middle packet is 16.5: 15 shield
     * points, 1 armor, and 0.5 HP. Integer gameplay rounds that residual hit
     * up to one point when routing through hit_target. */
    assert(u[one_armor].shield==0 && u[one_armor].hp==99);
    /* Two armor absorbs the 1.5 packet remainder entirely. */
    assert(u[two_armor].shield==0 && u[two_armor].hp==100);
}

static void test_diagonal_integer_distance_boundary(void)
{
    clear_combat_fixture();
    assert(race_p7_archon_xy_length(22,22)==29u);
    int archon=add_raw_unit(SIDE,68,100,100,350);
    int primary=add_raw_unit(ENEMY,64,130,100,100);
    int diagonal=add_raw_unit(ENEMY,68,149,119,100);
    unsigned distance=0;
    assert(race_p7_archon_distance_to_actor(68,149,119,0,260,200,&distance));
    assert(distance==29u); /* A Euclidean boundary would be beyond 30. */
    tick_combat(&u[archon],primary);
    assert(u[diagonal].hp==92); /* Outer 8.25 packet maps to integer HP. */
}

static void test_source_bounds_range_and_approach(void)
{
    unsigned distance=0;
    assert(race_p7_archon_distance_between(68,100,100,0,
                                            65,145,100,0,&distance));
    assert(distance==SC64_P7_ARCHON_RANGE_SOURCE_PX);
    clear_combat_fixture();
    int archon=add_raw_unit(SIDE,68,100,100,350);
    int edge=add_raw_unit(ENEMY,65,145,100,100);
    tick_combat(&u[archon],edge);
    assert(u[edge].hp==67 && u[archon].cd==SC64_P7_ARCHON_COOLDOWN_GBA_TICKS);

    clear_combat_fixture();
    archon=add_raw_unit(SIDE,68,100,100,350);
    int beyond=add_raw_unit(ENEMY,65,146,100,100);
    tick_combat(&u[archon],beyond);
    assert(u[beyond].hp==100 && u[archon].cd==0);
    assert(u[archon].path_timer || u[archon].x!=100 || u[archon].y!=100);
}

static void test_actual_building_primary_and_attack_hook(void)
{
    clear_combat_fixture();
    int archon=add_raw_unit(SIDE,68,100,100,350);
    int nexus=spawn_build(ENEMY,B_NEXUS,130,100,0);
    assert(nexus>=0);
    b[nexus].raw_type=154;
    b[nexus].hp=b[nexus].maxhp=100;
    b[nexus].shield=0;
    tick_combat(&u[archon],MAX_UNITS+nexus);
    assert(b[nexus].hp==68); /* 33 Normal damage minus engine armor 1. */
    assert(u[archon].cd==SC64_P7_ARCHON_COOLDOWN_GBA_TICKS);
}

static void test_invincible_building_array_preserves_shields(void)
{
    clear_combat_fixture();
    int archon=add_raw_unit(SIDE,68,100,100,350);
    int nexus=spawn_build(ENEMY,B_NEXUS,130,100,0);
    assert(nexus>=0);
    b[nexus].raw_type=154;
    b[nexus].hp=b[nexus].maxhp=100;
    b[nexus].shield=7;
    invincible_buildings[nexus]=1;
    tick_combat(&u[archon],MAX_UNITS+nexus);
    assert(b[nexus].hp==100 && b[nexus].shield==7);
}

int main(void)
{
    test_actual_splash_targeting_and_damage();
    test_enemy_splash_primary_ally_exception();
    test_enemy_splash_uses_source_owner_when_collapsed();
    test_fractional_shield_overflow_engine_quantization();
    test_diagonal_integer_distance_boundary();
    test_source_bounds_range_and_approach();
    test_actual_building_primary_and_attack_hook();
    test_invincible_building_array_preserves_shields();
    return 0;
}
