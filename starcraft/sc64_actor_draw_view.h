/* SPDX-License-Identifier: GPL-3.0-or-later */
#if defined(SC64_WORLD_DEPTH) && defined(GBA_SCENE_CLIP) && defined(SC64_HAS_SPRITES)
static inline int
sc64_unit_crop_intersects(int x, int y, int width, int height)
{
    return x < 240 && x + width > 0 && y < 120 && y + height > 12;
}

static inline int
sc64_unit_frame_in_view(const Sc64SpriteFrame *sprite, int x, int y, int mirror)
{
    if (!sprite) return 0;
    x = mirror ? x + sprite->canvas_w / 2 - sprite->offset_x - sprite->w
               : x - sprite->canvas_w / 2 + sprite->offset_x;
    y += sprite->offset_y - sprite->canvas_h / 2;
    return sc64_unit_crop_intersects(x, y, sprite->w, sprite->h);
}

#ifdef SC64_UNIT_FACING
static int
sc64_unit_facing_frame_in_view(unsigned raw, int direction, int x, int y)
{
#if defined(SC64_UNIT_WORD_SPRITES) && defined(SC64_UNIT_FACING_WORD_METADATA)
    const Sc64UnitFacingWordFrame *word = sc64_unit_facing_word_frame(raw, direction);
    if (word)
        return sc64_unit_crop_intersects(x - word->canvas_w / 2 + word->offset_x,
                    y - word->canvas_h / 2 + word->offset_y, word->sprite->width, word->sprite->height);
#endif
    const Sc64SpriteFrame *frames = sc64_unit_facing_frames(raw);
    return frames && sc64_unit_frame_in_view(&frames[direction <= 16 ? direction : 32 - direction],
                                             x, y, direction > 16);
}
#endif

static int __attribute__((noinline))
sc64_unit_body_crop_in_view(const Unit *q, int x, int y)
{
    unsigned slot = (unsigned)(q - u);
    unsigned bits = SC64_WALK_BITS[slot];
    unsigned stepped = (bits & SC64_WALK_KEY_MASK) != sc64_walk_ground_key(q);
    int flying = sc64_race_unit_flying(q->type);
    int walk = flying ? sc64_walk_flyer_phase(q) : sc64_walk_ground_phase(q, bits, stepped);
    int lift = SC64_WALK_LIFT(walk);
#if defined(SC64_WEAPON_SOURCE_VFX) && defined(SC64_UNIT_ATTACK_POSES_ENABLED)
    const Sc64SpriteFrame *attack = sc64_source_unit_attack_pose(q);
    if (attack) return sc64_unit_frame_in_view(attack, x, y - lift, 0);
#endif
    int raw = sc64_unit_art_raw(q);
#ifdef SC64_UNIT_FACING
    int tank = q->type == U_TANK, sieged = q->type == U_SIEGE_TANK;
    int body_raw = tank ? 5 : sieged ? 30 : raw;
    const Sc64SpriteFrame *facing = body_raw >= 0 ? sc64_unit_facing_frames((unsigned)body_raw) : 0;
    if (facing) {
        int direction;
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
        if (sc64_race_is_battlecruiser(q->type))
            direction = sc64_bc_heading_face((u8)((unsigned)q->work_timer >> 3));
        else
#endif
            direction = sc64_facing_order_direction(q);
        if (direction < 0) direction = sc64_facing[slot] & SC64_FACING_MASK;

#ifdef SC64_UNIT_WALK_FRAMES
        const Sc64SpriteFrame *pose = (flying || walk) && !sieged ? sc64_walk_pose((unsigned)body_raw, (int)slot) : 0;
        if (pose) {
            if (sc64_unit_frame_in_view(&pose[direction], x, y, 0)) return 1;
            return tank && sc64_unit_facing_frame_in_view(6, direction, x, y);
        }
#endif
        if ((tank || sieged) && sc64_unit_facing_frame_in_view(sieged ? 31 : 6, direction, x, y - lift))
            return 1;
        return sc64_unit_facing_frame_in_view((unsigned)body_raw, direction, x, y - lift);
    }
#endif
    const Sc64SpriteFrame *sprite = q->raw_type >= 0 ? sc64_p8_unit_sprite_for_raw(raw)
                                                   : mission_unit_sprite(q->type);
    return sc64_unit_frame_in_view(sprite, x, y - lift, 0);
}
#endif

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
    if (x >= -8 && x <= 247 && y >= 10 && y <= 125) return 1;
#if defined(SC64_WORLD_DEPTH) && defined(GBA_SCENE_CLIP) && defined(SC64_HAS_SPRITES)
    /* Bound the lookup by the prepared body art's maximum reach, including
     * stride lift. The current raster crop determines admission. */
    if (x < -64 || x >= 304 || y < -60 || y >= 192) return 0;
    return sc64_unit_body_crop_in_view(q, x, y);
#else
    return 0;
#endif
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
