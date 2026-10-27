/* Default-profile controls for the race engine (SC64_FIELD_CONTROLS).
 *
 * The control half of field_ui.h, for builds that keep the StarCraft 64 menus
 * and HUD: A on empty ground deselects, A trains only on the selected
 * structure, A on a visible hostile target attacks, R acts on a clean tap,
 * the order list reopens on the last command, and armed orders and targets
 * get the same cursors and ground markers. The functions share field_ui.h's names so
 * game.c's hooks serve both. Presentation and input routing only: no saved
 * state, and every action goes through the existing order and build paths. */
#ifndef SC64_FIELD_CONTROLS
#error "field_controls.h is the race-engine port of the default controls"
#endif
#ifdef SC_DEFAULT_UI
#error "the default profile uses field_ui.h"
#endif

static int currently_visible(int x, int y);
static int nearest_refinery(int x, int y);

/* ui_action (game.c, beside the other field state) holds one of these. */
enum {
    UI_A_NONE, UI_A_SELECT, UI_A_DESELECT, UI_A_TRAIN, UI_A_ATTACK,
    UI_A_MOVE_HERE, UI_A_ATTACK_HERE, UI_A_PLACE, UI_A_GAS
};

/* Visible units and structures the player's side may fight: the engine's
 * owners_hostile(), so skirmish opponents 1, 3 and 4 all count. */
static int
ui_hostile_at(int x, int y, int *tx, int *ty)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && owners_hostile(SIDE, u[i].owner) &&
            dist2(x, y, u[i].x, u[i].y) < 9 * 9 && currently_visible(u[i].x, u[i].y)) {
            *tx = u[i].x; *ty = u[i].y;
            return 1;
        }
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && owners_hostile(SIDE, b[i].owner) && dist2(x, y, b[i].x, b[i].y) < 16 * 16 &&
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

/* What A does at the cursor, in interact()'s order (double tap, gas,
 * structure, attack, select); drives the hint row and attack cursor. */
static int
field_ui_a_action(int selected)
{
    int hx, hy, target;
    if (mode == MODE_MOVE) return UI_A_MOVE_HERE;
    if (mode == MODE_ATTACK) return UI_A_ATTACK_HERE;
    if (mode == MODE_BUILD) return UI_A_PLACE;
    if (mode != MODE_NORMAL) return UI_A_NONE;
    target = selection_target();
    if (selected_building < 0 && unit_tap >= 0 && target == unit_tap &&
        (u32)frame - unit_tap_frame <= UNIT_DOUBLE_TAP_TICKS &&
        dist2(cx, cy, unit_tap_x, unit_tap_y) <= UNIT_DOUBLE_TAP_RADIUS * UNIT_DOUBLE_TAP_RADIUS)
        return UI_A_SELECT;
    if (find_selected_worker() >= 0) {
        int refinery = nearest_refinery(cx, cy);
        if (refinery >= 0 && dist2(cx, cy, b[refinery].x, b[refinery].y) < 24 * 24)
            return UI_A_GAS;
    }
    if (selected_building >= 0 && b[selected_building].alive && field_ui_on_selected_building())
        return b[selected_building].under_construction ? UI_A_NONE : UI_A_TRAIN;
    if (selected && ui_hostile_at(cx, cy, &hx, &hy)) return UI_A_ATTACK;
    if (target >= 0) return UI_A_SELECT;
    return selected || selected_building >= 0 ? UI_A_DESELECT : UI_A_NONE;
}

/* The renderer draws several frames per simulation update in crowded scenes,
 * and resolving every drawn frame cost about 4% of Carrier Approach's update
 * rate with the army selected. The cursor and selection only change in
 * updates that advance frame; arming an order from the list changes mode
 * without advancing it. So resolve once per (frame, mode). A restart or a
 * load jumps frame, so init_state() and load_game() clear the key
 * (ui_tick_mode is mode + 1; 0 forces a resolve). */

static void
field_ui_refresh(void)
{
    if ((u8)frame == ui_tick && (u8)(mode + 1) == ui_tick_mode) return;
    ui_tick = (u8)frame; ui_tick_mode = (u8)(mode + 1);
    ui_action = (u8)(box_state == 2 ? UI_A_NONE : field_ui_a_action(selected_count()));
}

static void
field_ui_ping(int x, int y, u8 color)
{
    ping_x = (s16)x; ping_y = (s16)y; ping_color = color; ping_life = 18;
}

/* A on a visible enemy with units selected attacks it; selection keeps
 * priority everywhere else. */
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
 * cursor keeps the current selection. */
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

/* Ground marker plus the armed-order cursors, drawn over the race cursor. */
static void
field_controls_draw_cursor(int x, int y)
{
    if (ping_life) {
        int r = 3 + ping_life / 2, px = ping_x - camx, py = ping_y - camy + 12;
        ping_life--;
        gfx_diamond(px, py, r, r / 2 + 1, ping_color);
        if (ping_life > 9) gfx_diamond(px, py, r - 3, (r - 3) / 2 + 1, ping_color);
    }
    if (mode == MODE_ATTACK) {
        gfx_frame(x - 5, y - 5, 11, 11, C_RED);
        gfx_rect(x - 9, y, 7, 1, C_RED); gfx_rect(x + 3, y, 7, 1, C_RED);
        gfx_rect(x, y - 9, 1, 7, C_RED); gfx_rect(x, y + 3, 1, 7, C_RED);
        gfx_pixel(x, y, C_WHITE);
    } else if (mode == MODE_MOVE) {
        gfx_rect(x - 7, y, 15, 1, C_GREEN);
        gfx_rect(x, y - 7, 1, 15, C_GREEN);
        gfx_line(x - 6, y - 3, x - 3, y, C_GREEN); gfx_line(x - 6, y + 3, x - 3, y, C_GREEN);
        gfx_line(x + 6, y - 3, x + 3, y, C_GREEN); gfx_line(x + 6, y + 3, x + 3, y, C_GREEN);
    } else if (!menu_open && ui_action == UI_A_ATTACK) {
        gfx_frame(x - 6, y - 6, 13, 13, C_RED);
    }
}

/* Colour of the HUD title for the current mode (armed orders stand out). */
static u8
field_controls_title_color(void)
{
    return mode == MODE_MOVE ? C_GREEN : mode == MODE_ATTACK ? C_RED : C_WHITE;
}

/* Hint row text for MODE_NORMAL/MOVE/ATTACK; 0 keeps the profile's own row.
 * While the order list is open its A and B choose and close instead. */
static const char *
field_controls_hint(void)
{
    if (menu_open) return "A CHOOSE  B CLOSE";
    switch (ui_action) {
    case UI_A_MOVE_HERE: return "A MOVE HERE  B BACK";
    case UI_A_ATTACK_HERE: return "A ATTACK HERE  B BACK";
    case UI_A_ATTACK: return "B COMMANDS  A ATTACK";
    case UI_A_SELECT: return "B COMMANDS  A SELECT";
    case UI_A_DESELECT: return "B COMMANDS  A DESELECT";
    case UI_A_GAS: return "B COMMANDS  A GATHER GAS";
    default: return 0;
    }
}
