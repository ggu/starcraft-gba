/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_INTERCEPTOR_MOTION_KERNEL_H
#define SC64_INTERCEPTOR_MOTION_KERNEL_H

/* Project-authored weapon-79 motion. Values are in source 24 Hz frames
 * and Q8.8 pixel units. Actor orders and damage consumption are caller work. */
#include "sc64_interceptor_direction_tables.h"

typedef struct {
    s32 x_q8, y_q8;
    s32 destination_x, destination_y;
    s32 launch_x, launch_y;
    u16 speed, next_speed;
    u16 flingy_turn_rate_raw;
    u8 heading, velocity_direction, next_velocity_direction;
    u16 target_ref, owner_ref;
    u16 map_width_px, map_height_px;
    s16 remaining_time;
    u8 state, order_signal;
    u8 movement_flags, hit_flags;
    u8 script_phase;
    u8 raw_owner;
    u8 damage_event, alive;
} Sc64InterceptorMotion;

enum {
    SC64_INTERCEPTOR_MOTION_INIT = 0,
    SC64_INTERCEPTOR_MOTION_MOVE = 1,
    SC64_INTERCEPTOR_MOTION_DYING = 2,
    SC64_INTERCEPTOR_MOTION_FREE = 3,
    SC64_INTERCEPTOR_FLINGY_TOP_SPEED = 21333,
    SC64_INTERCEPTOR_FLINGY_ACCEL = 850,
    SC64_INTERCEPTOR_FLINGY_TURN_RATE = 127,
    SC64_INTERCEPTOR_FLINGY_TYPE1_TURN_RATE = 63,
    SC64_INTERCEPTOR_WEAPON79_LIFETIME = 255
};

static s32 sc64_interceptor_abs32(s32 x)
{
    /* Callers are bounded map/Q8 coordinates; INT32_MIN is outside domain. */
    return x < 0 ? -x : x;
}
static s32 sc64_interceptor_trunc_div(s32 n, s32 d)
{
    if (d == 0) return 0;
    return n / d;
}
/* fixed_point::integer_part is raw >> 8, which rounds negative values
 * down. Spell that out without relying on C's implementation-defined shift. */
static s32 sc64_interceptor_integer_part(s32 raw)
{
    if (raw >= 0) return raw / 256;
    return -(s32)(((u32)(-(raw + 1)) + 1u + 255u) / 256u);
}
static s32 sc64_interceptor_len_raw(s32 x, s32 y)
{
    u32 a = (u32)sc64_interceptor_abs32(x), b = (u32)sc64_interceptor_abs32(y), t;
    if (a < b) { t = a; a = b; b = t; }
    if (a / 4u < b)
        a = a - a / 16u + b * 3u / 8u - a / 64u + b * 3u / 256u;
    return (s32)a;
}
static u8 sc64_interceptor_dir(s32 x, s32 y)
{
    s32 q, den, num;
    u32 mag;
    u8 r;
    if (x == 0) return y <= 0 ? 0 : 128;
    num = y;
    den = x;
    if (den < 0) { num = -num; den = -den; }
    /* atan receives Q8.8 y/x; its raw unsigned lookup uses the lower bound. */
    q = sc64_interceptor_trunc_div(num * 256, den);
    mag = (u32)sc64_interceptor_abs32(q);
    for (r = 0; r < 64 && sc64_interceptor_tangent_cutoff[r] < mag; ++r) {}
    if (x > 0) return (u8)(64 + (q < 0 ? -(s32)r : (s32)r));
    return (u8)(-64 + (q < 0 ? -(s32)r : (s32)r));
}
static s32 sc64_interceptor_turn_delta(u8 from, u8 to)
{
    s32 d = (s32)(u8)(to - from);
    return d >= 128 ? d - 256 : d;
}
static void sc64_interceptor_toward(u8 *value, u8 desired, u16 limit)
{
    s32 d = sc64_interceptor_turn_delta(*value, desired);
    if (d > limit) d = limit;
    else if (d < -(s32)limit) d = -(s32)limit;
    *value = (u8)(*value + d);
}
static void sc64_interceptor_velocity(const Sc64InterceptorMotion *p, s32 *vx, s32 *vy)
{
    const Sc64InterceptorDirectionVector v = sc64_interceptor_direction_vector[p->velocity_direction];
    *vx = sc64_interceptor_integer_part((s32)v.x * p->speed);
    *vy = sc64_interceptor_integer_part((s32)v.y * p->speed);
}

/* Source-order tick: Init IScript, bullet_execute state, then Death opcode. */
/* Shared 30 Hz game clock to 24 Hz source-frame cadence. Call once per
 * gameplay update and advance every projectile only when this returns true. */
int sc64_interceptor_source_frame_due(u8 *phase)
{
    unsigned next;
    if (!phase || *phase >= 5) return 0;
    next = (unsigned)*phase + 4u;
    if (next < 5u) { *phase = (u8)next; return 0; }
    *phase = (u8)(next - 5u);
    return 1;
}

static void sc64_interceptor_projectile_tick(Sc64InterceptorMotion *p,
                                    int target_alive, int owner_alive)
{
    if (!p || !p->alive) return;
    p->damage_event = 0; /* one-update impact pulse; never latched */

    /* Source order is IScript, then bullet_execute. Init begins on wait(1);
     * its next IScript update signals order and waits, which bullet_execute
     * consumes on that update. Death starts on impact/expiry; domissiledmg is
     * synchronous within that same update, followed by wait(1) and next-update free. */
    if (p->state == SC64_INTERCEPTOR_MOTION_DYING) {
        /* Death wait(1) was entered on impact/expiry. Its next source update
         * executes end, then bullet_execute frees the bullet. */
        p->alive = 0;
        p->state = SC64_INTERCEPTOR_MOTION_FREE;
        p->script_phase = 0;
        return;
    }
    if (p->state == SC64_INTERCEPTOR_MOTION_INIT) {
        if (p->script_phase == 0) {
            p->script_phase = 1; /* Initial Init wait(1) expires. */
            return;
        }
        if (p->script_phase == 1) {
            p->order_signal |= 1;
            p->script_phase = 2; /* sigorder(1), then wait(1). */
        }
        if (!(p->order_signal & 1u)) return;
        p->order_signal &= (u8)~1u;
        p->state = SC64_INTERCEPTOR_MOTION_MOVE;
        p->movement_flags |= 1;
        p->next_velocity_direction = p->heading;
        /* GndAttkInit's wait(1) yields; the transitioned bullet state moves. */
    }
    if (p->state != SC64_INTERCEPTOR_MOTION_MOVE) return;
    if (!target_alive) p->target_ref = 0xffffu;
    if (!owner_alive) p->owner_ref = 0xffffu;
    {
        s32 px = sc64_interceptor_integer_part(p->x_q8), py = sc64_interceptor_integer_part(p->y_q8);
        s32 dx = p->destination_x - px;
        s32 dy = p->destination_y - py;
        s32 target_dx = dx, target_dy = dy;
        u8 desired;
        s32 d, vx, vy, remain;
        if (dx == 0 && dy == 0) {
            /* With the fixed move endpoint also at the current position,
             * OpenBW's final waypoint fallback is the current heading. */
            target_dx = 0;
            target_dy = 0;
            desired = p->heading;
        } else {
            desired = sc64_interceptor_dir(target_dx, target_dy);
        }
        {
            /* unit_turn_rate uses unsigned flingy rate / 2, clamps in fp8,
             * then truncates the signed result back to direction_t. */
            sc64_interceptor_toward(&p->next_velocity_direction, desired,
                                    p->flingy_turn_rate_raw / 2u);
            p->velocity_direction = p->next_velocity_direction;
        }
        p->speed = p->next_speed;
        if (dx == 0 && dy == 0) {
            p->movement_flags &= (u8)~4u;
            p->speed = 0;
        } else {
            d = sc64_interceptor_len_raw(dx, dy);
            if (p->velocity_direction == desired || d >= 32) {
                p->speed += SC64_INTERCEPTOR_FLINGY_ACCEL;
            } else {
                s32 turn = sc64_interceptor_abs32(sc64_interceptor_turn_delta(
                    p->velocity_direction, desired));
    s32 rate_raw = p->flingy_turn_rate_raw;
                s32 numerator = turn * 2 + rate_raw - 1;
                /* OpenBW divide_multiply is rawA/rawB*rawC: division happens
                 * first and truncates toward zero, then integer_part takes
                 * signed raw >> 8 (floor for negative values on ARM). */
                s32 quotient = sc64_interceptor_trunc_div(numerator, rate_raw);
                s32 val_raw = quotient * (s32)p->next_speed;
                s32 val_integer = sc64_interceptor_integer_part(val_raw);
                /* The source stores this signed fixed-point integer in
                 * unsigned int before its 3/2 threshold calculation. */
                if (((u32)val_integer * 3u) / 2u <= (u32)d)
                    p->speed += SC64_INTERCEPTOR_FLINGY_ACCEL;
                else
                    p->speed = p->speed > SC64_INTERCEPTOR_FLINGY_ACCEL
                             ? p->speed - SC64_INTERCEPTOR_FLINGY_ACCEL : 0;
            }
            if (p->speed > SC64_INTERCEPTOR_FLINGY_TOP_SPEED)
                p->speed = SC64_INTERCEPTOR_FLINGY_TOP_SPEED;
        }
        if (dx || dy) p->movement_flags |= 2;
        else p->movement_flags &= (u8)~2u;
        sc64_interceptor_velocity(p, &vx, &vy);
        remain = sc64_interceptor_len_raw((p->destination_x * 256) - p->x_q8,
                                 (p->destination_y * 256) - p->y_q8);
        if ((p->movement_flags & 2u) && p->speed) {
            if (p->speed >= remain) {
                p->x_q8 = p->destination_x * 256;
                p->y_q8 = p->destination_y * 256;
            } else {
                p->x_q8 += vx;
                p->y_q8 += vy;
            }
        }
        p->next_speed = p->speed;
        p->next_velocity_direction = p->velocity_direction;
        if (p->map_width_px && p->map_height_px) {
            s32 new_x = sc64_interceptor_integer_part(p->x_q8), new_y = sc64_interceptor_integer_part(p->y_q8);
            s32 clamped_x = new_x, clamped_y = new_y;
            if (new_x < 0 || new_y < 0 || new_x >= p->map_width_px ||
                new_y >= p->map_height_px) p->remaining_time = 0;
            if (clamped_x < 0) clamped_x = 0;
            else if (clamped_x >= p->map_width_px) clamped_x = p->map_width_px - 1;
            if (clamped_y < 0) clamped_y = 0;
            else if (clamped_y >= p->map_height_px) clamped_y = p->map_height_px - 1;
            if (new_x != clamped_x || new_y != clamped_y) {
                p->x_q8 = clamped_x * 256;
                p->y_q8 = clamped_y * 256;
            }
        }
        p->next_velocity_direction = p->velocity_direction;
        if (!(p->movement_flags & 2u) || (p->movement_flags & 1u)) {
            /* OpenBW truncates the signed fp8 rate to an 8-bit direction.
             * direction_t has the same 8 fractional bits, then wraps signed. */
            s32 turn = sc64_interceptor_turn_delta(p->heading, desired);
            s32 max_turn = (s32)(p->flingy_turn_rate_raw & 0xffu);
            if (max_turn >= 128) max_turn -= 256;
            s32 neg_rate_raw = -(s32)p->flingy_turn_rate_raw;
            s32 min_turn = neg_rate_raw & 0xff;
            if (min_turn >= 128) min_turn -= 256;
            if (turn > max_turn) turn = max_turn;
            else if (turn < min_turn) turn = -max_turn;
            p->heading = (u8)(p->heading + turn);
            /* flingy id 155 is in OpenBW's rate-growth interval 0x8d..0xab. */
            ++p->flingy_turn_rate_raw;
            if (p->velocity_direction == desired && p->heading == desired)
                p->movement_flags &= (u8)~1u;
        }
        if (p->remaining_time-- == 0 ||
            (sc64_interceptor_integer_part(p->x_q8) == p->destination_x &&
             sc64_interceptor_integer_part(p->y_q8) == p->destination_y)) {
            p->state = SC64_INTERCEPTOR_MOTION_DYING;
            p->script_phase = 1; /* Death wait(1), after synchronous opcode. */
            p->damage_event = (u8)((p->hit_flags & 1u) == 0 && target_alive);
        }
    }
}
#endif
