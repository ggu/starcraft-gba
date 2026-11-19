/* SPDX-License-Identifier: GPL-3.0-or-later */
#if defined(SC64_RACES) && defined(SC64_IMPORTED_MISSION) && \
    !defined(SC64_TILE_RENDERER) && !defined(SC64_DISABLE_WORLD_DEPTH)
#define SC64_WORLD_DEPTH 1
#endif

#if defined(SC64_FOREGROUND_PICK) && defined(SC64_WORLD_DEPTH) && defined(SC64_HAS_SPRITES)
#define SC64_FOREGROUND_PICK_ENABLED 1
static int sc64_foreground_pick(int unit, int building, int proximity);
#endif

static inline int
sc64_unit_anchor_in_view(const Unit *q, int x, int y)
{
#ifdef SC64_GUARDIAN_COCOON_RENDER_STAGE
    /* Include both the primary canvas and its displaced underlay. */
    if (q->type == U_GUARDIAN_COCOON)
        return x >= -64 && x <= 304 && y >= -80 && y <= 180;
#else
    (void)q;
#endif
    return x >= -8 && x <= 247 && y >= 10 && y <= 125;
}
