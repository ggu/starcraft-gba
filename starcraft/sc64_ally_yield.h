/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ALLY_YIELD_H
#define SC64_ALLY_YIELD_H

/* Blocked ground movers in the ROM-derived profiles (SC64_RACES). In
 * StarCraft an idle unit steps aside for a moving unit of its own side, and
 * a Move whose mover cannot get closer to its point ends where it stands.
 * The engine had neither: a mover boxed in by idle allies waited 6 updates,
 * repathed and tried again for ever (T4: Raynor's group stopped short of
 * loc14 behind idle Marines, two of them in ORDER_MOVE 7-8 px short of their
 * slots).
 *
 * In move_toward():
 *  1. Yield: when a Move or attack-move mover's intended step is taken, the
 *     idle ally of its owner nearest that step moves SC64_ALLY_YIELD_STEP px
 *     aside: to the side of the mover's line it already stands on, else the
 *     other side, else straight ahead. The mover takes its step if that
 *     cleared it, and steers as before otherwise.
 *  2. Give up: when no step of the 8 directions is free, a Move or
 *     attack-move within SC64_MOVE_GIVE_UP_RADIUS px of its own order point
 *     ends where it stands. Farther out the engine's bounded wait and repath
 *     continue.
 *
 * Bounded: one scan of the unit table per blocked mover per update, no new
 * state. Positions and orders are the only effects, so saves need nothing.
 * Both constants are provisional: the N64 captures (T1, five Marines) end
 * every move in formation with no crowd, so they hold no blocked-mover
 * evidence. 2 px is one ordinary ground step; 12 px is two of the 6 px
 * personal-space radii, enough for a mover pressed against a resting ally
 * on its own point. */
enum { SC64_ALLY_YIELD_STEP = 2, SC64_MOVE_GIVE_UP_RADIUS = 12 };

static int
sc64_ally_yield_radius(const Unit *q, const Unit *a)
{
    int radius = 6;
#if defined(SC64_ZP_CAMPAIGN_ACTORS) || defined(SC64_EGG_COLLISION_RADIUS) || defined(SC64_GUARDIAN_DISTINCT_STAGE)
    int mover_radius = sc64_game_unit_collision_radius(q->type);
    int candidate_radius = sc64_game_unit_collision_radius(a->type);
    radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
#else
    (void)q; (void)a;
#endif
    return radius;
}

/* An idle ground ally that collides with the mover and may be moved. */
static int
sc64_ally_yield_candidate(const Unit *q, const Unit *a)
{
    if (a == q || !a->alive || a->owner != q->owner || a->order != ORDER_IDLE ||
        SC64_TRANSPORT_LOADED(a) || SC64_SCARAB_NONCOLLIDING(a) || SC64_SCARAB_CHILD(a) ||
        a->type == U_CHRYSALIS || a->type == U_SIEGE_TANK ||
        sc64_race_unit_flying(a->type) != sc64_race_unit_flying(q->type))
        return 0;
#ifdef SC64_P6_COMBAT_ENABLED
    if (race_p6_combat_unit_burrowed((int)(a - u))) return 0;
#endif
#ifdef SC64_EGG_RUNTIME
    if (a->type == SC64_EGG_ACTOR) return 0;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_unit_actor_flags(a->type) &&
        (!(sc64_game_unit_actor_flags(a->type) & SC64_GAME_ACTOR_MOVABLE) ||
         !(sc64_game_unit_actor_flags(a->type) & SC64_GAME_ACTOR_COLLIDABLE)))
        return 0;
#endif
    /* A row born earlier in this update may not be linked in the target
     * index yet; moving it would relink through its slot's stale cell. */
    if (sc64_target_index_active) {
        int cell, entity = (int)(a - u);
        if (!sc64_target_cell_for(a->x, a->y, &cell) || sc64_target_cell[entity] != cell) return 0;
    }
    return 1;
}

/* Move ally a one step aside from the mover's line q -> (nx, ny). */
static int
sc64_ally_yield_step(const Unit *q, Unit *a, int nx, int ny)
{
    int dx = nx > q->x ? 1 : nx < q->x ? -1 : 0, dy = ny > q->y ? 1 : ny < q->y ? -1 : 0;
    int cross = dx * (a->y - q->y) - dy * (a->x - q->x);
    /* The side the ally stands on first, then the other, then ahead. */
    int first = cross >= 0 ? 1 : -1;
    const int option[3][2] = {{-dy * first, dx * first}, {dy * first, -dx * first}, {dx, dy}};
    int from_cell = a->x / 16 + (a->y / 16) * MAP_COLS;
    for (int k = 0; k < 3; ++k) {
        int sx = a->x + option[k][0] * SC64_ALLY_YIELD_STEP, sy = a->y + option[k][1] * SC64_ALLY_YIELD_STEP;
        if ((!option[k][0] && !option[k][1]) || sx < 0 || sy < 0 || sx >= WORLD_W || sy >= WORLD_H ||
            !is_traversable(sx / 16, sy / 16, from_cell) || !unit_step_clear(a, sx, sy))
            continue;
        a->x = sx;
        a->y = sy;
        sc64_unit_bucket_move(a);
        sc64_target_index_move(a);
        return 1;
    }
    return 0;
}

/* The mover's intended step (nx, ny) is taken: an idle ally in it steps
 * aside. Only Move and attack-move movers push. Returns 1 when one moved. */
static int __attribute__((noinline))
sc64_move_yield(Unit *q, int nx, int ny)
{
    int best = -1, best_distance = 0x7fffffff;
    if (q->order != ORDER_MOVE && q->order != ORDER_ATTACK_MOVE) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *a = &u[i];
        if (!a->alive || a->owner != q->owner || a->order != ORDER_IDLE) continue;
        int radius = sc64_ally_yield_radius(q, a);
        if (a->x - nx >= radius || nx - a->x >= radius || a->y - ny >= radius || ny - a->y >= radius) continue;
        int d = dist2(nx, ny, a->x, a->y);
        if (d >= radius * radius || d >= best_distance || !sc64_ally_yield_candidate(q, a)) continue;
        best = i;
        best_distance = d;
    }
    return best >= 0 && sc64_ally_yield_step(q, &u[best], nx, ny);
}

/* No step toward (tx, ty) is free: a Move or attack-move near its own
 * order point ends where it stands. Returns 1 when it ended. */
static int __attribute__((noinline))
sc64_move_give_up(Unit *q, int tx, int ty)
{
    if ((q->order != ORDER_MOVE && q->order != ORDER_ATTACK_MOVE) || tx != q->tx || ty != q->ty ||
        dist2(q->x, q->y, tx, ty) > SC64_MOVE_GIVE_UP_RADIUS * SC64_MOVE_GIVE_UP_RADIUS)
        return 0;
    q->order = ORDER_IDLE;
    q->path_timer = 0;
    q->path_retry = 0;
    q->path_x = q->x;
    q->path_y = q->y;
    return 1;
}

#endif
