/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdint.h>
typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
#include "sc64_interceptor_motion_kernel.h"
#include "sc64_interceptor_motion.h"

static int
sc64_interceptor_row_domain_valid(const Sc64ProjectileRow *row)
{
    return row && row->x_q8_source >= -2097152 &&
        row->x_q8_source <= 2097151 && row->y_q8_source >= -2097152 &&
        row->y_q8_source <= 2097151 &&
        row->destination_x_source >= -8192 &&
        row->destination_x_source <= 8191 &&
        row->destination_y_source >= -8192 &&
        row->destination_y_source <= 8191 &&
        row->next_speed <= SC64_INTERCEPTOR_FLINGY_TOP_SPEED &&
        row->turn_rate_raw < 512 &&
        (row->target_ref <= 399 || row->target_ref == 511) &&
        (row->owner_ref <= 239 || row->owner_ref == 255) &&
        row->remaining_time >= -1 && row->remaining_time <= 255 &&
        row->state <= SC64_INTERCEPTOR_MOTION_FREE &&
        row->order_signal_bit0 <= 1 && row->movement_flags_low4 <= 15 &&
        row->hit_flags_low2 <= 3 && row->script_phase <= 3 &&
        row->raw_source_owner <= 11 && row->pending_damage_event <= 1 &&
        row->next_link <= SC64_END && row->reserved_snapshot_bits == 0;
}

int
sc64_interceptor_motion_tick(Sc64ProjectileRow *row,
                             const Sc64InterceptorMotionContext *context)
{
    Sc64InterceptorMotion p;
    Sc64ProjectileRow result;
    if (!row || !context || !sc64_interceptor_row_domain_valid(row) ||
        !context->map_width_px || context->map_width_px > 8192 ||
        !context->map_height_px || context->map_height_px > 8192 ||
        context->target_alive > 1 ||
        context->owner_alive > 1)
        return 0;
    p.x_q8 = row->x_q8_source;
    p.y_q8 = row->y_q8_source;
    p.destination_x = row->destination_x_source;
    p.destination_y = row->destination_y_source;
    p.launch_x = p.launch_y = 0; /* v8 update does not read launch fields. */
    p.speed = row->next_speed;
    p.next_speed = row->next_speed;
    p.flingy_turn_rate_raw = row->turn_rate_raw;
    p.heading = row->heading;
    p.velocity_direction = row->next_velocity_direction;
    p.next_velocity_direction = row->next_velocity_direction;
    p.target_ref = row->target_ref;
    p.owner_ref = row->owner_ref;
    p.map_width_px = context->map_width_px;
    p.map_height_px = context->map_height_px;
    p.remaining_time = row->remaining_time;
    p.state = row->state;
    p.order_signal = row->order_signal_bit0;
    p.movement_flags = row->movement_flags_low4;
    p.hit_flags = row->hit_flags_low2;
    p.script_phase = row->script_phase;
    p.raw_owner = row->raw_source_owner;
    p.damage_event = row->pending_damage_event;
    p.alive = (uint8_t)(row->state != SC64_INTERCEPTOR_MOTION_FREE);

    sc64_interceptor_projectile_tick(
        &p, context->target_alive != 0 && row->target_ref != 511,
        context->owner_alive != 0 && row->owner_ref != 255);

    result = *row;
    result.x_q8_source = p.x_q8;
    result.y_q8_source = p.y_q8;
    result.destination_x_source = (int16_t)p.destination_x;
    result.destination_y_source = (int16_t)p.destination_y;
    result.next_speed = p.next_speed;
    result.turn_rate_raw = p.flingy_turn_rate_raw;
    result.heading = p.heading;
    result.next_velocity_direction = p.next_velocity_direction;
    result.target_ref = p.target_ref == 0xffffu ? 511u : p.target_ref;
    result.owner_ref = (uint8_t)(p.owner_ref == 0xffffu ? 255u : p.owner_ref);
    result.remaining_time = p.remaining_time;
    result.state = p.state;
    result.order_signal_bit0 = (uint8_t)(p.order_signal & 1u);
    result.movement_flags_low4 = (uint8_t)(p.movement_flags & 15u);
    result.hit_flags_low2 = (uint8_t)(p.hit_flags & 3u);
    result.script_phase = p.script_phase;
    result.raw_source_owner = p.raw_owner;
    result.pending_damage_event = p.damage_event;
    if (!sc64_interceptor_row_domain_valid(&result))
        return 0;
    *row = result;
    return 1;
}

int
sc64_interceptor_direction_for_delta(int32_t dx_source, int32_t dy_source,
                                     uint8_t *direction)
{
    if (!direction || dx_source < -16383 || dx_source > 16383 ||
        dy_source < -16383 || dy_source > 16383 ||
        (dx_source == 0 && dy_source == 0))
        return 0;
    *direction = sc64_interceptor_dir(dx_source, dy_source);
    return 1;
}

int
sc64_interceptor_forward_offset(uint8_t heading,
                                int32_t *x_offset_q8_source,
                                int32_t *y_offset_q8_source)
{
    const Sc64InterceptorDirectionVector v =
        sc64_interceptor_direction_vector[heading];
    if (!x_offset_q8_source || !y_offset_q8_source ||
        x_offset_q8_source == y_offset_q8_source)
        return 0;
    /* Native fire_weapon adds to_xy(direction_xy(...)) to the integer sprite
     * position, then create_bullet constructs its exact Q8 position. Round
     * each component down before adding; preserving the vector fraction or
     * truncating negative values toward zero changes non-cardinal launches. */
    *x_offset_q8_source = sc64_interceptor_integer_part((int32_t)v.x*20)*256;
    *y_offset_q8_source = sc64_interceptor_integer_part((int32_t)v.y*20)*256;
    return 1;
}

int
sc64_interceptor_projectile_launch(
    Sc64ProjectileRow *out,
    int32_t origin_x_q8_source, int32_t origin_y_q8_source,
    int16_t target_x_source, int16_t target_y_source,
    uint8_t current_heading, uint16_t target_ref, uint8_t owner_ref,
    uint8_t raw_source_owner)
{
    Sc64ProjectileRow row;
    int32_t muzzle_x, muzzle_y, x, y;
    if (!out || origin_x_q8_source < -2097152 ||
        origin_x_q8_source > 2097151 ||
        origin_y_q8_source < -2097152 ||
        origin_y_q8_source > 2097151 ||
        target_x_source < -8192 || target_x_source > 8191 ||
        target_y_source < -8192 || target_y_source > 8191 ||
        (target_ref > 399 && target_ref != 511) ||
        (owner_ref > 239 && owner_ref != 255) || raw_source_owner > 11)
        return 0;
    if (!sc64_interceptor_forward_offset(current_heading, &muzzle_x,
                                         &muzzle_y))
        return 0;
    /* Bounds were checked above and the offset is at most 5120 in magnitude,
     * so these additions cannot overflow int32_t. */
    x = sc64_interceptor_integer_part(origin_x_q8_source)*256 + muzzle_x;
    y = sc64_interceptor_integer_part(origin_y_q8_source)*256 + muzzle_y;
    if (x < -2097152 || x > 2097151 || y < -2097152 || y > 2097151)
        return 0;
    row.x_q8_source = x;
    row.y_q8_source = y;
    row.destination_x_source = target_x_source;
    row.destination_y_source = target_y_source;
    row.next_speed = 0;
    row.turn_rate_raw = SC64_INTERCEPTOR_FLINGY_TURN_RATE;
    row.heading = current_heading;
    row.next_velocity_direction = current_heading;
    row.target_ref = target_ref;
    row.owner_ref = owner_ref;
    row.remaining_time = SC64_INTERCEPTOR_WEAPON79_LIFETIME;
    row.state = SC64_INTERCEPTOR_MOTION_INIT;
    row.order_signal_bit0 = 0;
    /* Native weapon-79 launch initializes the low-four movement field with
     * the source's initial movement/status bit (observed value 8). */
    row.movement_flags_low4 = 8;
    row.hit_flags_low2 = 0;
    row.script_phase = 1;
    row.raw_source_owner = raw_source_owner;
    row.pending_damage_event = 0;
    row.next_link = 0;
    row.reserved_snapshot_bits = 0;
    *out = row;
    return 1;
}
