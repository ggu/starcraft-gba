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
 * When a Move or attack-move mover's intended nonzero step is taken,
 * move_toward() calls sc64_move_blocked() (one out-of-line call: move_toward
 * is IWRAM code):
 *  1. Yield: the idle ally of its owner nearest that step that blocks it
 *     (a unit the step does not move away from, as in unit_step_clear())
 *     moves SC64_ALLY_YIELD_STEP px aside: to the side of the mover's line
 *     it already stands on, else the other side, else straight ahead. The
 *     mover takes its step if that cleared it, and steers as before
 *     otherwise.
 *  2. Give up: when no step of the 8 directions is free, a Move or
 *     attack-move within SC64_MOVE_GIVE_UP_RADIUS px of its own order point
 *     ends where it stands. Farther out the engine's bounded wait and repath
 *     continue. A zero step (the stock motion's speed this update) neither
 *     pushes nor ends an order.
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
#ifdef SC64_T6_RUNTIME
    if (race_t6_active() && race_t6_unit_burrowed((int)(a - u))) return 0;
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
    return 1;
}

/* A row born earlier in this update may not be linked in the target index
 * yet, and its slot's cached cell, left by the slot's previous occupant,
 * proves nothing; sc64_target_index_move() would then relink it through
 * stale links. Move only a row that is in its cell's chain: the chain's head,
 * or the successor of a live row of the same owner in the same cell. */
static int
sc64_ally_yield_linked(const Unit *a)
{
    int entity = (int)(a - u), cell, prev;
    if (!sc64_target_index_active) return 1;
    if ((unsigned)a->owner >= 5u || !sc64_target_cell_for(a->x, a->y, &cell) ||
        sc64_target_cell[entity] != cell) return 0;
    prev = sc64_target_prev[entity];
    if (prev == SC64_TARGET_NONE) return sc64_target_head[a->owner][cell] == entity;
    return prev < MAX_UNITS && u[prev].alive && u[prev].owner == a->owner &&
           sc64_target_cell[prev] == cell && sc64_target_next[prev] == entity;
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
    if (!sc64_ally_yield_linked(a)) return 0;
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

/* The mover's intended step (nx, ny) is taken. Returns 1 when the mover
 * may take it after all (an idle ally stepped aside), else 0 and the engine
 * steers as before. A Move or attack-move mover only, with a nonzero step:
 *  1. Yield: the idle ally nearest the step that blocks it moves aside.
 *  2. Give up: with no ally moved, an order within SC64_MOVE_GIVE_UP_RADIUS
 *     of its own point whose mover has no free step in any of the 8
 *     directions (the probes the engine's steering makes) ends where it
 *     stands; the steering then finds no step either and waits. */
static int __attribute__((noinline))
sc64_move_blocked(Unit *q, int nx, int ny)
{
    static const s8 dx[8] = {1,1,0,-1,-1,-1,0,1};
    static const s8 dy[8] = {0,1,1,1,0,-1,-1,-1};
    int best = -1, best_distance = 0x7fffffff;
    int speed = FAST_ABS(nx - q->x) > FAST_ABS(ny - q->y) ? FAST_ABS(nx - q->x) : FAST_ABS(ny - q->y);
    if ((q->order != ORDER_MOVE && q->order != ORDER_ATTACK_MOVE) || !speed) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *a = &u[i];
        if (!a->alive || a->owner != q->owner || a->order != ORDER_IDLE) continue;
        int radius = sc64_ally_yield_radius(q, a);
        if (a->x - nx >= radius || nx - a->x >= radius || a->y - ny >= radius || ny - a->y >= radius) continue;
        int d = dist2(nx, ny, a->x, a->y);
        /* As unit_step_clear(): a unit the step moves away from does not block. */
        if (d >= radius * radius || d >= best_distance || d > dist2(q->x, q->y, a->x, a->y) ||
            !sc64_ally_yield_candidate(q, a)) continue;
        best = i;
        best_distance = d;
    }
    if (best >= 0 && sc64_ally_yield_step(q, &u[best], nx, ny) && unit_step_clear(q, nx, ny)) return 1;
    if (dist2(q->x, q->y, q->tx, q->ty) > SC64_MOVE_GIVE_UP_RADIUS * SC64_MOVE_GIVE_UP_RADIUS) return 0;
    int from_cell = q->x / 16 + (q->y / 16) * MAP_COLS;
    for (int k = 0; k < 8; ++k) {
        int sx = q->x + dx[k] * speed, sy = q->y + dy[k] * speed;
        if (is_traversable(sx / 16, sy / 16, from_cell) && unit_step_clear(q, sx, sy)) return 0;
    }
    q->order = ORDER_IDLE;
    return 0;
}

#endif
