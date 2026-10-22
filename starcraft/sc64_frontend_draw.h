#ifndef SC64_FRONTEND_DRAW_H
#define SC64_FRONTEND_DRAW_H

#include "sc64_menu_generated.h"

/* Original N64 artwork is converted offline into ROM tables. These screens
 * draw straight into the existing Mode 4 framebuffer, using a temporary
 * interface palette; launching/loading restores the world palette. */
static void front_button(int x,int y,int w,int h,const char *label,int selected)
{
    gfx_rect(x+2,y+2,w,h,C_BLACK);
    gfx_rect(x,y,w,h,C_PANEL);
    gfx_frame(x,y,w,h,selected?C_GOLD:C_EDGE);
    gfx_line(x+2,y+2,x+w-3,y+2,selected?C_SILVER:C_EDGE);
    gfx_line(x+2,y+h-3,x+w-3,y+h-3,C_SHADOW);
    int len=0;while(label[len])len++;
    gfx_text(x+(w-len*6)/2+(selected?2:0),y+(h-7)/2,label,selected?C_GOLD:C_SILVER);
    if(selected) {
        int mid=y+h/2;
        gfx_line(x+5,mid-3,x+8,mid,C_GOLD);
        gfx_line(x+8,mid,x+5,mid+3,C_GOLD);
        int pulse=(race_front.ticks/6)%4;
        gfx_rect(x+w-10,y+h-6,4,2,pulse<2?C_GOLD:C_GOLD_DARK);
    }
}

/* A compact metallic wordmark, kept as code rather than a replacement raster
 * asset. The original artwork supplies the surrounding space and characters. */
static void front_logo(void)
{
    static const unsigned char glyph[6][7]={
        {31,16,16,31,1,1,31},{31,4,4,4,4,4,4},
        {14,17,17,31,17,17,17},{30,17,17,30,20,18,17},
        {15,16,16,16,16,16,15},{31,16,16,30,16,16,16}};
    static const unsigned char letters[9]={0,1,2,3,4,3,2,5,1};
    for(int n=0;n<9;n++)for(int row=0;row<7;row++)for(int col=0;col<5;col++)
        if(glyph[letters[n]][row]&(16>>col)) {
            int x=40+n*18+col*3,y=14+row*3;
            gfx_rect(x+1,y+2,3,3,C_SHADOW);
            gfx_rect(x,y,3,3,row<2?C_WHITE:row<4?C_SILVER:C_DIM);
        }
    gfx_line(39,38,200,38,C_GOLD_DARK);
    gfx_line(82,41,158,41,C_EDGE);
}

static void front_header(const char *title,const char *subtitle)
{
    int len=0;while(title[len])len++;
    gfx_rect(8,6,224,15,C_BLACK);
    gfx_text((240-len*6)/2,10,title,C_GOLD);
    gfx_line(10,24,230,24,C_EDGE);
    if(subtitle){gfx_rect(110,29,124,14,C_BLACK);gfx_text(114,33,subtitle,C_SILVER);}
}

static const char *front_mission_title(int mission)
{
    const Sc64MissionInfo *info = sc64_catalog_get(mission);
    return info ? info->title : "UNKNOWN MISSION";
}

static const char *front_map_title(int map)
{
    static const char *names[2]={"OUTPOST","BLOOD BATH"};
    if (map < 0 || map >= SC64_FRONT_MAP_COUNT) return names[0];
    return names[map];
}

static const char *front_campaign_title(int faction)
{
#ifdef SC64_UNIFIED
    return faction==SC_FRONT_TERRAN?"TERRAN CAMPAIGN":
           faction==SC_FRONT_ZERG?"THE OVERMIND":"THE FALL";
#else
    return faction?"THE FALL":"THE OVERMIND";
#endif
}

static const u8 *front_campaign_art(int faction)
{
#ifdef SC64_UNIFIED
    return faction==SC_FRONT_TERRAN?sc64_menu_terran:
           faction==SC_FRONT_PROTOSS?sc64_menu_protoss:sc64_menu_zerg;
#else
    return faction?sc64_menu_protoss:sc64_menu_zerg;
#endif
}

static void front_footer(const char *text)
{
    gfx_rect(0,147,240,13,C_PANEL);
    gfx_line(0,146,239,146,C_EDGE);
    gfx_text(9,151,text,C_DIM);
}

static void front_uplink(void)
{
    gfx_text(12,137,"UPLINK",C_DIM);
    for(int i=0;i<8;i++) {
        int level=2+((race_front.ticks/3+i*7)%9);
        gfx_rect(55+i*5,143-level,3,level,(i+race_front.ticks/5)%3?C_GREEN_DARK:C_GREEN);
    }
}

static void front_briefing(void)
{
#ifdef SC64_UNIFIED
#if defined(SC64_T3_ENABLED)
    static const char *lines[7][5]={
#else
    static const char *lines[6][5]={
#endif
        {"CREATE SPAWNING POOL","CREATE HYDRALISK DEN","PROTECT THE CHRYSALIS","DESTROY TERRAN BASE",""},
        {"ESCORT THE CHRYSALIS","TO THE WARP POINT","","KEEP THE CARRIER","ALIVE"},
        {"RESCUE FENIX","RECLAIM THE BASE","","DESTROY THE ZERG","FENIX MUST SURVIVE"},
        {"WAIT FOR FENIX","DESTROY THE CEREBRATE","","FENIX MUST SURVIVE",""},
        {"FIND JIM RAYNOR","TRAIN 10 MARINES","BUILD A BARRACKS","KEEP RAYNOR ALIVE",""},
        {"DESTROY THE INFESTED","COMMAND CENTER","RAYNOR MUST SURVIVE","",""}
#if defined(SC64_T3_ENABLED)
        ,{"SURVIVE 30 MINUTES","PROTECT YOUR BASE","OPTIONAL OBJECTIVE","DESTROY ZERG HATCHERY",""}
#endif
    };
#else
    static const char *lines[4][5]={
        {"CREATE SPAWNING POOL","CREATE HYDRALISK DEN","PROTECT THE CHRYSALIS","DESTROY TERRAN BASE",""},
        {"ESCORT THE CHRYSALIS","TO THE WARP POINT","","KEEP THE CARRIER","ALIVE"},
        {"RESCUE FENIX","RECLAIM THE BASE","","DESTROY THE ZERG","FENIX MUST SURVIVE"},
        {"WAIT FOR FENIX","DESTROY THE CEREBRATE","","FENIX MUST SURVIVE",""}};
#endif
    static const char *generic_lines[5]={"COMPLETE THE MISSION","FOLLOW THE BRIEFING","","",""};
    const char *const *mission_lines = race_front.mission <
#ifdef SC64_UNIFIED
#if defined(SC64_T3_ENABLED)
        7 ? lines[race_front.mission] : generic_lines;
#else
        6 ? lines[race_front.mission] : generic_lines;
#endif
#else
        4 ? lines[race_front.mission] : generic_lines;
#endif
    unsigned age=race_front.ticks-race_front_transition;
    front_header("MISSION BRIEFING",front_mission_title(race_front.mission));
    gfx_rect(108,45,126,68,C_PANEL);
    gfx_line(108,45,108,112,C_EDGE);
    gfx_text(113,48,"PRIMARY OBJECTIVES",C_DIM);
    for(int i=0;i<5;i++) {
        char text[22];int n=0,budget=(int)age*2-i*18;
        while(mission_lines[i][n]&&n<20&&n<budget){text[n]=mission_lines[i][n];n++;}
        text[n]=0;gfx_text(112,61+i*10,text,C_SILVER);
    }
    /* A slow comms scan and level indicators give the static original render
       a transmission treatment without adding animation buffers. */
    int scan=43+(race_front.ticks/2)%84;
    gfx_line(12,scan,99,scan,C_EDGE);
    front_uplink();
    front_button(116,120,112,22,"DEPLOY",1);
    front_footer("A DEPLOY     B BACK");
}

static void sc64_frontend_draw(void)
{
    int page=race_front.page,faction=page==SC_FRONT_CAMPAIGN?
        sc64_front_campaign_faction(race_front.row):race_front.faction;
    const u8 *background=sc64_menu_home;
    if(page==SC_FRONT_CAMPAIGN||page==SC_FRONT_MISSION||page==SC_FRONT_BRIEFING)
        background=front_campaign_art(faction);
    if(page==SC_FRONT_SKIRMISH)
        background=race_front.race==R_TER?sc64_menu_terran:race_front.race==R_ZERG?sc64_menu_zerg:sc64_menu_protoss;
    unsigned age=race_front.ticks-race_front_transition;
    if(age<12) {
        u16 fade[224];int amount=(int)age+1;
        for(int i=0;i<224;i++) {
            u16 p=sc64_menu_palette[i];
            fade[i]=((p&31)*amount/12)|((((p>>5)&31)*amount/12)<<5)|((((p>>10)&31)*amount/12)<<10);
        }
        gfx_set_palette(32,224,fade);
    } else gfx_set_palette(32,224,sc64_menu_palette);
    gfx_blit_band(0,160,background);
    gfx_frame(0,0,240,147,C_EDGE);
    gfx_line(3,3,22,3,C_SILVER);gfx_line(3,3,3,20,C_SILVER);
    gfx_line(217,3,236,3,C_SILVER);gfx_line(236,3,236,20,C_SILVER);
    if(page==SC_FRONT_HOME) {
        front_logo();
        gfx_text(134,56,"SECTOR COMMAND",C_DIM);
        front_button(128,70,103,21,"CAMPAIGN",race_front.row==0);
        front_button(128,96,103,21,"CUSTOM GAME",race_front.row==1);
        front_button(128,122,103,21,"LOAD GAME",race_front.row==2);
        gfx_text(12,123,"KOPRULU SECTOR",C_DIM);
        front_uplink();
        for(int i=0;i<8;i++) {
            int x=17+(i*37)%96,y=50+(i*23)%62;
            if(((race_front.ticks/8+i)%7)==0)gfx_pixel(x,y,C_SILVER);
        }
        front_footer("UP/DN SELECT   A ENTER   L LOAD");
    } else if(page==SC_FRONT_CAMPAIGN) {
#ifdef SC64_CAMPAIGN_RACE
        front_header("CAMPAIGN",0);
        front_button(118,70,112,28,
            faction==SC_FRONT_TERRAN?"TERRAN":faction==SC_FRONT_ZERG?"ZERG":"PROTOSS",1);
        gfx_text(121,108,front_campaign_title(faction),C_GOLD);
#else
#ifdef SC64_UNIFIED
        front_header("CHOOSE YOUR CAMPAIGN",0);
        front_button(118,43,112,24,"TERRAN",race_front.row==0);
        front_button(118,75,112,24,"ZERG",race_front.row==1);
        front_button(118,107,112,24,"PROTOSS",race_front.row==2);
        gfx_text(121,135,front_campaign_title(faction),C_GOLD);
#else
        front_header("CHOOSE YOUR CAMPAIGN",0);
        front_button(118,52,112,28,"ZERG",race_front.row==0);
        front_button(118,91,112,28,"PROTOSS",race_front.row==1);
        gfx_text(121,126,faction?"III: THE FALL":"II: THE OVERMIND",C_GOLD);
#endif
#endif
        front_uplink();front_footer("ARROWS FACTION  A SELECT  B BACK");
    } else if(page==SC_FRONT_MISSION) {
        front_header(front_campaign_title(faction),
#ifdef SC64_UNIFIED
                     faction==SC_FRONT_TERRAN?"TERRAN CAMPAIGN":faction==SC_FRONT_PROTOSS?"PROTOSS CAMPAIGN":"ZERG CAMPAIGN");
#else
                     faction?"PROTOSS CAMPAIGN":"ZERG CAMPAIGN");
#endif
        int catalog_faction=sc64_front_catalog_faction(faction);
        int available=sc64_catalog_playable_count(catalog_faction);
        for(int i=0;i<2;i++) {
            int mission=sc64_catalog_playable_at(catalog_faction,race_front.mission_page*2+i);
            if(mission<0) continue;
            front_button(110,55+i*33,122,25,front_mission_title(mission),race_front.row==i);
            if(race_completed&(1<<mission))gfx_rect(102,64+i*33,4,4,C_GREEN);
        }
        gfx_rect(110,117,124,12,C_BLACK);
        gfx_text(115,121,(race_completed&(1<<(sc64_catalog_playable_at(catalog_faction,race_front.mission_page*2+race_front.row))))?"MISSION COMPLETE":"AWAITING YOUR ORDERS",C_DIM);
        if((available+1)/2>1) gfx_text(199,121,"L/R PAGE",C_GOLD);
        front_uplink();front_footer("UP/DN MISSION  L/R PAGE  A BRIEFING  B BACK");
    } else if(page==SC_FRONT_BRIEFING)front_briefing();
    else if(page==SC_FRONT_SKIRMISH) {
        front_header("CUSTOM GAME",0);
#ifdef SC64_IMPORTED_SKIRMISH
        char option[24];
        for(int row=0;row<3+race_front.bots;row++) {
            if(row==0) {
                int n=0;const char *label="MAP ";while(label[n]){option[n]=label[n];n++;}
                const char *value=front_map_title(race_front.map);int j=0;while(value[j])option[n++]=value[j++];option[n]=0;
            } else if(row==1) {
                int n=0;const char *label="HUMAN ";while(label[n]){option[n]=label[n];n++;}
                const char *value=sc64_race_names[race_front.race];int j=0;while(value[j])option[n++]=value[j++];option[n]=0;
            } else if(row==2) {
                option[0]='O';option[1]='P';option[2]='P';option[3]='O';option[4]='N';option[5]='E';option[6]='N';option[7]='T';option[8]='S';option[9]=' ';
                option[10]=(char)('0'+race_front.bots);option[11]=0;
            } else {
                int ai=row-3;option[0]='A';option[1]='I';option[2]=' ';
                option[3]=(char)('1'+ai);option[4]=' ';
                const char *value=sc64_race_names[race_front.bot_races[ai]];int j=0;
                while(value[j]) { option[5+j]=value[j]; ++j; }
                option[5+j]=0;
            }
            front_button(112,28+row*17,119,15,option,race_front.row==row);
        }
        front_button(112,28+(3+race_front.bots)*17,119,15,"LAUNCH",race_front.row==3+race_front.bots);
        front_footer("UP/DN SELECT L/R CHANGE A ACCEPT");
#else
        gfx_rect(110,31,123,13,C_BLACK);
        gfx_text(115,36,"FACTION",C_DIM);
        front_button(112,47,119,22,sc64_race_names[race_front.race],race_front.row==0);
        gfx_rect(110,74,123,12,C_BLACK);gfx_text(115,78,"COMPUTER OPPONENTS",C_DIM);
        char opponents[6]={'<',' ',(char)('0'+race_front.bots),' ','>',0};
        front_button(112,89,119,22,opponents,race_front.row==1);
        front_button(112,121,119,22,"LAUNCH",race_front.row==2);
        gfx_text(12,136,"FREE FOR ALL",C_DIM);
        for(int i=0;i<race_front.bots;i++)gfx_rect(17+i*15,124,10,5,i==0?C_ZERG:i==1?C_TERRAN:C_PROTOSS);
        front_footer("UP/DN SELECT L/R CHANGE A ACCEPT");
#endif
    }
    if(race_front_load_error) {
        gfx_rect(36,106,168,29,C_BLACK);gfx_frame(36,106,168,29,C_RED_DARK);
        gfx_text(63,116,"NO SAVED GAME FOUND",C_GOLD);
    }
}

#endif
