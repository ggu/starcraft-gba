/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static int setup(void)
{
    race_mode=0; race_selected_mission=SC64_P7_GLOBAL_ID;
    init_state(); assert(screen==SCREEN_PLAY && !mission_unsupported_count);
    assert(race_save_game() && race_load_game());
    int carrier=-1;
    for (int i=0;i<MAX_UNITS;++i) {
        u[i].selected=0;
        if (u[i].alive && u[i].type==U_P7_CARRIER && u[i].owner==SIDE) carrier=i;
    }
    /* Authenticated starting Carriers belong to source force 4. Supply an
     * owned Carrier to isolate the player-command component; production and
     * enemy AI ordering are separate pending integrations. */
    if(carrier<0) carrier=spawn_unit(SIDE,U_P7_CARRIER,64,64);
    assert(carrier>=0); u[carrier].selected=1; selected_building=-1; minerals=100;
    return carrier;
}

static int find_child(int parent,unsigned state)
{
    for (int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].type==U_P7_INTERCEPTOR && u[i].build==parent &&
           sc64_p7_carrier_child_state(&u[i])==state) return i;
    return -1;
}

static void test_training_and_hidden_state(void)
{
    int parent=setup();
    assert(race_p7_carrier_training_menu_available());
    assert(strcmp(field_menu_base_label(MENU_STOP+1,0),"BUILD INTERCEPTOR")==0);
    choose_command(MENU_STOP+1);
    assert(minerals==75 && u[parent].carry==1);
    /* First 30 Hz update is not a source update. The second starts training. */
    game_update(0,0); assert(find_child(parent,SC64_P7_INTERCEPTOR_TRAINING)<0);
    game_update(0,0);
    int child=find_child(parent,SC64_P7_INTERCEPTOR_TRAINING);
    assert(child>=0 && u[child].hp==4 && u[child].shield==40 && u[child].work_timer==300);
    assert(!sc64_game_unit_selectable(u[child].type));
    assert(sc64_game_unit_has_flag(u[child].type,SC64_GAME_ACTOR_HIDDEN));
    assert(!SC64_CAN_ATTACK_FROM_SOURCE(U_MARINE,child,ENEMY,0));
    assert(!race_p7_projectile_fire(child,parent,64,u[child].x*512,u[child].y*512));
    assert(race_save_game() && race_load_game());
    for (unsigned tick=0;tick<299;++tick) race_p7_carrier_training_source_tick();
    assert(u[child].work_timer==1 && u[child].hp==39);
    assert(race_save_game() && race_load_game());
    race_p7_carrier_training_source_tick();
    assert(u[child].hp==40 && u[child].carry==0 &&
        sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_INSIDE);
    assert(race_save_game() && race_load_game());
    /* The opening mission resource trigger may have changed the wallet in
     * the two full game updates. Isolate the next three purchase prices. */
    minerals=75; u[parent].selected=1;
    for (unsigned n=0;n<3;++n) {
        choose_command(MENU_STOP+1);
        race_p7_carrier_training_source_tick();
        for (unsigned tick=0;tick<300;++tick) race_p7_carrier_training_source_tick();
    }
    assert(race_p7_carrier_training_child_count(parent)==4 && minerals==0);
    assert(race_p7_carrier_training_menu_available());
    choose_command(MENU_STOP+1); assert(minerals==0 && u[parent].carry==0);
    assert(race_save_game() && race_load_game());
    hit_target(parent,10000,U_MARINE,ENEMY);
    assert(!u[parent].alive);
    for (int i=0;i<MAX_UNITS;++i) assert(!u[i].alive || u[i].type!=U_P7_INTERCEPTOR);
    assert(race_save_game() && race_load_game());
}

static void reject_without_live_mutation(const RaceSave *bad)
{
    static Unit before_u[MAX_UNITS];
    static Building before_b[MAX_BUILDINGS];
    static u16 before_pool[SC64_ARENA_WORDS];
    Sc64P7PersistentState before_p7=race_p7;
    int before_frame=frame,before_minerals=minerals,before_gas=gas;
    u32 before_seed=seed;
    memcpy(before_u,u,sizeof(u)); memcpy(before_b,b,sizeof(b));
    memcpy(before_pool,race_p7_projectile_host_words,sizeof(before_pool));
    save_write(bad,sizeof(*bad),SAVE_VERSION);
    assert(!race_load_game());
    assert(memcmp(before_u,u,sizeof(u))==0 && memcmp(before_b,b,sizeof(b))==0);
    assert(memcmp(before_pool,race_p7_projectile_host_words,sizeof(before_pool))==0);
    assert(memcmp(&before_p7,&race_p7,sizeof(before_p7))==0);
    assert(frame==before_frame && minerals==before_minerals && gas==before_gas &&
           seed==before_seed);
}

static void test_save_rejection(void)
{
    static RaceSave good,bad;
    int parent=setup();
    choose_command(MENU_STOP+1); race_p7_carrier_training_source_tick();
    int child=find_child(parent,SC64_P7_INTERCEPTOR_TRAINING);
    assert(child>=0 && race_save_game()); good=race_data;
    for (unsigned test=0;test<12;++test) {
        bad=good;
        Unit actor=u[test==9?parent:child];
        if(test==0) actor.build=MAX_UNITS-1;
        if(test==1) { actor.owner=ENEMY; actor.raw_owner=4; }
        if(test==2) actor.work_timer=299;
        if(test==3) actor.carry=256;
        if(test==4) actor.hp=5;
        if(test==5) sc64_p7_carrier_child_set_state(&actor,3);
        if(test==6) sc64_p7_carrier_child_set_rank(&actor,1);
        if(test==7) actor.selected=1;
        if(test==8) actor.order=ORDER_MOVE;
        if(test==9) actor.carry=4;
        if(test==10) actor.shield=39;
        if(test==11) actor.target=parent;
        assert(sc64_p7_typed_unit_encode(&bad.units[test==9?parent:child],&actor,
            (unsigned)(race_cols*16),(unsigned)(race_rows*16)));
        reject_without_live_mutation(&bad);
        save_write(&good,sizeof(good),SAVE_VERSION); assert(race_load_game());
    }
    /* An otherwise-valid projectile packet cannot originate in this stage. */
    static u16 words[SC64_ARENA_WORDS];
    Sc64Arena arena={words,SC64_ARENA_WORDS};
    Sc64ProjectileRow row;
    u8 ordinal,native_index;
    assert(sc64_pool_init(&arena));
    assert(sc64_pool_acquire(&arena,&ordinal,&native_index));
    assert(sc64_interceptor_projectile_launch(&row,64*512,64*512,
        u[parent].x*2,u[parent].y*2,64,(u16)parent,(u8)child,7));
    assert(sc64_pool_set_row(&arena,ordinal,&row));
    bad=good;
    assert(sc64_codec_encode(&arena,bad.interceptor_projectiles,SC64_CODEC_BYTES));
    reject_without_live_mutation(&bad);
}

static void test_script_cleanup_and_reuse(void)
{
    int parent=setup();
    for(unsigned n=0;n<3;++n) choose_command(MENU_STOP+1);
    race_p7_carrier_training_source_tick();
    for(unsigned tick=0;tick<900;++tick) race_p7_carrier_training_source_tick();
    assert(race_p7_carrier_training_child_count(parent)==3);
    int middle=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].type==U_P7_INTERCEPTOR &&
        sc64_p7_carrier_child_rank(&u[i])==1) middle=i;
    assert(middle>=0);
    u[middle].alive=0; /* Script removal bypasses hit_target. */
    race_p7_carrier_training_end_frame();
    assert(race_p7_carrier_training_child_count(parent)==2);
    assert(race_p7_carrier_training_live_world_valid());
    assert(spawn_unit(SIDE,U_PROBE,64,64)==middle);
    assert(race_save_game() && race_load_game());
    static RaceSave good,bad;
    good=race_data;
    int child=find_child(parent,SC64_P7_INTERCEPTOR_INSIDE);
    assert(child>=0);
    for(unsigned test=0;test<2;++test) {
        Unit actor=u[child];
        /* Two children remain. A repeated rank and a missing rank both fail. */
        unsigned rank=sc64_p7_carrier_child_rank(&actor);
        sc64_p7_carrier_child_set_rank(&actor,test?2:1u-rank);
        bad=good;
        assert(sc64_p7_typed_unit_encode(&bad.units[child],&actor,
            (unsigned)(race_cols*16),(unsigned)(race_rows*16)));
        reject_without_live_mutation(&bad);
        save_write(&good,sizeof(good),SAVE_VERSION); assert(race_load_game());
    }
    u[parent].alive=0;
    race_p7_carrier_training_end_frame();
    assert(race_p7_carrier_training_child_count(parent)==0);
    assert(race_p7_carrier_training_live_world_valid());
    assert(race_save_game() && race_load_game());
}

static unsigned free_slots(void)
{
    unsigned count=0;
    for(int i=0;i<MAX_UNITS;++i) count+=!u[i].alive;
    return count;
}

static void test_slot_reservations(void)
{
    int parent=setup();
    assert(race_p7.created_mask==0);
    while(free_slots()>4) assert(spawn_unit(SIDE,U_PROBE,64,64)>=0);
    choose_command(MENU_STOP+1);
    assert(u[parent].carry==1 && minerals==75);
    choose_command(MENU_STOP+1);
    assert(u[parent].carry==1 && minerals==75);
    /* The paid child consumes one slot; three remain reserved for scripts. */
    race_p7_carrier_training_source_tick();
    assert(find_child(parent,SC64_P7_INTERCEPTOR_TRAINING)>=0 && free_slots()==3);
    assert(!race_p7_carrier_training_slots_available(1));
    /* An ordinary producer's paid order competes for the same slots. */
    race_p7.created_mask=1;
    assert(race_p7_carrier_training_slots_available(1));
    int building=-1;
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive) { building=i; break; }
    assert(building>=0);
    b[building].queue_head=0; b[building].queue_count=1;
    b[building].queue[0]=U_PROBE;
    assert(!race_p7_carrier_training_slots_available(1));
    b[building].queue[0]=U_ZERGLING;
    assert(!race_p7_carrier_training_slots_available(0));
}

static void test_source_owner_queue(void)
{
    (void)setup();
    int parent=-1;
    for(int i=0;i<MAX_UNITS;++i) {
        u[i].selected=0;
        if(u[i].alive && u[i].type==U_P7_CARRIER && u[i].raw_owner==4) parent=i;
    }
    assert(parent>=0 && u[parent].owner==ENEMY);
    u[parent].selected=1;
    assert(!race_p7_carrier_training_menu_available());
    assert(!race_p7_carrier_training_queue_selected() && minerals==100);
    /* Advance a pre-existing source-owner queue; this does not add AI orders. */
    u[parent].selected=0; u[parent].carry=1;
    race_p7_carrier_training_source_tick();
    int child=find_child(parent,SC64_P7_INTERCEPTOR_TRAINING);
    assert(child>=0 && u[child].raw_owner==4 && u[child].owner==ENEMY);
    assert(race_save_game() && race_load_game());
    for(unsigned tick=0;tick<300;++tick) race_p7_carrier_training_source_tick();
    assert(find_child(parent,SC64_P7_INTERCEPTOR_INSIDE)==child);
    assert(race_save_game() && race_load_game());
}

static void test_hidden_damage_sight_and_pause(void)
{
    int parent=setup();
    choose_command(MENU_STOP+1); race_p7_carrier_training_source_tick();
    int child=find_child(parent,SC64_P7_INTERCEPTOR_TRAINING);
    int hp=u[child].hp,shields=u[child].shield;
    hit_target(child,10000,U_MARINE,ENEMY);
    hit_target(parent,1,U_FIREBAT,ENEMY);
    assert(u[child].alive && u[child].hp==hp && u[child].shield==shields);
    update_exploration(); assert(!sc64_unit_visibility[child].active);
    game_update(KEY_START,KEY_START); assert(screen==SCREEN_PAUSE);
    Unit before=u[child]; u8 phase=race_p7.reserved[0]; int paused_frame=frame;
    for(unsigned i=0;i<20;++i) game_update(0,0);
    assert(memcmp(&before,&u[child],sizeof(before))==0 &&
           phase==race_p7.reserved[0] && paused_frame==frame);
    /* Isolate optical sight: a hidden child alone contributes to neither
     * human fog nor the source-owner optical query. Restore via next setup. */
    for(int i=0;i<MAX_UNITS;++i) if(i!=child) u[i].alive=0;
    for(int i=0;i<MAX_BUILDINGS;++i) b[i].alive=0;
    sc64_visibility_reset(); update_exploration();
    int cell=(u[child].y/16)*MAP_COLS+u[child].x/16;
    assert(!SC64_GRID_GET(visible,cell));
    u[child].owner=ENEMY; u[child].raw_owner=4;
    assert(!sc64_p7_observer_owner_sees_cell(4,ENEMY,cell));
}

static void test_profile_matrix(void)
{
    for(int mission=0;mission<=SC64_P7_GLOBAL_ID;++mission) {
        race_mode=0; race_selected_mission=mission; init_state();
        if(screen==SCREEN_PLAY && !mission_unsupported_count)
            assert(race_save_game() && race_load_game());
    }
    for(int map=0;map<2;++map) for(int human=0;human<3;++human)
        for(int bots=1;bots<=3;++bots) {
            race_mode=1; race_human=human; race_bots=bots; race_skirmish_map=map;
            race_skirmish_init();
            assert(race_save_game() && race_load_game());
            assert(race_mode && race_human==human && race_bots==bots && race_skirmish_map==map);
        }
}

int main(void)
{
    game_init();
    assert(SC64_RACE_SAVE_GENERATION==42u && sizeof(RaceSave)==16292u);
    assert(sizeof(Sc64ZpRouteSaveScratch)==sizeof(u16)*MAP_CELLS);
    test_training_and_hidden_state();
    test_save_rejection();
    test_script_cleanup_and_reuse();
    test_slot_reservations();
    test_source_owner_queue();
    test_hidden_damage_sight_and_pause();
    test_profile_matrix();
    puts("PASS Carrier price/capacity/clock/save, 15 atomic rejections, cleanup/reuse, reservations, hidden damage/sight, pause and 18 skirmish setups");
    return 0;
}
