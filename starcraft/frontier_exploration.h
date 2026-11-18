/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FRONTIER_EXPLORATION_H
#define FRONTIER_EXPLORATION_H

/* Frontier keeps a byte visibility grid. Clear aligned words to avoid a
 * byte-store loop over all 4,096 cells in every simulation update. */
typedef u32 FrontierVisibilityWord __attribute__((may_alias));
_Static_assert(sizeof(visible) % 16 == 0, "Visibility clear uses four-word groups");

static inline void
frontier_clear_visible(void)
{
    volatile FrontierVisibilityWord *words = (FrontierVisibilityWord *)(void *)visible;
    for (unsigned i = 0; i < sizeof(visible) / sizeof(*words); i += 4) {
        words[i] = 0;
        words[i + 1] = 0;
        words[i + 2] = 0;
        words[i + 3] = 0;
    }
}

/* Exploration only adds tiles until the explored grid is replaced. Each
 * center has one bit per supported radius, packed two centers to a byte. */
static u8 frontier_explored_centers[MAP_CELLS / 2];

static void
frontier_reset_exploration_cache(void)
{
    memset(frontier_explored_centers, 0, sizeof(frontier_explored_centers));
}

static inline int
frontier_explored_before(int col, int row, int radius)
{
    static const u8 radius_bit[] = {[3] = 1, [4] = 2, [5] = 4, [7] = 8};
    if ((unsigned)col >= MAP_COLS || (unsigned)row >= MAP_ROWS) return 0;
    unsigned cell = (unsigned)(row * MAP_COLS + col);
    u8 mask = (u8)(radius_bit[radius] << ((cell & 1u) * 4));
    u8 *known = &frontier_explored_centers[cell >> 1];
    if (*known & mask) return 1;
    *known |= mask;
    return 0;
}

#endif
