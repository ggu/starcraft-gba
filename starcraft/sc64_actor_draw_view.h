/* SPDX-License-Identifier: GPL-3.0-or-later */
static inline int
sc64_unit_body_anchor_in_view(const Unit *q, int x, int y)
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

#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
static inline unsigned sc64_yamato_release_charge_ticks(const Unit *q)
{
    unsigned remaining = q->work_timer & 7u;
    return q->order == ORDER_ATTACK_TARGET && remaining <= 6u ? remaining : 0;
}
#endif

#if defined(SC64_WORLD_DEPTH) && defined(SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT)
static inline int sc64_unit_charge_anchor_in_view(const Unit *q, int x, int y)
{
    /* Charge image 543 is centered on its caster in an 80-pixel canvas.
     * Let the source raster clip the particular frame and check its fog. */
    if (x < -40 || x >= 280 || y < -28 || y >= 160 ||
        !sc64_race_is_battlecruiser(q->type)) return 0;
    if (sc64_yamato_release_charge_ticks(q)) return 1;
    unsigned caster = (unsigned)(q - u);
    for (unsigned i = 0; i < SC64_BC_SHOT_LIMIT; ++i)
        if (sc64_bc_shots[i].state == SC64_BC_SHOT_CHARGING && sc64_bc_shots[i].caster == caster)
            return 1;
    return 0;
}
#endif

static inline int
sc64_unit_anchor_in_view(const Unit *q, int x, int y)
{
    if (sc64_unit_body_anchor_in_view(q, x, y)) return 1;
#if defined(SC64_WORLD_DEPTH) && defined(SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT)
    return sc64_unit_charge_anchor_in_view(q, x, y);
#else
    return 0;
#endif
}
