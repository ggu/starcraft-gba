/* Playable Backwater completion replay. All post-init world changes come
 * from controller inputs through game_update; arrays are read only for route
 * selection, diagnostics, and expected-state checks. */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

typedef struct { unsigned long tick; u16 held; } InputEvent;
typedef struct {
    unsigned long tick;
    int frame, minerals, gas, marines, firebats, scvs, zerglings, infested_cc;
    int raynor_owner, raynor_alive, won, lost, objective;
} Checkpoint;
static InputEvent events[4096];
static int event_count;
static Checkpoint checkpoints[128];
static int checkpoint_count;
static unsigned long input_tick;
static u16 previous_held, movement_base;
static int count_units(int owner, int type);
static int count_buildings(int owner, int type);

static int abs_i(int n) { return n < 0 ? -n : n; }
static int is_trace_arg(const char *s)
{ return s[0]=='-'&&s[1]=='-'&&s[2]=='t'&&s[3]=='r'&&s[4]=='a'&&s[5]=='c'&&s[6]=='e'&&s[7]=='\0'; }

static void input_frame(u16 held)
{
    u16 pressed = held & (u16)~previous_held;
    if (held != previous_held && event_count < (int)(sizeof(events)/sizeof(events[0]))) {
        events[event_count].tick = input_tick;
        events[event_count++].held = held;
    }
    game_update(held, pressed);
    previous_held = held;
    input_tick++;
    if(input_tick%1000==0&&checkpoint_count<(int)(sizeof(checkpoints)/sizeof(checkpoints[0]))) {
        Checkpoint *c=&checkpoints[checkpoint_count++];
        int i;
        c->tick=input_tick;c->frame=frame;c->minerals=minerals;c->gas=gas;
        c->marines=count_units(SIDE,U_MARINE);c->firebats=count_units(SIDE,U_FIREBAT);
        c->scvs=count_units(SIDE,U_SCV);c->zerglings=count_units(ENEMY,U_ZERGLING);
        c->infested_cc=count_buildings(ENEMY,B_INFESTED_COMMAND);
        c->raynor_owner=-1;c->raynor_alive=0;
        for(i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].type==U_RAYNOR){c->raynor_owner=u[i].owner;c->raynor_alive=1;break;}
        c->won=won;c->lost=lost;c->objective=sc64_backwater_objective(&backwater);
    }
}

static void press_key(u16 key)
{
    input_frame(movement_base | key);
    input_frame(movement_base);
}

static void move_cursor_near(int x, int y)
{
    int guard = 0;
    while ((abs_i(cx-x)>2 || abs_i(cy-y)>2) && guard++ < 1500) {
        int dx=x-cx, dy=y-cy;
        u16 h=movement_base;
        if (dx>2) h|=KEY_RIGHT; else if (dx< -2) h|=KEY_LEFT;
        if (dy>2) h|=KEY_DOWN; else if (dy< -2) h|=KEY_UP;
        input_frame(h);
    }
}

static void move_cursor_exact(int x, int y)
{
    int guard=0;
    movement_base &= (u16)~KEY_R;
    while ((cx!=x || cy!=y) && guard++<1500) {
        int dx=x-cx, dy=y-cy;
        u16 h=movement_base;
        if(dx>0)h|=KEY_RIGHT; else if(dx<0)h|=KEY_LEFT;
        if(dy>0)h|=KEY_DOWN; else if(dy<0)h|=KEY_UP;
        input_frame(h);
    }
}

static int count_units(int owner, int type)
{
    int i,n=0;
    for(i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==owner && u[i].type==type)n++;
    return n;
}

static int count_buildings(int owner, int type)
{
    int i,n=0;
    for(i=0;i<MAX_BUILDINGS;i++) if(b[i].alive && b[i].owner==owner && b[i].type==type)n++;
    return n;
}

static int nearest_worker(void)
{
    int i,best=-1,dmin=0x7fffffff;
    for(i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==SIDE && u[i].type==U_SCV) {
        int dx=u[i].x-cx,dy=u[i].y-cy,d=dx*dx+dy*dy;
        if(d<dmin){dmin=d;best=i;}
    }
    return best;
}

static int nearest_patch(int x,int y)
{
    int i,best=-1,dmin=0x7fffffff;
    for(i=0;i<MAX_MINERALS;i++) if(m[i].amount>0) {
        int dx=m[i].x-x,dy=m[i].y-y,d=dx*dx+dy*dy;
        if(d<dmin){dmin=d;best=i;}
    }
    return best;
}

static int attack_command(void)
{
    int guard=0;
    press_key(KEY_B);
    while(menu_cursor!=MENU_ATTACK && guard++<MENU_COUNT)
        press_key(menu_cursor<MENU_ATTACK?KEY_DOWN:KEY_UP);
    if(menu_cursor!=MENU_ATTACK)return 0;
    press_key(KEY_A);
    return mode==MODE_ATTACK;
}

static int attack_move(int x,int y)
{
    if(!attack_command())return 0;
    move_cursor_near(x,y);
    press_key(KEY_A);
    return mode==MODE_NORMAL;
}

static int test_select_combat_group(void)
{
    int i,n=0,left=WORLD_W,top=WORLD_H,right=0,bottom=0;
    /* L+D-pad drag selection works after a building was selected for
     * production and is still an ordinary controller action. */
    for(i=0;i<MAX_UNITS;i++) if(u[i].alive&&u[i].owner==SIDE&&
        (u[i].type==U_MARINE||u[i].type==U_FIREBAT||u[i].type==U_MEDIC||u[i].type==U_RAYNOR)) {
        if(u[i].x<left)left=u[i].x;
        if(u[i].x>right)right=u[i].x;
        if(u[i].y<top)top=u[i].y;
        if(u[i].y>bottom)bottom=u[i].y;
        n++;
    }
    if(!n)return 0;
    movement_base=0;
    move_cursor_near(left-2,top-2);
    input_frame(KEY_L);
    while(cx<right+2||cy<bottom+2) {
        u16 h=KEY_L;
        if(cx<right+2)h|=KEY_RIGHT;
        if(cy<bottom+2)h|=KEY_DOWN;
        input_frame(h);
    }
    input_frame(0);
    n=0;
    for(i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected)n++;
    movement_base=KEY_R;
    return n>0;
}

static int move_order(int x,int y)
{
    int guard=0;
    press_key(KEY_B);
    while(menu_cursor!=MENU_MOVE && guard++<MENU_COUNT)
        press_key(menu_cursor<MENU_MOVE?KEY_DOWN:KEY_UP);
    if(menu_cursor!=MENU_MOVE)return 0;
    press_key(KEY_A);
    move_cursor_near(x,y);
    press_key(KEY_A);
    return mode==MODE_NORMAL;
}

static int select_worker(void)
{
    int worker=nearest_worker();
    if(worker<0)return 0;
    move_cursor_near(u[worker].x,u[worker].y);
    press_key(KEY_A);
    return find_selected_worker()>=0;
}

static int begin_mining(void)
{
    int worker,patch;
    if(!select_worker())return 0;
    worker=find_selected_worker();
    patch=nearest_patch(u[worker].x,u[worker].y);
    if(patch<0)return 0;
    move_cursor_near(m[patch].x,m[patch].y);
    press_key(KEY_A);
    return 1;
}

static int wait_for_minerals(int target,int limit)
{
    int i;
    for(i=0;i<limit && minerals<target && screen==SCREEN_PLAY;i++)input_frame(movement_base);
    return minerals>=target;
}

static int rescue_at(int x,int y,int max_ticks)
{
    int i, before=count_units(SIDE,U_FIREBAT)+count_units(SIDE,U_SCV);
    movement_base=KEY_R;
    if(!test_select_combat_group())return 0;
    if(!move_order(x,y))return 0;
    for(i=0;i<max_ticks && screen==SCREEN_PLAY;i++) {
        input_frame(movement_base);
        if(count_units(SIDE,U_FIREBAT)+count_units(SIDE,U_SCV)>before)return 1;
    }
    return count_units(SIDE,U_FIREBAT)+count_units(SIDE,U_SCV)>before;
}

static int location_center(int id,int *x,int *y)
{
    int l,t,r,bottom;
    if(!bw_location_bounds(id,&l,&t,&r,&bottom))return 0;
    *x=(l+r)/2;*y=(t+bottom)/2;return 1;
}

static void save_trace(const char *path)
{
    FILE *f=fopen(path,"w");
    int i;
    if(!f)return;
    fprintf(f,"{\"total_ticks\":%lu,\"events\":[",input_tick);
    for(i=0;i<event_count;i++)fprintf(f,"%s{\"tick\":%lu,\"held\":%u}",i?",":"",events[i].tick,events[i].held);
    fprintf(f,"],\"checkpoints\":[");
    for(i=0;i<checkpoint_count;i++) {
        Checkpoint *c=&checkpoints[i];
        fprintf(f,"%s{\"tick\":%lu,\"frame\":%d,\"minerals\":%d,\"gas\":%d,\"marines\":%d,\"firebats\":%d,\"scvs\":%d,\"zerglings\":%d,\"infested_cc\":%d,\"raynor_owner\":%d,\"raynor_alive\":%d,\"won\":%d,\"lost\":%d,\"objective\":%d}",
            i?",":"",c->tick,c->frame,c->minerals,c->gas,c->marines,c->firebats,c->scvs,c->zerglings,c->infested_cc,c->raynor_owner,c->raynor_alive,c->won,c->lost,c->objective);
    }
    fprintf(f,"],\"expected\":{\"won\":%d,\"lost\":%d,\"frame\":%d,\"minerals\":%d,\"gas\":%d,\"marines\":%d,\"firebats\":%d,\"scvs\":%d,\"zerglings\":%d,\"infested_cc\":%d}}\n",
        won,lost,frame,minerals,gas,count_units(SIDE,U_MARINE),count_units(SIDE,U_FIREBAT),count_units(SIDE,U_SCV),count_units(ENEMY,U_ZERGLING),count_buildings(ENEMY,B_INFESTED_COMMAND));
    fclose(f);
}

int main(int argc,char **argv)
{
    const char *trace=(argc==3 && is_trace_arg(argv[1]))?argv[2]:NULL;
    int x,y,i,hero=-1,cc=-1;
    if(argc!=1 && !trace){fprintf(stderr,"usage: %s [--trace PATH]\n",argv[0]);return 64;}
    game_init();
#ifdef SC64_UNIFIED
    /* Enter T2 through the normal campaign and paged mission menus in the
     * unified full-Terran profile. */
    input_frame(KEY_A); input_frame(0);
#ifndef SC64_CAMPAIGN_RACE
    if (race_front.page != SC_FRONT_CAMPAIGN) return 66;
    input_frame(KEY_A); input_frame(0); /* Terran campaign -> mission list. */
#endif
    if (race_front.page != SC_FRONT_MISSION) return 66;
    input_frame(KEY_DOWN); input_frame(0); /* T2. */
    input_frame(KEY_A); input_frame(0); /* Briefing. */
    if (race_front.page != SC_FRONT_BRIEFING || race_front.mission != 5) return 66;
    input_frame(KEY_A); input_frame(0); /* Deploy. */
    if (screen != SCREEN_PLAY || race_selected_mission != 5) return 66;
#else
#ifdef SC64_TERRAN_CAMPAIGN
    /* Select T2 through the real title input path, then release before start. */
    input_frame(KEY_DOWN); input_frame(0);
#endif
    input_frame(KEY_A); input_frame(0);
#endif
    if(minerals!=200||gas!=200)return 1;
    for(i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].type==U_RAYNOR)hero=i;
    if(hero<0)return 2;

    /* Start real SCV mining, and rescue Raynor by walking the selected force
     * into his authored nearby location. */
    if(!begin_mining())return 3;
    if(u[hero].owner!=SIDE) {
        movement_base=KEY_R; if(!test_select_combat_group())return 4;
        if(!move_order(u[hero].x,u[hero].y))return 4;
        for(i=0;i<10000 && u[hero].owner!=SIDE && screen==SCREEN_PLAY;i++)input_frame(movement_base);
        if(u[hero].owner!=SIDE)return 5;
    }

    /* The northern bunker is the first NPC rescue route; its reinforced
     * squad becomes part of the ordinary selected combat force. */
    if(!location_center(SC64_BW_LOC_FIREBAT_BUNKER_2,&x,&y))return 6;
    if(!rescue_at(x,y,16000))return 7;
    if(count_units(SIDE,U_FIREBAT)<2)return 7;

    /* Keep economy alive with ordinary production from the authored Barracks. */
    if(!wait_for_minerals(50,30000))return 8;
    for(i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_BARRACKS){cc=i;break;}
    if(cc<0)return 9;
    move_cursor_exact(b[cc].x,b[cc].y); press_key(KEY_A);
    if(selected_building!=cc)return 10;
    for(i=0;i<8 && minerals>=50;i++)press_key(KEY_A);

    /* Continue toward the settlement and bunker, then the southern SCV bunker. */
    if(!location_center(SC64_BW_LOC_ABANDONED_SETTLEMENT,&x,&y))return 11;
    movement_base=KEY_R; if(!test_select_combat_group()){fprintf(stderr,"no group before settlement mar=%d fb=%d frame=%d minerals=%d\n",count_units(SIDE,U_MARINE),count_units(SIDE,U_FIREBAT),frame,minerals);return 12;}
    if(!move_order(x,y)){fprintf(stderr,"settlement order failed sel=%d building=%d mode=%d menu=%d cursor=%d,%d target=%d,%d\n",selected_count(),selected_building,mode,menu_open,cx,cy,x,y);return 12;}
    for(i=0;i<16000 && screen==SCREEN_PLAY;i++)input_frame(movement_base);
    if(!location_center(SC64_BW_LOC_SCV_BUNKER,&x,&y))return 13;
    if(!rescue_at(x,y,18000))return 14;
    if(count_units(SIDE,U_SCV)<3)return 14;

    if(!wait_for_minerals(300,50000))fprintf(stderr,"economy short: %d minerals at frame %d\n",minerals,frame);
    for(i=0;i<MAX_BUILDINGS;i++) if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_BARRACKS) {
        move_cursor_exact(b[i].x,b[i].y); press_key(KEY_A);
        if(selected_building==i) {
            while(minerals>=50 && supply_used+2<=supply_cap && b[i].queue_count<5)press_key(KEY_A);
        }
    }
    for(i=0;i<6000&&screen==SCREEN_PLAY;i++)input_frame(0);

    cc=-1;
    for(i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].type==B_INFESTED_COMMAND)cc=i;
    if(cc<0)return 16;
    movement_base=KEY_R;
    if(!test_select_combat_group())return 17;
    if(!attack_move(b[cc].x,b[cc].y))return 18;
    for(i=0;i<80000&&screen==SCREEN_PLAY;i++)input_frame(movement_base);
    if(trace)save_trace(trace);
    fprintf(stderr,"Backwater combat result: frame=%d ticks=%lu won=%d lost=%d minerals=%d gas=%d Marines=%d Firebats=%d SCVs=%d Zerglings=%d InfCC=%d Raynor=%d\n",
        frame,input_tick,won,lost,minerals,gas,count_units(SIDE,U_MARINE),count_units(SIDE,U_FIREBAT),count_units(SIDE,U_SCV),count_units(ENEMY,U_ZERGLING),count_buildings(ENEMY,B_INFESTED_COMMAND),u[hero].alive&&u[hero].owner==SIDE);
    return won&&!lost&&u[hero].alive&&u[hero].owner==SIDE&&
        count_buildings(ENEMY,B_INFESTED_COMMAND)==0&&
        count_units(SIDE,U_FIREBAT)>=2&&count_units(SIDE,U_SCV)>=3&&
        sc64_backwater_objective(&backwater)?0:19;
}
