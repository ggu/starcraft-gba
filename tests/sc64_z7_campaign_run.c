/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Normal-input Z7 campaign qualification controller.
 * The controller observes the live world for steering, but every gameplay
 * change is made through game_update held/pressed input. The opening keeps
 * the existing combat force together and attack-moves toward observed threats. */
static void z7_hit_observer(int target, int damage, int source_type, int source_owner);
#define SC64_TEST_HIT_OBSERVER(t,d,s,o) z7_hit_observer((t),(d),(s),(o))
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define Z7_DEFAULT_LIMIT 120000u
#define Z7_CHECKPOINT_PERIOD 300

static FILE *trace_file;
static int campaign_source_region4(int x,int y);
static u16 prior_keys;
static u32 host_ticks;
static unsigned drone_deaths;
static u32 run_limit=Z7_DEFAULT_LIMIT;
/* Opt-in research policy; neither mode is a verified mission winner. */
static int early_gas_policy;
static int trace_first=1;
static int next_macro_frame;
static int next_attack_frame;
static int last_attack_building=-1;
static int defense_staged;
static int campaign_assault_active;
static int campaign_assault_target=-1;
static int campaign_assault_is_building;
static int campaign_assault_target_raw;
static int campaign_assault_hold_set;
static int campaign_recon_active,campaign_recon_x,campaign_recon_y,campaign_recon_stage;
static int campaign_guardian_target=-1;
static int campaign_guardian_return_set;
static int campaign_single_sunken_opening=1;
static int defense_hydras_rallied;
static unsigned next_hydra_rally_frame;
static int mutas_air_ordered,mutas_guardian_ordered;
static int mutas_air_retry_frame,mutas_guardian_retry_frame;
static int focused_muta_slot=-1;
static int focused_muta_started,focused_muta_next_order;
static int early_hydra_target_slot=-1;
static int early_hydra_return_to_cover;
static unsigned early_hydra_next_order;
static unsigned early_muta_return_retry_frame;
static int defense_rally_after_focus;
static int local_air_target_slot=-1;
static int local_air_return_to_cover;
static unsigned local_air_next_order;
static int accept_guardian_air_screen;
static int last_direct_order_target=-1;
static int muta_scout_leg=-1;
static int late_air_breakthrough_active;
static unsigned late_air_scout_retry_frame;
static int late_air_scout_active,late_air_scout_x,late_air_scout_y;
static unsigned muta_scout_order_frame;
static unsigned muta_scout_retry_frame;
static int muta_scout_complete;
static int muta_guard_set;
static int hydra_ground_target_slot=-1;
static int hydra_ground_return_to_cover;
static unsigned hydra_ground_next_order;
static int defense_cover_set;
static int defense_air_screen_set;
static int defense_screen_combat_count=-1;
static int defense_screen_enemy=-1;
static int defense_screen_return_pending;
static unsigned defense_screen_order_retry_frame;
static int macro_worker_goal=3;
static int defense_attack_frame;
static int workers_retreating;
static unsigned workers_safe_since;
static unsigned retreat_phase;
static unsigned z7_evacuation_retry_frame;
static int geyser_attack_sent;
static int geyser_secured;
static int geyser_scout_ordered;
static int geyser_visible_extractor_found;
static int geyser_mutas_returned;
static unsigned geyser_return_retry_frame;
static int scourge_outputs_ordered;
static int select_mutalisk_cluster_for_scout;
static unsigned scourge_guard_retry_frame;
static void z7_mineral_cover_point(int hatch,int *x,int *y);
static int raw_building(int raw,int owner);
static int z7_townhall(int owner);
static int z7_front_colony_site(int hatch,int *out_x,int *out_y);
static int geyser_target_x=-1,geyser_target_y=-1;
static int geyser_clear_order_frame;
static int actions_taken;
static int last_build_fail_type=-1;
static int last_build_fail_frame=-10000;
static int build_probe_done;
static int hatch_probe_x=-1,hatch_probe_y=-1;
static const char *trace_path=".local/build/z7-controller/z7-controller-attempt.json";

static void z7_hit_observer(int target, int damage, int source_type, int source_owner)
{
    int victim_owner=-1, victim_raw=-1, victim_type=-1;
    int victim_x=-1, victim_y=-1, victim_hp=-1, victim_shield=-1;
    const char *victim_kind="invalid";
    if(target>=0&&target<MAX_UNITS) {
        victim_kind="unit"; victim_owner=u[target].owner; victim_raw=u[target].raw_type;
        victim_type=u[target].type; victim_x=u[target].x; victim_y=u[target].y;
        victim_hp=u[target].hp; victim_shield=u[target].shield;
    } else if(target>=MAX_UNITS&&target<MAX_UNITS+MAX_BUILDINGS) {
        int slot=target-MAX_UNITS;
        victim_kind="building"; victim_owner=b[slot].owner; victim_raw=b[slot].raw_type;
        victim_type=b[slot].type; victim_x=b[slot].x; victim_y=b[slot].y;
        victim_hp=b[slot].hp; victim_shield=b[slot].shield;
    } else return;
    if(victim_owner!=SIDE)return;
    int near_ground=-1,near_air=-1,ground_d2=0x7fffffff,air_d2=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&owners_hostile(SIDE,u[i].owner)&&
        currently_visible(u[i].x,u[i].y)&&sc64_race_unit_damage(u[i].type)>0) {
        int d=dist2(victim_x,victim_y,u[i].x,u[i].y);
        if(sc64_race_unit_flying(u[i].type)) {
            if(d<air_d2){air_d2=d;near_air=i;}
        } else if(d<ground_d2){ground_d2=d;near_ground=i;}
    }
    fprintf(stderr,"HIT host%u frame%u target%d %s raw%d type%d owner%d xy%d,%d hp%d shield%d damage%d src_type%d src_owner%d",
        host_ticks,(unsigned)frame,target,victim_kind,victim_raw,victim_type,victim_owner,
        victim_x,victim_y,victim_hp,victim_shield,damage,source_type,source_owner);
    if(near_ground>=0)fprintf(stderr," near_ground slot%d raw%d type%d xy%d,%d hp%d order%d target%d cd%d d2%d",
        near_ground,u[near_ground].raw_type,u[near_ground].type,u[near_ground].x,u[near_ground].y,
        u[near_ground].hp,u[near_ground].order,u[near_ground].target,u[near_ground].cd,ground_d2);
    if(near_air>=0)fprintf(stderr," near_air slot%d raw%d type%d xy%d,%d hp%d order%d target%d cd%d d2%d",
        near_air,u[near_air].raw_type,u[near_air].type,u[near_air].x,u[near_air].y,
        u[near_air].hp,u[near_air].order,u[near_air].target,u[near_air].cd,air_d2);
    fputc('\n',stderr);
    /* Diagnostic-only census: record raw44 positions and visibility at the
       instant a Guardian-class hit is dealt. These coordinates never feed
       an order or target-selection decision. */
    static unsigned last_guardian_sample=~0u;
    if(source_type==U_Z6_GUARDIAN&&last_guardian_sample!=(unsigned)frame) {
        last_guardian_sample=(unsigned)frame;
        int hatch=z7_townhall(SIDE),cover_x=-1,cover_y=-1;
        if(hatch>=0)z7_mineral_cover_point(hatch,&cover_x,&cover_y);
        fprintf(stderr,"GUARDIAN_HIT_CENSUS frame%u victim%d xy%d,%d cover%d,%d hatch%d,%d\n",
            (unsigned)frame,target,victim_x,victim_y,cover_x,cover_y,
            hatch>=0?b[hatch].x:-1,hatch>=0?b[hatch].y:-1);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==44)
            fprintf(stderr,"  raw44 slot%d type%d visible%d xy%d,%d distVictim%d distCover%d distHatch%d order%d target%d\n",
                i,u[i].type,currently_visible(u[i].x,u[i].y),u[i].x,u[i].y,
                dist2(victim_x,victim_y,u[i].x,u[i].y),
                cover_x>=0?dist2(cover_x,cover_y,u[i].x,u[i].y):-1,
                hatch>=0?dist2(b[hatch].x,b[hatch].y,u[i].x,u[i].y):-1,
                u[i].order,u[i].target);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==43&&
            (u[i].owner==SIDE||dist2(u[i].x,u[i].y,victim_x,victim_y)<256*256)) {
            int cell_x=u[i].x/16,cell_y=u[i].y/16;
            int guardian_x=-1,guardian_y=-1,guardian_slot=-1;
            for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==ENEMY&&u[j].raw_type==44&&
                (guardian_slot<0||dist2(u[i].x,u[i].y,u[j].x,u[j].y)<
                                  dist2(u[i].x,u[i].y,u[guardian_slot].x,u[guardian_slot].y)))
                guardian_slot=j;
            if(guardian_slot>=0){guardian_x=u[guardian_slot].x/16;guardian_y=u[guardian_slot].y/16;}
            fprintf(stderr,"  muta_census slot%d owner%d raw_owner%d alive%d loaded%d flags%x type%d xy%d,%d order%d target%d tx%d,%d sight_source_px%d radius%d visCenter%d nearGuardian%d guardianCell%d,%d guardianVisible%d\n",
                i,u[i].owner,u[i].raw_owner,u[i].alive,SC64_TRANSPORT_LOADED(&u[i]),u[i].flags,
                u[i].type,u[i].x,u[i].y,u[i].order,u[i].target,u[i].tx,u[i].ty,
                sc64_actor_unit_sight_pixels(u[i].type),
                sc64_actor_unit_sight_pixels(u[i].type)/2,
                currently_visible(u[i].x,u[i].y),guardian_slot,
                guardian_slot>=0?u[guardian_slot].x:-1,guardian_slot>=0?u[guardian_slot].y:-1,
                guardian_slot>=0?currently_visible(u[guardian_slot].x,u[guardian_slot].y):0);
            for(int dy=-2;dy<=2;dy++) {
                fprintf(stderr,"    muta_vis_row%d",dy);
                for(int dx=-2;dx<=2;dx++) {
                    int col=cell_x+dx,row=cell_y+dy,cell=col+row*MAP_COLS;
                    fprintf(stderr," %d",col>=0&&row>=0&&col<MAP_COLS&&row<MAP_ROWS?
                        SC64_GRID_GET(visible,cell):0);
                }
                fputc('\n',stderr);
            }
            if(guardian_slot>=0)for(int dy=-2;dy<=2;dy++) {
                fprintf(stderr,"    guardian_vis_row%d",dy);
                for(int dx=-2;dx<=2;dx++) {
                    int col=guardian_x+dx,row=guardian_y+dy,cell=col+row*MAP_COLS;
                    fprintf(stderr," %d",col>=0&&row>=0&&col<MAP_COLS&&row<MAP_ROWS?
                        SC64_GRID_GET(visible,cell):0);
                }
                fputc('\n',stderr);
            }
        }
    }
}

static void step(u16 held);
static void tap(u16 key) { step(key); step(0); }

static void trace_input(u16 held,u16 pressed)
{
    if(!trace_file||held==prior_keys)return;
    fprintf(trace_file,"%s{\"kind\":\"input\",\"host_tick\":%u,\"held\":%u,\"pressed\":%u}",
        trace_first?"":",",host_ticks,(unsigned)held,(unsigned)pressed);
    trace_first=0;
}

static void step(u16 held)
{
    if(screen==SCREEN_RESULT||won||lost)return;
    u16 pressed=(u16)(held&(u16)~prior_keys);
    int drone_hp[MAX_UNITS];
    int drone_order[MAX_UNITS];
    int builder_order[MAX_UNITS],builder_target[MAX_UNITS];
    int builder_x[MAX_UNITS],builder_y[MAX_UNITS];
    int build_progress[MAX_BUILDINGS],build_hp[MAX_BUILDINGS],build_uc[MAX_BUILDINGS];
    int defender_hp[MAX_UNITS];
    int defender_raw[MAX_UNITS];
    int guardian_hp_before[MAX_UNITS];
    for(int i=0;i<MAX_UNITS;i++)guardian_hp_before[i]=
        u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==44?u[i].hp:-1;
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41) {
            drone_hp[i]=u[i].hp;drone_order[i]=u[i].order;
        } else {drone_hp[i]=-1;drone_order[i]=-1;}
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41) {
        builder_order[i]=u[i].order;builder_target[i]=u[i].target;
        builder_x[i]=u[i].x;builder_y[i]=u[i].y;
    } else {builder_order[i]=-1;builder_target[i]=-1;builder_x[i]=builder_y[i]=-1;}
    for(int i=0;i<MAX_BUILDINGS;i++) {
        build_progress[i]=b[i].build_progress;build_hp[i]=b[i].hp;build_uc[i]=b[i].under_construction;
    }
    for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&
           (u[i].type==U_HYDRALISK||u[i].type==U_MUTALISK)) {
            defender_hp[i]=u[i].hp;defender_raw[i]=u[i].raw_type;
        } else {defender_hp[i]=-1;defender_raw[i]=-1;}
    game_update(held,pressed);
    if((unsigned)frame>=11200u&&(unsigned)frame<=13200u) {
        int observed=0;
        for(int i=0;i<MAX_UNITS;i++)if(guardian_hp_before[i]>=0&&u[i].alive&&
           u[i].owner==ENEMY&&u[i].raw_type==44&&u[i].hp<guardian_hp_before[i]) {
            fprintf(stderr,"Z7 COMBAT_HIT frame%u guardian%d hp%d->%d delta%d xy%d,%d\n",
                (unsigned)frame,i,guardian_hp_before[i],u[i].hp,
                guardian_hp_before[i]-u[i].hp,u[i].x,u[i].y);
            observed=1;
        }
        if(observed||(frame%20u)==0u) {
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==43) {
                int d=-1,target=-1;
                for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==ENEMY&&u[j].raw_type==44&&
                    (d<0||dist2(u[i].x,u[i].y,u[j].x,u[j].y)<d)) {
                    d=dist2(u[i].x,u[i].y,u[j].x,u[j].y);target=j;
                }
                fprintf(stderr,"Z7 COMBAT_MUTA frame%u slot%d hp%d xy%d,%d order%d target%d to%d,%d cd%d visible%d guardian%d d2%d range%d\n",
                    (unsigned)frame,i,u[i].hp,u[i].x,u[i].y,u[i].order,u[i].target,
                    u[i].tx,u[i].ty,u[i].cd,currently_visible(u[i].x,u[i].y),target,d,
                    (sc64_race_unit_range_vs(u[i].type,1)+1)/2);
            }
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==44&&
                currently_visible(u[i].x,u[i].y))fprintf(stderr,
                "Z7 COMBAT_GUARDIAN frame%u slot%d hp%d xy%d,%d order%d target%d cd%d visible1\n",
                (unsigned)frame,i,u[i].hp,u[i].x,u[i].y,u[i].order,u[i].target,u[i].cd);
        }
    }
    /* Read-only resource-return diagnostic for the early normal-input window.
       This observes the live worker state after an update and never steers it. */
    if(frame<8000) {
        static int carry_before[MAX_UNITS], carry_seen[MAX_UNITS];
        static unsigned last_worker_report[MAX_UNITS];
        static int worker_report_x[MAX_UNITS], worker_report_y[MAX_UNITS];
        for(int i=0;i<MAX_UNITS;i++) {
            if(!u[i].alive||u[i].owner!=SIDE||u[i].raw_type!=41) continue;
            if(!carry_seen[i]) {
                carry_before[i]=u[i].carry; carry_seen[i]=1;
                worker_report_x[i]=u[i].x; worker_report_y[i]=u[i].y;
                continue;
            }
            if(carry_before[i]!=u[i].carry) {
                int home=-1,hd=0x7fffffff;
                for(int j=0;j<MAX_BUILDINGS;j++) if(b[j].alive&&b[j].owner==SIDE&&
                    !b[j].under_construction&&(b[j].type==B_HATCHERY||b[j].type==B_LAIR||b[j].type==B_HIVE)) {
                    int d=dist2(u[i].x,u[i].y,b[j].x,b[j].y);
                    if(d<hd){home=j;hd=d;}
                }
                fprintf(stderr,"Z7 CARRY frame%u slot%d %d->%d order%d target%d xy%d,%d depot%d xy%d,%d d2%d retry%d path%d,%d timer%d work%d minerals%d gas%d\n",
                    (unsigned)frame,i,carry_before[i],u[i].carry,u[i].order,u[i].target,u[i].x,u[i].y,
                    home,home>=0?b[home].x:-1,home>=0?b[home].y:-1,hd,u[i].path_retry,
                    u[i].path_x,u[i].path_y,u[i].path_timer,u[i].work_timer,minerals,gas);
                last_worker_report[i]=frame;
                worker_report_x[i]=u[i].x; worker_report_y[i]=u[i].y;
                carry_before[i]=u[i].carry;
            }
            if(u[i].carry>0&&frame-last_worker_report[i]>=120u) {
                int home=-1,hd=0x7fffffff;
                for(int j=0;j<MAX_BUILDINGS;j++) if(b[j].alive&&b[j].owner==SIDE&&
                    !b[j].under_construction&&(b[j].type==B_HATCHERY||b[j].type==B_LAIR||b[j].type==B_HIVE)) {
                    int d=dist2(u[i].x,u[i].y,b[j].x,b[j].y);
                    if(d<hd){home=j;hd=d;}
                }
                fprintf(stderr,"Z7 CARRY_SAMPLE frame%u slot%d carry%d order%d target%d xy%d,%d moved2%d depot%d xy%d,%d d2%d retry%d path%d,%d timer%d work%d\n",
                    (unsigned)frame,i,u[i].carry,u[i].order,u[i].target,u[i].x,u[i].y,
                    dist2(u[i].x,u[i].y,worker_report_x[i],worker_report_y[i]),
                    home,home>=0?b[home].x:-1,home>=0?b[home].y:-1,hd,u[i].path_retry,
                    u[i].path_x,u[i].path_y,u[i].path_timer,u[i].work_timer);
                last_worker_report[i]=frame;
                worker_report_x[i]=u[i].x; worker_report_y[i]=u[i].y;
            }
        }
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41)
            carry_before[i]=u[i].carry;
    }
    if(frame>=7000&&frame<12000&&frame%100u==0u) {
        fprintf(stderr,"DEF43_MUTA host%u frame%u scout%d attackSent%d extractorFound%d secured%d target%d,%d guard%d focus%d localAir%d\n",
            host_ticks,(unsigned)frame,geyser_scout_ordered,geyser_attack_sent,
            geyser_visible_extractor_found,geyser_secured,geyser_target_x,geyser_target_y,
            muta_guard_set,focused_muta_slot,local_air_target_slot);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==43)
            fprintf(stderr,"  OWN_MUTA slot%d xy%d,%d hp%d order%d target%d tx%d,%d cd%d selected%d visible%d\n",
                i,u[i].x,u[i].y,u[i].hp,u[i].order,u[i].target,u[i].tx,u[i].ty,u[i].cd,
                u[i].selected,currently_visible(u[i].x,u[i].y));
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&
            b[i].raw_owner==4&&b[i].raw_type==149&&currently_visible(b[i].x,b[i].y))
            fprintf(stderr,"  VISIBLE_EXTRACTOR slot%d xy%d,%d hp%d under%d progress%d\n",
                i,b[i].x,b[i].y,b[i].hp,b[i].under_construction,b[i].build_progress);
    }
    if(frame>=800&&frame<=5000) {
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&
           (build_uc[i]||b[i].under_construction)&&
           (build_progress[i]!=b[i].build_progress||build_uc[i]!=b[i].under_construction)&&
           (build_progress[i]<0||b[i].build_progress/60!=build_progress[i]/60||
            build_uc[i]!=b[i].under_construction))
            fprintf(stderr,"Z7 build progress host%u frame%u building%d raw%d hp%d->%d progress%d->%d under%d->%d\n",
                host_ticks,(unsigned)frame,i,b[i].raw_type,build_hp[i],b[i].hp,
                build_progress[i],b[i].build_progress,build_uc[i],b[i].under_construction);
        for(int i=0;i<MAX_UNITS;i++)if(builder_order[i]>=0&&
           (u[i].order!=builder_order[i]||u[i].target!=builder_target[i]||
            (frame%120u==0u&&u[i].order==ORDER_BUILD)))
            fprintf(stderr,"Z7 builder state host%u frame%u worker%d order%d->%d target%d->%d xy%d,%d->%d,%d targetAlive%d distance2%d screen%d mode%d menu%d cursor%d,%d selected%d\n",
                host_ticks,(unsigned)frame,i,builder_order[i],u[i].order,builder_target[i],u[i].target,
                builder_x[i],builder_y[i],u[i].x,u[i].y,
                u[i].target>=0&&u[i].target<MAX_BUILDINGS?b[u[i].target].alive:0,
                u[i].target>=0&&u[i].target<MAX_BUILDINGS?dist2(u[i].x,u[i].y,b[u[i].target].x,b[u[i].target].y):-1,
                screen,mode,menu_open,cx,cy,selected_count());
    }
    if(frame>=5400&&frame<=14000)for(int i=0;i<MAX_UNITS;i++)
        if(drone_hp[i]>=0&&drone_order[i]==ORDER_MOVE&&u[i].alive&&
           u[i].owner==SIDE&&u[i].raw_type==41&&u[i].order==ORDER_MINE)
            fprintf(stderr,"Z7 move-to-mine transition host%u frame%u slot%d input%u/%u pos%d,%d tx%d,%d target%d\n",
                host_ticks,(unsigned)frame,i,(unsigned)held,(unsigned)pressed,
                u[i].x,u[i].y,u[i].tx,u[i].ty,u[i].target);
    if(frame>=5400&&frame<=14000)for(int i=0;i<MAX_UNITS;i++)
        if(drone_hp[i]>=0&&(!u[i].alive||u[i].hp<drone_hp[i])) {
            if(!u[i].alive)++drone_deaths;
            fprintf(stderr,"Z7 drone damage host%u frame%u slot%d hp%d->%d alive%d xy%d,%d order%d target%d\n",
                host_ticks,(unsigned)frame,i,drone_hp[i],u[i].hp,u[i].alive,
                u[i].x,u[i].y,u[i].order,u[i].target);
            for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==ENEMY&&
                (u[j].target==i||dist2(u[j].x,u[j].y,u[i].x,u[i].y)<320*320))
                fprintf(stderr,"  damage-source? slot%d raw%d type%d xy%d,%d hp%d order%d target%d cd%d dist2%d\n",
                    j,u[j].raw_type,u[j].type,u[j].x,u[j].y,u[j].hp,u[j].order,u[j].target,u[j].cd,
                    dist2(u[j].x,u[j].y,u[i].x,u[i].y));
        }
    if(frame>=9000&&frame<=16000)for(int i=0;i<MAX_UNITS;i++)
        if(defender_hp[i]>=0&&u[i].alive&&u[i].hp<defender_hp[i]) {
            fprintf(stderr,"Z7 defender damage host%u frame%u slot%d raw%d type%d hp%d->%d xy%d,%d order%d target%d cd%d\n",
                host_ticks,(unsigned)frame,i,defender_raw[i],u[i].type,defender_hp[i],u[i].hp,
                u[i].x,u[i].y,u[i].order,u[i].target,u[i].cd);
            for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==ENEMY&&
                dist2(u[j].x,u[j].y,u[i].x,u[i].y)<192*192)
                fprintf(stderr,"  nearby hostile slot%d raw%d type%d hp%d xy%d,%d order%d target%d cd%d d2%d\n",
                    j,u[j].raw_type,u[j].type,u[j].hp,u[j].x,u[j].y,u[j].order,u[j].target,u[j].cd,
                    dist2(u[j].x,u[j].y,u[i].x,u[i].y));
        }
    ++host_ticks;
    trace_input(held,pressed);
    prior_keys=held;
}

static void move_cursor(int x,int y)
{
    unsigned guard=0;
    x=limit(cx+((x-cx)>=0?((x-cx+2)/4):((x-cx-2)/4))*4,0,WORLD_W-1);
    y=limit(cy+((y-cy)>=0?((y-cy+2)/4):((y-cy-2)/4))*4,0,WORLD_H-1);
    while((cx!=x||cy!=y)&&screen==SCREEN_PLAY&&!won&&!lost&&guard++<6000u) {
        u16 held=KEY_R;
        if(cx<x)held|=KEY_RIGHT; if(cx>x)held|=KEY_LEFT;
        if(cy<y)held|=KEY_DOWN; if(cy>y)held|=KEY_UP;
        step(held);
    }
    if(guard>=6000u&&(cx!=x||cy!=y))
        fprintf(stderr,"Z7 cursor move stalled from %d,%d to %d,%d at %d,%d mode%d menuopen%d frame%u host%u\n",
            x,y,cx,cy,cx,cy,mode,menu_open,(unsigned)frame,host_ticks);
    step(0);
}

static int box_select(int left,int top,int right,int bottom)
{
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    move_cursor(left,top); step(KEY_L);
    unsigned guard=0;
    while((cx<right||cy<bottom)&&screen==SCREEN_PLAY&&!won&&!lost&&guard++<6000u) {
        u16 held=KEY_L;
        if(cx<right)held|=KEY_RIGHT;
        if(cy<bottom)held|=KEY_DOWN;
        step(held);
    }
    step(0);
    return selected_count();
}

static int cursor_lattice_coordinate(int current,int target,int upper);

static int local_rect_empty_for_selection(int left,int top,int right,int bottom)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&
        sc64_game_unit_selectable(u[i].type)) {
        int dx=u[i].x<left?left-u[i].x:(u[i].x>right?u[i].x-right:0);
        int dy=u[i].y<top?top-u[i].y:(u[i].y>bottom?u[i].y-bottom:0);
        int margin=sc64_game_unit_collision_radius(u[i].type)+8;
        if(dx*dx+dy*dy<=margin*margin)return 0;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&
        sc64_game_building_selectable(b[i].type)) {
        int dx=b[i].x<left?left-b[i].x:(b[i].x>right?b[i].x-right:0);
        int dy=b[i].y<top?top-b[i].y:(b[i].y>bottom?b[i].y-bottom:0);
        /* Buildings can be selected by their sprite body, so keep a broad
           empty margin instead of relying on center-point separation. */
        if(dx*dx+dy*dy<=72*72)return 0;
    }
    return 1;
}

/* Clear a stale selection with an ordinary small box near the current cursor.
   Every candidate is screened against live selectable actors, then the actual
   gesture must verify both unit and building selection are empty. */
static int deselect_with_local_box(void)
{
    if(selected_count()==0&&selected_building<0)return 1;
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    for(int radius=0;radius<=256;radius+=16)
        for(int dy=-radius;dy<=radius;dy+=16)
            for(int dx=-radius;dx<=radius;dx+=16) {
                int ax=dx<0?-dx:dx,ay=dy<0?-dy:dy;
                if(radius&&(ax>ay?ax:ay)!=radius)continue;
                int left=cursor_lattice_coordinate(cx,cx+dx,WORLD_W);
                int top=cursor_lattice_coordinate(cy,cy+dy,WORLD_H);
                int right=left+16,bottom=top+16;
                if(right>=WORLD_W||bottom>=WORLD_H||
                   !local_rect_empty_for_selection(left,top,right,bottom))continue;
                (void)box_select(left,top,right,bottom);
                if(selected_count()==0&&selected_building<0) {
                    fprintf(stderr,"Z7 local deselect verified rect%d,%d-%d,%d cursor%d,%d frame%u\n",
                        left,top,right,bottom,cx,cy,(unsigned)frame);
                    return 1;
                }
                fprintf(stderr,"Z7 local deselect candidate not empty rect%d,%d-%d,%d units%d building%d frame%u\n",
                    left,top,right,bottom,selected_count(),selected_building,(unsigned)frame);
            }
    fprintf(stderr,"Z7 local deselect failed cursor%d,%d units%d building%d frame%u\n",
        cx,cy,selected_count(),selected_building,(unsigned)frame);
    return 0;
}

static int choose_menu_row(int row)
{
    if(!menu_open)tap(KEY_B);
    int count=field_menu_count();
    if(!menu_open||row<0||row>=count) {
        if(menu_open)tap(KEY_B);
        return 0;
    }
    for(int i=0;i<count&&menu_cursor!=row&&screen==SCREEN_PLAY;i++)tap(KEY_DOWN);
    if(menu_cursor!=row) { if(menu_open)tap(KEY_B); return 0; }
    tap(KEY_A);
    return 1;
}

static void cancel_ui_mode(void)
{
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    if(menu_open)tap(KEY_B);
}

static int raw_unit(int raw,int owner)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_type==raw&&
        (owner<0||u[i].owner==owner))return i;
    return -1;
}

static int raw_building(int raw,int owner)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].raw_type==raw&&
        (owner<0||b[i].owner==owner))return i;
    return -1;
}

static int z7_townhall(int owner)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&
        (owner<0||b[i].owner==owner)&&sc64_race_is_zerg_townhall(b[i].type))return i;
    return -1;
}

static int owned_townhall_count(void)
{
    int n=0;
    for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&
        sc64_race_is_zerg_townhall(b[i].type);
    return n;
}

static int unit_count(int raw,int owner)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&
        (raw<0||u[i].raw_type==raw)&&(owner<0||u[i].owner==owner);
    return n;
}

static int unit_type_count(int type,int owner)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].type==type&&
        (owner<0||u[i].owner==owner);
    return n;
}

static int building_count(int owner)
{
    int n=0;
    for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&(owner<0||b[i].owner==owner);
    return n;
}

static int combat_count(int owner)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==owner&&
        u[i].type>=0&&u[i].type<SC_RU_COUNT&&
        !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
        sc64_race_unit_damage(u[i].type)>0)n++;
    return n;
}

static int worker_count(int order)
{
    int n=0;
    for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type==41&&(order<0||u[i].order==order);
    return n;
}

static int z7_threat_near_workers(int enemy,int radius)
{
    if(enemy<0||enemy>=MAX_UNITS||!u[enemy].alive)return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41&&
        dist2(u[enemy].x,u[enemy].y,u[i].x,u[i].y)<=radius*radius)return 1;
    return 0;
}

static int queued_unit_count(int type)
{
    int n=0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE)
        for(int j=0;j<b[i].queue_count;j++)
            n+=b[i].queue[(b[i].queue_head+j)%5]==type;
#ifdef SC64_Z7_EGG_PRODUCTION
    /* Eggs are independent paid jobs, not producer-queue entries. Count their
       eventual outputs so macro decisions do not buy a duplicate while the
       product is incubating. */
    if(race_z7_active())for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_Z7_EGG&&
           u[i].target==type&&sc64_z7_egg_product_valid(type))
            n+=sc64_z7_egg_product_count(type);
#endif
    return n;
}

static int gas_worker_count(void)
{
    return worker_count(ORDER_GAS);
}

static int owned_building_count(int type)
{
    int n=0;
    for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&b[i].type==type;
    return n;
}

static int completed_building_count(int type)
{
    int n=0;
    for(int i=0;i<MAX_BUILDINGS;i++)n+=b[i].alive&&b[i].owner==SIDE&&
        b[i].type==type&&building_complete(i);
    return n;
}

/* move_cursor moves in four-pixel steps relative to the current cursor
   lattice. Candidate placement must be checked at the coordinate the input
   can actually reach, rather than at the ideal point before quantization. */
static int cursor_lattice_coordinate(int current,int target,int upper)
{
    int delta=target-current;
    int steps=delta>=0?(delta+2)/4:(delta-2)/4;
    return limit(current+steps*4,0,upper-1);
}

/* Choose a nearby cursor lattice point whose ordinary A click selects this
   Drone rather than a neighboring combat unit or building. Dense resource
   lines otherwise make a mathematically correct center click ambiguous. */
static int worker_select_point(int slot,int *out_x,int *out_y)
{
    if(slot<0||slot>=MAX_UNITS)return 0;
    for(int radius=0;radius<=20;radius+=4)
        for(int oy=-radius;oy<=radius;oy+=4)
            for(int ox=-radius;ox<=radius;ox+=4) {
                if(radius&&ox!=-radius&&ox!=radius&&oy!=-radius&&oy!=radius)continue;
                int x=cursor_lattice_coordinate(cx,u[slot].x+ox,WORLD_W);
                int y=cursor_lattice_coordinate(cy,u[slot].y+oy,WORLD_H);
                int best_unit=-1,best_unit_d=31*31;
                for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&
                    !SC64_TRANSPORT_LOADED(&u[j])&&
#ifdef SC64_RACES
                    u[j].type!=U_CHRYSALIS&&
#endif
                    sc64_game_unit_selectable(u[j].type)) {
                    int d=dist2(x,y,u[j].x,u[j].y);
                    if(d<best_unit_d){best_unit_d=d;best_unit=j;}
                }
                int best_building=-1,best_building_d=26*26;
                for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&
                    sc64_game_building_selectable(b[j].type)) {
                    int d=dist2(x,y,b[j].x,b[j].y);
                    if(d<best_building_d){best_building_d=d;best_building=j;}
                }
                if(best_unit==slot&&(best_building<0||best_unit_d<best_building_d)) {
                    *out_x=x;*out_y=y;return 1;
                }
            }
    return 0;
}

/* A same-type double tap need not isolate a particular actor: any ordinary
   click on the intended type is a valid seed. This matters for dense moving
   groups where no cursor lattice point can isolate one unit from its peers. */
static int type_select_point(int type,int *out_x,int *out_y)
{
    int preferred=-1,best_count=-1,best_distance=0x7fffffff;
    if(select_mutalisk_cluster_for_scout&&type==U_MUTALISK) {
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
            u[i].type==type&&!SC64_TRANSPORT_LOADED(&u[i])&&
            sc64_game_unit_selectable(u[i].type)) {
            int left=limit(u[i].x-120,0,WORLD_W-240)&~3;
            int top=limit(u[i].y-54,0,WORLD_H-108);
            int count=0;
            for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&
                u[j].type==type&&!SC64_TRANSPORT_LOADED(&u[j])&&
                u[j].x>=left&&u[j].x<left+240&&u[j].y>=top&&u[j].y<top+108)
                ++count;
            int d=dist2(cx,cy,u[i].x,u[i].y);
            if(count>best_count||(count==best_count&&d<best_distance)) {
                preferred=i;best_count=count;best_distance=d;
            }
        }
    }
    for(int slot=0;slot<MAX_UNITS;slot++)if(u[slot].alive&&u[slot].owner==SIDE&&
        (preferred<0||slot==preferred)&&
        u[slot].type==type&&!SC64_TRANSPORT_LOADED(&u[slot])&&
        sc64_game_unit_selectable(u[slot].type)) {
        for(int radius=0;radius<=20;radius+=4)
            for(int oy=-radius;oy<=radius;oy+=4)
                for(int ox=-radius;ox<=radius;ox+=4) {
                    if(radius&&ox!=-radius&&ox!=radius&&oy!=-radius&&oy!=radius)continue;
                    int x=cursor_lattice_coordinate(cx,u[slot].x+ox,WORLD_W);
                    int y=cursor_lattice_coordinate(cy,u[slot].y+oy,WORLD_H);
                    int best_unit=-1,best_unit_d=31*31;
                    for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&
                        !SC64_TRANSPORT_LOADED(&u[j])&&
#ifdef SC64_RACES
                        u[j].type!=U_CHRYSALIS&&
#endif
                        sc64_game_unit_selectable(u[j].type)) {
                        int d=dist2(x,y,u[j].x,u[j].y);
                        if(d<best_unit_d){best_unit_d=d;best_unit=j;}
                    }
                    int best_building=-1,best_building_d=26*26;
                    for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&
                        sc64_game_building_selectable(b[j].type)) {
                        int d=dist2(x,y,b[j].x,b[j].y);
                        if(d<best_building_d){best_building_d=d;best_building=j;}
                    }
                    if(best_unit>=0&&u[best_unit].type==type&&
                       (best_building<0||best_unit_d<best_building_d)) {
                        *out_x=x;*out_y=y;return 1;
                    }
                }
    }
    return 0;
}

static int select_worker(int slot)
{
    if(slot<0||slot>=MAX_UNITS||!u[slot].alive||u[slot].owner!=SIDE||
       u[slot].raw_type!=41)return 0;
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    /* A normal A click on a unit clears the prior unit selection itself.
       Avoid a full-map box-selection detour: its live cursor travel lets
       miners finish a retreat and get autoassigned before the next order. */
    if(selected_building>=0&&!deselect_with_local_box())return 0;
    /* A miner can travel several dozen pixels while the cursor crosses the
       map. Recompute an isolated click point as we approach so the final A
       press uses the Drone's current position, not the position observed at
       the start of cursor travel. */
    int x=u[slot].x,y=u[slot].y;
    for(int attempt=0;attempt<5;attempt++) {
        if(!u[slot].alive||u[slot].owner!=SIDE||u[slot].raw_type!=41)return 0;
        x=u[slot].x;y=u[slot].y;
        (void)worker_select_point(slot,&x,&y);
        move_cursor(x,y);
        int dx=u[slot].x-cx,dy=u[slot].y-cy;
        if(dx*dx+dy*dy<=16*16)break;
    }
    tap(KEY_A);
    return selected_count()==1&&u[slot].selected;
}

static int select_building_slot(int slot)
{
    if(slot<0||slot>=MAX_BUILDINGS||!b[slot].alive||b[slot].owner!=SIDE)return 0;
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    /* A selected nonproducer building makes A clicks queue at that old
       building. Clear building selection through the ordinary box gesture
       before selecting another building. */
    if((selected_building>=0||selected_count())&&!deselect_with_local_box())return 0;
    move_cursor(b[slot].x,b[slot].y);tap(KEY_A);
    return selected_building==slot;
}

static int base_center(int *x,int *y)
{
    int sx=0,sy=0,n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].raw_owner==3&&u[i].owner==SIDE) {
        sx+=u[i].x;sy+=u[i].y;++n;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE) {
        sx+=b[i].x;sy+=b[i].y;++n;
    }
    if(!n)return 0;
    *x=sx/n;*y=sy/n;return 1;
}

static int z7_worker_can_reach_site(int x,int y)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
       u[i].raw_type==41&&u[i].order!=ORDER_BUILD) {
        int nx=-1,ny=-1;
        /* start_building chooses the first available Drone, so qualify that
           exact actor instead of accepting a site reachable only by another. */
        return path_next_tile(u[i].x,u[i].y,x,y,&nx,&ny);
    }
    return 0;
}

static int find_build_site(int type,int *out_x,int *out_y)
{
    int bx,by,best=0x7fffffff,found=0;
    if(!out_x||!out_y)return 0;
    if(!base_center(&bx,&by)){bx=cx;by=cy;}
    /* The initial four Drones start well south of their nearest mineral
       field. Move the first Hatchery toward that field without placing it
       on the resource-cluster edge; live placement still decides legality. */
    if(type==B_HATCHERY) {
        if(hatch_probe_x>=0&&hatch_probe_y>=0&&!owned_townhall_count()) {
            int px=cursor_lattice_coordinate(cx,hatch_probe_x,WORLD_W);
            int py=cursor_lattice_coordinate(cy,hatch_probe_y,WORLD_H);
            int unsafe=-1;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
               currently_visible(u[i].x,u[i].y)&&
               SC64_CAN_ATTACK_FROM_SOURCE(u[i].type,B_HATCHERY,u[i].owner,u[i].raw_owner)&&
               dist2(px,py,u[i].x,u[i].y)<160*160) {unsafe=i;break;}
            if(unsafe>=0) {
                static unsigned last_hatch_safety_log;
                if(!last_hatch_safety_log||frame-last_hatch_safety_log>=600u) {
                    fprintf(stderr,"Z7 near-mineral Hatch wait frame%u site%d,%d visibleAttacker%d raw%d xy%d,%d d2%d\n",
                        (unsigned)frame,px,py,unsafe,u[unsafe].raw_type,u[unsafe].x,u[unsafe].y,
                        dist2(px,py,u[unsafe].x,u[unsafe].y));
                    last_hatch_safety_log=frame;
                }
                return 0;
            }
            *out_x=hatch_probe_x;*out_y=hatch_probe_y;return 1;
        }
        if(owned_townhall_count()) {
            for(int p=0;p<MAX_MINERALS;p++)if(m[p].amount>0) {
                int px=m[p].x,py=m[p].y,near_base=0x7fffffff;
                for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
                    sc64_race_is_zerg_townhall(b[i].type)) {
                    int d=dist2(px,py,b[i].x,b[i].y);if(d<near_base)near_base=d;
                }
                if(near_base<208*208)continue;
                for(int y=py-160;y<=py+160;y+=16)for(int x=px-160;x<=px+160;x+=16) {
                    if(x<16||y<20||x>=WORLD_W-16||y>=WORLD_H-16)continue;
                    int d=dist2(x,y,px,py),threat=0x7fffffff;
                    if(d<48*48||d>160*160)continue;
                    if(!race_can_site(SIDE,type,x,y)||!can_place(type,x,y)||
                       !z7_worker_can_reach_site(x,y))continue;
                    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY) {
                        int enemy_d=dist2(x,y,u[i].x,u[i].y);if(enemy_d<threat)threat=enemy_d;
                    }
                    if(threat<144*144)continue;
                    int score=near_base+d*2-threat/8;
                    if(score<best){best=score;*out_x=x;*out_y=y;found=1;}
                }
            }
            return found;
        }
        int patch=-1,patch_d=0x7fffffff;
        for(int i=0;i<MAX_MINERALS;i++)if(m[i].amount>0) {
            int d=dist2(bx,by,m[i].x,m[i].y);
            if(d<patch_d){patch=i;patch_d=d;}
        }
        if(patch>=0) {
            int px=m[patch].x,py=m[patch].y;
            best=0x7fffffff;found=0;
            for(int y=py-192;y<=py+192;y+=8)
                for(int x=px-192;x<=px+192;x+=8) {
                    if(x<16||y<20||x>=WORLD_W-16||y>=WORLD_H-16)continue;
                    int sx=(x+2)&~3,sy=(y+2)&~3;
                    int d=dist2(sx,sy,px,py),threat=0x7fffffff;
                    if(d<64*64||d>192*192)continue;
                    if(!race_can_site(SIDE,type,sx,sy)||!can_place(type,sx,sy)||
                       !z7_worker_can_reach_site(sx,sy))continue;
                    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY) {
                        int enemy_d=dist2(sx,sy,u[i].x,u[i].y);
                        if(enemy_d<threat)threat=enemy_d;
                    }
                    /* The staged source army includes a raw37 at the old
                       midpoint site. Avoid a 1HP Zerg foundation being
                       attacked on the same update it is placed. */
                    if(threat<120*120)continue;
                    int base_d=dist2(sx,sy,bx,by);
                    int score=d*2+base_d;
                    if(score<best){best=score;*out_x=sx;*out_y=sy;found=1;}
                }
            if(found)return 1;
        }
    }
    if(type==B_SPAWNING_POOL&&hatch_probe_x>=0&&hatch_probe_y>=0) {
        int hatch=z7_townhall(SIDE);
        if(hatch>=0) {
            int hx=b[hatch].x,hy=b[hatch].y,worker=-1;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
               u[i].raw_type==41&&u[i].order!=ORDER_BUILD){worker=i;break;}
            if(worker>=0) {
                int best_score=0x7fffffff,site_x=0,site_y=0;
                for(int dy=80;dy<=176;dy+=8)for(int dx=-80;dx<=80;dx+=8) {
                    int x=cursor_lattice_coordinate(cx,hx+dx,WORLD_W);
                    int y=cursor_lattice_coordinate(cy,hy+dy,WORLD_H);
                    if(x<24||y<24||x>=WORLD_W-24||y>=WORLD_H-24||
                       !race_can_site(SIDE,type,x,y)||!can_place(type,x,y)||
                       !z7_worker_can_reach_site(x,y))continue;
                    int patch_clear=1;
                    for(int p=0;p<MAX_MINERALS;p++)if(m[p].amount>0&&
                       dist2(x,y,m[p].x,m[p].y)<112*112){patch_clear=0;break;}
                    if(!patch_clear)continue;
                    int score=abs(dy-120)*8+abs(dx)+dist2(x,y,u[worker].x,u[worker].y)/64;
                    if(score<best_score){best_score=score;site_x=x;site_y=y;}
                }
                if(best_score<0x7fffffff) {
                    *out_x=site_x;*out_y=site_y;
                    fprintf(stderr,"Z7 near-mineral Pool site%d,%d south of Hatch%d,%d frame%u\n",
                        site_x,site_y,hx,hy,(unsigned)frame);
                    return 1;
                }
            }
        }
    }
    /* Keep the first colony and its in-place Spore morph inside the defended
       base footprint. The generic base-center search can average the initial
       army westward and choose a site close to the source Zergling pack. */
    if(type==B_CREEP_COLONY&&owned_building_count(B_SUNKEN_COLONY)==0) {
        int hatch=z7_townhall(SIDE);
        if(hatch>=0) {
            if(z7_front_colony_site(hatch,out_x,out_y))return 1;
            int hx=b[hatch].x,hy=b[hatch].y,best_score=0x7fffffff;
            int site_x=0,site_y=0;
            for(int dy=-160;dy<=160;dy+=16)for(int dx=-160;dx<=160;dx+=16) {
                int x=cursor_lattice_coordinate(cx,hx+dx,WORLD_W);
                int y=cursor_lattice_coordinate(cy,hy+dy,WORLD_H);
                if(x<24||y<24||x>=WORLD_W-24||y>=WORLD_H-24||
                   dist2(x,y,hx,hy)<64*64||!race_can_site(SIDE,type,x,y)||
                       !can_place(type,x,y)||!z7_worker_can_reach_site(x,y))continue;
                int nearest=0x7fffffff,reachable=0;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY) {
                    int d=dist2(x,y,u[i].x,u[i].y);if(d<nearest)nearest=d;
                }
                if(nearest<144*144)continue;
                for(int i=0;i<MAX_UNITS&&!reachable;i++)if(u[i].alive&&
                    u[i].owner==SIDE&&u[i].raw_type==41&&u[i].order!=ORDER_BUILD) {
                    int nx,ny;
                    reachable=path_next_tile(u[i].x,u[i].y,x,y,&nx,&ny);
                }
                if(!reachable)continue;
                int score=dist2(x,y,hx,hy)-nearest/2;
                if(score<best_score){best_score=score;site_x=x;site_y=y;found=1;}
            }
            if(found){*out_x=site_x;*out_y=site_y;return 1;}
        }
    }
    /* Put the second Sunken Colony behind the first defense, south of the
       Hatchery. This remains a normal legal site search; prefer buildable
       cells with the greatest current enemy clearance on the first ring. */
    if(type==B_CREEP_COLONY&&owned_building_count(B_SUNKEN_COLONY)>0) {
        int hatch=z7_townhall(SIDE);
        if(hatch>=0) {
            if(z7_front_colony_site(hatch,out_x,out_y))return 1;
            int hx=b[hatch].x,hy=b[hatch].y;
            for(int radius=96;radius<=192;radius+=16) {
                int danger_best=-1,site_x=0,site_y=0;
                for(int dx=-96;dx<=96;dx+=16) {
                    int x=cursor_lattice_coordinate(cx,hx+dx,WORLD_W);
                    int y=cursor_lattice_coordinate(cy,hy+radius,WORLD_H);
                    if(x<16||y<20||x>=WORLD_W-16||y>=WORLD_H-16||
                       !race_can_site(SIDE,type,x,y)||!can_place(type,x,y)||
                       !z7_worker_can_reach_site(x,y))continue;
                    int nearest=0x7fffffff;
                    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY) {
                        int d=dist2(x,y,u[i].x,u[i].y);if(d<nearest)nearest=d;
                    }
                    if(nearest>danger_best){danger_best=nearest;site_x=x;site_y=y;}
                }
                if(danger_best>=0){*out_x=site_x;*out_y=site_y;return 1;}
            }
        }
    }
    if(type==B_EXTRACTOR) {
        /* Prefer Z7's recovered raw-149 node over its distant baseline node. */
        int home=z7_townhall(SIDE);
        if(home<0)return 0;
        for(int g=1;g<geyser_count;g++) {
            if(dist2(b[home].x,b[home].y,geysers[g].x,geysers[g].y)>400*400)continue;
            if(race_can_site(SIDE,type,geysers[g].x,geysers[g].y)&&
               can_place(type,geysers[g].x,geysers[g].y)&&
               z7_worker_can_reach_site(geysers[g].x,geysers[g].y)) {
                *out_x=geysers[g].x;*out_y=geysers[g].y;return 1;
            }
        }
        return 0;
    }
    /* A remote build site can leave the first selected Drone in a stationary
       BUILD order while the structure remains at 75 HP. For non-creep tech
       buildings, prefer a legal, reachable nearby tile around that exact
       first builder before using the general base-center search. */
    if(type!=B_CREEP_COLONY) {
        int worker=-1;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
           u[i].raw_type==41&&u[i].order!=ORDER_BUILD) {worker=i;break;}
        if(worker>=0)for(int radius=48;radius<=160;radius+=16) {
            int local_best=0x7fffffff,local_x=0,local_y=0;
            for(int y=u[worker].y-radius;y<=u[worker].y+radius;y+=16)
                for(int x=u[worker].x-radius;x<=u[worker].x+radius;x+=16) {
                    if(x<16||y<20||x>=WORLD_W-16||y>=WORLD_H-16)continue;
                    int d=dist2(x,y,u[worker].x,u[worker].y);
                    if(d>radius*radius||d<24*24||
                       !race_can_site(SIDE,type,x,y)||!can_place(type,x,y))continue;
                    int nx=-1,ny=-1;
                    if(!path_next_tile(u[worker].x,u[worker].y,x,y,&nx,&ny))continue;
                    if(d<local_best){local_best=d;local_x=x;local_y=y;}
                }
            if(local_best<0x7fffffff) {
                *out_x=local_x;*out_y=local_y;return 1;
            }
        }
    }
    for(int radius=48;radius<=240;radius+=32) {
        for(int y=by-radius;y<=by+radius;y+=16)
            for(int x=bx-radius;x<=bx+radius;x+=16) {
                if(x<16||y<20||x>=WORLD_W-16||y>=WORLD_H-16)continue;
                int d=dist2(x,y,bx,by);
                /* Restrict each pass to the current radius shell. */
                if(d>radius*radius)continue;
                if(!race_can_site(SIDE,type,x,y)||!can_place(type,x,y)||
                   !z7_worker_can_reach_site(x,y))continue;
                if(d<best){best=d;*out_x=x;*out_y=y;found=1;}
            }
        if(found)return 1;
    }
    return 0;
}

/* Diagnostic for the reported midpoint Hatchery loss. Include dead as well
   as live slots so a spawn-then-destroy update cannot look like a rejected
   placement. Unit.cd and target expose the hostile attack state directly. */
static void log_hatch_probe(const char *stage,int worker)
{
    fprintf(stderr,"Z7 hatch snapshot %s frame%u cursor%d,%d bank%d/%d worker%d alive%d order%d target%d buildings:",
        stage,(unsigned)frame,cx,cy,minerals,gas,worker,
        worker>=0&&worker<MAX_UNITS?u[worker].alive:0,
        worker>=0&&worker<MAX_UNITS?u[worker].order:-1,
        worker>=0&&worker<MAX_UNITS?u[worker].target:-1);
    for(int i=0;i<MAX_BUILDINGS;i++)
        fprintf(stderr," [%d a%d o%d t%d r%d/%d hp%d/%d p%d uc%d xy%d,%d]",
            i,b[i].alive,b[i].owner,b[i].type,b[i].raw_type,b[i].raw_owner,
            b[i].hp,b[i].maxhp,b[i].build_progress,b[i].under_construction,b[i].x,b[i].y);
    fputs(" enemies:",stderr);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY)
        fprintf(stderr," [%d r%d t%d xy%d,%d hp%d cd%d ord%d target%d weap%d]",
            i,u[i].raw_type,u[i].type,u[i].x,u[i].y,u[i].hp,u[i].cd,
            u[i].order,u[i].target,u[i].weapon);
    fputc('\n',stderr);
}

static int start_building(int type,int menu_row)
{
    int worker=-1,x=-1,y=-1,before=owned_building_count(type);
    if(before&&type!=B_HATCHERY&&type!=B_CREEP_COLONY)return 1;
    if(!find_build_site(type,&x,&y)) {
        if(last_build_fail_type!=type||frame-last_build_fail_frame>=600) {
            fprintf(stderr,"Z7 cannot site type%d frame%d bank%d/%d workers%d geysers%d\n",
                type,frame,minerals,gas,worker_count(-1),geyser_count);
            last_build_fail_type=type;last_build_fail_frame=frame;
        }
        return 0;
    }
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type==41&&u[i].order!=ORDER_BUILD){worker=i;break;}
    if(worker<0) {
        if(last_build_fail_type!=type||frame-last_build_fail_frame>=600) {
            fprintf(stderr,"Z7 no builder type%d frame%d workers%d bank%d/%d\n",
                type,frame,worker_count(-1),minerals,gas);
            last_build_fail_type=type;last_build_fail_frame=frame;
        }
        return 0;
    }
    int selected=0;
    for(int i=worker;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type==41&&u[i].order!=ORDER_BUILD) {
        if(select_worker(i)){worker=i;selected=1;break;}
        cancel_ui_mode();
    }
    if(!selected||!choose_menu_row(menu_row)) {
        if(last_build_fail_type!=type||frame-last_build_fail_frame>=600) {
            fprintf(stderr,"Z7 builder UI failed type%d frame%d worker%d mode%d menu%d\n",
                type,frame,worker,mode,menu);
            last_build_fail_type=type;last_build_fail_frame=frame;
        }
        cancel_ui_mode();
        return 0;
    }
    if(mode!=MODE_BUILD||menu!=type) {
        if(last_build_fail_type!=type||frame-last_build_fail_frame>=600) {
            fprintf(stderr,"Z7 wrong build menu type%d frame%d mode%d menu%d\n",type,frame,mode,menu);
            last_build_fail_type=type;last_build_fail_frame=frame;
        }
        cancel_ui_mode();
        return 0;
    }
    move_cursor(x,y);
    if(type==B_HATCHERY&&!build_probe_done)
        fprintf(stderr,"Z7 build probe before A frame%u mode%d menu%d menuopen%d cursor%d,%d selectedworker%d bank%d/%d site%d place%d\n",
            (unsigned)frame,mode,menu,menu_open,cx,cy,find_selected_worker(),minerals,gas,
            race_can_site(SIDE,type,cx,cy),can_place(type,cx,cy));
    if(!race_can_site(SIDE,type,cx,cy)||!can_place(type,cx,cy)) {
        if(last_build_fail_type!=type||frame-last_build_fail_frame>=600) {
            fprintf(stderr,"Z7 snapped site rejected type%d wanted%d,%d actual%d,%d frame%d\n",
                type,x,y,cx,cy,frame);
            last_build_fail_type=type;last_build_fail_frame=frame;
        }
        cancel_ui_mode();
        return 0;
    }
    if(type==B_HATCHERY&&!build_probe_done) {
        log_hatch_probe("pre-A",worker);
        step(KEY_A);
        fprintf(stderr,"Z7 build probe after A frame%u mode%d menu%d menuopen%d bank%d/%d worker%d alive%d order%d target%d pos%d,%d count%d\n",
            (unsigned)frame,mode,menu,menu_open,minerals,gas,worker,u[worker].alive,u[worker].order,u[worker].target,
            u[108].x,u[108].y,building_count(-1));
        log_hatch_probe("post-A",worker);
        step(0);
        fprintf(stderr,"Z7 build probe after release frame%u mode%d menuopen%d bank%d/%d worker%d alive%d order%d target%d count%d\n",
            (unsigned)frame,mode,menu_open,minerals,gas,worker,u[worker].alive,u[worker].order,u[worker].target,
            building_count(-1));
        log_hatch_probe("post-release",worker);
        build_probe_done=1;
    } else tap(KEY_A);
    int made=owned_building_count(type)>before;
    if(made){++actions_taken;fprintf(stderr,"Z7 input built raw%d at%d,%d worker%d frame%u bank%d/%d\n",
        sc64_race_raw_building(type),x,y,worker,(unsigned)frame,minerals,gas);}
    else if(last_build_fail_type!=type||frame-last_build_fail_frame>=600) {
        int selected_worker=find_selected_worker();
        const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
        fprintf(stderr,"Z7 placement rejected type%d requested%d,%d cursor%d,%d worker%d selected%d mode%d menu%d site%d place%d bank%d/%d frame%d\n",
            type,x,y,cx,cy,worker,selected_worker,mode,menu,
            race_can_site(SIDE,type,cx,cy),can_place(type,cx,cy),minerals,gas,frame);
        fprintf(stderr,"  build predicates rule%p race%d/%d workerType%d/%d slot%d cost%d/%d prereq%d\n",
            (const void*)rule,rule?rule->race:-1,race_owner_identity(SIDE),
            selected_worker>=0?u[selected_worker].type:-1,rule?rule->worker:-1,
            find_build_slot(),rule?rule->minerals:-1,rule?rule->gas:-1,
            rule?race_owner_has_prereq(SIDE,rule->prereq):0);
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE)
            fprintf(stderr,"  side building slot%d type%d xy%d,%d raw%d progress%d under%d\n",
                i,b[i].type,b[i].x,b[i].y,b[i].raw_type,b[i].build_progress,b[i].under_construction);
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner!=SIDE&&i>=112)
            fprintf(stderr,"  extra building slot%d owner%d type%d xy%d,%d raw%d progress%d under%d\n",
                i,b[i].owner,b[i].type,b[i].x,b[i].y,b[i].raw_type,
                b[i].build_progress,b[i].under_construction);
        last_build_fail_type=type;last_build_fail_frame=frame;
    }
    cancel_ui_mode();
    return made;
}

/* Start the source-authored Hatchery morph through its visible command row.
   Do not transform the building state directly: the ordinary Lair command
   owns the paid 150/100 cost and the 1,875-update morph clock. */
static int start_lair_morph(void)
{
    int hatch=z7_townhall(SIDE);
    if(hatch<0||!building_complete(hatch)||b[hatch].queue_count||
       minerals<150||gas<100||!select_building_slot(hatch))return 0;
    int accepted=choose_menu_row(MENU_ZP_LAIR)&&b[hatch].alive&&
        b[hatch].type==B_LAIR&&b[hatch].raw_type==132&&b[hatch].under_construction;
    /* The queue menu leaves the Hatchery selected; clear it with the same
       local box gesture used by other ordinary controller commands. */
    cancel_ui_mode();
    if(accepted)fprintf(stderr,"Z7 normal-input paid Lair morph accepted hatch%d frame%u progress%d bank%d/%d\n",
        hatch,(unsigned)frame,b[hatch].build_progress,minerals,gas);
    return accepted;
}

static int z7_scourge_tech_pending(void)
{
    return frame<15000&&scourge_outputs_ordered<6;
}

static int z7_spore_morph_slot(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].raw_type==143&&b[i].type==B_CREEP_COLONY&&b[i].under_construction&&
        (b[i].flags&SC64_Z7_SPORE_MORPH_FLAG))return i;
    return -1;
}

static int z7_fresh_creep_colony(void)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_CREEP_COLONY&&b[i].raw_type==143&&
        !(b[i].flags&(SC64_Z7_SPORE_MORPH_FLAG|SC64_Z7_SUNKEN_MORPH_FLAG)))return i;
    return -1;
}

static int start_sunken_colony(void)
{
    static unsigned retry_after_frame;
    int colony=z7_fresh_creep_colony();
    if(colony>=0&&b[colony].under_construction)colony=-1;
    if(colony<0||minerals<50) {
        fprintf(stderr,"Z7 Sunken precondition frame%u colony%d minerals%d\n",
            (unsigned)frame,colony,minerals);return 0;
    }
    if((unsigned)frame<retry_after_frame) {
        fprintf(stderr,"Z7 Sunken UI retry throttled frame%u until%u colony%d\n",
            (unsigned)frame,retry_after_frame,colony);return 0;
    }
    int started_at=(int)frame;
    fprintf(stderr,"Z7 urgent Sunken UI attempt begin frame%d colony%d xy%d,%d pool%d/%d minerals%d mode%d menu%d cursor%d,%d\n",
        started_at,colony,b[colony].x,b[colony].y,raw_building(142,SIDE),
        building_complete(raw_building(142,SIDE)),minerals,mode,menu,cx,cy);
    int selected=select_building_slot(colony);
    if(!selected) {
        fprintf(stderr,"Z7 Sunken select failed frame%u colony%d selectedbuilding%d mode%d menuopen%d cursor%d,%d site%d,%d\n",
            (unsigned)frame,colony,selected_building,mode,menu_open,cx,cy,b[colony].x,b[colony].y);
        retry_after_frame=frame+600u;return 0;
    }
    int fields=field_menu_count();
    int sunken_row=MENU_STOP+1;
    int rowok=(fields>sunken_row&&choose_menu_row(sunken_row));
    if(!rowok) {
        fprintf(stderr,"Z7 Sunken menu failed frame%u colony%d fields%d row%d selectedbuilding%d mode%d menu%d open%d\n",
            (unsigned)frame,colony,fields,sunken_row,selected_building,mode,menu,menu_open);
        cancel_ui_mode();retry_after_frame=frame+600u;return 0;
    }
    if(b[colony].type!=B_CREEP_COLONY||!b[colony].under_construction||
       !(b[colony].flags&SC64_Z7_SUNKEN_MORPH_FLAG)) {
        fprintf(stderr,"Z7 Sunken morph verify failed frame%u colony%d type%d under%d flags%x mode%d menu%d\n",
            (unsigned)frame,colony,b[colony].type,b[colony].under_construction,b[colony].flags,mode,menu);
        retry_after_frame=frame+600u;return 0;
    }
    ++actions_taken;
    fprintf(stderr,"Z7 normal-input Sunken morph colony%d frame%u hp%d minerals%d\n",
        colony,(unsigned)frame,b[colony].hp,minerals);
    fprintf(stderr,"Z7 urgent Sunken UI attempt accepted begin%d end%u elapsed%u colony%d flags%x\n",
        started_at,(unsigned)frame,(unsigned)frame-(unsigned)started_at,colony,b[colony].flags);
    retry_after_frame=0;
    cancel_ui_mode();
    return 1;
}

static int start_spore_colony(void)
{
    int colony=z7_fresh_creep_colony();
    if(colony>=0&&b[colony].under_construction)colony=-1;
    if(colony<0||minerals<50||!select_building_slot(colony))return 0;
    if(field_menu_count()!=MENU_STOP+3||!choose_menu_row(MENU_STOP+2)) {
        cancel_ui_mode();return 0;
    }
    if(b[colony].type!=B_CREEP_COLONY||!b[colony].under_construction||
       !(b[colony].flags&SC64_Z7_SPORE_MORPH_FLAG))return 0;
    ++actions_taken;
    fprintf(stderr,"Z7 normal-input Spore morph colony%d frame%u hp%d minerals%d\n",
        colony,(unsigned)frame,b[colony].hp,minerals);
    cancel_ui_mode();
    return 1;
}

static int queue_hatchery_type(int type)
{
    static unsigned last_queue_fail_frame;
    static int next_producer_slot;
    /* Hold the full colony+Sunken cost while the second ordinary defense
       still needs a new Creep Colony. Once that Colony exists, preserve only
       the 50 minerals needed for its normal Sunken morph. Keep a worker queue
       available if the live workforce has already fallen below the two-Drone
       build minimum. */
    int restoring_worker=type==U_DRONE&&
        worker_count(-1)+queued_unit_count(U_DRONE)<macro_worker_goal;
    if(!campaign_single_sunken_opening&&!restoring_worker&&
       owned_building_count(B_SUNKEN_COLONY)==1&&worker_count(-1)>=2) {
        const Sc64RaceUnitRule *rule=sc64_race_unit_rule(type);
        int colony=z7_fresh_creep_colony();
        int morphing=0;
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
            b[i].raw_type==146&&b[i].under_construction)morphing=1;
        int reserve=morphing?0:colony>=0?50:125;
        if(rule&&minerals-rule->minerals<reserve) {
            static unsigned last_reserve_note;
            if(!last_reserve_note||frame-last_reserve_note>=600u) {
                fprintf(stderr,"Z7 reserve minerals for Sunken #2: block unit%d cost%d bank%d reserve%d colony%d frame%u\n",
                    type,rule->minerals,minerals,reserve,colony,(unsigned)frame);
                last_reserve_note=frame;
            }
            return 0;
        }
    }
    int producer=-1;
    /* Round-robin visible townhalls so a completed expansion's larva can
       contribute instead of always feeding the lowest building slot first. */
    for(int n=0;n<MAX_BUILDINGS;n++) {
        int i=(next_producer_slot+n)%MAX_BUILDINGS;
        if(b[i].alive&&b[i].owner==SIDE&&
           sc64_race_is_zerg_townhall(b[i].type)&&
           (building_complete(i)||race_z7_morphing_egg_hall(SIDE,i))&&
           b[i].queue_count<5&&b[i].larva>0) {producer=i;break;}
    }
    if(producer<0||!select_building_slot(producer)) {
        if((type==U_DRONE||type==U_HYDRALISK)&&
           (last_queue_fail_frame==0||frame-last_queue_fail_frame>=600)) {
            fprintf(stderr,"Z7 unit queue blocked type%d producer%d workers%d minerals%d gas%d frame%d mode%d menu%d\n",
                type,producer,worker_count(-1),minerals,gas,frame,mode,menu);
            last_queue_fail_frame=frame;
        }
        return 0;
    }
    for(int n=0;n<5&&train_type!=type;n++)tap(KEY_R);
    if(train_type!=type) {
        if((type==U_DRONE||type==U_HYDRALISK)&&
           (last_queue_fail_frame==0||frame-last_queue_fail_frame>=600)) {
            fprintf(stderr,"Z7 unit selector blocked wanted%d got%d producer%d frame%d mode%d menu%d\n",
                type,train_type,producer,frame,mode,menu);
            last_queue_fail_frame=frame;
        }
        return 0;
    }
    int before=b[producer].queue_count;
    int pending_before=queued_unit_count(type);
    int minerals_before=minerals,gas_before=gas,larva_before=b[producer].larva;
    tap(KEY_A);
    if(b[producer].queue_count>before||queued_unit_count(type)>pending_before
#ifdef SC64_Z7_EGG_PRODUCTION
       ||(race_z7_active()&&b[producer].larva<larva_before&&
          (minerals<minerals_before||gas<gas_before))
#endif
       ){
        ++actions_taken;
        if(owned_building_count(B_HATCHERY)>=2) {
            fprintf(stderr,"Z7 multi-Hatch paid queue type%d producer%d larva-after%d hatches:",
                type,producer,b[producer].larva);
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
                sc64_race_is_zerg_townhall(b[i].type))
                fprintf(stderr," %d(type%d complete%d larva%d queue%d)",i,
                    b[i].type,building_complete(i),b[i].larva,b[i].queue_count);
            fputc('\n',stderr);
        }
        next_producer_slot=(producer+1)%MAX_BUILDINGS;
        fprintf(stderr,"Z7 admitted paid unit type%d producer%d frame%u queue%d->%d pending%d->%d larva%d->%d bank%d/%d->%d/%d\n",
            type,producer,(unsigned)frame,before,b[producer].queue_count,
            pending_before,queued_unit_count(type),larva_before,b[producer].larva,
            minerals_before,gas_before,minerals,gas);
        return 1;
    }
    if((type==U_DRONE||type==U_HYDRALISK)&&
       (last_queue_fail_frame==0||frame-last_queue_fail_frame>=600)) {
        fprintf(stderr,"Z7 unit admission rejected wanted%d producer%d complete%d building%d larva%d queue%d minerals%d gas%d frame%d den%d\n",
            type,producer,building_complete(producer),b[producer].type,b[producer].larva,
            b[producer].queue_count,minerals,gas,frame,
            race_owner_has_prereq(SIDE,SC_PR_HYDRALISK_DEN));
        last_queue_fail_frame=frame;
    }
    return 0;
}

static int assign_gas_worker(void)
{
    static unsigned last_gas_report;
    int refinery=raw_building(149,SIDE),worker=-1;
    if(refinery<0||!building_complete(refinery))return 0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type==41&&u[i].order==ORDER_MINE) {
        if(select_worker(i)){worker=i;break;}
        cancel_ui_mode();
    }
    if(worker<0)return 0;
    int old_order=u[worker].order,old_target=u[worker].target;
    move_cursor(b[refinery].x,b[refinery].y);tap(KEY_A);
    int assigned=u[worker].order==ORDER_GAS;
    if(assigned)fprintf(stderr,"Z7 normal-input worker%d assigned restored extractor%d at frame%u\n",
        worker,refinery,(unsigned)frame);
    else if((unsigned)frame<12000u&&(!last_gas_report||(unsigned)frame-last_gas_report>=600u)) {
        fprintf(stderr,"Z7 gas click failed worker%d old%d/%d now%d/%d refinery%d xy%d,%d mode%d menu%d selected%d frame%u\n",
            worker,old_order,old_target,u[worker].order,u[worker].target,refinery,
            b[refinery].x,b[refinery].y,mode,menu_open,selected_count(),(unsigned)frame);
        last_gas_report=frame;
    }
    return assigned;
}

static int assign_idle_workers_to_minerals(void)
{
    int assigned=0;
    for(int worker=0;worker<MAX_UNITS;worker++)if(u[worker].alive&&u[worker].owner==SIDE&&
        u[worker].raw_type==41&&u[worker].order!=ORDER_BUILD&&u[worker].order!=ORDER_GAS&&
        !(u[worker].order==ORDER_MINE&&u[worker].target>=0&&u[worker].target<MAX_MINERALS&&
          m[u[worker].target].amount>0)) {
        int target=-1,best=0x7fffffff;
        for(int j=0;j<MAX_MINERALS;j++)if(m[j].amount>0) {
            int d=dist2(u[worker].x,u[worker].y,m[j].x,m[j].y);
            if(d<best){best=d;target=j;}
        }
        if(target<0)continue;
        if(!select_worker(worker)) {
            fprintf(stderr,"Z7 mineral select failed worker%d order%d target%d frame%d\n",
                worker,u[worker].order,u[worker].target,frame);
            continue;
        }
        move_cursor(m[target].x,m[target].y);tap(KEY_A);
        if(u[worker].order==ORDER_MINE&&u[worker].target==target) {
            ++assigned;
            fprintf(stderr,"Z7 normal-input worker%d assigned mineral%d at frame%d\n",worker,target,frame);
        } else fprintf(stderr,"Z7 mineral click failed worker%d order%d target%d intended%d cursor%d,%d field%d,%d frame%d\n",
            worker,u[worker].order,u[worker].target,target,cx,cy,m[target].x,m[target].y,frame);
    }
    return assigned;
}

/* The source mineral autopicker binds on a Drone's current position. The
   opening builders can finish far west of the surviving Hatchery, so restore
   six safe miners to the eastern patches nearest that actual depot. */
static int z7_assign_near_hatch_miners(int hatch,int live_workers)
{
    static int pending[MAX_UNITS],pending_target[MAX_UNITS],attempts[MAX_UNITS];
    int patches[3]={-1,-1,-1},patch_distance[3]={0x7fffffff,0x7fffffff,0x7fffffff};
    if(hatch<0||hatch>=MAX_BUILDINGS||!b[hatch].alive||live_workers<6)return 0;
    for(int p=0;p<MAX_MINERALS;p++)if(m[p].amount>0&&m[p].x>=976&&m[p].x<=1088&&
       m[p].y>=1552&&m[p].y<=1624&&currently_visible(m[p].x,m[p].y)) {
        int safe=1,d=dist2(b[hatch].x,b[hatch].y,m[p].x,m[p].y);
        for(int e=0;e<MAX_UNITS;e++)if(u[e].alive&&u[e].owner==ENEMY&&
           currently_visible(u[e].x,u[e].y)&&sc64_race_unit_damage(u[e].type)>0&&
           dist2(m[p].x,m[p].y,u[e].x,u[e].y)<180*180) {safe=0;break;}
        if(!safe)continue;
        for(int n=0;n<3;n++)if(d<patch_distance[n]) {
            for(int k=2;k>n;k--){patches[k]=patches[k-1];patch_distance[k]=patch_distance[k-1];}
            patches[n]=p;patch_distance[n]=d;break;
        }
    }
    if(patches[0]<0)return 0;
    int assigned_count[3]={0,0,0};
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41) {
        int rank=-1;
        for(int n=0;n<3;n++)if(u[i].order==ORDER_MINE&&u[i].target==patches[n])rank=n;
        if(rank>=0)assigned_count[rank]++;
        if(pending[i]) {
            int reached=0;
            if(u[i].order==ORDER_MINE)
                for(int n=0;n<3;n++)if(u[i].target==patches[n])reached=1;
            if(reached||u[i].order!=ORDER_MOVE) {pending[i]=0;pending_target[i]=-1;}
        }
    }
    int worker_rank=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41&&
       worker_rank++<6) {
        int current_rank=-1;
        for(int n=0;n<3;n++)if(u[i].order==ORDER_MINE&&u[i].target==patches[n])current_rank=n;
        if(current_rank>=0||pending[i])continue;
        if(u[i].order!=ORDER_MINE||u[i].carry>0||attempts[i]>=2)continue;
        int target=-1,target_rank=-1;
        for(int n=0;n<3;n++)if(patches[n]>=0&&assigned_count[n]<2) {
            int nx=-1,ny=-1;
            if(!path_next_tile(u[i].x,u[i].y,m[patches[n]].x,m[patches[n]].y,&nx,&ny))continue;
            if(target<0||assigned_count[n]<assigned_count[target_rank]||
               (assigned_count[n]==assigned_count[target_rank]&&patch_distance[n]<patch_distance[target_rank])) {
                target=patches[n];target_rank=n;
            }
        }
        if(target<0)continue;
        if(!select_worker(i)){cancel_ui_mode();continue;}
        if(!menu_open)tap(KEY_B);
        if(!choose_menu_row(MENU_MOVE)||mode!=MODE_MOVE) {cancel_ui_mode();continue;}
        move_cursor(m[target].x,m[target].y);tap(KEY_A);
        int moved=u[i].alive&&u[i].order==ORDER_MOVE&&
            abs(u[i].tx-m[target].x)<=12&&abs(u[i].ty-m[target].y)<=12;
        fprintf(stderr,"Z7 east-mineral move frame%u worker%d target%d xy%d,%d from%d,%d hatch%d,%d d2%d accepted%d\n",
            (unsigned)frame,i,target,m[target].x,m[target].y,u[i].x,u[i].y,b[hatch].x,b[hatch].y,
            patch_distance[target_rank],moved);
        if(moved) {pending[i]=1;pending_target[i]=target;attempts[i]++;return 1;}
    }
    return 0;
}

static void z7_defense_screen_point(int *x,int *y)
{
    /* Keep the source approach at its established opening point. Once the
       enemy geyser is cleared and our Extractor exists, move the ground screen
       toward it while keeping the army on the mineral side of the line. */
    if(geyser_secured&&raw_building(149,SIDE)>=0) {*x=960;*y=1600;}
    else {*x=1050;*y=1620;}
}

static int army_order(int attack,int x,int y)
{
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    if(selected_building>=0&&!deselect_with_local_box())return 0;
    tap(KEY_R); /* Normal combat-group selection. */
    if(!selected_count())return 0;
    if(!menu_open)tap(KEY_B);
    if(attack)tap(KEY_DOWN);
    tap(KEY_A);
    if(mode!=(attack?MODE_ATTACK:MODE_MOVE))return 0;
    move_cursor(x,y);tap(KEY_A);
    int accepted=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].selected) {
        if(u[i].order==(attack?ORDER_ATTACK_MOVE:ORDER_MOVE)) { accepted++;continue; }
        if(!attack||u[i].order!=ORDER_ATTACK_TARGET)continue;
        int target=u[i].target;
        if(target>=0&&target<MAX_UNITS&&u[target].alive&&u[target].owner==ENEMY&&
           currently_visible(u[target].x,u[target].y)&&
           dist2(x,y,u[target].x,u[target].y)<=32*32)accepted++;
        else if(target>=MAX_UNITS&&target<MAX_UNITS+MAX_BUILDINGS) {
            int slot=target-MAX_UNITS;
            if(b[slot].alive&&b[slot].owner==ENEMY&&currently_visible(b[slot].x,b[slot].y)&&
               dist2(x,y,b[slot].x,b[slot].y)<=32*32)accepted++;
        }
    }
    if(accepted)++actions_taken;
    return accepted>0;
}

/* Select one visible combat type through ordinary double-click selection,
   then issue a normal attack-move waypoint. Attack is intentionally a
   location order in this UI, never a direct target write. */
static int typed_attack_order(int type,int target_x,int target_y,int direct_target)
{
    last_direct_order_target=-1;
    unsigned start_frame=(unsigned)frame;
    unsigned select_finish=start_frame,menu_finish=start_frame;
    int representative=-1,clicked=-1,px=0,py=0;
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    int selected=0,wrong=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&u[i].selected) {
        if(u[i].owner==SIDE&&u[i].type==type)++selected;else ++wrong;
    }
    int live_type=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&
        !SC64_TRANSPORT_LOADED(&u[i])&&sc64_game_unit_selectable(u[i].type))++live_type;
    int selected_already=selected>0&&!wrong&&selected_building<0;
    if(select_mutalisk_cluster_for_scout&&type==U_MUTALISK&&selected_already) {
        if(!deselect_with_local_box())return 0;
        selected=wrong=0;
        selected_already=0;
    }
    if(!selected_already) {
        if(selected_building>=0&&!deselect_with_local_box())return 0;
        unsigned char tried[MAX_UNITS]={0};
        int isolated=0;
        for(int attempt=0;attempt<MAX_UNITS;attempt++) {
            representative=-1;
            int representative_distance=0x7fffffff;
            for(int i=0;i<MAX_UNITS;i++)if(!tried[i]&&u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&
                !SC64_TRANSPORT_LOADED(&u[i])&&sc64_game_unit_selectable(u[i].type)) {
                int d=dist2(u[i].x,u[i].y,cx,cy);
                if(d<representative_distance) {
                    representative=i;representative_distance=d;
                }
            }
            if(representative<0)break;
            tried[representative]=1;
            px=u[representative].x;py=u[representative].y;
            if(!type_select_point(type,&px,&py))continue;
            move_cursor(px,py);
            select_finish=(unsigned)frame;
            /* Cursor travel advances the battle. Re-read the actual nearest
               selectable target before committing an A press. */
            int target=selection_target();
            if(target<0||target>=MAX_UNITS||!u[target].alive||u[target].owner!=SIDE||
               u[target].type!=type||SC64_TRANSPORT_LOADED(&u[target])||
               !sc64_game_unit_selectable(u[target].type))continue;
            clicked=target;
            tap(KEY_A);
            if(unit_tap!=clicked||selection_target()!=clicked)continue;
            isolated=1;
            /* The double tap has a 12-tick / 8-pixel window. Follow the
               selected unit with ordinary cursor input as it moves. */
            for(int follow=0;follow<8&&unit_tap==clicked&&
                u[clicked].alive&&u[clicked].owner==SIDE&&u[clicked].type==type&&
                (u32)frame-unit_tap_frame<=UNIT_DOUBLE_TAP_TICKS;follow++) {
                int nx=cursor_lattice_coordinate(cx,u[clicked].x,WORLD_W);
                int ny=cursor_lattice_coordinate(cy,u[clicked].y,WORLD_H);
                int dx=u[clicked].x-cx,dy=u[clicked].y-cy;
                if(dx*dx+dy*dy<=UNIT_DOUBLE_TAP_RADIUS*UNIT_DOUBLE_TAP_RADIUS&&
                   selection_target()==clicked)break;
                u16 held=KEY_R;
                if(cx<nx)held|=KEY_RIGHT;else if(cx>nx)held|=KEY_LEFT;
                if(cy<ny)held|=KEY_DOWN;else if(cy>ny)held|=KEY_UP;
                if(held==KEY_R)break;
                step(held);step(0);
            }
            int dx=u[clicked].x-cx,dy=u[clicked].y-cy;
            if(u[clicked].alive&&u[clicked].owner==SIDE&&u[clicked].type==type&&
               unit_tap==clicked&&
               (u32)frame-unit_tap_frame<=UNIT_DOUBLE_TAP_TICKS&&
               dx*dx+dy*dy<=UNIT_DOUBLE_TAP_RADIUS*UNIT_DOUBLE_TAP_RADIUS&&
               selection_target()==clicked) {
                tap(KEY_A);
                isolated=1;
            } else isolated=0;
            break;
        }
        if(!isolated) {
            fprintf(stderr,"Z7 typed attack select: no valid type%d cursor target frame%u\n",type,(unsigned)frame);
            cancel_ui_mode();return 0;
        }
        selected=0;wrong=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&!SC64_TRANSPORT_LOADED(&u[i])&&u[i].selected) {
            if(u[i].owner==SIDE&&u[i].type==type)++selected;else ++wrong;
        }
        int visible=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&
            !SC64_TRANSPORT_LOADED(&u[i])&&sc64_game_unit_selectable(u[i].type)&&
            dist2(u[i].x,u[i].y,cx,cy)<31*31)++visible;
        fprintf(stderr,"Z7 typed selection type%d selected%d global%d visible%d wrong%d anchor%d clicked%d frame%u\n",
            type,selected,live_type,visible,wrong,representative,clicked,(unsigned)frame);
    }
    if(selected_already) {
        int visible=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type&&
            !SC64_TRANSPORT_LOADED(&u[i])&&sc64_game_unit_selectable(u[i].type)&&
            dist2(u[i].x,u[i].y,cx,cy)<31*31)++visible;
        fprintf(stderr,"Z7 typed selection reused type%d selected%d global%d visible%d wrong%d frame%u\n",
            type,selected,live_type,visible,wrong,(unsigned)frame);
    }
    if(select_mutalisk_cluster_for_scout&&type==U_MUTALISK&&selected<2) {
        fprintf(stderr,"Z7 scout cluster selection rejected selected%d live%d frame%u\n",
            selected,live_type,(unsigned)frame);
        cancel_ui_mode();return 0;
    }
    if(!selected||wrong||!choose_menu_row(MENU_ATTACK)||mode!=MODE_ATTACK) {
        fprintf(stderr,"Z7 typed attack select failed type%d representative%d xy%d,%d cursor%d,%d selected%d wrong%d count%d mode%d menu%d open%d frame%u\n",
            type,representative,representative>=0?u[representative].x:-1,representative>=0?u[representative].y:-1,cx,cy,
            selected,wrong,selected_count(),mode,menu,menu_open,(unsigned)frame);
        cancel_ui_mode();return 0;
    }
    menu_finish=(unsigned)frame;
    if(direct_target>=0) {
        if(direct_target>=MAX_UNITS||!u[direct_target].alive||u[direct_target].owner!=ENEMY) {
            cancel_ui_mode();return 0;
        }
        target_x=u[direct_target].x;target_y=u[direct_target].y;
    }
    move_cursor(target_x,target_y);
    if(direct_target>=0) {
        /* Cursor travel advances the battle. Re-center on the live hostile
           and refuse to fall back to an unintended location order. */
        for(int retry=0;retry<3&&u[direct_target].alive&&u[direct_target].owner==ENEMY;retry++) {
            int dx=u[direct_target].x-cx,dy=u[direct_target].y-cy;
            if(dx*dx+dy*dy<20*20)break;
            move_cursor(u[direct_target].x,u[direct_target].y);
        }
        int dx=u[direct_target].x-cx,dy=u[direct_target].y-cy;
        int visible=currently_visible(u[direct_target].x,u[direct_target].y);
        int found=sc64_direct_attack_find_target(cx,cy);
        Sc64DirectTargetState state={0};
        int state_ok=sc64_direct_target_state(direct_target,SIDE,&state);
        int shooters=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&
            !SC64_TRANSPORT_LOADED(&u[i])&&
            SC64_CAN_ATTACK_FROM_SOURCE(u[i].type,direct_target,u[i].owner,u[i].raw_owner))shooters++;
        int requested_target=direct_target;
        int screen_accepted=0;
        if(accept_guardian_air_screen&&direct_target<MAX_UNITS&&found>=0&&found<MAX_UNITS&&
           u[direct_target].raw_type==44&&u[found].raw_type==43&&u[found].owner==ENEMY&&
           u[found].alive&&!SC64_TRANSPORT_LOADED(&u[found])&&currently_visible(u[found].x,u[found].y)) {
            Sc64DirectTargetState screen_state={0};
            int screen_shooters=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&
                !SC64_TRANSPORT_LOADED(&u[i])&&
                SC64_CAN_ATTACK_FROM_SOURCE(u[i].type,found,u[i].owner,u[i].raw_owner))screen_shooters++;
            screen_accepted=sc64_direct_target_state(found,SIDE,&screen_state)&&
                screen_state.alive&&screen_state.hostile&&!screen_state.loaded&&
                !screen_state.invincible&&screen_state.detectable&&screen_shooters>0;
            if(screen_accepted) {
                fprintf(stderr,"Z7 Guardian screen resolved requested%d raw44 to legal air target%d raw43 at frame%u\n",
                    requested_target,found,(unsigned)frame);
                direct_target=found;
                shooters=screen_shooters;
            }
        }
        fprintf(stderr,"Z7 direct candidate type%d target%d requested%d raw%d owner%d rawowner%d flags%x actorflags%x loaded%d visible%d state%d/%d/%d/%d/%d/%d finder%d foundxy%d,%d d2%d shooters%d cursor%d,%d frame%u\n",
            type,direct_target,requested_target,u[direct_target].raw_type,u[direct_target].owner,u[direct_target].raw_owner,
            (unsigned)u[direct_target].flags,sc64_game_unit_actor_flags(u[direct_target].type),
            SC64_TRANSPORT_LOADED(&u[direct_target]),visible,state_ok,state.alive,state.hostile,
            state.loaded,state.invincible,state.detectable,found,
            found>=0&&found<MAX_UNITS?u[found].x:-1,found>=0&&found<MAX_UNITS?u[found].y:-1,
            dx*dx+dy*dy,shooters,cx,cy,(unsigned)frame);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE)
            fprintf(stderr,"  direct shooter slot%d raw%d type%d rawowner%d flags%x selected%d can%d\n",
                i,u[i].raw_type,u[i].type,u[i].raw_owner,(unsigned)u[i].flags,u[i].selected,
                SC64_CAN_ATTACK_FROM_SOURCE(u[i].type,direct_target,u[i].owner,u[i].raw_owner));
        if(!u[direct_target].alive||u[direct_target].owner!=ENEMY||
           dx*dx+dy*dy>=20*20||!visible||!state_ok||(!screen_accepted&&found!=direct_target)||!shooters) {
            fprintf(stderr,"Z7 direct attack rejected type%d target%d raw%d\n",type,direct_target,u[direct_target].raw_type);
            cancel_ui_mode();return 0;
        }
    }
    tap(KEY_A);
    int accepted=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].selected&&
        u[i].type==type&&((direct_target>=0&&u[i].order==ORDER_ATTACK_TARGET&&u[i].target==direct_target)||
                         (direct_target<0&&u[i].order==ORDER_ATTACK_MOVE)))++accepted;
    if(!accepted&&direct_target<0) {
        /* A normal Attack click can resolve onto a visible enemy actor
           under the cursor. Verify the selected actors' exact live target
           before treating that ordinary key input as accepted. */
        for(int target=0;target<MAX_UNITS;target++)if(u[target].alive&&
           owners_hostile(SIDE,u[target].owner)&&!SC64_TRANSPORT_LOADED(&u[target])&&
           currently_visible(u[target].x,u[target].y)&&
           dist2(target_x,target_y,u[target].x,u[target].y)<=32*32) {
            int attacking=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].selected&&
               u[i].type==type&&u[i].order==ORDER_ATTACK_TARGET&&u[i].target==target)
                ++attacking;
            if(attacking) {
                accepted=attacking;
                last_direct_order_target=target;
                fprintf(stderr,"Z7 visible unit target accepted slot%d raw%d attackers%d xy%d,%d frame%u\n",
                    target,u[target].raw_type,attacking,u[target].x,u[target].y,(unsigned)frame);
                break;
            }
        }
    }
    if(!accepted&&direct_target<0) {
        /* A normal Attack click can resolve onto a visible enemy building
           under the cursor. Treat that as accepted only after reading the
           actual selected actors' target back and matching its exact live,
           visible hostile building to the clicked location. */
        for(int slot=0;slot<MAX_BUILDINGS;slot++)if(b[slot].alive&&
           owners_hostile(SIDE,b[slot].owner)&&currently_visible(b[slot].x,b[slot].y)&&
           dist2(target_x,target_y,b[slot].x,b[slot].y)<=32*32) {
            int expected=MAX_UNITS+slot,attacking=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].selected&&
               u[i].type==type&&u[i].order==ORDER_ATTACK_TARGET&&u[i].target==expected)
                ++attacking;
            if(attacking) {
                accepted=attacking;
                fprintf(stderr,"Z7 visible building target accepted building%d raw%d attackers%d xy%d,%d frame%u\n",
                    slot,b[slot].raw_type,attacking,b[slot].x,b[slot].y,(unsigned)frame);
                break;
            }
        }
    }
    if(accepted&&direct_target>=0)last_direct_order_target=direct_target;
    fprintf(stderr,"Z7 typed %s type%d selected%d live%d target%d raw%d xy%d,%d accepted%d frame%u elapsed%u select%u menu%u targettravel%u\n",
        direct_target>=0?"attack-target":"attack-move",type,selected,live_type,
        direct_target,direct_target>=0?u[direct_target].raw_type:-1,target_x,target_y,
        accepted,(unsigned)frame,(unsigned)frame-start_frame,
        select_finish-start_frame,menu_finish-select_finish,(unsigned)frame-menu_finish);
    if(!accepted)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type)
        fprintf(stderr,"  type%d slot%d selected%d order%d target%d xy%d,%d to%d,%d\n",
            type,i,u[i].selected,u[i].order,u[i].target,u[i].x,u[i].y,u[i].tx,u[i].ty);
    return accepted;
}

static int typed_attack_move(int type,int target_x,int target_y)
{
    return typed_attack_order(type,target_x,target_y,-1);
}

/* The normal attack click can resolve onto the intended visible Extractor.
   The shared helper reports only ORDER_ATTACK_MOVE, so verify this exact
   building target among the still-selected actors before accepting it. */
static int typed_attack_move_to_extractor(int type,int target_x,int target_y,int building)
{
    int accepted=typed_attack_move(type,target_x,target_y);
    if(accepted||building<0||building>=MAX_BUILDINGS||!b[building].alive||
       b[building].owner!=ENEMY||b[building].raw_owner!=4||b[building].raw_type!=149||
       !currently_visible(b[building].x,b[building].y))return accepted;
    int expected=MAX_UNITS+building,attacking=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].selected&&
        u[i].type==type&&u[i].order==ORDER_ATTACK_TARGET&&u[i].target==expected)++attacking;
    fprintf(stderr,"Z7 visible Extractor target accepted%d building%d expected%d selectedAttackers%d frame%u\n",
        attacking>0,building,expected,attacking,(unsigned)frame);
    return attacking>0;
}

static int typed_attack_target(int type,int target)
{
    if(target<0||target>=MAX_UNITS||!u[target].alive||u[target].owner!=ENEMY)return 0;
    return typed_attack_order(type,u[target].x,u[target].y,target);
}

/* Type selection only captures one nearby cluster on this imported map. Move
   the cursor to each remaining live Mutalisk cluster and issue the same normal
   order so the complete air screen converges on its rally/visible target. */
static int campaign_mutalisk_order(int target_x,int target_y,int direct_target)
{
    unsigned char attempted[MAX_UNITS]={0};
    int any=0;
    for(int pass=0;pass<4;pass++) {
        int seed=-1;
        for(int i=0;i<MAX_UNITS;i++)if(!attempted[i]&&u[i].alive&&
           u[i].owner==SIDE&&u[i].type==U_MUTALISK&&!SC64_TRANSPORT_LOADED(&u[i])) {
            seed=i;break;
        }
        if(seed<0)break;
        if(direct_target>=0&&(direct_target>=MAX_UNITS||!u[direct_target].alive||
           u[direct_target].owner!=ENEMY||!currently_visible(u[direct_target].x,u[direct_target].y)))
            return any;
        move_cursor(u[seed].x,u[seed].y);
        select_mutalisk_cluster_for_scout=1;
        int accepted=typed_attack_order(U_MUTALISK,target_x,target_y,direct_target);
        select_mutalisk_cluster_for_scout=0;
        int selected_now=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
           u[i].type==U_MUTALISK&&u[i].selected) {
            attempted[i]=1;++selected_now;
        }
        if(accepted)any=1;
        if(!selected_now)attempted[seed]=1;
    }
    select_mutalisk_cluster_for_scout=0;
    return any;
}

/* Attack-move has no per-unit target ID, so log the commanded hostile and
   the accepted group with its eligibility against that target's class. */
static void log_group_attack(int target)
{
    int flying=target>=0&&target<MAX_UNITS&&sc64_race_unit_flying(u[target].type);
    fprintf(stderr,"Z7 group attack target%d raw%d type%d flying%d accepted:",
        target,target>=0&&target<MAX_UNITS?u[target].raw_type:-1,
        target>=0&&target<MAX_UNITS?u[target].type:-1,flying);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].selected&&
        u[i].order==ORDER_ATTACK_MOVE)
        fprintf(stderr," %d(raw%d/t%d/air%d)",i,u[i].raw_type,u[i].type,
            sc64_race_unit_can_attack_air(u[i].type));
    fputc('\n',stderr);
}

/* Normal double-tap type selection, followed by the ordinary Attack command.
   This lets the Hydralisks raid while Mutalisks stay near the Sunken to answer
   source Guardian pressure. No selected bits are assigned by the controller. */
static int unit_type_attack_order(int type,int x,int y)
{
    int seed=-1,selected=0,accepted=0,best=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==type) {
        int d=dist2(u[i].x,u[i].y,cx,cy);
        if(d<best){best=d;seed=i;}
    }
    if(seed<0) {
        fprintf(stderr,"Z7 retreat skipped no eligible worker frame%u count%d\n",
            (unsigned)frame,worker_count(-1));
        return 0;
    }
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    if((selected_building>=0||selected_count())&&!deselect_with_local_box())return 0;
    move_cursor(u[seed].x,u[seed].y);
    tap(KEY_A);tap(KEY_A);
    for(int i=0;i<MAX_UNITS;i++)selected+=u[i].alive&&u[i].owner==SIDE&&
        u[i].type==type&&u[i].selected;
    if(!selected||(!menu_open&&mode!=MODE_NORMAL))return 0;
    if(!menu_open)tap(KEY_B);
    for(int i=0;i<MENU_COUNT&&menu_cursor!=MENU_ATTACK;i++)tap(KEY_DOWN);
    if(menu_cursor!=MENU_ATTACK)return 0;
    tap(KEY_A);
    if(mode!=MODE_ATTACK)return 0;
    move_cursor(x,y);tap(KEY_A);
    for(int i=0;i<MAX_UNITS;i++)accepted+=u[i].alive&&u[i].owner==SIDE&&
        u[i].type==type&&u[i].order==ORDER_ATTACK_MOVE;
    if(accepted)++actions_taken;
    return accepted>0;
}

static int campaign_unit_can_attack_point(int unit,int x,int y,int target_flying)
{
    if(unit<0||unit>=MAX_UNITS||!u[unit].alive||u[unit].owner!=SIDE||
       sc64_race_is_worker(u[unit].type)||u[unit].type==U_OVERLORD||
       !sc64_race_unit_damage(u[unit].type)||
       sc64_race_unit_range_vs(u[unit].type,target_flying)<=0)return 0;
    if(sc64_race_unit_flying(u[unit].type))return 1;
    int range=(sc64_race_unit_range_vs(u[unit].type,target_flying)+1)/2;
    if(dist2(u[unit].x,u[unit].y,x,y)<=range*range)return 1;
    int nx=-1,ny=-1;
    if(path_next_tile(u[unit].x,u[unit].y,x,y,&nx,&ny))return 1;
    static const int offsets[8][2]={{-32,0},{32,0},{0,-32},{0,32},
        {-32,-32},{32,-32},{-32,32},{32,32}};
    for(int k=0;k<8;k++)if(path_next_tile(u[unit].x,u[unit].y,
        x+offsets[k][0],y+offsets[k][1],&nx,&ny))return 1;
    return 0;
}

static int campaign_any_combat_can_attack_point(int x,int y,int target_flying)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
       campaign_unit_can_attack_point(i,x,y,target_flying))return 1;
    return 0;
}

/* Late campaign target selection must not confuse attack range with a viable
   path. A stranded or currently nearby actor can be in nominal range while
   the target lies beyond impassable terrain; use only ordinary ground routes
   to the target or a nearby attack approach tile. */
static int campaign_unit_has_path_to_attack_point(int unit,int x,int y,int target_flying)
{
    if(unit<0||unit>=MAX_UNITS||!u[unit].alive||u[unit].owner!=SIDE||
       sc64_race_is_worker(u[unit].type)||u[unit].type==U_OVERLORD||
       !sc64_race_unit_damage(u[unit].type)||
       sc64_race_unit_range_vs(u[unit].type,target_flying)<=0)return 0;
    if(sc64_race_unit_flying(u[unit].type))return 1;
    int nx=-1,ny=-1;
    if(path_next_tile(u[unit].x,u[unit].y,x,y,&nx,&ny))return 1;
    static const int offsets[8][2]={{-32,0},{32,0},{0,-32},{0,32},
        {-32,-32},{32,-32},{-32,32},{32,32}};
    for(int k=0;k<8;k++)if(path_next_tile(u[unit].x,u[unit].y,
        x+offsets[k][0],y+offsets[k][1],&nx,&ny))return 1;
    return 0;
}

static int campaign_any_combat_has_path_to_attack_point(int x,int y,int target_flying)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
       campaign_unit_has_path_to_attack_point(i,x,y,target_flying))return 1;
    return 0;
}

static int campaign_target_assignment_useful(int target,int is_building)
{
    if(is_building) {
        if(target<0||target>=MAX_BUILDINGS||!b[target].alive||b[target].owner!=ENEMY)return 0;
        int expected=MAX_UNITS+target;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
           !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
           sc64_race_unit_damage(u[i].type)>0) {
            if(u[i].order==ORDER_ATTACK_TARGET&&u[i].target==expected&&
               campaign_unit_has_path_to_attack_point(i,b[target].x,b[target].y,0))return 1;
            if(u[i].order==ORDER_ATTACK_MOVE&&abs(u[i].tx-b[target].x)<=24&&
               abs(u[i].ty-b[target].y)<=24&&
               campaign_unit_has_path_to_attack_point(i,b[target].x,b[target].y,0))return 1;
        }
        return 0;
    }
    if(target<0||target>=MAX_UNITS||!u[target].alive||u[target].owner!=ENEMY)return 0;
    int flying=sc64_race_unit_flying(u[target].type);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
       !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
       sc64_race_unit_damage(u[i].type)>0&&u[i].order==ORDER_ATTACK_TARGET&&
       u[i].target==target&&campaign_unit_has_path_to_attack_point(i,u[target].x,u[target].y,flying))return 1;
    return 0;
}

static int nearest_enemy_building(int *out_x,int *out_y,int *out_slot)
{
    int ax=0,ay=0,n=0,best=-1,best_d=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
        sc64_race_unit_damage(u[i].type)>0){ax+=u[i].x;ay+=u[i].y;++n;}
    if(!n)return 0;
    ax/=n;ay/=n;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_owner==4&&
        currently_visible(b[i].x,b[i].y)) {
        if((unsigned)frame>=100000u&&
           !campaign_any_combat_has_path_to_attack_point(b[i].x,b[i].y,0))continue;
        int d=dist2(ax,ay,b[i].x,b[i].y);
        if(d<best_d){best=i;best_d=d;}
    }
    if(best<0)return 0;
    *out_x=b[best].x;*out_y=b[best].y;*out_slot=best;return 1;
}

static int campaign_nearest_unexplored(int *out_x,int *out_y)
{
    int ax=0,ay=0,n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
       !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
       sc64_race_unit_damage(u[i].type)>0) { ax+=u[i].x;ay+=u[i].y;++n; }
    if(!n)return 0;
    ax/=n;ay/=n;
    for(int pass=0;pass<2;pass++) {
        int rejected_x[256],rejected_y[256],rejected=0;
        int min_d=pass?0:256*256;
        for(int attempt=0;attempt<256;attempt++) {
            int best_x=-1,best_y=-1,best_d=0x7fffffff;
            for(int row=0;row<MAP_ROWS;row++)for(int col=0;col<MAP_COLS;col++) {
                int idx=col+row*MAP_COLS;
                if(SC64_GRID_GET(explored,idx)||SC64_GRID_GET(visible,idx))continue;
                int x=col*16+8,y=row*16+8;
                if(x<16||y<16||x>=WORLD_W-16||y>=WORLD_H-16)continue;
                int d=dist2(ax,ay,x,y),skip=0;
                if(d<min_d)continue;
                for(int i=0;i<rejected;i++)if(rejected_x[i]==x&&rejected_y[i]==y){skip=1;break;}
                if(!skip&&d<best_d){best_x=x;best_y=y;best_d=d;}
            }
            if(best_x<0)break;
            int nx,ny;
            if(path_next_tile(ax,ay,best_x,best_y,&nx,&ny)) {
                *out_x=best_x;*out_y=best_y;return 1;
            }
            rejected_x[rejected]=best_x;rejected_y[rejected]=best_y;++rejected;
        }
    }
    return 0;
}

static int campaign_local_unexplored(int *out_x,int *out_y)
{
    int ax=0,ay=0,n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&
       u[i].type==U_HYDRALISK) {ax+=u[i].x;ay+=u[i].y;n++;}
    if(!n)return 0;
    ax/=n;ay/=n;
    for(int pass=0;pass<2;pass++) {
        int rejected_x[256],rejected_y[256],rejected=0;
        int min_d=pass?0:128*128;
        for(int attempt=0;attempt<256;attempt++) {
            int best_x=-1,best_y=-1,best_d=0x7fffffff;
            for(int row=0;row<MAP_ROWS;row++)for(int col=0;col<MAP_COLS;col++) {
                int idx=col+row*MAP_COLS;
                if(SC64_GRID_GET(explored,idx)||SC64_GRID_GET(visible,idx))continue;
                int x=col*16+8,y=row*16+8,d=dist2(ax,ay,x,y),skip=0;
                if(x<16||y<16||x>=WORLD_W-16||y>=WORLD_H-16||d<min_d)continue;
                if(campaign_recon_stage>=4&&
                   dist2(campaign_recon_x,campaign_recon_y,x,y)<=128*128)continue;
                for(int i=0;i<rejected;i++)if(rejected_x[i]==x&&rejected_y[i]==y){skip=1;break;}
                if(!skip&&d<best_d){best_x=x;best_y=y;best_d=d;}
            }
            if(best_x<0)break;
            int reachable=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&
               u[i].type==U_HYDRALISK) {
                int nx,ny;
                if(path_next_tile(u[i].x,u[i].y,best_x,best_y,&nx,&ny)){reachable=1;break;}
            }
            if(reachable){*out_x=best_x;*out_y=best_y;return 1;}
            rejected_x[rejected]=best_x;rejected_y[rejected]=best_y;++rejected;
        }
    }
    return 0;
}

/* Select a coherent local Hydra cluster through an ordinary held-L box, then
   issue Attack-Move through the normal command menu. */
static int campaign_local_cluster_attack_move(int target_x,int target_y)
{
    if(menu_open)tap(KEY_B);
    if(mode!=MODE_NORMAL)tap(KEY_B);
    if((selected_count()||selected_building>=0)&&!deselect_with_local_box())return 0;
    int seed=-1,best_count=-1,best_dist=0x7fffffff;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK) {
        int count=0;
        for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&u[j].type==U_HYDRALISK&&
           abs(u[j].x-u[i].x)<=48&&abs(u[j].y-u[i].y)<=48)count++;
        int d=campaign_recon_stage>=19?dist2(u[i].x,u[i].y,campaign_recon_x,campaign_recon_y):
            dist2(u[i].x,u[i].y,target_x,target_y);
        if(count>best_count||(count==best_count&&d<best_dist)){
            seed=i;best_count=count;best_dist=d;
        }
    }
    if(seed<0)return 0;
    int selected=0,wrong=0,rect=0;
    static const int radii[]={48,32,24};
    for(int r=0;r<3;r++) {
        if(selected_count()||selected_building>=0)if(!deselect_with_local_box())return 0;
        int radius=radii[r],left=limit(u[seed].x-radius,0,WORLD_W-16),
            top=limit(u[seed].y-radius,0,WORLD_H-16);
        selected=box_select(left,top,left+radius*2,top+radius*2);
        wrong=0;int hydras=0,eligible=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected) {
            int own_ground_combat=u[i].owner==SIDE&&u[i].type==U_HYDRALISK;
            if((unsigned)frame>=100000u&&u[i].owner==SIDE&&u[i].type==U_ZERGLING)
                own_ground_combat=1;
            if(own_ground_combat&&!sc64_race_is_worker(u[i].type)&&
               u[i].type!=U_OVERLORD&&!sc64_race_unit_flying(u[i].type)&&
               sc64_race_unit_damage(u[i].type)>0&&sc64_game_unit_selectable(u[i].type)) {
                eligible++;
                if(u[i].type==U_HYDRALISK)hydras++;
            } else wrong++;
        }
        if(hydras>0&&!wrong){selected=eligible;rect=radius;break;}
        selected=0;
    }
    if(!selected){
        fprintf(stderr,"Z7 local recon box rejected seed%d xy%d,%d nearby%d frame%u\\n",
            seed,u[seed].x,u[seed].y,best_count,(unsigned)frame);
        if(selected_count()||selected_building>=0)(void)deselect_with_local_box();
        return 0;
    }
    int tx,ty;
    if(!campaign_local_unexplored(&tx,&ty)){
        fprintf(stderr,"Z7 local recon no pathable fog seed%d selected%d frame%u\\n",
            seed,selected,(unsigned)frame);
        (void)deselect_with_local_box();return 0;
    }
    fprintf(stderr,"Z7 local recon box seed%d at%d,%d radius%d selected%d waypoint%d,%d frame%u\\n",
        seed,u[seed].x,u[seed].y,rect,selected,tx,ty,(unsigned)frame);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&u[i].type==U_HYDRALISK) {
        int nx=-1,ny=-1,route=path_next_tile(u[i].x,u[i].y,tx,ty,&nx,&ny);
        fprintf(stderr,"  local scout slot%d at%d,%d order%d to%d,%d path%d next%d,%d\\n",
            i,u[i].x,u[i].y,u[i].order,u[i].tx,u[i].ty,route,nx,ny);
    }
    if(!menu_open)tap(KEY_B);
    tap(KEY_DOWN);tap(KEY_A);
    if(mode!=MODE_ATTACK){cancel_ui_mode();return 0;}
    move_cursor(tx,ty);tap(KEY_A);
    int accepted=0,routable=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected&&u[i].owner==SIDE&&
       (u[i].type==U_HYDRALISK||((unsigned)frame>=100000u&&u[i].type==U_ZERGLING))&&
       u[i].order==ORDER_ATTACK_MOVE&&abs(u[i].tx-tx)<=16&&abs(u[i].ty-ty)<=16) {
        int nx=-1,ny=-1,route=path_next_tile(u[i].x,u[i].y,u[i].tx,u[i].ty,&nx,&ny);
        accepted++;
        if(route)routable++;
        fprintf(stderr,"  local box Hydra slot%d xy%d,%d assigned%d,%d path%d next%d,%d retry%d\n",
            i,u[i].x,u[i].y,u[i].tx,u[i].ty,route,nx,ny,u[i].path_retry);
    }
    fprintf(stderr,"Z7 local box recon accepted%d selected%d routable%d target%d,%d frame%u\n",
        accepted,selected,routable,tx,ty,(unsigned)frame);
    if(accepted){campaign_recon_x=tx;campaign_recon_y=ty;}
    return accepted>0;
}

static int campaign_recon_waypoint(int stage,int *out_x,int *out_y)
{
    static const int route[][2]={{224,864},{656,152},{1168,104},{1856,776}};
    if(stage<4) {
        int tx=route[stage][0],ty=route[stage][1];
        int ax=0,ay=0,n=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
           !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
           sc64_race_unit_damage(u[i].type)>0) { ax+=u[i].x;ay+=u[i].y;++n; }
        if(!n)return 0;
        ax/=n;ay/=n;
        int nx,ny;
        if(path_next_tile(ax,ay,tx,ty,&nx,&ny)){*out_x=tx;*out_y=ty;return 1;}
        int best_x=-1,best_y=-1,best_d=0x7fffffff;
        for(int dy=-128;dy<=128;dy+=32)for(int dx=-128;dx<=128;dx+=32) {
            int x=tx+dx,y=ty+dy;
            if(x<16||y<16||x>=WORLD_W-16||y>=WORLD_H-16)continue;
            int d=dist2(x,y,tx,ty);
            if(d>=best_d||!path_next_tile(ax,ay,x,y,&nx,&ny))continue;
            best_x=x;best_y=y;best_d=d;
        }
        if(best_x<0)return 0;
        *out_x=best_x;*out_y=best_y;return 1;
    }
    return campaign_nearest_unexplored(out_x,out_y);
}

static int campaign_source_region4(int x,int y)
{
    return x>=672&&x<1088&&y>=1520&&y<1760;
}

static int z7_local_extractor_safe(void)
{
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
       owners_hostile(SIDE,u[i].owner)&&currently_visible(u[i].x,u[i].y)&&
       sc64_race_unit_damage(u[i].type)>0&&dist2(832,1584,u[i].x,u[i].y)<160*160)
        return 0;
    return 1;
}

/* Clear local combat first. Before the source AI's 9,000-frame switch, target
   the visible Lair and Pool, then the nearby extractor to unlock a legitimate
   local gas node. Leave the Creep Colony and Spire intact while the army holds
   the cleared approach. */
static int campaign_any_ground_combat_can_reach_enemy_building(void)
{
    static const int offsets[8][2]={{-32,0},{32,0},{0,-32},{0,32},
        {-32,-32},{32,-32},{-32,32},{32,32}};
    for(int bslot=0;bslot<MAX_BUILDINGS;bslot++)if(b[bslot].alive&&b[bslot].owner==ENEMY) {
        for(int unit=0;unit<MAX_UNITS;unit++)if(u[unit].alive&&u[unit].owner==SIDE&&
            !sc64_race_is_worker(u[unit].type)&&u[unit].type!=U_OVERLORD&&
            !sc64_race_unit_flying(u[unit].type)&&sc64_race_unit_damage(u[unit].type)>0) {
            int nx=-1,ny=-1;
            if(path_next_tile(u[unit].x,u[unit].y,b[bslot].x,b[bslot].y,&nx,&ny))return 1;
            for(int k=0;k<8;k++)if(path_next_tile(u[unit].x,u[unit].y,
                b[bslot].x+offsets[k][0],b[bslot].y+offsets[k][1],&nx,&ny))return 1;
        }
    }
    return 0;
}

/* Preserve a live visible air attack while it damages a target. If the
   selected Mutalisks have gone idle or HP stalls, reissue coordinate
   Attack-Move so fog reconnaissance can continue through ordinary UI. */
static int campaign_late_air_scout_step(void)
{
    static const int route[][2]={{1168,104},{1056,80},{1056,176}};
    static int tracked_kind=-1,tracked_slot=-1,tracked_hp=-1;
    static unsigned last_target_progress_frame;
    if(!late_air_breakthrough_active||unit_type_count(U_MUTALISK,SIDE)==0)return 0;
    if(muta_scout_leg<0||muta_scout_leg>=3)muta_scout_leg=0;
    int x=route[muta_scout_leg][0],y=route[muta_scout_leg][1];
    int near=0,total=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_MUTALISK) {
        ++total;if(dist2(u[i].x,u[i].y,x,y)<=112*112)++near;
    }
    int cell=x/16+(y/16)*MAP_COLS;
    if(late_air_scout_active&&near>0&&
       (SC64_GRID_GET(explored,cell)||SC64_GRID_GET(visible,cell))) {
        fprintf(stderr,"Z7 late Muta scout reached leg%d xy%d,%d near%d/%d frame%u\n",
            muta_scout_leg,x,y,near,total,(unsigned)frame);
        muta_scout_leg=(muta_scout_leg+1)%3;late_air_scout_active=0;
        tracked_kind=-1;tracked_slot=-1;tracked_hp=-1;
    }
    if(late_air_scout_active) {
        int useful_orders=0,kind=-1,slot=-1,hp=-1;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_MUTALISK) {
            if(u[i].order==ORDER_ATTACK_MOVE&&dist2(u[i].tx,u[i].ty,x,y)<=320*320)
                ++useful_orders;
            if(u[i].order==ORDER_ATTACK_TARGET) {
                int t=u[i].target;
                if(t>=0&&t<MAX_UNITS&&u[t].alive&&owners_hostile(SIDE,u[t].owner)) {
                    if(kind<0){kind=0;slot=t;hp=u[t].hp;}
                    ++useful_orders;
                } else if(t>=MAX_UNITS&&t<MAX_UNITS+MAX_BUILDINGS) {
                    int bs=t-MAX_UNITS;
                    if(b[bs].alive&&owners_hostile(SIDE,b[bs].owner)) {
                        if(kind<0){kind=1;slot=bs;hp=b[bs].hp;}
                        ++useful_orders;
                    }
                }
            }
        }
        if(useful_orders&&kind>=0) {
            if(kind!=tracked_kind||slot!=tracked_slot) {
                tracked_kind=kind;tracked_slot=slot;tracked_hp=hp;
                last_target_progress_frame=(unsigned)frame;
            } else if(hp!=tracked_hp) {
                tracked_hp=hp;last_target_progress_frame=(unsigned)frame;
            }
            if((unsigned)frame-last_target_progress_frame>=1800u) {
                fprintf(stderr,"Z7 late Muta attack stalled target kind%d slot%d hp%d no progress1800; refresh scout leg%d xy%d,%d frame%u\n",
                    kind,slot,hp,muta_scout_leg,x,y,(unsigned)frame);
                useful_orders=0;tracked_kind=-1;tracked_slot=-1;tracked_hp=-1;
            } else return 1;
        }
        if(useful_orders)return 1;
        fprintf(stderr,"Z7 late Muta scout assignment stale leg%d xy%d,%d; reissue ordinary Attack-Move frame%u\n",
            muta_scout_leg,x,y,(unsigned)frame);
        late_air_scout_active=0;late_air_scout_retry_frame=(unsigned)frame;
        tracked_kind=-1;tracked_slot=-1;tracked_hp=-1;
    }
    if((unsigned)frame<late_air_scout_retry_frame)return 0;
    int accepted=campaign_mutalisk_order(x,y,-1);
    fprintf(stderr,"Z7 late ordinary Muta Attack-Move scout leg%d xy%d,%d accepted%d mutas%d frame%u\n",
        muta_scout_leg,x,y,accepted,unit_type_count(U_MUTALISK,SIDE),(unsigned)frame);
    late_air_scout_retry_frame=(unsigned)frame+(accepted?900u:300u);
    if(accepted){late_air_scout_active=1;late_air_scout_x=x;late_air_scout_y=y;}
    return accepted;
}

static int campaign_assault_retarget(void)
{
    int full_clear=(unsigned)frame>=9000u&&worker_count(-1)>=6&&combat_count(SIDE)>=16;
    /* Retire visible guard targets that accept orders but make no HP progress;
       the full force can then resume reachable structures and reconnaissance. */
    static unsigned char stalled_unit_targets[MAX_UNITS];
    static int tracked_unit_target=-1,tracked_unit_hp=-1;
    static unsigned tracked_unit_progress_frame;
    if((unsigned)frame>=100000u&&!campaign_assault_is_building&&
       campaign_assault_target>=0&&campaign_assault_target<MAX_UNITS&&
       u[campaign_assault_target].alive&&u[campaign_assault_target].owner==ENEMY) {
        int slot=campaign_assault_target;
        if(tracked_unit_target!=slot) {
            tracked_unit_target=slot;tracked_unit_hp=u[slot].hp;
            tracked_unit_progress_frame=(unsigned)frame;
        } else if(u[slot].hp!=tracked_unit_hp) {
            tracked_unit_hp=u[slot].hp;tracked_unit_progress_frame=(unsigned)frame;
        } else if((unsigned)frame-tracked_unit_progress_frame>=1800u) {
            stalled_unit_targets[slot]=1;
            fprintf(stderr,"Z7 retire stalled visible guard slot%d raw%d hp%d no HP progress for1800 frame%u; resume reachable targets/recon\n",
                slot,u[slot].raw_type,u[slot].hp,(unsigned)frame);
            campaign_assault_target=-1;campaign_assault_target_raw=0;
            campaign_assault_is_building=0;tracked_unit_target=-1;
        }
    } else if((unsigned)frame>=100000u)tracked_unit_target=-1;
    int local_extractor=-1;
    /* Focus an observed Guardian before it can siege the worker line. The
       nearby enemy Mutalisks remain the next air target; Hydralisks handle
       those and the ground attackers from their worker-cover position. */
    int guardian_phase=(unsigned)frame>=9000u;
    int priority_raw=guardian_phase?44:38;
    if(guardian_phase&&campaign_guardian_target>=0&&
       campaign_guardian_target<MAX_UNITS&&u[campaign_guardian_target].alive&&
       u[campaign_guardian_target].raw_type==43)
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
           u[i].raw_type==44&&currently_visible(u[i].x,u[i].y)) {
            campaign_guardian_target=-1;break;
        }
    if(unit_type_count(U_MUTALISK,SIDE)>0&&!late_air_breakthrough_active&&
       (campaign_guardian_target<0||
        campaign_guardian_target>=MAX_UNITS||!u[campaign_guardian_target].alive||
        u[campaign_guardian_target].owner!=ENEMY||
        (u[campaign_guardian_target].raw_type!=priority_raw&&
         !(guardian_phase&&u[campaign_guardian_target].raw_type==44))||
        !currently_visible(u[campaign_guardian_target].x,u[campaign_guardian_target].y))) {
        campaign_guardian_target=-1;
        int hatch=z7_townhall(SIDE),best=guardian_phase?400*400:192*192;
        for(int pass=0;pass<(guardian_phase?2:1)&&campaign_guardian_target<0;pass++)
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
           u[i].raw_type==(pass==0?priority_raw:43)&&currently_visible(u[i].x,u[i].y)) {
            int d=guardian_phase&&hatch>=0?
                dist2(b[hatch].x,b[hatch].y,u[i].x,u[i].y):
                dist2(832,1584,u[i].x,u[i].y);
            if((hatch<0||d<=best)&&(campaign_guardian_target<0||d<best)) {
                campaign_guardian_target=i;best=d;
            }
        }
        if(campaign_guardian_target>=0) {
            int g=campaign_guardian_target;
            int ok=campaign_mutalisk_order(u[g].x,u[g].y,g);
            if(ok) {
                campaign_guardian_return_set=0;
                fprintf(stderr,"Z7 Mutalisk %s direct target slot%d raw%d frame%u mutas%d\n",
                    guardian_phase?(u[g].raw_type==44?"screen Guardian":"screen Mutalisk"):
                                   "support local Hydralisk",
                    g,u[g].raw_type,(unsigned)frame,unit_type_count(U_MUTALISK,SIDE));
            }
        } else if(!late_air_breakthrough_active&&!campaign_guardian_return_set) {
            /* Keep the air screen on the observed Guardian approach above
               the Hatchery. This makes the long-range threat visible before
               it starts damaging workers or buildings from outside Hydra
               range, while keeping the four Mutalisks together. */
            int hx=980,hy=1510;
            if(campaign_mutalisk_order(hx,hy,-1))campaign_guardian_return_set=1;
        }
    }
    /* Keep the local Extractor intact through the authored opening so the
       player can spend the finite starting gas on Hydralisks before sending
       Drones onto the contested geyser. */
    if((unsigned)frame>=(early_gas_policy?2500u:9000u))for(int i=0;i<MAX_BUILDINGS;i++)
        if(b[i].alive&&b[i].owner==ENEMY&&b[i].raw_owner==4&&
           b[i].raw_type==149&&campaign_source_region4(b[i].x,b[i].y)&&
           currently_visible(b[i].x,b[i].y)&&z7_local_extractor_safe()) {
            local_extractor=i;break;
        }
    if(campaign_assault_is_building) {
        int t=campaign_assault_target;
        if(t>=0&&t<MAX_BUILDINGS&&b[t].alive&&b[t].owner==ENEMY&&
           currently_visible(b[t].x,b[t].y)&&
           ((unsigned)frame<100000u||campaign_target_assignment_useful(t,1)))return 1;
        if((unsigned)frame>=100000u&&t>=0){campaign_assault_target=-1;
            campaign_assault_target_raw=0;campaign_assault_is_building=0;}
        if(campaign_assault_target_raw==149&&t>=0&&t<MAX_BUILDINGS&&
           (!b[t].alive||b[t].hp<=0))geyser_secured=1;
    } else {
        int t=campaign_assault_target;
        if(!full_clear&&t>=0&&t<MAX_UNITS&&u[t].alive&&u[t].owner==ENEMY&&
           currently_visible(u[t].x,u[t].y)&&local_extractor<0&&
           ((unsigned)frame<100000u||campaign_target_assignment_useful(t,0)))return 1;
        if((unsigned)frame>=100000u&&t>=0){campaign_assault_target=-1;
            campaign_assault_target_raw=0;campaign_assault_is_building=0;}
    }
    int ax=0,ay=0,n=0,best=0x7fffffff,target=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
        sc64_race_unit_damage(u[i].type)>0) {ax+=u[i].x;ay+=u[i].y;++n;}
    if(n){ax/=n;ay/=n;}
    int tx=-1,ty=-1,accepted=0;
    if(local_extractor>=0) {
        tx=b[local_extractor].x;ty=b[local_extractor].y;
        accepted=(unsigned)frame>=45000u?army_order(1,tx,ty):
            typed_attack_move_to_extractor(U_HYDRALISK,tx,ty,local_extractor);
        if(accepted) {
            campaign_assault_target=local_extractor;campaign_assault_target_raw=149;
            campaign_assault_is_building=1;campaign_assault_hold_set=0;
            fprintf(stderr,"Z7 full-army assault visible local Extractor slot%d xy%d,%d frame%u\n",
                local_extractor,tx,ty,(unsigned)frame);
            return 1;
        }
    }
    if(full_clear) {
        int building=-1;
        if(nearest_enemy_building(&tx,&ty,&building)) {
            accepted=late_air_breakthrough_active&&unit_type_count(U_MUTALISK,SIDE)>0?
                campaign_mutalisk_order(tx,ty,-1):((unsigned)frame>=45000u?army_order(1,tx,ty):
                typed_attack_move(U_HYDRALISK,tx,ty));
            if(accepted) {
                campaign_assault_target=building;campaign_assault_target_raw=b[building].raw_type;
                campaign_assault_is_building=1;campaign_assault_hold_set=0;
                fprintf(stderr,"Z7 full-clear army assault visible building slot%d raw%d xy%d,%d frame%u army%d workers%d\n",
                    building,b[building].raw_type,tx,ty,(unsigned)frame,combat_count(SIDE),worker_count(-1));
                return 1;
            }
        }
    }
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
        ((unsigned)frame<100000u||!stalled_unit_targets[i])&&
        owners_hostile(SIDE,u[i].owner)&&!sc64_race_is_worker(u[i].type)&&
        sc64_race_unit_damage(u[i].type)>0&&currently_visible(u[i].x,u[i].y)&&
        ((unsigned)frame<100000u||campaign_any_combat_has_path_to_attack_point(
            u[i].x,u[i].y,sc64_race_unit_flying(u[i].type)))&&
        ((unsigned)frame>=9000u||campaign_source_region4(u[i].x,u[i].y))) {
        int d=dist2(ax,ay,u[i].x,u[i].y);
        if(d<best){best=d;target=i;}
    }
    if(accepted) return 1;
    if(target>=0) {
        if(campaign_recon_active)campaign_recon_stage++;
        campaign_recon_active=0;
        tx=u[target].x;ty=u[target].y;
        accepted=target<0?0:(late_air_breakthrough_active&&unit_type_count(U_MUTALISK,SIDE)>0?
            campaign_mutalisk_order(tx,ty,target):((unsigned)frame>=100000u?
            army_order(1,tx,ty):typed_attack_target(U_HYDRALISK,target)));
        if(accepted) {
            campaign_assault_target=target;campaign_assault_target_raw=u[target].raw_type;
            campaign_assault_is_building=0;campaign_assault_hold_set=0;
            fprintf(stderr,"Z7 full-army assault visible %s guard slot%d raw%d xy%d,%d frame%u army%d\n",
                sc64_race_unit_flying(u[target].type)?"air":"ground",
                target,u[target].raw_type,tx,ty,(unsigned)frame,combat_count(SIDE));
        }
    } else {
        int building=-1,building_d=0x7fffffff;
        if((unsigned)frame>=9000u) {
            if(!nearest_enemy_building(&tx,&ty,&building))building=-1;
        } else {
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&
                b[i].raw_owner==4&&currently_visible(b[i].x,b[i].y)&&
                campaign_source_region4(b[i].x,b[i].y)&&
                (early_gas_policy||b[i].raw_type==132||b[i].raw_type==142||
                 ((unsigned)frame>=9000u&&b[i].raw_type==149))) {
                /* Keep the scripted placements first; then clear the observed
                   enemy Extractor only after the opening gas has funded the
                   first Hydralisk reinforcements. */
                int priority=b[i].raw_type==132||b[i].raw_type==142?0:1;
                int current_priority=building>=0&&
                    (b[building].raw_type==132||b[building].raw_type==142)?0:1;
                int d=dist2(ax,ay,b[i].x,b[i].y);
                if(building<0||priority<current_priority||
                   (priority==current_priority&&d<building_d)) {
                    building=i;building_d=d;
                }
            }
            if(building>=0){tx=b[building].x;ty=b[building].y;}
        }
        if(building>=0) {
            if(campaign_recon_active)campaign_recon_stage++;
            campaign_recon_active=0;
            accepted=(unsigned)frame>=45000u?army_order(1,tx,ty):
                typed_attack_move(U_HYDRALISK,tx,ty);
            if(accepted) {
                campaign_assault_target=building;campaign_assault_target_raw=b[building].raw_type;
                campaign_assault_is_building=1;campaign_assault_hold_set=0;
                fprintf(stderr,"Z7 full-army assault visible %s building slot%d raw%d xy%d,%d frame%u army%d\n",
                    (unsigned)frame<9000u?"priority":"",
                    building,b[building].raw_type,tx,ty,(unsigned)frame,combat_count(SIDE));
            }
        } else if((unsigned)frame>=100000u&&campaign_recon_stage>=19&&
                  worker_count(-1)>=6&&combat_count(SIDE)>=10) {
            static int local_recon_activated;
            static unsigned local_recon_retry;
            if(!local_recon_activated) {
                local_recon_activated=1;campaign_recon_active=0;
            }
            int col=campaign_recon_x/16,row=campaign_recon_y/16;
            int idx=col+row*MAP_COLS;
            if(campaign_recon_active&&col>=0&&col<MAP_COLS&&row>=0&&row<MAP_ROWS&&
               (SC64_GRID_GET(explored,idx)||SC64_GRID_GET(visible,idx))) {
                campaign_recon_active=0;campaign_recon_stage++;
                fprintf(stderr,"Z7 local recon tile revealed stage%d xy%d,%d frame%u\n",
                    campaign_recon_stage,campaign_recon_x,campaign_recon_y,(unsigned)frame);
            }
            if(campaign_recon_active) return 1;
            if((unsigned)frame>=local_recon_retry) {
                if(campaign_local_cluster_attack_move(ax,ay)) {
                    campaign_assault_target=-1;campaign_assault_target_raw=0;
                    campaign_assault_is_building=0;campaign_recon_active=1;
                    return 1;
                }
                local_recon_retry=(unsigned)frame+600u;
            }
            if(late_air_breakthrough_active&&campaign_late_air_scout_step())return 1;
        } else if((unsigned)frame>=45000u&&worker_count(-1)>=6&&combat_count(SIDE)>=10) {
            int matching=0,near_waypoint=0;
            if(campaign_recon_active)for(int i=0;i<MAX_UNITS;i++)
                if(u[i].alive&&u[i].owner==SIDE&&!sc64_race_is_worker(u[i].type)&&
                   u[i].type!=U_OVERLORD&&sc64_race_unit_damage(u[i].type)>0&&
                   u[i].order==ORDER_ATTACK_MOVE&&
                   abs(u[i].tx-campaign_recon_x)<=16&&abs(u[i].ty-campaign_recon_y)<=16) {
                    matching++;
                    if(dist2(u[i].x,u[i].y,campaign_recon_x,campaign_recon_y)<=128*128)
                        near_waypoint++;
                }
            if(campaign_recon_active&&matching>0&&near_waypoint*2>=matching) {
                campaign_recon_active=0;campaign_recon_stage++;
                fprintf(stderr,"Z7 recon waypoint reached stage%d xy%d,%d frame%u group%d\n",
                    campaign_recon_stage,campaign_recon_x,campaign_recon_y,
                    (unsigned)frame,matching);
            } else if(campaign_recon_active&&matching==0) {
                campaign_recon_active=0;campaign_recon_stage++;
            }
            static unsigned next_recon_retry;
            if(!campaign_recon_active&&(unsigned)frame>=next_recon_retry) {
                int scout_x=-1,scout_y=-1;
                if(campaign_recon_waypoint(campaign_recon_stage,&scout_x,&scout_y)) {
                    accepted=army_order(1,scout_x,scout_y);
                    if(accepted) {
                        int movers=0;
                        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
                           !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
                           sc64_race_unit_damage(u[i].type)>0&&u[i].selected&&
                           u[i].order==ORDER_ATTACK_MOVE&&abs(u[i].tx-scout_x)<=16&&
                           abs(u[i].ty-scout_y)<=16)movers++;
                        campaign_assault_target=-1;campaign_assault_target_raw=0;
                        campaign_assault_is_building=0;
                        if(movers>0) {
                            campaign_recon_active=1;campaign_recon_x=scout_x;campaign_recon_y=scout_y;
                        } else {campaign_recon_stage++;next_recon_retry=(unsigned)frame+600u;}
                        fprintf(stderr,"Z7 late ordinary Attack-Move reconnaissance waypoint stage%d xy%d,%d frame%u army%d movers%d\n",
                            campaign_recon_stage,scout_x,scout_y,(unsigned)frame,
                            combat_count(SIDE),movers);
                    } else next_recon_retry=(unsigned)frame+600u;
                } else next_recon_retry=(unsigned)frame+600u;
            }
        } else if((unsigned)frame<9000u&&!campaign_assault_hold_set) {
            accepted=typed_attack_move(U_HYDRALISK,1030,1620);
            if(accepted) {
                campaign_assault_target=-1;campaign_assault_target_raw=0;
                campaign_assault_is_building=0;campaign_assault_hold_set=1;
                fprintf(stderr,"Z7 full-army hold at1030,1620 frame%u army%d; noncritical source structures remain\n",
                    (unsigned)frame,combat_count(SIDE));
            }
        }
    }
    if(!accepted&&target<0)campaign_assault_target=-1;
    return accepted;
}

static int enemy_combat_near_geyser(void)
{
    if(geyser_count<=0)return 0;
    int count=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&currently_visible(u[i].x,u[i].y)&&
        u[i].type>=0&&u[i].type<SC_RU_COUNT&&sc64_race_unit_damage(u[i].type)>0&&
        dist2(u[i].x,u[i].y,geysers[0].x,geysers[0].y)<256*256)++count;
    return count;
}

static int enemy_combat_near_source_node(int x,int y,int radius)
{
    int radius2=radius*radius;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&currently_visible(u[i].x,u[i].y)&&
        u[i].type>=0&&u[i].type<SC_RU_COUNT&&
        sc64_race_unit_damage(u[i].type)>0&&dist2(u[i].x,u[i].y,x,y)<radius2)
        return 1;
    return 0;
}

static int geyser_node_at(int x,int y)
{
    for(int i=0;i<geyser_count;i++)
        if(dist2(geysers[i].x,geysers[i].y,x,y)<=96*96)return 1;
    return 0;
}

static int source_extractor_at_position(int x,int y)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&currently_visible(b[i].x,b[i].y)&&
        b[i].raw_owner==4&&b[i].raw_type==149&&dist2(b[i].x,b[i].y,x,y)<=96*96)return i;
    return -1;
}

static void source_checkpoint(const char *kind)
{
    if(!trace_file)return;
    unsigned n_enemy_units=0,n_enemy_buildings=0,n_human_units=0,n_human_buildings=0;
    for(int i=0;i<MAX_UNITS;i++) {
        n_enemy_units+=u[i].alive&&u[i].raw_owner==4;
        n_human_units+=u[i].alive&&u[i].owner==SIDE;
    }
    for(int i=0;i<MAX_BUILDINGS;i++) {
        n_enemy_buildings+=b[i].alive&&b[i].raw_owner==4;
        n_human_buildings+=b[i].alive&&b[i].owner==SIDE;
    }
    fprintf(trace_file,"%s{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%u,"
        "\"screen\":%d,\"won\":%d,\"lost\":%d,\"outcome\":%u,"
        "\"started\":%u,\"fired\":%u,\"action_cursors\":[",
        trace_first?"":",",kind,host_ticks,(unsigned)frame,screen,won,lost,
        race_z7.runtime.model.outcome,race_z7.runtime.model.started,
        race_z7.runtime.model.started);
    trace_first=0;
    for(unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;i++)
        fprintf(trace_file,"%s%u",i?",":"",race_z7.runtime.model.action_cursor[i]);
    fputs("],\"action_ticks\":[",trace_file);
    for(unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;i++)
        fprintf(trace_file,"%s%u",i?",":"",race_z7.runtime.action_ticks[i]);
    fprintf(trace_file,"],\"switch10\":%u,\"ai_script_mask\":%u,"
        "\"ai_plan_ticks\":%u,\"ai_plan_phase\":%u,\"fault\":%u,"
        "\"side_units\":%u,\"side_buildings\":%u,\"enemy_units\":%u,"
        "\"enemy_buildings\":%u,\"workers\":%d,\"minerals\":%d,\"gas\":%d,"
        "\"action_count\":%d}",race_z7.runtime.switch10,race_z7.runtime.ai_script_mask,
        race_z7.runtime.ai_plan_ticks,race_z7.runtime.ai_plan_phase,
        race_z7.runtime.fault,n_human_units,n_human_buildings,n_enemy_units,
        n_enemy_buildings,worker_count(-1),minerals,gas,actions_taken);
    fflush(trace_file);
}

static void launch_from_normal_menu(void)
{
    game_init();prior_keys=0;
    assert(screen==SCREEN_TITLE);
    tap(KEY_A);
    assert(race_front.page==SC_FRONT_MISSION&&race_front.faction==SC_FRONT_ZERG);
    int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z7_GLOBAL_ID);
    assert(ordinal>=0);
    int page=ordinal/2,row=ordinal%2;
    for(int guard=0;race_front.mission_page!=page&&guard<10;guard++)tap(KEY_RIGHT);
    while(race_front.row!=row)tap(race_front.row<row?KEY_DOWN:KEY_UP);
    assert(race_front.mission_page==page&&race_front.row==row);
    tap(KEY_A);
    assert(race_front.page==SC_FRONT_BRIEFING&&race_front.mission==SC64_Z7_GLOBAL_ID);
    tap(KEY_START);
    assert(screen==SCREEN_PLAY&&race_selected_mission==SC64_Z7_GLOBAL_ID&&race_z7_active());
    assert(race_z7.runtime.fault==0&&race_z7_map==&sc64_race_maps[SC64_Z7_GLOBAL_ID]);
    for(int i=0;i<MAX_MINERALS;i++)if(m[i].amount>0)
        fprintf(stderr,"Z7 mineral%d xy%d,%d amount%d\n",i,m[i].x,m[i].y,m[i].amount);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41)
        fprintf(stderr,"Z7 start drone%d order%d target%d xy%d,%d\n",i,u[i].order,u[i].target,u[i].x,u[i].y);
    for(int g=0;g<geyser_count;g++) {
        int best=-1,best_d=0x7fffffff;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY) {
            int d=dist2(geysers[g].x,geysers[g].y,u[i].x,u[i].y);
            if(d<best_d){best=i;best_d=d;}
        }
        fprintf(stderr,"Z7 source geyser%d xy%d,%d nearestEnemy%d raw%d xy%d,%d d2%d\n",
            g,geysers[g].x,geysers[g].y,best,
            best>=0?u[best].raw_type:-1,best>=0?u[best].x:-1,best>=0?u[best].y:-1,best_d);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
            dist2(geysers[g].x,geysers[g].y,u[i].x,u[i].y)<240*240)
            fprintf(stderr,"  enemy near geyser slot%d raw%d type%d hp%d xy%d,%d d2%d\n",
                i,u[i].raw_type,u[i].type,u[i].hp,u[i].x,u[i].y,
                dist2(geysers[g].x,geysers[g].y,u[i].x,u[i].y));
    }
}

static int z7_worker_threat(int worker,int gas_only,int *threat_slot)
{
    if(threat_slot)*threat_slot=-1;
    if(worker<0||worker>=MAX_UNITS||!u[worker].alive||u[worker].owner!=SIDE||
       u[worker].raw_type!=41)return 0;
    int patch=-1;
    if(!gas_only&&u[worker].order==ORDER_MINE&&u[worker].target>=0&&
       u[worker].target<MAX_MINERALS&&m[u[worker].target].amount>0)patch=u[worker].target;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
        currently_visible(u[i].x,u[i].y)&&sc64_race_unit_damage(u[i].type)>0) {
        int threatened=0;
        if(gas_only) {
            int range=(sc64_race_unit_range_vs(u[i].type,0)+1)/2;
            /* Match game.c actor-to-actor weapon reach: BWAPI fact range / 2. */
            threatened=range>0&&(u[i].target==worker||
                dist2(u[worker].x,u[worker].y,u[i].x,u[i].y)<=range*range);
        } else {
            int range=(sc64_race_unit_range_vs(u[i].type,0)+1)/2;
            threatened=(range>0&&(u[i].target==worker||
                dist2(u[worker].x,u[worker].y,u[i].x,u[i].y)<=range*range))||
                (patch>=0&&dist2(m[patch].x,m[patch].y,u[i].x,u[i].y)<180*180);
        }
        if(threatened){if(threat_slot)*threat_slot=i;return 1;}
    }
    return 0;
}

static int z7_worker_is_exposed(int worker)
{
    return worker>=0&&worker<MAX_UNITS&&u[worker].alive&&u[worker].owner==SIDE&&
        u[worker].raw_type==41&&
        (u[worker].order==ORDER_MINE||u[worker].order==ORDER_GAS||u[worker].carry>0);
}

static int retreat_worker_individually(int worker,int threat)
{
    if(worker<0||worker>=MAX_UNITS||!u[worker].alive)return 0;
    if(!select_worker(worker)) {
        fprintf(stderr,"Z7 evacuation selection failed frame%u slot%d alive%d order%d xy%d,%d mode%d menu%d cursor%d,%d selected%d selectedBuilding%d\n",
            (unsigned)frame,worker,u[worker].alive,u[worker].order,u[worker].x,u[worker].y,
            mode,menu_open,cx,cy,selected_count(),selected_building);
        return 0;
    }
    static const int candidate[][2]={{560,1800},{560,1940},{640,1800},{640,1940}};
    int safe_x=-1,safe_y=-1;
    unsigned phase=retreat_phase++;
    for(unsigned probe=0;probe<sizeof(candidate)/sizeof(candidate[0]);probe++) {
        unsigned ci=(phase+probe)%((unsigned)(sizeof(candidate)/sizeof(candidate[0])));
        int x=candidate[ci][0],y=candidate[ci][1],nx=-1,ny=-1;
        if(!path_next_tile(u[worker].x,u[worker].y,x,y,&nx,&ny))continue;
        int safe=1;
        for(int i=0;i<MAX_UNITS&&safe;i++)if(u[i].alive&&u[i].owner==ENEMY&&
            currently_visible(u[i].x,u[i].y)&&sc64_race_unit_damage(u[i].type)>0&&
            dist2(x,y,u[i].x,u[i].y)<280*280)safe=0;
        if(safe){safe_x=x;safe_y=y;break;}
    }
    if(safe_x<0) {
        fprintf(stderr,"Z7 evacuation waypoint failed frame%u slot%d order%d threat%d mode%d menu%d cursor%d,%d selected%d\n",
            (unsigned)frame,worker,u[worker].order,threat,mode,menu_open,cx,cy,selected_count());
        return 0;
    }
    if(!menu_open)tap(KEY_B);
    if(!choose_menu_row(MENU_MOVE)||mode!=MODE_MOVE) {
        fprintf(stderr,"Z7 evacuation move-menu failed frame%u slot%d mode%d menu%d cursor%d,%d selected%d\n",
            (unsigned)frame,worker,mode,menu_open,cx,cy,selected_count());
        cancel_ui_mode();return 0;
    }
    move_cursor(safe_x,safe_y);tap(KEY_A);
    int moved=u[worker].alive&&u[worker].order==ORDER_MOVE&&
        abs(u[worker].tx-safe_x)<=12&&abs(u[worker].ty-safe_y)<=12;
    fprintf(stderr,"Z7 evacuation worker frame%u slot%d order%d carry%d threat%d raw%d threatxy%d,%d d2%d homeD2%d dest%d,%d moved%d mode%d menu%d selected%d\n",
        (unsigned)frame,worker,u[worker].order,u[worker].carry,threat,
        threat>=0?u[threat].raw_type:-1,threat>=0?u[threat].x:-1,threat>=0?u[threat].y:-1,
        threat>=0?dist2(u[worker].x,u[worker].y,u[threat].x,u[threat].y):-1,
        z7_townhall(SIDE)>=0?dist2(u[worker].x,u[worker].y,b[z7_townhall(SIDE)].x,b[z7_townhall(SIDE)].y):-1,
        safe_x,safe_y,moved,mode,menu_open,selected_count());
    return moved;
}

static int retreat_workers_group(void)
{
    int seed=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type==41&&u[i].order!=ORDER_BUILD) {
        seed=i;break;
    }
    if(seed<0)return 0;
    int selected=0,all_selected_are_workers=selected_building<0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].selected) {
        if(u[i].owner==SIDE&&u[i].raw_type==41&&u[i].order!=ORDER_BUILD)++selected;
        else all_selected_are_workers=0;
    }
    int reuse_selection=selected>0&&selected_count()==selected&&all_selected_are_workers;
    if(!reuse_selection) {
        if(selected_building>=0&&!deselect_with_local_box())return 0;
        if(!select_worker(seed)) {
            fprintf(stderr,"Z7 retreat single-select failed frame%u seed%d alive%d order%d xy%d,%d\n",
                (unsigned)frame,seed,u[seed].alive,u[seed].order,u[seed].x,u[seed].y);
            return 0;
        }
        tap(KEY_A);
        selected=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
            u[i].raw_type==41&&u[i].selected&&u[i].order!=ORDER_BUILD)++selected;
    }
    if(!selected) {
        fprintf(stderr,"Z7 retreat selection failed frame%u seed%d alive%d order%d xy%d,%d\n",
            (unsigned)frame,seed,u[seed].alive,u[seed].order,u[seed].x,u[seed].y);
        return 0;
    }
    /* Select an in-bounds, actually reachable point west of the mineral line.
       Multi-unit orders receive per-unit formation offsets, so validate each
       resulting destination rather than only the anchor. Alternate direction
       along the west shelf to keep a completed move from idling in place. */
    static const int candidate[][2]={{560,1800},{560,1940},{640,1800},{640,1940}};
    int safe_x=-1,safe_y=-1;
    unsigned phase=retreat_phase++;
    for(unsigned probe=0;probe<sizeof(candidate)/sizeof(candidate[0]);probe++) {
        unsigned ci=(phase+probe)%((unsigned)(sizeof(candidate)/sizeof(candidate[0])));
        int x=candidate[ci][0],y=candidate[ci][1];
        int valid=1,n=0;
        for(int i=0;i<MAX_UNITS&&valid;i++)if(u[i].alive&&u[i].owner==SIDE&&
            u[i].raw_type==41&&u[i].selected&&u[i].order!=ORDER_BUILD) {
            int tx=limit(x+((n%5)-2)*8,8,WORLD_W-8);
            int ty=limit(y+((n/5)-1)*8,8,WORLD_H-8),nx=-1,ny=-1;
            if(!path_next_tile(u[i].x,u[i].y,tx,ty,&nx,&ny))valid=0;
            n++;
        }
        for(int i=0;i<MAX_UNITS&&valid;i++)if(u[i].alive&&u[i].owner==ENEMY&&
            sc64_race_unit_damage(u[i].type)>0&&dist2(x,y,u[i].x,u[i].y)<280*280)
            valid=0;
        if(valid&&n) {safe_x=x;safe_y=y;break;}
    }
    if(safe_x<0) {
        fprintf(stderr,"Z7 retreat found no reachable west waypoint frame%u selected%d\n",
            (unsigned)frame,selected);
        return 0;
    }
    if(!menu_open)tap(KEY_B);
    if(!choose_menu_row(MENU_MOVE)||mode!=MODE_MOVE) {
        cancel_ui_mode();return 0;
    }
    move_cursor(safe_x,safe_y);tap(KEY_A);
    int moved=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].raw_type==41&&u[i].selected&&u[i].order==ORDER_MOVE&&
        abs(u[i].tx-safe_x)<=12&&abs(u[i].ty-safe_y)<=12)++moved;
    fprintf(stderr,"Z7 retreat detail frame%u seed%d selected%d moved%d dest%d,%d mode%d menu%d cursor%d,%d:",
        (unsigned)frame,seed,selected,moved,safe_x,safe_y,mode,menu_open,cx,cy);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41)
        fprintf(stderr," %d(o%d/t%d xy%d,%d to%d,%d sel%d)",i,u[i].order,u[i].target,
            u[i].x,u[i].y,u[i].tx,u[i].ty,u[i].selected);
    fputc('\n',stderr);
    return moved;
}

/* Stable defensive rally between the completed Hatchery and its source
   mineral field. The field coordinates are static, so this point does not
   follow a moving army or shift when one patch is depleted. */
static void z7_mineral_cover_point(int hatch,int *x,int *y)
{
    if(hatch<0||hatch>=MAX_BUILDINGS||!b[hatch].alive) {
        *x=1050;*y=1680;return;
    }
    int sx=0,sy=0,n=0;
    for(int i=0;i<MAX_MINERALS;i++)
        if(dist2(b[hatch].x,b[hatch].y,m[i].x,m[i].y)<=300*300) {
            sx+=m[i].x;sy+=m[i].y;++n;
        }
    if(n) {
        *x=(3*(sx/n)+b[hatch].x)/4;
        *y=(3*(sy/n)+b[hatch].y)/4;
    } else {
        *x=b[hatch].x-85;*y=b[hatch].y-54;
    }
}

static int z7_mutas_near_point(int x,int y,int radius)
{
    int total=0,near=0,limit2=radius*radius;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_MUTALISK) {
        ++total;if(dist2(x,y,u[i].x,u[i].y)<=limit2)++near;
    }
    return total>0&&near*2>=total;
}

/* Search real, reachable building cells around the worker-field cover point
   before considering a safer but distant fallback site. The current site
   checks remain authoritative, including snapped cursor placement. */
static int z7_front_colony_site(int hatch,int *out_x,int *out_y)
{
    int cover_x,cover_y;
    z7_mineral_cover_point(hatch,&cover_x,&cover_y);
    int best=0x7fffffff,found=0,site_x=0,site_y=0;
    for(int dy=-96;dy<=96;dy+=16)for(int dx=-96;dx<=96;dx+=16) {
        int x=cursor_lattice_coordinate(cx,cover_x+dx,WORLD_W);
        int y=cursor_lattice_coordinate(cy,cover_y+dy,WORLD_H);
        int field_d=dist2(x,y,cover_x,cover_y);
        if(x<24||y<24||x>=WORLD_W-24||y>=WORLD_H-24||
           field_d>96*96||dist2(x,y,b[hatch].x,b[hatch].y)<64*64||
           !race_can_site(SIDE,B_CREEP_COLONY,x,y)||!can_place(B_CREEP_COLONY,x,y))continue;
        int nearest=0x7fffffff,reachable=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY) {
            int d=dist2(x,y,u[i].x,u[i].y);if(d<nearest)nearest=d;
        }
        if(nearest<144*144)continue;
        for(int i=0;i<MAX_UNITS&&!reachable;i++)if(u[i].alive&&u[i].owner==SIDE&&
            u[i].raw_type==41&&u[i].order!=ORDER_BUILD) {
            int nx,ny;reachable=path_next_tile(u[i].x,u[i].y,x,y,&nx,&ny);
        }
        if(!reachable)continue;
        int score=field_d*2+dist2(x,y,b[hatch].x,b[hatch].y)/8-nearest/32;
        if(score<best){best=score;site_x=x;site_y=y;found=1;}
    }
    if(found){*out_x=site_x;*out_y=site_y;}
    return found;
}

/* After the first Sunken finishes, reserve a usable Drone and minerals for
   the second ordinary Creep Colony and morph before buying more Hydralisks. */
static int z7_progress_second_sunken(int worker_goal)
{
    int owned_sunkens=owned_building_count(B_SUNKEN_COLONY);
    int complete_sunkens=completed_building_count(B_SUNKEN_COLONY);
    if(owned_sunkens>=2)return 0;
    if(owned_sunkens!=1)return 0;
    /* Let Den placement proceed while the first Sunken is still morphing. */
    if(complete_sunkens<1)return 0;
    int colony=z7_fresh_creep_colony();
    if(colony<0) {
        if(worker_count(-1)>=2&&minerals>=75) {
            if(start_building(B_CREEP_COLONY,MENU_Z7_CREEP))return 1;
        }
        if(worker_count(-1)+queued_unit_count(U_DRONE)<worker_goal&&minerals>=50)
            return queue_hatchery_type(U_DRONE);
        return 0;
    }
    if(b[colony].under_construction) {
        (void)assign_idle_workers_to_minerals();
        return 0;
    }
    return start_sunken_colony();
}

static int z7_urgent_worker_evacuation(void)
{
    if((unsigned)frame<1000u||(unsigned)frame<z7_evacuation_retry_frame)return 0;
    int any_threat=0;
    for(int w=0;w<MAX_UNITS;w++)if(z7_worker_is_exposed(w)) {
        int threat=-1;
        /* Returning or carrying workers have no stable mineral target to
           evaluate. Check the worker itself against observed weapon reach. */
        int gas_only=u[w].order==ORDER_GAS||u[w].carry>0;
        if(!z7_worker_threat(w,gas_only,&threat))continue;
        any_threat=1;
        if(retreat_worker_individually(w,threat)) {
            workers_retreating=1;workers_safe_since=0;
            z7_evacuation_retry_frame=frame+24u;
            return 1;
        }
        /* A failed real input selection is bounded; do not spin on the same
           worker inside one host loop or starve the ordinary defense step. */
        z7_evacuation_retry_frame=frame+24u;
        return 0;
    }
    if(any_threat)return 0;
    /* Explain why gas carriers were deliberately left on their native order
       whenever an assigned mineral worker is under a qualifying threat. */
    int mineral_threat=0;
    for(int w=0;w<MAX_UNITS;w++)if(u[w].alive&&u[w].owner==SIDE&&u[w].raw_type==41&&
        u[w].order==ORDER_MINE) {int t=-1;if(z7_worker_threat(w,0,&t)){mineral_threat=1;break;}}
    if(mineral_threat)for(int w=0;w<MAX_UNITS;w++)
        if(u[w].alive&&u[w].owner==SIDE&&u[w].raw_type==41&&u[w].order==ORDER_GAS) {
            int t=-1,endangered=z7_worker_threat(w,1,&t),h=z7_townhall(SIDE);
            fprintf(stderr,"Z7 safe gas carrier frame%u slot%d carry%d target%d xy%d,%d homeD2%d threat%d threatened%d\n",
                (unsigned)frame,w,u[w].carry,u[w].target,u[w].x,u[w].y,
                h>=0?dist2(u[w].x,u[w].y,b[h].x,b[h].y):-1,t,endangered);
        }
    z7_evacuation_retry_frame=frame+24u;
    return 0;
}

static void macro_step(void)
{
    if(frame<next_macro_frame)return;
    next_macro_frame=frame+120u;
    int enemy_at_minerals=0;
    /* Only assigned patches and active worker trips can trigger evacuation;
       nearby unworked mineral fields do not strand safe miners. Keep a shorter
       worker-centered fallback for a hostile combat unit actually reaching a miner. */
    for(int w=0;w<MAX_UNITS&&!enemy_at_minerals;w++)
        if(z7_worker_is_exposed(w)) {
            int threat=-1;
            int worker_only=u[w].order==ORDER_GAS||u[w].carry>0;
            if(z7_worker_threat(w,worker_only,&threat))
                enemy_at_minerals=1;
        }
    /* Use live worker and mineral-patch proximity as the evacuation trigger;
       a broad Hatchery radius can strand otherwise productive Drones. */
    int home=z7_townhall(SIDE);
    /* Only pay for the Lair/Spire/Muta route after at most three remaining
       enemy structures have no ground-combat path; all actions use normal UI. */
    if((unsigned)frame>=100000u&&building_count(ENEMY)<=3&&
       !campaign_any_ground_combat_can_reach_enemy_building()) {
        if(!late_air_breakthrough_active) {
            late_air_breakthrough_active=1;muta_scout_leg=0;
            fprintf(stderr,"Z7 activate late air breakthrough: enemy buildings%d all ground paths blocked frame%u bank%d/%d\n",
                building_count(ENEMY),(unsigned)frame,minerals,gas);
        }
        int lair=raw_building(132,SIDE);
        if(lair<0) {
            if(minerals>=150&&gas>=100&&start_lair_morph())return;
            static unsigned lair_retry_log;
            if(!lair_retry_log||(unsigned)frame-lair_retry_log>=600u) {
                fprintf(stderr,"Z7 late air waiting paid Lair bank/menu frame%u bank%d/%d\n",
                    (unsigned)frame,minerals,gas);lair_retry_log=(unsigned)frame;
            }
            return;
        }
        if(!building_complete(lair))return;
        if(!owned_building_count(B_SPIRE)) {
            if(minerals>=200&&gas>=150&&worker_count(-1)>=3&&
               start_building(B_SPIRE,MENU_ZP_SPIRE))return;
            static unsigned spire_retry_log;
            if(!spire_retry_log||(unsigned)frame-spire_retry_log>=600u) {
                fprintf(stderr,"Z7 late air waiting paid Spire bank/site/menu frame%u bank%d/%d workers%d\n",
                    (unsigned)frame,minerals,gas,worker_count(-1));spire_retry_log=(unsigned)frame;
            }
            return;
        }
        int spire=raw_building(sc64_race_raw_building(B_SPIRE),SIDE);
        if(spire<0||!building_complete(spire))return;
        int live=unit_count(-1,-1),queued=queued_unit_count(U_MUTALISK);
        if(unit_type_count(U_MUTALISK,SIDE)+queued<8&&live+queued<MAX_UNITS&&
           minerals>=100&&gas>=100&&queue_hatchery_type(U_MUTALISK))return;
        static unsigned muta_queue_log;
        if(!muta_queue_log||(unsigned)frame-muta_queue_log>=600u) {
            fprintf(stderr,"Z7 late air Muta queue gate frame%u live-all%d cap%d live-muta%d queued%d bank%d/%d larva%d\n",
                (unsigned)frame,live,MAX_UNITS,unit_type_count(U_MUTALISK,SIDE),queued,
                minerals,gas,home>=0?b[home].larva:0);muta_queue_log=(unsigned)frame;
        }
        return;
    }
    int hatch_egg_producer=home>=0&&
        (building_complete(home)||race_z7_morphing_egg_hall(SIDE,home));
    int worker_goal=hatch_egg_producer?8:3;
    /* Scale workers after either the second defense or a paid expansion. */
    if(hatch_egg_producer&&(completed_building_count(B_SUNKEN_COLONY)>=2||
       owned_building_count(B_HATCHERY)>=2))
        worker_goal=12;
    macro_worker_goal=worker_goal;
    /* Stabilize the ordinary-input opening before the legacy defensive macro
       can spend its starting bank on optional colonies and air tech. */
    if(home>=0&&(unsigned)frame<30000u) {
        int pool=raw_building(142,SIDE);
        int den=raw_building(135,SIDE);
        int extractor=raw_building(149,SIDE);
        int live_workers=worker_count(-1);
        int pending_workers=queued_unit_count(U_DRONE);
        if(!building_complete(home))return;
        if(live_workers+pending_workers<4&&minerals>=50&&
           queue_hatchery_type(U_DRONE))return;
        /* Do not wait for the Den or two Sunken defenses: seize the cleared
           local geyser through ordinary builder/menu inputs while it is safe. */
        if(geyser_secured&&!owned_building_count(B_EXTRACTOR)) {
            static unsigned last_early_gas_diag;
            if(!last_early_gas_diag||frame-last_early_gas_diag>=300u) {
                int nearest=0x7fffffff,enemy=-1;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
                   currently_visible(u[i].x,u[i].y)&&sc64_race_unit_damage(u[i].type)>0) {
                    int d=dist2(832,1584,u[i].x,u[i].y);
                    if(d<nearest){nearest=d;enemy=i;}
                }
                fprintf(stderr,"Z7 private early gas gate frame%u home%d complete%d workers%d bank%d/%d safe%d ext%d pool%d/%d den%d/%d nearestEnemy%d raw%d d2%d\n",
                    (unsigned)frame,home,building_complete(home),live_workers,minerals,gas,
                    z7_local_extractor_safe(),owned_building_count(B_EXTRACTOR),pool,
                    building_complete(pool),den,building_complete(den),enemy,
                    enemy>=0?u[enemy].raw_type:-1,nearest);
                last_early_gas_diag=frame;
            }
        }
        if(geyser_secured&&!owned_building_count(B_EXTRACTOR)&&
           live_workers>=4&&minerals>=50&&z7_local_extractor_safe()) {
            if(start_building(B_EXTRACTOR,MENU_REFINERY))return;
        }
        { int early_extractor=raw_building(149,SIDE);
          if(early_extractor>=0&&building_complete(early_extractor)&&
             gas_worker_count()<2&&worker_count(ORDER_MINE)>=3&&assign_gas_worker())return; }
        if(!owned_building_count(B_SPAWNING_POOL)) {
            if(live_workers>=3&&minerals>=200)
                (void)start_building(B_SPAWNING_POOL,MENU_BARRACKS);
            return;
        }
        if(!building_complete(pool))return;
        /* Buy the native Den before optional static defense so the starting
           200 gas remains available for six additional Hydralisks. */
        if(!owned_building_count(B_HYDRALISK_DEN)) {
            if(live_workers>=3&&minerals>=100)
                (void)start_building(B_HYDRALISK_DEN,MENU_ACADEMY);
            return;
        }
        if(!building_complete(den))return;
        /* Private economic trial: start the ordinary Extractor as soon as
           the cleared geyser is safe, then staff it while retaining miners. */
        if(geyser_secured&&!owned_building_count(B_EXTRACTOR)&&
           live_workers>=4&&minerals>=50) {
            int safe=z7_local_extractor_safe();
            static unsigned last_gas_gate_report;
            if(!safe&&(!last_gas_gate_report||frame-last_gas_gate_report>=300u)) {
                fprintf(stderr,"Z7 private gas gate blocked frame%u workers%d bank%d/%d\n",
                    (unsigned)frame,live_workers,minerals,gas);
                last_gas_gate_report=frame;
            }
            if(safe&&start_building(B_EXTRACTOR,MENU_REFINERY))return;
        }
        if(extractor>=0&&building_complete(extractor)&&gas_worker_count()<2&&
           worker_count(ORDER_MINE)>=3&&assign_gas_worker())return;
        /* Economic trial: raise the renewable worker floor before any paid
           expansion or optional combat queue can consume the bank. */
        if(live_workers+pending_workers<8&&minerals>=50&&
           queue_hatchery_type(U_DRONE))return;
        int supply_cap=race_owner_supply_cap(SIDE);
        int supply_used=race_owner_reserved_supply(SIDE);
        if(supply_cap>0&&supply_cap-supply_used<=4&&
           queued_unit_count(U_OVERLORD)==0&&minerals>=100&&
           queue_hatchery_type(U_OVERLORD))return;
        /* Test one paid expansion only after two completed defenses and a
           usable ten-worker line; keep the bank for worker recovery first. */
        int expansion_hatches=owned_building_count(B_HATCHERY);
        int expansion_sunkens=completed_building_count(B_SUNKEN_COLONY);
        static unsigned last_expansion_diag;
        if((unsigned)frame>=9000u&&(!last_expansion_diag||
           (unsigned)frame-last_expansion_diag>=600u)) {
            fprintf(stderr,"Z7 expansion gate frame%u home%d hatch%d sunken%d workers%d pending%d bank%d threat%d retreat%d\n",
                (unsigned)frame,home,expansion_hatches,expansion_sunkens,
                live_workers,pending_workers,minerals,enemy_at_minerals,
                workers_retreating);
            last_expansion_diag=(unsigned)frame;
        }
        if(0&&expansion_hatches==1&&expansion_sunkens>=2&&live_workers>=10&&
           (!early_gas_policy||(expansion_sunkens>=2&&combat_count(SIDE)>=20&&
                               !enemy_at_minerals&&!workers_retreating))) {
            if(minerals>=300) {
                static unsigned last_expansion_attempt;
                if(!last_expansion_attempt||
                   (unsigned)frame-last_expansion_attempt>=600u) {
                    unsigned begin=(unsigned)frame;
                    last_expansion_attempt=begin;
                    int accepted=start_building(B_HATCHERY,MENU_DEPOT);
                    fprintf(stderr,"Z7 expansion Hatch input accepted%d frame%u->%u hatch%d->%d workers%d bank%d/%d threat%d retreat%d\n",
                        accepted,begin,(unsigned)frame,expansion_hatches,
                        owned_building_count(B_HATCHERY),live_workers,minerals,
                        gas,enemy_at_minerals,workers_retreating);
                    if(accepted)return;
                }
            } else {
                static unsigned last_expansion_reserve;
                if(!last_expansion_reserve||
                   (unsigned)frame-last_expansion_reserve>=600u) {
                    fprintf(stderr,"Z7 expansion reserve wait frame%u need300 bank%d workers%d threat%d retreat%d\n",
                        (unsigned)frame,minerals,live_workers,
                        enemy_at_minerals,workers_retreating);
                    last_expansion_reserve=(unsigned)frame;
                }
                return;
            }
        }
        if(live_workers+pending_workers<8&&minerals>=50&&
           queue_hatchery_type(U_DRONE))return;
        /* Correct the source's position-relative autoassignment after the
           six-worker line exists; keep the protected eastern mineral band. */
        if(live_workers>=6&&completed_building_count(B_HYDRALISK_DEN)>=1&&
           z7_assign_near_hatch_miners(home,live_workers))return;
        /* Establish both ordinary ground defenses while the initial combat
           group holds; spend the finite starting gas on Hydras afterward. */
        int ground_defenses=owned_building_count(B_SUNKEN_COLONY);
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].under_construction&&
           (b[i].flags&SC64_Z7_SUNKEN_MORPH_FLAG))ground_defenses++;
        if(ground_defenses<1) {
            int creep=z7_fresh_creep_colony();
            if(creep<0) {
                if(live_workers>=3&&minerals>=75)
                    (void)start_building(B_CREEP_COLONY,MENU_Z7_CREEP);
                return;
            }
            if(!b[creep].under_construction) {
                (void)start_sunken_colony();
                return;
            }
            return;
        }
        /* After the first ground tank finishes, let the gas pay for a small
           Hydralisk reinforcement before reserving the second Sunken. */
        if(completed_building_count(B_SUNKEN_COLONY)>=2&&live_workers>=6&&
           unit_type_count(U_HYDRALISK,SIDE)+queued_unit_count(U_HYDRALISK)<16&&
           minerals>=75&&gas>=25&&queue_hatchery_type(U_HYDRALISK))return;

        if(!early_gas_policy&&gas==0&&extractor>=0&&building_complete(extractor)&&gas_worker_count()<2&&
           worker_count(ORDER_MINE)>=3) {
            if(assign_gas_worker())return;
        }
        /* Reserve one post-Den 125-mineral budget for a second legal Sunken:
           this protects the ground line without delaying the Pool, Den, first
           Sunken, or the initial eight Hydralisks. Keep the six-worker line
           established first, then wait for the full Creep-plus-morph cost. */
        int completed_sunkens=completed_building_count(B_SUNKEN_COLONY);
        int morphing_sunkens=0;
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
           b[i].under_construction&&(b[i].flags&SC64_Z7_SUNKEN_MORPH_FLAG))
            morphing_sunkens++;
        if(live_workers>=6&&completed_sunkens>=1) {
            int creep=z7_fresh_creep_colony();
            static int second_creep_state_logged;
            if(!second_creep_state_logged&&(unsigned)frame>=11000u) {
                for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
                   (b[i].raw_type==143||b[i].type==B_SUNKEN_COLONY))
                    fprintf(stderr,"Z7 ground defense witness slot%d raw%d type%d hp%d/%d progress%d under%d flags%x xy%d,%d\n",
                        i,b[i].raw_type,b[i].type,b[i].hp,b[i].maxhp,b[i].build_progress,
                        b[i].under_construction,b[i].flags,b[i].x,b[i].y);
                second_creep_state_logged=1;
            }
            if(morphing_sunkens) return;
            if(completed_sunkens<2) {
                if(creep<0) {
                    static unsigned last_second_sunken_wait;
                    if((unsigned)frame>=9000u&&(!last_second_sunken_wait||
                       (unsigned)frame-last_second_sunken_wait>=600u)) {
                        fprintf(stderr,"Z7 second-Sunken budget wait frame%u bank%d/%d workers%d hydras%d\n",
                            (unsigned)frame,minerals,gas,live_workers,
                            unit_type_count(U_HYDRALISK,SIDE)+queued_unit_count(U_HYDRALISK));
                        last_second_sunken_wait=frame;
                    }
                    if(minerals>=125&&start_building(B_CREEP_COLONY,MENU_Z7_CREEP))return;
                    return;
                }
                if(b[creep].under_construction)return;
                if(minerals<50)return;
                if(start_sunken_colony())return;
                return;
            }
        }
        /* Scale the paid mineral line after the two ordinary Sunken defenses
           finish, and replace workers before spending that income on Hydras. */
        if(completed_sunkens>=2&&live_workers+pending_workers<worker_goal&&
           minerals>=50&&queue_hatchery_type(U_DRONE))return;
        /* Continue Hydra production after the reserved defense is complete. */
        if(unit_type_count(U_HYDRALISK,SIDE)+queued_unit_count(U_HYDRALISK)<24&&
           minerals>=75&&gas>=25&&queue_hatchery_type(U_HYDRALISK))return;
        return;
    }
    /* The recovered gas node is independent of townhall morph completion.
       Start its ordinary paid Extractor before any defense/tech/morph return,
       then assign gas only when two mineral workers remain so one keeps mining. */
    if(gas==0&&geyser_secured) {
        if(z7_local_extractor_safe()&&!owned_building_count(B_EXTRACTOR)&&worker_count(-1)>=2&&
           worker_count(ORDER_MINE)>=2&&minerals>=50) {
            if(start_building(B_EXTRACTOR,MENU_REFINERY))return;
            /* If site legality still blocks the paid Extractor, queue one
               replacement Drone only while retaining its full 50-mineral cost. */
            if(home>=0&&race_z7_morphing_egg_hall(SIDE,home)&&
               worker_count(-1)+queued_unit_count(U_DRONE)<4&&minerals>=100&&
               queue_hatchery_type(U_DRONE))return;
            return;
        }
        int extractor=raw_building(149,SIDE);
        if(extractor>=0&&building_complete(extractor)&&gas_worker_count()<2&&
           worker_count(ORDER_MINE)>=2&&assign_gas_worker())return;
    }
    /* A finished ground Creep with a completed Pool is immediately morphable.
       Do that before worker replacement, army selection, or raid decisions;
       failed UI selection is logged and retried only after a cooldown. */
    static unsigned urgent_sunken_retry_frame;
    static unsigned urgent_sunken_last_status;
    int pool_for_sunken=raw_building(142,SIDE);
    int spore_for_sunken=raw_building(144,SIDE);
    int spore_morph_for_sunken=z7_spore_morph_slot();
    int ready_creep_for_sunken=z7_fresh_creep_colony();
    int active_sunkens=completed_building_count(B_SUNKEN_COLONY);
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].under_construction&&
        (b[i].flags&SC64_Z7_SUNKEN_MORPH_FLAG))++active_sunkens;
    int ready_to_morph=(unsigned)frame<18000u&&
        (spore_for_sunken>=0||spore_morph_for_sunken>=0)&&
        pool_for_sunken>=0&&building_complete(pool_for_sunken)&&
        active_sunkens<2&&ready_creep_for_sunken>=0&&
        !b[ready_creep_for_sunken].under_construction&&minerals>=50;
    if(ready_to_morph) {
        if(!urgent_sunken_last_status||frame-urgent_sunken_last_status>=600u) {
            fprintf(stderr,"Z7 Sunken-ready witness frame%u colony%d xy%d,%d flags%x pool%d complete%d spore%d sporeMorph%d sunkenCount%d minerals%d workers%d mode%d menu%d\n",
                (unsigned)frame,ready_creep_for_sunken,b[ready_creep_for_sunken].x,
                b[ready_creep_for_sunken].y,b[ready_creep_for_sunken].flags,
                pool_for_sunken,building_complete(pool_for_sunken),spore_for_sunken,
                spore_morph_for_sunken,active_sunkens,minerals,worker_count(-1),mode,menu);
            urgent_sunken_last_status=frame;
        }
        if((unsigned)frame>=urgent_sunken_retry_frame) {
            unsigned attempt_begin=frame;
            int attempted=start_sunken_colony();
            fprintf(stderr,"Z7 urgent Sunken dispatch result%d begin%u end%u elapsed%u readyColony%d pool%d complete%d minerals%d mode%d menu%d\n",
                attempted,attempt_begin,(unsigned)frame,(unsigned)frame-attempt_begin,
                ready_creep_for_sunken,pool_for_sunken,building_complete(pool_for_sunken),
                minerals,mode,menu);
            if(attempted){urgent_sunken_retry_frame=0;return;}
            urgent_sunken_retry_frame=frame+600u;
        }
    }
    /* Preserve the opening worker minimum during a paid Lair/Hive morph. The
       producer gate below uses the same exact native morph predicate as Egg
       admission; keep this to four live+queued workers until the hall completes. */
    if(home>=0&&race_z7_morphing_egg_hall(SIDE,home)&&
       worker_count(-1)+queued_unit_count(U_DRONE)<worker_goal&&
       worker_count(-1)+queued_unit_count(U_DRONE)<4&&minerals>=100&&
       queue_hatchery_type(U_DRONE))return;
    /* Once the first paid Sunken order is in flight or complete, prioritize
       the air-tech chain before routine worker, Evo/Creep, second-Sunken, or
       Den spending. The first defense has already consumed its native cost;
       no extra mineral/gas is reserved for optional follow-on defense here. */
    int first_sunken_started=active_sunkens>0||
        completed_building_count(B_SUNKEN_COLONY)>0;
    int tech_window=(unsigned)frame<18000u&&home>=0&&building_complete(home)&&
        completed_building_count(B_SPAWNING_POOL)>0&&
        completed_building_count(B_SPORE_COLONY)>0&&first_sunken_started&&
        completed_building_count(B_SUNKEN_COLONY)>=2&&
        building_complete(raw_building(135,SIDE))&&worker_count(-1)>=6;
    if(tech_window) {
        int lair=raw_building(132,SIDE);
        if(lair<0) {
            if(minerals>=150&&gas>=100&&start_lair_morph())return;
            /* Hold discretionary purchases while the Lair bank is being
               rebuilt. This avoids repeatedly paying for workers or a
               second defense just before reaching the native 150 mineral
               cost; the first Sunken order has already been accepted. */
            static unsigned last_lair_bank_log;
            if(!last_lair_bank_log||(unsigned)frame-last_lair_bank_log>=300u) {
                fprintf(stderr,"Z7 bank for Lair frame%u bank%d/%d workers%d queue%d\n",
                    (unsigned)frame,minerals,gas,worker_count(-1),
                    home>=0?b[home].queue_count:-1);
                last_lair_bank_log=frame;
            }
            return;
        }
        if(lair>=0&&building_complete(lair)&&!owned_building_count(B_SPIRE)&&
           minerals>=200&&gas>=150&&worker_count(-1)>=3&&
           start_building(B_SPIRE,MENU_ZP_SPIRE))return;
        /* Each accepted call creates a separate paid pair Egg. Continue on
           later macro turns so independent pairs train concurrently. */
        if(completed_building_count(B_SPIRE)>0&&scourge_outputs_ordered<6&&
           minerals>=25&&gas>=75) {
            int before=queued_unit_count(U_SCOURGE);
            if(queue_hatchery_type(U_SCOURGE)) {
                int output=queued_unit_count(U_SCOURGE)-before;
                scourge_outputs_ordered+=output>0?output:2;
                fprintf(stderr,"Z7 priority Scourge pair queued outputs%d total%d frame%u bank%d/%d\n",
                    output,scourge_outputs_ordered,(unsigned)frame,minerals,gas);
                return;
            }
        }
    }
    /* Keep 125 minerals for a future Creep+Sunken, or 50 when its Creep
       already exists. This lets the Lair/Spire clocks overlap the second
       ground defense without consuming its ordinary paid morph. */
    if((unsigned)frame<15000u&&home>=0&&building_complete(home)&&
       completed_building_count(B_SPAWNING_POOL)>0&&
       completed_building_count(B_SPORE_COLONY)>0&&first_sunken_started&&
       completed_building_count(B_SUNKEN_COLONY)>=2&&
       building_complete(raw_building(135,SIDE))&&worker_count(-1)>=6) {
        int sunken_reserve=active_sunkens>=2?0:z7_fresh_creep_colony()>=0?50:125;
        if(geyser_secured&&!owned_building_count(B_EXTRACTOR)&&
           worker_count(-1)>=2&&minerals>=50&&
           start_building(B_EXTRACTOR,MENU_REFINERY))return;
        int lair=raw_building(132,SIDE);
        int any_paid_product=queued_unit_count(U_DRONE)||queued_unit_count(U_OVERLORD)||
            queued_unit_count(U_ZERGLING)||queued_unit_count(U_HYDRALISK)||
            queued_unit_count(U_MUTALISK)||queued_unit_count(U_SCOURGE);
        if(lair<0&&minerals>=150+sunken_reserve&&gas>=100&&!any_paid_product&&
           start_lair_morph())return;
        if(lair>=0&&building_complete(lair)&&!owned_building_count(B_SPIRE)&&
           minerals>=200+sunken_reserve&&gas>=150&&worker_count(-1)>=3&&
           start_building(B_SPIRE,MENU_ZP_SPIRE))return;
        if(completed_building_count(B_SPIRE)>0&&scourge_outputs_ordered<6&&
           minerals>=25&&gas>=75) {
            int before=queued_unit_count(U_SCOURGE);
            if(queue_hatchery_type(U_SCOURGE)) {
                int output=queued_unit_count(U_SCOURGE)-before;
                scourge_outputs_ordered+=output>0?output:2;
                fprintf(stderr,"Z7 paid Scourge pair queued outputs%d total%d frame%u bank%d/%d\n",
                    output,scourge_outputs_ordered,(unsigned)frame,minerals,gas);
                return;
            }
        }
    }
    /* Evacuate only for a threat close to a live worker or an actual mineral
       patch. A broad 300px Hatchery radius emptied this line while incoming
       air was still outside defender weapon range. */
    if(!enemy_at_minerals)for(int w=0;w<MAX_UNITS&&!enemy_at_minerals;w++)
        if(u[w].alive&&u[w].owner==SIDE&&u[w].raw_type==41)
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
                currently_visible(u[i].x,u[i].y)&&
                sc64_race_unit_damage(u[i].type)>0&&
                dist2(u[w].x,u[w].y,u[i].x,u[i].y)<128*128) {
                enemy_at_minerals=1;break;
            }
    /* While a second ground Creep is still being built, pre-position the
       normal cursor at its site during a safe window. This hides cursor travel
       inside the construction wait instead of delaying its first Sunken
       command once it becomes ready. */
    int prep_pool=raw_building(142,SIDE);
    int prep_spore=raw_building(144,SIDE);
    int prep_spore_morph=z7_spore_morph_slot();
    int prep_creep=z7_fresh_creep_colony();
    if(!enemy_at_minerals&&worker_count(-1)>=worker_goal&&
       (prep_spore>=0||prep_spore_morph>=0)&&prep_pool>=0&&
       building_complete(prep_pool)&&prep_creep>=0&&b[prep_creep].under_construction&&
       dist2(cx,cy,b[prep_creep].x,b[prep_creep].y)>64*64) {
        unsigned begin=(unsigned)frame;
        fprintf(stderr,"Z7 pending Creep cursor preposition begin frame%u slot%d xy%d,%d cursor%d,%d workers%d pool%d spore%d morph%d\n",
            begin,prep_creep,b[prep_creep].x,b[prep_creep].y,cx,cy,
            worker_count(-1),prep_pool,prep_spore,prep_spore_morph);
        move_cursor(b[prep_creep].x,b[prep_creep].y);
        fprintf(stderr,"Z7 pending Creep cursor preposition end frame%u elapsed%u slot%d cursor%d,%d\n",
            (unsigned)frame,(unsigned)frame-begin,prep_creep,cx,cy);
        return;
    }
    static unsigned last_threat_probe;
    if((unsigned)frame>=6500u&&(!last_threat_probe||
       (unsigned)frame-last_threat_probe>=600u)) {
        int nearest=0x7fffffff,nearest_slot=-1;
        if(home>=0)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
            currently_visible(u[i].x,u[i].y)&&
            sc64_race_unit_damage(u[i].type)>0) {
            int d=dist2(b[home].x,b[home].y,u[i].x,u[i].y);
            if(d<nearest){nearest=d;nearest_slot=i;}
        }
        fprintf(stderr,"Z7 threat probe frame%u flag%d home%d xy%d,%d nearest%d d2%d workers%d\n",
            (unsigned)frame,enemy_at_minerals,home,home>=0?b[home].x:-1,home>=0?b[home].y:-1,
            nearest_slot,nearest,worker_count(-1));
        last_threat_probe=(unsigned)frame;
    }
    /* Build the Evo/Creep/Spore chain first. Start the Pool during the Spore
       morph clock, then add a separate Sunken and Den before clearing the
       optional source Extractor. All structures use ordinary UI commands. */
    if((unsigned)frame<18000u&&home>=0&&building_complete(home)) {
        /* Launch the 1,200-update Pool clock immediately. The Evo/Creep/Spore
           line and both Sunken defenses remain paid, but Lair prerequisites
           no longer wait for the whole Spore morph. */
        if(!owned_building_count(B_SPAWNING_POOL)&&minerals>=200&&
           start_building(B_SPAWNING_POOL,MENU_BARRACKS))return;
        int completed_ground_colony=-1;
        if(owned_building_count(B_SPORE_COLONY)&&!owned_building_count(B_SUNKEN_COLONY))
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
                b[i].type==B_CREEP_COLONY&&b[i].raw_type==143&&!b[i].under_construction) {
                completed_ground_colony=i;break;
            }
        /* Morphing a completed Creep Colony is free of a Drone and should
           take priority over replacing workers already lost to the wave. */
        if(completed_ground_colony<0&&worker_count(-1)+queued_unit_count(U_DRONE)<worker_goal&&minerals>=50) {
            if(queue_hatchery_type(U_DRONE))return;
        }
        if(!owned_building_count(B_EVOLUTION_CHAMBER)) {
            (void)start_building(B_EVOLUTION_CHAMBER,MENU_ENG_BAY);return;
        }
        int evo=raw_building(139,SIDE);
        if(evo>=0&&!building_complete(evo)) {
            (void)assign_idle_workers_to_minerals();return;
        }
        if(!owned_building_count(B_CREEP_COLONY)&&!owned_building_count(B_SPORE_COLONY)) {
            (void)start_building(B_CREEP_COLONY,MENU_Z7_CREEP);return;
        }
        int spore_morph=z7_spore_morph_slot();
        int colony=raw_building(143,SIDE);
        /* An in-place Spore still reports raw143 while morphing. Do not let
           the generic unfinished-Creep wait swallow its parallel branch. */
        if(colony>=0&&b[colony].under_construction&&colony!=spore_morph) {
            (void)assign_idle_workers_to_minerals();return;
        }
        if(colony>=0&&!owned_building_count(B_SPORE_COLONY)) {
            if(start_spore_colony())return;
        }
        if(spore_morph>=0) {
            if(!owned_building_count(B_SPAWNING_POOL)) {
                if(start_building(B_SPAWNING_POOL,MENU_BARRACKS))return;
            }
            int fresh_creep=z7_fresh_creep_colony();
            if(fresh_creep<0&&worker_count(-1)>=2&&minerals>=75) {
                fprintf(stderr,"Z7 parallel Creep accepted during Spore morph slot%d at%d,%d frame%u workers%d bank%d\n",
                    spore_morph,b[spore_morph].x,b[spore_morph].y,(unsigned)frame,
                    worker_count(-1),minerals);
                if(start_building(B_CREEP_COLONY,MENU_Z7_CREEP))return;
            }
            /* Creep placement is legal before Pool completion; only its
               Sunken morph waits for Pool. Let the general macro continue
               while either independent construction clock advances. */
            if(!owned_building_count(B_SPAWNING_POOL))
                (void)start_building(B_SPAWNING_POOL,MENU_BARRACKS);
        }
        int spore=raw_building(144,SIDE);
        if(spore>=0&&b[spore].under_construction) {
            if(!owned_building_count(B_SPAWNING_POOL)&&
               start_building(B_SPAWNING_POOL,MENU_BARRACKS))return;
            /* Keep placing the second ordinary Creep during Spore/Pool
               construction; do not treat a worker already building it as
               a reason to starve the Den or economy branch. */
            if(z7_fresh_creep_colony()<0&&worker_count(-1)>=2&&minerals>=75&&
               start_building(B_CREEP_COLONY,MENU_Z7_CREEP))return;
        }
        if(spore>=0) {
            if(!owned_building_count(B_SPAWNING_POOL)) {
                if(start_building(B_SPAWNING_POOL,MENU_BARRACKS))return;
            }
            int pool=raw_building(142,SIDE);
            int sunken_morph_active=0;
            for(int i=0;i<MAX_BUILDINGS;i++)
                if(b[i].alive&&b[i].owner==SIDE&&
                   (b[i].flags&SC64_Z7_SUNKEN_MORPH_FLAG)) {
                    sunken_morph_active=1;break;
                }
            if(owned_building_count(B_SUNKEN_COLONY)==0&&!sunken_morph_active) {
                int ground_colony=z7_fresh_creep_colony();
                if(ground_colony<0) {
                    if(worker_count(-1)>=2&&minerals>=75&&
                       start_building(B_CREEP_COLONY,MENU_Z7_CREEP))return;
                }
                if(ground_colony>=0&&!b[ground_colony].under_construction&&
                   pool>=0&&building_complete(pool)&&start_sunken_colony())return;
                if(ground_colony>=0&&b[ground_colony].under_construction) {
                    /* The colony can build while Pool completes; Den may
                       also start once Pool is ready. */
                }
            }
            /* After the first Sunken is complete, start and morph the second
               ground defense before spending a Drone on Den placement. Once
               the second morph has actually started, Den construction can
               overlap its morph clock. */
            int second_sunken_morphing=0;
            for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].under_construction&&
                (b[i].flags&SC64_Z7_SUNKEN_MORPH_FLAG)) {second_sunken_morphing=1;break;}
            if(!second_sunken_morphing&&z7_progress_second_sunken(worker_goal))return;
            if(!owned_building_count(B_HYDRALISK_DEN)) {
                /* Preserve enough minerals to place and morph the second
                   defense if its Creep has not been paid for yet. */
                int reserve=z7_fresh_creep_colony()<0?125:50;
                if(pool>=0&&building_complete(pool)&&minerals>=225+reserve&&
                   start_building(B_HYDRALISK_DEN,MENU_ACADEMY))return;
            }
            int den=raw_building(135,SIDE);
            if(den>=0&&!building_complete(den)) {
                if(z7_progress_second_sunken(worker_goal))return;
                (void)assign_idle_workers_to_minerals();return;
            }
            if(z7_progress_second_sunken(worker_goal))return;
            if(unit_type_count(U_HYDRALISK,SIDE)+queued_unit_count(U_HYDRALISK)<12&&
               minerals>=75&&gas>=25&&queue_hatchery_type(U_HYDRALISK))return;
        }
    }
    /* The first hostile air group can arrive before a paid colony finishes.
       Spend the available gas on legal Hydralisks as soon as the Den is
       complete, even if that first Creep Colony is still being built. */
    if(enemy_at_minerals&&building_complete(raw_building(135,SIDE))&&
       unit_type_count(U_HYDRALISK,SIDE)+queued_unit_count(U_HYDRALISK)<12&&
       minerals>=75&&gas>=25&&queue_hatchery_type(U_HYDRALISK))return;
    /* The Den can finish while a second Drone is still available. Start the
       paid Extractor on the cleared restored node during that interval rather
       than letting Drone production or later colony placement defer it. */
    if(!owned_building_count(B_EXTRACTOR)&&geyser_secured&&
       raw_building(135,SIDE)>=0&&worker_count(-1)>=2&&minerals>=50&&
       start_building(B_EXTRACTOR,MENU_REFINERY))return;
    /* Per-worker evacuation now runs before macro production/return branches. */
    /* Zerg building construction consumes its Drone. Replace those workers
       before starting another paid structure, otherwise the safe restored
       Extractor ends up with no miners after the Den and colony jobs. */
    static unsigned last_worker_goal_log;
    if((unsigned)frame>=5600u&&(!last_worker_goal_log||
       (unsigned)frame-last_worker_goal_log>=600u)) {
        fprintf(stderr,"Z7 worker economy frame%u goal%d live%d queued%d gas%d extractor%d complete%d hatch%d hatch_complete%d bank%d/%d\n",
            (unsigned)frame,worker_goal,worker_count(-1),queued_unit_count(U_DRONE),
            gas_worker_count(),owned_building_count(B_EXTRACTOR),
            completed_building_count(B_EXTRACTOR),z7_townhall(SIDE),
            building_complete(z7_townhall(SIDE)),minerals,gas);
        last_worker_goal_log=(unsigned)frame;
    }
    int live_workers=worker_count(-1),queued_drones=queued_unit_count(U_DRONE);
    int home_hatch=z7_townhall(SIDE);
    int home_complete=building_complete(home_hatch);
    int need_worker=live_workers+queued_drones<worker_goal;
    if((unsigned)frame>=5600u&&last_worker_goal_log==((unsigned)frame))
        fprintf(stderr,"Z7 worker decision need%d complete%d minerals%d live%d queued%d\n",
            need_worker,home_complete,minerals,live_workers,queued_drones);
    if(need_worker&&home_complete&&minerals>=50) {
        int queued=queue_hatchery_type(U_DRONE);
        if(queued)return;
        static unsigned last_worker_goal_failure;
        if((unsigned)frame>=5600u&&(!last_worker_goal_failure||
           (unsigned)frame-last_worker_goal_failure>=600u)) {
            fprintf(stderr,"Z7 worker replacement not admitted frame%u goal%d live%d queued%d minerals%d larva%d mode%d menu%d\n",
                (unsigned)frame,worker_goal,worker_count(-1),queued_unit_count(U_DRONE),
                minerals,z7_townhall(SIDE)>=0?b[z7_townhall(SIDE)].larva:-1,mode,menu);
            last_worker_goal_failure=(unsigned)frame;
        }
    }
    /* Production orders still work while workers are being evacuated. Keep
       turning guarded starting gas into Hydralisks instead of freezing the
       army macro during a mineral-line threat. */
    if(unit_type_count(U_HYDRALISK,SIDE)+queued_unit_count(U_HYDRALISK)<12&&
       building_complete(raw_building(135,SIDE))&&gas>=25&&
       (raw_building(143,SIDE)>=0||raw_building(146,SIDE)>=0)&&
       minerals>=(owned_building_count(B_SUNKEN_COLONY)<2?150:75)) {
        if(queue_hatchery_type(U_HYDRALISK))return;
    }
    {
        int morphing=0,ordinary=0;
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE) {
            if(b[i].raw_type==146&&b[i].under_construction)morphing=1;
            else if(b[i].raw_type==143)ordinary++;
        }
        if(morphing&&ordinary==0&&owned_building_count(B_SUNKEN_COLONY)<2&&
           worker_count(-1)>=2&&minerals>=75&&
           start_building(B_CREEP_COLONY,MENU_Z7_CREEP))return;
    }
    /* Preserve the second ordinary defense: when the first Sunken finishes
       at the same time the mineral line becomes threatened, place the next
       Creep Colony through the normal worker/build menu before evacuating.
       Later, morph that completed colony through the normal Sunken menu. */
    if(enemy_at_minerals&&owned_building_count(B_SUNKEN_COLONY)==1&&
       raw_building(143,SIDE)<0&&worker_count(-1)>=2&&minerals>=125&&
       start_building(B_CREEP_COLONY,MENU_Z7_CREEP))return;
    if(enemy_at_minerals&&owned_building_count(B_SUNKEN_COLONY)<2&&
       start_sunken_colony())return;
    /* When the mineral line is threatened, keep converting spare minerals
       into legal paired Zerglings even if the finite starting gas is nearly
       exhausted. This remains an ordinary Hatchery queue action. */
    if(enemy_at_minerals&&building_complete(raw_building(135,SIDE))&&
       owned_building_count(B_SUNKEN_COLONY)>=2&&minerals>=50&&
       queue_hatchery_type(U_ZERGLING))return;
    if(workers_retreating) {
        if(enemy_at_minerals)workers_safe_since=0;
        else if(!workers_safe_since)workers_safe_since=(unsigned)frame;
        else if((unsigned)frame-workers_safe_since>=1200u) {
            workers_retreating=0;workers_safe_since=0;
            fprintf(stderr,"Z7 normal-input workers resume mining after safe interval frame%u\n",
                (unsigned)frame);
        }
    }
    /* Once the source attack reaches the mineral line, preserve remaining
       miners with ordinary per-worker Move orders. Keep any active builder
       on its source construction; do not select or mutate it here. */
    /* Normal player orders are required to put newly spawned Drones onto
       resource fields; do not count idle workers as an operating economy. */
    if(!workers_retreating&&assign_idle_workers_to_minerals())return;
    if(!owned_townhall_count()) {
        (void)start_building(B_HATCHERY,MENU_DEPOT);return;
    }
    { int h=z7_townhall(SIDE); if(h>=0&&!building_complete(h))return; }
    /* Once the nearby source gas node has been restored and verified clear,
       start its ordinary paid Extractor before the Pool/Den chain. This lets
       the opening gas arrive while the required tech buildings are being
       built instead of after the first scripted air pressure. */
    if(!owned_building_count(B_EXTRACTOR)&&geyser_secured&&
       worker_count(-1)>=2&&minerals>=50&&
       start_building(B_EXTRACTOR,MENU_REFINERY))return;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&
        b[i].type==B_HATCHERY&&!building_complete(i))return;
    if(!owned_building_count(B_SPAWNING_POOL)) {
        (void)start_building(B_SPAWNING_POOL,MENU_BARRACKS);return;
    }
    { int p=raw_building(142,SIDE); if(p>=0&&!building_complete(p))return; }
    /* Spend part of the authored starting gas before the 300-second enemy
       script begins.  The previous order waited for two colony morphs before
       the Den, so the first wave arrived while the initial army was still
       the only anti-air force.  Keep the two ordinary Sunken defenses, but
       start the Den as soon as the Pool completes. */
    if(!owned_building_count(B_HYDRALISK_DEN)) {
        (void)start_building(B_HYDRALISK_DEN,MENU_ACADEMY);return;
    }
    { int d=raw_building(135,SIDE); if(d>=0&&!building_complete(d)) {
        if(worker_count(-1)+queued_unit_count(U_DRONE)<4&&minerals>=50)
            (void)queue_hatchery_type(U_DRONE);
        /* Keep one worker on the restored gas node while the already-started
           Den finishes. Delaying Den placement to move several workers onto
           gas left no time to buy Hydras before the first air wave. */
        if(completed_building_count(B_EXTRACTOR)&&gas_worker_count()<1&&
           worker_count(ORDER_MINE)>=2&&assign_gas_worker())return;
        /* The restored gas node is already physically cleared. Use a second
           Drone to build its Extractor while the Den construction continues;
           serializing these two ordinary worker jobs leaves no gas income
           before the source's 300-second pressure wave. */
        if(!owned_building_count(B_EXTRACTOR)&&geyser_secured&&
           worker_count(-1)>=2&&minerals>=50&&
           start_building(B_EXTRACTOR,MENU_REFINERY))return;
        return;
    } }
    /* Spend the source starting gas on anti-air as soon as the Den completes,
       before using the same opening minerals on a ground-only Sunken defense. */
    if(building_complete(raw_building(135,SIDE))&&
       unit_type_count(U_HYDRALISK,SIDE)+queued_unit_count(U_HYDRALISK)<12&&
       minerals>=75&&gas>=25&&queue_hatchery_type(U_HYDRALISK))return;
    if(!owned_building_count(B_EXTRACTOR)&&geyser_secured&&
       worker_count(-1)>=2&&minerals>=50) {
        if(start_building(B_EXTRACTOR,MENU_REFINERY))return;
    }
    /* Assign one worker as soon as the Extractor is operational, then fill
       the second gas job as Drones become available. This runs before the
       extra colony/army queues so gas is accumulated before the hostile air
       wave without skipping any source building requirement. */
    if(owned_building_count(B_EXTRACTOR)&&building_complete(raw_building(149,SIDE))&&
       gas_worker_count()<2&&worker_count(ORDER_MINE)>=2) {
        (void)assign_gas_worker();return;
    }
    if(!owned_building_count(B_CREEP_COLONY)&&
       !owned_building_count(B_SUNKEN_COLONY)) {
        (void)start_building(B_CREEP_COLONY,MENU_Z7_CREEP);return;
    }
    { int c=raw_building(143,SIDE); if(c>=0&&b[c].under_construction) {
        /* Keep three Drones alive while this worker-built defense progresses;
           the paid-Hydralisk priority above spends the remaining gas. */
        if(worker_count(-1)+queued_unit_count(U_DRONE)<3&&minerals>=50)
            (void)queue_hatchery_type(U_DRONE);
        return;
    } }
    if(owned_building_count(B_SUNKEN_COLONY)<2&&
       owned_building_count(B_CREEP_COLONY)==0&&worker_count(-1)>=2&&minerals>=125) {
        (void)start_building(B_CREEP_COLONY,MENU_Z7_CREEP);return;
    }
    if(owned_building_count(B_SUNKEN_COLONY)<2&&start_sunken_colony())return;
    if(worker_count(-1)+queued_unit_count(U_DRONE)<4&&minerals>=50) {
        if(queue_hatchery_type(U_DRONE))return;
    }
    if(worker_count(-1)<4)return;
    if(!owned_building_count(B_HYDRALISK_DEN)) {
        (void)start_building(B_HYDRALISK_DEN,MENU_ACADEMY);return;
    }
    { int d=raw_building(135,SIDE); if(d>=0&&!building_complete(d))return; }
    if(worker_count(-1)+queued_unit_count(U_DRONE)<6&&minerals>=100) {
        if(queue_hatchery_type(U_DRONE))return;
    }
    /* Avoid spending the recovery bank on an expansion while the starting
       base and worker line are still under attack. */
    if(!z7_scourge_tech_pending()&&owned_building_count(B_HYDRALISK_DEN)&&minerals>=75) {
        /* Keep 75 minerals available for a Hydralisk while starting gas can
           still fund one. Once it cannot, spend on paired Zerglings. */
        if(gas>=25)(void)queue_hatchery_type(U_HYDRALISK);
        else (void)queue_hatchery_type(U_ZERGLING);
    }
}

static void attack_step(void)
{
    int bx,by,slot;
    int hatch=z7_townhall(SIDE);
    int guard_x=1050,guard_y=1620;
    /* Research policy: keep pursuing observed targets after the switch and
       periodically reselect to admit paid reinforcements. This has not won. */
    if(early_gas_policy&&campaign_assault_active) {
        static unsigned next_assault_refresh;
        if((unsigned)frame>=next_assault_refresh) {
            campaign_assault_target=-1;campaign_assault_is_building=0;
            campaign_assault_target_raw=0;
            next_assault_refresh=(unsigned)frame+600u;
        }
        (void)campaign_assault_retarget();return;
    }
    /* Controller strategy: let the home-cover opening build a stable line,
       then preserve Mutalisks for Guardian defense while Hydralisks attack. */
    int own_extractor=raw_building(149,SIDE);
    if(!early_gas_policy&&defense_staged&&!campaign_assault_active&&
       (unsigned)frame>=9000u&&worker_count(-1)>=6&&combat_count(SIDE)>=16&&
       own_extractor>=0&&building_complete(own_extractor)&&gas_worker_count()>=2) {
        campaign_assault_active=1;
        fprintf(stderr,"Z7 private delayed Hydra assault start frame%u workers%d combat%d\n",
            (unsigned)frame,worker_count(-1),combat_count(SIDE));
    }
    if(campaign_assault_active&&(unsigned)frame>=9000u) {
        (void)campaign_assault_retarget();return;
    }
    /* Clear the local approach before the native switch. Once the source wave
       starts, stop the assault and place the whole combat group at the north
       mineral boundary where it can meet Guardians before they reach workers. */
    if(campaign_assault_active&&(unsigned)frame<9000u) {
        (void)campaign_assault_retarget();return;
    }
    if(campaign_assault_active) {
        campaign_assault_active=0;campaign_assault_target=-1;
        campaign_assault_is_building=0;campaign_assault_target_raw=0;
        fprintf(stderr,"Z7 source switch: replace opening assault with north screen frame%u\n",(unsigned)frame);
    }
    if(!defense_staged&&frame>=100&&hatch>=0&&combat_count(SIDE)) {
        /* Controller strategy: keep the starting army on home worker cover
           instead of spending nine thousand frames on an early assault. */
        campaign_assault_active=0;
        defense_staged=1;
        defense_hydras_rallied=unit_type_count(U_HYDRALISK,SIDE);
        fprintf(stderr,"Z7 private home-cover opening frame%u army%d workers%d\n",
            (unsigned)frame,combat_count(SIDE),worker_count(-1));
        return;
    }
    if(defense_staged&&hatch>=0&&building_complete(hatch)&&
       combat_count(SIDE)>=6&&!defense_cover_set) {
        int cover_x,cover_y;
        z7_defense_screen_point(&cover_x,&cover_y);
        /* Keep the mixed screen on the Hatchery side of the mineral line.
           The earlier northward offset put Hydras inside Guardian range
           before the Mutalisks could establish air control. */
        int accepted=army_order(1,cover_x,cover_y);
        fprintf(stderr,"Z7 source-switch north-screen accepted%d xy%d,%d frame%u army%d workers%d\n",
            accepted,cover_x,cover_y,(unsigned)frame,combat_count(SIDE),worker_count(-1));
        if(accepted){
            defense_cover_set=1;defense_screen_combat_count=combat_count(SIDE);
            defense_screen_order_retry_frame=frame+180u;geyser_mutas_returned=1;return;
        }
    }
    /* The post-switch screen is an authoritative policy. Keep Hydralisks and
       Mutalisks together at the north mineral boundary; direct the air screen
       only at a visible Guardian or Mutalisk near that point, then regroup. */
    if(defense_staged&&defense_cover_set&&(unsigned)frame>=9000u&&hatch>=0) {
        int cover_x,cover_y,air_x=1050,air_y=1510;
        z7_defense_screen_point(&cover_x,&cover_y);
        /* Guardians can hit the mineral line from outside Mutalisk sight.
           Keep the Hydras on worker cover and advance only the air group to
           a nearby forward screen where it can see the ground-only threat. */
        if(!defense_air_screen_set&&(unsigned)frame>=defense_screen_order_retry_frame&&
           unit_type_count(U_MUTALISK,SIDE)>0&&
           defense_screen_enemy<0) {
            int accepted=typed_attack_move(U_MUTALISK,air_x,air_y);
            fprintf(stderr,"Z7 forward Mutalisk screen accepted%d xy%d,%d workerCover%d,%d frame%u mutas%d\n",
                accepted,air_x,air_y,cover_x,cover_y,(unsigned)frame,unit_type_count(U_MUTALISK,SIDE));
            if(accepted){defense_air_screen_set=1;defense_screen_order_retry_frame=frame+60u;return;}
            defense_screen_order_retry_frame=frame+60u;
            return;
        }
        int had_focus=defense_screen_enemy>=0;
        /* Controller strategy: identify and focus an observed Guardian as soon as
           it enters the forward screen, before ground weapon range threatens
           a Drone. */
        int guardian_threat=-1,guardian_best=300*300;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
           u[i].raw_type==44&&currently_visible(u[i].x,u[i].y)) {
            int d=dist2(air_x,air_y,u[i].x,u[i].y);
            if(d<=guardian_best) {guardian_threat=i;guardian_best=d;}
        }
        if(guardian_threat>=0)fprintf(stderr,"Z7 private Guardian pre-focus frame%u slot%d xy%d,%d d2%d\n",
            (unsigned)frame,guardian_threat,u[guardian_threat].x,u[guardian_threat].y,guardian_best);
        if(defense_screen_enemy>=0&&
           (defense_screen_enemy>=MAX_UNITS||!u[defense_screen_enemy].alive||
            u[defense_screen_enemy].owner!=ENEMY||
            !currently_visible(u[defense_screen_enemy].x,u[defense_screen_enemy].y)||
            dist2(air_x,air_y,u[defense_screen_enemy].x,u[defense_screen_enemy].y)>220*220)) {
            defense_screen_enemy=-1;defense_screen_return_pending=1;
        }
        /* A Guardian entering weapon range of a live Drone preempts an
           in-progress Mutalisk duel. The target must be currently visible,
           and the ordinary attack action is bounded to the air-screen area. */
        if(guardian_threat>=0&&defense_screen_enemy>=0&&
           u[defense_screen_enemy].raw_type!=44) {
            fprintf(stderr,"Z7 Guardian-priority preempt old%d new%d frame%u\n",
                defense_screen_enemy,guardian_threat,(unsigned)frame);
            defense_screen_enemy=-1;defense_screen_return_pending=0;
            defense_screen_order_retry_frame=frame;
        }
        if(defense_screen_enemy<0&&(unsigned)frame>=defense_screen_order_retry_frame) {
            int target=-1;
            for(int pass=0;pass<2&&target<0;pass++) {
                /* A visible Guardian already inside worker weapon range takes
                   priority; otherwise remove nearby Mutalisks to protect our
                   forward air screen. Both choices stay inside this visible
                   forward area and use an ordinary target click. */
                int raw=guardian_threat>=0?(pass==0?44:43):(pass==0?43:44);
                int best=(raw==44&&guardian_threat>=0)?220*220:160*160;
                for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
                   u[i].raw_type==raw&&currently_visible(u[i].x,u[i].y)) {
                    int d=dist2(air_x,air_y,u[i].x,u[i].y);
                    if(d<=best){target=i;best=d;}
                }
            }
            if(target>=0) {
                int tx=u[target].x,ty=u[target].y,raw=u[target].raw_type;
                int accepted=campaign_mutalisk_order(tx,ty,target),location_fallback=0;
                if(!accepted) {
                    /* A direct target may leave vision during ordinary cursor
                       travel. Its last observed position remains a legal
                       Attack-Move waypoint; the group then reacquires only
                       visible enemies inside the screen area. */
                    accepted=typed_attack_move(U_MUTALISK,tx,ty);
                    location_fallback=accepted;
                }
                fprintf(stderr,"Z7 north-screen air focus accepted%d locationFallback%d target%d raw%d priority%d xy%d,%d air%d,%d cover%d,%d frame%u mutas%d\n",
                    accepted,location_fallback,target,raw,guardian_threat>=0&&raw==44,
                    tx,ty,air_x,air_y,cover_x,cover_y,
                    (unsigned)frame,unit_type_count(U_MUTALISK,SIDE));
                if(accepted) {
                    defense_screen_enemy=last_direct_order_target>=0?last_direct_order_target:target;
                    defense_screen_return_pending=0;
                    defense_screen_order_retry_frame=frame+180u;return;
                }
                defense_screen_order_retry_frame=frame+60u;
            }
        }
        if(defense_screen_enemy>=0)return;
        if(defense_screen_return_pending&&defense_air_screen_set&&
           (unsigned)frame>=defense_screen_order_retry_frame&&unit_type_count(U_MUTALISK,SIDE)>0) {
            int accepted=typed_attack_move(U_MUTALISK,1050,1510);
            fprintf(stderr,"Z7 forward Mutalisk screen restore accepted%d frame%u mutas%d\n",
                accepted,(unsigned)frame,unit_type_count(U_MUTALISK,SIDE));
            if(accepted){defense_screen_return_pending=0;defense_screen_order_retry_frame=frame+180u;return;}
            defense_screen_order_retry_frame=frame+60u;
            return;
        }
        int live=combat_count(SIDE);
        if((defense_screen_return_pending||live!=defense_screen_combat_count)&&
           live>0&&(unsigned)frame>=defense_screen_order_retry_frame&&
           unit_type_count(U_HYDRALISK,SIDE)>0) {
            /* A whole-army regroup repeatedly pulled the Mutalisks off their
               forward sight line whenever a Hydra joined or died. Keep the
               ground screen on worker cover without overwriting the air order. */
            int accepted=typed_attack_move(U_HYDRALISK,cover_x,cover_y),ordered=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
               u[i].type==U_HYDRALISK&&
               u[i].order==ORDER_ATTACK_MOVE&&
               abs(u[i].tx-cover_x)<=40&&abs(u[i].ty-cover_y)<=40)++ordered;
            fprintf(stderr,"Z7 north-screen Hydra regroup accepted%d ordered%d live%d xy%d,%d frame%u focusWas%d\n",
                accepted,ordered,live,cover_x,cover_y,(unsigned)frame,had_focus);
            if(accepted) {
                defense_screen_combat_count=live;defense_screen_order_retry_frame=frame+1u;return;
            }
            defense_screen_order_retry_frame=frame+60u;
        }
        return;
    }
    /* Keep later-produced Hydralisks on the same ordinary Attack-Move rally
       that covers the Hatchery and worker route. Reissuing only when the live
       count changes avoids repeatedly pulling an engaged group off its rally. */
    if(defense_staged&&hatch>=0&&combat_count(SIDE)>=6) {
        int live_hydras=unit_type_count(U_HYDRALISK,SIDE);
        if(live_hydras<defense_hydras_rallied)
            defense_hydras_rallied=live_hydras;
        if(live_hydras>defense_hydras_rallied&&early_hydra_target_slot<0&&
           hydra_ground_target_slot<0&&(unsigned)frame>=next_hydra_rally_frame) {
            int rx,ry;
            z7_defense_screen_point(&rx,&ry);
            int accepted=typed_attack_move(U_HYDRALISK,rx,ry);
            fprintf(stderr,"Z7 normal-input Hydralisk Attack-Move rally accepted%d target%d,%d base%d,%d frame%u count%d previous%d workers%d\n",
                accepted,rx,ry,b[hatch].x,b[hatch].y,(unsigned)frame,live_hydras,
                defense_hydras_rallied,worker_count(-1));
            if(accepted)defense_hydras_rallied=live_hydras;
            next_hydra_rally_frame=frame+300u;
            return;
        }
    }
    /* Stabilize the starting anti-air force before sending it across the map
       to clear gas. The first local Mutalisk wave can arrive while the
       extractor escort is being formed; normal type-group attack-move keeps
       Hydralisks on that air threat without assigning an artificial target. */
    if(!building_complete(hatch)&&defense_staged&&hatch>=0&&combat_count(SIDE)>=6) {
        if(focused_muta_slot>=0) {
            int focus_gone=focused_muta_slot>=MAX_UNITS||!u[focused_muta_slot].alive||
                u[focused_muta_slot].owner!=ENEMY||!sc64_race_unit_flying(u[focused_muta_slot].type);
            if(focus_gone) {
                fprintf(stderr,"Z7 early Muta focus ended slot%d frame%u; preserve other unit orders\n",
                    focused_muta_slot,(unsigned)frame);
                focused_muta_slot=-1;
                mutas_air_ordered=0;
            } else {
                int fx,fy;
                z7_defense_screen_point(&fx,&fy);
                int outside=!currently_visible(u[focused_muta_slot].x,u[focused_muta_slot].y)||
                    dist2(u[focused_muta_slot].x,u[focused_muta_slot].y,fx,fy)>300*300;
                if(outside&&!defense_rally_after_focus&&(unsigned)frame>=early_muta_return_retry_frame) {
                    int accepted=typed_attack_move(U_MUTALISK,fx,fy);
                    fprintf(stderr,"Z7 early Muta fixed-cover return accepted%d threat%d xy%d,%d cover%d,%d frame%u\n",
                        accepted,focused_muta_slot,u[focused_muta_slot].x,u[focused_muta_slot].y,fx,fy,(unsigned)frame);
                    if(accepted){defense_rally_after_focus=1;focused_muta_slot=-1;mutas_air_ordered=0;return;}
                    early_muta_return_retry_frame=frame+300u;
                }
            }
        }
        /* Mutalisks keep their accepted target order; Hydralisks independently
           clear visible ground attackers threatening the mineral/Hatchery
           route. This bounded gate avoids replacing the Muta order on every
           controller pass or pulling either group after distant units. */
        int cover_x,cover_y;
        z7_defense_screen_point(&cover_x,&cover_y);
        if(early_hydra_target_slot>=0) {
            int t=early_hydra_target_slot;
            if(t<MAX_UNITS&&u[t].alive&&u[t].owner==ENEMY&&u[t].raw_type==38&&
               currently_visible(u[t].x,u[t].y)&&
               (dist2(u[t].x,u[t].y,cover_x,cover_y)<=224*224||z7_threat_near_workers(t,128))) {
                /* The direct order remains live; let macro work and the Muta
                   focus proceed without another expensive selection gesture. */
            } else {
                early_hydra_target_slot=-1;
                early_hydra_return_to_cover=1;
            }
        }
        if(early_hydra_target_slot<0&&early_hydra_return_to_cover) {
            int accepted=typed_attack_move(U_HYDRALISK,cover_x,cover_y);
            fprintf(stderr,"Z7 early Hydra return-to-worker-cover accepted%d xy%d,%d frame%u\n",
                accepted,cover_x,cover_y,(unsigned)frame);
            if(accepted){early_hydra_return_to_cover=0;early_hydra_next_order=frame+60u;}
        } else if(early_hydra_target_slot<0&&(unsigned)frame>=early_hydra_next_order) {
            int ground=-1,best=128*128;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==38&&
                owners_hostile(SIDE,u[i].owner)&&!SC64_TRANSPORT_LOADED(&u[i])&&
                currently_visible(u[i].x,u[i].y)) {
                int d=dist2(u[i].x,u[i].y,cover_x,cover_y);
                if((d<=best||z7_threat_near_workers(i,128))&&
                   (ground<0||d<best)){ground=i;best=d;}
            }
            if(ground>=0) {
                int accepted=typed_attack_target(U_HYDRALISK,ground);
                fprintf(stderr,"Z7 pre-Hatch local Hydra target accepted%d slot%d raw%d xy%d,%d cover%d,%d d2%d frame%u\n",
                    accepted,ground,u[ground].raw_type,u[ground].x,u[ground].y,cover_x,cover_y,best,(unsigned)frame);
                early_hydra_next_order=frame+120u;
                if(accepted)early_hydra_target_slot=ground;
            }
        }
        int threat=-1,best=300*300;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
            owners_hostile(SIDE,u[i].owner)&&!SC64_TRANSPORT_LOADED(&u[i])&&
            sc64_race_unit_flying(u[i].type)&&sc64_race_unit_damage(u[i].type)>0&&
            currently_visible(u[i].x,u[i].y)) {
            int d=dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y);
            if(d<best){best=d;threat=i;}
        }
        if(threat>=0) {
            if(!mutas_air_ordered&&frame>=mutas_air_retry_frame) {
                int ordered=typed_attack_target(U_MUTALISK,threat);
                fprintf(stderr,"Z7 early air defense Mutalisk direct-target flyer raw%d xy%d,%d base%d,%d d2%d radius300 accepted%d frame%u\n",
                    u[threat].raw_type,u[threat].x,u[threat].y,b[hatch].x,b[hatch].y,best,
                    ordered,(unsigned)frame);
                if(ordered) {
                    mutas_air_ordered=1;focused_muta_slot=threat;
                    focused_muta_started=frame;focused_muta_next_order=frame+240;
                    defense_rally_after_focus=0;
                } else mutas_air_retry_frame=frame+600;
            }
            if(focused_muta_slot<0)return;
        }
    }
    /* After the Hatchery completes, hold a fixed screen just north of the
       mineral cover. This puts Hydralisks inside their normal attack reach of
       the observed Guardian approach before it can reach returning workers. */
    if(defense_staged&&hatch>=0&&building_complete(hatch)&&combat_count(SIDE)>=6) {
        int ax=0,ay=0,n=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
            !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
            sc64_race_unit_damage(u[i].type)>0) {ax+=u[i].x;ay+=u[i].y;++n;}
        if(n){ax/=n;ay/=n;}
        int cover_x,cover_y;
        z7_defense_screen_point(&cover_x,&cover_y);
        if(!defense_cover_set) {
            int accepted=army_order(1,cover_x,cover_y);
            fprintf(stderr,"Z7 post-Hatch mineral-cover rally accepted%d xy%d,%d Hatch%d,%d army%d,%d frame%u\n",
                accepted,cover_x,cover_y,b[hatch].x,b[hatch].y,ax,ay,(unsigned)frame);
            if(accepted){defense_cover_set=1;return;}
        }
        /* Hold a fixed point on the previously observed northern Guardian
           approach. At (1100,1500), an incoming Guardian around (1090,1497)
           is inside the Mutalisk attack-move seek radius, so the group can
           engage before a long direct-target UI sequence finishes. This is
           a defensive waypoint near the owned Hatchery, not an enemy-base
           scout. */
        if(unit_type_count(U_MUTALISK,SIDE)>0&&!muta_guard_set) {
            int accepted=typed_attack_move(U_MUTALISK,guard_x,guard_y);
            fprintf(stderr,"Z7 Mutalisk Hatchery guard accepted%d xy%d,%d Hatch%d,%d frame%u\n",
                accepted,guard_x,guard_y,b[hatch].x,b[hatch].y,(unsigned)frame);
            if(accepted){muta_guard_set=1;local_air_next_order=frame+180u;return;}
            return;
        }
        /* Keep Hydralisks on the worker route. Intercept only raw38 Hydras
           already close to the fixed cover point; Mutalisks are reserved for
           Guardian and other local air targets below. */
        int idle_scourge=0;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
            u[i].type==U_SCOURGE&&u[i].order==ORDER_IDLE) {idle_scourge=1;break;}
        if(idle_scourge&&(unsigned)frame>=scourge_guard_retry_frame) {
            int accepted=typed_attack_move(U_SCOURGE,guard_x,guard_y);
            fprintf(stderr,"Z7 normal-input Scourge air-defense Attack-Move accepted%d target%d,%d live%d frame%u\n",
                accepted,guard_x,guard_y,unit_type_count(U_SCOURGE,SIDE),(unsigned)frame);
            scourge_guard_retry_frame=frame+(accepted?600u:60u);
            if(accepted)return;
        }
        if(hydra_ground_target_slot>=0) {
            int t=hydra_ground_target_slot;
            if(t<MAX_UNITS&&u[t].alive&&u[t].owner==ENEMY&&u[t].raw_type==38&&
               (dist2(u[t].x,u[t].y,cover_x,cover_y)<=144*144||
                z7_threat_near_workers(t,128)))
                return;
            hydra_ground_target_slot=-1;
            hydra_ground_return_to_cover=1;
        }
        if(hydra_ground_return_to_cover) {
            int accepted=typed_attack_move(U_HYDRALISK,cover_x,cover_y);
            fprintf(stderr,"Z7 Hydra return-to-cover accepted%d xy%d,%d frame%u\n",
                accepted,cover_x,cover_y,(unsigned)frame);
            if(accepted){hydra_ground_return_to_cover=0;hydra_ground_next_order=frame+60u;return;}
            hydra_ground_next_order=frame+30u;
            return;
        }
        if((unsigned)frame>=hydra_ground_next_order) {
            int ground=-1,best=0x7fffffff;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&u[i].raw_type==38&&
                owners_hostile(SIDE,u[i].owner)&&!SC64_TRANSPORT_LOADED(&u[i])&&
                currently_visible(u[i].x,u[i].y)) {
                int d=dist2(u[i].x,u[i].y,cover_x,cover_y);
                int near_worker=z7_threat_near_workers(i,128);
                if((d<=224*224||near_worker)&&d<best){best=d;ground=i;}
            }
            if(ground>=0) {
                int tx=u[ground].x,ty=u[ground].y;
                int accepted=typed_attack_target(U_HYDRALISK,ground);
                fprintf(stderr,"Z7 local Hydra direct-target accepted%d slot%d raw38 xy%d,%d cover%d,%d d2%d frame%u\n",
                    accepted,ground,tx,ty,cover_x,cover_y,best,(unsigned)frame);
                hydra_ground_next_order=frame+120u;
                if(accepted){hydra_ground_target_slot=ground;return;}
            }
        }
        if(local_air_target_slot>=0) {
            int t=local_air_target_slot;
            if(t<MAX_UNITS&&u[t].alive&&u[t].owner==ENEMY&&
               sc64_race_unit_flying(u[t].type)&&
               dist2(u[t].x,u[t].y,b[hatch].x,b[hatch].y)<=320*320)
                return;
            fprintf(stderr,"Z7 local air threat ended slot%d live%d owner%d xy%d,%d army%d,%d frame%u; return-to-guard\n",
                t,t<MAX_UNITS?u[t].alive:0,t<MAX_UNITS?u[t].owner:-1,
                t<MAX_UNITS?u[t].x:-1,t<MAX_UNITS?u[t].y:-1,ax,ay,(unsigned)frame);
            local_air_target_slot=-1;
            local_air_return_to_cover=1;
        }
        if(local_air_return_to_cover) {
            int accepted=typed_attack_move(U_MUTALISK,guard_x,guard_y);
            fprintf(stderr,"Z7 Mutalisk return-to-guard accepted%d xy%d,%d frame%u\n",
                accepted,guard_x,guard_y,(unsigned)frame);
            if(accepted) {
                local_air_return_to_cover=0;
                muta_guard_set=1;
                local_air_next_order=frame+60u;
                return;
            }
            local_air_next_order=frame+30u;
            return;
        }
        if(!local_air_return_to_cover&&(unsigned)frame>=local_air_next_order) {
            int threat=-1,best=320*320,best_priority=3;
            int guardian_present=0,guardian_hostile=0,guardian_valid=0;
            int guardian_visible=0,guardian_inrange=0;
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
                u[i].raw_type==44) {
                ++guardian_present;
                if(!owners_hostile(SIDE,u[i].owner)||SC64_TRANSPORT_LOADED(&u[i]))continue;
                ++guardian_hostile;
                if(u[i].type<0||u[i].type>=SC_RU_COUNT||
                   !sc64_race_unit_flying(u[i].type)||sc64_race_unit_damage(u[i].type)<=0)continue;
                ++guardian_valid;
                if(!currently_visible(u[i].x,u[i].y))continue;
                ++guardian_visible;
                if(dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y)<=320*320)++guardian_inrange;
            }
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
                owners_hostile(SIDE,u[i].owner)&&!SC64_TRANSPORT_LOADED(&u[i])&&
                u[i].type>=0&&u[i].type<SC_RU_COUNT&&
                sc64_race_unit_flying(u[i].type)&&sc64_race_unit_damage(u[i].type)>0&&
                currently_visible(u[i].x,u[i].y)) {
                int d=dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y);
                int priority=u[i].raw_type==44?0:(u[i].raw_type==43?1:2);
                if(d<=320*320&&(priority<best_priority||
                   (priority==best_priority&&d<best))) {
                    best=d;best_priority=priority;threat=i;
                }
            }
            static unsigned last_guardian_scan_log;
            if(!last_guardian_scan_log||frame-last_guardian_scan_log>=600u) {
                fprintf(stderr,"Z7 local air scan frame%u raw44 present%d hostile%d validFlyDamage%d visible%d within320ofHatch%d chosen%d raw%d priority%d d2%d\n",
                    (unsigned)frame,guardian_present,guardian_hostile,guardian_valid,
                    guardian_visible,guardian_inrange,threat,threat>=0?u[threat].raw_type:-1,
                    best_priority,threat>=0?best:0);
                last_guardian_scan_log=frame;
            }
            if(threat>=0) {
                int tx=u[threat].x,ty=u[threat].y,raw=u[threat].raw_type;
                accept_guardian_air_screen=(raw==44);
                int accepted=typed_attack_target(U_MUTALISK,threat);
                accept_guardian_air_screen=0;
                fprintf(stderr,"Z7 local Mutalisk direct-target accepted%d slot%d raw%d xy%d,%d army%d,%d d2%d frame%u\n",
                    accepted,threat,raw,tx,ty,ax,ay,best,(unsigned)frame);
                local_air_next_order=frame+180u;
                if(accepted){local_air_target_slot=last_direct_order_target>=0?last_direct_order_target:threat;return;}
            }
        }
    }
    if(defense_staged&&!geyser_attack_sent&&!geyser_secured&&hatch>=0&&
       (unsigned)frame<9000u) {
        int nearest_air=-1,nearest_air_d2=320*320;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
            currently_visible(u[i].x,u[i].y)&&
            u[i].type>=0&&u[i].type<SC_RU_COUNT&&sc64_race_unit_flying(u[i].type)&&
            sc64_race_unit_damage(u[i].type)>0) {
            int d=dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y);
            if(d<nearest_air_d2){nearest_air_d2=d;nearest_air=i;}
        }
        static unsigned last_raid_gate_log;
        if(!last_raid_gate_log||frame-last_raid_gate_log>=600u) {
            fprintf(stderr,"Z7 Extractor raid gate frame%u completeSunken%d Spore%d Den%d hydras%d combat%d workers%d bank%d/%d localAir%d raw%d xy%d,%d d2%d threshold320\n",
                (unsigned)frame,completed_building_count(B_SUNKEN_COLONY),
                completed_building_count(B_SPORE_COLONY),building_complete(raw_building(135,SIDE)),
                unit_type_count(U_HYDRALISK,SIDE),combat_count(SIDE),worker_count(-1),
                minerals,gas,nearest_air,nearest_air>=0?u[nearest_air].raw_type:-1,
                nearest_air>=0?u[nearest_air].x:-1,nearest_air>=0?u[nearest_air].y:-1,
                nearest_air>=0?nearest_air_d2:0);
            last_raid_gate_log=frame;
        }
        if(nearest_air>=0||!building_complete(raw_building(135,SIDE))||
           unit_type_count(U_HYDRALISK,SIDE)+queued_unit_count(U_HYDRALISK)<8||
           combat_count(SIDE)<12||worker_count(-1)<4)return;
        /* First sweep the known mineral-side approach. Only after ordinary
           visibility reveals an Extractor may the controller direct the
           Mutalisks toward its observed position. */
        int node_x=850,node_y=1580;
        select_mutalisk_cluster_for_scout=1;
        int scout_accepted=!geyser_scout_ordered&&
            typed_attack_move(U_MUTALISK,node_x,node_y);
        select_mutalisk_cluster_for_scout=0;
        if(scout_accepted) {
            geyser_scout_ordered=1;
            geyser_attack_sent=1;
            geyser_target_x=node_x;geyser_target_y=node_y;
            fprintf(stderr,"Z7 normal-input Mutalisk Attack-Move to mineral approach%d,%d frame%u army%d workers%d\n",
                node_x,node_y,(unsigned)frame,combat_count(SIDE),worker_count(-1));
            return;
        }
    }
    if(geyser_scout_ordered&&!geyser_secured&&!geyser_visible_extractor_found&&
       combat_count(SIDE)>=2&&frame<30000) {
        int best=-1,best_d=480*480;
        for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==ENEMY&&
            b[i].raw_owner==4&&b[i].raw_type==149&&currently_visible(b[i].x,b[i].y)) {
            int d=dist2(b[i].x,b[i].y,850,1580);
            if(d<best_d){best=i;best_d=d;}
        }
        if(best>=0) {
            int tx=b[best].x,ty=b[best].y;
            if(typed_attack_move_to_extractor(U_MUTALISK,tx,ty,best)) {
                geyser_visible_extractor_found=1;
                geyser_target_x=tx;geyser_target_y=ty;
                geyser_clear_order_frame=frame+240;
                fprintf(stderr,"Z7 visible Extractor approach accepted%d,%d from slot%d d2%d frame%u\n",
                    tx,ty,best,best_d,(unsigned)frame);
                return;
            }
        }
    }
    if(geyser_attack_sent&&!geyser_secured&&geyser_target_x>=0&&
       frame>=geyser_clear_order_frame&&combat_count(SIDE)>=2) {
        int target=-1,nearest=160*160;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&currently_visible(u[i].x,u[i].y)&&
            u[i].type>=0&&u[i].type<SC_RU_COUNT&&
            sc64_race_unit_damage(u[i].type)>0) {
            int d=dist2(u[i].x,u[i].y,geyser_target_x,geyser_target_y);
            if(d<nearest){nearest=d;target=i;}
        }
        if(target>=0) {
            int tx=u[target].x,ty=u[target].y,raw=u[target].raw_type;
            geyser_clear_order_frame=frame+240;
            if(typed_attack_move(U_MUTALISK,tx,ty))
                fprintf(stderr,"Z7 normal-input escort clears local raw%d slot%d xy%d,%d near source%d,%d d2%d frame%u\n",
                    raw,target,tx,ty,geyser_target_x,geyser_target_y,nearest,(unsigned)frame);
            return;
        }
    }
    if(geyser_visible_extractor_found&&!geyser_secured&&geyser_target_x>=0&&
       currently_visible(geyser_target_x,geyser_target_y)&&
       source_extractor_at_position(geyser_target_x,geyser_target_y)<0&&
       geyser_node_at(geyser_target_x,geyser_target_y)&&
       !enemy_combat_near_source_node(geyser_target_x,geyser_target_y,160)) {
        geyser_secured=1;
        fprintf(stderr,"Z7 restored source geyser secured at%d,%d frame%u enemieswithin160=0 nodes%d\n",
            geyser_target_x,geyser_target_y,(unsigned)frame,geyser_count);
    }
    if(geyser_secured&&!geyser_mutas_returned&&hatch>=0&&unit_type_count(U_MUTALISK,SIDE)>0&&
       (unsigned)frame>=geyser_return_retry_frame) {
        int gx=cursor_lattice_coordinate(cx,1100,WORLD_W);
        int gy=cursor_lattice_coordinate(cy,1500,WORLD_H);
        select_mutalisk_cluster_for_scout=1;
        int accepted=typed_attack_move(U_MUTALISK,gx,gy);
        select_mutalisk_cluster_for_scout=0;
        fprintf(stderr,"Z7 normal-input Mutalisk return from gas waypoint accepted%d guard%d,%d frame%u\n",
            accepted,gx,gy,(unsigned)frame);
        if(accepted)geyser_mutas_returned=1;
        else geyser_return_retry_frame=frame+120u;
        return;
    }
    if(hatch>=0&&combat_count(SIDE)>=6) {
        int guardian=-1,gd=240*240;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
            owners_hostile(SIDE,u[i].owner)&&!SC64_TRANSPORT_LOADED(&u[i])&&
            currently_visible(u[i].x,u[i].y)) {
            int d=dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y);
            if(u[i].raw_type==44&&d<gd){gd=d;guardian=i;}
        }
        if(!mutas_guardian_ordered&&guardian>=0&&frame>=mutas_guardian_retry_frame) {
            accept_guardian_air_screen=1;
            int ordered=typed_attack_target(U_MUTALISK,guardian);
            accept_guardian_air_screen=0;
            fprintf(stderr,"Z7 focused Mutalisk direct-target raw44 slot%d xy%d,%d base%d,%d d2%d radius240 accepted%d frame%u\n",
                guardian,u[guardian].x,u[guardian].y,b[hatch].x,b[hatch].y,gd,ordered,(unsigned)frame);
            if(ordered){mutas_guardian_ordered=1;local_air_target_slot=last_direct_order_target>=0?last_direct_order_target:guardian;}
            else mutas_guardian_retry_frame=frame+600;
            return;
        }
    }
    if(hatch>=0&&combat_count(SIDE)>=6&&frame>=900&&frame>=defense_attack_frame) {
        int hostile=-1,nearest=0x7fffffff,threat_raw=0;
        /* Source raw43 is Mutalisk and attacks both air/ground; raw44 is
           Guardian and attacks ground only. Focus the former before the latter. */
        const int priority_raws[]={38,37};
        for(int p=0;p<2&&hostile<0;p++) {
            for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
                u[i].raw_type==priority_raws[p]&&sc64_race_unit_damage(u[i].type)>0) {
                int d=dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y);
                /* Do not drag the whole defense across the map for a distant
                   patrol; only retarget when the air threat is local. */
                /* Leave the force on its Hatchery attack-move rally; repeated
                   individual-target orders pull the whole group into the
                   larger enemy force. Nearby threats are handled by auto-fire. */
                if(d<64*64&&d<nearest){nearest=d;hostile=i;threat_raw=priority_raws[p];}
            }
        }
        if(hostile>=0) {
            int tx=u[hostile].x,ty=u[hostile].y;
            int hydra_count=unit_type_count(U_HYDRALISK,SIDE);
            int accepted=hydra_count>0?typed_attack_target(U_HYDRALISK,hostile):0;
            if(accepted) {
                defense_attack_frame=frame+120u;
                fprintf(stderr,"Z7 normal-input Hydralisk-only defense target slot%d raw%d xy%d,%d dist2%d base%d,%d frame%u mutas remain on air task count%d\n",
                    hostile,threat_raw,tx,ty,nearest,b[hatch].x,b[hatch].y,(unsigned)frame,
                    unit_type_count(U_MUTALISK,SIDE));
                return;
            }
            defense_attack_frame=frame+60u;
            fprintf(stderr,"Z7 army defense order rejected slot%d raw%d xy%d,%d dist2%d frame%u selected%d mode%d menuopen%d cursor%d,%d\n",
                hostile,threat_raw,tx,ty,nearest,(unsigned)frame,selected_count(),mode,menu_open,cx,cy);
        }
    }
    if(frame<9000||frame<next_attack_frame||combat_count(SIDE)<10||
       worker_count(-1)<4||!building_complete(raw_building(135,SIDE))||
       minerals<25||gas<25)return;
    if(!nearest_enemy_building(&bx,&by,&slot))return;
    int active=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].selected&&
        u[i].order==ORDER_ATTACK_MOVE)active++;
    if(slot==last_attack_building&&active&&frame<next_attack_frame+600)return;
    next_attack_frame=frame+600u;
    last_attack_building=slot;
    if(unit_type_attack_order(U_HYDRALISK,bx,by))
        fprintf(stderr,"Z7 normal-input Hydralisk raid target slot%d raw%d xy%d,%d frame%u live enemy buildings%d\n",
            slot,b[slot].raw_type,bx,by,(unsigned)frame,building_count(ENEMY));
}

static void log_status(void)
{
    static int route_probe_done;
    if(!route_probe_done&&(unsigned)frame>=6000u) {
        int worker=-1;
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41) {worker=i;break;}
        if(worker>=0) {
            static const int xs[]={300,500,700,900};
            static const int ys[]={1200,1500,1800,1900};
            fprintf(stderr,"Z7 retreat-route probe frame%u worker%d xy%d,%d\n",
                (unsigned)frame,worker,u[worker].x,u[worker].y);
            for(unsigned yi=0;yi<sizeof(ys)/sizeof(ys[0]);yi++)
                for(unsigned xi=0;xi<sizeof(xs)/sizeof(xs[0]);xi++) {
                    int nx=-1,ny=-1;
                    int route=path_next_tile(u[worker].x,u[worker].y,xs[xi],ys[yi],&nx,&ny);
                    fprintf(stderr,"  goal%d,%d route%d next%d,%d\n",xs[xi],ys[yi],route,nx,ny);
                }
            route_probe_done=1;
        }
    }
    int nearest_slot=-1,nearest_d=0x7fffffff;
    int ax=0,ay=0,n=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        !sc64_race_is_worker(u[i].type)&&u[i].type!=U_OVERLORD&&
        sc64_race_unit_damage(u[i].type)>0){ax+=u[i].x;ay+=u[i].y;++n;}
    if(n){ax/=n;ay/=n;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&
        b[i].owner==ENEMY){int d=dist2(ax,ay,b[i].x,b[i].y);if(d<nearest_d){nearest_d=d;nearest_slot=i;}}}
    int hatch=z7_townhall(SIDE);
    int zerglings=unit_type_count(U_ZERGLING,SIDE), hydras=unit_type_count(U_HYDRALISK,SIDE);
    int mutas=unit_type_count(U_MUTALISK,SIDE), drones=unit_type_count(U_DRONE,SIDE);
    int type_counts[SC_RU_COUNT]={0};
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
        u[i].type>=0&&u[i].type<SC_RU_COUNT)type_counts[u[i].type]++;
    unsigned queue_total=0; int queue_entries=0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE) {
        queue_total+=b[i].queue_count;
        queue_entries+=b[i].queue_count>0;
    }
    fprintf(stderr,"Z7 frame%u host%u screen%d model%u started%02x cursors%u/%u/%u/%u/%u/%u/%u/%u switch%u ai%u/%u hp-units%d enemybuildings%d resources%d/%d army%d(z%d h%d m%d)@%d,%d units(d%d z%d h%d m%d) workers%d geysers%d H/P/D/E%d/%d/%d/%d complete%d/%d/%d/%d larva%d queue%d/%d target%d d2%d orders%d\n",
        (unsigned)frame,host_ticks,screen,race_z7.runtime.model.outcome,
        race_z7.runtime.model.started,race_z7.runtime.model.action_cursor[0],
        race_z7.runtime.model.action_cursor[1],race_z7.runtime.model.action_cursor[2],
        race_z7.runtime.model.action_cursor[3],race_z7.runtime.model.action_cursor[4],
        race_z7.runtime.model.action_cursor[5],race_z7.runtime.model.action_cursor[6],
        race_z7.runtime.model.action_cursor[7],race_z7.runtime.switch10,
        race_z7.runtime.ai_script_mask,race_z7.runtime.ai_plan_ticks,
        unit_count(-1,SIDE),building_count(ENEMY),minerals,gas,combat_count(SIDE),
        zerglings,hydras,mutas,ax,ay,drones,zerglings,hydras,mutas,
        worker_count(-1),geyser_count,owned_townhall_count(),
        owned_building_count(B_SPAWNING_POOL),owned_building_count(B_HYDRALISK_DEN),
        owned_building_count(B_EXTRACTOR),completed_building_count(B_HATCHERY),
        completed_building_count(B_SPAWNING_POOL),completed_building_count(B_HYDRALISK_DEN),
        completed_building_count(B_EXTRACTOR),hatch>=0?b[hatch].larva:0,
        queue_total,queue_entries,
        nearest_slot,nearest_d,actions_taken);
    if(frame>=9000&&frame<=16000) {
        fprintf(stderr,"Z7 local-defense state workers-live%d drone-deaths%u hatch%d complete%d cover",
            worker_count(-1),drone_deaths,hatch,hatch>=0?building_complete(hatch):0);
        if(hatch>=0) {
            int cover_x,cover_y;z7_mineral_cover_point(hatch,&cover_x,&cover_y);
            fprintf(stderr,"=%d,%d",cover_x,cover_y);
        }
        fprintf(stderr," MutaFocus%d HydraEarly%d HydraGround%d\n",
            focused_muta_slot,early_hydra_target_slot,hydra_ground_target_slot);
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE)
        fprintf(stderr,"  side-building%d raw%d type%d hp%d/%d under%d progress%d xy%d,%d\n",
            i,b[i].raw_type,b[i].type,b[i].hp,b[i].maxhp,b[i].under_construction,
            b[i].build_progress,b[i].x,b[i].y);
    if(hatch>=0)for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
        sc64_race_unit_damage(u[i].type)>0&&
        dist2(u[i].x,u[i].y,b[hatch].x,b[hatch].y)<256*256)
        fprintf(stderr,"  hostile-near-base%d raw%d type%d hp%d xy%d,%d order%d target%d cd%d\n",
            i,u[i].raw_type,u[i].type,u[i].hp,u[i].x,u[i].y,u[i].order,u[i].target,u[i].cd);
    if(frame>=5400&&frame<=14000) {
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41) {
            int nx=-1,ny=-1;
            int route=path_next_tile(u[i].x,u[i].y,u[i].tx,u[i].ty,&nx,&ny);
            fprintf(stderr,"  worker-live%d hp%d xy%d,%d to%d,%d route%d next%d,%d order%d target%d cd%d carry%d\n",
                i,u[i].hp,u[i].x,u[i].y,u[i].tx,u[i].ty,route,nx,ny,
                u[i].order,u[i].target,u[i].cd,u[i].carry);
            for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==ENEMY&&
                sc64_race_unit_damage(u[j].type)>0&&dist2(u[j].x,u[j].y,u[i].x,u[i].y)<512*512)
                fprintf(stderr,"    worker-threat%d raw%d type%d hp%d xy%d,%d order%d target%d cd%d d2%d\n",
                    j,u[j].raw_type,u[j].type,u[j].hp,u[j].x,u[j].y,u[j].order,u[j].target,u[j].cd,
                    dist2(u[j].x,u[j].y,u[i].x,u[i].y));
        }
    }
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&u[i].raw_type==41) {
        int patch=nearest_mineral(u[i].x,u[i].y),nx=-1,ny=-1;
        int route=patch>=0&&path_next_tile(u[i].x,u[i].y,m[patch].x,m[patch].y,&nx,&ny);
        fprintf(stderr,"  drone%d order%d target%d carry%d timer%d pos%d,%d nearest%d xy%d,%d route%d next%d,%d\n",
            i,u[i].order,u[i].target,u[i].carry,u[i].work_timer,u[i].x,u[i].y,
            patch,patch>=0?m[patch].x:-1,patch>=0?m[patch].y:-1,route,nx,ny);
    }
    if(frame>=9000&&frame<=14000) {
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&
            (u[i].type==U_HYDRALISK||u[i].type==U_MUTALISK))
            fprintf(stderr,"  FRIEND slot%d raw%d type%d hp%d xy%d,%d order%d target%d cd%d to%d,%d sel%d\n",
                i,u[i].raw_type,u[i].type,u[i].hp,u[i].x,u[i].y,u[i].order,
                u[i].target,u[i].cd,u[i].tx,u[i].ty,u[i].selected);
        for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==ENEMY&&
            (u[i].raw_type==43||u[i].raw_type==44))
            fprintf(stderr,"  ENEMY_AIR slot%d raw%d type%d hp%d xy%d,%d order%d target%d cd%d\n",
                i,u[i].raw_type,u[i].type,u[i].hp,u[i].x,u[i].y,u[i].order,u[i].target,u[i].cd);
    }
    if(frame%1200u==18u) {
        fputs("  normalized-types",stderr);
        for(int i=0;i<SC_RU_COUNT;i++)if(type_counts[i])fprintf(stderr," %d:%d",i,type_counts[i]);
        fputs("\n",stderr);
    }
}

int main(int argc,char **argv)
{
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"--policy")&&i+1<argc) {
            const char *name=argv[++i];
            if(!strcmp(name,"baseline"))early_gas_policy=0;
            else if(!strcmp(name,"early-gas"))early_gas_policy=1;
            else {fprintf(stderr,"unknown policy: %s\n",name);return 2;}
        }
        else if(!strcmp(argv[i],"--limit")&&i+1<argc)run_limit=(u32)strtoul(argv[++i],0,10);
        else if(!strcmp(argv[i],"--trace")&&i+1<argc)trace_path=argv[++i];
        else if(!strcmp(argv[i],"--probe-hatch-site")&&i+2<argc) {
            hatch_probe_x=(int)strtol(argv[++i],0,10);hatch_probe_y=(int)strtol(argv[++i],0,10);
        }
        else {fprintf(stderr,"usage: %s [--policy baseline|early-gas] [--limit HOST_TICKS] [--trace PATH] [--probe-hatch-site X Y]\n",argv[0]);return 2;}
    }
    if(!run_limit||run_limit>1000000u)return 2;
    mkdir(".local",0777);mkdir(".local/sc64",0777);
    trace_file=fopen(trace_path,"w");if(!trace_file){perror("open Z7 controller trace");return 2;}
    fprintf(trace_file,"{\"schema\":1,\"profile\":\"zerg-z7\",\"mission\":18,"
        "\"policy\":\"%s\",\"events\":[",
        early_gas_policy?"normal-input Z7 early-gas trial":"normal-input Z7 defensive baseline");
    launch_from_normal_menu();
    source_checkpoint("start");
    int last_checkpoint=frame;
    while(host_ticks<run_limit&&screen==SCREEN_PLAY&&!won&&!lost) {
        /* Give the visible combat threat one ordinary-input response before
           the worker evacuation menu consumes its multi-update selection and
           move gestures. Evacuation still runs in the same host iteration. */
        attack_step();
        int worker_evacuated=0; /* Retain workers on the protected line. */
        /* Keep paid unit and worker replacement running during retreats. */
        macro_step();
        if(frame>=last_checkpoint+Z7_CHECKPOINT_PERIOD) {
            last_checkpoint=frame;source_checkpoint("checkpoint");log_status();
        }
        step(0);
    }
    const char *terminal=(screen==SCREEN_RESULT&&won&&race_z7.runtime.model.outcome==SC64_Z7_WON)?"won":
        (screen==SCREEN_RESULT&&lost&&race_z7.runtime.model.outcome==SC64_Z7_LOST)?"lost":"limit";
    source_checkpoint(terminal);
    fputs("]}\n",trace_file);
    if(fclose(trace_file)!=0){perror("write Z7 controller trace");return 2;}
    trace_file=NULL;
    fprintf(stderr,"Z7 controller result=%s host_ticks=%u frame=%u model=%u started=%02x enemy_buildings=%d trace=%s\n",
        terminal,host_ticks,(unsigned)frame,race_z7.runtime.model.outcome,
        race_z7.runtime.model.started,building_count(ENEMY),trace_path);
    return !strcmp(terminal,"won")?0:1;
}
