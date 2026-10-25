/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sc64_interceptor_orders.h"

static int sc64_interceptor_order_bool(uint8_t value)
{
    return value <= 1;
}

static int sc64_interceptor_order_valid(const Sc64InterceptorOrder *order,
                                        const Sc64InterceptorOrderInput *in)
{
    if (!order || !in || order->order > SC64_INTERCEPTOR_ORDER_RETURN ||
        !sc64_interceptor_order_bool(order->alive) || !order->alive ||
        !sc64_interceptor_order_bool(order->outside) ||
        !sc64_interceptor_order_bool(in->parent_alive) ||
        !sc64_interceptor_order_bool(in->target_present) ||
        !sc64_interceptor_order_bool(in->target_invincible) ||
        !sc64_interceptor_order_bool(in->autoattack_found_target) ||
        !sc64_interceptor_order_bool(in->parent_leash_ok) ||
        !sc64_interceptor_order_bool(in->weapon_range_ok) ||
        !sc64_interceptor_order_bool(in->target_near_50) ||
        !sc64_interceptor_order_bool(in->waypoint_near_50) ||
        !sc64_interceptor_order_bool(in->parent_distance_le_60) ||
        !sc64_interceptor_order_bool(in->parent_distance_le_10) ||
        in->move_min_x > in->move_max_x || in->move_min_y > in->move_max_y ||
        in->reflect_min_x >= in->reflect_max_x ||
        in->reflect_min_y >= in->reflect_max_y ||
        in->target_x < -8192 || in->target_x > 8191 ||
        in->target_y < -8192 || in->target_y > 8191 ||
        in->parent_x < -8192 || in->parent_x > 8191 ||
        in->parent_y < -8192 || in->parent_y > 8191 ||
        in->move_min_x < -8192 || in->move_max_x > 8191 ||
        in->move_min_y < -8192 || in->move_max_y > 8191 ||
        in->reflect_min_x < -8192 || in->reflect_max_x > 8192 ||
        in->reflect_min_y < -8192 || in->reflect_max_y > 8192 ||
        (in->parent_distance_le_10 && !in->parent_distance_le_60) ||
        order->shields_q8 > order->max_shields_q8 ||
        in->random_x < -127 || in->random_x > 128 ||
        in->random_y < -127 || in->random_y > 128)
        return 0;
    if (order->order == SC64_INTERCEPTOR_ORDER_ATTACK)
        return order->order_state == SC64_INTERCEPTOR_ORDER_STATE_LAUNCH ||
               order->order_state == SC64_INTERCEPTOR_ORDER_STATE_ASCEND ||
               order->order_state == SC64_INTERCEPTOR_ORDER_STATE_ATTACK ||
               order->order_state == SC64_INTERCEPTOR_ORDER_STATE_STRAFE ||
               order->order_state == SC64_INTERCEPTOR_ORDER_STATE_RETURN_STRAFE;
    return order->order_state <= 1;
}

static int16_t sc64_interceptor_order_clamp(int32_t v, int16_t lo, int16_t hi)
{
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return (int16_t)v;
}

static void sc64_interceptor_order_offset(Sc64InterceptorOrder *order,
                                          const Sc64InterceptorOrderInput *in,
                                          int multiplier,
                                          Sc64InterceptorOrderResult *result)
{
    int32_t x = (int32_t)in->target_x + (int32_t)in->random_x * multiplier;
    int32_t y = (int32_t)in->target_y + (int32_t)in->random_y * multiplier;
    /* Native movement setup mirrors an out-of-bounds random offset back
     * across the target, then calls the caller's bounded-target equivalent. */
    if (x < in->reflect_min_x || x >= in->reflect_max_x)
        x = (int32_t)in->target_x - (int32_t)in->random_x * multiplier;
    if (y < in->reflect_min_y || y >= in->reflect_max_y)
        y = (int32_t)in->target_y - (int32_t)in->random_y * multiplier;
    order->move_x = sc64_interceptor_order_clamp(x, in->move_min_x,
                                                 in->move_max_x);
    order->move_y = sc64_interceptor_order_clamp(y, in->move_min_y,
                                                 in->move_max_y);
    result->actions |= SC64_INTERCEPTOR_ACTION_MOVE_TO_OFFSET;
}

int sc64_interceptor_order_tick(Sc64InterceptorOrder *order,
                                const Sc64InterceptorOrderInput *input,
                                Sc64InterceptorOrderResult *result)
{
    Sc64InterceptorOrder next;
    Sc64InterceptorOrderResult out = {0, 0};
    if (!result || !sc64_interceptor_order_valid(order, input)) return 0;
    next = *order;

    if (next.order == SC64_INTERCEPTOR_ORDER_ATTACK) {
        /* Native order checks shield, then target/reacquisition, then the
         * parent leash before executing any order-state branch. */
        if (input->parent_alive &&
            next.shields_q8 < next.max_shields_q8 / 4u) {
            next.order = SC64_INTERCEPTOR_ORDER_RETURN;
            next.order_state = 0;
            out.actions = SC64_INTERCEPTOR_ACTION_RETURN_ORDER;
            goto commit;
        }
        if ((!input->target_present || input->target_invincible) &&
            !input->autoattack_found_target) {
            next.order = SC64_INTERCEPTOR_ORDER_RETURN;
            next.order_state = 0;
            out.actions = SC64_INTERCEPTOR_ACTION_RETURN_ORDER;
            goto commit;
        }
        /* A successful caller-owned reacquisition supplies the replacement
         * target coordinates used by the remaining decisions. */
        if (input->parent_alive && !input->parent_leash_ok) {
            next.order = SC64_INTERCEPTOR_ORDER_RETURN;
            next.order_state = 0;
            out.actions = SC64_INTERCEPTOR_ACTION_RETURN_ORDER;
            goto commit;
        }
        next.target_x = input->target_x;
        next.target_y = input->target_y;

        if (next.order_state == SC64_INTERCEPTOR_ORDER_STATE_LAUNCH) {
            if (!input->parent_alive) {
                next.alive = 0;
                out.actions = SC64_INTERCEPTOR_ACTION_KILL;
                goto commit;
            }
            sc64_interceptor_order_offset(&next, input, 3, &out);
            next.timer = 15;
            next.order_state = SC64_INTERCEPTOR_ORDER_STATE_ASCEND;
            out.events = SC64_INTERCEPTOR_EVENT_SOUND_616;
            goto commit;
        }
        if (next.order_state == SC64_INTERCEPTOR_ORDER_STATE_ASCEND) {
            if (next.timer != 0) goto commit;
            if (next.elevation > INT16_MAX - 2) return 0;
            next.elevation += 2;
            next.order_state = SC64_INTERCEPTOR_ORDER_STATE_ATTACK;
        }
        if (next.order_state == SC64_INTERCEPTOR_ORDER_STATE_ATTACK) {
            if (input->weapon_range_ok)
                out.events |= SC64_INTERCEPTOR_EVENT_FIRE_REQUEST;
            if (input->target_near_50) {
                sc64_interceptor_order_offset(&next, input, 1, &out);
                next.order_state = SC64_INTERCEPTOR_ORDER_STATE_STRAFE;
            } else {
                next.move_x = input->target_x;
                next.move_y = input->target_y;
                out.actions |= SC64_INTERCEPTOR_ACTION_MOVE_TO_TARGET;
            }
        } else if (next.order_state == SC64_INTERCEPTOR_ORDER_STATE_STRAFE) {
            if (input->waypoint_near_50) {
                sc64_interceptor_order_offset(&next, input, 2, &out);
                next.order_state = SC64_INTERCEPTOR_ORDER_STATE_RETURN_STRAFE;
            }
        } else if (next.order_state == SC64_INTERCEPTOR_ORDER_STATE_RETURN_STRAFE) {
            if (input->waypoint_near_50) {
                next.move_x = input->target_x;
                next.move_y = input->target_y;
                next.order_state = SC64_INTERCEPTOR_ORDER_STATE_ATTACK;
                out.actions |= SC64_INTERCEPTOR_ACTION_MOVE_TO_TARGET;
            }
        }
    } else {
        if (!input->parent_alive) {
            next.alive = 0;
            out.actions = SC64_INTERCEPTOR_ACTION_KILL;
            goto commit;
        }
        if (next.order_state == 0 && input->parent_distance_le_60) {
            next.order_state = 1;
            if (input->parent_elevation < INT16_MIN + 2) return 0;
            next.elevation = (int16_t)(input->parent_elevation - 2);
        }
        if (input->parent_distance_le_10) {
            next.outside = 0;
            out.actions = SC64_INTERCEPTOR_ACTION_RELINK_INSIDE |
                          SC64_INTERCEPTOR_ACTION_HIDE |
                          SC64_INTERCEPTOR_ACTION_NOTHING_ORDER;
            goto commit;
        }
        next.move_x = input->parent_x;
        next.move_y = input->parent_y;
        out.actions = SC64_INTERCEPTOR_ACTION_MOVE_TO_TARGET;
    }

commit:
    *order = next;
    *result = out;
    return 1;
}
