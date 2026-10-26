#define SC64_SIEGE_SPLASH 1
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

static void test_siege_weapon27_real_tick_splash_and_eligibility(void)
{
    clear_combat_fixture();
    int tank=add_raw_unit(SIDE,30,100,100,150);
    weapons_level=0;
    int primary=add_raw_unit(ENEMY,0,200,100,100);
    int friendly_secondary=add_raw_unit(SIDE,0,214,100,100);
    int middle=add_raw_unit(ENEMY,0,216,100,100);
    int outer=add_raw_unit(RESCUABLE,0,224,100,100);
    int outside=add_raw_unit(ENEMY,0,225,100,100);
    int air=add_raw_unit(ENEMY,8,200,100,120);
    int loaded=add_raw_unit(ENEMY,0,214,100,100);
    int immune=add_raw_unit(ENEMY,0,214,100,100);
    int array_immune=add_raw_unit(ENEMY,0,214,100,100);
    int hidden=add_raw_unit(ENEMY,101,214,100,100);
    int invalid_far=add_raw_unit(ENEMY,0,400,100,100);
    int invalid_air=add_raw_unit(ENEMY,8,214,100,100);
    int invalid_loaded=add_raw_unit(ENEMY,0,214,100,100);
    u[loaded].order=ORDER_TRANSPORTED;
    u[immune].flags|=16u;
    invincible_units[array_immune]=1;
    u[invalid_far].raw_type=254; u[invalid_air].raw_type=254;
    u[invalid_loaded].raw_type=254; u[invalid_loaded].order=ORDER_TRANSPORTED;
    tick_combat(&u[tank],primary);
    assert(u[primary].hp==65);                  /* primary full packet, once */
    assert(u[friendly_secondary].hp==83);       /* radial friendly-fire collateral */
    assert(u[middle].hp==83);                   /* <=25 source-pixel tier */
    assert(u[outer].hp==92);                    /* <=40 source-pixel tier */
    assert(u[outside].hp==100 && u[air].hp==120);
    assert(u[loaded].hp==100 && u[immune].hp==100 && u[array_immune].hp==100);
    assert(sc64_game_unit_has_flag(u[hidden].type,SC64_GAME_ACTOR_HIDDEN));
    assert(u[hidden].hp==100);
    assert(u[invalid_far].hp==100 && u[invalid_air].hp==100 && u[invalid_loaded].hp==100);
    assert(u[tank].hp==150 && u[tank].cd==((75*5+3)/4));
}

static void test_siege_q8_armor_shields_and_all_normal_sizes(void)
{
    clear_combat_fixture();
    int tank=add_raw_unit(SIDE,30,100,100,150);
    weapons_level=0;
    int primary=add_raw_unit(ENEMY,5,200,100,500);
    int small=add_raw_unit(ENEMY,0,214,100,500);
    int medium=add_raw_unit(ENEMY,2,214,100,500);
    int large=add_raw_unit(ENEMY,5,214,100,500);
    u[primary].shield=15; u[primary].armor=2;
    u[small].armor=u[medium].armor=u[large].armor=2;
    tick_combat(&u[tank],primary);
    assert(u[primary].shield==0 && u[primary].hp==447); /* 70 - 15 shields - 2 armor */
    assert(u[small].hp==484 && u[medium].hp==476 && u[large].hp==467);
}

static void test_siege_building_death_and_q8_ring_boundaries(void)
{
    clear_combat_fixture();
    int tank=add_raw_unit(SIDE,30,100,100,150);
    int nexus=spawn_build(ENEMY,B_NEXUS,200,100,0);
    assert(nexus>=0); b[nexus].raw_type=154; b[nexus].hp=b[nexus].maxhp=2; b[nexus].shield=0;
    tick_combat(&u[tank],MAX_UNITS+nexus);
    assert(!b[nexus].alive);
    assert(sizeof(siege_bounds)/sizeof(siege_bounds[0])==228);
    assert(siege_bounds[204][2]==128); /* max extent stays positive */
    assert(sc64_siege_xy_length(40,0)==40);
    assert(sc64_siege_xy_length(10,10)==13);
    assert(sc64_siege_xy_length(20,10)==22);
    assert(sc64_siege_xy_length(25,25)==33);
    assert(sc64_siege_xy_length(-64,64)==83);
    assert(sc64_siege_xy_length(64,-64)==83);
    assert(sc64_siege_distance(0,209,100,200,100)==10);
    assert(sc64_siege_distance(0,210,100,200,100)==12);
    assert(sc64_siege_distance(0,216,100,200,100)==24);
    assert(sc64_siege_distance(0,217,100,200,100)==26);
    assert(sc64_siege_distance(0,224,100,200,100)==40);
    assert(sc64_siege_distance(0,225,100,200,100)==42);
    assert(sc64_siege_packet_q8(70,10,(int[1]){0})==70u*256u);
    assert(sc64_siege_packet_q8(70,11,(int[1]){0})==35u*256u);
    assert(sc64_siege_packet_q8(70,25,(int[1]){0})==35u*256u);
    assert(sc64_siege_packet_q8(70,26,(int[1]){0})==70u*64u);
    assert(sc64_siege_packet_q8(70,40,(int[1]){0})==70u*64u);
    assert(sc64_siege_packet_q8(70,41,(int[1]){0})==0);
}

static void test_siege_burrowed_targets_inner_only(void)
{
    clear_combat_fixture();
    race_mode=0; race_selected_mission=SC64_P6_GLOBAL_ID;
    int tank=add_raw_unit(SIDE,30,100,100,150);
    int primary=add_raw_unit(ENEMY,0,200,100,100);
    int burrow_inner=add_raw_unit(ENEMY,37,209,100,100);
    int burrow_middle=add_raw_unit(ENEMY,37,215,100,100);
    int burrow_outer=add_raw_unit(ENEMY,37,226,100,100);
    int drone_inner=add_raw_unit(ENEMY,41,209,100,100);
    int drone_outer=add_raw_unit(ENEMY,41,226,100,100);
    u[burrow_inner].flags|=2u;
    u[burrow_middle].flags|=2u;
    u[burrow_outer].flags|=2u;
    u[drone_inner].flags|=2u;
    u[drone_outer].flags|=2u;
    assert(race_p6_active());
    assert(race_p6_combat_unit_burrowed(burrow_inner));
    race_siege_splash_attack(&u[tank],primary,70);
    assert(u[burrow_inner].hp==65);
    assert(u[burrow_middle].hp==100 && u[burrow_outer].hp==100);
    assert(u[drone_inner].hp==65 && u[drone_outer].hp==100);
}

static void test_siege_weapon28_and_unit_death_supply_bookkeeping(void)
{
    clear_combat_fixture();
    int tank=add_raw_unit(SIDE,30,100,100,150);
    u[tank].raw_type=25; /* same live Siege-mode actor, Duke weapon row 28 */
    int primary=add_raw_unit(ENEMY,5,200,100,100);
    int side_victim=add_raw_unit(SIDE,0,214,100,1);
    weapons_level=0;
    Sc64StockAttack duke_attack;
    assert(sc64_stock_attack_select(25,0,&duke_attack) && duke_attack.weapon_id==28);
    int before=supply_used;
    tick_combat(&u[tank],primary);
    assert(u[primary].hp==0 || !u[primary].alive); /* Duke 150, selected target once */
    assert(!u[side_victim].alive);
    assert(supply_used<before);
}


static void test_siege_unknown_nearby_ground_actor_falls_back_to_primary(void)
{
    clear_combat_fixture();
    int tank=add_raw_unit(SIDE,30,100,100,150);
    int primary=add_raw_unit(ENEMY,0,200,100,100);
    int friendly=add_raw_unit(SIDE,0,214,100,100);
    int unknown=add_raw_unit(ENEMY,0,215,100,100);
    u[unknown].raw_type=254;
    weapons_level=0;
    tick_combat(&u[tank],primary);
    assert(u[primary].hp==65); /* still a live full primary hit */
    assert(u[friendly].hp==100); /* no unverified partial collateral */
    assert(u[unknown].hp==100);
}


static void test_siege_unknown_primary_uses_safe_large_fallback(void)
{
    clear_combat_fixture();
    int tank=add_raw_unit(SIDE,30,100,100,150);
    int primary=add_raw_unit(ENEMY,0,200,100,100);
    int friendly=add_raw_unit(SIDE,0,214,100,100);
    u[primary].raw_type=254; /* invalid identity exercises NULL stock profile */
    weapons_level=0;
    tick_combat(&u[tank],primary);
    assert(u[primary].hp==30); /* full primary impact still resolves safely */
    assert(u[friendly].hp==100); /* fallback refuses guessed collateral */
}

int main(void)
{
 test_siege_weapon27_real_tick_splash_and_eligibility();
 test_siege_q8_armor_shields_and_all_normal_sizes();
 test_siege_building_death_and_q8_ring_boundaries();
 test_siege_burrowed_targets_inner_only();
 test_siege_weapon28_and_unit_death_supply_bookkeeping();
 test_siege_unknown_nearby_ground_actor_falls_back_to_primary();
 test_siege_unknown_primary_uses_safe_large_fallback();
 return 0;
}
