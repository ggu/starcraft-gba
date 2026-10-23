/* Normal-input completion probe for late Terran assault missions.
 * The compile-time entry selects the map. After game_init all mutations go
 * through game_update with ordinary buttons; world/source reads plan orders. */
#include "../starcraft/game.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if SC64_TERRAN_LATE_PROBE_MISSION != 13
#error "this runner targets The Hammer Falls / T10"
#endif
static FILE *trace;
static u32 ticks;
static int run_limit=200000,first=1,last=-1;
static u16 prior;
static int hero=-1;
static int ion_cannon_slot(void);
static int active(void){return ticks<(u32)run_limit&&screen==SCREEN_PLAY&&!won&&!lost;}
static int live_enemy_buildings(void){int n=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner))n++;return n;}
static void checkpoint(const char *kind){if(!trace)return;if(!first)fputs(",\n",trace);first=0;
 fprintf(trace,"{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,\"screen\":%d,\"mission\":%d,\"won\":%d,\"lost\":%d,\"faults\":%d,\"phase\":%u,\"objective_flags\":%u,\"race_completed\":%d,\"hostile_buildings\":%d,\"ion_cannon_slot\":%d,\"ion_cannon_alive\":%d,\"hero_slot\":%d,\"hero_raw\":%d,\"hero_alive\":%d,\"minerals\":%d,\"gas\":%d}",kind,(unsigned)ticks,(unsigned)frame,screen,race_selected_mission,won,lost,mission_unsupported_count,(unsigned)race_campaign.phase,(unsigned)race_campaign.objective_flags,race_completed,live_enemy_buildings(),ion_cannon_slot(),ion_cannon_slot()>=0,hero,hero>=0?u[hero].raw_type:-1,hero>=0?u[hero].alive:0,minerals,gas);fflush(trace);}
static void step(u16 held){if(!active())return;u16 pressed=held&(u16)~prior;game_update(held,pressed);prior=held;ticks++;
 if(trace&&(int)held!=last){if(!first)fputs(",\n",trace);first=0;fprintf(trace,"{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",(unsigned)ticks,held,pressed);last=held;}
 if(ticks%1000u==0)checkpoint("periodic");}
static void tap(u16 key){step(key);step(0);}
static void wait_ticks(int n){for(int i=0;i<n&&active();i++)step(0);}
static int normal(void){for(int i=0;i<3&&(menu_open||mode!=MODE_NORMAL);i++)tap(KEY_B);return active()&&!menu_open&&mode==MODE_NORMAL;}
static int clamp(int v,int max){return v<0?0:v>=max?max-1:v;}
static int snap4(int from,int to,int max){int d=to-from;return clamp(from+4*(d>=0?(d+2)/4:(d-2)/4),max);}
static void cursor(int x,int y){x=snap4(cx,x,WORLD_W);y=snap4(cy,y,WORLD_H);for(int i=0;i<3000&&(cx!=x||cy!=y)&&active();i++){u16 key=KEY_R;if(cx<x)key|=KEY_RIGHT;if(cx>x)key|=KEY_LEFT;if(cy<y)key|=KEY_DOWN;if(cy>y)key|=KEY_UP;step(key);}step(0);}
static void leave_building(void){if(selected_building>=0&&normal()){tap(KEY_B);tap(KEY_DOWN);tap(KEY_A);tap(KEY_A);}}
static int select_unit(int slot){if(slot<0||!u[slot].alive||u[slot].owner!=SIDE||!normal())return 0;leave_building();for(int n=0;n<=MAX_UNITS&&active();n++){if(u[slot].selected&&selected_count()==1&&selected_building<0)return 1;tap(KEY_L);}return 0;}
static int select_building(int slot){if(slot<0||!b[slot].alive||b[slot].owner!=SIDE||!normal())return 0;leave_building();cursor(b[slot].x,b[slot].y);tap(KEY_A);return selected_building==slot;}
static int order_unit(int slot,int x,int y,int attack){if(!select_unit(slot))return 0;tap(KEY_B);if(attack)tap(KEY_DOWN);tap(KEY_A);cursor(x,y);tap(KEY_A);return 1;}
static int group_type(int type,int x,int y){if(!normal())return 0;leave_building();int slot=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&!SC64_TRANSPORT_LOADED(&u[i])){cursor(u[i].x,u[i].y);if(selection_target()==i){slot=i;break;}}if(slot<0)return 0;tap(KEY_A);tap(KEY_A);if(selected_building>=0||!selected_count())return 0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&(u[i].owner!=SIDE||u[i].type!=type))return 0;tap(KEY_B);tap(KEY_DOWN);tap(KEY_A);cursor(x,y);tap(KEY_A);return 1;}
static int source_unit(int raw){for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw)return i;return -1;}
static int own_building(int type){int airborne=-1;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&building_complete(i)){if(!race_late_airborne(i))return i;airborne=i;}return airborne;}
static int own_units(int type){int n=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&(type<0||u[i].type==type))n++;return n;}
static int queue_at(int slot,int type){if(!select_building(slot))return 0;for(int i=0;i<32&&train_type!=type&&active();i++)tap(KEY_R);int before=b[slot].queue_count;tap(KEY_A);return b[slot].queue_count>before;}
static int nearest_enemy_building(void){int ax=hero>=0?u[hero].x:cx,ay=hero>=0?u[hero].y:cy;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&!IS_WORKER(u[i].type)&&i!=hero){ax=u[i].x;ay=u[i].y;break;}int best=-1,distance=0x7fffffff;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)){int d=dist2(ax,ay,b[i].x,b[i].y);if(d<distance){distance=d;best=i;}}return best;}
static void inspect(void){fprintf(stderr,"mission=%d frame=%d wallet=%d/%d supply=%d/%d\n",race_selected_mission,frame,minerals,gas,supply_used,supply_cap);for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE)fprintf(stderr,"U %d raw=%d type=%d xy=%d,%d hp=%d order=%d target=%d\n",i,u[i].raw_type,u[i].type,u[i].x,u[i].y,u[i].hp,u[i].order,u[i].target);for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner)&&(sc64_race_unit_damage_vs(u[i].type,0)>0||sc64_race_unit_damage_vs(u[i].type,1)>0))fprintf(stderr,"THREAT U%d raw=%d type=%d xy=%d,%d hp=%d ground=%d/%d air=%d/%d\n",i,u[i].raw_type,u[i].type,u[i].x,u[i].y,u[i].hp,sc64_race_unit_damage_vs(u[i].type,0),sc64_race_unit_range_vs(u[i].type,0),sc64_race_unit_damage_vs(u[i].type,1),sc64_race_unit_range_vs(u[i].type,1));for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&(b[i].owner==SIDE||b[i].raw_type==127||b[i].raw_type==124||b[i].raw_type==125))fprintf(stderr,"B %d raw=%d owner=%d type=%d xy=%d,%d hp=%d complete=%d level=%d flags=%d goal=%u\n",i,b[i].raw_type,b[i].owner,b[i].type,b[i].x,b[i].y,b[i].hp,building_complete(i),b[i].level,b[i].flags,(unsigned)race_late.flight_goals[i]);}
static void inspect_routes(void){for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&(u[i].raw_type==19||u[i].raw_type==29||u[i].raw_type==20))fprintf(stderr,"HERO %d raw=%d owner=%d xy=%d,%d hp=%d\n",i,u[i].raw_type,u[i].owner,u[i].x,u[i].y,u[i].hp);for(int i=0;i<MAX_MINERALS;i++)if(m[i].amount>0&&m[i].y>WORLD_H/2)fprintf(stderr,"M %d xy=%d,%d amount=%d\n",i,m[i].x,m[i].y,m[i].amount);}
static int choose_menu(int choice){if(!normal())return 0;tap(KEY_B);for(int i=0;i<choice&&active();i++)tap(KEY_DOWN);tap(KEY_A);return 1;}
static int land_building(int slot,int x,int y){if(!select_building(slot))return 0;if(!choose_menu(MENU_LIFT_LAND)||mode!=MODE_LAND)return 0;int px=x,py=y,found=0;for(int rad=0;rad<=24&&!found;rad+=4)for(int dy=-rad;dy<=rad&&!found;dy+=4)for(int dx=-rad;dx<=rad;dx+=4){int qx=snap4(cx,x+dx,WORLD_W),qy=snap4(cy,y+dy,WORLD_H);if(race_late_landing_site(slot,qx,qy)){px=qx;py=qy;found=1;break;}}if(!found)return 0;cursor(px,py);tap(KEY_A);return race_late.flight_goals[slot]!=0;}
static int choose_landing(int slot,int *x,int *y){int patch=nearest_mineral(b[slot].x,b[slot].y);if(patch<0)return 0;int best=0x7fffffff,found=0;for(int yy=24;yy<WORLD_H-24;yy+=16)for(int xx=24;xx<WORLD_W-24;xx+=16){if(!race_late_landing_site(slot,xx,yy))continue;int d=dist2(xx,yy,m[patch].x,m[patch].y);if(d<48*48||d>144*144)continue;int penalty=0;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)&&dist2(xx,yy,b[i].x,b[i].y)<200*200)penalty+=100000;int cost=d+penalty;if(cost<best){best=cost;*x=xx;*y=yy;found=1;}}return found;}
static void opening_t8(void){int cc=own_building(B_COMMAND),lx=0,ly=0;int w=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_WRAITH){w=i;break;}if(w>=0&&hero>=0){order_unit(w,u[hero].x,u[hero].y,0);wait_ticks(180);checkpoint("norad-contact");}
 if(cc<0||!choose_landing(cc,&lx,&ly)){fprintf(stderr,"No source base landing site\n");return;}fprintf(stderr,"Land CC%d at%d,%d\n",cc,lx,ly);land_building(cc,lx,ly);checkpoint("cc-relocation-started");
 int ship=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_DROPSHIP){ship=i;break;}if(ship>=0){int workers[8],n=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&n<8)workers[n++]=i;for(int k=0;k<n&&active();k++){int worker=workers[k];order_unit(ship,u[worker].x,u[worker].y,0);for(int t=0;t<1000&&active()&&dist2(u[ship].x,u[ship].y,u[worker].x,u[worker].y)>10*10;t++)step(0);if(select_unit(worker)){cursor(u[ship].x,u[ship].y);tap(KEY_A);}for(int t=0;t<1000&&active()&&!SC64_TRANSPORT_LOADED(&u[worker]);t++)step(0);fprintf(stderr,"Boardworker%d loaded%d\n",worker,SC64_TRANSPORT_LOADED(&u[worker]));}
 order_unit(ship,lx+64,ly+64,0);for(int t=0;t<6000&&active()&&dist2(u[ship].x,u[ship].y,lx+64,ly+64)>8*8;t++)step(0);if(select_unit(ship)){choose_menu(MENU_UNLOAD);cursor(lx+64,ly+64);tap(KEY_A);}wait_ticks(600);checkpoint("worker-landing");}
 int producers[]={B_BARRACKS,B_FACTORY,B_STARPORT,B_SCIENCE_FACILITY};for(unsigned k=0;k<sizeof(producers)/sizeof(producers[0])&&active();k++){int slot=own_building(producers[k]);if(slot<0)continue;int x=lx+(k%2?112:-112),y=ly+((int)k/2?112:-112);if(!race_late_landing_site(slot,x,y)){if(!choose_landing(slot,&x,&y))continue;}land_building(slot,x,y);}
 wait_ticks(3000);checkpoint("base-established");inspect();}
static int building_site_clearance(int type,int x,int y)
{
 Sc64TerranLateRect proposed;
 int raw=sc64_race_raw_building(type);
 if(raw<0||!sc64_terran_late_actor_rect(raw,1,x,y,&proposed))return 0;
 /* can_place uses circular center clearance, which can allow two large
  * Terran buildings to overlap at adjacent legal centers. Require their
  * source-derived footprint rectangles to leave an 8 GBA-pixel service gap. */
 for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&sc64_t4_building_collidable(i)){
  Sc64TerranLateRect old;
  int oldraw=sc64_terran_late_raw_building(&b[i]);
  /* Static source props without runtime footprints remain protected by
   * can_place's existing obstacle radius; only expand known building art. */
  if(!sc64_terran_late_actor_rect(oldraw,1,b[i].x,b[i].y,&old))continue;
  if(proposed.left<old.right+16&&old.left<proposed.right+16&&
     proposed.top<old.bottom+16&&old.top<proposed.bottom+16)return 0;
 }
 return 1;
}
static int paid_build_near(int type,int menu_choice,int nearx,int neary,int radius){int worker=-1,best=0x7fffffff;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&!SC64_TRANSPORT_LOADED(&u[i])&&u[i].order!=ORDER_GAS){int d=dist2(u[i].x,u[i].y,nearx,neary);if(d<best){best=d;worker=i;}}if(worker<0){fprintf(stderr,"Buildtype%d no eligible SCV\n",type);return -1;}if(!select_unit(worker)){fprintf(stderr,"Buildtype%d select SCV%d failed selected=%d building=%d mode=%d menu=%d\n",type,worker,selected_count(),selected_building,mode,menu_open);return -1;}int x=-1,y=-1,site=0,place=0,clear=0;best=0x7fffffff;for(int yy=24;yy<WORLD_H-24;yy+=8)for(int xx=24;xx<WORLD_W-24;xx+=8){int qx=snap4(cx,xx,WORLD_W),qy=snap4(cy,yy,WORLD_H),d=dist2(qx,qy,nearx,neary);if(d>radius*radius)continue;if(!race_can_site(SIDE,type,qx,qy))continue;site++;if(!can_place(type,qx,qy))continue;place++;if(!building_site_clearance(type,qx,qy))continue;clear++;if(d>=best)continue;best=d;x=qx;y=qy;}if(x<0){fprintf(stderr,"Buildtype%d no site radius=%d scanned=%d site=%d place=%d clear=%d\n",type,radius,((WORLD_W-48)/8)*((WORLD_H-48)/8),site,place,clear);return -1;}if(!choose_menu(menu_choice))return -1;cursor(x,y);tap(KEY_A);int result=-1;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==type&&dist2(b[i].x,b[i].y,x,y)<24*24){result=i;break;}if(result>=0){fprintf(stderr,"Buildtype%d slot%d at%d,%d worker%d\n",type,result,x,y,worker);for(int t=0;t<5000&&active()&&b[result].alive&&!building_complete(result);t++)step(0);checkpoint("paid-building");}else fprintf(stderr,"Buildtype%d placement click failed at%d,%d mode=%d menu=%d\n",type,x,y,mode,menu_open);return result;}
static void establish_economy(void){int cc=own_building(B_COMMAND);if(cc<0)return;if(minerals>=100)paid_build_near(B_DEPOT,MENU_DEPOT,b[cc].x,b[cc].y,160);int refinery=own_building(B_REFINERY);if(refinery<0&&minerals>=100){int gi=-1,best=0x7fffffff;for(int i=0;i<geyser_count;i++){int d=dist2(geysers[i].x,geysers[i].y,b[cc].x,b[cc].y);if(d<best){best=d;gi=i;}}if(gi>=0){fprintf(stderr,"Nearestgas%d at%d,%d distance%d\n",gi,geysers[gi].x,geysers[gi].y,best);refinery=paid_build_near(B_REFINERY,MENU_REFINERY,geysers[gi].x,geysers[gi].y,24);}}
 if(refinery>=0&&building_complete(refinery)){int assigned=0;for(int i=0;i<MAX_UNITS&&assigned<3&&active();i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_SCV&&!SC64_TRANSPORT_LOADED(&u[i])&&select_unit(i)){cursor(b[refinery].x,b[refinery].y);tap(KEY_A);if(u[i].order==ORDER_GAS)assigned++;}fprintf(stderr,"Gasassigned%d\n",assigned);}
}
static int group_all_to(int x,int y){if(!normal())return 0;leave_building();tap(KEY_R);if(!selected_count())return 0;choose_menu(MENU_ATTACK);cursor(x,y);tap(KEY_A);int cc=own_building(B_COMMAND);if(hero>=0&&u[hero].alive&&u[hero].owner==SIDE&&cc>=0)order_unit(hero,b[cc].x+48,b[cc].y+48,0);return 1;}
enum { T10_GRID_STEP=32 };
static int t10_risk_in_range(int d2,int range){int r=range+20;return d2<r*r?1+(r*r-d2)/1024:0;}
static u16 t10_cell_risk(int x,int y,int flying){int risk=0;
 for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner)){
  int damage=sc64_race_unit_damage_vs(u[i].type,flying);if(damage<=0)continue;
  int range=(sc64_race_unit_range_vs(u[i].type,flying)+1)/2;
  int dx=x-u[i].x,dy=y-u[i].y;risk+=t10_risk_in_range(dx*dx+dy*dy,range);
 }
 for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&owners_hostile(SIDE,b[i].owner)){
  int range=0;
  /* BWAPI Longbolt Missile range is 224 source pixels (112 GBA pixels). */
  if(b[i].raw_type==124&&flying)range=112;
  /* A bunker may contain a Marine squad; reserve its verified rifle-range
   * envelope. This is conservative because native bunker cargo is not read. */
  if(b[i].raw_type==125)range=96;
  if(!range)continue;int dx=x-b[i].x,dy=y-b[i].y;
  risk+=t10_risk_in_range(dx*dx+dy*dy,range);
 }
 if(risk>1000)risk=1000;return (u16)risk;
}
static int t10_threat_route(int sx,int sy,int gx,int gy,int flying,int outx[256],int outy[256])
{
 int cols=(WORLD_W+T10_GRID_STEP-1)/T10_GRID_STEP,rows=(WORLD_H+T10_GRID_STEP-1)/T10_GRID_STEP;
 int cells=cols*rows;if(cells<=0||cells>4096)return 0;
 static u16 risk[4096];static u32 cost[4096];static s16 prev[4096];static u8 done[4096];
 for(int row=0;row<rows;row++)for(int col=0;col<cols;col++){
  int i=col+row*cols,x=col*T10_GRID_STEP+T10_GRID_STEP/2,y=row*T10_GRID_STEP+T10_GRID_STEP/2;
  if(x>=WORLD_W)x=WORLD_W-1;if(y>=WORLD_H)y=WORLD_H-1;
  risk[i]=t10_cell_risk(x,y,flying);cost[i]=0x7fffffffu;prev[i]=-1;done[i]=0;
 }
 int sc=clamp(sx/T10_GRID_STEP,cols),sr=clamp(sy/T10_GRID_STEP,rows);
 int gc=clamp(gx/T10_GRID_STEP,cols),gr=clamp(gy/T10_GRID_STEP,rows);
 int start=sc+sr*cols,goal=gc+gr*cols;cost[start]=0;
 for(int n=0;n<cells;n++){
  int at=-1;u32 best=0x7fffffffu;
  for(int i=0;i<cells;i++)if(!done[i]&&cost[i]<best){best=cost[i];at=i;}
  if(at<0||at==goal)break;done[at]=1;
  int c=at%cols,r=at/cols;
  for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(dx||dy){int nc=c+dx,nr=r+dy;if(nc<0||nr<0||nc>=cols||nr>=rows)continue;int ni=nc+nr*cols;if(done[ni])continue;
   u32 edge=(dx&&dy?14u:10u)+(u32)risk[ni]*18u+(u32)risk[at]*5u;
   if(best<=0x7fffffffu-edge&&best+edge<cost[ni]){cost[ni]=best+edge;prev[ni]=(s16)at;}
  }
 }
 if(cost[goal]==0x7fffffffu)return 0;
 int reverse[4096],n=0;for(int at=goal;at>=0&&n<cells;at=prev[at]){reverse[n++]=at;if(at==start)break;}
 if(!n||reverse[n-1]!=start)return 0;
 int emitted=0,last_dirx=0,last_diry=0,last_keep=0;
 /* Keep bends and periodic checkpoints, capped to the fixed controller path. */
 for(int k=n-1;k>=0;k--){int idx=reverse[k],c=idx%cols,r=idx/cols;int x=c*T10_GRID_STEP+T10_GRID_STEP/2,y=r*T10_GRID_STEP+T10_GRID_STEP/2;
  if(x>=WORLD_W)x=WORLD_W-1;if(y>=WORLD_H)y=WORLD_H-1;
  int keep=(k==n-1||k==0||k%6==0);
  if(k<n-1&&k>0){int p=reverse[k+1],q=reverse[k-1];int dx1=(idx%cols)-(p%cols),dy1=(idx/cols)-(p/cols);int dx2=(q%cols)-(idx%cols),dy2=(q/cols)-(idx/cols);if(dx1!=dx2||dy1!=dy2)keep=1;}
  if(keep&&emitted<256){outx[emitted]=x;outy[emitted]=y;emitted++;last_keep=idx;}
  (void)last_dirx;(void)last_diry;(void)last_keep;
 }
 return emitted;
}
static int ion_cannon_slot(void){for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==127&&owners_hostile(SIDE,b[i].owner))return i;return -1;}
static void build_t10_supply(void){int cc=own_building(B_COMMAND);if(cc<0)return;for(int guard=0;guard<10&&active()&&supply_cap<188;guard++){if(minerals<100)break;int before=supply_cap;int dep=paid_build_near(B_DEPOT,MENU_DEPOT,b[cc].x,b[cc].y,320);if(dep<0||!building_complete(dep)||supply_cap<=before)break;fprintf(stderr,"T10 supply cap=%d depot=%d\n",supply_cap,dep);}}
static int t10_send_wraiths_to_ion(void){int cannon=ion_cannon_slot();if(cannon<0)return 0;int sx=0,sy=0,n=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_WRAITH&&!SC64_TRANSPORT_LOADED(&u[i])){sx+=u[i].x;sy+=u[i].y;n++;}if(n<8)return 0;sx/=n;sy/=n;int wx[256],wy[256];int count=t10_threat_route(sx,sy,b[cannon].x,b[cannon].y,1,wx,wy);if(!count)return 0;fprintf(stderr,"T10 Wraith route wing=%d waypoints=%d IonCannon=%d at%d,%d cost-planned\n",n,count,cannon,b[cannon].x,b[cannon].y);
 for(int i=0;i<count&&active()&&ion_cannon_slot()>=0;i++){if(!group_type(U_WRAITH,wx[i],wy[i])){fprintf(stderr,"T10 Wraith waypoint selection failed %d/%d selected=%d\n",i,count,selected_count());return 0;}for(int t=0;t<2400&&active();t++){int near=0;for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&u[j].type==U_WRAITH&&!SC64_TRANSPORT_LOADED(&u[j])&&dist2(u[j].x,u[j].y,wx[i],wy[i])<72*72)near++;if(near>=n*2/3)break;step(0);}if(i==count-1)checkpoint("ion-cannon-assault");}
 return ion_cannon_slot()<0;
}
static void opening_t10(void){if(hero>=0){int v=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_VULTURE){v=i;break;}if(v>=0){order_unit(v,u[hero].x,u[hero].y,0);wait_ticks(180);}}int cc=own_building(B_COMMAND);if(cc<0)return;int barracks=paid_build_near(B_BARRACKS,MENU_BARRACKS,b[cc].x,b[cc].y,192);(void)barracks;establish_economy();int factory=paid_build_near(B_FACTORY,MENU_FACTORY,b[cc].x,b[cc].y,256);if(factory>=0&&building_complete(factory))paid_build_near(B_STARPORT,MENU_STARPORT,b[cc].x,b[cc].y,256);build_t10_supply();}
static int run(void){
 wait_ticks(120);checkpoint("source-opening");
 if(race_selected_mission==11){opening_t8();establish_economy();int refinery=own_building(B_REFINERY);for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_COMMAND&&race_late_airborne(i)&&refinery>=0){int x=0,y=0;int best=0x7fffffff;for(int yy=24;yy<WORLD_H-24;yy+=16)for(int xx=24;xx<WORLD_W-24;xx+=16){int d=dist2(xx,yy,b[refinery].x,b[refinery].y);if(d<48*48||d>160*160||d>=best||!race_late_landing_site(i,xx,yy))continue;best=d;x=xx;y=yy;}if(best<0x7fffffff)land_building(i,x,y);}wait_ticks(3600);}
 if(race_selected_mission==13)opening_t10();
 u32 next_order=ticks,next_production=ticks;
 while(active()){
  if(ticks>=next_production){next_production=ticks+300;int cc=own_building(B_COMMAND);if(cc>=0&&b[cc].queue_count<3&&own_units(U_SCV)<14&&minerals>=50)queue_at(cc,U_SCV);int sp=own_building(B_STARPORT);if(sp>=0&&b[sp].queue_count<3&&own_units(U_WRAITH)<24&&minerals>=150&&gas>=100)queue_at(sp,U_WRAITH);int r=own_building(B_BARRACKS);if(r>=0&&b[r].queue_count<2&&own_units(U_MARINE)<10&&minerals>=200)queue_at(r,U_MARINE);}
  if(ticks>=next_order){next_order=ticks+9000;if(own_units(U_WRAITH)>=8){t10_send_wraiths_to_ion();checkpoint("airwing-progress");}}
  step(0);
 }
 checkpoint(won?"won":lost?"lost":"limit");inspect();
 return won&&!lost&&mission_unsupported_count==0&&hero>=0&&u[hero].alive&&u[hero].raw_type==19&&ion_cannon_slot()<0&&race_campaign.phase==SC64_RACE_PHASE_COMPLETE&&(race_completed&(1<<race_selected_mission));
}
int main(int argc,char **argv){const char *path=0;int read_only=0;for(int i=1;i<argc;i++){if(!strcmp(argv[i],"--trace")&&i+1<argc)path=argv[++i];else if(!strcmp(argv[i],"--limit")&&i+1<argc)run_limit=atoi(argv[++i]);else if(!strcmp(argv[i],"--inspect"))read_only=1;else return 2;}game_init();hero=source_unit(race_selected_mission==11?29:19);if(read_only){inspect();inspect_routes();return 0;}if(path){trace=fopen(path,"w");if(!trace)return 2;fprintf(trace,"{\"trace_schema\":1,\"entry\":\"t10-probe-entry\",\"policy\":\"bounded-normal-controller-attempt\",\"profile\":\"terran-t10\",\"mission\":13,\"tick_limit\":%d,\"events\":[\n",run_limit);}checkpoint("start");int success=run();if(trace){fputs("\n]}\n",trace);fclose(trace);}fprintf(stderr,"mission=%d win=%d lost=%d tick=%u frame=%u faults=%d success=%d\n",race_selected_mission,won,lost,(unsigned)ticks,(unsigned)frame,mission_unsupported_count,success);return success?0:1;}
