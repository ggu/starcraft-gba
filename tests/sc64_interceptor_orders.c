/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_interceptor_orders.h"

static Sc64InterceptorOrderInput input_default(void)
{
    Sc64InterceptorOrderInput in;
    memset(&in, 0, sizeof in);
    in.parent_alive = 1;
    in.target_present = 1;
    in.parent_leash_ok = 1;
    in.target_x = 200;
    in.target_y = 300;
    in.parent_x = 20;
    in.parent_y = 30;
    in.move_min_x = 0;
    in.move_max_x = 511;
    in.move_min_y = 0;
    in.move_max_y = 511;
    in.reflect_min_x = 0;
    in.reflect_max_x = 512;
    in.reflect_min_y = 0;
    in.reflect_max_y = 512;
    return in;
}

static Sc64InterceptorOrder order_default(uint8_t order, uint8_t state)
{
    Sc64InterceptorOrder o;
    memset(&o, 0, sizeof o);
    o.order = order;
    o.order_state = state;
    o.alive = 1;
    o.outside = 1;
    o.shields_q8 = 10u * 256u;
    o.max_shields_q8 = 40u * 256u;
    return o;
}

static void launch_and_timer_contract(void)
{
    Sc64InterceptorOrder o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 0);
    Sc64InterceptorOrderResult r;
    Sc64InterceptorOrderInput in = input_default();
    in.random_x = 128;
    in.random_y = -127;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order_state == 1 && o.timer == 15 && o.elevation == 0);
    assert(o.move_x == 0 && o.move_y == 511); /* reflected then bounded */
    assert(r.events == SC64_INTERCEPTOR_EVENT_SOUND_616);
    assert(r.actions == SC64_INTERCEPTOR_ACTION_MOVE_TO_OFFSET);

    /* Timer progression belongs to the caller. The order callback waits while
     * nonzero, then transitions and executes the state-3 branch on that tick. */
    in = input_default();
    o.timer = 1;
    in.weapon_range_ok = 1;
    in.target_near_50 = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.timer == 1 && o.order_state == 1 && o.elevation == 0);
    o.timer = 0;
    in.target_near_50 = 1;
    in.random_x = 5;
    in.random_y = -7;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order_state == 4 && o.elevation == 2);
    assert(o.move_x == 205 && o.move_y == 293);
    assert(r.events == SC64_INTERCEPTOR_EVENT_FIRE_REQUEST);
    assert(r.actions == SC64_INTERCEPTOR_ACTION_MOVE_TO_OFFSET);
}

static void attack_state_matrix(void)
{
    Sc64InterceptorOrder o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 3);
    Sc64InterceptorOrderResult r;
    Sc64InterceptorOrderInput in = input_default();
    in.weapon_range_ok = 1;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(r.events == SC64_INTERCEPTOR_EVENT_FIRE_REQUEST);
    assert(r.actions == SC64_INTERCEPTOR_ACTION_MOVE_TO_TARGET);
    assert(o.move_x == in.target_x && o.move_y == in.target_y &&
           o.order_state == 3);

    o.order_state = 4;
    in.waypoint_near_50 = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(r.events == 0 && r.actions == 0 && o.order_state == 4);
    in.waypoint_near_50 = 1;
    in.random_x = -2;
    in.random_y = 3;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order_state == 5 && o.move_x == 196 && o.move_y == 306);
    assert(r.actions == SC64_INTERCEPTOR_ACTION_MOVE_TO_OFFSET);

    in.waypoint_near_50 = 1;
    o.order_state = 5;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order_state == 3 && o.move_x == in.target_x &&
           o.move_y == in.target_y);
    assert(r.actions == SC64_INTERCEPTOR_ACTION_MOVE_TO_TARGET);

    /* The bottom 32-pixel band belongs to final unit-bounds restriction,
     * after random-offset reflection against the map and sprite dimensions. */
    o.order_state = 3;
    in.target_y = 500;
    in.move_max_y = 479;
    in.target_near_50 = 1;
    in.random_y = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.move_y == 479);
}

static void early_return_priority_and_q8_threshold(void)
{
    Sc64InterceptorOrder o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 0);
    Sc64InterceptorOrderResult r;
    Sc64InterceptorOrderInput in = input_default();
    o.shields_q8 = 40u * 256u / 4u - 1u;
    in.target_present = 0;
    in.parent_leash_ok = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order == SC64_INTERCEPTOR_ORDER_RETURN && o.order_state == 0);
    assert(r.actions == SC64_INTERCEPTOR_ACTION_RETURN_ORDER);

    o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 3);
    o.shields_q8 = 40u * 256u / 4u;
    in = input_default();
    in.target_present = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order == SC64_INTERCEPTOR_ORDER_RETURN); /* exact quarter survives */

    o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 3);
    in = input_default();
    in.target_present = 0;
    in.autoattack_found_target = 1;
    in.target_x = 400;
    in.target_y = 401;
    in.target_near_50 = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order == SC64_INTERCEPTOR_ORDER_ATTACK && o.target_x == 400 &&
           o.target_y == 401 && o.move_x == 400);

    o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 3);
    in = input_default();
    in.target_invincible = 1;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order == SC64_INTERCEPTOR_ORDER_RETURN &&
           r.actions == SC64_INTERCEPTOR_ACTION_RETURN_ORDER);

    o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 3);
    in = input_default();
    in.parent_leash_ok = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order == SC64_INTERCEPTOR_ORDER_RETURN &&
           r.actions == SC64_INTERCEPTOR_ACTION_RETURN_ORDER);

    o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 0);
    in = input_default();
    in.parent_alive = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(!o.alive && r.actions == SC64_INTERCEPTOR_ACTION_KILL);
}

static void return_boundaries(void)
{
    Sc64InterceptorOrder o = order_default(SC64_INTERCEPTOR_ORDER_RETURN, 0);
    Sc64InterceptorOrderResult r;
    Sc64InterceptorOrderInput in = input_default();
    in.parent_distance_le_60 = 1;
    in.parent_distance_le_10 = 1;
    in.parent_elevation = 12;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order_state == 1 && o.elevation == 10 && !o.outside);
    assert(r.actions == (SC64_INTERCEPTOR_ACTION_RELINK_INSIDE |
                         SC64_INTERCEPTOR_ACTION_HIDE |
                         SC64_INTERCEPTOR_ACTION_NOTHING_ORDER));

    o = order_default(SC64_INTERCEPTOR_ORDER_RETURN, 0);
    in = input_default();
    in.parent_distance_le_60 = 1;
    in.parent_distance_le_10 = 0;
    in.parent_elevation = 12;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(o.order_state == 1 && o.elevation == 10 && o.outside);
    assert(o.move_x == in.parent_x && o.move_y == in.parent_y);
    assert(r.actions == SC64_INTERCEPTOR_ACTION_MOVE_TO_TARGET);

    in.parent_alive = 0;
    assert(sc64_interceptor_order_tick(&o, &in, &r));
    assert(!o.alive && r.actions == SC64_INTERCEPTOR_ACTION_KILL);
}

static void invalid_is_transactional(void)
{
    Sc64InterceptorOrder o = order_default(SC64_INTERCEPTOR_ORDER_ATTACK, 3), before;
    Sc64InterceptorOrderResult r = {0x1234, 0x5678}, before_r = r;
    Sc64InterceptorOrderInput in = input_default();
    before = o;
    in.random_x = 129;
    assert(!sc64_interceptor_order_tick(&o, &in, &r));
    assert(memcmp(&o, &before, sizeof o) == 0);
    assert(memcmp(&r, &before_r, sizeof r) == 0);
    in = input_default();
    in.parent_distance_le_10 = 1; /* impossible nested threshold facts */
    assert(!sc64_interceptor_order_tick(&o, &in, &r));
    assert(memcmp(&o, &before, sizeof o) == 0);
    assert(memcmp(&r, &before_r, sizeof r) == 0);
    in = input_default();
    o.order_state = 2;
    assert(!sc64_interceptor_order_tick(&o, &in, &r));
}

int main(void)
{
    launch_and_timer_contract();
    attack_state_matrix();
    early_return_priority_and_q8_threshold();
    return_boundaries();
    invalid_is_transactional();
    puts("PASS Interceptor attack/return order decisions");
    return 0;
}
