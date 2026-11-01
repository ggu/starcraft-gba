/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_INTERCEPTOR_POOL_CODEC_H
#define SC64_INTERCEPTOR_POOL_CODEC_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SC64_POOL_CAPACITY 100u
#define SC64_POOL_GROWTH 10u
#define SC64_CODEC_BYTES 2103u
#define SC64_ARENA_WORDS 2408u
#define SC64_END 127u
/* Runtime arena access is through volatile 16-bit words only. The caller
 * supplies aligned storage and keeps packet buffers disjoint from it. */
typedef struct { volatile uint16_t *words; size_t word_count; } Sc64Arena;
/* Source-coordinate Q8 positions; lifecycle states 0=init, 1=moving,
 * 2=dying, 3=free. References are bounded symbolic slots, not pointers.
 * World-reference invalidation and exactly-once damage remain caller work. */
typedef struct {
    int32_t x_q8_source, y_q8_source;
    int16_t destination_x_source, destination_y_source;
    uint16_t next_speed, turn_rate_raw, target_ref;
    uint8_t heading, next_velocity_direction, owner_ref;
    int16_t remaining_time;
    uint8_t state, order_signal_bit0, movement_flags_low4, hit_flags_low2;
    uint8_t script_phase, raw_source_owner, pending_damage_event;
    uint8_t next_link, reserved_snapshot_bits;
} Sc64ProjectileRow;
int sc64_pool_init(Sc64Arena *a);
int sc64_pool_acquire(Sc64Arena *a, uint8_t *ordinal, uint8_t *native_index);
int sc64_pool_retire(Sc64Arena *a, uint8_t ordinal);
int sc64_pool_validate(const Sc64Arena *a);
/* Bounded list cursor for the exclusively owned runtime arena. Returns 255
 * on malformed head metadata, SC64_END for an empty active list. */
uint8_t sc64_pool_active_head(const Sc64Arena *a);
int sc64_codec_encode(const Sc64Arena *a, uint8_t *out, size_t out_size);
/* Full packet preflight without modifying an arena or a live game world. */
int sc64_codec_validate(const uint8_t *in, size_t in_size);
/* Packet preflight plus the destination arena's size and disjointness.
 * The arena descriptor and packet must remain stable until decode. This does
 * not guarantee decode's final pool validation or make decode infallible. */
int sc64_codec_validate_target(const Sc64Arena *a, const uint8_t *in, size_t in_size);
/* Header-only count, or -1 for malformed header/size. This is not a full
 * preflight; callers accepting external packets must also validate them. */
int sc64_codec_allocated_count(const uint8_t *in, size_t in_size);
/* Read one bounded row. Full list membership must first be checked with
 * sc64_codec_validate when consuming an untrusted save candidate. */
int sc64_codec_get_row(const uint8_t *in, size_t in_size, uint8_t ordinal,
                       Sc64ProjectileRow *out);
int sc64_codec_decode(Sc64Arena *a, const uint8_t *in, size_t in_size);
int sc64_pool_get_row(const Sc64Arena *a, uint8_t ordinal, Sc64ProjectileRow *out);
/*
 * Writes projectile fields for an allocated ordinal but preserves intrusive
 * F_PREV/F_NEXT links. row->next_link is ignored. The caller must supply field
 * values inside the codec-v3 domains and keep row->state consistent with the
 * ordinal's active/free list; pool_validate/encode enforce those conditions.
 */
int sc64_pool_set_row(Sc64Arena *a, uint8_t ordinal, const Sc64ProjectileRow *row);
uint8_t sc64_native_index(uint8_t ordinal);
#ifdef __cplusplus
}
#endif
#endif
