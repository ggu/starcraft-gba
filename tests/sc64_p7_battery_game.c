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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(sc64_p7_typed_unit_encode(&candidate->units[slot],actor,
                                     race_cols*16,race_rows*16));
#else
    assert(sc64_zp_unit_save_encode(&candidate->units[slot],actor,
                                    race_cols*16,race_rows*16));
#endif
}

#ifdef SC64_P7_TYPED_COMPACT_ENABLED
static void battery_candidate_store_overmax_shield(RaceSave *candidate,int slot,
                                                    const Unit *actor)
{
    Unit valid=*actor;
    Sc64P7TypedUnitSave row;
    int maxhp,armor,maxshield;
    assert(sc64_p7_typed_unit_canonical(valid.type,&maxhp,&armor,&maxshield));
    valid.maxhp=maxhp; valid.armor=(u8)armor; valid.shield=maxshield;
    assert(sc64_p7_typed_unit_encode(&row,&valid,race_cols*16,race_rows*16));
    /* Unit layout places the 9-bit shield field at bit 177. Craft an
       over-maximum value that the ordinary encoder correctly refuses. */
    const unsigned bit=177u;
    unsigned byte=bit>>3,shift=bit&7u;
    u16 word=(u16)row.bytes[byte] | ((u16)row.bytes[byte+1]<<8);
    word=(u16)((word&~(0x1ffu<<shift))|((unsigned)(maxshield+1)<<shift));
    row.bytes[byte]=(u8)word; row.bytes[byte+1]=(u8)(word>>8);
    candidate->units[slot]=row;
}
#endif

static Building battery_candidate_building(RaceSave *candidate,int slot)
{
    Building building;
    assert(race_save_decode_building(candidate,(unsigned)slot,&building));
    return building;
}

static void battery_candidate_store_building(RaceSave *candidate,int slot,
                                             const Building *building)
{
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(sc64_p7_typed_building_encode(&candidate->buildings[slot],building, race_cols*16,race_rows*16));
#else
    assert(sc64_building_save_encode(&candidate->buildings[slot],building, race_cols*16,race_rows*16));
#endif
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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    { int armor,maxshield; assert(sc64_p7_typed_unit_canonical(actor.type,&actor.maxhp,&armor,&maxshield)); actor.armor=(u8)armor; actor.shield=0; actor.shield_remainder=0; }
#endif
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    actor=battery_candidate_unit(&battery_candidate,unit_slot);
    actor.shield=rule->shields;
    actor.shield_remainder=0;
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    { int maxhp,armor,maxshield; assert(sc64_p7_typed_unit_canonical(actor.type,&maxhp,&armor,&maxshield)); actor.shield=maxshield; }
#endif
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
    /* Reaching full shields between the Recharge actor's service slot and a
       pause is a valid snapshot; its next order tick completes the order. */
    assert(race_p5_transport_save_candidate_valid(&battery_candidate,
                                                   race_cols,race_rows));
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    actor.shield=rule->shields+1;
    battery_candidate_store_overmax_shield(&battery_candidate,unit_slot,&actor);
#else
    actor.shield=rule->shields+1;
    battery_candidate_store_unit(&battery_candidate,unit_slot,&actor);
#endif
    assert(!race_p5_transport_save_candidate_valid(&battery_candidate,
                                                    race_cols,race_rows));
    actor.shield=rule->shields;
    actor.shield_remainder=1;
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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    target.maxhp=sc64_p7_typed_building_maxhp(target.type);
#endif
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
    /* Another Recharge actor can consume the last energy before this actor's
       service slot; preserve that pending order for next-tick cancellation. */
    assert(race_p5_transport_save_candidate_valid(&battery_candidate,
                                                   race_cols,race_rows));
    memcpy(&battery_candidate,&race_data,sizeof(battery_candidate));
    target=battery_candidate_building(&battery_candidate,battery_slot);
    int changed_pylon=0;
    for (int i=0;i<MAX_BUILDINGS;++i) {
        Building pylon=battery_candidate_building(&battery_candidate,i);
        if (!pylon.alive || pylon.type!=B_PYLON || pylon.raw_type!=156 ||
            pylon.owner!=target.owner || pylon.raw_owner!=target.raw_owner) continue;
        pylon.raw_owner=(pylon.raw_owner+1)&7;
        battery_candidate_store_building(&battery_candidate,i,&pylon);
        ++changed_pylon;
    }
    assert(changed_pylon>0);
    /* Power loss is transient runtime state, not malformed save identity. */
    assert(race_p5_transport_save_candidate_valid(&battery_candidate,
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

#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
static void cache_frames(unsigned count)
{
    sc64_target_index_active=0;
    while (count--) {
        sc64_p7_arbiter_cache_frame_tick();
        sc64_p7_arbiter_cache_process_due();
    }
}
#endif

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
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(SC64_RACE_SAVE_GENERATION==40u);
#elif defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED)
    assert(SC64_RACE_SAVE_GENERATION==39u);
#endif
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
    /* Pinned OpenBW psi-field edges/mask in source-pixel coordinates. */
    assert(sc64_p7_in_psi_range(127,0));
    assert(!sc64_p7_in_psi_range(128,0));
    assert(sc64_p7_in_psi_range(-127,0));
    assert(!sc64_p7_in_psi_range(-128,0));
    assert(sc64_p7_in_psi_range(0,79));
    assert(!sc64_p7_in_psi_range(0,80));
    assert(sc64_p7_in_psi_range(0,-79));
    assert(!sc64_p7_in_psi_range(96,64)); /* false cell at mask row 4, col 6 */

    game_init();
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    assert(screen==SCREEN_TITLE);
    /* P7 is still catalog-locked; seed the normal Briefing page with this
       private fixture's authenticated mission and launch through Start. */
    sc64_frontend_reset(&race_front,SC64_P7_GLOBAL_ID,R_PROTOSS,1,-1);
    race_front.page=SC_FRONT_BRIEFING;
    race_front.mission=SC64_P7_GLOBAL_ID;
    game_update(KEY_START,KEY_START); game_update(0,0);
    assert(screen==SCREEN_PLAY && !mission_unsupported_count);
    assert(sc64_p7_arbiter_cache_initialized);
#else
    race_mode=0;
    race_selected_mission=SC64_P7_GLOBAL_ID;
    race_campaign_init();
    assert(screen==SCREEN_PLAY && !mission_unsupported_count);
#endif
    assert(race_p7_active() && race_p7_runtime_state_valid());
    assert(race_p7_battery_live_world_valid());

    /* The authenticated source map has enemy Batteries but no invented
       human Battery. Their Q8 starting energy comes from generated CHK rows. */
    int source0=source_actor(0,172,1), source4=source_actor(4,172,1);
    assert(source0>=0 && source4>=0);
    assert(b[source0].queue_progress==SC64_P7_BATTERY_MAX_ENERGY_Q8);
    assert(b[source4].queue_progress==SC64_P7_BATTERY_MAX_ENERGY_Q8);
    assert(source_actor(7,172,1)<0);
    /* Source players 0 and 4 share a live opponent owner, but their Pylons
       must not cross-power each other's Batteries. */
    int test_pylon=spawn_build(b[source0].owner,B_PYLON,b[source0].x,
                               b[source0].y,0);
    assert(test_pylon>=0);
    assert(b[source0].owner==b[source4].owner);
    b[test_pylon].raw_type=156;
    b[test_pylon].owner=b[source0].owner;
    int other_pylons[MAX_BUILDINGS],other_raw_owner[MAX_BUILDINGS],other_count=0;
    for (int i=0;i<MAX_BUILDINGS;++i) if (i!=test_pylon && b[i].alive &&
        b[i].type==B_PYLON && b[i].raw_type==156) {
        other_pylons[other_count]=i;
        other_raw_owner[other_count++]=b[i].raw_owner;
        b[i].raw_owner=6;
    }
    b[test_pylon].raw_owner=0;
    assert(sc64_p7_battery_powered_live(source0));
    assert(!sc64_p7_battery_powered_live(source4));
    b[test_pylon].raw_owner=4;
    b[test_pylon].x=b[source4].x; b[test_pylon].y=b[source4].y;
    assert(sc64_p7_battery_powered_live(source4));
    assert(!sc64_p7_battery_powered_live(source0));
    b[test_pylon].under_construction=1;
    assert(!sc64_p7_battery_powered_live(source4));
    b[test_pylon].under_construction=0;
    b[test_pylon].alive=0;
    assert(!sc64_p7_battery_powered_live(source4));
    for (int n=0;n<other_count;++n)
        b[other_pylons[n]].raw_owner=other_raw_owner[n];
    b[source0].alive=0;
    assert(!race_p7_battery_initialize_source(&sc64_race_maps[SC64_P7_GLOBAL_ID]));
    b[source0].alive=1;
    assert(race_p7_battery_live_world_valid());

    int probe=source_actor(7,64,0), gateway=source_actor(7,160,1);
    assert(probe>=0 && gateway>=0 && !b[gateway].under_construction);
    int pylon_slots[MAX_BUILDINGS],pylon_raw_owner[MAX_BUILDINGS];
    int pylon_hp[MAX_BUILDINGS],pylon_shield[MAX_BUILDINGS],pylon_count=0;
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && b[i].type==B_PYLON &&
        b[i].raw_type==156 && b[i].raw_owner==7) {
        pylon_slots[pylon_count]=i;
        pylon_raw_owner[pylon_count]=b[i].raw_owner;
        pylon_hp[pylon_count]=b[i].hp;
        pylon_shield[pylon_count]=b[i].shield;
        ++pylon_count;
    }
    assert(pylon_count>0);
    int pylon_x=b[pylon_slots[0]].x,pylon_y=b[pylon_slots[0]].y;
    assert(race_p7_battery_site_powered(SIDE,pylon_x,pylon_y));
    assert(race_can_site(SIDE,B_P7_SHIELD_BATTERY,pylon_x,pylon_y));
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].raw_owner=0;
    assert(!race_p7_battery_site_powered(SIDE,pylon_x,pylon_y));
    assert(!race_can_site(SIDE,B_P7_SHIELD_BATTERY,pylon_x,pylon_y));
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].raw_owner=pylon_raw_owner[n];
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].under_construction=1;
    assert(!race_p7_battery_site_powered(SIDE,pylon_x,pylon_y));
    assert(!race_can_site(SIDE,B_P7_SHIELD_BATTERY,pylon_x,pylon_y));
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].under_construction=0;
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].alive=0;
    assert(!race_p7_battery_site_powered(SIDE,pylon_x,pylon_y));
    assert(!race_can_site(SIDE,B_P7_SHIELD_BATTERY,pylon_x,pylon_y));
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].alive=1;
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
    /* Native construction does not pause after a placed building loses psi.
       Exercise that distinction on the real paid Battery timer. */
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].raw_owner=6;
    assert(!race_p7_battery_site_powered(SIDE,pylon_x,pylon_y));
    for (int tick=0;tick<3;++tick) race_tick_buildings();
    assert(b[human_battery].under_construction &&
           b[human_battery].build_progress==3);
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].raw_owner=pylon_raw_owner[n];
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

    /* Losing the Battery's source-owner psi field disables it immediately:
       Recharge cancels before any shield or energy transfer, and cancellation
       remains saveable. Restoring power does not resurrect the old order. */
    int power_energy=b[human_battery].queue_progress;
    int power_shield=u[zealot].shield;
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].raw_owner=6;
    assert(!sc64_p7_battery_target_valid(human_battery));
    assert(!race_p7_battery_command_available());
    /* Power is an instantaneous runtime eligibility check. A Pylon can be
       lost after this actor's service slot; saving must preserve that real
       transient order without changing it. */
    assert(race_save_game());
    assert(u[zealot].order==ORDER_P7_RECHARGE &&
           u[zealot].target==human_battery &&
           u[zealot].shield==power_shield &&
           b[human_battery].queue_progress==power_energy);
    assert(race_p7_battery_tick_unit(zealot));
    assert(u[zealot].order==ORDER_IDLE && u[zealot].target==-1 &&
           u[zealot].shield==power_shield &&
           b[human_battery].queue_progress==power_energy);
    assert(race_save_game());
    for (int n=0;n<pylon_count;++n) b[pylon_slots[n]].raw_owner=pylon_raw_owner[n];

    /* Return to a powered valid order for the candidate/save round trip. */
    u[zealot].order=ORDER_P7_RECHARGE; u[zealot].target=human_battery;
    u[zealot].work_timer=0;
    for (int i=0;i<MAX_UNITS;++i) {
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
        Sc64P7TypedUnitSave row;
        assert(sc64_p7_typed_unit_encode(&row,&u[i],race_cols*16,race_rows*16));
#else
        Sc64ZpPackedUnitSave row;
        assert(sc64_zp_unit_save_encode(&row,&u[i],race_cols*16,race_rows*16));
#endif
    }
    for (int i=0;i<MAX_BUILDINGS;++i) {
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
        Sc64P7TypedBuildingSave row;
        assert(sc64_p7_typed_building_encode(&row,&b[i],race_cols*16,race_rows*16));
#else
        Sc64PackedBuildingSave row;
        assert(sc64_building_save_encode(&row,&b[i],race_cols*16,race_rows*16));
#endif
    }
    int valid_battery_energy=b[human_battery].queue_progress;
    int valid_shield=u[zealot].shield;
    int valid_remainder=u[zealot].shield_remainder;
    /* Candidate validation rejects corrupt identity/order fields but accepts
       legitimate end-of-slot states that the next Recharge tick cancels. */
    u[zealot].target=-1;
    assert(!race_save_game());
    u[zealot].target=human_battery;
    u[zealot].work_timer=1;
    assert(!race_save_game());
    u[zealot].work_timer=0;
    const Sc64RaceUnitRule *zealot_rule=sc64_race_unit_rule(u[zealot].type);
    assert(zealot_rule && zealot_rule->shields>0);
    u[zealot].shield=zealot_rule->shields;
    u[zealot].shield_remainder=0;
    assert(race_save_game());
    u[zealot].shield=zealot_rule->shields+1;
    assert(!race_save_game());
    u[zealot].shield=zealot_rule->shields;
    u[zealot].shield_remainder=1;
    assert(!race_save_game());
    u[zealot].shield_remainder=valid_remainder;
    u[zealot].shield=valid_shield;
    u[zealot].order=ORDER_BOARD;
    assert(!race_save_game());
    u[zealot].order=ORDER_P7_RECHARGE;
    b[human_battery].queue_progress=0;
    assert(race_save_game());
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

    /* Kill the last supporting Pylon through the real combat path, then
       immediately pause and use the UI save/load commands before the next
       gameplay update. The order is legitimate in that snapshot; after load,
       its next service tick cancels without transferring shield or energy. */
    assert(sc64_p7_battery_powered_live(human_battery));
    for (int n=0;n<pylon_count;++n) if (b[pylon_slots[n]].alive)
        hit_target(MAX_UNITS+pylon_slots[n],10000,U_HYDRALISK,ENEMY);
    assert(!sc64_p7_battery_powered_live(human_battery));
    assert(u[zealot].order==ORDER_P7_RECHARGE &&
           u[zealot].target==human_battery);
    int transient_energy=b[human_battery].queue_progress;
    int transient_shield=u[zealot].shield;
    assert(screen==SCREEN_PLAY);
    game_update(KEY_START,KEY_START);
    assert(screen==SCREEN_PAUSE);
    game_update(KEY_B,KEY_B);
    assert(screen==SCREEN_PAUSE && u[zealot].order==ORDER_P7_RECHARGE &&
           u[zealot].target==human_battery &&
           b[human_battery].queue_progress==transient_energy &&
           u[zealot].shield==transient_shield);
    u[zealot].order=ORDER_IDLE; u[zealot].target=-1;
    game_update(KEY_L,KEY_L);
    assert(screen==SCREEN_PLAY && u[zealot].order==ORDER_P7_RECHARGE &&
           u[zealot].target==human_battery &&
           b[human_battery].queue_progress==transient_energy &&
           u[zealot].shield==transient_shield &&
           !sc64_p7_battery_powered_live(human_battery));
    assert(race_p7_battery_tick_unit(zealot));
    assert(u[zealot].order==ORDER_IDLE && u[zealot].target==-1 &&
           b[human_battery].queue_progress==transient_energy &&
           u[zealot].shield==transient_shield);
    /* Native update_unit_energy regenerates completed Battery energy without
       consulting disabled/power state. The GBA 30 Hz adapter likewise keeps
       its passive regeneration independent from Recharge availability. */
    race_tick_state();
    assert(b[human_battery].queue_progress>transient_energy &&
           !sc64_p7_battery_powered_live(human_battery));
    assert(race_save_game());
    /* Restore the fixture's support structures after the boundary test so
       later allocator-reuse coverage retains its original slot layout. */
    for (int n=0;n<pylon_count;++n) {
        b[pylon_slots[n]].alive=1;
        b[pylon_slots[n]].raw_owner=pylon_raw_owner[n];
        b[pylon_slots[n]].hp=pylon_hp[n];
        b[pylon_slots[n]].shield=pylon_shield[n];
    }

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
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    /* Generation 39 stores bounded cache state in Unit.weapon and P7's
       three reserved bytes. Native source updates occur 4/5 GBA frames. */
    int arbiter=source_actor(0,71,0);
    int target=source_actor(0,65,0);
    int detector=source_actor(7,84,0);
    assert(arbiter>=0 && target>=0 && detector>=0);
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].raw_owner==0 &&
        u[i].raw_type==71) { u[i].x=u[target].x; u[i].y=u[target].y; }
    int target_x=u[target].x, target_y=u[target].y;
    u[detector].x=target_x; u[detector].y=target_y;
    int cell=(target_x/16)+(target_y/16)*MAP_COLS;
    SC64_GRID_SET(visible,cell,1);
    assert(u[arbiter].owner==u[target].owner);
    cache_frames(5); /* four source updates; newly cloaked target detects now */
    assert(sc64_p7_observer_arbiter_aura_cloaks(target));
    int aura_weapon=u[target].weapon;
    assert(SC64_P7_ARBITER_CACHE_VALID(aura_weapon) &&
           (aura_weapon&SC64_P7_ARBITER_AURA));
    assert(race_save_game());
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].raw_owner==0 &&
        u[i].raw_type==71) u[i].x+=500;
    cache_frames(5);
    assert(!sc64_p7_observer_arbiter_aura_cloaks(target));
    assert(race_load_game());
    assert(u[arbiter].x==target_x && u[arbiter].y==target_y &&
           sc64_p7_observer_arbiter_aura_cloaks(target) &&
           u[target].weapon==aura_weapon);

    cache_frames(5);
    assert(sc64_p7_observer_detector_sees(target,7,SIDE));
    assert(sc64_p7_observer_can_attack_from(U_MARINE,target,SIDE,7));
    assert(race_save_game());
    u[detector].x=0; u[detector].y=0;
    assert(sc64_p7_observer_detector_sees(target,7,SIDE));
    /* The target's detected flag survives detector movement until its
       native timer expires, provided current optical visibility remains. */
    assert(sc64_p7_observer_can_attack_from(U_MARINE,target,SIDE,7));
    SC64_GRID_SET(visible,cell,0);
    assert(!sc64_p7_observer_detector_sees(target,7,SIDE));
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,target,SIDE,7));
    SC64_GRID_SET(visible,cell,1);
    assert(race_load_game());
    assert(u[detector].x==target_x && u[detector].y==target_y &&
           sc64_p7_observer_can_attack_from(U_MARINE,target,SIDE,7));
    u[detector].x=0; u[detector].y=0;
    unsigned left=SC64_P7_ARBITER_COUNTDOWN(u[target].weapon);
    for(unsigned i=0;i<left;++i) {
        sc64_p7_arbiter_cache_source_tick();
        assert(sc64_p7_observer_can_attack_from(U_MARINE,target,SIDE,7));
    }
    assert(SC64_P7_ARBITER_COUNTDOWN(u[target].weapon)==0);
    sc64_p7_arbiter_cache_source_tick();
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,target,SIDE,7));
    assert(race_save_game());
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].raw_owner==0 &&
        u[i].raw_type==71) u[i].alive=0;
    cache_frames(5);
    assert(!sc64_p7_observer_arbiter_aura_cloaks(target));
    assert(race_save_game());
    assert(race_load_game());
    assert(!sc64_p7_observer_arbiter_aura_cloaks(target));
#else
    puts("P7 Battery construction, recharge, reference lifetime and saves passed");
#endif
    return 0;
}
