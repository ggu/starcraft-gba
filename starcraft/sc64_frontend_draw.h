#ifndef SC64_FRONTEND_DRAW_H
#define SC64_FRONTEND_DRAW_H

#include "sc64_chrome.h"

static void front_text(int x, int y, const char *text, u8 color)
{
    sc64_font_text(x, y, x < 240 ? 240 - x : 0, text, color);
}

static void front_center_text(int y, const char *text, u8 color)
{
    char fitted[64];
    int width = sc64_font_width(text);
    if (width > 216) {
        sc64_font_ellipsis(text,216,fitted,sizeof(fitted));
        text=fitted;
        width=sc64_font_width(text);
    }
    int x = (240 - width) / 2;
    if (x < 12) x = 12;
    sc64_font_text(x, y, 216, text, color);
}

/* Original N64 artwork is converted offline into ROM tables. These screens
 * draw straight into the existing Mode 4 framebuffer, using a temporary
 * interface palette; launching/loading restores the world palette. */
static void front_button(int x,int y,int w,int h,const char *label,int selected)
{
    (void)w;
    sc64_chrome_button(x,y,h<=15,h>=25,label,selected);
}

static void front_logo(void)
{
    sc64_chrome_draw_title_plate(72,31,"StarCraft");
}

static void front_header(const char *title,const char *subtitle)
{
    front_center_text(34,title,C_GOLD);
    if(subtitle) front_center_text(45,subtitle,C_SILVER);
}

static const char *front_mission_title(int mission)
{
    const Sc64MissionInfo *info = sc64_catalog_get(mission);
    return info ? info->title : "Unknown Mission";
}

static const char *front_map_title(int map)
{
    static const char *names[2]={"Outpost","Blood Bath"};
    if (map < 0 || map >= SC64_FRONT_MAP_COUNT) return names[0];
    return names[map];
}

#ifdef SC64_SHOWCASE_ENABLED
static const char *front_showcase_title(int scene)
{
    return sc64_front_showcase_name(scene);
}
#endif

static const char *front_campaign_title(int faction)
{
#ifdef SC64_UNIFIED
    return faction==SC_FRONT_TERRAN?"Terran Campaign":
           faction==SC_FRONT_ZERG?"The Overmind":"The Fall";
#else
    return faction?"The Fall":"The Overmind";
#endif
}

static const u8 *front_campaign_art(int faction)
{
#ifdef SC64_UNIFIED
    return faction==SC_FRONT_TERRAN?sc64_chrome_menu_terran:
           faction==SC_FRONT_PROTOSS?sc64_chrome_menu_protoss:sc64_chrome_menu_zerg;
#else
    return faction?sc64_chrome_menu_protoss:sc64_chrome_menu_zerg;
#endif
}

static void front_footer(const char *text)
{
    sc64_chrome_draw_footer(text);
}

static void front_uplink(void)
{
    front_text(24,136,"Uplink",C_DIM);
    for(int i=0;i<8;i++) {
        int level=2+((race_front.ticks/3+i*7)%9);
        gfx_rect(55+i*5,135-level,3,level,(i+race_front.ticks/5)%3?C_GREEN_DARK:C_GREEN);
    }
}

static void front_load_error(void)
{
    if(race_front_load_error)
        sc64_chrome_draw_notice("No Saved Game Found","Start a Campaign or Custom Game");
}

/* Text is authored by the source script and wrapped offline to the visible
 * area. Never clip words or substitute objective summaries for dialogue. */
static void front_briefing(void)
{
    const Sc64BriefingPage *page=race_front_briefing_page();
    const char *title=front_mission_title(race_front.mission);
    front_text(12,9,title,C_GOLD);
    if(!page) {
        front_text(62,28,"Mission Orders",C_SILVER);
        front_text(12,69,"Briefing unavailable",C_SILVER);
        front_text(12,87,"Start to deploy",C_DIM);
    } else {
        if(page->portrait>=0 && page->portrait<SC64_BRIEFING_PORTRAIT_COUNT)
            gfx_blit_sprite(12,23,SC64_BRIEFING_PORTRAIT_WIDTH,
                            SC64_BRIEFING_PORTRAIT_HEIGHT,
                            sc64_briefing_portraits[page->portrait]);
        else {
            front_text(14,35,"Orders",C_GOLD);
            front_text(20,47,"Data",C_DIM);
        }
        front_text(62,28,page->speaker,C_SILVER);
        front_text(62,43,"Page",C_DIM);
        sc64_font_num(92,43,18,(int)race_front.briefing_page+1,C_GOLD);
        front_text(110,43,"/",C_DIM);
        sc64_font_num(122,43,24,(int)race_front.briefing_pages,C_DIM);
        if(race_front.briefing_page>0)front_text(163,43,"< Prev",C_DIM);
        char text[SC64_BRIEFING_MAX_TEXT_BYTES+1];
        unsigned visible=sc64_frontend_briefing_visible(&race_front),i=0;
        while(i<visible && i<SC64_BRIEFING_MAX_TEXT_BYTES && page->text[i]) {
            text[i]=page->text[i];i++;
        }
        text[i]=0;
        sc64_font_text(12,69,216,text,C_SILVER);
    }
    const char *next=!sc64_frontend_briefing_is_revealed(&race_front)?"A Reveal":
        race_front.briefing_page+1<race_front.briefing_pages?"A Next":"A Deploy";
    front_text(12,145,next,C_GOLD);
    front_text(77,145,"B Back",C_DIM);
    front_text(143,145,"Start Deploy",C_GOLD);
}

static void sc64_frontend_draw(void)
{
    int page=race_front.page,faction=page==SC_FRONT_CAMPAIGN?
        sc64_front_campaign_faction(race_front.row):race_front.faction;
    const u8 *background=sc64_chrome_menu_home;
    if(page==SC_FRONT_CAMPAIGN||page==SC_FRONT_MISSION||page==SC_FRONT_BRIEFING)
        background=front_campaign_art(faction);
    if(page==SC_FRONT_SKIRMISH)
        background=race_front.race==R_TER?sc64_chrome_menu_terran:race_front.race==R_ZERG?sc64_chrome_menu_zerg:sc64_chrome_menu_protoss;
    const u16 *palette=sc64_chrome_menu_palette;
    if(page==SC_FRONT_BRIEFING) {
        background=sc64_briefing_background;
        palette=sc64_briefing_palette;
    }
    unsigned age=race_front.ticks-race_front_transition;
    if(age<12) {
        u16 fade[224];int amount=(int)age+1;
        for(int i=0;i<224;i++) {
            u16 p=palette[i];
            fade[i]=((p&31)*amount/12)|((((p>>5)&31)*amount/12)<<5)|((((p>>10)&31)*amount/12)<<10);
        }
        gfx_set_palette(32,224,fade);
    } else gfx_set_palette(32,224,palette);
    gfx_blit_band(0,160,background);
    if(page==SC_FRONT_BRIEFING) {
        front_briefing();
        front_load_error();
        return;
    }
    sc64_chrome_draw_tab(page==SC_FRONT_HOME?SC64_CHROME_TAB_OVERVIEW:
        page==SC_FRONT_CAMPAIGN?SC64_CHROME_TAB_UNITS:
        page==SC_FRONT_MISSION?SC64_CHROME_TAB_BUILDINGS:SC64_CHROME_TAB_RESOURCES);
    if(page==SC_FRONT_HOME) {
        front_logo();
        front_center_text(57,"Sector Command",C_DIM);
        front_button(96,68,128,22,"Campaign",race_front.row==0);
        front_button(96,92,128,22,"Custom Game",race_front.row==1);
        front_button(96,116,128,22,"Load Game",race_front.row==2);
        front_text(24,123,"Koprulu Sector",C_DIM);
        front_uplink();
        for(int i=0;i<8;i++) {
            int x=17+(i*37)%96,y=50+(i*23)%62;
            if(((race_front.ticks/8+i)%7)==0)gfx_pixel(x,y,C_SILVER);
        }
        front_footer("Up/Down Select  A Enter  L Load");
    } else if(page==SC_FRONT_CAMPAIGN) {
#ifdef SC64_CAMPAIGN_RACE
        front_header("Campaign",0);
        front_button(96,70,128,22,
            faction==SC_FRONT_TERRAN?"TERRAN":faction==SC_FRONT_ZERG?"ZERG":"PROTOSS",1);
        front_text(121,108,front_campaign_title(faction),C_GOLD);
#else
#ifdef SC64_UNIFIED
        front_header("Choose a Campaign",0);
        front_button(96,52,128,22,"Terran",race_front.row==0);
        front_button(96,81,128,22,"Zerg",race_front.row==1);
        front_button(96,110,128,22,"Protoss",race_front.row==2);
        front_text(24,135,front_campaign_title(faction),C_GOLD);
#else
        front_header("Choose a Campaign",0);
        front_button(96,62,128,22,"Zerg",race_front.row==0);
        front_button(96,98,128,22,"Protoss",race_front.row==1);
        front_text(24,128,faction?"III: The Fall":"II: The Overmind",C_GOLD);
#endif
#endif
        front_uplink();front_footer("Arrows Change  A Select  B Back");
    } else if(page==SC_FRONT_MISSION) {
        front_header(front_campaign_title(faction),
#ifdef SC64_UNIFIED
                     faction==SC_FRONT_TERRAN?"Terran Campaign":faction==SC_FRONT_PROTOSS?"Protoss Campaign":"Zerg Campaign");
#else
                     faction?"Protoss Campaign":"Zerg Campaign");
#endif
        int catalog_faction=sc64_front_catalog_faction(faction);
        int available=sc64_catalog_playable_count(catalog_faction);
        for(int i=0;i<2;i++) {
            int mission=sc64_catalog_playable_at(catalog_faction,race_front.mission_page*2+i);
            if(mission<0) continue;
            front_button(96,53+i*36,128,30,front_mission_title(mission),race_front.row==i);
            if(race_completed&(1<<mission))gfx_rect(91,64+i*36,4,4,C_GREEN);
        }
        front_text(24,127,(race_completed&(1<<(sc64_catalog_playable_at(catalog_faction,race_front.mission_page*2+race_front.row))))?"Mission Complete":"Awaiting Your Orders",C_DIM);
        if((available+1)/2>1) front_text(174,127,"L/R Page",C_GOLD);
        front_uplink();front_footer("U/D Pick  L/R Page  A Brief  B Back");
    }
#ifdef SC64_SHOWCASE_ENABLED
    else if(page==SC_FRONT_SHOWCASE) {
        front_header("Showcase Scenes","Filming Setups");
        for(int i=0;i<8;i++)
            front_button(104,44+i*11,120,11,front_showcase_title(i),race_front.row==i);
        front_text(24,58,"Staged",C_GOLD);
        front_text(24,70,"Battles",C_GOLD);
        front_text(24,94,"Quick",C_DIM);
        front_text(24,104,"Restart",C_DIM);
        if(race_front.row==0) {
            front_text(24,119,"Carrier",C_SILVER);
            front_text(24,129,"Preview",C_SILVER);
        }
        front_footer("Up/Down Scene  A Start  B Back");
    }
#endif
    else if(page==SC_FRONT_SKIRMISH) {
        front_header("Battle Setup",0);
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
            front_button(104,50+row*12,120,11,option,race_front.row==row);
        }
        front_button(104,50+(3+race_front.bots)*12,120,11,"LAUNCH",race_front.row==3+race_front.bots);
        front_footer("U/D Row L/R Edit A OK B Back");
#else
        front_text(24,66,"Faction",C_DIM);
        front_button(96,61,128,22,sc64_race_names[race_front.race],race_front.row==0);
        front_text(24,87,"Computer",C_DIM);
        front_text(24,98,"Opponents",C_DIM);
        char opponents[6]={'<',' ',(char)('0'+race_front.bots),' ','>',0};
        front_button(96,85,128,22,opponents,race_front.row==1);
        front_button(96,111,128,22,"LAUNCH",race_front.row==2);
        front_text(24,126,"Free for All",C_DIM);
        front_footer("U/D Row L/R Edit A OK B Back");
#endif
    }
    front_load_error();
}

#endif
