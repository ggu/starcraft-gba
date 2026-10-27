/* Default-profile menus and controller feedback (SC_DEFAULT_UI).
 *
 * Included by game.c for the original-art skirmish only. Everything here is
 * presentation or input routing: it calls the same order, placement,
 * training, save and research functions as before and adds no saved state.
 * Imported SC64 profiles keep their own menus, so their replays are
 * unaffected. */
#ifndef SC_DEFAULT_UI
#error "field_ui.h is the default-profile UI"
#endif

static int currently_visible(int x, int y);
static int nearest_refinery(int x, int y);
static const char *unit_name(int type);

/* What A does at the cursor right now. The HUD hint, the cursor and
 * interact() all read this so the prompt never disagrees with the action. */
enum {
    UI_A_NONE, UI_A_SELECT, UI_A_DESELECT, UI_A_TRAIN, UI_A_ASSIGN_GAS,
    UI_A_GATHER_GAS, UI_A_ATTACK, UI_A_MOVE_HERE, UI_A_ATTACK_HERE,
    UI_A_PLACE, UI_A_GROUP
};

/* Facts the cursor, list and HUD all need, gathered in one pass per drawn
 * frame (field_ui_refresh) instead of a pool scan per row. */
static int ui_selected, ui_worker, ui_action, ui_site, ui_site_x, ui_site_y, ui_site_blocked;
static unsigned ui_built; /* completed friendly structure types, as bits */

static const char *const ui_command_label[MENU_COUNT] = {
    "MOVE", "ATTACK MOVE", "BARRACKS", "SUPPLY DEPOT", "REFINERY",
    "ACADEMY", "ENG BAY", "WEAPONS", "ARMOR", "STOP"
};
static const char *const ui_command_note[MENU_COUNT] = {
    "WALK TO A POINT", "FIGHT ALONG THE WAY", "TRAINS INFANTRY",
    "+8 SUPPLY", "HARVESTS GEYSER GAS", "UNLOCKS FIREBAT MEDIC",
    "UNLOCKS UPGRADES", "+1 INFANTRY DAMAGE", "+1 INFANTRY ARMOR",
    "HALT AND HOLD"
};
static const u8 ui_command_building[MENU_COUNT] = {
    0, 0, B_BARRACKS, B_DEPOT, B_REFINERY, B_ACADEMY, B_ENG_BAY, 0, 0, 0
};

static int
ui_text_width(const char *text)
{
    int n = 0;
    while (text[n]) n++;
    return n * 6 - 1;
}

/* Right-aligned number in one text call. */
static void
ui_num_right(int right, int y, int value, u8 color)
{
    char text[8];
    int n = 7;
    text[n] = 0;
    do { text[--n] = (char)('0' + value % 10); value /= 10; } while (value && n);
    gfx_text(right - (7 - n) * 6 + 1, y, text + n, color);
}

static int
ui_num_width(int value)
{
    int n = 1;
    while (value >= 10) { value /= 10; n++; }
    return n * 6 - 1;
}

/* Framed panel: dark fill, edge, an accent rule under an optional title. */
static void
ui_panel(int x, int y, int w, int h, u8 accent, const char *title)
{
    gfx_rect(x, y, w, h, C_PANEL);
    gfx_frame(x, y, w, h, C_EDGE);
    gfx_rect(x, y, 3, 1, accent); gfx_rect(x, y, 1, 3, accent);
    gfx_rect(x + w - 3, y, 3, 1, accent); gfx_rect(x + w - 1, y, 1, 3, accent);
    gfx_rect(x, y + h - 1, 3, 1, accent); gfx_rect(x, y + h - 3, 1, 3, accent);
    gfx_rect(x + w - 3, y + h - 1, 3, 1, accent);
    gfx_rect(x + w - 1, y + h - 3, 1, 3, accent);
    if (title) {
        gfx_rect(x + 2, y + 2, w - 4, 11, C_UI);
        gfx_rect(x + 2, y + 13, w - 4, 1, accent);
        gfx_text(x + 5, y + 4, title, accent);
    }
}

/* A button cap in front of its action, e.g. [A] SELECT. Returns the x after it. */
static int
ui_key(int x, int y, const char *key, const char *action, u8 color)
{
    int w = ui_text_width(key) + 4;
    gfx_rect(x, y - 1, w, 9, C_SILVER);
    gfx_rect(x, y + 8, w, 1, C_DIM);
    gfx_text(x + 2, y, key, C_PANEL);
    x += w + 3;
    if (action) {
        gfx_text(x, y, action, color);
        x += ui_text_width(action) + 6;
    }
    return x;
}

/* Selected-row bar shared by every list. */
static void
ui_highlight(int x, int y, int w)
{
    gfx_rect(x, y, w, 9, C_UI);
    gfx_rect(x, y, 2, 9, C_GOLD);
}

static void
ui_unit_glyph(int type, int x, int y, u8 col)
{
    if (type == U_SCV) {
        gfx_rect(x - 3, y - 2, 7, 5, col);
        gfx_pixel(x + 2, y - 3, C_SILVER);
        gfx_pixel(x - 3, y + 3, C_GOLD);
    } else if (type == U_MARINE) {
        gfx_rect(x - 2, y - 4, 5, 8, col);
        gfx_rect(x - 3, y - 5, 6, 2, C_SILVER);
        gfx_line(x + 1, y, x + 5, y - 2, C_SILVER);
    } else if (type == U_FIREBAT) {
        gfx_rect(x - 3, y - 4, 7, 8, col);
        gfx_rect(x - 4, y - 5, 8, 2, C_ORANGE);
        gfx_pixel(x + 5, y, C_ORANGE);
    } else if (type == U_MEDIC) {
        gfx_rect(x - 3, y - 4, 7, 8, C_WHITE);
        gfx_rect(x, y - 2, 1, 5, C_RED);
        gfx_rect(x - 2, y, 5, 1, C_RED);
    } else if (type == U_ZERGLING) {
        gfx_diamond(x, y, 5, 3, col);
        gfx_line(x - 3, y - 1, x - 6, y - 4, C_BLOOD);
        gfx_line(x + 3, y - 1, x + 6, y - 4, C_BLOOD);
    } else {
        gfx_diamond(x, y, 5, 4, col);
        gfx_pixel(x - 4, y - 3, C_PURPLE);
        gfx_pixel(x + 4, y - 3, C_PURPLE);
    }
}

/* Miniature of the in-game structure art, centered on x,y (about 11x10). */
static void
ui_building_glyph(int type, int x, int y, u8 col)
{
    if (type == B_HATCHERY) {
        gfx_diamond(x, y, 7, 4, C_ZERG);
        gfx_diamond(x, y, 4, 2, C_PURPLE);
        return;
    }
    gfx_rect(x - 5, y - 3, 11, 8, C_STONE);
    gfx_rect(x - 5, y - 3, 11, 1, C_SILVER);
    gfx_rect(x - 3, y - 5, 7, 2, col);
    if (type == B_COMMAND) gfx_rect(x - 2, y - 1, 5, 3, C_GOLD);
    else if (type == B_BARRACKS) gfx_rect(x - 1, y - 1, 3, 3, C_ORANGE);
    else if (type == B_DEPOT) gfx_rect(x - 3, y - 1, 7, 3, C_BLUE_DARK);
    else if (type == B_REFINERY) gfx_rect(x - 1, y - 2, 3, 6, C_GREEN_DARK);
    else {
        gfx_rect(x - 2, y - 1, 5, 4, C_PURPLE);
        gfx_pixel(x, y, type == B_ACADEMY ? C_CYAN : C_GOLD);
    }
}

static void
ui_command_icon(int command, int x, int y)
{
    if (command == MENU_MOVE) {
        gfx_line(x - 4, y + 3, x + 3, y - 4, C_GREEN);
        gfx_line(x - 3, y + 3, x + 3, y - 3, C_GREEN);
        gfx_rect(x, y - 4, 4, 1, C_GREEN);
        gfx_rect(x + 3, y - 4, 1, 4, C_GREEN);
    } else if (command == MENU_ATTACK) {
        gfx_frame(x - 3, y - 3, 7, 7, C_RED);
        gfx_rect(x - 5, y, 11, 1, C_RED);
        gfx_rect(x, y - 5, 1, 11, C_RED);
        gfx_pixel(x, y, C_WHITE);
    } else if (command == MENU_WEAPONS) {
        gfx_line(x - 3, y + 3, x + 3, y - 3, C_SILVER);
        gfx_line(x - 2, y + 3, x + 3, y - 2, C_WHITE);
        gfx_line(x - 4, y + 1, x - 1, y + 4, C_GOLD);
    } else if (command == MENU_ARMOR) {
        gfx_rect(x - 3, y - 4, 7, 5, C_SILVER);
        gfx_rect(x - 2, y + 1, 5, 2, C_SILVER);
        gfx_pixel(x, y + 3, C_SILVER);
        gfx_rect(x - 1, y - 3, 3, 4, C_BLUE);
    } else if (command == MENU_STOP) {
        gfx_rect(x - 4, y - 4, 9, 9, C_RED);
        gfx_rect(x - 2, y - 2, 5, 5, C_RED_DARK);
        gfx_rect(x - 2, y, 5, 1, C_WHITE);
    } else ui_building_glyph(ui_command_building[command], x, y, C_TERRAN);
}

static const char *
ui_building_name(int type)
{
    return type == B_COMMAND ? "COMMAND CENTER" : type == B_BARRACKS ? "BARRACKS" :
        type == B_DEPOT ? "SUPPLY DEPOT" : type == B_REFINERY ? "REFINERY" :
        type == B_ACADEMY ? "ACADEMY" : type == B_ENG_BAY ? "ENGINEERING BAY" :
        type == B_HATCHERY ? "HATCHERY" : "STRUCTURE";
}

/* Why a structure cannot be placed at x,y right now, or 0 when it can.
 * Mirrors start_build()'s checks and order. */
enum {
    UI_SITE_OK, UI_SITE_WORKER, UI_SITE_TECH, UI_SITE_MINERALS,
    UI_SITE_BLOCKED, UI_SITE_FULL, UI_SITE_GAS
};

/* has_tech_for() over the cached structure bits. */
static int
ui_has_tech(int type)
{
    int needs = type == B_BARRACKS ? B_DEPOT : type == B_ACADEMY ? B_BARRACKS :
        type == B_ENG_BAY ? B_COMMAND : -1;
    return needs < 0 || (ui_built >> needs & 1);
}

static int
ui_site_problem(int type, int x, int y)
{
    if (!ui_has_tech(type)) return UI_SITE_TECH;
    if (minerals < building_cost(type)) return UI_SITE_MINERALS;
    if (gas < building_gas_cost(type)) return UI_SITE_GAS;
    if (!can_place(type, x, y)) return UI_SITE_BLOCKED;
    if (ui_worker < 0) return UI_SITE_WORKER;
    if (find_build_slot() < 0) return UI_SITE_FULL;
    return UI_SITE_OK;
}

/* Structures snap to an 8-pixel grid so rows line up; a Refinery snaps to
 * the geyser it covers. Preview and placement share this point. */
static void
field_ui_site(int type, int *x, int *y)
{
    if (type == B_REFINERY) {
        int geyser = geyser_at(*x, *y);
        if (geyser >= 0) { *x = geysers[geyser].x; *y = geysers[geyser].y; }
        return;
    }
    *x = limit((*x + 4) & ~7, 8, WORLD_W - 8);
    *y = limit((*y + 4) & ~7, 8, WORLD_H - 8);
}

static int
ui_hostile_at(int x, int y, int *tx, int *ty)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == ENEMY && dist2(x, y, u[i].x, u[i].y) < 9 * 9 &&
            currently_visible(u[i].x, u[i].y)) {
            *tx = u[i].x; *ty = u[i].y;
            return 1;
        }
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == ENEMY && dist2(x, y, b[i].x, b[i].y) < 16 * 16 &&
            currently_visible(b[i].x, b[i].y)) {
            *tx = b[i].x; *ty = b[i].y;
            return 1;
        }
    return 0;
}

static int
field_ui_on_selected_building(void)
{
    return selected_building >= 0 &&
        dist2(cx, cy, b[selected_building].x, b[selected_building].y) < 26 * 26;
}

static int
field_ui_a_action(void)
{
    int tx, hx, hy, n;
    if (mode == MODE_MOVE) return UI_A_MOVE_HERE;
    if (mode == MODE_ATTACK) return UI_A_ATTACK_HERE;
    if (mode == MODE_BUILD) return UI_A_PLACE;
    n = ui_selected;
    tx = selection_target();
    /* Same order as interact(): double tap, gas, building, attack, select. */
    if (selected_building < 0 && unit_tap >= 0 && tx == unit_tap &&
        (u32)frame - unit_tap_frame <= UNIT_DOUBLE_TAP_TICKS &&
        dist2(cx, cy, unit_tap_x, unit_tap_y) <= UNIT_DOUBLE_TAP_RADIUS * UNIT_DOUBLE_TAP_RADIUS)
        return UI_A_GROUP;
    if (n && ui_worker >= 0) {
        int refinery = nearest_refinery(cx, cy);
        if (refinery >= 0 && dist2(cx, cy, b[refinery].x, b[refinery].y) < 24 * 24)
            return UI_A_GATHER_GAS;
    }
    if (selected_building >= 0 && b[selected_building].alive && field_ui_on_selected_building()) {
        int type = b[selected_building].type;
        if (type == B_COMMAND || type == B_BARRACKS) return UI_A_TRAIN;
        return type == B_REFINERY ? UI_A_ASSIGN_GAS : UI_A_NONE;
    }
    if (n && ui_hostile_at(cx, cy, &hx, &hy)) return UI_A_ATTACK;
    if (tx >= 0) return UI_A_SELECT;
    return n || selected_building >= 0 ? UI_A_DESELECT : UI_A_NONE;
}

static void
field_ui_refresh(void)
{
    int i;
    ui_selected = 0; ui_worker = -1; ui_built = 0;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].selected && !SC64_TRANSPORT_LOADED(&u[i])) {
            ui_selected++;
            if (ui_worker < 0 && u[i].owner == SIDE && IS_WORKER(u[i].type)) ui_worker = i;
        }
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == SIDE && !b[i].under_construction) ui_built |= 1u << b[i].type;
    ui_action = box_state == 2 ? UI_A_NONE : field_ui_a_action();
    if (mode == MODE_BUILD) {
        ui_site_x = cx; ui_site_y = cy;
        field_ui_site(menu, &ui_site_x, &ui_site_y);
        ui_site = ui_site_problem(menu, ui_site_x, ui_site_y);
        ui_site_blocked = ui_site == UI_SITE_BLOCKED || !can_place(menu, ui_site_x, ui_site_y);
    }
}

static void
field_ui_ping(int x, int y, u8 color)
{
    ping_x = (s16)x; ping_y = (s16)y; ping_color = color; ping_life = 18;
}

/* A on a visible enemy with units selected attacks it, as a right-click
 * would. Selection keeps priority everywhere else. */
static int
field_ui_smart_attack(void)
{
    int tx, ty;
    if (!selected_count() || !ui_hostile_at(cx, cy, &tx, &ty)) return 0;
    issue_order(ORDER_ATTACK_MOVE, tx, ty);
    field_ui_ping(tx, ty, C_RED);
    return 1;
}

/* R's army/type action fires on a clean release, so holding R to speed the
 * cursor no longer replaces the current selection. */
static int
field_ui_r_tap(u16 held, u16 pressed)
{
    if (pressed & KEY_R) r_tap_state = 1;
    if (!(held & KEY_R)) {
        int fire = r_tap_state == 1;
        r_tap_state = 0;
        return fire;
    }
    if ((held & (KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN)) || box_state) r_tap_state = 2;
    return 0;
}

static void
field_ui_title_input(u16 pressed)
{
    if (title_notice) title_notice--;
    if (pressed & (KEY_UP | KEY_DOWN)) title_cursor ^= 1;
    if ((pressed & KEY_A) && !title_cursor) {
        init_state();
        return;
    }
    if (pressed & (KEY_A | KEY_B | KEY_L)) {
        if (load_game()) title_cursor = 0;
        else { title_cursor = 1; title_notice = 90; }
    }
}

static void
field_ui_pause_input(u16 pressed)
{
    int choice = -1;
    if (pause_notice_ticks && !--pause_notice_ticks) pause_notice = 0;
    if (pressed & KEY_UP) pause_cursor = (u8)((pause_cursor + 3) % 4);
    if (pressed & KEY_DOWN) pause_cursor = (u8)((pause_cursor + 1) % 4);
    if (pressed & (KEY_A)) choice = pause_cursor;
    else if (pressed & (KEY_START | KEY_SELECT)) choice = 0;
    else if (pressed & KEY_B) choice = 1;
    else if (pressed & KEY_L) choice = 2;
    else if (pressed & KEY_R) choice = 3;
    if (choice < 0) return;
    pause_cursor = (u8)choice;
    if (choice == 0) screen = SCREEN_PLAY;
    else if (choice == 1) { save_game(); pause_notice = 1; pause_notice_ticks = 90; }
    else if (choice == 2) {
        if (!load_game()) { pause_notice = 2; pause_notice_ticks = 90; return; }
    } else init_state();
    if (screen != SCREEN_PAUSE) { pause_cursor = 0; pause_notice = pause_notice_ticks = 0; }
}

/* ---- Drawing ---------------------------------------------------------- */

static void
field_ui_draw_cursor(int x, int y)
{
    int action = ui_action;
    u8 color = mode == MODE_MOVE ? C_GREEN : mode == MODE_ATTACK || action == UI_A_ATTACK ? C_RED :
        mode == MODE_BUILD ? C_GOLD : C_HIGHLIGHT;
    if (ping_life) {
        int r = 3 + ping_life / 2, px = ping_x - camx, py = ping_y - camy + 12;
        ping_life--;
        gfx_diamond(px, py, r, r / 2 + 1, ping_color);
        if (ping_life > 9) gfx_diamond(px, py, r - 3, (r - 3) / 2 + 1, ping_color);
    }
    if (mode == MODE_BUILD && box_state != 2) {
        int sx = ui_site_x - camx, sy = ui_site_y - camy + 12, big = menu == B_COMMAND;
        /* Colour describes the ground: red whenever the site itself is
         * blocked, even if A would first fail for another reason; gold for a
         * clear site that still lacks tech, resources or an SCV; green when
         * A places it. The HUD text names start_build()'s first failure. */
        u8 site = ui_site_blocked ? C_RED : ui_site == UI_SITE_OK ? C_GREEN : C_GOLD;
        /* Ghost of the structure footprint plus a clearance outline. */
        gfx_frame(sx - (big ? 12 : 8), sy - (big ? 8 : 7), big ? 25 : 17, big ? 18 : 15, site);
        gfx_frame(sx - (big ? 13 : 9), sy - (big ? 9 : 8), big ? 27 : 19, big ? 20 : 17, site);
        gfx_rect(sx - 6, sy - 10, 13, 3, site);
        if (sx != x || sy != y) gfx_line(x, y, sx, sy, C_DIM);
    }
    if (mode == MODE_ATTACK) {
        gfx_frame(x - 5, y - 5, 11, 11, C_RED);
        gfx_rect(x - 9, y, 7, 1, C_RED); gfx_rect(x + 3, y, 7, 1, C_RED);
        gfx_rect(x, y - 9, 1, 7, C_RED); gfx_rect(x, y + 3, 1, 7, C_RED);
        gfx_pixel(x, y, C_WHITE);
        return;
    }
    if (mode == MODE_MOVE) {
        gfx_rect(x - 7, y, 15, 1, C_GREEN);
        gfx_rect(x, y - 7, 1, 15, C_GREEN);
        gfx_line(x - 6, y - 3, x - 3, y, C_GREEN); gfx_line(x - 6, y + 3, x - 3, y, C_GREEN);
        gfx_line(x + 6, y - 3, x + 3, y, C_GREEN); gfx_line(x + 6, y + 3, x + 3, y, C_GREEN);
        return;
    }
    if (action == UI_A_ATTACK) {
        gfx_frame(x - 6, y - 6, 13, 13, C_RED);
        gfx_rect(x - 2, y - 7, 5, 1, C_PANEL); gfx_rect(x - 2, y + 6, 5, 1, C_PANEL);
        gfx_rect(x - 7, y - 2, 1, 5, C_PANEL); gfx_rect(x + 6, y - 2, 1, 5, C_PANEL);
    } else if (action == UI_A_SELECT || action == UI_A_GROUP || action == UI_A_TRAIN ||
               action == UI_A_ASSIGN_GAS || action == UI_A_GATHER_GAS) {
        gfx_frame(x - 5, y - 5, 11, 11, color);
        gfx_rect(x - 1, y - 6, 3, 1, C_PANEL); gfx_rect(x - 1, y + 5, 3, 1, C_PANEL);
        gfx_rect(x - 6, y - 1, 1, 3, C_PANEL); gfx_rect(x + 5, y - 1, 1, 3, C_PANEL);
    } else if (mode == MODE_BUILD) gfx_frame(x - 4, y - 4, 9, 9, C_GOLD);
    gfx_rect(x - 7, y, 15, 1, color);
    gfx_rect(x, y - 7, 1, 15, color);
}

static int
ui_command_state(int command, int *cost)
{
    int type = ui_command_building[command];
    *cost = 0;
    if (command <= MENU_ATTACK || command == MENU_STOP)
        return ui_selected ? UI_SITE_OK : UI_SITE_WORKER;
    if (command == MENU_WEAPONS || command == MENU_ARMOR) {
        int level = command == MENU_WEAPONS ? weapons_level : armor_level;
        int busy = command == MENU_WEAPONS ? weapons_research : armor_research;
        *cost = 100;
        if (!(ui_built >> B_ENG_BAY & 1) || level >= 2 || busy) return UI_SITE_TECH;
        return minerals < 100 || gas < 100 ? UI_SITE_MINERALS : UI_SITE_OK;
    }
    /* start_build() order, without the site checks made at placement. */
    *cost = building_cost(type);
    if (!ui_has_tech(type)) return UI_SITE_TECH;
    if (minerals < *cost) return UI_SITE_MINERALS;
    if (gas < building_gas_cost(type)) return UI_SITE_GAS;
    return ui_worker < 0 ? UI_SITE_WORKER : UI_SITE_OK;
}

static void
field_ui_draw_command_menu(void)
{
    int i, x = 118, y = 14, w = 120;
    ui_panel(x, y, w, 96, C_GOLD, 0);
    for (i = 0; i < MENU_COUNT; i++) {
        int row = y + 4 + i * 9, cost, state = ui_command_state(i, &cost);
        int short_funds = state == UI_SITE_MINERALS || state == UI_SITE_GAS;
        u8 label = state == UI_SITE_OK || short_funds ? C_SILVER : C_DIM;
        if (i == menu_cursor) {
            ui_highlight(x + 2, row - 1, w - 4);
            if (label == C_SILVER) label = C_WHITE;
        }
        ui_command_icon(i, x + 9, row + 3);
        gfx_text(x + 17, row, ui_command_label[i], label);
        if (cost && label != C_DIM) /* the footer explains disabled rows */
            ui_num_right(x + w - 5, row, cost, short_funds ? C_RED : C_GOLD);
    }
}

static void
ui_portrait(int kind, int type)
{
    gfx_rect(3, 124, 22, 22, C_FOG);
    gfx_frame(3, 124, 22, 22, C_EDGE);
    if (kind == 0) ui_unit_glyph(type, 14, 135, C_TERRAN);
    else if (kind == 1) ui_building_glyph(type, 14, 136, C_TERRAN);
    else ui_command_icon(type, 14, 135);
}

static void
ui_hint_line(int a_action)
{
    static const char *const a_text[] = {
        0, "SELECT", "DESELECT", "TRAIN", "ADD GAS WORKER", "GATHER GAS",
        "ATTACK", "MOVE HERE", "ATTACK HERE", "PLACE", "ALL OF TYPE"
    };
    int x = 3;
    if (a_action == UI_A_TRAIN) {
        x = ui_key(x, 150, "A", b[selected_building].type == B_COMMAND ? "TRAIN SCV" : "TRAIN", C_WHITE);
        if (b[selected_building].type == B_BARRACKS) x = ui_key(x, 150, "R", "TYPE", C_DIM);
        ui_key(x, 150, "L", "UNDO", C_DIM);
        return;
    }
    if (a_action) x = ui_key(x, 150, "A", a_text[a_action], a_action == UI_A_ATTACK ||
                             a_action == UI_A_ATTACK_HERE ? C_RED : C_WHITE);
    if (mode != MODE_NORMAL) { ui_key(x, 150, "B", "BACK", C_DIM); return; }
#ifdef SC_COMMAND_RING
    x = ui_key(x, 150, "B", "HOLD: RING", C_DIM);
#else
    x = ui_key(x, 150, "B", "ORDERS", C_DIM);
#endif
    if (x < 130) ui_key(x, 150, "R", "ARMY", C_DIM);
}

static void
ui_status(int x, int y, int problem, int type)
{
    static const char *const text[] = {
        "READY", "SELECT AN SCV", "NEEDS ", "NEED MINERALS", "SITE BLOCKED", "NO FREE SLOT",
        "NEED GAS"
    };
    if (problem == UI_SITE_TECH) {
        gfx_text(x, y, "NEEDS", C_RED);
        gfx_text(x + 36, y, type == B_BARRACKS ? "SUPPLY DEPOT" : type == B_ACADEMY ?
                 "BARRACKS" : "COMMAND CENTER", C_RED);
        return;
    }
    if (problem == UI_SITE_MINERALS) {
        gfx_text(x, y, "NEED", C_RED);
        gfx_num(x + 30, y, building_cost(type) - minerals, C_RED);
        gfx_text(x + 36 + ui_num_width(building_cost(type) - minerals), y, "MORE MINERALS", C_RED);
        return;
    }
    gfx_text(x, y, text[problem], problem ? C_RED : C_GREEN);
}

/* Bottom command band for the default profile. */
static void
field_ui_draw_hud(void)
{
    int i, n = ui_selected, action = ui_action;
    /* draw_hud_commands() has already filled the band. */
    gfx_rect(0, 147, 179, 1, C_SHADOW);
    if (box_state == 2) {
        ui_portrait(2, MENU_MOVE);
        gfx_text(30, 126, "BOX SELECT", C_CYAN);
        gfx_text(30, 136, "DRAG TO COVER UNITS", C_DIM);
        ui_key(ui_key(3, 150, "L", "RELEASE: SELECT", C_WHITE), 150, "B", "CANCEL", C_DIM);
        return;
    }
    if (menu_open) {
        int cost, state = ui_command_state(menu_cursor, &cost);
        int type = ui_command_building[menu_cursor];
        ui_portrait(2, menu_cursor);
        gfx_text(30, 125, ui_command_label[menu_cursor], C_WHITE);
        if (cost) {
            int x = 30 + ui_text_width(ui_command_label[menu_cursor]) + 7;
            gfx_num(x, 125, cost, C_GOLD);
            gfx_text(x + ui_num_width(cost) + 3, 125, "MIN", C_GOLD);
            if (menu_cursor == MENU_WEAPONS || menu_cursor == MENU_ARMOR)
                gfx_text(x + ui_num_width(cost) + 24, 125, "+100 GAS", C_CYAN);
        }
        if (state == UI_SITE_OK)
            gfx_text(30, 136, ui_command_note[menu_cursor], C_DIM);
        else if (state == UI_SITE_WORKER)
            gfx_text(30, 136, type ? "SELECT AN SCV FIRST" : "SELECT UNITS FIRST", C_RED);
        else if (type) ui_status(30, 136, state, type);
        else if (state == UI_SITE_MINERALS)
            gfx_text(30, 136, gas < 100 ? "NEED 100 GAS" : "NEED 100 MINERALS", C_RED);
        else if (!(ui_built >> B_ENG_BAY & 1)) gfx_text(30, 136, "NEEDS ENGINEERING BAY", C_RED);
        else gfx_text(30, 136, (menu_cursor == MENU_WEAPONS ? weapons_research : armor_research) ?
                      "RESEARCHING" : "FULLY UPGRADED", C_GOLD);
        ui_key(ui_key(ui_key(3, 150, "A", "CHOOSE", C_WHITE), 150, "B", "CLOSE", C_DIM),
               150, "+", "SCROLL", C_DIM);
        return;
    }
    if (mode == MODE_BUILD) {
        ui_portrait(1, menu);
        gfx_text(30, 125, ui_building_name(menu), C_WHITE);
        gfx_num(130, 125, building_cost(menu), C_GOLD);
        gfx_text(130 + ui_num_width(building_cost(menu)) + 3, 125, "MIN", C_GOLD);
        ui_status(30, 136, ui_site, menu);
    } else if (mode == MODE_MOVE || mode == MODE_ATTACK) {
        ui_portrait(2, mode == MODE_MOVE ? MENU_MOVE : MENU_ATTACK);
        gfx_text(30, 125, mode == MODE_MOVE ? "MOVE ORDER" : "ATTACK MOVE",
                 mode == MODE_MOVE ? C_GREEN : C_RED);
        gfx_num(30, 136, n, C_WHITE);
        i = 36 + ui_num_width(n);
        gfx_text(i, 136, n == 1 ? "UNIT" : "UNITS", C_DIM);
        gfx_text(i + (n == 1 ? 30 : 36), 136, mode == MODE_MOVE ? "TO THE CURSOR" : "FIGHT EN ROUTE",
                 C_DIM);
    } else if (selected_building >= 0 && b[selected_building].alive) {
        Building *q = &b[selected_building];
        ui_portrait(1, q->type);
        gfx_text(30, 125, ui_building_name(q->type), C_WHITE);
        gfx_bar(30, 136, 40, 3, q->hp, q->maxhp, C_GREEN, C_RED_DARK);
        if (q->under_construction) {
            gfx_text(76, 134, "BUILDING", C_GOLD);
            gfx_bar(126, 136, 40, 3, q->build_progress, building_build_time(q->type), C_GOLD, C_GOLD_DARK);
        } else if (q->type == B_COMMAND || q->type == B_BARRACKS) {
            int next = q->type == B_COMMAND ? U_SCV : train_type;
            const char *name = q->type == B_COMMAND ? "SCV" : unit_name(next);
            /* Next unit and its price, then five queue slots; the first fills
             * as it trains. */
            gfx_text(118, 125, name, C_CYAN);
            ui_num_right(178, 125, unit_cost(next), minerals < unit_cost(next) ? C_RED : C_GOLD);
            if (unit_gas_cost(next))
                ui_num_right(178, 136, unit_gas_cost(next), gas < unit_gas_cost(next) ? C_RED : C_CYAN);
            for (i = 0; i < 5; i++) {
                int sx = 76 + i * 11;
                gfx_rect(sx, 132, 10, 10, i < q->queue_count ? C_FOG : C_SHADOW);
                if (i < q->queue_count) ui_unit_glyph(q->queue[(q->queue_head + i) % 5], sx + 5, 137, C_TERRAN);
            }
            if (q->queue_count)
                gfx_bar(76, 143, 10, 2, q->queue_progress, unit_build_time(q->queue[q->queue_head]),
                        C_GOLD, C_SHADOW);
        } else if (q->type == B_REFINERY) gfx_text(30, 139, "GAS FOR UPGRADES", C_CYAN);
        else if (q->type == B_ENG_BAY) {
            gfx_text(76, 134, "WEAPONS", C_DIM); gfx_num(122, 134, weapons_level, C_WHITE);
            gfx_text(134, 134, "ARMOR", C_DIM); gfx_num(168, 134, armor_level, C_WHITE);
        }
    } else if (n) {
        int counts[4] = {0, 0, 0, 0}, first = -1, hp = 0, maxhp = 0;
        for (i = 0; i < MAX_UNITS; i++)
            if (u[i].alive && u[i].selected) {
                if (first < 0) first = i;
                if (u[i].type <= U_MEDIC) counts[u[i].type]++;
                hp += u[i].hp; maxhp += u[i].maxhp;
            }
        ui_portrait(0, u[first].type);
        if (n == 1) {
            gfx_text(30, 125, unit_name(u[first].type), C_WHITE);
            gfx_bar(30, 135, 50, 3, hp, maxhp, C_GREEN, C_RED_DARK);
            gfx_num(86, 133, hp, C_GREEN);
            if (IS_WORKER(u[first].type))
                gfx_text(110, 133, u[first].order == ORDER_GAS ? "GAS" : u[first].order == ORDER_BUILD ?
                         "BUILDING" : u[first].order == ORDER_MINE ? "MINING" : "IDLE", C_GOLD);
        } else {
            int x = 30;
            gfx_text(30, 125, "GROUP", C_WHITE);
            gfx_num(66, 125, n, C_GOLD);
            gfx_bar(86, 127, 40, 3, hp, maxhp, C_GREEN, C_RED_DARK);
            for (i = 0; i < 4; i++) if (counts[i]) {
                ui_unit_glyph(i, x + 4, 139, C_TERRAN);
                gfx_num(x + 11, 136, counts[i], C_WHITE);
                x += 17 + ui_num_width(counts[i]);
            }
        }
    } else {
        int seconds = frame / 30;
        char clock[6] = {(char)('0' + seconds / 600 % 10), (char)('0' + seconds / 60 % 10), ':',
                         (char)('0' + seconds / 10 % 6), (char)('0' + seconds % 10), 0};
        ui_portrait(1, B_HATCHERY);
        gfx_text(30, 125, "DESTROY THE HATCHERY", C_GOLD);
        gfx_text(30, 136, clock, C_WHITE);
    }
    ui_hint_line(action);
}

static void
ui_title_scene(void)
{
    int i;
    gfx_rect(26, 62, 188, 34, C_DIRT);
    gfx_rect(26, 62, 188, 1, C_BROWN);
    for (i = 0; i < 8; i++) gfx_pixel(36 + i * 23, 70 + (i * 7) % 20, C_SAND);
    /* Terran outpost on the left, the Hatchery on the right. */
    gfx_rect(42, 70, 25, 18, C_STONE);
    gfx_rect(45, 67, 18, 5, C_TERRAN);
    gfx_frame(42, 70, 25, 18, C_SILVER);
    gfx_rect(50, 65, 8, 3, C_GOLD);
    for (i = 0; i < 4; i++) ui_unit_glyph(U_MARINE, 82 + i * 9, 80 + (i & 1) * 4, C_TERRAN);
    ui_unit_glyph(U_FIREBAT, 120, 78, C_TERRAN);
    for (i = 0; i < 3; i++) ui_unit_glyph(U_ZERGLING, 146 + i * 10, 82 - (i & 1) * 6, C_ZERG);
    gfx_diamond(190, 79, 16, 9, C_ZERG);
    gfx_diamond(190, 78, 10, 6, C_PURPLE);
    gfx_rect(113, 80, 28, 1, C_GOLD);
    gfx_pixel(141, 80, C_WHITE);
}

static void
field_ui_draw_title(void)
{
    int i;
    for (i = 0; i < 40; i++)
        gfx_pixel((i * 97 + 13) % 240, (i * 53 + 7) % 160, i % 5 ? C_DIM : C_WHITE);
    ui_panel(16, 10, 208, 140, C_CYAN, 0);
    gfx_text(85, 19, "STARCRAFT", C_GOLD_DARK);
    gfx_text(84, 18, "STARCRAFT", C_GOLD);
    gfx_rect(40, 29, 160, 1, C_EDGE);
    gfx_text(78, 33, "TERRAN OUTPOST", C_WHITE);
    gfx_text(45, 47, "DESTROY THE ZERG HATCHERY", C_DIM);
    ui_title_scene();
    for (i = 0; i < 2; i++) {
        int y = 103 + i * 11;
        if (i == title_cursor) ui_highlight(60, y - 1, 120);
        if (i) { /* a save cartridge */
            gfx_rect(66, y, 9, 7, C_SILVER);
            gfx_rect(68, y + 1, 5, 3, C_BLUE_DARK);
        } else ui_command_icon(MENU_MOVE, 70, y + 3);
        gfx_text(80, y, i ? "LOAD SAVED GAME" : "NEW MISSION", i == title_cursor ? C_WHITE : C_SILVER);
    }
    if (title_notice) gfx_text(84, 127, "NO SAVE FOUND", C_RED);
    else gfx_text(60, 127, "ORIGINAL-ART GBA RTS", C_DIM);
    ui_key(ui_key(ui_key(40, 139, "A", "SELECT", C_WHITE), 139, "B", "LOAD", C_DIM), 139, "+", "CHOOSE", C_DIM);
}

static void
field_ui_draw_pause(void)
{
    static const char *const items[4] = { "RESUME", "SAVE GAME", "LOAD SAVE", "RESTART" };
    static const char *const keys[4] = { "A", "B", "L", "R" };
    int i, seconds = frame / 30;
    char clock[6] = {(char)('0' + seconds / 600 % 10), (char)('0' + seconds / 60 % 10), ':',
                     (char)('0' + seconds / 10 % 6), (char)('0' + seconds % 10), 0};
    ui_panel(24, 14, 192, 132, C_CYAN, "PAUSED");
    gfx_text(178, 18, clock, C_WHITE);
    for (i = 0; i < 4; i++) {
        int y = 33 + i * 11;
        if (i == pause_cursor) ui_highlight(30, y - 1, 92);
        gfx_text(36, y, items[i], i == pause_cursor ? C_WHITE : C_SILVER);
        ui_key(110, y, keys[i], 0, C_DIM);
    }
    gfx_text(130, 33, "OBJECTIVE", C_GOLD);
    gfx_text(130, 43, "DESTROY THE", C_DIM);
    gfx_text(130, 52, "HATCHERY", C_DIM);
    if (pause_notice) gfx_text(130, 66, pause_notice == 1 ? "GAME SAVED" : "NO SAVE FOUND",
                               pause_notice == 1 ? C_GREEN : C_RED);
    gfx_rect(30, 80, 180, 1, C_EDGE);
    gfx_text(30, 84, "CONTROLS", C_GOLD);
    ui_key(30, 96, "A", "SELECT, CONFIRM, ATTACK", C_SILVER);
    ui_key(30, 107, "B", "ORDER LIST / BACK", C_SILVER);
    ui_key(30, 118, "L", "NEXT UNIT  HOLD: BOX SELECT", C_SILVER);
    ui_key(30, 129, "R", "ARMY  HOLD: FAST CURSOR", C_SILVER);
}

static void
field_ui_draw_result(void)
{
    int i, units = 0, buildings = 0, seconds = frame / 30;
    char clock[6] = {(char)('0' + seconds / 600 % 10), (char)('0' + seconds / 60 % 10), ':',
                     (char)('0' + seconds / 10 % 6), (char)('0' + seconds % 10), 0};
    u8 accent = won ? C_GOLD : C_RED;
    for (i = 0; i < MAX_UNITS; i++) units += u[i].alive && u[i].owner == SIDE;
    for (i = 0; i < MAX_BUILDINGS; i++) buildings += b[i].alive && b[i].owner == SIDE;
    ui_panel(28, 30, 184, 86, accent, 0);
    gfx_rect(30, 32, 180, 13, accent == C_GOLD ? C_GOLD_DARK : C_RED_DARK);
    gfx_text(won ? 81 : 84, 35, won ? "MISSION CLEAR" : "OUTPOST LOST", accent == C_GOLD ? C_HIGHLIGHT : C_WHITE);
    gfx_text(won ? 60 : 48, 51, won ? "ZERG BASE ELIMINATED" : "COMMAND CENTER DESTROYED", C_WHITE);
    gfx_rect(40, 62, 160, 1, C_EDGE);
    gfx_text(44, 68, "TIME", C_DIM); gfx_text(44, 78, clock, C_WHITE);
    gfx_text(98, 68, "UNITS", C_DIM); gfx_num(98, 78, units, C_WHITE);
    gfx_text(146, 68, "BUILDINGS", C_DIM); gfx_num(146, 78, buildings, C_WHITE);
    ui_key(ui_key(58, 99, "A", "RETRY", C_WHITE), 99, "B", "TITLE SCREEN", C_DIM);
}
