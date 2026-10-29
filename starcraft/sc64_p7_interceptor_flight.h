/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_INTERCEPTOR_FLIGHT_H
#define SC64_P7_INTERCEPTOR_FLIGHT_H

#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
#if !defined(SC64_P7_CARRIER_TRAINING_ENABLED) || \
    !defined(SC64_P7_PROJECTILES_ENABLED)
#error "Interceptor flight requires P7 Carrier training and projectiles"
#endif
#include "sc64_interceptor_orders.h"

/* Generation 43: Carriers launch their hangar Interceptors, which fly attack
 * passes through the authored order model (sc64_interceptor_orders.c), fire
 * real moving weapon-79 projectiles and return to dock.
 *
 * Carrier (raw72), on the shared source-24 clock: a Carrier with an attack
 * target, or an attack-move/idle Carrier with an enemy within its 8-tile
 * acquisition range, stops and releases its healthiest completed inside
 * child (at least half HP) every 7 source ticks. work_timer holds that
 * release timer. Its target follows the existing attack queries.
 *
 * A launched (OUTSIDE) or orphaned (DETACHED) child keeps its order state in
 * fields the typed save codec already stores; no save bytes are added:
 *   order       ORDER_ATTACK_TARGET (attack passes) or ORDER_MOVE (return)
 *   path_retry  order state: 0/1/3/4/5 attacking, 0/1 returning
 *   work_timer  native main order timer (15-tick ascent)
 *   tx, ty      current waypoint, runtime pixels (half source precision)
 *   path_timer  attack animation: 1 fires on the next tick, 2 recovers
 *   cd          weapon cooldown, 1 + (rand & 3) - 1 source ticks
 *   carry       DETACHED only: 15-45 source ticks until removal
 *
 * Approximations, all deterministic and save-stable: random pass offsets and
 * cooldown jitter hash the saved frame counter and slot instead of the
 * native RNG stream; ranges are centre distances, not unit-box distances;
 * the muzzle heading is the current flight direction rather than a turned
 * flingy heading; movement uses the stock flyer step without acceleration;
 * a docked child is repaired to full at once. */

#define SC64_P7_CARRIER_LAUNCH_TICKS 7
#define SC64_P7_CARRIER_ACQUIRE_PX 128   /* 8 tiles, 256 source px */
#define SC64_P7_CARRIER_LEASH_PX 160     /* 8 + 2 tiles, 320 source px */
#define SC64_P7_INTERCEPTOR_WEAPON_PX 64 /* weapon 79, 128 source px */
#define SC64_P7_INTERCEPTOR_PASS_PX 25   /* 50 source px */
#define SC64_P7_INTERCEPTOR_SLOW_PX 30   /* 60 source px */
#define SC64_P7_INTERCEPTOR_DOCK_PX 5    /* 10 source px */
#define SC64_P7_INTERCEPTOR_DETACH_MIN 15
#define SC64_P7_INTERCEPTOR_DETACH_SPAN 31
#define SC64_P7_INTERCEPTOR_ELEVATION 12

enum {
    SC64_P7_INTERCEPTOR_ANIM_IDLE = 0,
    SC64_P7_INTERCEPTOR_ANIM_FIRE = 1,
    SC64_P7_INTERCEPTOR_ANIM_RECOVER = 2
};

static unsigned race_p7_flight_hash(unsigned a, unsigned b)
{
    u32 h = (u32)a * 0x9e3779b1u ^ ((u32)b + 0x7f4a7c15u) * 0x85ebca6bu;
    h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12;
    return (unsigned)h;
}

/* A value in [-127, 128], like the native lcg_rand(11, -127, 128). */
static int race_p7_flight_random(int slot, unsigned axis)
{
    return (int)(race_p7_flight_hash((unsigned)frame,
                                     (unsigned)slot * 4u + axis) & 255u) - 127;
}

static int race_p7_flight_near(int ax, int ay, int bx, int by, int px)
{
    return dist2(ax, ay, bx, by) <= px * px;
}

/* Pulse Cannon hits ground and air. raw73 keeps no generic weapon row (so
 * ordinary combat cannot apply instant damage); the -1 wildcard attacker
 * keeps the shared sight, detection, hidden and invincibility rules. */
#define SC64_P7_PULSE_CANNON_ATTACKER (-1)

static int race_p7_flight_target_ok(const Unit *q, int target)
{
    int owner;
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS || !target_alive(target))
        return 0;
    owner = target < MAX_UNITS ? u[target].owner : b[target - MAX_UNITS].owner;
    return owners_hostile(q->owner, owner) &&
        SC64_CAN_ATTACK_FROM_SOURCE(SC64_P7_PULSE_CANNON_ATTACKER, target,
                                    q->owner, q->raw_owner);
}

static int race_p7_flight_carrier_target(int carrier)
{
    const Unit *q = &u[carrier];
    if (q->order == ORDER_ATTACK_TARGET &&
        race_p7_flight_target_ok(q, q->target)) return q->target;
    if (q->order == ORDER_ATTACK_MOVE || q->order == ORDER_IDLE)
        return nearest_enemy_typed(q->x, q->y, q->owner,
                                   SC64_P7_CARRIER_ACQUIRE_PX,
                                   SC64_P7_PULSE_CANNON_ATTACKER
                                   SC64_P7_SOURCE_OWNER_ARG(q->raw_owner));
    return -1;
}

/* update_orders skips an engaged Carrier: it stops, as native stop_unit. */
static int race_p7_interceptor_flight_carrier_holds(int carrier)
{
    int target;
    if (!race_protoss_services_active() || !sc64_p7_carrier_parent_valid(carrier) ||
        !race_p7_carrier_training_child_count(carrier)) return 0;
    target = race_p7_flight_carrier_target(carrier);
    return target >= 0 &&
        race_p7_flight_near(u[carrier].x, u[carrier].y, target_x(target),
                            target_y(target), SC64_P7_CARRIER_ACQUIRE_PX);
}

/* update_orders hook. Returns 1 when this adapter moved or held the
 * Carrier. A weaponless Carrier's attack order still closes on its target,
 * then stops in acquisition range (native carrier_reaver_attack). */
static int race_p7_interceptor_flight_carrier_order(int carrier)
{
    Unit *q = &u[carrier];
    if (race_p7_interceptor_flight_carrier_holds(carrier)) return 1;
    if (q->order != ORDER_ATTACK_TARGET || !target_alive(q->target) ||
        !race_p7_carrier_training_child_count(carrier)) return 0;
    sc64_stock_flying_move(q, target_x(q->target), target_y(q->target));
    return 1;
}

static void race_p7_flight_place(Unit *q, int x, int y)
{
    q->x = x; q->y = y; q->tx = x; q->ty = y;
    sc64_unit_bucket_move(q);
    sc64_target_index_move(q);
}

/* Native release_fighter: the first completed inside child with the most
 * HP, refused below half HP. Rank 0 is the front of the inside list. */
static int race_p7_flight_release(int carrier, int target)
{
    int best = -1;
    unsigned rank;
    Unit *q;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!sc64_p7_carrier_child_valid(i) || u[i].build != carrier ||
            sc64_p7_carrier_child_state(&u[i]) != SC64_P7_INTERCEPTOR_INSIDE)
            continue;
        if (best < 0 || u[i].hp > u[best].hp ||
            (u[i].hp == u[best].hp &&
             sc64_p7_carrier_child_rank(&u[i]) <
                 sc64_p7_carrier_child_rank(&u[best]))) best = i;
    }
    if (best < 0 || u[best].hp < 20) return 0;
    rank = sc64_p7_carrier_child_rank(&u[best]);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (i != best && sc64_p7_carrier_child_valid(i) && u[i].build == carrier &&
            sc64_p7_carrier_child_state(&u[i]) == SC64_P7_INTERCEPTOR_INSIDE &&
            sc64_p7_carrier_child_rank(&u[i]) > rank)
            sc64_p7_carrier_child_set_rank(&u[i],
                                           sc64_p7_carrier_child_rank(&u[i]) - 1u);
    q = &u[best];
    sc64_p7_carrier_child_set_state(q, SC64_P7_INTERCEPTOR_OUTSIDE);
    sc64_p7_carrier_child_set_rank(q, 0);
    race_p7_flight_place(q, u[carrier].x, u[carrier].y);
    q->shield = 40; q->shield_remainder = 0;
    q->order = ORDER_ATTACK_TARGET; q->target = target;
    q->path_retry = SC64_INTERCEPTOR_ORDER_STATE_LAUNCH;
    q->work_timer = 0; q->path_timer = SC64_P7_INTERCEPTOR_ANIM_IDLE;
    q->cd = 0; q->carry = 0; q->selected = 0;
    return 1;
}

static void race_p7_flight_dock(int child)
{
    Unit *q = &u[child];
    race_p7_flight_place(q, u[q->build].x, u[q->build].y);
    q->hp = 40; q->shield = 40; q->shield_remainder = 0;
    q->carry = 0; q->cd = 0; q->target = -1; q->order = ORDER_IDLE;
    q->path_retry = 0; q->path_timer = 0; q->selected = 0;
    sc64_p7_carrier_inside_insert_front(child);
}

static void race_p7_flight_kill(int child)
{
    u[child].hp = 0;
    u[child].shield = 0;
    sc64_p7_carrier_child_remove(child, race_p7_projectile_clear_references);
}

/* Native: a launched fighter outlives its Carrier for 15-45 source ticks. */
static void race_p7_interceptor_flight_detach(int child)
{
    Unit *q = &u[child];
    q->build = -1;
    sc64_p7_carrier_child_set_state(q, SC64_P7_INTERCEPTOR_DETACHED);
    sc64_p7_carrier_child_set_rank(q, 0);
    q->carry = SC64_P7_INTERCEPTOR_DETACH_MIN +
        (int)(race_p7_flight_hash((unsigned)frame, (unsigned)child + 0x200u) %
              SC64_P7_INTERCEPTOR_DETACH_SPAN);
}

/* Weapon event: launch a moving projectile; damage arrives on impact. */
static void race_p7_flight_fire(int child, int target)
{
    const Unit *q = &u[child];
    int dx = (q->tx - q->x) * 2, dy = (q->ty - q->y) * 2;
    u8 heading = 0;
    if (!dx && !dy) {
        dx = (target_x(target) - q->x) * 2;
        dy = (target_y(target) - q->y) * 2;
    }
    if (!sc64_interceptor_direction_for_delta(dx, dy, &heading)) heading = 0;
    (void)race_p7_projectile_fire(child, target, heading,
                                  (s32)q->x * 512, (s32)q->y * 512);
}

static int race_p7_flight_clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static void race_p7_flight_child_tick(int child)
{
    Unit *q = &u[child];
    unsigned state = sc64_p7_carrier_child_state(q);
    int parent = state == SC64_P7_INTERCEPTOR_OUTSIDE ? q->build : -1;
    int parent_ok = parent >= 0 && parent < MAX_UNITS &&
        sc64_p7_carrier_parent_valid(parent) &&
        u[parent].owner == q->owner && u[parent].raw_owner == q->raw_owner;
    int target = q->target, found = 0, tx, ty;
    int width = race_cols * 32, height = race_rows * 32;
    Sc64InterceptorOrder order;
    Sc64InterceptorOrderInput in;
    Sc64InterceptorOrderResult result;

    if (state == SC64_P7_INTERCEPTOR_DETACHED) {
        if (--q->carry <= 0) { race_p7_flight_kill(child); return; }
    } else if (!parent_ok) {
        race_p7_interceptor_flight_detach(child);
        return;
    }
    /* Attack IScript: wait(1) is consumed when the animation starts,
     * attackwith(1) runs on the next tick and the lock clears one later. */
    if (q->path_timer == SC64_P7_INTERCEPTOR_ANIM_FIRE) {
        if (race_p7_flight_target_ok(q, target)) race_p7_flight_fire(child, target);
        q->path_timer = SC64_P7_INTERCEPTOR_ANIM_RECOVER;
    } else if (q->path_timer == SC64_P7_INTERCEPTOR_ANIM_RECOVER) {
        q->path_timer = SC64_P7_INTERCEPTOR_ANIM_IDLE;
    }
    if (q->cd > 0) --q->cd;
    if (q->work_timer > 0) --q->work_timer;

    memset(&order, 0, sizeof(order));
    memset(&in, 0, sizeof(in));
    order.order = q->order == ORDER_MOVE ? SC64_INTERCEPTOR_ORDER_RETURN :
                                           SC64_INTERCEPTOR_ORDER_ATTACK;
    order.order_state = (uint8_t)q->path_retry;
    order.alive = 1; order.outside = 1;
    order.timer = (uint16_t)q->work_timer;
    order.elevation = SC64_P7_INTERCEPTOR_ELEVATION;
    order.shields_q8 = (uint32_t)race_p7_flight_clamp(q->shield, 0, 40) * 256u;
    order.max_shields_q8 = 40u * 256u;
    order.move_x = (int16_t)(q->tx * 2); order.move_y = (int16_t)(q->ty * 2);

    in.target_present = (uint8_t)race_p7_flight_target_ok(q, target);
    if (!in.target_present) {
        int candidate = parent_ok ? race_p7_flight_carrier_target(parent) : -1;
        if (!race_p7_flight_target_ok(q, candidate))
            candidate = nearest_enemy_typed(q->x, q->y, q->owner,
                                            SC64_P7_CARRIER_ACQUIRE_PX,
                                            SC64_P7_PULSE_CANNON_ATTACKER
                                            SC64_P7_SOURCE_OWNER_ARG(q->raw_owner));
        if (race_p7_flight_target_ok(q, candidate)) { target = candidate; found = 1; }
        else target = -1;
    }
    in.autoattack_found_target = (uint8_t)found;
    tx = target >= 0 ? target_x(target) : q->x;
    ty = target >= 0 ? target_y(target) : q->y;
    in.parent_alive = (uint8_t)parent_ok;
    in.parent_leash_ok = (uint8_t)(parent_ok && target >= 0 &&
        race_p7_flight_near(u[parent].x, u[parent].y, tx, ty,
                            SC64_P7_CARRIER_LEASH_PX));
    in.weapon_range_ok = (uint8_t)(target >= 0 &&
        race_p7_flight_near(q->x, q->y, tx, ty, SC64_P7_INTERCEPTOR_WEAPON_PX));
    in.target_near_50 = (uint8_t)(target >= 0 &&
        race_p7_flight_near(q->x, q->y, tx, ty, SC64_P7_INTERCEPTOR_PASS_PX));
    in.waypoint_near_50 = (uint8_t)race_p7_flight_near(q->x, q->y, q->tx, q->ty,
        SC64_P7_INTERCEPTOR_PASS_PX);
    if (parent_ok) {
        in.parent_distance_le_60 = (uint8_t)race_p7_flight_near(q->x, q->y,
            u[parent].x, u[parent].y, SC64_P7_INTERCEPTOR_SLOW_PX);
        in.parent_distance_le_10 = (uint8_t)race_p7_flight_near(q->x, q->y,
            u[parent].x, u[parent].y, SC64_P7_INTERCEPTOR_DOCK_PX);
        in.parent_x = (int16_t)(u[parent].x * 2);
        in.parent_y = (int16_t)(u[parent].y * 2);
    }
    in.parent_elevation = SC64_P7_INTERCEPTOR_ELEVATION;
    in.target_x = (int16_t)(tx * 2); in.target_y = (int16_t)(ty * 2);
    /* Interceptor dimensions are 8/8/7/7 source px (BWAPI raw73). */
    in.move_min_x = 8; in.move_max_x = (int16_t)(width - 8);
    in.move_min_y = 8; in.move_max_y = (int16_t)(height - 8);
    in.reflect_min_x = 8; in.reflect_max_x = (int16_t)(width - 7);
    in.reflect_min_y = 8; in.reflect_max_y = (int16_t)(height - 7);
    in.random_x = (int16_t)race_p7_flight_random(child, 0);
    in.random_y = (int16_t)race_p7_flight_random(child, 1);

    if (!sc64_interceptor_order_tick(&order, &in, &result)) {
        /* Unrepresentable input: head home rather than act on it. */
        q->order = ORDER_MOVE; q->path_retry = 0; q->target = -1;
        return;
    }
    if (result.actions & SC64_INTERCEPTOR_ACTION_KILL) {
        race_p7_flight_kill(child);
        return;
    }
    if (result.actions & SC64_INTERCEPTOR_ACTION_RELINK_INSIDE) {
        if (parent_ok) race_p7_flight_dock(child);
        else race_p7_flight_kill(child);
        return;
    }
    q->order = order.order == SC64_INTERCEPTOR_ORDER_RETURN ? ORDER_MOVE :
                                                              ORDER_ATTACK_TARGET;
    q->path_retry = order.order_state;
    q->work_timer = order.timer;
    q->target = order.order == SC64_INTERCEPTOR_ORDER_ATTACK ? target : -1;
    q->tx = race_p7_flight_clamp(order.move_x / 2, 0, WORLD_W - 1);
    q->ty = race_p7_flight_clamp(order.move_y / 2, 0, WORLD_H - 1);
    if ((result.events & SC64_INTERCEPTOR_EVENT_FIRE_REQUEST) &&
        q->path_timer == SC64_P7_INTERCEPTOR_ANIM_IDLE && !q->cd &&
        race_p7_flight_target_ok(q, target)) {
        /* Native: modified cooldown (1) + (rand & 3) - 1. */
        q->cd = (int)(race_p7_flight_hash((unsigned)frame,
                                          (unsigned)child + 0x400u) & 3u);
        q->path_timer = SC64_P7_INTERCEPTOR_ANIM_FIRE;
    }
}

static void race_p7_flight_carrier_tick(int carrier)
{
    Unit *q = &u[carrier];
    int target;
    if (q->work_timer > 0) --q->work_timer;
    if (q->work_timer || !race_p7_interceptor_flight_carrier_holds(carrier)) return;
    target = race_p7_flight_carrier_target(carrier);
    if (target >= 0 && race_p7_flight_release(carrier, target))
        q->work_timer = SC64_P7_CARRIER_LAUNCH_TICKS;
}

/* Shared source-24 tick, after training. Children act before Carriers so a
 * newly released child starts its order on the next source tick. */
static void race_p7_interceptor_flight_source_tick(void)
{
    if (!race_protoss_services_active()) return;
    for (int i = 0; i < MAX_UNITS; ++i) {
        unsigned state;
        if (!u[i].alive || u[i].type != U_P7_INTERCEPTOR ||
            u[i].raw_type != SC64_P7_INTERCEPTOR_RAW) continue;
        state = sc64_p7_carrier_child_state(&u[i]);
        if (state == SC64_P7_INTERCEPTOR_OUTSIDE ||
            state == SC64_P7_INTERCEPTOR_DETACHED)
            race_p7_flight_child_tick(i);
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (sc64_p7_carrier_parent_valid(i)) race_p7_flight_carrier_tick(i);
}

/* Every 30 Hz update: fly toward the current waypoint. */
static void race_p7_interceptor_flight_frame_move(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        unsigned state;
        if (!q->alive || q->type != U_P7_INTERCEPTOR) continue;
        state = sc64_p7_carrier_child_state(q);
        if ((state == SC64_P7_INTERCEPTOR_OUTSIDE ||
             state == SC64_P7_INTERCEPTOR_DETACHED) &&
            (q->x != q->tx || q->y != q->ty))
            sc64_stock_flying_move(q, q->tx, q->ty);
    }
}

/* Save/load invariants for launched and detached children. */
static int race_p7_interceptor_flight_child_valid_in(const Unit *actors,
                                                     int count, int slot)
{
    const Unit *q = &actors[slot];
    unsigned state = sc64_p7_carrier_child_state(q);
    if (!sc64_p7_carrier_child_valid_in(actors, count, slot) ||
        q->maxhp != 40 || q->hp < 1 || q->hp > 40 ||
        q->shield < 0 || q->shield > 40 || q->selected ||
        sc64_p7_carrier_child_rank(q) || q->work_timer > 15 ||
        q->path_timer > SC64_P7_INTERCEPTOR_ANIM_RECOVER || q->cd > 3)
        return 0;
    if (q->order == ORDER_ATTACK_TARGET) {
        if (q->path_retry == 2 || q->path_retry > 5) return 0;
    } else if (q->order == ORDER_MOVE) {
        if (q->path_retry > 1) return 0;
    } else return 0;
    if (state == SC64_P7_INTERCEPTOR_OUTSIDE)
        return q->carry == 0 && q->build >= 0 && q->build < count &&
            sc64_p7_carrier_parent_valid_in(actors, count, q->build) &&
            actors[q->build].owner == q->owner &&
            actors[q->build].raw_owner == q->raw_owner;
    return state == SC64_P7_INTERCEPTOR_DETACHED && q->build == -1 &&
        q->carry >= 1 &&
        q->carry < SC64_P7_INTERCEPTOR_DETACH_MIN + SC64_P7_INTERCEPTOR_DETACH_SPAN;
}

#endif /* SC64_P7_INTERCEPTOR_FLIGHT_ENABLED */
#endif /* SC64_P7_INTERCEPTOR_FLIGHT_H */
