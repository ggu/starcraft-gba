/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_INTERCEPTOR_MOTION_H
#define SC64_INTERCEPTOR_MOTION_H

#include <stdint.h>
#include "sc64_interceptor_pool_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t map_width_px;
    uint16_t map_height_px;
    uint8_t target_alive;
    uint8_t owner_alive;
} Sc64InterceptorMotionContext;

/* Call once per gameplay update. Advance each projectile only when true. */
int sc64_interceptor_source_frame_due(uint8_t *phase);

/* Mutates only representable projectile fields in row. Nullness is resolved
 * by the caller and reflected by target_ref/owner_ref sentinels. */
int sc64_interceptor_motion_tick(Sc64ProjectileRow *row,
                                const Sc64InterceptorMotionContext *context);

/* Source-coordinate helpers used by a launch adapter. Delta is integer source
 * pixels; forward offset is the native integer-pixel muzzle offset expressed
 * in signed Q8.8, with each component rounded down independently. */
int sc64_interceptor_direction_for_delta(int32_t dx_source, int32_t dy_source,
                                         uint8_t *direction);
int sc64_interceptor_forward_offset(uint8_t heading,
                                    int32_t *x_offset_q8_source,
                                    int32_t *y_offset_q8_source);

/* Build a codec-domain INIT row. origin is the child's source position in
 * signed Q8.8; the native sprite position uses its integer part. Muzzle is
 * offset 20 source pixels along current_heading, rounded separately. */
int sc64_interceptor_projectile_launch(
    Sc64ProjectileRow *out,
    int32_t origin_x_q8_source, int32_t origin_y_q8_source,
    int16_t target_x_source, int16_t target_y_source,
    uint8_t current_heading, uint16_t target_ref, uint8_t owner_ref,
    uint8_t raw_source_owner);

#ifdef __cplusplus
}
#endif
#endif
