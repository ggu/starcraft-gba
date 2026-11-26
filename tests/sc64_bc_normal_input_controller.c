/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary custom-game input controller. Game state is observed, never seeded. */
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main

static int select_building_fast(int slot)
{
    if(slot<0 || slot>=MAX_BUILDINGS || !b[slot].alive || b[slot].owner!=SIDE) return 0;
    if(menu_open || mode!=MODE_NORMAL) tap(KEY_B);
    move_cursor(b[slot].x,b[slot].y); tap(KEY_A);
    return selected_building==slot;
}
static int row_raw(int raw)
{
    for (int i = 0; i < field_menu_count(); ++i) {
        Sc64SkirmishMenuRow row;
        if (sc64_skirmish_menu_at(R_TER, (unsigned)i, &row) && row.record && row.record->raw_id == raw) return i;
    }
    return -1;
}
static int find_raw_build(int raw)
{
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].owner==SIDE && sc64_race_raw_building(b[i].type)==raw) return i;
    return -1;
}
static int wait_bank(int ore,int vespene)
{
    while ((minerals<ore || gas<vespene) && host_ticks<match_limit && !won && !lost) input(0);
    fprintf(stderr,"wait bank target=%d/%d actual=%d/%d tick=%u won=%d lost=%d screen=%d\n",ore,vespene,minerals,gas,host_ticks,won,lost,screen);
    return minerals>=ore && gas>=vespene && !won && !lost;
}
static int attack_selected(int x,int y)
{
    if(selected_count()<=0 || !choose_menu_command(MENU_ATTACK) || mode!=MODE_ATTACK) return 0;
    move_cursor(x,y);
    if(mode!=MODE_ATTACK || won || lost) return 0;
    tap(KEY_A);
    return mode==MODE_NORMAL;
}
static void defend_near(int x,int y)
{
    int enemy=-1, marine=-1, best=550*550;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner!=SIDE && u[i].owner!=NEUTRAL) {
        int d=dist2(x,y,u[i].x,u[i].y);if(d<best){best=d;enemy=i;}
    }
    if(enemy<0)return;
    best=0x7fffffff;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE && u[i].type==U_MARINE) {
        int d=dist2(x,y,u[i].x,u[i].y);if(d<best){best=d;marine=i;}
    }
    if(marine>=0 && ((u[marine].selected && selected_count()>0) || select_unit(marine))) {
        tap(KEY_A); /* second click groups visible Marines */
        int ex=u[enemy].x,ey=u[enemy].y;
        fprintf(stderr,"defense marine=%d enemy=%d xy=%d,%d tick=%u\n",marine,enemy,ex,ey,host_ticks);
        (void)attack_selected(ex,ey);
    }
}
/* Keep the Barracks training Marines with spare minerals and supply. */
static void reinforce(int barracks)
{
    static unsigned next;
    if(host_ticks<next) return;
    next=host_ticks+64;
    const int need=sc64_race_unit_rule(sc64_race_unit_from_raw(0))->supply;
#define REINFORCE_READY() (b[barracks].alive && b[barracks].queue_count<2 && minerals>=50 && \
                           race_owner_supply_cap(SIDE)-race_owner_reserved_supply(SIDE)>=need)
    if(!REINFORCE_READY()) return;
    /* A click on the selected Barracks trains; select it only when needed,
       then train once if the queue, bank and supply still allow it. */
    if(menu_open || mode!=MODE_NORMAL) tap(KEY_B);
    if(selected_building!=barracks && !select_building_fast(barracks)) return;
    if(menu_open || mode!=MODE_NORMAL || selected_building!=barracks || !REINFORCE_READY()) return;
#undef REINFORCE_READY
    int before=b[barracks].queue_count; tap(KEY_A);
    if(b[barracks].queue_count>before) fprintf(stderr,"reinforce tick=%u queue=%d bank=%d/%d\n",host_ticks,b[barracks].queue_count,minerals,gas);
}
/* The nearest enemy the Battlecruiser may fire Yamato at, or -1. */
static int yamato_target(int cruiser)
{
    int target=-1, best=0x7fffffff;
    for(int i=0;i<MAX_UNITS+MAX_BUILDINGS;++i) {
        int owner=i<MAX_UNITS?u[i].owner:b[i-MAX_UNITS].owner;
        if(owner==SIDE || owner==NEUTRAL || !race_bc_yamato_target_valid(i,&u[cruiser],1)) continue;
        int d=dist2(u[cruiser].x,u[cruiser].y,target_x(i),target_y(i));
        if(d<best){best=d;target=i;}
    }
    return target;
}
/* Death or loss of sight: the only reasons the witness aims again. */
static int target_gone(int target)
{
    return !target_alive(target) || !currently_visible(target_x(target),target_y(target));
}
/* Move the cursor onto a target that may be moving, until the Yamato
   cursor resolves to it; 0 when it dies or leaves sight first. */
static int aim_cursor(int target)
{
    for(unsigned guard=0;guard<6000u && host_ticks<match_limit && screen==SCREEN_PLAY && !won && !lost;++guard) {
        if(race_bc_yamato_target_at(cx,cy)==target) return 1;
        if(target_gone(target)) return 0;
        int dx=target_x(target)-cx, dy=target_y(target)-cy;
        if(abs(dx)<=1 && abs(dy)<=1) { if(race_bc_yamato_target_at(cx,cy)!=target) return 0; return 1; }
        u16 held=(abs(dx)>=4 && (abs(dy)>=4 || abs(dy)<=1)) || (abs(dy)>=4 && abs(dx)<=1) ? KEY_R : 0;
        if(dx>1) held|=KEY_RIGHT;
        if(dx<-1) held|=KEY_LEFT;
        if(dy>1) held|=KEY_DOWN;
        if(dy<-1) held|=KEY_UP;
        input(held);
    }
    return 0;
}
static int build_raw(int raw)
{
    int type=sc64_race_building_from_raw(raw), worker=-1, x=-1, y=-1;
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
    assert(rule && wait_bank(rule->minerals,rule->gas));
    for(int i=0;i<MAX_UNITS;++i) if(build_worker_available(i,SIDE,U_SCV) && u[i].order!=ORDER_GAS){worker=i;break;}
    assert(worker>=0);
    if(raw==110) {
        int best=-1, distance=0x7fffffff;
        for(int i=0;i<geyser_count;++i) {int d=dist2(race_base_x[0],race_base_y[0],geysers[i].x,geysers[i].y); if(d<distance){distance=d;best=i;}}
        assert(best>=0); x=geysers[best].x; y=geysers[best].y;
    } else if(raw==114 || raw==116) {
        int addon=sc64_race_building_from_raw(raw==114?115:118), best=0x7fffffff;
        for(int yy=128;yy<WORLD_H-32;yy+=16) for(int xx=128;xx<WORLD_W-80;xx+=16) {
            if(!race_can_site(SIDE,type,xx,yy) || !can_place(type,xx,yy) || !can_place(addon,xx+48,yy+8)) continue;
            int d=dist2(race_base_x[0],race_base_y[0],xx,yy);
            if(d<best){best=d;x=xx;y=yy;}
        }
        assert(x>=0);
    } else assert(choose_build_site(type,&x,&y));
    assert(select_unit(worker)); int row=row_raw(raw); assert(row>=0 && choose_menu_command(row));
    assert(mode==MODE_BUILD && menu==type); move_cursor(x,y);
#ifdef SC64_FIELD_CONTROLS
    x=cx; y=cy; field_ui_site(type,&x,&y);
#endif
    combat_build.state=BUILD_PENDING;combat_build.worker=worker;combat_build.type=type;combat_build.x=x;combat_build.y=y;combat_build.building=-1;combat_build.complete=0;
    tap(KEY_A);
    fprintf(stderr,"build requested raw=%d tick=%u bank=%d/%d mode=%d state=%d site=%d,%d\n",raw,host_ticks,minerals,gas,mode,combat_build.state,x,y);
    assert(combat_build.state!=BUILD_NONE);
    while(combat_build.state!=BUILD_NONE && !combat_build.complete && host_ticks<match_limit && !won && !lost) {
        if (combat_build.state==BUILD_FOUNDATION && host_ticks%64==0) defend_near(x,y);
        input(0);
    }
    assert(combat_build.complete && !lost); int slot=combat_build.building;
    fprintf(stderr,"build complete raw=%d slot=%d tick=%u bank=%d/%d\n",raw,slot,host_ticks,minerals,gas);
    combat_build.state=BUILD_NONE; return slot;
}
static void gas_worker(int refinery)
{
    int worker=-1;for(int i=0;i<MAX_UNITS;++i) if(build_worker_available(i,SIDE,U_SCV) && u[i].order!=ORDER_GAS){worker=i;break;}
    assert(worker>=0 && select_unit(worker)); move_cursor(b[refinery].x,b[refinery].y); tap(KEY_A);
    fprintf(stderr,"gas worker=%d order=%d target=%d tick=%u\n",worker,u[worker].order,u[worker].target,host_ticks);
    assert(u[worker].order==ORDER_GAS && u[worker].target==refinery);
}
static int addon_raw(int parent,int raw)
{
    int type=sc64_race_building_from_raw(raw); const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
    assert(rule && wait_bank(rule->minerals,rule->gas));
    assert(select_building_fast(parent)); int row=row_raw(raw);
    fprintf(stderr,"addon raw=%d parent=%d row=%d tick=%u bank=%d/%d\n",raw,parent,row,host_ticks,minerals,gas);
    assert(row>=0 && choose_menu_command(row));
    int slot=find_raw_build(raw); assert(slot>=0);
    while(!building_complete(slot) && b[slot].alive && host_ticks<match_limit && !won && !lost) input(0);
    assert(building_complete(slot) && !lost);
    fprintf(stderr,"addon complete raw=%d slot=%d tick=%u bank=%d/%d\n",raw,slot,host_ticks,minerals,gas);
    return slot;
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    trace = fopen(argv[1], "w");
    assert(trace);
    fputs("{\"profile\":\"unified-race-skirmish\",\"events\":[\n", trace);
    option_map = 1;
    option_human = R_TER;
    option_bots = 1;
    option_bot_races[0] = R_ZERG;
    match_limit = 250000;
    gba_init();
    assert(launch_match());
    fprintf(stderr, "start tick=%u minerals=%d gas=%d bots=%d workers=%d\n", host_ticks, minerals, gas, race_bots, count_units(SIDE,U_SCV));
    /* Custom matches open with the native Melee bank of 50 minerals (#306). */
    assert(minerals == 50 && gas == 0 && race_bots == 1 && count_units(SIDE,U_SCV) > 0);
    while (minerals < 150 && host_ticks < match_limit && !won && !lost) input(0);
    fprintf(stderr, "ready tick=%u minerals=%d gas=%d won=%d lost=%d\n", host_ticks, minerals, gas, won, lost);
    assert(!won && !lost && minerals >= 150);
    assert(build_barracks());
    while (combat_build.state != BUILD_NONE && !combat_build.complete && host_ticks < match_limit && !won && !lost) input(0);
    assert(combat_build.complete && !lost);
    fprintf(stderr, "barracks complete tick=%u minerals=%d gas=%d\n", host_ticks, minerals, gas);
    int barracks=combat_build.building;
    combat_build.state=BUILD_NONE;
    assert(select_building_fast(barracks));
    for(int i=0;i<4;++i) { assert(wait_bank(50,0)); int before=b[barracks].queue_count; tap(KEY_A); assert(b[barracks].queue_count>before); }
    fprintf(stderr,"queued four defenders tick=%u bank=%d/%d\n",host_ticks,minerals,gas);
    /* Four more SCVs: at the native mining pace one CC's opening workers
       cannot fund the Battlecruiser path before the Zerg computer attacks. */
    int command=find_raw_build(106);
    assert(command>=0 && select_building_fast(command));
    /* Queued plus live SCVs: an SCV may finish during the purchase tap. */
    for(int i=0;i<4;++i) { assert(wait_bank(50,0)); while(b[command].queue_count>=4 && !won && !lost) input(0);
        int before=b[command].queue_count+count_units(SIDE,U_SCV); tap(KEY_A); assert(b[command].queue_count+count_units(SIDE,U_SCV)>before); }
    fprintf(stderr,"queued four SCVs tick=%u bank=%d/%d\n",host_ticks,minerals,gas);
    int refinery=build_raw(110);
    gas_worker(refinery);
    gas_worker(refinery);
    assert(select_building_fast(barracks));
    for(int i=0;i<4;++i) { assert(wait_bank(50,0)); while(b[barracks].queue_count>=4 && !won && !lost) input(0); int before=b[barracks].queue_count; tap(KEY_A); assert(b[barracks].queue_count>before); }
    fprintf(stderr,"queued four more defenders tick=%u\n",host_ticks);
    assert(wait_bank(200,100));
    int factory=build_raw(113); (void)factory;
    fprintf(stderr,"factory reached tick=%u bank=%d/%d\n",host_ticks,minerals,gas);
    int starport=build_raw(114);
    int facility=build_raw(116);
    int tower=addon_raw(starport,115); (void)tower;
    int lab=addon_raw(facility,118);
    fprintf(stderr,"infrastructure reached tick=%u bank=%d/%d\n",host_ticks,minerals,gas);
    assert(wait_bank((int)sc64_battlecruiser_research_cost(SC64_BC_TECH_YAMATO),
                     (int)sc64_battlecruiser_research_cost(SC64_BC_TECH_YAMATO)) && select_building_fast(lab));
    int research_row=race_bc_menu_base();
    assert(choose_menu_command(research_row));
    fprintf(stderr,"research requested tick=%u level=%d progress=%d bank=%d/%d\n",host_ticks,b[lab].level,b[lab].queue_progress,minerals,gas);
    assert(b[lab].level);
    /* The Battlecruiser takes 8 supply (16 doubled, StarCraft 64): build a
       Supply Depot first, then confirm the free supply covers it. */
    int depot=build_raw(109); (void)depot;
    fprintf(stderr,"supply for BC used=%d reserved=%d cap=%d need=%d\n",race_owner_supply(SIDE),race_owner_reserved_supply(SIDE),
            race_owner_supply_cap(SIDE),sc64_race_unit_rule(sc64_race_unit_from_raw(12))->supply);
    assert(race_owner_supply_cap(SIDE)-race_owner_reserved_supply(SIDE) >= sc64_race_unit_rule(sc64_race_unit_from_raw(12))->supply);
    assert(wait_bank(400,300) && select_building_fast(starport));
    for(int i=0;i<20 && sc64_race_raw_unit(train_type)!=12;++i) tap(KEY_R);
    fprintf(stderr,"train selection raw=%d tick=%u towerAlive=%d labAlive=%d bank=%d/%d\n",sc64_race_raw_unit(train_type),host_ticks,b[tower].alive,b[lab].alive,minerals,gas);
    assert(sc64_race_raw_unit(train_type)==12);
    int queued=b[starport].queue_count; tap(KEY_A); assert(b[starport].queue_count>queued);
    fprintf(stderr,"BC queued tick=%u bank=%d/%d\n",host_ticks,minerals,gas);
    /* The Zerg computer's attack outlasts eight Marines while the
       Battlecruiser builds and charges: spend the floating minerals on one
       more Depot and a steady stream of Marines. */
    (void)build_raw(109);
    int cruiser=-1;
    while(host_ticks<match_limit && !won && !lost) {
        for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE && sc64_race_raw_unit(u[i].type)==12) {cruiser=i;break;}
        int key=sc64_battlecruiser_owner_key(race_mode,race_selected_mission,SIDE,0);
        if(cruiser>=0 && key>=0 && sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,(unsigned)key,SC64_BC_TECH_YAMATO)) break;
        if(host_ticks%500==0) fprintf(stderr,"queue watch tick=%u progress=%d count=%d starport_alive=%d lab_alive=%d BC=%d research=%u\n",host_ticks,b[starport].queue_progress,b[starport].queue_count,b[starport].alive,b[lab].alive,cruiser,(unsigned)sc64_battlecruiser_tech_bits);
        if(host_ticks%64==0) defend_near(b[starport].x,b[starport].y);
        reinforce(barracks);
        input(0);
    }
    fprintf(stderr,"BC=%d research bits=%u tick=%u won=%d lost=%d bank=%d/%d\n",cruiser,(unsigned)sc64_battlecruiser_tech_bits,host_ticks,won,lost,minerals,gas);
    assert(cruiser>=0 && !won && !lost);
    int tech_key=sc64_battlecruiser_owner_key(race_mode,race_selected_mission,u[cruiser].owner,u[cruiser].raw_owner);
    assert(tech_key>=0 && sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,(unsigned)tech_key,SC64_BC_TECH_YAMATO));
    /* The Zerg computer attacks the base, Hydralisks included, while the
       Battlecruiser charges to 150 energy: park it in the far corner away
       from both bases, and keep the Marines at the Starport. */
    assert(select_unit(cruiser) && choose_menu_command(MENU_MOVE) && mode==MODE_MOVE);
    move_cursor(WORLD_W-48,48); tap(KEY_A);
    fprintf(stderr,"BC parks tick=%u dest=%d,%d\n",host_ticks,WORLD_W-48,48);
    while(u[cruiser].alive && u[cruiser].carry<150*256 && host_ticks<match_limit && !won && !lost) {
        if(host_ticks%2000==0) fprintf(stderr,"energy watch tick=%u energy=%d hp=%d xy=%d,%d base_alive=%d\n",host_ticks,u[cruiser].carry,u[cruiser].hp,u[cruiser].x,u[cruiser].y,b[0].alive);
        if(host_ticks%128==0) defend_near(b[starport].x,b[starport].y);
        reinforce(barracks);
        input(0);
    }
    fprintf(stderr,"energy done tick=%u energy=%d alive=%d won=%d lost=%d\n",host_ticks,u[cruiser].carry,u[cruiser].alive,won,lost);
    assert(u[cruiser].alive && u[cruiser].carry>=150*256 && !won && !lost);
    assert(select_unit(cruiser));
    int target=yamato_target(cruiser);
    if(target<0) {
        assert(choose_menu_command(MENU_MOVE) && mode==MODE_MOVE);
        move_cursor(race_base_x[1],race_base_y[1]); tap(KEY_A);
        fprintf(stderr,"BC move toward enemy tick=%u dest=%d,%d\n",host_ticks,race_base_x[1],race_base_y[1]);
        while(target<0 && u[cruiser].alive && !won && !lost && host_ticks<match_limit) {
            target=yamato_target(cruiser);
            if(target<0) input(0);
        }
    }
    fprintf(stderr,"target=%d tick=%u BC_alive=%d won=%d lost=%d target_xy=%d,%d bc_xy=%d,%d\n",target,host_ticks,u[cruiser].alive,won,lost,
            target>=0?target_x(target):-1,target>=0?target_y(target):-1,u[cruiser].x,u[cruiser].y);
    assert(target>=0 && u[cruiser].alive && !won && !lost);
    /* Aim at the nearest valid target. Aim again only when the target dies
       or leaves sight before the click, or while an accepted cast is still
       approaching (the engine then clears it), scouting toward the enemy
       base when no target is in sight. A rejected click, any other
       cancellation, a cast that clears after charging and a stalled
       approach fail the witness. Charge and flight must be seen on the same
       cast by this cruiser. */
    int charged=0,launched=0;
    for(int attempt=0;attempt<16 && !launched && target>=0 && u[cruiser].alive && !won && !lost;++attempt) {
        assert(select_unit(cruiser) && choose_menu_command(race_bc_yamato_menu_base()) && mode==MODE_YAMATO);
        /* The target may move while the cursor travels: follow it, and
           click only while the UI would aim Yamato at it. */
        if(!aim_cursor(target)) {
            fprintf(stderr,"yamato target left before the click attempt=%d target=%d alive=%d tick=%u\n",attempt,target,target_alive(target),host_ticks);
            assert(target_gone(target));
            tap(KEY_B);
        } else {
            tap(KEY_A);
            fprintf(stderr,"yamato aim attempt=%d target=%d xy=%d,%d tick=%u\n",attempt,target,target_x(target),target_y(target),host_ticks);
            charged=0;
            int seen=0,n=0;
            for(;n<5000 && !won && !lost;++n) {
                int pending=0;
                for(int j=0;j<SC64_BC_SHOT_LIMIT;++j) {
                    if(!sc64_bc_shots[j].state || sc64_bc_shots[j].caster!=(unsigned)cruiser) continue;
                    pending=1; assert(sc64_bc_shots[j].target==(unsigned)target);
                    if(sc64_bc_shots[j].state==SC64_BC_SHOT_CHARGING) charged=1;
                    if(sc64_bc_shots[j].state==SC64_BC_SHOT_FLIGHT) launched=1;
                }
                if(launched) break;
                if(!pending) {
                    fprintf(stderr,"yamato cleared attempt=%d seen=%d charged=%d target_alive=%d target_gone=%d tick=%u\n",
                            attempt,seen,charged,target_alive(target),target_gone(target),host_ticks);
                    /* Only an accepted cast, seen approaching, may clear. */
                    assert(seen && !charged && target_gone(target));
                    break;
                }
                seen=1;
                input(0);
            }
            assert(launched || won || lost || n<5000);
        }
        if(!launched && !won && !lost) {
            int scouting=0;
            while((target=yamato_target(cruiser))<0 && u[cruiser].alive && !won && !lost && host_ticks<match_limit) {
                if(!scouting && u[cruiser].order==ORDER_IDLE) {
                    assert(select_unit(cruiser) && choose_menu_command(MENU_MOVE) && mode==MODE_MOVE);
                    move_cursor(race_base_x[1],race_base_y[1]); tap(KEY_A); scouting=1;
                    fprintf(stderr,"BC scouts toward enemy tick=%u\n",host_ticks);
                }
                input(0);
            }
        }
    }
    fprintf(stderr,"yamato charged=%d launched=%d tick=%u won=%d lost=%d\n",charged,launched,host_ticks,won,lost);
    assert(charged && launched);
    int clicked_target=target, flight_target=-1;
    for(int j=0;j<SC64_BC_SHOT_LIMIT;++j)
        if(sc64_bc_shots[j].state==SC64_BC_SHOT_FLIGHT && sc64_bc_shots[j].caster==(unsigned)cruiser)
            { assert(flight_target<0); flight_target=sc64_bc_shots[j].target; }
    assert(flight_target>=0 && flight_target<MAX_UNITS+MAX_BUILDINGS);
    target=flight_target;
    fprintf(stderr,"clicked_target=%d actual_flight_target=%d\n",clicked_target,target);
    int pre_hp=target<MAX_UNITS?u[target].hp:b[target-MAX_UNITS].hp;
    int pre_alive=target_alive(target), energy_at_flight=u[cruiser].carry;
    fprintf(stderr,"flight target=%d hp=%d alive=%d energy=%d tick=%u\n",target,pre_hp,pre_alive,energy_at_flight,host_ticks);
    assert(pre_alive && energy_at_flight<150*256);
    static unsigned char saved[sizeof(race_impact_record)]; u32 saved_len=0;
    /* The tag race_impact_save_read() loads: shared-transport builds save
       under the transport version. */
#if defined(SC64_SHARED_TRANSPORT) && !defined(SC64_SCARAB_NAMESPACE_STAGE)
    const u32 save_tag=SC64_TRANSPORT_SAVE_VERSION;
#else
    const u32 save_tag=SAVE_VERSION;
#endif
    assert(!save_read_latest(saved,(u32)sizeof(saved),save_tag,&saved_len));
    int save_frame=frame; tap(KEY_START); assert(screen==SCREEN_PAUSE && frame==save_frame);
    input(KEY_B); input(0);
    assert((pause_notice & 0xc0)==UI_NOTICE_SAVED && (pause_notice & 0x3f)>0);
    assert(save_read_latest(saved,(u32)sizeof(saved),save_tag,&saved_len) && saved_len>0);
    int saved_flight=0;for(int j=0;j<SC64_BC_SHOT_LIMIT;++j) if(sc64_bc_shots[j].state==SC64_BC_SHOT_FLIGHT) saved_flight=1;
    assert(saved_flight && frame==save_frame);
    fprintf(stderr,"flight save tick=%u frame=%d bytes=%u\n",host_ticks,frame,(unsigned)saved_len);
    tap(KEY_START); assert(screen==SCREEN_PLAY);
    int active=1;
    for(int n=0;n<5000 && active && !won && !lost;++n) {
        input(0); active=0;for(int j=0;j<SC64_BC_SHOT_LIMIT;++j) if(sc64_bc_shots[j].state) active=1;
    }
    int post_alive=target_alive(target), post_hp=target<MAX_UNITS?u[target].hp:b[target-MAX_UNITS].hp;
    fprintf(stderr,"impact target=%d before=%d after=%d alive=%d active=%d tick=%u energy=%d won=%d lost=%d\n",target,pre_hp,post_hp,post_alive,active,host_ticks,u[cruiser].carry,won,lost);
    assert(!active && post_hp<pre_hp && !won && !lost);
    trace_record("{\"kind\":\"result\",\"result\":\"impact\",\"host_tick\":%u,\"frame\":%d,\"won\":%d,\"lost\":%d,\"saved_flight\":1}",
                 host_ticks,frame,won,lost);
    fputs("\n]}\n", trace);
    assert(fclose(trace) == 0);
    return 0;
}
