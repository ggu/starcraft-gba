/* Panels, key caps, list bars and order icons shared by the default menus
 * (field_ui.h) and their race-engine port (field_controls.h). Drawing only. */

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

/* Icons of the order rows every race shares; 0 when command is not one. */
static int
ui_order_icon(int command, int x, int y)
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
    } else return 0;
    return 1;
}

/* mm:ss of a 30 Hz tick count, for the pause and result panels. */
static void
ui_clock(int x, int y, unsigned ticks, u8 color)
{
    unsigned seconds = ticks / 30;
    char clock[6] = {(char)('0' + seconds / 600 % 10), (char)('0' + seconds / 60 % 10), ':',
                     (char)('0' + seconds / 10 % 6), (char)('0' + seconds % 10), 0};
    gfx_text(x, y, clock, color);
}
