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
 *  1. Yield: of the idle allies of its owner that block that step (a unit
 *     the step does not move away from, as in unit_step_clear()), the
 *     nearest of up to SC64_ALLY_YIELD_TRIES that can moves
 *     SC64_ALLY_YIELD_STEP px aside: to the side of the mover's line
 *     it already stands on, else the other side, else straight ahead. The
 *     mover takes its step if that cleared it, and steers as before
 *     otherwise.
 *  2. Give up: when no step of the 8 directions is free and no ally moved,
 *     a Move within SC64_MOVE_GIVE_UP_RADIUS px of its own order point ends
 *     where it stands. Farther out the engine's bounded wait and repath
 *     continue. A zero step (the stock motion's speed this update) neither
 *     pushes nor ends an order.
 *
 * Bounded: at most two scans of the unit table per blocked mover per update,
 * no new state. Positions and orders are the only effects, so saves need
 * nothing; with no per-unit timer to save, the give-up is decided on the
 * update the mover is found boxed in, by stationary units or near-point
 * movers. The constants are provisional: the N64 captures (T1, five
 * Marines) end every move in formation with no crowd, so they hold no
 * blocked-mover evidence. 2 px is one ordinary ground step; 12 px is two of
 * the 6 px personal-space radii, enough for a mover pressed against a
 * resting ally on its own point; 16 px covers a 6 px radius plus the 3 px
 * longest probe step around the mover, with margin. */
enum { SC64_ALLY_YIELD_STEP = 2, SC64_MOVE_GIVE_UP_RADIUS = 12, SC64_MOVE_GIVE_UP_PROBE_STEPS = 3,
       SC64_MOVE_GIVE_UP_REACH = 16, SC64_MOVE_GIVE_UP_PHASES = 32, SC64_ALLY_YIELD_TRIES = 4 };

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
#ifdef SC64_SHARED_NUKE_PROBE
    if (sc64_race_ordinary_ghost(a->type) && race_nuke_ghost_locked((int)(a - u))) return 0;
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
 * yet, and its slot's cached cell and links, left by the slot's previous
 * occupants, prove nothing; sc64_target_index_move() would then relink it
 * through stale links. Move only a row found by walking its cell's chain
 * from the head (bounded by the entity count). */
static int
sc64_ally_yield_linked(const Unit *a)
{
    int entity = (int)(a - u), cell;
    if (!sc64_target_index_active) return 1;
    if ((unsigned)a->owner >= 5u || !sc64_target_cell_for(a->x, a->y, &cell) ||
        sc64_target_cell[entity] != cell) return 0;
    unsigned i = sc64_target_head[a->owner][cell];
    for (int n = 0; n < SC64_TARGET_ENTITY_COUNT && i != SC64_TARGET_NONE; ++n, i = sc64_target_next[i]) {
        if (i >= SC64_TARGET_ENTITY_COUNT) return 0;
        if ((int)i == entity) return 1;
    }
    return 0;
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
        /* Each shove must clear the step further, so shoves cannot shuttle
         * an ally between two sideways spots. */
        if ((!option[k][0] && !option[k][1]) || sx < 0 || sy < 0 || sx >= WORLD_W || sy >= WORLD_H ||
            dist2(sx, sy, nx, ny) <= dist2(a->x, a->y, nx, ny) ||
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

/* The longest step the mover's stock motion takes over the next
 * SC64_MOVE_GIVE_UP_PHASES updates in either motion mode, at most
 * SC64_MOVE_GIVE_UP_PROBE_STEPS: the give-up probes only lengths the mover
 * can use. */
static int
sc64_move_longest_step(const Unit *q)
{
    int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type), longest = 1;
    /* Both motion modes: a fresh route may pick a straight or a diagonal
     * waypoint, and ending a Move early is the worse error. */
    for (unsigned f = 0; f < SC64_MOVE_GIVE_UP_PHASES; ++f)
        for (int diagonal = 0; diagonal < 2; ++diagonal) {
#ifdef SC64_CONTACT_CACHE
            int s = (int)sc64_contact_stock_step(raw, (unsigned)frame + f, (unsigned)(q - u), diagonal);
#else
            int s = (int)sc64_stock_motion_step(raw, (unsigned)frame + f, (unsigned)(q - u), diagonal);
#endif
            if (s > longest) longest = s;
        }
    return longest < SC64_MOVE_GIVE_UP_PROBE_STEPS ? longest : SC64_MOVE_GIVE_UP_PROBE_STEPS;
}

/* The mover's intended step (nx, ny) is taken. Returns 1 when the mover
 * may take it after all (an idle ally stepped aside), else 0 and the engine
 * steers as before. A Move or attack-move mover only, with a nonzero step:
 *  1. Yield: the idle ally nearest the step that blocks it moves aside.
 *  2. Give up: with no ally moved, a Move within SC64_MOVE_GIVE_UP_RADIUS
 *     of its own point whose mover has no free step in any of the 8
 *     directions or toward its waypoint (the probes the engine's steering
 *     makes) at any length its stock motion takes, up to 3 px, and with no
 *     unit on its layer within SC64_MOVE_GIVE_UP_REACH px still travelling
 *     to a point beyond that radius, ends where it stands; the steering then finds no step either
 *     and waits. An attack-move is left alone: move_toward() also serves
 *     its chase of an enemy, away from its point. */
static int __attribute__((noinline))
sc64_move_blocked(Unit *q, int nx, int ny)
{
    static const s8 dx[8] = {1,1,0,-1,-1,-1,0,1};
    static const s8 dy[8] = {0,1,1,1,0,-1,-1,-1};
    int near[SC64_ALLY_YIELD_TRIES], near_distance[SC64_ALLY_YIELD_TRIES], count = 0;
    if ((q->order != ORDER_MOVE && q->order != ORDER_ATTACK_MOVE) || (nx == q->x && ny == q->y)) return 0;
    /* The SC64_ALLY_YIELD_TRIES nearest blockers, nearest first (slot order
     * on ties); the first that can step aside does. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *a = &u[i];
        if (!a->alive || a->owner != q->owner || a->order != ORDER_IDLE) continue;
        int radius = sc64_ally_yield_radius(q, a);
        if (a->x - nx >= radius || nx - a->x >= radius || a->y - ny >= radius || ny - a->y >= radius) continue;
        int d = dist2(nx, ny, a->x, a->y);
        /* As unit_step_clear(): a unit the step moves away from does not block. */
        if (d >= radius * radius || d > dist2(q->x, q->y, a->x, a->y) ||
            (count == SC64_ALLY_YIELD_TRIES && d >= near_distance[count - 1]) ||
            !sc64_ally_yield_candidate(q, a)) continue;
        int k = count < SC64_ALLY_YIELD_TRIES ? count++ : count - 1;
        while (k > 0 && near_distance[k - 1] > d) { near[k] = near[k - 1]; near_distance[k] = near_distance[k - 1]; --k; }
        near[k] = i;
        near_distance[k] = d;
    }
    /* An ally that moved aside may need more shoves: the mover retries. */
    for (int k = 0; k < count; ++k)
        if (sc64_ally_yield_step(q, &u[near[k]], nx, ny)) return unit_step_clear(q, nx, ny);
    if (q->order != ORDER_MOVE ||
        dist2(q->x, q->y, q->tx, q->ty) > SC64_MOVE_GIVE_UP_RADIUS * SC64_MOVE_GIVE_UP_RADIUS) return 0;
    /* The stock step alternates between lengths from update to update: an
     * order ends only when no length up to SC64_MOVE_GIVE_UP_PROBE_STEPS is
     * free in any direction. */
    int from_cell = q->x / 16 + (q->y / 16) * MAP_COLS, longest = sc64_move_longest_step(q);
    for (int speed = 1; speed <= longest; ++speed) {
        /* The waypoint step clamps each axis on its own, so it is probed too. */
        int wx = sc64_stock_motion_axis(q->x, q->path_x, (unsigned)speed);
        int wy = sc64_stock_motion_axis(q->y, q->path_y, (unsigned)speed);
        if ((wx != q->x || wy != q->y) && is_traversable(wx / 16, wy / 16, from_cell) &&
            unit_step_clear(q, wx, wy)) return 0;
        for (int k = 0; k < 8; ++k) {
            int sx = q->x + dx[k] * speed, sy = q->y + dy[k] * speed;
            if (is_traversable(sx / 16, sy / 16, from_cell) && unit_step_clear(q, sx, sy)) return 0;
        }
    }
    /* A crowd still on its way out may clear the mover next update: keep the
     * order while a unit within reach travels to a point beyond the radius.
     * Movers that are themselves near their points may all end. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *a = &u[i];
        if (a == q || !a->alive || (a->order != ORDER_MOVE && a->order != ORDER_ATTACK_MOVE) ||
            sc64_race_unit_flying(a->type) != sc64_race_unit_flying(q->type)) continue;
        if (dist2(a->x, a->y, q->x, q->y) < SC64_MOVE_GIVE_UP_REACH * SC64_MOVE_GIVE_UP_REACH &&
            dist2(a->x, a->y, a->tx, a->ty) > SC64_MOVE_GIVE_UP_RADIUS * SC64_MOVE_GIVE_UP_RADIUS) return 0;
    }
    q->order = ORDER_IDLE;
    return 0;
}

#endif
