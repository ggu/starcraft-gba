/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_INTERCEPTOR_ORDERS_H
#define SC64_INTERCEPTOR_ORDERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Protoss-Interceptor-only decision model of the observed InterceptorAttack and
 * InterceptorReturn order branches. It does not simulate movement, firing,
 * animation, target acquisition, or carrier cadence. The caller supplies
 * geometric predicates and source RNG samples, and reacquires only when the
 * pre-reacquisition target is absent or invincible (after the shield gate). */
enum {
    SC64_INTERCEPTOR_ORDER_ATTACK = 0,
    SC64_INTERCEPTOR_ORDER_RETURN = 1
};

enum {
    SC64_INTERCEPTOR_ORDER_STATE_LAUNCH = 0,
    SC64_INTERCEPTOR_ORDER_STATE_ASCEND = 1,
    SC64_INTERCEPTOR_ORDER_STATE_ATTACK = 3,
    SC64_INTERCEPTOR_ORDER_STATE_STRAFE = 4,
    SC64_INTERCEPTOR_ORDER_STATE_RETURN_STRAFE = 5
};

enum {
    SC64_INTERCEPTOR_EVENT_FIRE_REQUEST = 1u << 0,
    SC64_INTERCEPTOR_EVENT_SOUND_616 = 1u << 1
};

enum {
    SC64_INTERCEPTOR_ACTION_MOVE_TO_TARGET = 1u << 0,
    SC64_INTERCEPTOR_ACTION_MOVE_TO_OFFSET = 1u << 1,
    SC64_INTERCEPTOR_ACTION_RETURN_ORDER = 1u << 2,
    SC64_INTERCEPTOR_ACTION_KILL = 1u << 3,
    SC64_INTERCEPTOR_ACTION_RELINK_INSIDE = 1u << 4,
    SC64_INTERCEPTOR_ACTION_HIDE = 1u << 5,
    SC64_INTERCEPTOR_ACTION_NOTHING_ORDER = 1u << 6
};

typedef struct {
    uint8_t order;       /* ATTACK or RETURN */
    uint8_t order_state; /* 0, 1, 3, 4, or 5 */
    uint8_t alive;
    uint8_t outside;
    uint16_t timer;      /* caller advances the native order timer */
    int16_t elevation;
    uint32_t shields_q8;     /* nonnegative shield value in source Q8 units */
    uint32_t max_shields_q8; /* quarter-shield comparison preserves fractions */
    int16_t target_x, target_y;
    int16_t move_x, move_y;
} Sc64InterceptorOrder;

typedef struct {
    uint8_t parent_alive;
    uint8_t target_present; /* describes target before reacquisition */
    uint8_t target_invincible; /* describes target before reacquisition */
    uint8_t autoattack_found_target; /* used only when absent or invincible */
    uint8_t parent_leash_ok;       /* parent target range plus movement range */
    uint8_t weapon_range_ok;       /* weapon movement range predicate */
    uint8_t target_near_50;        /* target within 50 + its movement range */
    uint8_t waypoint_near_50;      /* interceptor waypoint distance <= 50 */
    uint8_t parent_distance_le_60; /* integer source-pixel distance */
    uint8_t parent_distance_le_10; /* integer source-pixel distance */
    int16_t target_x, target_y; /* selected target; replacement on reacquire */
    int16_t parent_elevation;
    int16_t parent_x, parent_y;
    /* Final legal center bounds after the caller's unit-bounds restriction. */
    int16_t move_min_x, move_max_x;
    int16_t move_min_y, move_max_y;
    /* Source random-offset reflection thresholds, half-open and adjusted for
     * map dimensions plus the Interceptor's authored dimensions. */
    int16_t reflect_min_x, reflect_max_x;
    int16_t reflect_min_y, reflect_max_y;
    /* Caller-sampled native RNG values, each in [-127, 128]. */
    int16_t random_x, random_y;
} Sc64InterceptorOrderInput;

typedef struct {
    uint16_t events;
    uint16_t actions;
} Sc64InterceptorOrderResult;

/* On invalid input, returns 0 without modifying order or result. On valid
 * input, applies one source-order decision tick and returns 1. Movement and
 * combat effects are represented only as result flags and destination fields
 * in order. Distances/range predicates, RNG, timer progression, and effect
 * execution remain the caller's responsibility. */
int sc64_interceptor_order_tick(Sc64InterceptorOrder *order,
                                const Sc64InterceptorOrderInput *input,
                                Sc64InterceptorOrderResult *result);

#ifdef __cplusplus
}
#endif
#endif
