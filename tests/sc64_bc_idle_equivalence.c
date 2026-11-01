/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Differential check against the pre-optimization dispatch and predicates. */
#define main sc64_bc_unused_skirmish_main
#include "sc64_skirmish_runtime.c"
#undef main
static Unit before_units[MAX_UNITS], after_units[MAX_UNITS];
static Building before_buildings[MAX_BUILDINGS], after_buildings[MAX_BUILDINGS];
static Sc64BcYamatoShot before_shots[SC64_BC_SHOT_LIMIT], after_shots[SC64_BC_SHOT_LIMIT];
static int old_recovery(Unit *q)
{
    unsigned recovery;
    if (!q || !q->alive || sc64_race_raw_unit(q->type)!=12) return 0;
    recovery=(unsigned)q->work_timer&7u;
    if (!recovery) return 0;
    if (recovery>6 || q->order!=ORDER_ATTACK_TARGET) { q->work_timer&=~7; return 0; }
    if (sc64_battlecruiser_source_tick_due()) {
        --recovery; q->work_timer=(q->work_timer&~7)|(int)recovery;
        if (!recovery) { q->order=ORDER_IDLE; q->target=-1; }
    }
    return 1;
}
static int old_dispatch(Unit *q,int slot,int recharge)
{
    if (sc64_race_raw_unit(q->type)==12 && sc64_battlecruiser_source_tick_due()) {
        if (q->build>=0) --q->build;
        race_bc_yamato_heading_tick(q);
    }
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (recharge && race_battery_tick_unit(slot)) return 1;
#else
    (void)recharge;
#endif
    return old_recovery(q) || race_bc_yamato_tick_caster(slot);
}
static int old_airborne(int slot)
{
    return slot>=0 && slot<MAX_BUILDINGS && b[slot].alive &&
        sc64_race_raw_building(b[slot].type)==116 && (b[slot].flags&4)!=0;
}
int main(void)
{
    unsigned cases=0;
    for (int type=-2;type<=SC_RU_COUNT+2;++type)
        assert(sc64_race_is_battlecruiser(type)==(sc64_race_raw_unit(type)==12));
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();
    for (int type=-1;type<=SC_RU_COUNT;++type)
      for (int alive=0;alive<2;++alive)
       for (int recharge=0;recharge<2;++recharge)
        for (int phase=0;phase<5;++phase)
         for (int scenario=0;scenario<8;++scenario) {
            memset(u,0,sizeof(u));memset(b,0,sizeof(b));memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));
            Unit *q=&u[0];q->alive=alive;q->type=type;q->owner=ENEMY;q->raw_owner=1;
            q->x=500;q->y=500;q->build=3;q->carry=200*256;q->work_timer=(64<<3)|(scenario&7);
            q->order=ORDER_ATTACK_TARGET;q->target=1;
            u[1].alive=1;u[1].type=U_MARINE;u[1].owner=SIDE;u[1].x=520;u[1].y=500;
#ifdef SC64_BATTERY_SERVICE_ENABLED
            if (recharge) {q->order=race_battery_recharge_order();q->target=-1;}
#endif
            if (scenario>=1 && scenario<=6) {
                int row=scenario&1;Sc64BcYamatoShot *shot=&sc64_bc_shots[row];
                shot->state=(scenario==5)?SC64_BC_SHOT_FLIGHT:(scenario==6?7:SC64_BC_SHOT_CHARGING);
                shot->caster=(scenario==4)?2:0;shot->target=1;shot->phase=ENEMY;shot->age=2;
                if (scenario==3) {sc64_bc_shots[1-row]=*shot;sc64_bc_shots[1-row].caster=0;}
            }
            frame=phase;sc64_bc_spell_rng=12345;
            memcpy(before_units,u,sizeof(u));memcpy(before_buildings,b,sizeof(b));memcpy(before_shots,sc64_bc_shots,sizeof(sc64_bc_shots));
            int old=old_dispatch(q,0,recharge);u32 old_rng=sc64_bc_spell_rng;
            memcpy(after_units,u,sizeof(u));memcpy(after_buildings,b,sizeof(b));memcpy(after_shots,sc64_bc_shots,sizeof(sc64_bc_shots));
            memcpy(u,before_units,sizeof(u));memcpy(b,before_buildings,sizeof(b));memcpy(sc64_bc_shots,before_shots,sizeof(sc64_bc_shots));
            frame=phase;sc64_bc_spell_rng=12345;
            int now=race_bc_order_dispatch_needed(&u[0],0,recharge) && race_bc_order_dispatch(&u[0],0,recharge);
            assert(now==old && sc64_bc_spell_rng==old_rng);
            assert(!memcmp(after_units,u,sizeof(u)) && !memcmp(after_buildings,b,sizeof(b)) && !memcmp(after_shots,sc64_bc_shots,sizeof(sc64_bc_shots)));
            ++cases;
         }
#ifdef SC64_BATTERY_SERVICE_ENABLED
    /* A powered Battery transfer wins the ordering race against a stale
     * non-BC caster reservation; its energy and shield effects are real. */
    game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;init_state();
    int battery=spawn_build(SIDE,building_raw(172),300,300,0);
    int pylon=spawn_build(SIDE,building_raw(156),300,260,0);
    int unit=spawn_unit(SIDE,unit_raw(65),304,300);
    assert(battery>=0 && pylon>=0 && unit>=0);
    b[battery].raw_type=b[battery].raw_owner=-1;b[pylon].raw_type=b[pylon].raw_owner=-1;
    b[battery].queue_progress=12345;b[battery].level=0;
    u[unit].raw_type=u[unit].raw_owner=-1;u[unit].shield=1;u[unit].shield_remainder=7;
    u[unit].order=race_battery_recharge_order();u[unit].target=battery;u[unit].work_timer=0;
    memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));
    sc64_bc_shots[0].state=SC64_BC_SHOT_CHARGING;sc64_bc_shots[0].caster=(u16)unit;
    memcpy(before_units,u,sizeof(u));memcpy(before_buildings,b,sizeof(b));memcpy(before_shots,sc64_bc_shots,sizeof(sc64_bc_shots));
    int old_battery=old_dispatch(&u[unit],unit,1);
    assert(old_battery && b[battery].queue_progress<12345 && sc64_bc_shots[0].state);
    memcpy(after_units,u,sizeof(u));memcpy(after_buildings,b,sizeof(b));memcpy(after_shots,sc64_bc_shots,sizeof(sc64_bc_shots));
    memcpy(u,before_units,sizeof(u));memcpy(b,before_buildings,sizeof(b));memcpy(sc64_bc_shots,before_shots,sizeof(sc64_bc_shots));
    int new_battery=race_bc_order_dispatch_needed(&u[unit],unit,1) && race_bc_order_dispatch(&u[unit],unit,1);
    assert(new_battery==old_battery && !memcmp(after_units,u,sizeof(u)) &&
           !memcmp(after_buildings,b,sizeof(b)) && !memcmp(after_shots,sc64_bc_shots,sizeof(sc64_bc_shots)));
#endif
    for (int type=-1;type<=SC_RB_COUNT;++type) for (int alive=0;alive<2;++alive)
      for (int flags=0;flags<8;++flags) for (int level=0;level<3;++level) {
        memset(&b[0],0,sizeof(b[0]));b[0].type=type;b[0].alive=alive;b[0].flags=flags;b[0].level=level;
        assert(old_airborne(0)==race_bc_facility_airborne(0));
        assert((old_airborne(0) && b[0].level==1)==(b[0].level==1 && race_bc_facility_airborne(0)));
        assert(!!(sc64_race_raw_building(type)==118 && level) ==
               !!(level && sc64_race_raw_building(type)==118));
      }
    assert(!race_bc_facility_airborne(-1) && !race_bc_facility_airborne(MAX_BUILDINGS));
    printf("BC idle dispatch, alias, recharge, stale caster and building predicates: %u differential cases pass\n",cases);
}
