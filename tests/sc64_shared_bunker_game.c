/* SPDX-License-Identifier: GPL-3.0-or-later
 * Real custom-game Bunker input, occupancy, combat and destruction. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define SC64_OWNER_LIST_TEST 1
static void sc64_owner_lists_test_probe(void);
#include "../starcraft/game.c"
static int probe_empty_slot=-1, probe_board_unit=-1;
static void sc64_owner_lists_test_probe(void)
{
    if(probe_empty_slot<0 && probe_board_unit<0) return;
    assert(target_lists_valid);
    sc64_target_index_begin();
    int before_bucket=sc64_unit_bucket_active, before_index=sc64_target_index_active;
    if(probe_empty_slot>=0) {
        race_bunker_clear_slot(probe_empty_slot);
        assert(target_lists_valid && sc64_unit_bucket_active==before_bucket && sc64_target_index_active==before_index);
        probe_empty_slot=-1;
    }
    if(probe_board_unit>=0) {
        race_bunker_tick_board(probe_board_unit);
        assert(SC64_TRANSPORT_LOADED(&u[probe_board_unit]));
        assert(!target_lists_valid && !sc64_unit_bucket_active && !sc64_target_index_active);
        probe_board_unit=-1;
    }
}

static void fixture_clear_selection(void)
{
    for (int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    selected_building=-1; menu_open=0; mode=MODE_NORMAL;
}
static void tick(void) { game_update(0,0); assert(screen==SCREEN_PLAY); }
static void board_input(int unit,int bunker)
{
    fixture_clear_selection(); u[unit].selected=1; cx=b[bunker].x; cy=b[bunker].y;
    interact();
    assert(u[unit].order==ORDER_BOARD && u[unit].target==MAX_UNITS+bunker);
    for (int t=0;t<120 && !SC64_TRANSPORT_LOADED(&u[unit]);++t) tick();
    assert(SC64_TRANSPORT_LOADED(&u[unit]));
    assert(!u[unit].selected && u[unit].target==MAX_UNITS+bunker);
}
static void open_site(int *x,int *y)
{
    for(int row=4;row<race_rows-4;++row) for(int col=4;col<race_cols-4;++col) {
        int px=col*16+8, py=row*16+8, open=1;
        for(int dy=-2;dy<=2;++dy) for(int dx=-2;dx<=2;++dx)
            if(terrain_blocked(col+dx,row+dy)) open=0;
        for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && dist2(px,py,u[i].x,u[i].y)<96*96) open=0;
        for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && dist2(px,py,b[i].x,b[i].y)<96*96) open=0;
        if(open) { *x=px; *y=py; return; }
    }
    assert(!"no open fixture site");
}
static void paid_build_fixture(int x,int y)
{
    int worker=spawn_unit(SIDE,U_SCV,x,y); assert(worker>=0); minerals=gas=5000;
    fixture_clear_selection(); u[worker].selected=1;
    const Sc64SkirmishRecord *row=sc64_skirmish_menu_record(R_TER,125);
    assert(row && !race_skirmish_record_available(row));
    int barracks=spawn_build(SIDE,B_BARRACKS,x+96,y,0); assert(barracks>=0);
    assert(race_skirmish_record_available(row));
    int sx=-1,sy=-1;
    for(int yy=96;yy<WORLD_H-64 && sx<0;yy+=24) for(int xx=96;xx<WORLD_W-64 && sx<0;xx+=24) {
        int tx=xx,ty=yy;
        if(race_build_request_valid(SIDE,worker,B_BUNKER,&tx,&ty,0)) { sx=tx;sy=ty; }
    }
    assert(sx>=0); u[worker].x=sx+80; u[worker].y=sy;
    assert(race_skirmish_choose_record(row) && mode==MODE_BUILD);
    int old_m=minerals,old_g=gas; cx=sx;cy=sy;interact();
    assert(u[worker].order==ORDER_BUILD_PENDING && minerals==old_m && gas==old_g);
    int built=-1, before_arrival_m=0, before_arrival_g=0;
    for(int i=0;i<MAX_UNITS;++i) if(i!=worker && u[i].alive && u[i].owner==SIDE && u[i].type==U_SCV) {u[i].order=ORDER_IDLE;u[i].carry=0;}
    for(int t=0;t<500 && built<0;++t) {
        before_arrival_m=minerals;before_arrival_g=gas; tick();
        for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && b[i].owner==SIDE && b[i].type==B_BUNKER && b[i].under_construction) {built=i;break;}
    }
    assert(built>=0); const Sc64RaceBuildingRule *rule=sc64_race_building_rule(B_BUNKER);
    assert(rule && minerals==before_arrival_m-rule->minerals && gas==before_arrival_g-rule->gas);
    assert(!race_bunker_request_board(worker,built));
    for(int t=0;t<building_build_time(B_BUNKER)+16 && !building_complete(built);++t) tick();
    assert(building_complete(built));
}
int main(void)
{
    setbuf(stdout,NULL); gba_init(); game_init(); race_mode=1; race_human=R_TER; race_bots=1;
    init_state(); assert(screen==SCREEN_PLAY && race_mode);
    int x=0,y=0; open_site(&x,&y);
    paid_build_fixture(x,y);
    open_site(&x,&y);
    int bunker=spawn_build(SIDE,B_BUNKER,x,y,0); assert(bunker>=0);
    int marines[5];
    for(int i=0;i<5;++i) { marines[i]=spawn_unit(SIDE,U_MARINE,x,y); assert(marines[i]>=0); }
    int enemy=spawn_unit(ENEMY,U_MARINE,x+100,y), air=spawn_unit(SIDE,U_WRAITH,x+100,y);
    assert(enemy>=0 && air>=0);
    assert(!race_bunker_request_board(enemy,bunker));
    assert(!race_bunker_request_board(air,bunker));
    int old_raw=u[marines[0]].raw_type; u[marines[0]].raw_type=7;
    assert(!race_bunker_request_board(marines[0],bunker)); u[marines[0]].raw_type=old_raw;
    u[marines[0]].flags|=2; assert(!race_bunker_request_board(marines[0],bunker)); u[marines[0]].flags&=~2;
    /* Pending boarders do not reserve space: arrival must enforce capacity. */
    for(int i=0;i<5;++i) assert(race_bunker_request_board(marines[i],bunker));
    probe_empty_slot=MAX_BUILDINGS-1; assert(!b[probe_empty_slot].alive);
    probe_board_unit=marines[0]; update_orders();
    assert(probe_empty_slot==-1 && probe_board_unit==-1);
    for(int i=1;i<4;++i) { race_bunker_tick_board(marines[i]); assert(SC64_TRANSPORT_LOADED(&u[marines[i]])); }
    race_bunker_tick_board(marines[4]);
    assert(u[marines[4]].order==ORDER_IDLE && u[marines[4]].target==-1);
    assert(race_bunker_request_unload(bunker)==4);
    for(int i=0;i<4;++i) board_input(marines[i],bunker);
    assert(race_bunker_cargo_space(bunker)==4);
    assert(!race_bunker_request_board(marines[4],bunker));
    assert(!race_bunker_request_board(marines[0],bunker));
    for(int i=0;i<4;++i) assert(!race_can_attack(enemy,marines[i]));
    /* Test the extended range using the actual order/update combat path. */
    u[enemy].x=x+160; u[enemy].y=y;
    Sc64StockAttack attack={0}; assert(sc64_stock_attack_select(0,0,&attack));
    int extended=(attack.weapon->max_range+1)/2+32+18;
    int target=spawn_build(ENEMY,B_BUNKER,x+extended-1,y,0); assert(target>=0);
    for(int i=1;i<4;++i) u[marines[i]].cd=100;
    u[marines[0]].cd=0; u[marines[0]].build=-1;
    memset(tracers,0,sizeof(tracers));
    int hp=b[target].hp;
    assert(race_bunker_target_in_range(&u[marines[0]],MAX_UNITS+target,0));
    b[target].x=x+extended+1;
    assert(!race_bunker_target_in_range(&u[marines[0]],MAX_UNITS+target,0));
    b[target].x=x+extended-1;
    for(int t=0;t<8 && b[target].hp==hp;++t) tick();
    assert(b[target].hp<hp && u[marines[0]].cd>1);
#ifdef SC64_WEAPON_SOURCE_VFX
    int source_found=0;
    for(int i=0;i<MAX_TRACERS;++i) {
        Sc64WeaponSourceVfxKey key;
        if(tracers[i].life>0 && sc64_weapon_source_vfx_unpack((uint32_t)tracers[i].color,&key) &&
           key.actor_raw==0 && key.actor_slot_plus1==marines[0]+1 &&
           key.weapon_id==attack.weapon_id && !key.target_air) source_found=1;
    }
    assert(source_found);
#endif
    int after_shot=b[target].hp, cooldown=u[marines[0]].cd;
    tick(); assert(b[target].hp==after_shot && u[marines[0]].cd==cooldown-1);
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    invincible_buildings[target]=1;
    assert(!race_bunker_target_in_range(&u[marines[0]],MAX_UNITS+target,0));
    invincible_buildings[target]=0;
#endif
    if(MAX_UNITS+target>=240) {
        assert(u[marines[0]].build==-1);
        assert(race_save_game() && race_load_game());
        assert(u[marines[0]].build==-1 && SC64_TRANSPORT_LOADED(&u[marines[0]]));
    }
    destroy_building(target,SIDE);
    fixture_clear_selection(); selected_building=bunker;
    int unload=-1;
    for(int i=0;i<field_menu_count();++i) if(!strcmp(race_menu_label(i),"UNLOAD CARGO")) unload=i;
    assert(unload>=0); choose_command(unload);
    assert(race_bunker_cargo_space(bunker)==0);
    for(int i=0;i<4;++i) assert(u[marines[i]].alive && !SC64_TRANSPORT_LOADED(&u[marines[i]]));
    /* Diagonal approach around one fixture-blocked navigation cell. */
    u[marines[0]].x=x+80; u[marines[0]].y=y+80;
    build_path_grid(); int blocked_col=(x+48)/16, blocked_row=(y+48)/16;
    SC64_GRID_SET(path_walkable,blocked_col+blocked_row*MAP_COLS,0);
    fixture_clear_selection(); u[marines[0]].selected=1; cx=x;cy=y;interact();
    assert(u[marines[0]].order==ORDER_BOARD);
    for(int t=0;t<240 && !SC64_TRANSPORT_LOADED(&u[marines[0]]);++t) {
        tick();
        assert(u[marines[0]].x/16!=blocked_col || u[marines[0]].y/16!=blocked_row);
    }
    assert(SC64_TRANSPORT_LOADED(&u[marines[0]]));
    path_grid_valid=0;
    int enemy_bunker=spawn_build(ENEMY,B_BUNKER,x+120,y,0);
    assert(enemy_bunker>=0); u[enemy].x=b[enemy_bunker].x; u[enemy].y=b[enemy_bunker].y;
    assert(race_bunker_request_board(enemy,enemy_bunker));
    race_bunker_tick_board(enemy);
    assert(SC64_TRANSPORT_LOADED(&u[enemy]) && race_bunker_cargo_space(enemy_bunker)==1);
    assert(!race_bunker_request_board(marines[4],enemy_bunker));
    /* An explicit Dropship unload command must not be stolen by Bunker boarding. */
    if(sc64_race_unit_from_raw(11)==U_DROPSHIP) {
        int ship=spawn_unit(SIDE,U_DROPSHIP,x+80,y); assert(ship>=0);
        int passenger=spawn_unit(SIDE,U_MARINE,x+80,y); assert(passenger>=0);
        assert(race_transport_request_board(passenger,ship));
        race_transport_tick_unit(passenger); assert(SC64_TRANSPORT_LOADED(&u[passenger]));
        fixture_clear_selection(); u[ship].selected=1; u[marines[4]].selected=1;
        mode=MODE_UNLOAD; cx=x; cy=y;
        interact();
        assert(u[marines[4]].order!=ORDER_BOARD);
        assert(u[ship].order==ORDER_UNLOAD);
    }
    destroy_building(bunker,ENEMY);
    assert(!b[bunker].alive && u[marines[0]].alive && !SC64_TRANSPORT_LOADED(&u[marines[0]]));
    /* Boarding eligibility is broader than fire eligibility. */
    int bunker2=spawn_build(SIDE,B_BUNKER,x,y,0); assert(bunker2>=0);
    int worker=spawn_unit(SIDE,U_SCV,x,y), medic=spawn_unit(SIDE,U_MEDIC,x,y);
    int firebat=spawn_unit(SIDE,U_FIREBAT,x,y), foeair=spawn_unit(ENEMY,U_WRAITH,x+32,y);
    assert(worker>=0 && medic>=0 && firebat>=0 && foeair>=0);
    board_input(worker,bunker2); board_input(medic,bunker2); board_input(firebat,bunker2);
    assert(!race_bunker_target_in_range(&u[firebat],foeair,32));
    assert(race_bunker_target_in_range(&u[marines[0]],foeair,0));
    int air_hp=u[foeair].hp;
    u[worker].cd=u[medic].cd=0; u[worker].build=u[medic].build=-1;
    assert(race_bunker_tick_loaded(worker) && race_bunker_tick_loaded(medic));
    assert(u[foeair].hp==air_hp && u[worker].build==-1 && u[medic].build==-1);
    int pending=spawn_unit(SIDE,U_MARINE,x+80,y); assert(pending>=0);
    assert(race_bunker_request_board(pending,bunker2));
    int used_before=supply_used;
    int used_cargo=unit_slot_supply(&u[worker])+unit_slot_supply(&u[medic])+unit_slot_supply(&u[firebat]);
    /* Fixture-only blocked navigation grid forces all unload placements to fail. */
#ifdef SC64_DEATH_SOURCE_ART
    death_vfx_reset(); death_vfx_scan();
#endif
    memset(path_walkable,0,sizeof(path_walkable)); path_grid_valid=1;
    destroy_building(bunker2,ENEMY);
    assert(!u[worker].alive && !u[medic].alive && !u[firebat].alive);
    assert(u[worker].order==ORDER_TRANSPORTED && u[medic].order==ORDER_TRANSPORTED && u[firebat].order==ORDER_TRANSPORTED);
    assert(!u[worker].selected && !u[medic].selected && !u[firebat].selected);
    assert(supply_used==used_before-used_cargo); path_grid_valid=0;
#ifdef SC64_DEATH_SOURCE_ART
    death_vfx_scan();
    Sc64DeathVfx pool[SC64_DEATH_VFX_POOL]; death_vfx_test_load_pool(pool);
    for(int i=0;i<SC64_DEATH_VFX_POOL;++i) assert(!pool[i].style);
#endif
    race_bunker_tick_board(pending);
    assert(u[pending].alive && u[pending].order==ORDER_IDLE && u[pending].target==-1);
    int reused=spawn_build(SIDE,B_BUNKER,x,y,0); assert(reused==bunker2);
    assert(race_bunker_cargo_space(reused)==0);
    puts("shared Bunker input, combat, capacity, unload and destruction PASS");
    return 0;
}
