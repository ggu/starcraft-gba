/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHOWCASE_ADAPTER_H
#define SC64_SHOWCASE_ADAPTER_H

/* Repeatable authored starting positions. All movement, hits, shield
 * regeneration and deaths below use the ordinary game simulation. */
static int sc64_showcase_actor(int owner,int type,int x,int y,int order,
                               int tx,int ty)
{
    int n=spawn_unit(owner,type,x,y);
    if(n<0)return n;
    u[n].raw_type=sc64_race_raw_unit(type);
    u[n].raw_owner=-1;
    u[n].order=order;u[n].tx=tx;u[n].ty=ty;
    return n;
}

static int sc64_showcase_base(int owner,int type,int x,int y)
{
    int n=spawn_build(owner,type,x,y,0);
    if(n>=0) { b[n].raw_type=sc64_race_raw_building(type);b[n].raw_owner=-1; }
    return n;
}

static void sc64_showcase_group(int owner,int type,int count,int x,int y,
                                int cols,int spacing,int order,int tx,int ty)
{
    for(int i=0;i<count;++i)
        (void)sc64_showcase_actor(owner,type,x+(i%cols)*spacing,
                                  y+(i/cols)*spacing,order,tx,ty);
}

static void sc64_showcase_reveal(void)
{
    memset(visible,0xff,sizeof(visible));
    memset(explored,0xff,sizeof(explored));
}

static void sc64_showcase_start(int scene)
{
    scene=limit(scene,0,SC64_FRONT_SHOWCASE_COUNT-1);
    sc64_showcase_scene=-1;
    race_mode=1;race_skirmish_map=0;race_bots=scene==6?2:1;
    race_human=scene==0||scene==4||scene==5?R_PROTOSS:
               scene==3?R_ZERG:R_TER;
    race_bot_races[0]=R_ZERG;race_bot_races[1]=R_PROTOSS;
    if(scene==3)race_bot_races[0]=R_TER;
    init_state();
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));memset(m,0,sizeof(m));
    clear_tracers();
    death_vfx_reset();
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    geyser_count=0;minerals=gas=0;supply_used=supply_cap=0;
    memset(race_minerals,0,sizeof(race_minerals));
    memset(race_gas,0,sizeof(race_gas));
    seed=0x53484f57u+(unsigned)scene;frame=0;
    sc64_showcase_scene=scene;sc64_showcase_ticks=0;
    cx=512;cy=512;selected_building=-1;
    /* Bases outside the set keep ordinary skirmish outcome rules active. */
    if(scene<2||scene>4)
        (void)sc64_showcase_base(SIDE,race_human==R_PROTOSS?B_NEXUS:B_COMMAND,160,160);
    (void)sc64_showcase_base(ENEMY,scene==3?B_COMMAND:B_HATCHERY,864,864);
    if(scene==6)(void)sc64_showcase_base(RESCUABLE,B_NEXUS,864,160);
    if(scene==0) {
        for(int i=0;i<3;++i)
            (void)sc64_showcase_actor(SIDE,U_P7_CARRIER,437,480+i*28,
                                      ORDER_MOVE,520,480+i*28);
        sc64_showcase_group(SIDE,U_SCOUT,4,475,480,2,18,ORDER_ATTACK_MOVE,585,510);
        sc64_showcase_group(SIDE,U_ZEALOT,6,475,525,3,14,ORDER_ATTACK_MOVE,585,530);
        sc64_showcase_group(ENEMY,U_HYDRALISK,12,566,476,4,14,ORDER_ATTACK_MOVE,475,510);
        sc64_showcase_group(ENEMY,U_ZP_ULTRALISK,1,584,540,1,16,ORDER_ATTACK_MOVE,475,530);
    } else if(scene==1) {
        sc64_showcase_group(SIDE,U_SIEGE_TANK,6,440,480,2,25,ORDER_IDLE,0,0);
        sc64_showcase_group(ENEMY,U_ZERGLING,32,557,467,8,10,ORDER_ATTACK_MOVE,449,510);
    } else if(scene>=2&&scene<=4) {
        int type=scene==2?B_COMMAND:scene==3?B_HATCHERY:B_NEXUS;
        int base=sc64_showcase_base(SIDE,type,512,512);
        /* Eight regular Marine shots at updates17,36,...150. HP is staged;
         * the final hit, building death and defeat use normal combat rules. */
        if(base>=0) {
            /* Normal Nexus shield regeneration absorbs three HP damage
             * over this staged eight-shot sequence. */
            b[base].hp=scene==4?37:40;
            b[base].shield=0;b[base].shield_remainder=0;
        }
        int attacker=sc64_showcase_actor(ENEMY,U_MARINE,552,512,
                                         ORDER_IDLE,552,512);
        if(attacker>=0) {u[attacker].target=MAX_UNITS+base;u[attacker].cd=17;}
        int worker=scene==2?U_SCV:scene==3?U_DRONE:U_PROBE;
        sc64_showcase_group(SIDE,worker,4,415,487,2,17,ORDER_IDLE,0,0);
        sc64_showcase_group(ENEMY,U_ZERGLING,4,401,480,1,18,ORDER_ATTACK_MOVE,421,500);
    } else if(scene==5) {
        sc64_showcase_group(SIDE,U_ZEALOT,8,463,508,4,14,ORDER_ATTACK_MOVE,582,512);
        sc64_showcase_group(SIDE,U_DRAGOON,4,438,477,2,20,ORDER_ATTACK_MOVE,580,510);
        sc64_showcase_group(SIDE,U_SCOUT,3,455,465,3,23,ORDER_ATTACK_MOVE,585,510);
        sc64_showcase_group(ENEMY,U_HYDRALISK,12,574,482,4,14,ORDER_ATTACK_MOVE,464,512);
        sc64_showcase_group(ENEMY,U_ZERGLING,12,548,525,6,10,ORDER_ATTACK_MOVE,464,512);
    } else if(scene==6) {
        sc64_showcase_group(SIDE,U_MARINE,10,435,492,2,14,ORDER_ATTACK_MOVE,528,510);
        sc64_showcase_group(ENEMY,U_ZERGLING,18,557,474,6,12,ORDER_ATTACK_MOVE,506,512);
        sc64_showcase_group(RESCUABLE,U_ZEALOT,6,502,534,3,16,ORDER_ATTACK_MOVE,520,500);
        sc64_showcase_group(RESCUABLE,U_DRAGOON,3,483,558,3,20,ORDER_ATTACK_MOVE,520,500);
    } else {
        (void)sc64_showcase_base(ENEMY,B_HATCHERY,581,510);
        (void)sc64_showcase_base(ENEMY,B_SPAWNING_POOL,615,480);
        sc64_showcase_group(SIDE,U_SIEGE_TANK,4,430,480,2,25,ORDER_IDLE,0,0);
        sc64_showcase_group(SIDE,U_MARINE,8,483,487,2,13,ORDER_ATTACK_MOVE,581,510);
        sc64_showcase_group(ENEMY,U_HYDRALISK,8,560,477,2,15,ORDER_ATTACK_MOVE,483,512);
        sc64_showcase_group(ENEMY,U_ZERGLING,10,558,535,5,11,ORDER_ATTACK_MOVE,483,512);
    }
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
    reset_selection_gestures();invalidate_path_grid();sc64_visibility_reset();
    update_exploration();sc64_showcase_reveal();
    screen=SCREEN_PAUSE;
}

static void sc64_showcase_leave(u16 held)
{
    int scene=sc64_showcase_scene;
    sc64_showcase_scene=-1;sc64_showcase_ticks=0;
    reset_selection_gestures();won=lost=0;screen=SCREEN_TITLE;
    race_frontend_reset();race_front.page=SC_FRONT_SHOWCASE;
    race_front.row=scene;race_front.prev_buttons=held;
}

static void sc64_showcase_draw_label(void)
{
    gfx_rect(0,147,240,13,C_PANEL);
    gfx_text(4,149,sc64_front_showcase_name(sc64_showcase_scene),C_CYAN);
    if(sc64_showcase_scene>=2&&sc64_showcase_scene<=4) {
        gfx_text(155,149,"STAGED",C_DIM);
        gfx_num(207,149,(150-(int)sc64_showcase_ticks+29)/30,C_GOLD);
    } else if(sc64_showcase_scene==0)gfx_text(121,149,"CARRIER PREVIEW",C_DIM);
    else gfx_text(162,149,"SEL SCENES",C_DIM);
}
#endif
