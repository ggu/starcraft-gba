/* Genuine controller-input Z1 replay. Includes game.c only to observe the
 * resulting state; every post-title world change comes through game_update. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

typedef struct { unsigned tick; unsigned short held; } Z1Event;
typedef struct { unsigned tick; int minerals,gas,drones,hydras,zerglings,overlords,
    extractor,pool,den,enemy_buildings,enemy_units,won,lost,phase; } Z1Checkpoint;
/* An open menu or pause holds the world clock; bound host ticks so a
   stuck controller fails instead of hanging (a win takes about 60k). */
#define Z1_TICK_LIMIT 240000u
/* Host capture only; physical-key replay streams the tape externally. */
#define Z1_EVENT_CAPACITY 16384
static Z1Event events[Z1_EVENT_CAPACITY];
static Z1Checkpoint checkpoints[256];
static unsigned input_tick;
static unsigned short previous_held;
static int event_count,checkpoint_count;
static int event_overflow;
static int home_hatch=-1;
static const char *trace_path;

static int count_units(int owner,int type)
{ int n=0; for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==owner&&u[i].type==type)++n;return n; }
static int count_buildings(int owner,int type)
{ int n=0; for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==owner&&b[i].type==type)++n;return n; }
static int fail(const char *why);
static void controller_frame(unsigned short held)
{
    if(input_tick>=Z1_TICK_LIMIT){fail("host-tick limit reached; world clock stopped");exit(1);}
    unsigned short pressed=held&(unsigned short)~previous_held;
    if(held!=previous_held) {
        if(event_count<(int)(sizeof(events)/sizeof(events[0])))
            events[event_count++]=(Z1Event){input_tick,held};
        else event_overflow=1;
    }
    game_update(held,pressed); previous_held=held; ++input_tick;
    if(input_tick%1000==0&&checkpoint_count<(int)(sizeof(checkpoints)/sizeof(checkpoints[0]))) {
        int eu=0,eb=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY)++eu;
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY)++eb;
        checkpoints[checkpoint_count++]=(Z1Checkpoint){input_tick,minerals,gas,
            count_units(SIDE,U_DRONE),count_units(SIDE,U_HYDRALISK),count_units(SIDE,U_ZERGLING),
            count_units(SIDE,U_OVERLORD),count_buildings(SIDE,B_EXTRACTOR),
            count_buildings(SIDE,B_SPAWNING_POOL),count_buildings(SIDE,B_HYDRALISK_DEN),
            eb,eu,won,lost,race_campaign.phase};
    }
}
static void press(unsigned short key) { controller_frame(key); controller_frame(0); }
static int abs_i(int x){return x<0?-x:x;}
static int cursor_near(int x,int y)
{
    int guard=0;
    while((abs_i(cx-x)>2||abs_i(cy-y)>2)&&guard++<5000) {
        unsigned short h=KEY_R;
        if(abs_i(cx-x)>2)h|=cx<x?KEY_RIGHT:KEY_LEFT;
        if(abs_i(cy-y)>2)h|=cy<y?KEY_DOWN:KEY_UP;
        controller_frame(h);
    }
    controller_frame(0);
    return guard<5000;
}
static int click(int x,int y)
{ return cursor_near(x,y)&&(press(KEY_A),1); }
static int wait_until(int (*predicate)(void),int ticks)
{ for(int i=0;i<ticks&&screen==SCREEN_PLAY;i++){controller_frame(0);if(predicate())return 1;}return predicate(); }
static int has_second_overlord(void){return count_units(SIDE,U_OVERLORD)>=2;}
static int has_pool(void){for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_SPAWNING_POOL&&!b[i].under_construction)return 1;return 0;}
static int has_den(void){for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_HYDRALISK_DEN&&!b[i].under_construction)return 1;return 0;}
static int build_menu(int option,int x,int y)
{
    /* B reopens on the last chosen row, so walk to the wanted row. */
    int guard=0;
    press(KEY_B);
    while(menu_cursor!=option&&guard++<MENU_COUNT)press(menu_cursor<option?KEY_DOWN:KEY_UP);
    if(!menu_open||menu_cursor!=option)return 0;
    press(KEY_A);
    return mode==MODE_BUILD&&cursor_near(x,y)&&(press(KEY_A),1);
}
static int choose_worker(int require_far)
{
    int best=-1,bestd=-1,best_still=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE&&u[i].order!=ORDER_BUILD) {
        int d=0x7fffffff;
        for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE) {
            int q=dist2(u[i].x,u[i].y,b[j].x,b[j].y);if(q<d)d=q;
        }
        if((!require_far||d>36*36) &&
           (u[i].work_timer>best_still || (u[i].work_timer==best_still&&d>bestd)))
            { best=i;bestd=d;best_still=u[i].work_timer; }
    }
    return best;
}
static int select_worker(int require_far)
{
    int i=choose_worker(require_far),guard=0;
    if(i<0)return 0;
    /* L tap advances the current selection. Starting at a selected Hatchery,
       it clears that selection and selects the first visible unit; cycling
       uses only controller input and puts the cursor directly on each unit. */
    while(guard++<MAX_UNITS) {
        press(KEY_L);
        int selected=find_selected_worker();
        if(selected>=0) {
            int nearest=0x7fffffff;
            for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE) {
                int d=dist2(u[selected].x,u[selected].y,b[j].x,b[j].y);
                if(d<nearest)nearest=d;
            }
            if(!require_far||nearest>36*36)return 1;
        }
    }
    return 0;
}
static int select_hatchery(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_HATCHERY)
        return click(b[i].x,b[i].y)&&selected_building==i;
    return 0;
}
static int select_next_train(void){press(KEY_R);return 1;}
static void select_train_type(int type)
{ int guard=0;while(train_type!=type&&guard++<5)select_next_train(); }
enum { Z1_SITE_FOUNDED, Z1_SITE_NOT_ACCEPTED, Z1_SITE_CANCELLED, Z1_SITE_TIMED_OUT };
static const char *const z1_site_result[]={"founded","not accepted","cancelled","timed out"};
static int pending_before[MAX_UNITS];
static int pending_site(int i)
{ return u[i].alive&&u[i].owner==SIDE&&SC64_PENDING_BUILD_ORDER(u[i].order)?u[i].target:-1; }
static void note_pending_sites(void){for(int i=0;i<MAX_UNITS;i++)pending_before[i]=pending_site(i);}
/* The Drone pays on arrival, so spending first (a queued Drone) cancels the
   site. Only a new owned foundation of this type at the accepted site counts
   as success; call note_pending_sites() before placing. */
static int await_foundation(int type,int ticks)
{
    int w=-1,sx,sy;
    for(int i=0;i<MAX_UNITS&&w<0;i++)if(pending_site(i)==type&&pending_before[i]!=type)w=i;
    if(w<0)return Z1_SITE_NOT_ACCEPTED;
    sx=u[w].tx;sy=u[w].ty;
    for(int t=0;;t++) {
        for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&b[j].type==type&&
            b[j].x==sx&&b[j].y==sy)return Z1_SITE_FOUNDED;
        if(pending_site(w)!=type||u[w].tx!=sx||u[w].ty!=sy)return Z1_SITE_CANCELLED;
        if(t>=ticks||screen!=SCREEN_PLAY)return Z1_SITE_TIMED_OUT;
        controller_frame(0);
    }
}
static int start_build_action(int option,int x,int y,int far)
{
    if(!select_worker(far)){fprintf(stderr,"no selected worker (far=%d, cursor=%d,%d)\n",far,cx,cy);return 0;}
    note_pending_sites();
    if(!build_menu(option,x,y)){fprintf(stderr,"build menu failed opt=%d mode=%d menu=%d cursor=%d,%d worker=%d mineral=%d gas=%d\n",option,mode,menu,cx,cy,find_selected_worker(),minerals,gas);return 0;}
    int site=await_foundation(menu,1200);
    if(site!=Z1_SITE_FOUNDED){fprintf(stderr,"building %d site %s (tick=%u minerals=%d gas=%d)\n",menu,z1_site_result[site],input_tick,minerals,gas);return 0;}
    return 1;
}
static int nearest_geyser(int x,int y,int *gx,int *gy);
static int find_site(int type,int bx,int by,int *sx,int *sy);
static int ensure_extractor(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_EXTRACTOR)return 0;
    if(minerals<50||home_hatch<0||!b[home_hatch].alive)return 0;
    int gx=-1,gy=-1;
    if(!nearest_geyser(b[home_hatch].x,b[home_hatch].y,&gx,&gy))return 0;
    if(selected_building!=home_hatch)click(b[home_hatch].x,b[home_hatch].y);
    return start_build_action(MENU_REFINERY,gx,gy,0);
}
static int ensure_campaign_tech(void)
{
    if(ensure_extractor())return 1;
    if(!race_owner_has_type(SIDE,B_SPAWNING_POOL)) {
        int sx,sy;
        if(minerals>=200&&find_site(B_SPAWNING_POOL,b[home_hatch].x,b[home_hatch].y,&sx,&sy)) {
            if(selected_building!=home_hatch)click(b[home_hatch].x,b[home_hatch].y);
            return start_build_action(MENU_BARRACKS,sx,sy,0);
        }
        return 0;
    }
    if(!race_owner_has_type(SIDE,B_HYDRALISK_DEN)) {
        int sx,sy;
        if(minerals>=100&&gas>=50&&find_site(B_HYDRALISK_DEN,b[home_hatch].x,b[home_hatch].y,&sx,&sy)) {
            if(selected_building!=home_hatch)click(b[home_hatch].x,b[home_hatch].y);
            return start_build_action(MENU_ACADEMY,sx,sy,0);
        }
    }
    return 0;
}
static int nearest_geyser(int x,int y,int *gx,int *gy)
{
    int best=-1,bd=0x7fffffff;
    for(int i=0;i<geyser_count;i++){int d=dist2(x,y,geysers[i].x,geysers[i].y);
        if(d<bd){best=i;bd=d;}}
    if(best<0)return 0;*gx=geysers[best].x;*gy=geysers[best].y;return 1;
}
static int find_site(int type,int bx,int by,int *sx,int *sy)
{
    static const int delta[16][2]={{48,0},{-48,0},{0,48},{0,-48},{48,48},{-48,48},{48,-48},{-48,-48},
      {64,0},{-64,0},{0,64},{0,-64},{64,64},{-64,64},{64,-64},{-64,-64}};
    for(int i=0;i<16;i++){int x=bx+delta[i][0],y=by+delta[i][1];
        if(race_can_site(SIDE,type,x,y)&&can_place(type,x,y)){*sx=x;*sy=y;return 1;}}
    return 0;
}
static void assign_gas(void)
{
    int ref=-1,worker=-1,bd=0x7fffffff,assigned=0,total=0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_EXTRACTOR&&!b[i].under_construction)ref=i;
    if(ref<0)return;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE) {
        ++total;
        if(u[i].order==ORDER_GAS&&u[i].target==ref)++assigned;
    }
    /* Preserve at least two mineral trips; a tiny opening cannot support four
       gas workers after Zerg buildings consume their Drones. */
    if(assigned>=2||assigned>=total-2)return;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE&&u[i].order!=ORDER_BUILD&&u[i].order!=ORDER_GAS) {
        int d=dist2(u[i].x,u[i].y,b[ref].x,b[ref].y);
        if(d<bd){bd=d;worker=i;}
    }
    if(worker<0)return;
    int guard=0;
    while(guard++<MAX_UNITS&&find_selected_worker()!=worker)press(KEY_L);
    if(find_selected_worker()!=worker){fprintf(stderr,"gas select failed id%d selected%d\n",worker,find_selected_worker());return;}
    /* StarCraft's context action assigns the selected worker to this
       completed refinery regardless of its current position. */
    if(cursor_near(b[ref].x,b[ref].y))press(KEY_A);
}
/* New workers can be idle after the Pool consumes a miner. Issue the
 * ordinary contextual mineral command rather than assuming auto-assignment. */
static int assign_idle_miner(void)
{
    static const int offsets[][2]={{0,0},{-8,0},{8,0},{0,-8},{0,8},
        {-8,-8},{8,-8},{-8,8},{8,8},{-12,0},{12,0},{0,-12},{0,12}};
    int worker=-1,patch=-1,best=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DRONE&&u[i].order==ORDER_IDLE)
            {worker=i;break;}
    if(worker<0)return 0;
    for(int i=0;i<MAX_MINERALS;i++)if(m[i].amount>0&&currently_visible(m[i].x,m[i].y)) {
        int d=dist2(u[worker].x,u[worker].y,m[i].x,m[i].y);
        if(d<best){best=d;patch=i;}
    }
    if(patch<0)return 0;
    int guard=0;
    while(guard++<MAX_UNITS&&find_selected_worker()!=worker)press(KEY_L);
    if(find_selected_worker()!=worker)return 0;
    for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++) {
        if(!u[worker].alive||!u[worker].selected||selected_count()!=1)return 0;
        if(!cursor_near(m[patch].x+offsets[i][0],m[patch].y+offsets[i][1]))return 0;
        if(selected_worker_mineral_target()!=patch)continue;
        press(KEY_A);
        return u[worker].alive&&u[worker].order==ORDER_MINE&&u[worker].target==patch;
    }
    return 0;
}
static int queued_unit(int h,int type)
{
    if(h<0||h>=MAX_BUILDINGS)return 0;
    for(int k=0;k<b[h].queue_count;k++)
        if(b[h].queue[(b[h].queue_head+k)%5]==type)return 1;
    return 0;
}
static int queued_units(int h,int type)
{
    if(h<0||h>=MAX_BUILDINGS)return 0;
    int n=0;
    for(int k=0;k<b[h].queue_count;k++)
        if(b[h].queue[(b[h].queue_head+k)%5]==type)++n;
    return n;
}
static int order_local_defense(void);
static int enemy_count(void)
{ int n=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY)++n;return n; }
static int nearest_enemy_building(int *x,int *y)
{
    int best=-1,bd=0x7fffffff;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY) {
        int d=0x7fffffff;
        for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&!sc64_race_is_worker(u[j].type)) {
            if(sc64_race_unit_damage_vs(u[j].type,0)<=0&&sc64_race_unit_damage_vs(u[j].type,1)<=0)continue;
            int q=dist2(u[j].x,u[j].y,b[i].x,b[i].y);if(q<d)d=q;
        }
        if(d<bd){bd=d;best=i;}
    }
    if(best<0)return -1;*x=b[best].x;*y=b[best].y;return best;
}
static int issue_home_defense(int x,int y)
{
    int guard=0;
    press(KEY_L); /* controller cycles off the Hatchery to a unit */
    press(KEY_R); /* R selects all combat units */
    press(KEY_B);
    while(menu_cursor!=MENU_ATTACK&&guard++<MENU_COUNT)press(menu_cursor<MENU_ATTACK?KEY_DOWN:KEY_UP);
    if(menu_cursor!=MENU_ATTACK)return 0;
    press(KEY_A);
    return cursor_near(x,y)&& (press(KEY_A),1);
}
static int attack_nearest(int x,int y,int ticks)
{
    int guard=0,interrupted_for_defense=0;
    click(limit(cx+96,0,WORLD_W-1),limit(cy+96,0,WORLD_H-1));
    press(KEY_R); /* select combat group; R alone when no building is selected */
    press(KEY_B);
    while(menu_cursor!=MENU_ATTACK&&guard++<MENU_COUNT)press(menu_cursor<MENU_ATTACK?KEY_DOWN:KEY_UP);
    if(menu_cursor!=MENU_ATTACK)return 0;
    press(KEY_A);if(!cursor_near(x,y))return 0;press(KEY_A);
    for(int i=0;i<ticks&&screen==SCREEN_PLAY&&!won&&!lost;i++) {
        controller_frame(0);
        if(i%600==599&&order_local_defense()) {
            interrupted_for_defense=1;
            break;
        }
        /* Keep a second wave in production while the first group attacks.
           Every action here is still made through the player's selection and
           train hotkeys; queued units are included in the next attack order. */
        if(i%900==899) {
            ensure_campaign_tech();
            assign_gas();
            int h=-1,den=0;
            for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&b[j].type==B_HATCHERY)h=j;
            for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&b[j].type==B_HYDRALISK_DEN&&!b[j].under_construction)den=1;
            if(h>=0&&den&&b[h].larva>0&&b[h].queue_count<5) {
                if(selected_building!=h)click(b[h].x,b[h].y);
                if(selected_building==h) {
                    int drones=count_units(SIDE,U_DRONE);
                    int type=(drones<2&&!queued_unit(h,U_DRONE))?U_DRONE:
                        (race_owner_reserved_supply(SIDE)+2>race_owner_supply_cap(SIDE)?U_OVERLORD:U_HYDRALISK);
                    select_train_type(type);
                    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(train_type);
                    if(rule&&minerals>=rule->minerals&&gas>=rule->gas&&
                       !queued_unit(h,type))press(KEY_A);
                }
                click(limit(cx+96,0,WORLD_W-1),limit(cy+96,0,WORLD_H-1));
            }
            select_hatchery();
        }
    }
    /* Pick up any reinforcements that completed during the wait. */
    if(!interrupted_for_defense&&screen==SCREEN_PLAY&&!won&&!lost) {
        click(limit(cx+96,0,WORLD_W-1),limit(cy+96,0,WORLD_H-1));press(KEY_R);press(KEY_B);
        int guard=0;while(menu_cursor!=MENU_ATTACK&&guard++<MENU_COUNT)
            press(menu_cursor<MENU_ATTACK?KEY_DOWN:KEY_UP);
        if(menu_cursor==MENU_ATTACK){press(KEY_A);if(cursor_near(x,y))press(KEY_A);}
    }
    return interrupted_for_defense?2:1;
}
static int attack_with_hydras(int x,int y)
{
    int first=-1,guard=0;
    /* Cycle selection off the Hatchery and onto a Hydra; using a single
       selected unit avoids sending the defensive Zerglings out of position. */
    while(guard++<MAX_UNITS) {
        press(KEY_L);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
            u[i].type==U_HYDRALISK&&u[i].order==ORDER_IDLE&&u[i].selected){first=i;break;}
        if(first>=0)break;
    }
    if(first<0)return 0;
    press(KEY_B);
    while(menu_cursor!=MENU_ATTACK&&guard++<MENU_COUNT)press(menu_cursor<MENU_ATTACK?KEY_DOWN:KEY_UP);
    if(menu_cursor!=MENU_ATTACK)return 0;
    press(KEY_A);
    if(!cursor_near(x,y))return 0;
    press(KEY_A);
    return 1;
}
static int defend_hatch_with_army(int x,int y)
{
    int guard=0;
    press(KEY_L); /* move off the Hatchery; R then selects only combat units */
    press(KEY_R);
    press(KEY_B);
    while(menu_cursor!=MENU_ATTACK&&guard++<MENU_COUNT)press(menu_cursor<MENU_ATTACK?KEY_DOWN:KEY_UP);
    if(menu_cursor!=MENU_ATTACK)return 0;
    press(KEY_A);
    return cursor_near(x,y)&&(press(KEY_A),1);
}
static void refresh_local_defense(void);
/* After a costly engagement, fall back to the north-front rally and rebuild
   from banked resources so the next target meets one full group instead of
   a dwindling stream. Every action is a selection, train or attack input. */
static void regroup_hydras(int below,int target)
{
    int h=-1;
    if(count_units(SIDE,U_HYDRALISK)>=below)return;
    for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&b[j].type==B_HATCHERY)h=j;
    if(h<0||!race_owner_has_type(SIDE,B_HYDRALISK_DEN))return;
    if(count_units(SIDE,U_HYDRALISK)+count_units(SIDE,U_ZERGLING)>0)defend_hatch_with_army(200,790);
    for(int t=0;t<9000&&screen==SCREEN_PLAY&&!won&&!lost;t++) {
        /* Regrouping can span the loss of the Extractor; restore it before
         * queued combat units consume the remaining mineral reserve. */
        if(!count_buildings(SIDE,B_EXTRACTOR)&&minerals>=50) {
            (void)ensure_extractor();
            if(screen!=SCREEN_PLAY||won||lost||!b[h].alive)break;
        }
        int live=count_units(SIDE,U_HYDRALISK)+queued_units(h,U_HYDRALISK);
        if(live>=target||(gas<25&&!queued_units(h,U_HYDRALISK)))break;
        if(selected_building!=h)click(b[h].x,b[h].y);
        if(selected_building==h&&b[h].larva>0&&b[h].queue_count<5) {
            int type=race_owner_reserved_supply(SIDE)+2>race_owner_supply_cap(SIDE)?U_OVERLORD:U_HYDRALISK;
            select_train_type(type);
            const Sc64RaceUnitRule *rule=sc64_race_unit_rule(train_type);
            if(rule&&minerals>=rule->minerals&&gas>=rule->gas&&
               (type==U_HYDRALISK||!queued_unit(h,type)))press(KEY_A);
        }
        controller_frame(0);
        if(t%300==0)refresh_local_defense();
        if(t%600==0)assign_gas();
        if(!b[h].alive)break;
    }
    for(int t=0;t<900&&screen==SCREEN_PLAY&&!won&&!lost&&b[h].queue_count;t++)controller_frame(0);
    select_hatchery();
}
static int order_local_defense(void)
{
    int threat=-1,td=200*200;
    int combat=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        !sc64_race_is_worker(u[i].type)&&
        (sc64_race_unit_damage_vs(u[i].type,0)>0||sc64_race_unit_damage_vs(u[i].type,1)>0))++combat;
    if(!combat)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
        !sc64_race_unit_flying(u[i].type)) {
        int nearest=0x7fffffff;
        for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE) {
            int d=dist2(u[i].x,u[i].y,b[j].x,b[j].y);
            if(d<nearest)nearest=d;
        }
        if(nearest<td){td=nearest;threat=i;}
    }
    if(threat<0)return 0;
    int tx=u[threat].x,ty=u[threat].y;
    int ok=defend_hatch_with_army(tx,ty);
    return ok;
}
/* Controller replay refreshes its local attack-move while an enemy remains
   close.  The order is issued only through the same attack menu inputs as a
   player; this prevents the opening Lings from standing idle while Goliaths
   fire on the Hatchery from outside their automatic-acquisition radius. */
static unsigned next_defense_refresh;
static void refresh_local_defense(void)
{
    if(input_tick<next_defense_refresh)return;
    if(order_local_defense()) {
        next_defense_refresh=input_tick+600;
        select_hatchery();
    } else next_defense_refresh=input_tick+120;
}
static void write_trace(void)
{
    if(!trace_path)return;FILE*f=fopen(trace_path,"w");if(!f)return;
    fprintf(f,"{\"mission\":\"Z1\",\"total_ticks\":%u,\"events\":[",input_tick);
    for(int i=0;i<event_count;i++)fprintf(f,"%s{\"tick\":%u,\"held\":%u}",i?",":"",events[i].tick,events[i].held);
    fprintf(f,"],\"checkpoints\":[");
    for(int i=0;i<checkpoint_count;i++){Z1Checkpoint*c=&checkpoints[i];
        fprintf(f,"%s{\"tick\":%u,\"minerals\":%d,\"gas\":%d,\"drones\":%d,\"hydras\":%d,\"zerglings\":%d,\"overlords\":%d,\"extractors\":%d,\"pools\":%d,\"dens\":%d,\"enemy_buildings\":%d,\"enemy_units\":%d,\"won\":%d,\"lost\":%d,\"phase\":%d}",i?",":"",c->tick,c->minerals,c->gas,c->drones,c->hydras,c->zerglings,c->overlords,c->extractor,c->pool,c->den,c->enemy_buildings,c->enemy_units,c->won,c->lost,c->phase);}
    fprintf(f,"],\"expected\":{\"won\":%d,\"lost\":%d,\"phase\":%d,\"minerals\":%d,\"gas\":%d,\"hydras\":%d,\"enemy_buildings\":%d}}\n",won,lost,race_campaign.phase,minerals,gas,count_units(SIDE,U_HYDRALISK),enemy_count());
    fclose(f);
}
static int fail(const char *why){fprintf(stderr,"Z1 controller replay: %s (tick=%u, M=%d G=%d, D=%d H=%d, buildings=%d, won=%d lost=%d phase=%d)\n",why,input_tick,minerals,gas,count_units(SIDE,U_DRONE),count_units(SIDE,U_HYDRALISK),enemy_count(),won,lost,race_campaign.phase);write_trace();return 1;}

static int start_z1(void)
{
    game_init();
    /* HOME → CAMPAIGN → Zerg → Z1 → briefing → play. */
    press(KEY_A);
    press(KEY_A);
    press(KEY_A);
    press(KEY_START);
    if(screen!=SCREEN_PLAY||race_campaign.mission!=SC64_RACE_MISSION_Z1||race_human!=R_ZERG)
        return fail("title did not start default Z1");
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_HATCHERY)home_hatch=i;
    if(home_hatch<0)return fail("Z1 player has no Hatchery");
    return 0;
}
/* --check-site cancel|timeout: the Pool wait must not report success when
   the site is cancelled (a Drone bought first) or still pending at its
   limit. Both cases use the same ordinary inputs as the controller. */
static int check_site(const char *kind)
{
    int site_x,site_y,result;
    if(start_z1())return 1;
    for(int wait=0;wait<8000&&screen==SCREEN_PLAY&&minerals<200;wait++)controller_frame(0);
    if(!select_hatchery()||!find_site(B_SPAWNING_POOL,b[home_hatch].x,b[home_hatch].y,&site_x,&site_y)||
       !select_worker(0))return fail("site check setup");
    note_pending_sites();
    if(!build_menu(MENU_BARRACKS,site_x,site_y))return fail("site check placed Spawning Pool");
    if(!strcmp(kind,"cancel")) {
        if(!select_hatchery())return fail("site check selected Hatchery");
        select_train_type(U_DRONE);
        for(int i=0;i<5&&minerals>=200;i++)press(KEY_A);
        if(minerals>=200)return fail("site check could not spend below the Pool cost");
        result=await_foundation(B_SPAWNING_POOL,1200);
        if(result!=Z1_SITE_CANCELLED||count_buildings(SIDE,B_SPAWNING_POOL))
            return fprintf(stderr,"Z1 site check cancel: got %s\n",z1_site_result[result]),1;
    } else if(!strcmp(kind,"timeout")) {
        result=await_foundation(B_SPAWNING_POOL,1);
        if(result!=Z1_SITE_TIMED_OUT||count_buildings(SIDE,B_SPAWNING_POOL))
            return fprintf(stderr,"Z1 site check timeout: got %s\n",z1_site_result[result]),1;
        /* The same site still founds once the Drone arrives. */
        result=await_foundation(B_SPAWNING_POOL,1200);
        if(result!=Z1_SITE_FOUNDED)
            return fprintf(stderr,"Z1 site check timeout follow-up: got %s\n",z1_site_result[result]),1;
    } else return 64;
    printf("Z1 site check %s: ok at tick %u\n",kind,input_tick);
    return 0;
}

int main(int argc,char **argv)
{
    int x,y,hatch,extractor=-1,site_x,site_y;
    if(argc==3&&strcmp(argv[1],"--trace")==0)trace_path=argv[2];
    else if(argc==3&&strcmp(argv[1],"--check-site")==0)return check_site(argv[2]);
    else if(argc!=1)return 64;
    if(start_z1())return 1;
    hatch=home_hatch;
    /* Grow the mining line first; keep the initial Zerglings near their
       original staging area while the economy/tech comes online. */
    if(!select_hatchery())return fail("select Hatchery");
    if(!has_pool()) {
        for(int wait=0;wait<8000&&screen==SCREEN_PLAY&&minerals<200;wait++)controller_frame(0);
        if(minerals<200)return fail("mineral trips did not fund Spawning Pool");
        if(!find_site(B_SPAWNING_POOL,b[hatch].x,b[hatch].y,&site_x,&site_y)||
           !start_build_action(MENU_BARRACKS,site_x,site_y,0))return fail("controller placed Spawning Pool");
        /* Keep a third Drone in production while the Pool is under
           construction, then start the Extractor as soon as minerals allow. */
        if(!select_hatchery())return fail("reselect Hatchery during tech rush");
        int queued_supply_overlord=0;
        for(int t=0;t<4800&&screen==SCREEN_PLAY&&count_units(SIDE,U_DRONE)<4;t++) {
            if(t%120==0 && assign_idle_miner()) {
                if(!select_hatchery())return fail("reselect Hatchery after assigning miner");
            }
            if(!queued_supply_overlord&&race_owner_reserved_supply(SIDE)+2>race_owner_supply_cap(SIDE)&&
               minerals>=100&&b[hatch].larva>0&&b[hatch].queue_count<5) {
                select_train_type(U_OVERLORD);
                int old_queue=b[hatch].queue_count;
                press(KEY_A);
                if(b[hatch].queue_count>old_queue)queued_supply_overlord=1;
                select_train_type(U_DRONE);
            } else {
                select_train_type(U_DRONE);
                if(minerals>=50&&b[hatch].larva>0&&b[hatch].queue_count<5)press(KEY_A);
            }
            controller_frame(0);
        }
    }
    if(!count_buildings(SIDE,B_EXTRACTOR)) {
        /* A real Drone builds at the nearby geyser; then it makes real gas trips. */
        int gx=-1,gy=-1;
        if(!nearest_geyser(b[hatch].x,b[hatch].y,&gx,&gy))return fail("Z1 has a geyser");
        for(int i=0;i<6000&&minerals<50&&screen==SCREEN_PLAY;i++)controller_frame(0);
        if(gx<0||!start_build_action(MENU_REFINERY,gx,gy,0))return fail("controller placed Extractor");
        for(int i=0;i<3000&&screen==SCREEN_PLAY;i++){
            controller_frame(0);
            for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&b[j].type==B_EXTRACTOR&&
                !b[j].under_construction)extractor=j;
            if(extractor>=0)break;
        }
        if(extractor<0)return fail("Extractor construction did not finish");
        assign_gas();
    }
    if(!wait_until(has_pool,5000))return fail("Spawning Pool construction did not finish");
    next_defense_refresh=input_tick;
    if(!has_den()) {
        for(int i=0;i<12000&&screen==SCREEN_PLAY&&(minerals<100||gas<50);i++){
            controller_frame(0);
            refresh_local_defense();
            if(i%180==0)assign_gas();
        }
        if(minerals<100||gas<50)return fail("real mineral and gas trips did not fund Hydralisk Den");
        if(!find_site(B_HYDRALISK_DEN,b[hatch].x,b[hatch].y,&site_x,&site_y) ||
           !start_build_action(MENU_ACADEMY,site_x,site_y,0))return fail("controller placed Hydralisk Den");
        if(!wait_until(has_den,5000))return fail("Hydralisk Den construction did not finish");
    }
    if(!(race_campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))
        for(int i=0;i<60&&screen==SCREEN_PLAY;i++)controller_frame(0);
    if(!(race_campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return fail("Z1 tech objective did not activate");
    if(!select_hatchery())return fail("reselect Hatchery for Hydras");
    /* The scripted enemy has already killed one starting Overlord by this
       point. Restore supply through the real larva queue before Hydras. */
    while(race_owner_reserved_supply(SIDE)+2>race_owner_supply_cap(SIDE)&&screen==SCREEN_PLAY) {
        int old_cap=race_owner_supply_cap(SIDE);
        refresh_local_defense();
        if(selected_building!=hatch)select_hatchery();
        select_train_type(U_OVERLORD);
        if(minerals<100||b[hatch].larva<1)break;
        press(KEY_A);
        if(b[hatch].queue_count==0)break;
        for(int i=0;i<1200&&screen==SCREEN_PLAY&&race_owner_supply_cap(SIDE)==old_cap;i++)controller_frame(0);
    }
    select_train_type(U_HYDRALISK);
    int max_hydras=count_units(SIDE,U_HYDRALISK);
    int focused_hydras=0;
    /* Keep the Hatchery queue fed as larvae regenerate instead of waiting for
       each individual Hydra before issuing the next controller A press. */
    for(int t=0;t<30000&&count_units(SIDE,U_HYDRALISK)+queued_units(hatch,U_HYDRALISK)<30&&screen==SCREEN_PLAY;t++) {
        int desired=count_units(SIDE,U_DRONE)<8&&!queued_unit(hatch,U_DRONE)?U_DRONE:U_HYDRALISK;
        const Sc64RaceUnitRule *wanted=sc64_race_unit_rule(desired);
        if(wanted&&race_owner_reserved_supply(SIDE)+wanted->supply>race_owner_supply_cap(SIDE))desired=U_OVERLORD;
        select_train_type(desired);
        const Sc64RaceUnitRule *desired_rule=sc64_race_unit_rule(desired);
        if(desired_rule&&minerals>=desired_rule->minerals&&gas>=desired_rule->gas&&
           b[hatch].larva>0&&b[hatch].queue_count<5&&
           !queued_unit(hatch,desired))press(KEY_A);
        controller_frame(0);
        if(t%300==0)refresh_local_defense();
        if(t%600==0) { ensure_campaign_tech();assign_gas();assign_idle_miner();select_hatchery(); }
        int live=count_units(SIDE,U_HYDRALISK);
        if(live>focused_hydras&&focused_hydras<3) {
            int wi=-1,wd=0x7fffffff;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].type==U_WRAITH) {
                int d=dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y);
                if(d<=200*200&&d<wd){wd=d;wi=i;}
            }
            /* Once the local Wraith is gone, keep later Hydras near the base
               and engage the closest ground attacker instead of chasing a
               distant air unit across the map. */
            if(wi<0)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
                !sc64_race_unit_flying(u[i].type)) {
                int d=dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y);
                if(d<=200*200&&d<wd){wd=d;wi=i;}
            }
            if(wi>=0&&attack_with_hydras(u[wi].x,u[wi].y)) {
                ++focused_hydras;
                if(!select_hatchery())break;
            }
        }
        if(live>max_hydras)max_hydras=live;
        if(!race_owner_has_type(SIDE,B_HYDRALISK_DEN))break;
    }
    if(max_hydras<1)return fail("no Hydralisk was produced from the user queue");
    if(count_units(SIDE,U_HYDRALISK)+count_units(SIDE,U_ZERGLING)>0)
        defend_hatch_with_army(200,790); /* hold the north-front base approaches */
    select_hatchery(); /* If the hall fell, the surviving army can still attack. */
    /* Click empty ground to clear building selection, then R selects combat.
       Each target is chosen from observed live hostile buildings; the attack
       command itself is entirely controller input. */
    click(limit(cx+96,0,WORLD_W-1),limit(cy+96,0,WORLD_H-1));
    for(int target=0;target<256&&screen==SCREEN_PLAY&&!won&&!lost;target++) {
        /* Defense interruptions can end an attack wait before its periodic
         * economy check. Rebuild a lost Extractor through ordinary inputs. */
        if(!count_buildings(SIDE,B_EXTRACTOR)&&minerals>=50) {
            (void)ensure_extractor();
            if(screen!=SCREEN_PLAY||won||lost)break;
        }
        if(order_local_defense()) {
            for(int hold=0;hold<2400&&screen==SCREEN_PLAY&&!won&&!lost;hold++) {
                controller_frame(0);
                if(hold%600==599)order_local_defense();
            }
            continue;
        }
        regroup_hydras(20,26);
        if(nearest_enemy_building(&x,&y)<0)break;
        int result=attack_nearest(x,y,900);
        if(!result)return fail("issue attack-move command");
    }
    for(int i=0;i<120&&screen==SCREEN_PLAY&&!won&&!lost;i++)controller_frame(0);
    write_trace();
    if(event_overflow)return fail("controller event trace exceeded host capture capacity");
    if(!won||lost||!race_campaign_won())return fail("controller attack did not complete Z1");
    if(count_units(SIDE,U_HYDRALISK)<1||count_buildings(SIDE,B_EXTRACTOR)<1||
       count_buildings(SIDE,B_SPAWNING_POOL)<1||count_buildings(SIDE,B_HYDRALISK_DEN)<1)
        return fail("victory omitted required economy, tech, or Hydralisk production");
    printf("Z1 controller replay completed at tick %u: Hydralisks=%d minerals=%d gas=%d\n",
           input_tick,count_units(SIDE,U_HYDRALISK),minerals,gas);
    return 0;
}
