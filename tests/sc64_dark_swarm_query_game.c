/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual field input plus synthetic actor positions for source-edge checks. */
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
#ifndef SC64_SHARED_DARK_SWARM_PROBE
#error "requires disabled query probe"
#endif
static int at(int type,int x,int y)
{
    int q=spawn_unit(ENEMY,type,x,y);assert(q>=0);
    int result=sc64_world_dark_swarm_affects_target(q);u[q].alive=0;return result;
}
static void weapon_case(int raw,int target_type,int x,int should_hit)
{
    for(int n=0;n<20 && sc64_shared_projectiles.active;n++)game_update(0,0);
    assert(!sc64_shared_projectiles.active);
    int shooter=spawn_unit(ENEMY,sc64_race_unit_from_raw(raw),x-20,320);
    int target=spawn_unit(SIDE,target_type,x,320);assert(shooter>=0 && target>=0);
    int bystander=-1;
    if(!should_hit) {bystander=spawn_unit(SIDE,U_SHARED_DEFILER,x-15,320);assert(bystander>=0);u[bystander].hp=u[bystander].maxhp=1000;}
    u[target].hp=u[target].maxhp=1000;
    u[shooter].order=ORDER_ATTACK_TARGET;u[shooter].target=target;
    tick_combat(&u[shooter],target);assert(sc64_ordinary_pending[shooter].active);
    int saw_bullet=0;
    for(int n=0;n<60;n++) {
        game_update(0,0);
        for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++) {
            Sc64OrdinaryBullet *bullet=&sc64_ordinary_bullets[i];
            if(bullet->handle && bullet->reservation_owner==(unsigned)shooter) {
                saw_bullet=1;
                if(!should_hit)assert(bullet->target==-1);
            }
        }
    }
    fprintf(stderr,"weapon raw=%d x=%d hit=%d hp=%d shooter_cd=%d order=%d target=%d qxy=%d,%d targetxy=%d,%d active=%u screen=%d\n",raw,x,should_hit,u[target].hp,u[shooter].cd,u[shooter].order,u[shooter].target,u[shooter].x,u[shooter].y,u[target].x,u[target].y,sc64_shared_projectiles.active,screen);
    assert(saw_bullet && u[target].alive);
    if(should_hit)assert(u[target].hp<1000);else {
        assert(u[target].hp==1000 && u[bystander].alive && u[bystander].hp==1000);
        u[bystander].alive=0;
    }
    u[shooter].alive=0;u[target].alive=0;
}
static void unknown_case(void)
{
    for(int n=0;n<20 && sc64_shared_projectiles.active;n++)game_update(0,0);
    assert(!sc64_shared_projectiles.active);
    int shooter=spawn_unit(ENEMY,U_MARINE,380,320);
    int target=spawn_unit(SIDE,U_ZERGLING,400,320);assert(shooter>=0 && target>=0);
    u[target].hp=u[target].maxhp=1000;
    u[target].raw_type=255; /* Deliberately malformed row for fail-closed test. */
    assert(sc64_world_dark_swarm_affects_target(target)==-1);
    u[shooter].order=ORDER_ATTACK_TARGET;u[shooter].target=target;
    tick_combat(&u[shooter],target);assert(u[shooter].cd>0 && sc64_ordinary_pending[shooter].active);
    for(int n=0;n<20 && sc64_ordinary_pending[shooter].active;n++)game_update(0,0);
    assert(!sc64_ordinary_pending[shooter].active && u[target].hp==1000);
    for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++)
        assert(!sc64_ordinary_bullets[i].handle || sc64_ordinary_bullets[i].reservation_owner!=(unsigned)shooter);
    u[shooter].alive=0;u[target].alive=0;
}
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    int c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);test_phase=0;click_cast(c,400,320);
    for(int n=0;n<40 && !fields();n++)game_update(0,0);assert(fields());
    /* Source field center (800,640). Even GBA coordinates exercise the
     * adjacent representable points either side of source rectangle edges. */
    assert(at(U_MARINE,356,320)==1);assert(at(U_MARINE,355,320)==0);
    assert(at(U_MARINE,443,320)==1);assert(at(U_MARINE,444,320)==0);
    assert(at(U_MARINE,400,275)==1);assert(at(U_MARINE,400,274)==0);
    assert(at(U_MARINE,400,364)==1);assert(at(U_MARINE,400,365)==0);
    assert(at(U_SHARED_DEFILER,354,320)==1);assert(at(U_SHARED_DEFILER,353,320)==0);
    assert(at(U_SHARED_DEFILER,446,320)==1);assert(at(U_SHARED_DEFILER,447,320)==0);
    assert(at(U_SHARED_UNCLEAN_ONE,353,320)==1);assert(at(U_SHARED_UNCLEAN_ONE,352,320)==0);
    assert(at(U_SHARED_UNCLEAN_ONE,446,320)==1);assert(at(U_SHARED_UNCLEAN_ONE,447,320)==0);
#ifdef SC64_SHARED_DARK_SWARM_SOURCE_RECTS
    assert(at(U_ZERGLING,400,320)==1); /* authenticated raw37 geometry */
#else
    assert(at(U_ZERGLING,400,320)==-1); /* unknown geometry fails closed */
#endif
    int f=-1;for(int i=0;i<MAX_UNITS;i++)if(sc64_defiler_fields[i].remaining)f=i;
    assert(f>=0);u16 saved_ttl=sc64_defiler_fields[f].remaining;
    sc64_defiler_fields[f].remaining=904;assert(at(U_MARINE,400,320)==0);
    sc64_defiler_fields[f].remaining=saved_ttl;
    assert(at(sc64_race_unit_from_raw(12),400,320)==0); /* flying */
    assert(sc64_world_dark_swarm_affects_target(MAX_UNITS)==0); /* building */
    /* Real update dispatch of Marine shots, with synthetic attack orders and
     * target HP. The guaranteed swarm miss retains its live bullet slot. */
    const int tested_raw[5]={0,2,8,38,66};
    for(int i=0;i<5;i++) {
        weapon_case(tested_raw[i],U_SHARED_DEFILER,400,0);
        weapon_case(tested_raw[i],U_SHARED_DEFILER,500,1);
    }
#ifdef SC64_SHARED_DARK_SWARM_SOURCE_RECTS
    weapon_case(0,U_ZERGLING,400,0);
#endif
    unknown_case();
    /* Real Yamato order/release; energy and heading are seeded only to
     * isolate launch policy, and are not claimed as ordinary acquisition. */
    int target=spawn_unit(ENEMY,U_MARINE,400,320);
    int bc=spawn_unit(SIDE,sc64_race_unit_from_raw(12),350,320);assert(target>=0 && bc>=0);
    u[bc].work_timer=64<<3;u[bc].carry=200*256;
    sc64_battlecruiser_tech_bits=(u16)SC64_BC_TECH_YAMATO;
    assert(race_bc_yamato_issue(bc,target));
    if(!sc64_battlecruiser_source_tick_due())++frame;
    assert(race_bc_yamato_tick_caster(bc));
    for(int n=0;n<100 && sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING;n++) {
        ++frame;assert(race_bc_yamato_tick_caster(bc));
    }
    assert(sc64_bc_shots[0].state==SC64_BC_SHOT_FLIGHT && (sc64_bc_shots[0].phase&64));
    int hp=u[target].hp;
    for(int n=0;n<100 && sc64_bc_shots[0].state;n++)assert(race_bc_yamato_projectile_step(0));
    assert(u[target].alive && u[target].hp==hp);
    puts("source-rectangle field edges and actual Yamato miss passed");return 0;
}
