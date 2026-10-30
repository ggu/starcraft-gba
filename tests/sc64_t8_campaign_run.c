/* Normal-input completion probe for late Terran assault missions.
 * The compile-time entry selects the map. After game_init all mutations go
 * through game_update with ordinary buttons; world/source reads plan orders. */
#include "../starcraft/game.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if SC64_TERRAN_LATE_PROBE_MISSION != 11 && SC64_TERRAN_LATE_PROBE_MISSION != 13
#error "assault probe is for The Big Push or The Hammer Falls"
#endif
static FILE *trace;
static u32 ticks;
static int run_limit=200000,first=1,last=-1;
static u16 prior;
static int hero=-1;
static int active(void){return ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost;}
static int live_enemy_buildings(void){int n=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner))n++;return n;}
static void checkpoint(const char *kind){if(!trace)return;if(!first)fputs(",\n",trace);first=0;
 fprintf(trace,"{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,\"screen\":%d,\"mission\":%d,\"won\":%d,\"lost\":%d,\"faults\":%d,\"phase\":%u,\"objective_flags\":%u,\"race_completed\":%d,\"hostile_buildings\":%d,\"hero_alive\":%d,\"minerals\":%d,\"gas\":%d}",kind,(unsigned)ticks,(unsigned)frame,screen,race_selected_mission,won,lost,mission_unsupported_count,(unsigned)race_campaign.phase,(unsigned)race_campaign.objective_flags,race_completed,live_enemy_buildings(),hero>=0?u[hero].alive:0,minerals,gas);fflush(trace);}
static void step(u16 held){if(!active())return;u16 pressed=held&(u16)~prior;game_update(held,pressed);prior=held;ticks++;
 if(trace&&(int)held!=last){if(!first)fputs(",\n",trace);first=0;fprintf(trace,"{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",(unsigned)ticks,held,pressed);last=held;}
 if(ticks%1000u==0)checkpoint("periodic");}
static void tap(u16 key){step(key);step(0);}

/* Move the remembered field menu to the first row with ordinary input. */
static void tap_b(void)
{
    int fresh = !menu_open && mode == MODE_NORMAL;
    tap(KEY_B);
    if (fresh && menu_open) {
        int count=field_menu_count();
        for (int i=0; i<count && menu_open && menu_cursor!=MENU_MOVE; ++i) tap(KEY_DOWN);
    }
}
static void wait_ticks(int n){for(int i=0;i<n&&active();i++)step(0);}
static int normal(void){for(int i=0;i<3&&(menu_open||mode!=MODE_NORMAL);i++)tap_b();return active()&&!menu_open&&mode==MODE_NORMAL;}
static int clamp(int v,int max){return v<0?0:v>=max?max-1:v;}
static int snap4(int from,int to,int max){int d=to-from;return clamp(from+4*(d>=0?(d+2)/4:(d-2)/4),max);}
static void cursor(int x,int y){x=snap4(cx,x,WORLD_W);y=snap4(cy,y,WORLD_H);for(int i=0;i<3000&&(cx!=x||cy!=y)&&active();i++){u16 key=KEY_R;if(cx<x)key|=KEY_RIGHT;if(cx>x)key|=KEY_LEFT;if(cy<y)key|=KEY_DOWN;if(cy>y)key|=KEY_UP;step(key);}step(0);}
static void leave_building(void){if(selected_building>=0&&normal())tap(KEY_SELECT);}
static int select_unit(int slot){if(slot<0||!u[slot].alive||u[slot].owner!=SIDE||!normal())return 0;leave_building();for(int n=0;n<=MAX_UNITS&&active();n++){if(u[slot].selected&&selected_count()==1&&selected_building<0)return 1;tap(KEY_L);}return 0;}
static int select_building(int slot){if(slot<0||!b[slot].alive||b[slot].owner!=SIDE||!normal())return 0;leave_building();cursor(b[slot].x-12,b[slot].y-12);tap(KEY_A);return selected_building==slot;}
static int order_unit(int slot,int x,int y,int attack){if(!select_unit(slot))return 0;tap_b();if(attack)tap(KEY_DOWN);tap(KEY_A);cursor(x,y);tap(KEY_A);return 1;}
static int group_type(int type,int x,int y){if(!normal())return 0;leave_building();int slot=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&!SC64_TRANSPORT_LOADED(&u[i])){cursor(u[i].x,u[i].y);if(selection_target()==i){slot=i;break;}}if(slot<0)return 0;tap(KEY_A);tap(KEY_A);if(selected_building>=0||!selected_count())return 0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&(u[i].owner!=SIDE||u[i].type!=type))return 0;tap_b();tap(KEY_DOWN);tap(KEY_A);cursor(x,y);tap(KEY_A);return 1;}
static int source_unit(int raw){for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw)return i;return -1;}
static int own_building(int type){int airborne=-1;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&building_complete(i)){if(!race_late_airborne(i))return i;airborne=i;}return airborne;}
static int own_units(int type){int n=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&(type<0||u[i].type==type))n++;return n;}
static int queue_at(int slot,int type){if(!select_building(slot))return 0;for(int i=0;i<32&&train_type!=type&&active();i++)tap(KEY_R);int before=b[slot].queue_count;tap(KEY_A);return b[slot].queue_count>before;}
static int nearest_enemy_building(void){int ax=hero>=0?u[hero].x:cx,ay=hero>=0?u[hero].y:cy;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!IS_WORKER(u[i].type)&&i!=hero){ax=u[i].x;ay=u[i].y;break;}int best=-1,distance=0x7fffffff;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)){int d=dist2(ax,ay,b[i].x,b[i].y);if(d<distance){distance=d;best=i;}}return best;}
static void inspect(void){fprintf(stderr,"mission=%d frame=%d wallet=%d/%d supply=%d/%d\n",race_selected_mission,frame,minerals,gas,supply_used,supply_cap);for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE)fprintf(stderr,"U %d raw=%d type=%d xy=%d,%d hp=%d order=%d target=%d\n",i,u[i].raw_type,u[i].type,u[i].x,u[i].y,u[i].hp,u[i].order,u[i].target);for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&(b[i].owner==SIDE||b[i].raw_type==127))fprintf(stderr,"B %d raw=%d owner=%d type=%d xy=%d,%d hp=%d complete=%d level=%d flags=%d goal=%u\n",i,b[i].raw_type,b[i].owner,b[i].type,b[i].x,b[i].y,b[i].hp,building_complete(i),b[i].level,b[i].flags,(unsigned)race_late.flight_goals[i]);}
static void inspect_routes(void){for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&(u[i].raw_type==19||u[i].raw_type==29||u[i].raw_type==20))fprintf(stderr,"HERO %d raw=%d owner=%d xy=%d,%d hp=%d\n",i,u[i].raw_type,u[i].owner,u[i].x,u[i].y,u[i].hp);for(int i=0;i<MAX_MINERALS;i++)if(m[i].amount>0&&m[i].y>WORLD_H/2)fprintf(stderr,"M %d xy=%d,%d amount=%d\n",i,m[i].x,m[i].y,m[i].amount);}
static int choose_menu(int choice){if(!normal())return 0;tap_b();for(int i=0;i<choice&&active();i++)tap(KEY_DOWN);tap(KEY_A);return 1;}
static int land_building(int slot,int x,int y){if(!select_building(slot))return 0;if(!choose_menu(MENU_LIFT_LAND)||mode!=MODE_LAND)return 0;int px=x,py=y,found=0;for(int rad=0;rad<=24&&!found;rad+=4)for(int dy=-rad;dy<=rad&&!found;dy+=4)for(int dx=-rad;dx<=rad;dx+=4){int qx=snap4(cx,x+dx,WORLD_W),qy=snap4(cy,y+dy,WORLD_H);if(race_late_landing_site(slot,qx,qy)){px=qx;py=qy;found=1;break;}}if(!found)return 0;cursor(px,py);tap(KEY_A);return race_late.flight_goals[slot]!=0;}
static int choose_landing(int slot,int *x,int *y){int patch=nearest_mineral(b[slot].x,b[slot].y);if(patch<0)return 0;int best=0x7fffffff,found=0;for(int yy=24;yy<WORLD_H-24;yy+=16)for(int xx=24;xx<WORLD_W-24;xx+=16){if(!race_late_landing_site(slot,xx,yy))continue;int d=dist2(xx,yy,m[patch].x,m[patch].y);if(d<48*48||d>144*144)continue;int penalty=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)&&dist2(xx,yy,b[i].x,b[i].y)<200*200)penalty+=100000;int cost=d+penalty;if(cost<best){best=cost;*x=xx;*y=yy;found=1;}}return found;}
static void opening_t8(void){int cc=own_building(B_COMMAND),lx=0,ly=0;int w=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_WRAITH){w=i;break;}if(w>=0&&hero>=0){order_unit(w,u[hero].x,u[hero].y,0);wait_ticks(180);checkpoint("norad-contact");}
 if(cc<0||!choose_landing(cc,&lx,&ly)){fprintf(stderr,"No source base landing site\n");return;}fprintf(stderr,"Land CC%d at%d,%d\n",cc,lx,ly);land_building(cc,lx,ly);checkpoint("cc-relocation-started");
 int ship=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DROPSHIP){ship=i;break;}if(ship>=0){int workers[8],n=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&n<8)workers[n++]=i;for(int k=0;k<n&&active();k++){int worker=workers[k];order_unit(ship,u[worker].x,u[worker].y,0);for(int t=0;t<1000&&active()&&dist2(u[ship].x,u[ship].y,u[worker].x,u[worker].y)>10*10;t++)step(0);if(select_unit(worker)){cursor(u[ship].x,u[ship].y);tap(KEY_A);}for(int t=0;t<1000&&active()&&!SC64_TRANSPORT_LOADED(&u[worker]);t++)step(0);fprintf(stderr,"Boardworker%d loaded%d\n",worker,SC64_TRANSPORT_LOADED(&u[worker]));}
 order_unit(ship,lx+64,ly+64,0);for(int t=0;t<6000&&active()&&dist2(u[ship].x,u[ship].y,lx+64,ly+64)>8*8;t++)step(0);if(select_unit(ship)){choose_menu(MENU_UNLOAD);cursor(lx+64,ly+64);tap(KEY_A);}wait_ticks(600);checkpoint("worker-landing");}
 int producers[]={B_BARRACKS,B_FACTORY,B_STARPORT,B_SCIENCE_FACILITY};for(unsigned k=0;k<sizeof(producers)/sizeof(producers[0])&&active();k++){int slot=own_building(producers[k]);if(slot<0)continue;int x=lx+(k%2?112:-112),y=ly+((int)k/2?112:-112);if(!race_late_landing_site(slot,x,y)){if(!choose_landing(slot,&x,&y))continue;}land_building(slot,x,y);}
 wait_ticks(3000);checkpoint("base-established");inspect();}
/* Return the mining crew to the original mineral line once the expansion is
 * depleted. The Dropship and each passenger are controlled through buttons. */
static void transfer_miners_home(void)
{
    int ship=-1, home=-1, workers[8], count=0;
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DROPSHIP){ship=i;break;}
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_COMMAND&&
           b[i].x>1100&&!race_late_airborne(i)){home=i;break;}
    if(ship<0||home<0)return;
    for(int i=0;i<MAX_UNITS&&count<8;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&u[i].x<1000&&
           u[i].order!=ORDER_GAS&&build_worker_available(i,SIDE,-1))workers[count++]=i;
    for(int k=0;k<count&&active();k++){
        int w=workers[k];
        if(!order_unit(ship,u[w].x,u[w].y,0))continue;
        for(int t=0;t<1500&&active()&&dist2(u[ship].x,u[ship].y,u[w].x,u[w].y)>10*10;t++)step(0);
        if(select_unit(w)){cursor(u[ship].x,u[ship].y);tap(KEY_A);}
        for(int t=0;t<1500&&active()&&!SC64_TRANSPORT_LOADED(&u[w]);t++)step(0);
    }
    if(!order_unit(ship,b[home].x-64,b[home].y+32,0))return;
    for(int t=0;t<6000&&active()&&dist2(u[ship].x,u[ship].y,b[home].x-64,b[home].y+32)>12*12;t++)step(0);
    if(select_unit(ship)&&choose_menu(MENU_UNLOAD)){cursor(b[home].x-64,b[home].y+32);tap(KEY_A);}
    wait_ticks(300);
    checkpoint("mineral-line-transfer");
}
/* Move an existing Command Center and eight SCVs to the third source mineral
 * field using only normal lift/land, move, board, and unload inputs. */
static void expand_northwest(void)
{
    int cc=-1,ship=-1,workers[8],n=0,x=0,y=0,best=0x7fffffff;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_COMMAND&&b[i].x>1100){cc=i;break;}
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].type==U_DROPSHIP){ship=i;break;}
    if(cc<0||ship<0)return;
    if(!race_late_airborne(cc)){
        if(!select_building(cc)||!choose_menu(MENU_LIFT_LAND))return;
        for(int t=0;t<500&&active()&&!race_late_airborne(cc);t++)step(0);
    }
    if(!race_late_airborne(cc))return;
    for(int yy=850;yy<1150;yy+=16)for(int xx=350;xx<700;xx+=16){
        int d=dist2(xx,yy,528,1000);
        if(d>=48*48&&d<=120*120&&d<best&&race_late_landing_site(cc,xx,yy))
            {best=d;x=xx;y=yy;}
    }
    if(best==0x7fffffff)return;
    if(!race_late_airborne(cc)||!land_building(cc,x,y))return;
    for(int i=0;i<MAX_UNITS&&n<8;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].type==U_SCV&&u[i].x>1100&&build_worker_available(i,SIDE,-1))workers[n++]=i;
    for(int k=0;k<n&&active();k++){
        int w=workers[k];
        if(!order_unit(ship,u[w].x,u[w].y,0))continue;
        for(int t=0;t<1500&&active()&&dist2(u[ship].x,u[ship].y,u[w].x,u[w].y)>10*10;t++)step(0);
        if(select_unit(w)){cursor(u[ship].x,u[ship].y);tap(KEY_A);}
        for(int t=0;t<1500&&active()&&!SC64_TRANSPORT_LOADED(&u[w]);t++)step(0);
    }
    if(!order_unit(ship,x+64,y,0))return;
    for(int t=0;t<6000&&active()&&dist2(u[ship].x,u[ship].y,x+64,y)>12*12;t++)step(0);
    if(select_unit(ship)&&choose_menu(MENU_UNLOAD)){cursor(x+64,y);tap(KEY_A);}
    wait_ticks(500);checkpoint("northwest-expansion");
}
static int paid_build_near(int type,int menu_choice,int nearx,int neary,int radius){int worker=-1,best=0x7fffffff;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&!SC64_TRANSPORT_LOADED(&u[i])&&u[i].order!=ORDER_GAS&&u[i].order!=ORDER_BUILD&&u[i].order!=ORDER_BUILD_PENDING){int d=dist2(u[i].x,u[i].y,nearx,neary);if(d<best){best=d;worker=i;}}if(worker<0||!select_unit(worker))return -1;int x=-1,y=-1;best=0x7fffffff;for(int yy=24;yy<WORLD_H-24;yy+=8)for(int xx=24;xx<WORLD_W-24;xx+=8){int qx=snap4(cx,xx,WORLD_W),qy=snap4(cy,yy,WORLD_H),d=dist2(qx,qy,nearx,neary),cost=dist2(qx,qy,u[worker].x,u[worker].y);if(d>radius*radius||cost>=best||!race_can_site(SIDE,type,qx,qy)||!can_place(type,qx,qy))continue;int room=1;for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&!race_late_airborne(j)&&dist2(qx,qy,b[j].x,b[j].y)<(type==B_REFINERY?36*36:48*48)){room=0;break;}if(!room)continue;best=cost;x=qx;y=qy;}if(x<0||!choose_menu(menu_choice))return -1;cursor(x,y);tap(KEY_A);if(u[worker].order!=ORDER_BUILD_PENDING||u[worker].target!=type){fprintf(stderr,"Buildtype%d pending intent failed worker%d order%d product%d at%d,%d\n",type,worker,u[worker].order,u[worker].target,x,y);return -1;}int result=-1;for(int t=0;t<5000&&active()&&u[worker].alive&&u[worker].order==ORDER_BUILD_PENDING;t++)step(0);for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&b[i].under_construction&&dist2(b[i].x,b[i].y,x,y)<24*24){result=i;break;}if(result>=0){fprintf(stderr,"Buildtype%d slot%d at%d,%d worker%d\n",type,result,x,y,worker);for(int t=0;t<5000&&active()&&b[result].alive&&!building_complete(result);t++)step(0);checkpoint("paid-building");}return result;}
static void establish_economy(void){int cc=own_building(B_COMMAND);if(cc<0)return;if(minerals>=100)paid_build_near(B_DEPOT,MENU_DEPOT,b[cc].x,b[cc].y,160);int refinery=own_building(B_REFINERY);if(refinery<0&&minerals>=100){int gi=-1,best=0x7fffffff;for(int i=0;i<geyser_count;i++){int d=dist2(geysers[i].x,geysers[i].y,b[cc].x,b[cc].y);if(d<best){best=d;gi=i;}}if(gi>=0){fprintf(stderr,"Nearestgas%d at%d,%d distance%d\n",gi,geysers[gi].x,geysers[gi].y,best);refinery=paid_build_near(B_REFINERY,MENU_REFINERY,geysers[gi].x,geysers[gi].y,24);}}
 if(refinery>=0&&building_complete(refinery)){int assigned=0;for(int i=0;i<MAX_UNITS&&assigned<1&&active();i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&!SC64_TRANSPORT_LOADED(&u[i])&&select_unit(i)){cursor(b[refinery].x,b[refinery].y);tap(KEY_A);if(u[i].order==ORDER_GAS)assigned++;}fprintf(stderr,"Gasassigned%d\n",assigned);}
}
static int group_all_to(int x,int y){if(!normal())return 0;leave_building();tap(KEY_R);if(!selected_count())return 0;choose_menu(MENU_ATTACK);cursor(x,y);tap(KEY_A);int cc=own_building(B_COMMAND);if(hero>=0&&u[hero].alive&&u[hero].owner==SIDE&&cc>=0)order_unit(hero,b[cc].x+48,b[cc].y+48,0);return 1;}
static void opening_t10(void){if(hero>=0){int v=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_VULTURE){v=i;break;}if(v>=0){order_unit(v,u[hero].x,u[hero].y,0);wait_ticks(180);}}int cc=own_building(B_COMMAND);if(cc<0)return;int barracks=paid_build_near(B_BARRACKS,MENU_BARRACKS,b[cc].x,b[cc].y,160);(void)barracks;establish_economy();int factory=paid_build_near(B_FACTORY,MENU_FACTORY,b[cc].x,b[cc].y,192);if(factory>=0&&building_complete(factory))paid_build_near(B_STARPORT,MENU_STARPORT,b[cc].x,b[cc].y,192);}
static int run(void){
 wait_ticks(120);checkpoint("source-opening");
 if(race_selected_mission==11){opening_t8();establish_economy();int refinery=own_building(B_REFINERY);for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_COMMAND&&race_late_airborne(i)&&refinery>=0){int x=0,y=0;int best=0x7fffffff;for(int yy=24;yy<WORLD_H-24;yy+=16)for(int xx=24;xx<WORLD_W-24;xx+=16){int d=dist2(xx,yy,b[refinery].x,b[refinery].y);if(d<48*48||d>160*160||d>=best||!race_late_landing_site(i,xx,yy))continue;best=d;x=xx;y=yy;}if(best<0x7fffffff)land_building(i,x,y);}wait_ticks(3600);}
 if(race_selected_mission==13)opening_t10();
 u32 next_order=ticks,next_production=ticks;
 int fleet_launched=0, transferred=0;
 while(active()){
  if(transferred==0&&ticks>=35000){++transferred;transfer_miners_home();}
  if(transferred==1&&ticks>=95000){++transferred;expand_northwest();}
  if(ticks>=next_production){next_production=ticks+300;int cc=own_building(B_COMMAND);if(cc>=0&&race_owner_reserved_supply(SIDE)+8>supply_cap&&minerals>=100){paid_build_near(B_DEPOT,MENU_DEPOT,b[cc].x,b[cc].y,224);checkpoint("supply-expanded");}if(cc>=0&&b[cc].queue_count<3&&own_units(U_SCV)<16&&minerals>=50)queue_at(cc,U_SCV);int ports=0;for(int k=0;k<MAX_BUILDINGS;k++)ports+=b[k].alive&&b[k].owner==SIDE&&b[k].type==B_STARPORT;
   if(cc>=0&&ports<2&&minerals>=400&&gas>=200)paid_build_near(B_STARPORT,MENU_STARPORT,b[cc].x,b[cc].y,224);
   for(int sp=0;sp<MAX_BUILDINGS;sp++)if(b[sp].alive&&b[sp].owner==SIDE&&b[sp].type==B_STARPORT&&building_complete(sp)&&b[sp].queue_count<3&&minerals>=150&&gas>=100)queue_at(sp,U_WRAITH);int r=own_building(B_BARRACKS);if(r>=0&&b[r].queue_count<2&&own_units(U_MARINE)<6&&minerals>=200)queue_at(r,U_MARINE);}
  if(ticks>=next_order){next_order=ticks+1000;int target=nearest_enemy_building();if(target>=0&&(own_units(U_WRAITH)>=16||(fleet_launched&&own_units(U_WRAITH)>0))){fleet_launched=1;group_type(U_WRAITH,b[target].x,b[target].y);checkpoint("assault-order");}}
  step(0);
 }
 checkpoint(won?"won":lost?"lost":"limit");inspect();
 return won&&!lost&&mission_unsupported_count==0&&hero>=0&&u[hero].alive&&race_campaign.phase==SC64_RACE_PHASE_COMPLETE&&(race_completed&(1<<race_selected_mission));
}
int main(int argc,char **argv){const char *path=0;int read_only=0;for(int i=1;i<argc;i++){if(!strcmp(argv[i],"--trace")&&i+1<argc)path=argv[++i];else if(!strcmp(argv[i],"--limit")&&i+1<argc)run_limit=atoi(argv[++i]);else if(!strcmp(argv[i],"--inspect"))read_only=1;else return 2;}if(run_limit<1||run_limit>200000)return 2;game_init();hero=source_unit(race_selected_mission==11?29:19);if(read_only){inspect();inspect_routes();return 0;}if(path){trace=fopen(path,"w");if(!trace)return 2;fprintf(trace,"{\"trace_schema\":1,\"entry\":\"t8-probe-entry\",\"policy\":\"bounded-normal-controller-attempt\",\"profile\":\"terran-t8\",\"mission\":%d,\"tick_limit\":%d,\"events\":[\n",race_selected_mission,run_limit);}checkpoint("start");int success=run();if(trace){fputs("\n]}\n",trace);fclose(trace);}fprintf(stderr,"mission=%d win=%d lost=%d tick=%u frame=%u faults=%d success=%d\n",race_selected_mission,won,lost,(unsigned)ticks,(unsigned)frame,mission_unsupported_count,success);return success?0:1;}
