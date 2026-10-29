/* Ordinary-input P8 "The Trial of Tassadar" campaign controller.
 * The controller may inspect live state to choose targets; every mutation is
 * an ordinary HOME/menu/gameplay input delivered through game_update(). */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef P8_CONTROLLER_PROFILE
#define P8_CONTROLLER_PROFILE "PROTOSS_P8_FLAGS"
#endif
#ifndef P8_RUN_LIMIT
#define P8_RUN_LIMIT 120000u
#endif
/* One ordinary pause-menu save; the replay tools resume from this record. */
#ifndef P8_SAVE_FRAME
#define P8_SAVE_FRAME 30000
#endif
#define KEY_TAP(k) do { input((k)); input(0); } while (0)

static FILE *trace;
static u16 prior;
static unsigned host_tick;

static void input(u16 held)
{
    u16 pressed=(u16)(held & (u16)~prior);
    int changed=held!=prior;
    unsigned call_index=host_tick;
    unsigned frame_before=(unsigned)frame;
    if (screen!=SCREEN_RESULT && !won && !lost) game_update(held,pressed);
    prior=held;
    ++host_tick;
    if(trace && changed)
        fprintf(trace,"{\"kind\":\"input\",\"call_index\":%u,\"frame_before\":%u,\"frame_after\":%u,\"held\":%u,\"pressed\":%u}\n",
            call_index,frame_before,(unsigned)frame,(unsigned)held,(unsigned)pressed);
}

/* Read-only route query over the engine's current walkability plane. */
static int waypoint_to(int sx,int sy,int tx,int ty,int *wx,int *wy,int *distance_out)
{
    enum { WAYPOINT_STEPS = 10 };
    static const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    static int prev[MAP_CELLS],queue[MAP_CELLS],path[MAP_CELLS];
    if(!path_grid_valid)return 0;
    sx=limit(sx/16,0,MAP_COLS-1);sy=limit(sy/16,0,MAP_ROWS-1);
    int start=sx+sy*MAP_COLS,gx=limit(tx/16,0,MAP_COLS-1),gy=limit(ty/16,0,MAP_ROWS-1);
    int head=0,tail=0,best=-1;
    for(int i=0;i<MAP_COLS*MAP_ROWS;i++)prev[i]=-2;
    prev[start]=-1;queue[tail++]=start;
    while(head<tail){
        int cell=queue[head++],col=cell%MAP_COLS,row=cell/MAP_COLS;
        if(FAST_ABS(col-gx)<=4&&FAST_ABS(row-gy)<=4){best=cell;break;}
        for(int k=0;k<4;k++){
            int nc=col+dx[k],nr=row+dy[k];
            if(nc<0||nr<0||nc>=MAP_COLS||nr>=MAP_ROWS)continue;
            int next=nc+nr*MAP_COLS;
            if(prev[next]!=-2||(!SC64_GRID_GET(path_walkable,next)&&next!=start))continue;
            prev[next]=cell;queue[tail++]=next;
        }
    }
    if(best<0)return 0;
    int len=0;
    for(int c=best;c>=0&&len<MAP_CELLS;c=prev[c])path[len++]=c;
    int distance=len-1;
    if(distance_out)*distance_out=distance;
    int pick=distance>WAYPOINT_STEPS?distance-WAYPOINT_STEPS:0;
    *wx=(path[pick]%MAP_COLS)*16+8;*wy=(path[pick]/MAP_COLS)*16+8;
    return 1;
}

static int hero(int raw)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw)return i;
    return -1;
}

static int stasis_cell(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==168)return i;
    return -1;
}

static int home_nexus(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_NEXUS)return i;
    return -1;
}

static void move_cursor(int x,int y)
{
    unsigned guard=0;
    x=limit(x,0,WORLD_W-1);y=limit(y,0,WORLD_H-1);
    x=cx+((x-cx)>=0?((x-cx+2)/4):((x-cx-2)/4))*4;
    y=cy+((y-cy)>=0?((y-cy+2)/4):((y-cy-2)/4))*4;
    while((cx!=x||cy!=y)&&screen==SCREEN_PLAY&&!won&&!lost&&guard++<12000u){
        u16 held=KEY_R;
        if(cx<x)held|=KEY_RIGHT;else if(cx>x)held|=KEY_LEFT;
        if(cy<y)held|=KEY_DOWN;else if(cy>y)held|=KEY_UP;
        input(held);
    }
    input(0);
    if(cx!=x||cy!=y)
        fprintf(stderr,"cursor guard frame=%d wanted=%d,%d got=%d,%d\n",frame,x,y,cx,cy);
}

static void clear_selected_building(void)
{
    if(selected_building<0)return;
    move_cursor(900,700);input(KEY_L);
    unsigned guard=0;
    while((cx<916||cy<716)&&guard++<64){
        u16 held=KEY_L;
        if(cx<916)held|=KEY_RIGHT;
        if(cy<716)held|=KEY_DOWN;
        input(held);
    }
    input(0);
}

static void close_menus(void)
{
    if(menu_open)KEY_TAP(KEY_B);
    if(mode!=MODE_NORMAL)KEY_TAP(KEY_B);
}

static int command_selected(int which,int x,int y)
{
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=which;n++)
        KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=which){
        fprintf(stderr,"command row frame=%d want=%d cursor=%d rows=%d sel=%d\n",
                frame,which,menu_cursor,field_menu_count(),selected_count());
        close_menus();return 0;
    }
    KEY_TAP(KEY_A);
    if(mode!=(which==MENU_ATTACK?MODE_ATTACK:MODE_MOVE)){close_menus();return 0;}
    move_cursor(x,y);KEY_TAP(KEY_A);
    return 1;
}

/* R is the ordinary whole-army selection shortcut. */
static int army_command(int which,int x,int y)
{
    close_menus();
    clear_selected_building();
    KEY_TAP(KEY_R);
    if(selected_count()<=0)return 0;
    return command_selected(which,x,y);
}

/* Tap one hero to select it alone, then move it. */
static int hero_command(int slot,int x,int y)
{
    if(slot<0||!u[slot].alive||u[slot].owner!=SIDE)return 0;
    close_menus();
    clear_selected_building();
    move_cursor(u[slot].x,u[slot].y);KEY_TAP(KEY_A);
    if(!u[slot].selected||selected_count()!=1)return 0;
    return command_selected(MENU_MOVE,x,y);
}

static int live_units_owner(int owner)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==owner;return n;
}

static int probe_count(void)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE;return n;
}

static int army_count(void)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&!IS_WORKER(u[i].type)&&
        u[i].raw_type!=28&&u[i].raw_type!=78&&u[i].raw_type!=72;
    return n;
}

static int count_type(int type,int complete_only)
{
    int n=0;for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&
        (!complete_only||building_complete(i));
    return n;
}

static int queue_probe(void)
{
    int home=home_nexus();
    if(home<0||!building_complete(home)||probe_count()+b[home].queue_count>=14||
       b[home].queue_count>=2||minerals<50||race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<2)return 0;
    close_menus();
    if(selected_building!=home){clear_selected_building();move_cursor(b[home].x,b[home].y);KEY_TAP(KEY_A);}
    if(selected_building!=home)return 0;
    int before=b[home].queue_count;
    KEY_TAP(KEY_A);
    if(b[home].queue_count<=before)return 0;
    fprintf(trace,"{\"kind\":\"production\",\"frame\":%u,\"unit\":\"probe\",\"bank_after\":%d}\n",(unsigned)frame,minerals);
    return 1;
}

static int queue_combat_unit(void)
{
    int wanted=count_type(B_CYBERNETICS_CORE,1)&&gas>=50&&minerals>=125?U_DRAGOON:U_ZEALOT;
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(wanted);
    if(!rule||minerals<(int)rule->minerals||gas<(int)rule->gas||
       race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<(int)rule->supply)return 0;
    int producer=-1,best_queue=2;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_GATEWAY&&building_complete(i)&&b[i].queue_count<best_queue){
        producer=i;best_queue=b[i].queue_count;
    }
    if(producer<0)return 0;
    close_menus();
    move_cursor(b[producer].x,b[producer].y);KEY_TAP(KEY_A);
    if(selected_building!=producer)return 0;
    for(int n=0;n<4&&train_type!=wanted;n++)KEY_TAP(KEY_R);
    if(train_type!=wanted)return 0;
    int before=b[producer].queue_count;
    KEY_TAP(KEY_A);
    if(b[producer].queue_count<=before)return 0;
    fprintf(trace,"{\"kind\":\"production\",\"frame\":%u,\"unit\":\"%s\",\"bank_after\":[%d,%d]}\n",
        (unsigned)frame,wanted==U_DRAGOON?"dragoon":"zealot",minerals,gas);
    return 1;
}

static int free_builder(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
        u[i].order==ORDER_MINE&&!u[i].carry)return i;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE&&
        !SC64_BUILD_WORKER_ORDER(u[i].order)&&u[i].order!=ORDER_GAS)return i;
    return -1;
}

/* Place `type` through the Probe build menu at the nearest legal site. */
static int build_structure(int type,int menu_row,int min_r,int max_r,int fixed_x,int fixed_y)
{
    int home=home_nexus(),best=0x7fffffff,site_x=fixed_x,site_y=fixed_y;
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
    if(home<0||!rule||minerals<(int)rule->minerals||gas<(int)rule->gas)return 0;
    if(site_x<0){
        for(int dy=-max_r;dy<=max_r;dy+=16)for(int dx=-max_r;dx<=max_r;dx+=16){
            int x=b[home].x+dx,y=b[home].y+dy,d=dx*dx+dy*dy;
            if(d<min_r*min_r||x<32||x>=WORLD_W-32||y<32||y>=WORLD_H-32||d>=best||
               !race_can_site(SIDE,type,x,y)||!can_place(type,x,y))continue;
            best=d;site_x=x;site_y=y;
        }
    }
    if(site_x<0)return 0;
    int worker=free_builder();
    if(worker<0)return 0;
    close_menus();
    clear_selected_building();
    move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    if(!u[worker].selected)return 0;
    KEY_TAP(KEY_B);
    for(int n=0;n<field_menu_count()&&menu_open&&menu_cursor!=menu_row;n++)KEY_TAP(KEY_DOWN);
    if(!menu_open||menu_cursor!=menu_row){close_menus();return 0;}
    KEY_TAP(KEY_A);
    if(mode!=MODE_BUILD||menu!=type){
        fprintf(stderr,"build menu frame=%d type=%d mode=%d menu=%d\n",frame,type,mode,menu);
        close_menus();return 0;
    }
    move_cursor(site_x,site_y);KEY_TAP(KEY_A);
    if(mode!=MODE_NORMAL)close_menus();
    if(!SC64_BUILD_WORKER_ORDER(u[worker].order)){
        fprintf(stderr,"build refused frame=%d type=%d site=%d,%d order=%d\n",frame,type,site_x,site_y,u[worker].order);
        return 0;
    }
    fprintf(trace,"{\"kind\":\"build\",\"frame\":%u,\"type\":%d,\"site\":[%d,%d]}\n",(unsigned)frame,type,site_x,site_y);
    return 1;
}

static int build_assimilator(void)
{
    int home=home_nexus(),best=-1,bd=0x7fffffff;
    if(home<0||count_type(B_ASSIMILATOR,0))return 0;
    for(int g=0;g<geyser_count;g++){
        int d=dist2(geysers[g].x,geysers[g].y,b[home].x,b[home].y);
        if(d<bd){bd=d;best=g;}
    }
    if(best<0||bd>320*320)return 0;
    return build_structure(B_ASSIMILATOR,MENU_REFINERY,0,0,geysers[best].x,geysers[best].y);
}

static int assign_gas_worker(void)
{
    int refinery=-1,gas_workers=0,worker=-1;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_ASSIMILATOR&&building_complete(i)){refinery=i;break;}
    if(refinery<0)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_PROBE){
        if(u[i].order==ORDER_GAS)++gas_workers;
        else if(worker<0&&u[i].order==ORDER_MINE&&!u[i].carry)worker=i;
    }
    if(gas_workers>=3||worker<0)return 0;
    close_menus();
    clear_selected_building();move_cursor(u[worker].x,u[worker].y);KEY_TAP(KEY_A);
    if(!u[worker].selected)return 0;
    move_cursor(b[refinery].x,b[refinery].y);KEY_TAP(KEY_A);
    return u[worker].order==ORDER_GAS;
}

static void census(const char *label)
{
    int cell=stasis_cell(),f=hero(78),h=hero(28);
    int enemy=0,enemy_b=0;
    for(int i=0;i<MAX_UNITS;i++)enemy+=u[i].alive&&u[i].owner==ENEMY;
    for(int i=0;i<MAX_BUILDINGS;i++)enemy_b+=b[i].alive&&b[i].owner==ENEMY;
    fprintf(trace,"{\"kind\":\"checkpoint\",\"label\":\"%s\",\"frame\":%u,\"call_index\":%u,\"screen\":%d,\"won\":%d,\"lost\":%d,"
        "\"side_units\":%d,\"army\":%d,\"probes\":%d,\"enemy_units\":%d,\"enemy_buildings\":%d,"
        "\"stasis_hp\":%d,\"fenix\":[%d,%d,%d],\"hyperion\":[%d,%d,%d],\"bank\":[%d,%d],\"supply\":[%d,%d],"
        "\"gateways\":%d,\"cursor\":[%d,%d],\"triggers_done\":[%u,%u,%u],\"outcome\":%u,\"fault\":[%u,%u,%u],"
        "\"towns\":[%u,%u,%u],\"waves\":[%u,%u],\"ai_wallets\":[%u,%u,%u,%u]}\n",
        label,(unsigned)frame,host_tick,screen,won,lost,
        live_units_owner(SIDE),army_count(),probe_count(),enemy,enemy_b,
        cell>=0?b[cell].hp:-1,f>=0?u[f].owner:-1,f>=0?u[f].hp:-1,f>=0?u[f].shield:-1,
        h>=0?u[h].owner:-1,h>=0?u[h].hp:-1,h>=0?u[h].shield:-1,minerals,gas,
        race_owner_supply(SIDE),race_owner_supply_cap(SIDE),count_type(B_GATEWAY,0),cx,cy,
        race_p8.runtime[0].done,race_p8.runtime[1].done,race_p8.runtime[2].done,
        race_p8.runtime[0].outcome,race_p8.runtime[0].fault,race_p8.runtime[1].fault,race_p8.runtime[2].fault,
        race_p8.towns[0].script,race_p8.towns[1].script,race_p8.towns[2].script,
        race_p8.towns[0].wave,race_p8.towns[1].wave,
        race_p8.ai_minerals[0],race_p8.ai_gas[0],race_p8.ai_minerals[1],race_p8.ai_gas[1]);
    fflush(trace);
}

int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:".local/traces/p8-controller.jsonl";
    unsigned limit_ticks=argc>2?(unsigned)strtoul(argv[2],0,10):P8_RUN_LIMIT;
    trace=fopen(path,"w");if(!trace){perror(path);return 2;}
    fprintf(trace,"{\"kind\":\"metadata\",\"schema\":1,\"mission\":27,\"profile\":\"%s\",\"input_policy\":\"ordinary game_update held/pressed only\"}\n",P8_CONTROLLER_PROFILE);
    game_init();prior=0;
    assert(screen==SCREEN_TITLE&&race_front.page==SC_FRONT_HOME);
    /* Campaign, page 4 of the two-entry mission list, second row, briefing. */
    KEY_TAP(KEY_A);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);KEY_TAP(KEY_RIGHT);
    KEY_TAP(KEY_DOWN);KEY_TAP(KEY_A);KEY_TAP(KEY_START);
    assert(screen==SCREEN_PLAY&&!won&&!lost&&race_selected_mission==27);
    census("launch");

    int saved=0;
    int next_checkpoint=2000;
    int next_probe=60,next_supply=400,next_build=600,next_gas=3000,next_production=900;
    int next_order=120,next_hero=240;
    int attacking=0,prior_distance=-1,stalls=0;
    while(host_tick<limit_ticks&&screen==SCREEN_PLAY&&!won&&!lost){
        static int stuck_reported;
        if(menu_open&&!stuck_reported){
            stuck_reported=1;
            fprintf(stderr,"menu left open frame=%d mode=%d cursor=%d rows=%d sel=%d selb=%d\n",
                    frame,mode,menu_cursor,field_menu_count(),selected_count(),selected_building);
        }
        if(menu_open)KEY_TAP(KEY_B);
        int home=home_nexus();
        int f=hero(78),h=hero(28);
        if(frame>=next_probe){(void)queue_probe();next_probe=frame+300;}
        if(frame>=next_supply){
            int pending=0;
            for(int i=0;i<MAX_BUILDINGS;i++)pending+=b[i].alive&&b[i].owner==SIDE&&
                b[i].type==B_PYLON&&b[i].under_construction;
            for(int i=0;i<MAX_UNITS;i++)pending+=u[i].alive&&u[i].owner==SIDE&&
                SC64_PENDING_BUILD_ORDER(u[i].order);
            if(pending<2&&race_owner_supply_cap(SIDE)-race_owner_supply(SIDE)<=12)
                (void)build_structure(B_PYLON,MENU_DEPOT,64,256,-1,-1);
            next_supply=frame+300;
        }
        if(frame>=next_build){
            if(count_type(B_GATEWAY,0)<1||(probe_count()>=10&&count_type(B_GATEWAY,0)<6))
                (void)build_structure(B_GATEWAY,MENU_BARRACKS,80,224,-1,-1);
            else if(!count_type(B_ASSIMILATOR,0))(void)build_assimilator();
            else if(!count_type(B_CYBERNETICS_CORE,0)&&count_type(B_GATEWAY,1))
                (void)build_structure(B_CYBERNETICS_CORE,MENU_ACADEMY,80,224,-1,-1);
            next_build=frame+(minerals>=400?240:600);
        }
        if(frame>=next_gas){(void)assign_gas_worker();next_gas=frame+700;}
        if(frame>=next_production){
            int queued=0;
            while(queued<3&&queue_combat_unit())++queued;
            next_production=frame+60;
        }
        /* Collect the rescuable heroes, then keep them at home. */
        if(frame>=next_hero){
            if(f>=0&&u[f].owner==RESCUABLE)
                (void)army_command(MENU_MOVE,u[f].x,u[f].y);
            else if(home>=0){
                int fx=b[home].x-120,fy=b[home].y-80;
                if(f>=0&&u[f].owner==SIDE&&dist2(u[f].x,u[f].y,fx,fy)>48*48)
                    (void)hero_command(f,fx,fy);
                if(h>=0&&u[h].owner==SIDE&&dist2(u[h].x,u[h].y,b[home].x,b[home].y)>96*96)
                    (void)hero_command(h,b[home].x,b[home].y+24);
            }
            next_hero=frame+900;
        }
        if(frame>=next_order&&home>=0){
            int threat=-1,bd=360*360;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY){
                int d=dist2(u[i].x,u[i].y,b[home].x,b[home].y);
                if(d<bd){bd=d;threat=i;}
            }
            if(threat>=0&&!attacking){
                (void)army_command(MENU_ATTACK,u[threat].x,u[threat].y);
                fprintf(trace,"{\"kind\":\"defend\",\"frame\":%u,\"target\":[%d,%d]}\n",
                    (unsigned)frame,u[threat].x,u[threat].y);
                next_hero=frame+30;
                next_order=frame+180;
            }
        }
        if(frame>=next_order){
            int cell=stasis_cell();
            if(!attacking&&army_count()>=24)attacking=1;
            if(attacking&&cell>=0){
                int sx=0,sy=0,n=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!IS_WORKER(u[i].type)&&
                    u[i].raw_type!=28&&u[i].raw_type!=78){sx+=u[i].x;sy+=u[i].y;++n;}
                int wx=b[cell].x,wy=b[cell].y,distance=-1;
                if(n)(void)waypoint_to(sx/n,sy/n,b[cell].x,b[cell].y,&wx,&wy,&distance);
                if(distance>=0&&prior_distance>=0&&distance+2>=prior_distance)++stalls;else stalls=0;
                prior_distance=distance;
                (void)army_command(MENU_ATTACK,wx,wy);
                fprintf(trace,"{\"kind\":\"order\",\"frame\":%u,\"target\":[%d,%d],\"route\":%d,\"army\":%d,\"stalls\":%d}\n",
                    (unsigned)frame,wx,wy,distance,army_count(),stalls);
                /* The whole-army order also moved the heroes; send them home. */
                next_hero=frame+30;
                if(army_count()<8)attacking=0;
            }
            next_order=frame+(attacking?360:600);
        }
        if(!saved&&frame>=P8_SAVE_FRAME){
            close_menus();
            unsigned first=host_tick;
            KEY_TAP(KEY_START);
            int paused=screen==SCREEN_PAUSE;
            KEY_TAP(KEY_B);
            KEY_TAP(KEY_START);
            saved=1;
            char save_path[512];
            snprintf(save_path,sizeof(save_path),"%s.sav",path);
            FILE *sf=fopen(save_path,"wb");
            if(!sf||fwrite(host_save,1,sizeof(host_save),sf)!=sizeof(host_save)){perror(save_path);return 2;}
            fclose(sf);
            fprintf(trace,"{\"kind\":\"save\",\"frame\":%u,\"first_call\":%u,\"resume_call\":%u,\"paused\":%d,\"screen\":%d}\n",
                (unsigned)frame,first,host_tick,paused,screen);
            census("saved");
        }
        if(frame>=next_checkpoint){census("progress");next_checkpoint=frame+2000;}
        input(0);
    }
    census("final");
    fclose(trace);
    printf("state: screen=%d menu_open=%d mode=%d src_pause=%u sel=%d selb=%d help=%d toast=%d\n",screen,menu_open,mode,race_p8.source_pause_active,selected_count(),selected_building,help,toast_timer);
    printf("p8 controller: frame=%u calls=%u won=%d lost=%d screen=%d fault=%u/%u/%u\n",
           (unsigned)frame,host_tick,won,lost,screen,race_p8.runtime[0].fault,
           race_p8.runtime[1].fault,race_p8.runtime[2].fault);
    return won?0:1;
}
