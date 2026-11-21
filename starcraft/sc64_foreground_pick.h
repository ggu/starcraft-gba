/* SPDX-License-Identifier: GPL-3.0-or-later */

/* Match draw_mission_sprite and sc64_facing_blit_frame. Indexed zero is
 * transparent in both the byte and word blitters. */
static int
sc64_pick_sprite_pixel(const Sc64SpriteFrame *sprite, int x, int y, int mirror)
{
    if (!sprite || !sprite->pixels) return 0;
    int left = mirror ? x + sprite->canvas_w / 2 - sprite->offset_x - sprite->w
                      : x - sprite->canvas_w / 2 + sprite->offset_x;
    int col = cx - left;
    int row = cy - (y - sprite->canvas_h / 2 + sprite->offset_y);
    if ((unsigned)col >= sprite->w || (unsigned)row >= sprite->h) return 0;
    if (mirror) col = sprite->w - 1 - col;
    return sprite->pixels[row * sprite->w + col] != 0;
}

/* Negative means the renderer uses a body pose this mask cannot verify. */
static int
sc64_pick_unit_body(const Unit *q, int raw, int flying)
{
    if (!sc64_unit_anchor_in_view(q, q->x - camx, q->y - camy + 12) ||
        SC64_TRANSPORT_LOADED(q) || SC64_SCARAB_HIDDEN(q)) return 0;
#ifdef SC64_P6_COMBAT_ENABLED
    if (race_p6_combat_unit_burrowed((int)(q - u))) return 0;
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active() && race_t6_unit_burrowed((int)(q - u))) return 0;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    unsigned flags = sc64_game_unit_actor_flags(q->type);
    if ((flags & SC64_GAME_ACTOR_HIDDEN) || (flags && !(flags & SC64_GAME_ACTOR_DRAWABLE)) ||
        SC64_CARRIER_STATE_HIDDEN(q)) return 0;
#endif
#ifdef SC64_GUARDIAN_COCOON_RENDER_STAGE
    if (q->type == U_GUARDIAN_COCOON) return 0;
#endif
#if defined(SC64_SHARED_NUKE_PROBE) && defined(SC64_UNIT_FACING)
    if (sc64_race_ordinary_ghost(q->type)) return -1;
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    if (sc64_race_is_battlecruiser(q->type)) return -1;
#endif
#if defined(SC64_WEAPON_SOURCE_VFX) && defined(SC64_UNIT_ATTACK_POSES_ENABLED)
    if (q->cd) return -1;
#endif
    if (q->type == U_TANK || q->type == U_SIEGE_TANK ||
        (q->type == U_GOLIATH && raw == 3)) return -1;
    const int slot = (int)(q - u);
    /* The last painted facing is already retained. A ground stride can be
     * held for one draw and then retire its latch. Limit the ground check to
     * standing art so selection never advances that presentation state. */
    if (!flying && (q->order != ORDER_IDLE || q->path_timer ||
                    (SC64_WALK_BITS[slot] & (SC64_WALK_KEY_MASK | SC64_WALK_HOLD))))
        return -1;
    const Sc64SpriteFrame *sprite = q->raw_type >= 0 ? sc64_p8_unit_sprite_for_raw(raw)
                                                    : mission_unit_sprite(q->type);
    int walk = 0;
    if (flying && q->order != ORDER_IDLE && !(q->cd && q->order == ORDER_ATTACK_TARGET) &&
        (q->x != q->tx || q->y != q->ty))
        walk = (int)(((unsigned)frame >> 2) & 3u) + 1;
#ifdef SC64_UNIT_FACING
    if (raw >= 0) {
        const Sc64SpriteFrame *facing = sc64_unit_facing_frames((unsigned short)raw);
        int dir = sc64_facing[slot] & SC64_FACING_MASK;
#ifdef SC64_UNIT_WALK_FRAMES
        if (flying && (sprite || facing)) {
            const Sc64UnitWalkCycle *cycle = sc64_unit_walk_cycle((unsigned short)raw);
            if (cycle) {
                const Sc64SpriteFrame *pose =
                    sc64_walk_pose_at(cycle, slot, (unsigned)frame * 4u / 5u, 0);
                return sc64_pick_sprite_pixel(&pose[dir], q->x, q->y, 0);
            }
        }
#endif
        if (facing)
            return sc64_pick_sprite_pixel(&facing[dir <= 16 ? dir : 32 - dir],
                                          q->x, q->y - SC64_WALK_LIFT(walk), dir > 16);
    }
#endif
    return sprite ? sc64_pick_sprite_pixel(sprite, q->x, q->y - SC64_WALK_LIFT(walk), 0)
                  : -1;
}

static int
sc64_pick_building_body(const Building *site)
{
    if (!building_art_in_view(site, site->x - camx, site->y - camy + 12)) return 0;
    int raw = site->raw_type >= 0 ? site->raw_type : sc64_race_raw_building(site->type);
    if (raw < 106 || raw > 175 || !building_art_is_static(site)) return -1;
    const Sc64SpriteFrame *sprite = building_art_sprite(site);
    return sprite ? sc64_pick_sprite_pixel(sprite, site->x, site->y, 0) : -1;
}

static int
sc64_pick_actor_body(int ref)
{
    if (ref < MAX_UNITS) {
        const Unit *q = &u[ref];
        if (!currently_visible(q->x, q->y)) return 0;
        return sc64_pick_unit_body(q, sc64_unit_art_raw(q), sc64_race_unit_flying(q->type));
    }
    const Building *site = &b[ref - MAX_UNITS];
    return currently_visible(site->x, site->y) ? sc64_pick_building_body(site) : 0;
}

static void
sc64_consider_foreground(int ref, u32 key, int *foreground, u32 *front_key, u32 *unknown_key)
{
    if (key <= *front_key) return;
    int mask = sc64_pick_actor_body(ref);
    if (mask > 0) {
        *foreground = ref;
        *front_key = key;
    } else if (mask < 0 && key > *unknown_key) {
        *unknown_key = key;
    }
}

/* Compare visible opaque candidates together, using the painter's unsigned
 * key for both kinds. Keep proximity when the initial hit or a potentially
 * occluding foreground pose cannot be verified. */
static int __attribute__((noinline))
sc64_foreground_pick(int first_unit, int last_unit,
                     int first_building, int last_building, int proximity)
{
    int sx = cx - camx, sy = cy - camy + 12;
    if ((unsigned)sx >= 240 || sy < 12 || sy >= 120 || !currently_visible(cx, cy) ||
        sc64_pick_actor_body(proximity) <= 0) return proximity;

    int foreground = proximity;
    u32 front_key = proximity < MAX_UNITS ? sc64_unit_depth_key((unsigned)proximity)
        : sc64_building_depth_key((unsigned)(proximity - MAX_UNITS));
    u32 unknown_key = 0;
    for (int i = first_unit; i >= 0 && i <= last_unit; ++i) {
        if (!selection_unit_candidate(i) || dist2(cx, cy, u[i].x, u[i].y) >= 31 * 31) continue;
        sc64_consider_foreground(i, sc64_unit_depth_key((unsigned)i),
                                 &foreground, &front_key, &unknown_key);
    }
    for (int i = first_building; i >= 0 && i <= last_building; ++i) {
        if (!selection_building_candidate(i) || dist2(cx, cy, b[i].x, b[i].y) >= 26 * 26) continue;
        sc64_consider_foreground(MAX_UNITS + i, sc64_building_depth_key((unsigned)i),
                                 &foreground, &front_key, &unknown_key);
    }
    return unknown_key > front_key ? proximity : foreground;
}
