/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SAVE_UI_CONTEXT_H
#define SC64_SAVE_UI_CONTEXT_H

/* state[30] belongs to the generic Protoss service clock. Word 31 was
 * unassigned: the tagged extension leaves older zero-filled saves readable. */
#define SC64_SAVE_UI_STATE_INDEX 31
#define SC64_SAVE_UI_TAG 0xa1000000u
#define SC64_SAVE_UI_MENU_MAX 63u

static unsigned sc64_save_ui_encode(int building, int building_count,
                                    int cursor, unsigned last)
{
    unsigned selected = building >= 0 && building < building_count ? (unsigned)building + 1u : 0u;
    unsigned safe_cursor = cursor >= 0 && (unsigned)cursor <= SC64_SAVE_UI_MENU_MAX ? (unsigned)cursor : 0u;
    unsigned safe_last = last <= SC64_SAVE_UI_MENU_MAX ? last : 0u;
    return SC64_SAVE_UI_TAG | selected | (safe_cursor << 8) | (safe_last << 16);
}

static int sc64_save_ui_valid(unsigned word, unsigned building_count)
{
    if (!word) return 1;
    return (word & 0xff000000u) == SC64_SAVE_UI_TAG &&
           (word & 255u) <= building_count &&
           ((word >> 8) & 255u) <= SC64_SAVE_UI_MENU_MAX &&
           ((word >> 16) & 255u) <= SC64_SAVE_UI_MENU_MAX;
}
#endif
