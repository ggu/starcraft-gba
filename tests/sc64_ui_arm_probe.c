/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * ARM probe for the imported interface (replaces common/main.c).
 *
 * An embedded key tape drives the shipped game_update()/game_render() through
 * HOME, Campaign, Missions, the first briefing (reveal, paging, objectives),
 * deployment, the order list or command card, pause, save and load. Results
 * go to the mGBA debug log; no debugger reads or writes game state.
 *
 * Run it twice against one isolated save directory. A first process finds no
 * save at HOME (L), plays the route, saves from the pause list and reloads in
 * place. A second, fresh process loads that SRAM from HOME and reports the
 * same state digest. Digests are taken on the frame that saves or loads,
 * before the world advances. Emulator evidence only.
 */
#include "../starcraft/game.c"

volatile u16 demo_keys;

#define DBG_ENABLE (*(volatile u16 *)0x04FFF780)
#define DBG_FLAGS (*(volatile u16 *)0x04FFF700)
#define DBG_TEXT ((volatile char *)0x04FFF600)

static char line[200];
static unsigned line_len;
static u16 probe_previous;
static int probe_failed;

static void out_text(const char *s) { while (*s && line_len < sizeof(line) - 1) line[line_len++] = *s++; }
static void out_num(long v)
{
    char tmp[12]; int n = 0; unsigned long m;
    if (v < 0) { out_text("-"); m = (unsigned long)(-v); } else m = (unsigned long)v;
    do { tmp[n++] = (char)('0' + m % 10u); m /= 10u; } while (m && n < 11);
    while (n) { char c[2] = { tmp[--n], 0 }; out_text(c); }
}
static void out_hex(u32 v)
{
    static const char digits[] = "0123456789abcdef";
    char c[2] = { 0, 0 };
    for (int s = 28; s >= 0; s -= 4) { c[0] = digits[v >> s & 15u]; out_text(c); }
}
static void out_flush(void)
{
    for (unsigned i = 0; i < line_len; ++i) DBG_TEXT[i] = line[i];
    DBG_TEXT[line_len] = 0;
    DBG_FLAGS = 0x100 | 2; /* warn level: visible with a narrow log mask */
    line_len = 0;
}
static void check(int ok, const char *what)
{
    out_text(ok ? "UI PASS " : "UI FAIL "); out_text(what);
    out_text(" screen="); out_num(screen); out_text(" page="); out_num(race_front.page);
    out_text(" row="); out_num(race_front.row); out_text(" brief="); out_num((long)race_front.briefing_page);
    out_text("/"); out_num((long)race_front.briefing_pages); out_text(" revealed=");
    out_num(race_front.briefing_revealed); out_flush();
    if (!ok) probe_failed = 1;
}

/* One frame of the shipped main-loop work. */
static void frame_with(u16 held)
{
    demo_keys = held;
    game_update(held, held & (u16)~probe_previous);
    audio_update();
    game_render();
    probe_previous = held;
}
static void press(u16 key) { frame_with(key); frame_with(0); }
static void idle(int frames) { while (frames-- > 0) frame_with(0); }

/* The page on screen after game_render(): mode 4 page select. */
static const volatile u8 *shown(void)
{
    return (const volatile u8 *)((*(volatile u16 *)0x04000000 & 0x10) ? 0x0600A000 : 0x06000000);
}
static u32 band_hash(int top, int rows)
{
    const volatile u8 *p = shown();
    u32 h = 2166136261u;
    for (int i = top * 240; i < (top + rows) * 240; ++i) h = (h ^ p[i]) * 16777619u;
    return h;
}
static int count_color(int x, int y, int w, int h, u8 color)
{
    const volatile u8 *p = shown();
    int n = 0;
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx) n += p[yy * 240 + xx] == color;
    return n;
}

/* Units, buildings, wallet and clock: the state a save must restore. */
static u32 state_digest(void)
{
    u32 h = 2166136261u;
#define MIX(v) (h = (h ^ (u32)(v)) * 16777619u)
    MIX(frame); MIX(minerals); MIX(gas); MIX(supply_used); MIX(supply_cap);
    MIX(race_selected_mission); MIX(race_human); MIX(race_mode);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive) { MIX(i); MIX(u[i].type); MIX(u[i].owner); MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp); }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive) { MIX(i); MIX(b[i].type); MIX(b[i].owner); MIX(b[i].x); MIX(b[i].y); MIX(b[i].hp); }
#undef MIX
    return h;
}
static void report_state(const char *what)
{
    int units = 0, buildings = 0;
    for (int i = 0; i < MAX_UNITS; ++i) units += u[i].alive;
    for (int i = 0; i < MAX_BUILDINGS; ++i) buildings += b[i].alive;
    out_text("UI STATE "); out_text(what);
    out_text(" digest="); out_hex(state_digest());
    out_text(" frame="); out_num((long)frame);
    out_text(" mission="); out_num(race_selected_mission);
    out_text(" units="); out_num(units); out_text(" buildings="); out_num(buildings);
    out_text(" minerals="); out_num(minerals); out_text(" gas="); out_num(gas);
    out_flush();
}

/* The order list or command card opens over the playfield and never paints
 * the resource bar or the HUD's top edge. Economy ticks can change the bar
 * between frames, so retry until the wallet is unchanged across the press. */
static void check_command_menu(void)
{
    int clean = 0;
    for (int attempt = 0; attempt < 8 && !clean; ++attempt) {
        int m0 = minerals, g0 = gas, s0 = supply_used, c0 = supply_cap;
        u32 top0;
        if (menu_open) press(KEY_B);
        press(KEY_SELECT); /* drops a selection; with none it pauses */
        if (screen == SCREEN_PAUSE) press(KEY_START);
        idle(2);
        top0 = band_hash(0, 12);
        press(KEY_B);
        if (!menu_open) break;
        clean = m0 == minerals && g0 == gas && s0 == supply_used && c0 == supply_cap;
        if (clean) check(band_hash(0, 12) == top0, "menu leaves the resource bar intact");
    }
    check(!selected_count() && menu_open && menu_cursor == MENU_MOVE, "B opens the orders on Move");
    check(count_color(0, 120, 240, 1, C_EDGE) == 240, "menu leaves the HUD edge intact");
#ifdef SC64_COMMAND_ICONS
    check(card_fits(), "orders use the command card");
    press(KEY_RIGHT);
    check(menu_cursor == MENU_STOP, "card RIGHT reaches Stop");
    press(KEY_RIGHT);
    check(menu_cursor == MENU_ATTACK, "card RIGHT reaches Attack");
    press(KEY_RIGHT);
    check(menu_cursor == MENU_MOVE, "card RIGHT wraps to Move");
    press(KEY_DOWN);
    check(menu_cursor == card_slot_command(3), "card DOWN reaches the next row");
    press(KEY_UP);
    check(menu_cursor == MENU_MOVE, "card UP returns to Move");
    {
        int rows = (field_menu_count() + CARD_COLUMNS - 1) / CARD_COLUMNS;
        int w = CARD_COLUMNS * CARD_PITCH + 3, h = rows * CARD_PITCH + 3;
        int x = 238 - w, y = 119 - h < 12 ? 12 : 119 - h;
        check(count_color(x, y, w, h, C_GOLD) > 0, "card draws on screen");
    }
#else
    {
        int count = field_menu_count(), steps = count > 5 ? 5 : count - 1;
        for (int k = 0; k < steps; ++k) press(KEY_DOWN);
        check(menu_cursor == steps, "list DOWN follows the cursor");
        check(count_color(100, 14, 138, 4 * 22 + 4, C_PANEL) > 0, "list draws on screen");
        for (int k = 0; k < steps; ++k) press(KEY_UP);
        check(menu_cursor == MENU_MOVE, "list UP returns to Move");
    }
#endif
    check(count_color(0, 120, 240, 1, C_EDGE) == 240, "navigation leaves the HUD edge intact");
    press(KEY_B);
    check(!menu_open, "B closes the orders");
}

static void run_first_process(void)
{
    unsigned pages;
    check(race_front.page == SC_FRONT_CAMPAIGN || race_front.page == SC_FRONT_HOME, "HOME after empty load");
#ifndef SC64_CAMPAIGN_RACE /* single-race profiles go from HOME to Missions */
    press(KEY_A);
    check(screen == SCREEN_TITLE && race_front.page == SC_FRONT_CAMPAIGN, "A opens Campaign");
    idle(20);
#endif
    press(KEY_A);
    check(screen == SCREEN_TITLE && race_front.page == SC_FRONT_MISSION, "A opens Missions");
    idle(20);
    press(KEY_A);
    pages = race_front.briefing_pages;
    check(screen == SCREEN_TITLE && race_front.page == SC_FRONT_BRIEFING && pages >= 2,
          "A opens a paged briefing");
    check(race_front.briefing_page == 0 && !race_front.briefing_revealed, "briefing starts unrevealed");
    press(KEY_A);
    check(race_front.briefing_page == 0 && race_front.briefing_revealed, "A reveals the page");
    press(KEY_A);
    check(race_front.briefing_page == 1, "A advances the page");
    press(KEY_LEFT);
    check(race_front.briefing_page == 0, "LEFT returns a page");
    press(KEY_RIGHT);
    check(race_front.briefing_page == 1, "RIGHT advances a page");
    for (int guard = 0; guard < 512 && race_front.briefing_page + 1 < pages; ++guard) press(KEY_A);
    check(race_front.briefing_page + 1 == pages, "A reaches the objectives page");
    idle(80);
    out_text("UI BRIEFING pages="); out_num((long)pages);
    out_text(" mission="); out_num(race_front.mission);
    out_text(" screen="); out_hex(band_hash(0, 160)); out_flush();
    press(KEY_START);
    check(screen == SCREEN_PLAY && !race_mode, "START deploys the campaign");
    idle(60);
    check_command_menu();

    press(KEY_START);
    check(screen == SCREEN_PAUSE && pause_cursor == 0, "START pauses on Resume");
    idle(2);
    out_text("UI PAUSE screen="); out_hex(band_hash(0, 160)); out_flush();
    press(KEY_DOWN);
    check(pause_cursor == 1, "DOWN reaches Save");
    report_state("saved");
    press(KEY_A);
    check(screen == SCREEN_PAUSE && (pause_notice & 0xc0) == UI_NOTICE_SAVED, "A saves");
    press(KEY_DOWN);
    check(pause_cursor == 2, "DOWN reaches Load");
    frame_with(KEY_A); /* digest the loaded state before it advances a tick */
    check(screen == SCREEN_PLAY, "A loads and resumes");
    report_state("reloaded");
    frame_with(0);
    idle(30);
    press(KEY_START);
    press(KEY_A);
    check(screen == SCREEN_PLAY, "A on Resume resumes");
}

static void run_second_process(void)
{
    idle(30);
    press(KEY_START);
    check(screen == SCREEN_PAUSE, "START pauses the loaded game");
    idle(2);
    out_text("UI PAUSE screen="); out_hex(band_hash(0, 160)); out_flush();
    press(KEY_START);
    check(screen == SCREEN_PLAY, "START resumes the loaded game");
    check_command_menu();
}

int main(void)
{
    gba_init();
    DBG_ENABLE = 0xC0DE;
    game_init();
    out_text("UI START debug="); out_num(DBG_ENABLE == 0x1DEA); out_flush();
    idle(30);
    check(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME, "boots to HOME");
    out_text("UI HOME screen="); out_hex(band_hash(0, 160)); out_flush();
    frame_with(KEY_L);
    if (screen == SCREEN_PLAY) {
        out_text("UI PROCESS second"); out_flush();
        report_state("loaded");
        frame_with(0);
        run_second_process();
    } else {
        frame_with(0);
        out_text("UI PROCESS first"); out_flush();
        check(race_front_load_error > 0, "HOME L reports no save");
        idle(100);
        run_first_process();
    }
    out_text(probe_failed ? "UI DONE fail" : "UI DONE ok"); out_flush();
    for (;;) idle(1);
}
