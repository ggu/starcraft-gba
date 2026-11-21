/* SPDX-License-Identifier: GPL-3.0-or-later */
static int
death_vfx_raw(int slot)
{
#ifdef SC64_RACES
    if (slot < MAX_UNITS)
        return u[slot].raw_type >= 0 ? u[slot].raw_type : sc64_race_raw_unit(u[slot].type);
    const Building *q = &b[slot - MAX_UNITS];
    return q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
#else
    /* Original-art types use the matching StarCraft unit ids. */
    static const unsigned char units[] = { 7, 0, 32, 34, 37, 38, 19 };
    static const unsigned char buildings[] = { 106, 111, 109, 110, 112, 122, 131,
                                               125, 130, 143 };
    int type = slot < MAX_UNITS ? u[slot].type : b[slot - MAX_UNITS].type;
    if (slot < MAX_UNITS)
        return type >= 0 && type < (int)sizeof(units) ? units[type] : -1;
    return type >= 0 && type < (int)sizeof(buildings) ? buildings[type] : -1;
#endif
}

#if defined(SC64_HAS_SPRITES) && !defined(SC64_RACES)
static const Sc64SpriteFrame *mission_unit_sprite(int type);
static const Sc64SpriteFrame *mission_building_sprite(int type);
#endif
static int
death_vfx_extent(int slot, int raw)
{
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite =
#ifdef SC64_RACES
        raw >= 0 ? sc64_p8_unit_sprite_for_raw((unsigned short)raw) : 0;
#else
        slot < MAX_UNITS ? mission_unit_sprite(u[slot].type) :
                           mission_building_sprite(b[slot - MAX_UNITS].type);
#endif
    (void)raw;
    if (sprite) return (sprite->w > sprite->h ? sprite->w : sprite->h) / 2;
#else
    (void)raw;
#endif
    return slot < MAX_UNITS ? 5 : 14;
}

static void
death_vfx_spawn_slot(Sc64DeathVfx *pool, int slot, int x, int y, int owner)
{
    int raw = death_vfx_raw(slot), building = slot >= MAX_UNITS;
    int style = sc64_death_vfx_style_for_raw(raw, building), source = 0;
    Sc64DeathVfxTiming timing = sc64_death_vfx_timing(style);
#ifdef SC64_DEATH_SOURCE_ART
    const Sc64DeathSource *art = raw >= 0 ? sc64_death_source(raw) : 0;
    if (sc64_death_source_valid(art)) {
        timing.burst_su = art->burst_su;
        timing.remnant_start = art->remnant_start;
        timing.total_su = art->total_su;
        source = 1;
    }
#endif
    int effect = sc64_death_vfx_spawn(pool, SC64_DEATH_VFX_POOL, death_vfx_now(), style, raw,
                                      x, y, death_vfx_extent(slot, raw), &timing, source,
                                      owner_color(owner));
#ifdef SC64_DEATH_WORLD_DEPTH
    if (effect >= 0) {
        unsigned layer = building ? sc64_building_world_layer((unsigned)(slot - MAX_UNITS)) :
                                    sc64_unit_world_layer(&u[slot]);
        pool[effect].actor_layer_plus1 = layer + 1;
        death_vfx_actor_plus1[effect] = (u16)(slot + 1);
    }
#else
    (void)effect;
#endif
}
