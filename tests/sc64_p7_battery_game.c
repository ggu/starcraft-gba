/* Actual-engine fixture for the gated P7 Shield Battery prototype.
 * It exercises the authenticated P7 map, paid Probe build path, UI command,
 * shield service, and the packed campaign save/load candidate path. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static RaceSave battery_candidate;

static Unit battery_candidate_unit(RaceSave *candidate,int slot)
{
    Unit actor;
    assert(race_save_decode_unit(candidate,(unsigned)slot,race_cols*16,
                                 race_rows*16,&actor));
    return actor;
}

static void battery_candidate_store_unit(RaceSave *candidate,int slot,
                                         const Unit *actor)
{
    assert(sc64_zp_unit_save_encode(&candidate->units[slot],actor,
                                    race_cols*16,race_rows*16));
}

static Building battery_candidate_building(RaceSave *candidate,int slot)
{
    Building building;
    assert(race_save_decode_building(candidate,(unsigned)slot,&building));
    return building;
}

static void battery_candidate_store_building(RaceSave *candidate,int slot,
                                             const Building *building)
{
    assert(sc64_building_save_encode(&candidate->buildings[slot],building,
                                     race_cols*16,race_rows*16));
}

static void test_p7_union_tail_guard(void)
{
    RaceSave candidate;
    u8 *payload;
    memset(&candidate,0,sizeof(candidate));
    candidate.state[0]=0;
    candidate.state[2]=SC64_P5_GLOBAL_ID;
    assert(sizeof(candidate.p6)>sizeof(candidate.p5));
    assert(race_p7_union_tail_valid(&candidate));
    payload=(u8 *)&candidate.p7;
    payload[sizeof(candidate.p5)]=0xa5;
    assert(!race_p7_union_tail_valid(&candidate));

    memset(&candidate,0,sizeof(candidate));
    candidate.state[0]=0;
    candidate.state[2]=SC64_P7_GLOBAL_ID;
    assert(sizeof(candidate.p6)>sizeof(candidate.p7));
    assert(race_p7_union_tail_valid(&candidate));
    payload=(u8 *)&candidate.p7;
    payload[sizeof(candidate.p7)]=0x5a;
    assert(!race_p7_union_tail_valid(&candidate));
}

static void test_battery_candidate_guards(int unit_slot,int battery_slot)
{
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(u[unit_slot].type);
    assert(rule && rule->shields>0);
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    assert(race_p5_transport_save_candidate_valid(&battery_candidate,
                                                   race_cols,race_rows));

    /* P7 recharge is accepted only in the active P7 candidate context. */
    battery_candidate.state[0]=1;
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    battery_candidate.state[0]=0;
    battery_candidate.state[2]=SC64_P7_GLOBAL_ID-1;
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));

    Unit actor=battery_candidate_unit(&battery_candidate,unit_slot);
    actor.owner=actor.owner==SIDE?ENEMY:SIDE;
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    actor=battery_candidate_unit(&battery_candidate,unit_slot);
    actor.raw_owner=(actor.raw_owner+1)&7;
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));

    actor=battery_candidate_unit(&battery_candidate,unit_slot);
    actor.type=U_DRONE;
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    actor=battery_candidate_unit(&battery_candidate,unit_slot);
    actor.shield=rule->shields;
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));

    actor=battery_candidate_unit(&battery_candidate,unit_slot);
    actor.order=ORDER_TRANSPORTED;
    actor.target=unit_slot;
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    actor=battery_candidate_unit(&battery_candidate,unit_slot);
    actor.order=ORDER_P7_RECHARGE;
    actor.target=-1; /* stale/missing Battery slot */
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));

    /* Existing P5 transport-order validation remains active in P7 saves. */
    actor=battery_candidate_unit(&battery_candidate,unit_slot);
    actor.order=ORDER_BOARD;
    actor.target=unit_slot;
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));

    Building target=battery_candidate_building(&battery_candidate,battery_slot);
    target.type=B_GATEWAY;
    battery_candidate_store_building(&battery_candidate,battery_slot,&target);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    target=battery_candidate_building(&battery_candidate,battery_slot);
    target.alive=0;
    battery_candidate_store_building(&battery_candidate,battery_slot,&target);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    target=battery_candidate_building(&battery_candidate,battery_slot);
    target.owner=ENEMY;
    battery_candidate_store_building(&battery_candidate,battery_slot,&target);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    target=battery_candidate_building(&battery_candidate,battery_slot);
    target.raw_owner=(target.raw_owner+1)&7;
    battery_candidate_store_building(&battery_candidate,battery_slot,&target);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    target=battery_candidate_building(&battery_candidate,battery_slot);
    target.under_construction=1;
    battery_candidate_store_building(&battery_candidate,battery_slot,&target);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    target=battery_candidate_building(&battery_candidate,battery_slot);
    target.queue_progress=0;
    battery_candidate_store_building(&battery_candidate,battery_slot,&target);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
}

static int source_actor(unsigned raw_owner, unsigned raw_type, int building)
{
    if (building) {
        for (int i=0;i<MAX_BUILDINGS;++i)
            if (b[i].alive && b[i].raw_owner==(int)raw_owner &&
                b[i].raw_type==(int)raw_type) return i;
    } else {
        for (int i=0;i<MAX_UNITS;++i)
            if (u[i].alive && u[i].raw_owner==(int)raw_owner &&
                u[i].raw_type==(int)raw_type) return i;
    }
    return -1;
}

static int legal_battery_site(int from_x,int from_y,int *out_x,int *out_y)
{
    int best=0x7fffffff,found=0;
    for (int y=16;y<race_rows*16-16;y+=8)
        for (int x=16;x<race_cols*16-16;x+=8)
            if (race_can_site(SIDE,B_P7_SHIELD_BATTERY,x,y) &&
                can_place(B_P7_SHIELD_BATTERY,x,y)) {
                int distance=dist2(from_x,from_y,x,y);
                if (distance<best) {
                    best=distance; *out_x=x; *out_y=y; found=1;
                }
            }
    return found;
}

int main(void)
{
    static const char *menu_labels[MENU_COUNT]={0};
    test_p7_union_tail_guard();
    /* OpenBW square_at(...,128) is half-open. Actors touching either
       boundary from outside do not intersect, while a one-pixel overlap does. */
    assert(!sc64_p7_battery_click_overlap(128,0,140,10,0,0));
    assert(sc64_p7_battery_click_overlap(127,0,140,10,0,0));
    assert(!sc64_p7_battery_click_overlap(0,128,10,140,0,0));
    assert(sc64_p7_battery_click_overlap(0,127,10,140,0,0));
    assert(!sc64_p7_battery_click_overlap(-140,0,-128,10,0,0));
    assert(!sc64_p7_battery_click_overlap(0,-140,10,-128,0,0));

    game_init();
    race_mode=0;
    race_selected_mission=SC64_P7_GLOBAL_ID;
    race_campaign_init();
    assert(screen==SCREEN_PLAY && !mission_unsupported_count);
    assert(race_p7_active() && race_p7_runtime_state_valid());
    assert(race_p7_battery_live_world_valid());

    /* The authenticated source map has enemy Batteries but no invented
       human Battery. Their Q8 starting energy comes from generated CHK rows. */
    int source0=source_actor(0,172,1), source4=source_actor(4,172,1);
    assert(source0>=0 && source4>=0);
    assert(b[source0].queue_progress==SC64_P7_BATTERY_MAX_ENERGY_Q8);
    assert(b[source4].queue_progress==SC64_P7_BATTERY_MAX_ENERGY_Q8);
    assert(source_actor(7,172,1)<0);
    b[source0].alive=0;
    assert(!race_p7_battery_initialize_source(&sc64_race_maps[SC64_P7_GLOBAL_ID]));
    b[source0].alive=1;
    assert(race_p7_battery_live_world_valid());

    int probe=source_actor(7,64,0), gateway=source_actor(7,160,1);
    assert(probe>=0 && gateway>=0 && !b[gateway].under_construction);
    int site_x=-1,site_y=-1;
    assert(legal_battery_site(u[probe].x,u[probe].y,&site_x,&site_y));

    /* Exercise the same contextual UI entry that opens a paid Battery build. */
    for (int i=0;i<MAX_UNITS;++i) u[i].selected=0;
    u[probe].selected=1;
    selected_building=-1; menu_open=1;
    assert(race_p7_battery_build_available());
    assert(!strcmp(field_menu_base_label(MENU_STOP+3,menu_labels),"SHIELD BATTERY"));
    menu_cursor=MENU_STOP+3;
    game_render(); /* Render the complete contextual Probe menu under ASan. */
    choose_command(MENU_STOP+3);
    assert(mode==MODE_BUILD && menu==B_P7_SHIELD_BATTERY);
    minerals=100; gas=0; cx=site_x; cy=site_y;
    start_build(B_P7_SHIELD_BATTERY);
    assert(u[probe].order==ORDER_BUILD && minerals==0);
    int human_battery=u[probe].target;
    assert(human_battery>=0 && b[human_battery].alive &&
           b[human_battery].raw_owner==7 && b[human_battery].raw_type==172 &&
           b[human_battery].under_construction);

    /* Arrive through the real Probe build-order path, serialize construction,
       reload it, then advance the actual Protoss building timer to completion. */
    for (int step=0;step<10000 &&
         dist2(u[probe].x,u[probe].y,b[human_battery].x,b[human_battery].y)>36*36;
         ++step)
        move_toward(&u[probe],b[human_battery].x,b[human_battery].y);
    assert(race_builder_arrived(probe));
    assert(u[probe].order==ORDER_IDLE && u[probe].target==-1 &&
           b[human_battery].under_construction &&
           b[human_battery].build_progress==0);
    for (int tick=0;tick<3;++tick) race_tick_buildings();
    assert(b[human_battery].under_construction &&
           b[human_battery].build_progress==3);
    int construction_progress=b[human_battery].build_progress;
    int construction_energy=b[human_battery].queue_progress;
    assert(race_save_game());
    memset(&race_p7_world,0,sizeof(race_p7_world));
    memset(&race_p7_ops,0,sizeof(race_p7_ops));
    race_p7_map=0;
    b[human_battery].queue_progress=0;
    b[human_battery].build_progress=9;
    assert(race_load_game());
    assert(b[human_battery].under_construction &&
           b[human_battery].build_progress==construction_progress &&
           b[human_battery].queue_progress==construction_energy &&
           u[probe].order==ORDER_IDLE && u[probe].target==-1);
    assert(race_p7_map==&sc64_race_maps[SC64_P7_GLOBAL_ID] &&
           race_p7_world.map==race_p7_map &&
           race_p7_ops.context==&race_p7_world &&
           race_p7_ops.condition==sc64_p7_world_condition &&
           race_p7_ops.action==sc64_p7_world_action);
    assert(race_p7_runtime_tick(race_data.campaign.elapsed_frames+1));
    const Sc64RaceBuildingRule *battery_rule=sc64_race_building_rule(B_P7_SHIELD_BATTERY);
    assert(battery_rule && battery_rule->build_frames>0);
    int construction_frames=(battery_rule->build_frames*5+3)/4;
    for (int tick=construction_progress;tick<construction_frames-1;++tick) {
        race_tick_buildings();
        assert(b[human_battery].under_construction);
    }
    race_tick_buildings();
    assert(!b[human_battery].under_construction &&
           b[human_battery].build_progress==0 &&
           b[human_battery].queue_progress==construction_energy);

    /* Verify the recharge UI path on the completed, paid-for Battery. */
    assert(!b[human_battery].under_construction);
    u[probe].selected=0;
    selected_building=human_battery; menu_open=1;
    assert(race_p7_battery_command_available());
    assert(!strcmp(field_menu_base_label(MENU_STOP+1,menu_labels),"RECHARGE"));
    menu_cursor=MENU_STOP+1;
    game_render();
    choose_command(MENU_STOP+1);
    assert(mode==MODE_P7_RECHARGE);

    /* Keep the target identity/race native: a map Protoss Zealot is moved
       into the Battery's inclusive source-rectangle range for this fixture. */
    int zealot=source_actor(7,65,0);
    assert(zealot>=0);
    int click_found=0;
    for (int dy=-60;dy<=60 && !click_found;dy+=4)
        for (int dx=-60;dx<=60 && !click_found;dx+=4) {
            int x=b[human_battery].x+dx, y=b[human_battery].y+dy;
            if (x<0 || y<0 || x>=WORLD_W || y>=WORLD_H) continue;
            u[zealot].x=x; u[zealot].y=y; cx=x; cy=y;
            if (sc64_p7_battery_distance_unit(zealot,human_battery)<=128 &&
                selection_target()==zealot) click_found=1;
        }
    assert(click_found);
    u[zealot].shield=0; u[zealot].shield_remainder=0;
    b[human_battery].queue_progress=100u*256u;
    int issued=race_p7_battery_issue_command(human_battery,cx,cy);
    assert(issued==1 && u[zealot].order==ORDER_P7_RECHARGE);
    int shield_before=u[zealot].shield;
    int energy_before=b[human_battery].queue_progress;
    assert(race_p7_battery_tick_unit(zealot));
    assert(u[zealot].shield>shield_before &&
           b[human_battery].queue_progress<energy_before);
    u[zealot].cd=3;
    update_orders();
    assert(u[zealot].cd==2 && u[zealot].order==ORDER_P7_RECHARGE &&
           u[zealot].target==human_battery);
    assert(race_p7_battery_live_world_valid());
    assert(race_p7_runtime_state_valid());
    for (int i=0;i<MAX_UNITS;++i) {
        Sc64ZpPackedUnitSave row;
        assert(sc64_zp_unit_save_encode(&row,&u[i],race_cols*16,race_rows*16));
    }
    for (int i=0;i<MAX_BUILDINGS;++i) {
        Sc64PackedBuildingSave row;
        assert(sc64_building_save_encode(&row,&b[i],race_cols*16,race_rows*16));
    }
    int valid_battery_energy=b[human_battery].queue_progress;
    int valid_shield=u[zealot].shield;
    /* Candidate validation must reject both a corrupt Recharge reference and
       a transport-only order on the P7 mission, without changing the world. */
    u[zealot].target=-1;
    assert(!race_save_game());
    u[zealot].target=human_battery;
    u[zealot].work_timer=1;
    assert(!race_save_game());
    u[zealot].work_timer=0;
    const Sc64RaceUnitRule *zealot_rule=sc64_race_unit_rule(u[zealot].type);
    assert(zealot_rule && zealot_rule->shields>0);
    u[zealot].shield=zealot_rule->shields;
    assert(!race_save_game());
    u[zealot].shield=valid_shield;
    u[zealot].order=ORDER_BOARD;
    assert(!race_save_game());
    u[zealot].order=ORDER_P7_RECHARGE;
    b[human_battery].queue_progress=0;
    assert(!race_save_game());
    b[human_battery].queue_progress=valid_battery_energy;
    b[human_battery].queue_count=1;
    assert(!race_save_game());
    b[human_battery].queue_count=0;
    b[human_battery].level=5;
    assert(!race_save_game());
    b[human_battery].level=0;

    /* The actual save adapter validates and round-trips Battery energy,
       shield fraction, order and target in the mission payload. */
    assert(race_save_game());
    test_battery_candidate_guards(zealot,human_battery);
    int saved_energy=b[human_battery].queue_progress;
    int saved_shield=u[zealot].shield;
    int saved_remainder=u[zealot].shield_remainder;
    b[human_battery].queue_progress=0;
    u[zealot].shield=0; u[zealot].shield_remainder=0;
    u[zealot].order=ORDER_IDLE; u[zealot].target=-1;
    assert(race_load_game());
    assert(b[human_battery].queue_progress==saved_energy &&
           u[zealot].shield==saved_shield &&
           u[zealot].shield_remainder==saved_remainder &&
           u[zealot].order==ORDER_P7_RECHARGE &&
           u[zealot].target==human_battery);

    /* Even an identical replacement must not inherit the dead instance's
       recharge orders. This allocator path also covers trigger removals. */
    int battery_x=b[human_battery].x, battery_y=b[human_battery].y;
    b[human_battery].alive=0;
    int replacement=spawn_build(SIDE,B_P7_SHIELD_BATTERY,
                                  battery_x,battery_y,0);
    assert(replacement==human_battery && b[replacement].raw_owner==7 &&
           b[replacement].raw_type==172 && b[replacement].queue_progress>0);
    assert(u[zealot].order==ORDER_IDLE && u[zealot].target==-1 &&
           u[zealot].work_timer==0 && !u[zealot].path_timer &&
           !u[zealot].path_retry);
    int replacement_energy=b[replacement].queue_progress;
    assert(!race_p7_battery_tick_unit(zealot));
    assert(u[zealot].shield==saved_shield &&
           b[replacement].queue_progress==replacement_energy);

    /* Combat death also clears orders immediately, so saving on that update
       never depends on waiting for the recharging actor's next turn. */
    u[zealot].order=ORDER_P7_RECHARGE;
    u[zealot].target=replacement;
    hit_target(MAX_UNITS+replacement,10000,U_HYDRALISK,ENEMY);
    assert(!b[replacement].alive && u[zealot].order==ORDER_IDLE &&
           u[zealot].target==-1);
    assert(race_save_game());
    b[source0].alive=0;
    assert(race_p7_battery_live_world_valid());
    assert(race_save_game()); /* Destroying a source Battery remains saveable. */
    puts("P7 Battery construction, recharge, reference lifetime and saves passed");
    return 0;
}
