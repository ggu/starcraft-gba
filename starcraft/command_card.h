/* StarCraft's command card and resource bar for the race HUD
 * (SC64_COMMAND_ICONS).
 *
 * The order list becomes a grid of the ROM's own command buttons, three to a
 * row, with Move, Stop and Attack on top as in StarCraft, then the list's
 * other rows in their usual order. Icons are tinted green when usable, gold
 * when highlighted and grey when unavailable; a red pip marks a command that
 * is only short of minerals or gas. The D-pad moves on the grid and the HUD
 * names the highlighted button. The icons come from a private header that
 * tools/sc64_command_icons.py writes from a verified ROM; menu_cursor keeps
 * the list's row numbers, so every command path is unchanged. Lists longer
 * than the card keep the framed text list. The resource bar's icons are in
 * hud_icons.h. */
#if !defined(SC64_COMMAND_ICONS) || !defined(SC64_HUD_ICONS)
#error "command_card.h needs the imported command icons"
#endif

#include "sc64_command_icons_generated.h"

static const char *unit_name(int type);
static int sc64_actor_max_shields(int type, int building);

enum { CARD_COLUMNS = 3, CARD_ROWS = 5, CARD_CELL = 20, CARD_PITCH = 21 };

static int
card_fits(void)
{
    return field_menu_count() <= CARD_COLUMNS * CARD_ROWS;
}

/* Grid slot to list row: Move, Stop, Attack, then the rest in list order. */
static int
card_slot_command(int slot)
{
    if (slot == 1) return MENU_STOP;
    if (slot == 2) return MENU_ATTACK;
    if (slot >= 3 && slot <= MENU_STOP) return slot - 1;
    return slot;
}

static int
card_command_slot(int i)
{
    if (i == MENU_STOP) return 1;
    if (i == MENU_ATTACK) return 2;
    if (i >= MENU_BARRACKS && i < MENU_STOP) return i + 1;
    return i;
}

/* The portrait of a raw unit id, or -1 when the ROM has no button-sized
 * frame for it. */
static int
card_portrait(int raw)
{
    return raw >= 0 && raw < SC64_CMD_ICON_PORTRAITS && sc64_command_icon_size[raw][0] > 1 ? raw : -1;
}

static int
card_building_icon(int type)
{
    return card_portrait(sc64_race_raw_building(type));
}

static int
card_unit_icon(const Unit *q)
{
    return card_portrait(q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type));
}

/* The button for list row i, or -1 for the drawn fallback glyph. */
static int
card_icon(int i)
{
    static const u16 weapons[3] = {SC64_CMD_ICON_TERRAN_WEAPONS, SC64_CMD_ICON_ZERG_WEAPONS,
                                  SC64_CMD_ICON_PROTOSS_WEAPONS};
    static const u16 armor[3] = {SC64_CMD_ICON_TERRAN_ARMOR, SC64_CMD_ICON_ZERG_ARMOR,
                                SC64_CMD_ICON_PROTOSS_ARMOR};
    static const struct { const char *label; u16 icon; } specials[] = {
        {"STORM", SC64_CMD_ICON_STORM}, {"CLOAK", SC64_CMD_ICON_CLOAK},
        {"UNCLOAK", SC64_CMD_ICON_CLOAK}, {"CONSUME", SC64_CMD_ICON_CONSUME},
        {"UNLOAD CARGO", SC64_CMD_ICON_UNLOAD}, {"VENTRAL SACS", SC64_CMD_ICON_VENTRAL_SACS},
        {"LIFT / LAND", SC64_CMD_ICON_LIFT}
    };
    const char *label;
    int type;
    unsigned k;
    if (i == MENU_MOVE) return SC64_CMD_ICON_MOVE;
    if (i == MENU_ATTACK) return SC64_CMD_ICON_ATTACK;
    if (i == MENU_STOP) return SC64_CMD_ICON_STOP;
    if (i == MENU_WEAPONS) return weapons[race_human];
    if (i == MENU_ARMOR) return armor[race_human];
    ui_row_kind(i, &type);
    if (type >= 0) return card_building_icon(type);
    label = race_menu_label(i);
    for (k = 0; k < sizeof specials / sizeof specials[0]; k++)
        if (ui_same(label, specials[k].label)) return specials[k].icon;
    return -1;
}

/* An icon centred in the cell at x, y, one rectangle per run of a shade. */
static void
card_icon_draw(int icon, int x, int y, const u8 *ramp)
{
    int w = sc64_command_icon_size[icon][0], h = sc64_command_icon_size[icon][1];
    int stride = (w + 3) / 4, row, col;
    const unsigned char *bits = sc64_command_icon_bits + sc64_command_icon_offset[icon];
    x += (CARD_CELL - w) / 2;
    y += (CARD_CELL - h) / 2;
    for (row = 0; row < h; row++, bits += stride) {
        for (col = 0; col < w;) {
            int s = bits[col >> 2] >> ((col & 3) * 2) & 3, start = col;
            while (++col < w && (bits[col >> 2] >> ((col & 3) * 2) & 3) == s) {}
            if (s) gfx_rect(x + start, y + row, col - start, 1, ramp[s - 1]);
        }
    }
}

static const u8 card_green[3] = {C_GREEN_DARK, C_GREEN, C_HIGHLIGHT};
static const u8 card_gold[3] = {C_GOLD_DARK, C_GOLD, C_HIGHLIGHT};
static const u8 card_grey[3] = {C_EDGE, C_DIM, C_SILVER};

/* The highlighted command's button in the HUD, left of its name. */
static void
card_hud_icon(int icon)
{
    if (icon >= 0) card_icon_draw(icon, 3, 124, card_green);
}

static void
field_controls_draw_command_card(void)
{
    int count = field_menu_count(), rows = (count + CARD_COLUMNS - 1) / CARD_COLUMNS;
    int w = CARD_COLUMNS * CARD_PITCH + 3, h = rows * CARD_PITCH + 3;
    int x = 238 - w, y = 119 - h, slot;
    /* The card draws after the HUD bands; a full card stays below the
     * resource bar. */
    if (y < 12) y = 12;
    ui_panel(x, y, w, h, C_GOLD, 0);
    for (slot = 0; slot < count; slot++) {
        int i = card_slot_command(slot), cost, gas_cost, icon = card_icon(i);
        int cx = x + 2 + slot % CARD_COLUMNS * CARD_PITCH, cy = y + 2 + slot / CARD_COLUMNS * CARD_PITCH;
        const char *need;
        int state = ui_row_state(i, &cost, &gas_cost, &need);
        int short_funds = state == UI_SITE_MINERALS || state == UI_SITE_GAS;
        const u8 *ramp = i == menu_cursor ? card_gold : state == UI_SITE_OK || short_funds ?
            card_green : card_grey;
        gfx_rect(cx, cy, CARD_CELL, CARD_CELL, i == menu_cursor ? C_UI : C_SHADOW);
        if (icon >= 0) card_icon_draw(icon, cx, cy, ramp);
        else ui_row_icon(i, cx + CARD_CELL / 2, cy + CARD_CELL / 2);
        if (short_funds) gfx_rect(cx + CARD_CELL - 4, cy + CARD_CELL - 4, 3, 3, C_RED);
        if (i == menu_cursor) gfx_frame(cx - 1, cy - 1, CARD_CELL + 2, CARD_CELL + 2, C_GOLD);
    }
}

/* D-pad on the card: left and right wrap within a row, up and down within
 * a column. Returns 0 when the list is too long for the card. */
static int
field_card_navigate(u16 pressed)
{
    int count = field_menu_count(), rows = (count + CARD_COLUMNS - 1) / CARD_COLUMNS;
    int slot, row, col, length;
    if (!card_fits()) return 0;
    slot = card_command_slot(menu_cursor);
    row = slot / CARD_COLUMNS;
    col = slot % CARD_COLUMNS;
    length = count - row * CARD_COLUMNS < CARD_COLUMNS ? count - row * CARD_COLUMNS : CARD_COLUMNS;
    if (pressed & KEY_LEFT) col = (col + length - 1) % length;
    if (pressed & KEY_RIGHT) col = (col + 1) % length;
    if (pressed & KEY_UP)
        do row = (row + rows - 1) % rows; while (row * CARD_COLUMNS + col >= count);
    if (pressed & KEY_DOWN) {
        row = (row + 1) % rows;
        if (row * CARD_COLUMNS + col >= count) row = 0;
    }
    menu_cursor = card_slot_command(row * CARD_COLUMNS + col);
    return 1;
}

/* The selected unit's portrait, then its name, or the group's size, over
 * its shield and health bars; the hint keeps its place at x 52. The name
 * uses the source font, ending by x 165 like the other HUD titles. */
static void
card_selection_header(int i, int n)
{
    int icon = card_unit_icon(&u[i]), shields = sc64_actor_max_shields(u[i].type, 0);
    if (icon >= 0) card_icon_draw(icon, 3, 124, card_green);
    if (n > 1) {
        gfx_text(27, 126, "X", C_DIM);
        gfx_num(34, 126, n, C_WHITE);
    } else {
        char title[48];
        sc64_font_ellipsis(unit_name(u[i].type), 138, title, sizeof title);
        sc64_font_text(27, 124, 138, title, C_WHITE);
    }
    if (shields > 0) gfx_bar(27, 137, 22, 2, u[i].shield, shields, C_CYAN, C_SHADOW);
    gfx_bar(27, 140, 22, 4, u[i].hp, u[i].maxhp, C_GREEN, C_RED_DARK);
}
