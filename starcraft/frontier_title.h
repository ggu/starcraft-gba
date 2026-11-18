/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FRONTIER_TITLE_H
#define FRONTIER_TITLE_H

static void
frontier_title_input(u16 pressed)
{
    int format_row = FRONTIER_SETUP_CPU_FIRST + skirmish_bots;
    if (title_notice) title_notice--;
    if (pressed & KEY_A) init_state();
    else if (pressed & (KEY_B | KEY_L)) {
        if (!load_game()) title_notice = 90;
    } else if (pressed & (KEY_UP | KEY_DOWN)) {
        int rows = format_row + 1;
        int direction = (pressed & KEY_DOWN) ? 1 : rows - 1;
        frontier_setup_row = (frontier_setup_row + direction) % rows;
    } else if (pressed & (KEY_LEFT | KEY_RIGHT)) {
        int forward = pressed & KEY_RIGHT;
        if (frontier_setup_row == FRONTIER_SETUP_MAP) {
            int direction = forward ? 1 : ORIGINAL_MAP_COUNT - 1;
            original_skirmish_map = (original_skirmish_map + direction) % ORIGINAL_MAP_COUNT;
        } else if (frontier_setup_row == FRONTIER_SETUP_BOTS) {
            skirmish_bots = (skirmish_bots - 1 + (forward ? 1 : 2)) % 3 + 1;
        } else if (frontier_setup_row == format_row) {
            frontier_cpu_team ^= 1;
        } else {
            unsigned char *faction = frontier_setup_row == FRONTIER_SETUP_PLAYER
                ? &frontier_human_faction
                : &frontier_cpu_faction[frontier_setup_row - FRONTIER_SETUP_CPU_FIRST];
            int direction = forward ? 1 : FRONTIER_FACTION_COUNT - 1;
            *faction = (*faction + direction) % FRONTIER_FACTION_COUNT;
        }
    }
}

static void
frontier_title_draw(void)
{
    static const char *const labels[] = {"MAP", "OPPONENTS", "PLAYER", "CPU 1", "CPU 2", "CPU 3"};
    int format_row = FRONTIER_SETUP_CPU_FIRST + skirmish_bots;
    int format_selected = frontier_setup_row == format_row;
    int format_color = format_selected ? C_HIGHLIGHT : C_WHITE;
    gfx_rect(0, 0, 240, 160, C_BLACK);
    gfx_rect(12, 8, 216, 144, C_PANEL);
    gfx_frame(12, 8, 216, 144, C_CYAN);
    gfx_text(25, 17, "FRONTIER SKIRMISH", C_GOLD);
    gfx_text(25, 31, "CARTRIDGE", C_WHITE);
    gfx_text(102, 31, frontier_faction_profiles[SC_FRONTIER_FACTION].name, C_CYAN);
    for (int row = 0; row < FRONTIER_SETUP_CPU_FIRST + 3; row++) {
        int y = 47 + row * 10;
        int active = row < format_row;
        int selected = active && row == frontier_setup_row;
        int color = selected ? C_HIGHLIGHT : active ? C_WHITE : C_DIM;
        if (selected) gfx_text(19, y, ">", C_HIGHLIGHT);
        gfx_text(32, y, labels[row], color);
        if (row == FRONTIER_SETUP_MAP)
            gfx_text(108, y, original_skirmish_maps[original_skirmish_map].title, color);
        else if (row == FRONTIER_SETUP_BOTS)
            gfx_num(108, y, skirmish_bots, color);
        else if (row == FRONTIER_SETUP_PLAYER)
            gfx_text(108, y, frontier_owner_profile(SIDE)->name, color);
        else
            gfx_text(108, y, active ? frontier_faction_profiles[frontier_cpu_faction[row - FRONTIER_SETUP_CPU_FIRST]].name
                                    : "CLOSED", color);
    }
    if (format_selected) gfx_text(19, 107, ">", C_HIGHLIGHT);
    gfx_text(32, 107, "MODE", format_color);
    gfx_text(108, 107, frontier_cpu_team ? "CPU TEAM" : "FREE FOR ALL", format_color);
    gfx_text(25, 119, "UP/DOWN ROW  LEFT/RIGHT EDIT", C_CYAN);
    gfx_text(25, 130, "A START  B LOAD", C_CYAN);
    if (title_notice) gfx_text(25, 141, "NO COMPATIBLE SAVE", C_RED);
}

#endif
