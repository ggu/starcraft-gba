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
    if (sc64_race_ordinary_ghost(q->type)) return 0;
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    if (sc64_race_is_battlecruiser(q->type)) return 0;
#endif
#if defined(SC64_WEAPON_SOURCE_VFX) && defined(SC64_UNIT_ATTACK_POSES_ENABLED)
    if (q->cd) return 0;
#endif
    if (q->type == U_TANK || q->type == U_SIEGE_TANK) return 0;
    const int slot = (int)(q - u);
    /* The last painted facing is already retained. A ground stride can be
     * held for one draw and then retire its latch. Limit the ground check to
     * standing art so selection never advances that presentation state. */
    if (!flying && (q->order != ORDER_IDLE || q->path_timer ||
                    (SC64_WALK_BITS[slot] & (SC64_WALK_KEY_MASK | SC64_WALK_HOLD))))
        return 0;
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
    return sc64_pick_sprite_pixel(sprite, q->x, q->y - SC64_WALK_LIFT(walk), 0);
}

static int __attribute__((noinline))
sc64_foreground_pick(int unit, int building, int proximity)
{
    const Unit *q = &u[unit];
    const Building *site = &b[building];
    int sx = cx - camx, sy = cy - camy + 12;
    if ((unsigned)sx >= 240 || sy < 12 || sy >= 120) return proximity;
    int unit_raw = sc64_unit_art_raw(q);
    int building_raw = site->raw_type >= 0 ? site->raw_type : sc64_race_raw_building(site->type);
    int flying = sc64_race_unit_flying(q->type);
    u32 unit_depth = sc64_world_depth_key(sc64_unit_world_layer(q),
                                         q->y, DEPTH_UNIT, (unsigned)unit);
    u32 building_depth = sc64_world_depth_key(sc64_building_world_layer((unsigned)building),
                                             site->y, DEPTH_BUILDING, (unsigned)building);
    /* Upper air layers set bit 31. The source picker and the GBA painter
     * compare unsigned keys. Equal layer/Y uses the painter's kind/slot tie. */
    int foreground = unit_depth > building_depth ? unit : MAX_UNITS + building;
    if (foreground == proximity) return proximity;

    /* Only override where both supported bodies and the click are visible.
     * Doodads and dynamic art without a primary mask keep proximity. */
    if (!currently_visible(cx, cy) || !currently_visible(q->x, q->y) ||
        !currently_visible(site->x, site->y) ||
        building_raw < 106 || building_raw > 175 || !building_art_is_static(site) ||
        !building_art_in_view(site, site->x - camx, site->y - camy + 12) ||
        !sc64_pick_sprite_pixel(building_art_sprite(site), site->x, site->y, 0) ||
        !sc64_pick_unit_body(q, unit_raw, flying)) return proximity;
    return foreground;
}
