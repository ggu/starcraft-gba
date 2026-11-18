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
/* Check the splice in an exclusively owned, validated arena. Growth and
 * suspect links retain the full validator. Initialize the acquired row
 * before any save or gameplay boundary, as with the full acquire. */
int sc64_pool_acquire_live(Sc64Arena *a, uint8_t *ordinal, uint8_t *native_index);
int sc64_pool_retire(Sc64Arena *a, uint8_t ordinal);
/* Only for an exclusively owned live arena from init or validated decode,
 * maintained by checked pool mutators and domain-valid row updates. Use an
 * ordinal from the active chain. Check the splice, not unseen rows; retain
 * the complete validator on incoming packets and save/export. Like strict
 * retirement, leaves non-list fields in the newly free row unchanged. */
int sc64_pool_retire_live(Sc64Arena *a, uint8_t ordinal);
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

#ifdef SC64_INTERCEPTOR_COMPACT_CODEC
/* Standalone compact format. The enclosing save owns its version and the
 * active cap (0..SC64_POOL_CAPACITY), fixed from pool init through loading.
 * Use only the capped acquire and canonical retire operations for this pool.
 * Every acquire must be followed by initialization before a save boundary. */
int sc64_compact_pool_acquire(Sc64Arena *a, unsigned cap,
                              uint8_t *ordinal, uint8_t *native_index);
int sc64_compact_pool_acquire_live(Sc64Arena *a, unsigned cap,
                                   uint8_t *ordinal, uint8_t *native_index);
int sc64_compact_pool_retire(Sc64Arena *a, uint8_t ordinal);
/* Live retirement with the same exclusive-ownership contract and compact
 * canonicalization of all non-list fields in the newly free row. */
int sc64_compact_pool_retire_live(Sc64Arena *a, uint8_t ordinal);
/* Worst-case bytes, or zero for invalid cap. */
size_t sc64_compact_codec_max_bytes(unsigned cap);
/* Returns the bytes written, or zero on rejection. Writes only those bytes. */
size_t sc64_compact_codec_encode(const Sc64Arena *a, unsigned cap,
                                  uint8_t *out, size_t out_capacity);
/* Detached preflight. Optionally visits active rows in list order, after
 * validating the entire packet; the predicate must not mutate live state or
 * the packet. Zero from the predicate rejects the candidate. The empty
 * unallocated pool encodes as two bytes. */
typedef int (*Sc64CompactRowCheck)(void *context, uint8_t ordinal,
                                    const Sc64ProjectileRow *row);
int sc64_compact_codec_validate(const uint8_t *in, size_t in_size,
                                unsigned cap, Sc64CompactRowCheck check,
                                void *context);
/* Pass the exact encoded length. Malformed/overlapping packets leave every
 * destination word intact. The caller keeps both descriptors and underlying
 * buffers stable and independently owned throughout the call. */
int sc64_compact_codec_validate_target(const Sc64Arena *a, const uint8_t *in,
                                       size_t in_size, unsigned cap,
                                       Sc64CompactRowCheck check, void *context);
/* Commit after the same preflight, without a world predicate. */
int sc64_compact_codec_decode(Sc64Arena *a, const uint8_t *in, size_t in_size,
                              unsigned cap);
#endif
#ifdef __cplusplus
}
#endif
#endif
