#ifndef SC64_FRONTEND_DRAW_H
#define SC64_FRONTEND_DRAW_H

#include "sc64_font.h"

static void front_text(int x, int y, const char *text, u8 color)
{
    sc64_font_text(x, y, x < 240 ? 240 - x : 0, text, color);
}

static void front_right_text(int right, int y, const char *text, u8 color)
{
    for (const char *letter = text; *letter; ++letter) right -= 6;
    gfx_text(right, y, text, color);
}

/* The menus use the common framebuffer palette and authored GBA glyphs. */
static void front_shell(const char *title, const char *section)
{
    gfx_rect(0, 0, 240, 23, C_PANEL);
    gfx_rect(0, 22, 240, 1, C_EDGE);
    gfx_rect(0, 22, 52, 1, C_CYAN);
    for (int x = 8; x < 240; x += 16)
        gfx_rect(x, 23, 1, 113, C_PANEL);
    for (int y = 32; y < 136; y += 16)
        gfx_rect(0, y, 240, 1, C_PANEL);
    gfx_text(12, 8, title, C_WHITE);
    front_right_text(230, 8, section, C_CYAN);
}

static const char *front_mission_title(int mission)
{
    const Sc64MissionInfo *info = sc64_catalog_get(mission);
    return info ? info->title : "Unknown Mission";
}

static const char *front_campaign_title(int faction)
{
    static const char *const names[] = { "Terran Campaign", "The Overmind", "The Fall" };
    return names[sc64_front_catalog_faction(faction)];
}

static void front_panel(int x, int y, int width, int height)
{
    gfx_rect(x, y, width, height, C_PANEL);
    gfx_frame(x, y, width, height, C_EDGE);
}

static void front_row(int x, int y, int width, int height,
                      const char *label, const char *value, int selected)
{
    int text_y = y + (height - 7) / 2;
    gfx_rect(x, y, width, height, selected ? C_BLUE_DARK : C_PANEL);
    gfx_frame(x, y, width, height, selected ? C_GOLD : C_EDGE);
    if (selected) gfx_rect(x + 3, y + 2, 2, height - 4, C_GOLD);
    gfx_text(x + 10, text_y, label, selected ? C_WHITE : C_SILVER);
    if (value) front_right_text(x + width - 8, text_y, value, selected ? C_GOLD : C_CYAN);
}

static void front_footer(const char *first, const char *second)
{
    gfx_rect(0, 136, 240, 24, C_PANEL);
    gfx_rect(0, 136, 240, 1, C_EDGE);
    gfx_text(12, 140, first, C_WHITE);
    gfx_text(12, 150, second, C_SILVER);
}

static void front_faction_mark(int x, int y, int faction)
{
    static const u8 colors[] = { C_TERRAN, C_ZERG, C_PROTOSS };
    gfx_rect(x, y, 7, 7, colors[faction]);
    gfx_frame(x, y, 7, 7, C_SILVER);
}

static void front_load_error(void)
{
    if (!race_front_load_error) return;
    gfx_rect(24, 80, 192, 42, C_PANEL);
    gfx_frame(24, 80, 192, 42, C_GOLD);
    gfx_text(66, 90, "NO SAVED GAME FOUND", C_GOLD);
    gfx_text(42, 104, "START A CAMPAIGN OR BATTLE", C_WHITE);
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

static void front_draw_home(void)
{
    front_shell("STARCRAFT GBA", "SINGLE PLAYER");
    front_panel(10, 33, 82, 92);
    gfx_frame(19, 47, 64, 64, C_EDGE);
    gfx_rect(50, 48, 1, 62, C_EDGE);
    gfx_rect(20, 79, 62, 1, C_EDGE);
    gfx_diamond(50, 79, 26, 19, C_EDGE);
    gfx_diamond(50, 79, 24, 17, C_PANEL);
    gfx_diamond(50, 79, 14, 10, C_CYAN);
    gfx_diamond(50, 79, 12, 8, C_BLUE_DARK);
    gfx_rect(48, 77, 5, 5, C_GOLD);
    front_row(102, 33, 128, 26, "CAMPAIGN", NULL, race_front.row == 0);
    front_row(102, 66, 128, 26, "CUSTOM GAME", NULL, race_front.row == 1);
    front_row(102, 99, 128, 26, "LOAD GAME", NULL, race_front.row == 2);
    front_footer("D-PAD UP/DOWN PICK      A SELECT",
                 "D-PAD RIGHT OPTIONS     L / B LOAD");
}

static void front_draw_campaign(void)
{
    static const char *const episodes[] = { "EPISODE I", "EPISODE II", "EPISODE III" };
    front_shell("CAMPAIGN", "CHOOSE A FACTION");
    for (int i = 0; i < sc64_front_campaign_count(); ++i) {
        int faction = sc64_front_catalog_faction(sc64_front_campaign_faction_at(i));
        int y = 33 + i * 33;
        front_row(10, y, 220, 26, episodes[faction],
                  sc64_race_names[faction], race_front.row == i);
        front_faction_mark(164, y + 9, faction);
    }
    front_footer("D-PAD UP/DOWN PICK      A MISSIONS", "B BACK");
}

static void front_draw_missions(void)
{
    int faction = sc64_front_catalog_faction(race_front.faction);
    int available = sc64_catalog_playable_count(faction);
    int pages = (available + 1) / 2;
    char page_label[] = "PAGE 0/0";
    page_label[5] += race_front.mission_page + 1;
    page_label[7] += pages;
    front_shell(front_campaign_title(race_front.faction), "CAMPAIGN / MISSIONS");
    front_faction_mark(13, 32, faction);
    gfx_text(25, 32, sc64_race_names[faction], C_SILVER);
    front_right_text(228, 32, page_label, C_CYAN);
    for (int i = 0; i < 2; ++i) {
        int mission = sc64_catalog_playable_at(faction, race_front.mission_page * 2 + i);
        const Sc64MissionInfo *info = sc64_catalog_get(mission);
        if (!info) continue;
        int y = 46 + i * 41;
        front_row(10, y, 220, 34, info->title,
                  ((unsigned)race_completed & (1u << mission)) ? "CLEARED" : NULL,
                  race_front.row == i);
        gfx_text(20, y + 4, "MISSION", C_SILVER);
        gfx_num(68, y + 4, info->chapter, C_CYAN);
        gfx_rect(19, y + 23, 200, 1, C_EDGE);
    }
    front_footer("D-PAD UP/DOWN PICK      A BRIEFING",
                 pages > 1 ? "D-PAD LEFT/RIGHT PAGE   B BACK" : "B BACK");
}

static void front_draw_options(void)
{
    front_shell("OPTIONS", "HOME / OPTIONS");
    front_row(10, 33, 220, 26, "CURSOR SPEED",
              sc64_front_setting_cursor_name(&race_front.settings),
              race_front.row == SC_FRONT_SETTING_CURSOR);
    front_row(10, 66, 220, 26, "MENU SOUND", race_front.settings.quiet_menu ? "OFF" : "ON",
              race_front.row == SC_FRONT_SETTING_SOUND);
    front_row(10, 99, 220, 26, "BACK TO HOME", NULL,
              race_front.row == SC_FRONT_SETTING_BACK);
    front_footer("D-PAD UP/DOWN PICK      B BACK",
                 race_front.row == SC_FRONT_SETTING_BACK ? "A RETURN" :
                 race_front.row == SC_FRONT_SETTING_SOUND ? "D-PAD LEFT/RIGHT OR A TOGGLE" :
                 "D-PAD LEFT/RIGHT EDIT");
}

#ifdef SC64_IMPORTED_SKIRMISH
static const char *front_map_title(int map)
{
    return map == SC64_MAP_BLOOD_BATH ? "BLOOD BATH" : "OUTPOST";
}

static void front_draw_map_browser(void)
{
    front_shell("MAP SELECT", "CUSTOM GAME / MAP");
    for (int i = 0; i < SC64_FRONT_MAP_COUNT; ++i) {
        front_row(10, 34 + i * 29, 126, 24, front_map_title(i),
                  race_front.map == i ? "SET" : NULL, race_front.row == i);
    }
    front_panel(144, 34, 86, 92);
    enum { PREVIEW_X = 163, PREVIEW_Y = 53, PREVIEW_SIZE = 48 };
    const Sc64SkirmishMap *map = race_front.row > SC64_MAP_OUTPOST ?
        &sc64_skirmish_maps[race_front.row - 1] : NULL;
    int width = map ? map->width : 64;
    int height = map ? map->height : 64;
    gfx_text(151, 41, "START SITES", C_CYAN);
    gfx_rect(PREVIEW_X, PREVIEW_Y, PREVIEW_SIZE, PREVIEW_SIZE, C_BLACK);
    gfx_frame(PREVIEW_X, PREVIEW_Y, PREVIEW_SIZE, PREVIEW_SIZE, C_EDGE);
    gfx_rect(PREVIEW_X + PREVIEW_SIZE / 2, PREVIEW_Y + 1, 1, PREVIEW_SIZE - 2, C_PANEL);
    gfx_rect(PREVIEW_X + 1, PREVIEW_Y + PREVIEW_SIZE / 2, PREVIEW_SIZE - 2, 1, C_PANEL);
    for (int i = 0; i < 4; ++i) {
        /* Authored Outpost places bases 10 tiles in from its corners. */
        int x = map ? map->start_x[i] : (i & 1 ? width - 10 : 10) * 16;
        int y = map ? map->start_y[i] : (i & 2 ? height - 10 : 10) * 16;
        int px = PREVIEW_X + 3 + x * (PREVIEW_SIZE - 7) / (width * 16);
        int py = PREVIEW_Y + 3 + y * (PREVIEW_SIZE - 7) / (height * 16);
        gfx_rect(px - 1, py - 1, 3, 3, C_GOLD);
    }
    gfx_num(163, 109, width, C_WHITE);
    gfx_text(183, 109, "X", C_SILVER);
    gfx_num(195, 109, height, C_WHITE);
    gfx_text(16, 103, "1 HUMAN / 1-3 CPU", C_SILVER);
    front_footer("D-PAD UP/DOWN PICK      A SET MAP", "B CANCEL");
}
#endif

static void front_draw_skirmish(void)
{
    char opponents[] = { (char)('0' + race_front.bots), 0 };
    front_shell("BATTLE SETUP", "CUSTOM GAME / LOBBY");
#ifdef SC64_IMPORTED_SKIRMISH
    enum { TOP = 27, STEP = 13, HEIGHT = 12 };
    const char *human =
#ifdef SC64_NATIVE_MELEE_STARTS
        race_front.race == SC_FRONT_HUMAN_RANDOM ? "Random" :
#endif
        sc64_race_names[race_front.race];
    front_row(10, TOP, 220, HEIGHT, "MAP", front_map_title(race_front.map), race_front.row == 0);
    front_row(10, TOP + STEP, 220, HEIGHT, "HUMAN RACE", human, race_front.row == 1);
    if ((unsigned)race_front.race < 3) front_faction_mark(164, TOP + STEP + 2, race_front.race);
    front_row(10, TOP + 2 * STEP, 220, HEIGHT, "OPPONENTS", opponents, race_front.row == 2);
    for (int row = SC_FRONT_BOT_FIRST_ROW; row < SC_FRONT_BOT_END_ROW; ++row) {
        int seat = row - SC_FRONT_BOT_FIRST_ROW;
        char label[] = { 'C', 'P', 'U', ' ', (char)('1' + seat), 0 };
        int race = race_front.bot_races[seat];
        const char *value = race == SC_FRONT_BOT_CLOSED ? "CLOSED" :
            race == SC_FRONT_BOT_RANDOM ? "RANDOM" : sc64_race_names[race];
        front_row(22, TOP + row * STEP, 208, HEIGHT, label, value, race_front.row == row);
        if ((unsigned)race < 3) front_faction_mark(164, TOP + row * STEP + 2, race);
    }
#ifdef SC64_NATIVE_MELEE_STARTS
    front_row(10, TOP + SC_FRONT_SKIRMISH_MODE_ROW * STEP, 220, HEIGHT, "MATCH TYPE",
              race_front.skirmish_melee ? "MELEE" : "FREE FOR ALL",
              race_front.row == SC_FRONT_SKIRMISH_MODE_ROW);
#endif
    int launch_selected = race_front.row == SC_FRONT_SKIRMISH_LAUNCH_ROW;
    front_row(10, TOP + SC_FRONT_SKIRMISH_LAUNCH_ROW * STEP, 220, HEIGHT,
              race_front.bots ? "START BATTLE" : "ADD AN OPPONENT", NULL, launch_selected);
    const char *edit = launch_selected ? "SELECT MAP" : "D-PAD LEFT/RIGHT EDIT   SELECT MAP";
#ifdef SC64_NATIVE_MELEE_STARTS
    if (race_front.row == SC_FRONT_SKIRMISH_MODE_ROW && race_front.map != SC64_MAP_BLOOD_BATH)
        edit = "SELECT MAP FOR MELEE";
#endif
    front_footer(launch_selected && race_front.bots ? "UP/DOWN ROW    A START     B BACK" :
                 launch_selected ? "UP/DOWN ROW    B BACK" : "UP/DOWN ROW  A GO TO START  B BACK", edit);
#else
    front_row(10, 33, 220, 26, "HUMAN RACE", sc64_race_names[race_front.race], race_front.row == 0);
    front_faction_mark(164, 42, race_front.race);
    front_row(10, 66, 220, 26, "OPPONENTS", opponents, race_front.row == 1);
    front_row(10, 99, 220, 26, "START BATTLE", NULL, race_front.row == 2);
    front_footer(race_front.row == 2 ? "D-PAD UP/DOWN ROW       A START" :
                 "D-PAD UP/DOWN ROW       A GO TO START", "D-PAD LEFT/RIGHT EDIT   B BACK");
#endif
}

#ifdef SC64_SHOWCASE_ENABLED
static void front_draw_showcase(void)
{
    front_shell("SHOWCASE SCENES", "SINGLE PLAYER");
    for (int i = 0; i < SC64_FRONT_SHOWCASE_COUNT; ++i)
        front_row(10, 27 + i * 13, 220, 12, sc64_front_showcase_name(i), NULL, race_front.row == i);
    front_footer("D-PAD UP/DOWN PICK      A START", "B BACK");
}
#endif

static void sc64_frontend_draw(void)
{
    switch (race_front.page) {
    case SC_FRONT_HOME: front_draw_home(); break;
    case SC_FRONT_CAMPAIGN: front_draw_campaign(); break;
    case SC_FRONT_MISSION: front_draw_missions(); break;
    case SC_FRONT_OPTIONS: front_draw_options(); break;
    case SC_FRONT_SKIRMISH: front_draw_skirmish(); break;
#ifdef SC64_IMPORTED_SKIRMISH
    case SC_FRONT_MAP_BROWSER: front_draw_map_browser(); break;
#endif
#ifdef SC64_SHOWCASE_ENABLED
    case SC_FRONT_SHOWCASE: front_draw_showcase(); break;
#endif
    case SC_FRONT_BRIEFING: {
        unsigned age = race_front.ticks - race_front_transition;
        if (age < 12) {
            u16 fade[224];
            int amount = (int)age + 1;
            for (int i = 0; i < 224; ++i) {
                u16 p = sc64_briefing_palette[i];
                fade[i] = ((p & 31) * amount / 12) |
                    ((((p >> 5) & 31) * amount / 12) << 5) |
                    ((((p >> 10) & 31) * amount / 12) << 10);
            }
            gfx_set_palette(32, 224, fade);
        } else {
            gfx_set_palette(32, 224, sc64_briefing_palette);
        }
        gfx_blit_band(0, 160, sc64_briefing_background);
        front_briefing();
        break;
    }
    }
    front_load_error();
}

#endif
