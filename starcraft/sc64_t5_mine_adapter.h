#ifndef SC64_T5_MINE_ADAPTER_H
#define SC64_T5_MINE_ADAPTER_H

#ifdef SC64_T5_RUNTIME

/* The GBA executes at 30 ticks/s while StarCraft's reference game advances
 * at 24 frames/s. The source mine's 60-frame arm delay is represented as 75
 * GBA ticks. Burrow/unburrow/explosion IScript signals are collapsed into
 * immediate state transitions; those presentation timings are not reproduced. */
enum {
    SC64_T5_MINE_ARM_TICKS = 75,
    SC64_T5_MINE_ARMING = 1,
    SC64_T5_MINE_ARMED = 2,
    SC64_T5_MINE_CHASING = 3,
    SC64_T5_MINE_STEP_PIXELS = 6,
    SC64_T5_MINE_ACQUIRE_SOURCE_PX = 96,
    SC64_T5_MINE_CHASE_SOURCE_PX = 576,
    SC64_T5_MINE_CONTACT_SOURCE_PX = 30,
    SC64_T5_MINE_INNER_SPLASH_SOURCE_PX = 50,
    SC64_T5_MINE_MID_SPLASH_SOURCE_PX = 75,
    SC64_T5_MINE_OUTER_SPLASH_SOURCE_PX = 100,
    SC64_T5_MINE_EXPLOSION_DAMAGE = 125
};

/* OpenBW's Spider Mine predicate rejects unknown1 == 0xc1 (commented
 * "hovering?") in addition to flyers. The T5 source roster includes Vulture
 * raw 2 and Raynor Vulture raw 19; reject those known hover-capable identities
 * explicitly. The N64 units.dat unknown1 bytes differ from OpenBW's PC table,
 * so this is a source-type policy rather than a direct byte-value translation. */
static int
sc64_t5_mine_source_hover_raw(int raw_type)
{
    return raw_type == 2 || raw_type == 19;
}

/* Source weapon 6 has radial splash. OpenBW divides by 1/2/4 before armor
 * processing; the callers pass this divisor to hit_target so armor precedes
 * the Explosive size multiplier there. Radii are source pixels and inclusive. */
static int
sc64_t5_mine_splash_divisor_sq(int source_distance_sq)
{
    const int inner = SC64_T5_MINE_INNER_SPLASH_SOURCE_PX;
    const int middle = SC64_T5_MINE_MID_SPLASH_SOURCE_PX;
    const int outer = SC64_T5_MINE_OUTER_SPLASH_SOURCE_PX;
    if (source_distance_sq > outer * outer) return 0;
    if (source_distance_sq <= inner * inner) return 1;
    if (source_distance_sq <= middle * middle) return 2;
    return 4;
}

static int
sc64_t5_mine_rect_for_unit(const Unit *unit, Sc64T4QueryRect *rect)
{
    int raw, left, up, right, down;
    if (!unit || !rect) return 0;
    raw = unit->raw_type >= 0 ? unit->raw_type : sc64_race_raw_unit(unit->type);
    if (sc64_t4_query_actor_rect((unsigned)raw, unit->x * 2,
                                 unit->y * 2, rect)) return 1;
    if (!sc64_t5_actor_bounds_source(unit->type, &left, &up, &right, &down))
        return 0;
    rect->left = unit->x * 2 - left;
    rect->top = unit->y * 2 - up;
    rect->right = unit->x * 2 + right + 1;
    rect->bottom = unit->y * 2 + down + 1;
    return 1;
}

static int
sc64_t5_mine_rect_gap_sq(const Sc64T4QueryRect *a,
                         const Sc64T4QueryRect *b)
{
    int dx = 0, dy = 0;
    if (a->right <= b->left) dx = b->left - a->right;
    else if (b->right <= a->left) dx = a->left - b->right;
    if (a->bottom <= b->top) dy = b->top - a->bottom;
    else if (b->bottom <= a->top) dy = a->top - b->bottom;
    return dx * dx + dy * dy;
}

/* Euclidean distance from the blast point to the nearest source-pixel in an
 * actor's half-open source rectangle, matching OpenBW unit_distance_to(). */
static int
sc64_t5_mine_point_rect_distance_sq(int x, int y,
                                    const Sc64T4QueryRect *rect)
{
    int dx = 0, dy = 0;
    if (x < rect->left) dx = rect->left - x;
    else if (x >= rect->right) dx = x - (rect->right - 1);
    if (y < rect->top) dy = rect->top - y;
    else if (y >= rect->bottom) dy = y - (rect->bottom - 1);
    return dx * dx + dy * dy;
}

static int
sc64_t5_mine_unit_targetable(const Unit *mine, const Unit *target)
{
    int raw;
#ifdef SC64_TRANSPORT_LOADED
    if (target && SC64_TRANSPORT_LOADED(target)) return 0;
#endif
    if (!mine || !target || !target->alive || target == mine ||
        !owners_hostile(mine->owner, target->owner) ||
        invincible_units[(int)(target - u)] || (target->flags & 16) ||
        sc64_race_unit_flying(target->type)) return 0;
    raw = target->raw_type >= 0 ? target->raw_type :
          sc64_race_raw_unit(target->type);
    return !sc64_t5_mine_source_hover_raw(raw);
}

static int
sc64_t5_mine_find_target(const Unit *mine)
{
    int selected = -1;
    int radius = SC64_T5_MINE_ACQUIRE_SOURCE_PX;
    /* Acquisition uses a square search region, while nearest selection is
       center-distance based; its corners are valid even when d² > r². */
    int best = 2 * radius * radius + 1;
    int mx, my;
    if (!mine) return -1;
    mx = mine->x * 2; my = mine->y * 2;
    /* Search source-unit slots in ascending order; strict improvement keeps
       the lowest slot on equal center distance. Native search uses indexed
       centers with an acquisition square; this is the bounded slot-order
       policy used by this runtime adapter. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        int dx, dy, d;
        if (!sc64_t5_mine_unit_targetable(mine, &u[i])) continue;
        dx = u[i].x * 2 - mx; dy = u[i].y * 2 - my;
        if (dx < -SC64_T5_MINE_ACQUIRE_SOURCE_PX ||
            dx > SC64_T5_MINE_ACQUIRE_SOURCE_PX ||
            dy < -SC64_T5_MINE_ACQUIRE_SOURCE_PX ||
            dy > SC64_T5_MINE_ACQUIRE_SOURCE_PX) continue;
        d = dx * dx + dy * dy;
        if (d < best) { best = d; selected = i; }
    }
    return selected;
}

static int
sc64_t5_mine_in_contact(const Unit *mine, const Unit *target)
{
    Sc64T4QueryRect a, b;
    int mine_raw;
    if (!mine || !target || !sc64_t5_mine_rect_for_unit(target, &b)) return 0;
    mine_raw = mine->raw_type >= 0 ? mine->raw_type : 13;
    if (!sc64_t4_query_actor_rect((unsigned)mine_raw, mine->x * 2,
                                  mine->y * 2, &a)) {
        int l=7, u0=7, r=7, d=7;
        a.left = mine->x * 2-l; a.top = mine->y * 2-u0;
        a.right = mine->x * 2+r+1; a.bottom = mine->y * 2+d+1;
    }
    return sc64_t5_mine_rect_gap_sq(&a, &b) <=
           SC64_T5_MINE_CONTACT_SOURCE_PX * SC64_T5_MINE_CONTACT_SOURCE_PX;
}

static void
sc64_t5_mine_explode(Unit *mine)
{
    int center_x, center_y;
    if (!mine) return;
    center_x = mine->x * 2; center_y = mine->y * 2;
    /* Radial splash is allowed to hit allied ground units too. The native
       weapon is Ground + NoBuild; buildings and flyers are not candidates. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        Sc64T4QueryRect rect;
        int divisor;
        if (!u[i].alive || &u[i] == mine || sc64_race_unit_flying(u[i].type) ||
            !sc64_t5_mine_rect_for_unit(&u[i], &rect)) continue;
        divisor = sc64_t5_mine_splash_divisor_sq(
            sc64_t5_mine_point_rect_distance_sq(center_x, center_y, &rect));
        if (divisor) hit_target(i, SC64_T5_MINE_EXPLOSION_DAMAGE / divisor,
                                U_SPIDER_MINE, mine->owner);
    }
    mine->alive = 0;
    mine->selected = 0;
}

/* Minimal collision-aware local pursuit. Top speed 16 source pixels/frame
 * converts to 6.4 GBA pixels/tick at 2:1 map scale and 30 vs 24 Hz. Use six
 * bounded substeps per tick; this approximates speed without sprite motion or
 * acceleration fidelity. */
static int
sc64_t5_mine_step(Unit *mine, int tx, int ty)
{
    int moved = 0;
    if (mine->x == tx && mine->y == ty) return 0;
    if (mine->path_timer <= 0) {
        if (direct_path_clear(mine->x, mine->y, tx, ty)) {
            mine->path_x = tx; mine->path_y = ty;
        } else if (!path_next_tile(mine->x, mine->y, tx, ty,
                                   &mine->path_x, &mine->path_y)) {
            mine->path_timer = 0;
            mine->path_retry = 6;
            return 0;
        }
        mine->path_timer = 16;
    }
    if (mine->path_retry > 0) { --mine->path_retry; return 0; }
    for (int step = 0; step < SC64_T5_MINE_STEP_PIXELS; ++step) {
        int nx = mine->x + (mine->path_x > mine->x ? 1 :
                            mine->path_x < mine->x ? -1 : 0);
        int ny = mine->y + (mine->path_y > mine->y ? 1 :
                            mine->path_y < mine->y ? -1 : 0);
        if ((nx == mine->x && ny == mine->y) ||
            !is_traversable(nx / 16, ny / 16, -1) ||
            !unit_step_clear(mine, nx, ny)) {
            mine->path_timer = 0;
            mine->path_retry = 2;
            break;
        }
        mine->x = nx; mine->y = ny;
        moved = 1;
    }
    if (mine->path_timer > 0) --mine->path_timer;
    if (moved) {
        sc64_unit_bucket_move(mine);
        sc64_target_index_move(mine);
    }
    return moved;
}

static void
race_t5_mines_tick(void)
{
    int changed = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *mine = &u[i];
        int raw, target;
        if (!mine->alive || mine->type != U_SPIDER_MINE) continue;
        raw = mine->raw_type >= 0 ? mine->raw_type : sc64_race_raw_unit(mine->type);
        if (raw != 13 || mine->raw_owner != 6) continue;
        if (mine->carry == 0) {
            mine->carry = SC64_T5_MINE_ARMING;
            mine->work_timer = SC64_T5_MINE_ARM_TICKS;
            mine->target = -1;
        }
        if (mine->carry == SC64_T5_MINE_ARMING) {
            if (mine->work_timer > 0) --mine->work_timer;
            if (mine->work_timer == 0) mine->carry = SC64_T5_MINE_ARMED;
            continue;
        }
        if (mine->carry == SC64_T5_MINE_ARMED) {
            target = sc64_t5_mine_find_target(mine);
            if (target >= 0) {
                mine->target = target;
                mine->carry = SC64_T5_MINE_CHASING;
                mine->path_timer = 0;
                mine->path_retry = 0;
            }
            continue;
        }
        if (mine->carry != SC64_T5_MINE_CHASING ||
            mine->target < 0 || mine->target >= MAX_UNITS ||
            !sc64_t5_mine_unit_targetable(mine, &u[mine->target])) {
            mine->carry = SC64_T5_MINE_ARMED;
            mine->target = -1;
            continue;
        }
        target = mine->target;
        {
            Sc64T4QueryRect mr, tr;
            int chase_sq = SC64_T5_MINE_CHASE_SOURCE_PX *
                           SC64_T5_MINE_CHASE_SOURCE_PX;
            if (!sc64_t5_mine_rect_for_unit(mine, &mr) ||
                !sc64_t5_mine_rect_for_unit(&u[target], &tr) ||
                sc64_t5_mine_rect_gap_sq(&mr, &tr) > chase_sq) {
                mine->carry = SC64_T5_MINE_ARMED;
                mine->target = -1;
                continue;
            }
        }
        if (sc64_t5_mine_in_contact(mine, &u[target])) {
            sc64_t5_mine_explode(mine);
            changed = 1;
        } else if (sc64_t5_mine_step(mine, u[target].x, u[target].y)) {
            changed = 1;
        }
    }
    if (changed) {
        target_lists_valid = 0;
        sc64_target_index_end();
        sc64_unit_buckets_end();
    }
}

#endif /* SC64_T5_RUNTIME */
#endif
