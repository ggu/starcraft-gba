/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sc64_interceptor_pool_codec.h"
#include <stdint.h>

/* Freestanding toolchains may omit string.h while providing libc memset. */
void *memset(void *destination, int value, size_t count);
/*
 * Arena words 0..7 hold heads/counts/tails; each fixed slot occupies 24
 * words.
 */
enum {
    H_ACTIVE = 0, H_FREE = 1, H_SIZE = 2, H_ACTIVE_TAIL = 3,
    H_FREE_TAIL = 4, H_ACTIVE_COUNT = 5, H_FREE_COUNT = 6,
    H_RESERVED = 7, ROW_BASE = 8, ROW_WORDS = 24
};
enum {
    F_PREV = 0, F_NEXT = 1, F_STATE = 2, F_XLO = 3, F_XHI = 4,
    F_YLO = 5, F_YHI = 6, F_DX = 7, F_DY = 8, F_SPEED = 9,
    F_TURN = 10, F_HEADING = 11, F_VDIR = 12, F_TARGET = 13,
    F_OWNER = 14, F_TIMER = 15, F_SIGNAL = 16, F_MOVE = 17,
    F_HIT = 18, F_PHASE = 19, F_RAWOWNER = 20, F_PENDING = 21,
    F_LAYER = 22, F_PADDING = 23
};
enum {
    FIXED_LAYER_FLAG = 0x20,
    FIXED_UNKNOWN_FLAGS = 0xc0,
    COMPACT_LAYER_FLAG = 0x80,
    COMPACT_DEST_MASK = 0x0fff
};
static int
arena_ok(const Sc64Arena * a)
{
    return a && a->words && a->word_count >= SC64_ARENA_WORDS;
}

/*
 * The packet is a byte buffer and the arena is a fixed halfword region.
 * Their intervals must be disjoint. Reject address arithmetic overflow so
 * the interval comparison remains bounded on the host and ARM targets.
 */
static int
packet_overlaps_arena(const Sc64Arena *a, const void *packet, size_t packet_bytes)
{
    if (!a || !a->words || !packet)
        return 1;
    const uintptr_t arena_start = (uintptr_t)(const void *)a->words;
    const uintptr_t packet_start = (uintptr_t)packet;
    const size_t arena_bytes = SC64_ARENA_WORDS * sizeof(uint16_t);
    if (arena_bytes > UINTPTR_MAX - arena_start ||
        packet_bytes > UINTPTR_MAX - packet_start)
        return 1;
    const uintptr_t arena_end = arena_start + arena_bytes;
    const uintptr_t packet_end = packet_start + packet_bytes;
    return arena_start < packet_end && packet_start < arena_end;
}
static size_t at(uint8_t id, unsigned f){
    return ROW_BASE + (size_t) id * ROW_WORDS + f;
}
static uint16_t rd(const Sc64Arena * a, size_t i){
    return a->words[i];
}
static void
wr(Sc64Arena * a, size_t i, uint16_t v)
{
    a->words[i] = v;
}
uint8_t
sc64_native_index(uint8_t o) {
    return o == 0 ? 0 : (uint8_t) (100u - o);
}
static uint8_t head(const Sc64Arena * a, unsigned i){
    return (uint8_t) rd(a, i);
}
static void
sethead(Sc64Arena * a, unsigned i, uint8_t v)
{
    wr(a, i, v);
}
uint8_t
sc64_pool_active_head(const Sc64Arena *a)
{
    if (!arena_ok(a) || rd(a,H_SIZE)>SC64_POOL_CAPACITY ||
        rd(a,H_ACTIVE)>SC64_END ||
        (rd(a,H_ACTIVE)!=SC64_END && rd(a,H_ACTIVE)>=rd(a,H_SIZE)))
        return 255;
    return head(a,H_ACTIVE);
}
static uint16_t rfield(const Sc64Arena * a, uint8_t id, unsigned f){
    return rd(a, at(id, f));
}
static void
sfield(Sc64Arena * a, uint8_t id, unsigned f, uint16_t v)
{
    wr(a, at(id, f), v);
}
static void
list_insert_after(Sc64Arena * a, unsigned h, unsigned t, unsigned c, uint8_t id)
{
    uint8_t	    x = head(a, h);
    sfield(a, id, F_PREV, x == SC64_END ? SC64_END : x);
    sfield(a, id, F_NEXT, SC64_END);
    if (x == SC64_END) {
	sethead(a, h, id);
	sethead(a, t, id);
    } else {
	uint8_t		n = (uint8_t) rfield(a, x, F_NEXT);
	sfield(a, id, F_NEXT, n);
	sfield(a, x, F_NEXT, id);
	if (n != SC64_END)
	    sfield(a, n, F_PREV, id);
	else
	    sethead(a, t, id);
    } sethead(a, c, (uint8_t) (head(a, c) + 1));
}
static void
list_erase(Sc64Arena * a, unsigned h, unsigned t, unsigned c, uint8_t id)
{
    uint8_t	    p = (uint8_t) rfield(a, id, F_PREV), n = (uint8_t) rfield(a, id, F_NEXT);
    if (p == SC64_END)
	sethead(a, h, n);
    else
	sfield(a, p, F_NEXT, n);
    if (n == SC64_END)
	sethead(a, t, p);
    else
	sfield(a, n, F_PREV, p);
    sfield(a, id, F_PREV, SC64_END);
    sfield(a, id, F_NEXT, SC64_END);
    sethead(a, c, (uint8_t) (head(a, c) - 1));
}
int
sc64_pool_init(Sc64Arena * a)
{
    if (!arena_ok(a))
	return 0;
    for (size_t i = 0; i < SC64_ARENA_WORDS; i++)
	wr(a, i, 0);
    sethead(a, H_ACTIVE, SC64_END);
    sethead(a, H_FREE, SC64_END);
    sethead(a, H_ACTIVE_TAIL, SC64_END);
    sethead(a, H_FREE_TAIL, SC64_END);
    for (unsigned i = 0; i < 100; i++) {
	sfield(a, (uint8_t) i, F_PREV, SC64_END);
	sfield(a, (uint8_t) i, F_NEXT, 0);
	sfield(a, (uint8_t) i, F_TARGET, 511);
	sfield(a, (uint8_t) i, F_OWNER, 255);
	sfield(a, (uint8_t) i, F_STATE, 0);
    } return 1;
}
static int
grow(Sc64Arena * a)
{
    uint8_t	    size = head(a, H_SIZE);
    if (size >= 100)
	return 0;
    uint8_t	    n = (uint8_t) ((100 - size) < 10 ? (100 - size) : 10);
    for (uint8_t j = 0; j < n; j++) {
	uint8_t		id = (uint8_t) (size + j);
	sfield(a, id, F_STATE, 3);
	sfield(a, id, F_TARGET, 511);
	sfield(a, id, F_OWNER, 255);
	sfield(a, id, F_PREV, SC64_END);
	sfield(a, id, F_NEXT, SC64_END);
	if (head(a, H_FREE) == SC64_END) {
	    sethead(a, H_FREE, id);
	    sethead(a, H_FREE_TAIL, id);
	} else {
	    sfield(a, head(a, H_FREE_TAIL), F_NEXT, id);
	    sfield(a, id, F_PREV, head(a, H_FREE_TAIL));
	    sethead(a, H_FREE_TAIL, id);
	} sethead(a, H_FREE_COUNT, (uint8_t) (head(a, H_FREE_COUNT) + 1));
    } sethead(a, H_SIZE, (uint8_t) (size + n));
    return 1;
}
static int
pool_acquire_linked(Sc64Arena *a, uint8_t *ordinal, uint8_t *native_index)
{
    uint8_t	    size = head(a, H_SIZE);
    if (head(a, H_FREE) == SC64_END) {
	if (size == 100 && !grow(a))
	    return 0;
	if (size < 100 && !grow(a))
	    return 0;
    } else if (size < 100 && head(a, H_FREE_COUNT) == 1) {
	if (!grow(a))
	    return 0;
    } uint8_t	    id = head(a, H_FREE);
    list_erase(a, H_FREE, H_FREE_TAIL, H_FREE_COUNT, id);
    sfield(a, id, F_STATE, 0);
    list_insert_after(a, H_ACTIVE, H_ACTIVE_TAIL, H_ACTIVE_COUNT, id);
    *ordinal = id;
    *native_index = sc64_native_index(id);
    return 1;
}

int
sc64_pool_acquire(Sc64Arena *a, uint8_t *ordinal, uint8_t *native_index)
{
    if (!ordinal || !native_index || !sc64_pool_validate(a)) return 0;
    return pool_acquire_linked(a, ordinal, native_index);
}
static void
pool_retire_linked(Sc64Arena *a, uint8_t id)
{
    list_erase(a, H_ACTIVE, H_ACTIVE_TAIL, H_ACTIVE_COUNT, id);
    sfield(a, id, F_STATE, 3);
    sfield(a, id, F_LAYER, 0);
    list_insert_after(a, H_FREE, H_FREE_TAIL, H_FREE_COUNT, id);
}

int
sc64_pool_retire(Sc64Arena * a, uint8_t id)
{
    if (!arena_ok(a) || !sc64_pool_validate(a) || id >= head(a, H_SIZE) || rfield(a, id, F_STATE) == 3)
	return 0;
    pool_retire_linked(a, id);
    return 1;
}
static int
row_storage_widths_valid(const Sc64Arena * a, uint8_t id)
{
    static const unsigned byte_fields[] = {
	F_STATE, F_HEADING, F_VDIR, F_OWNER, F_SIGNAL, F_MOVE, F_HIT,
	F_PHASE, F_RAWOWNER, F_PENDING, F_NEXT, F_LAYER
    };
    for (size_t i = 0; i < sizeof byte_fields / sizeof byte_fields[0]; ++i) {
	if (rfield(a, id, byte_fields[i]) > 255)
	    return 0;
    }
    return rfield(a, id, F_STATE) <= 3 &&
	rfield(a, id, F_SIGNAL) <= 1 && rfield(a, id, F_MOVE) <= 15 &&
	rfield(a, id, F_HIT) <= 3 && rfield(a, id, F_PHASE) <= 3 &&
	rfield(a, id, F_PENDING) <= 1 && rfield(a, id, F_NEXT) <= SC64_END &&
	rfield(a, id, F_LAYER) <= SC64_PROJECTILE_MAX_LAYER + 1u &&
	rfield(a, id, F_TARGET) <= 511 &&
	rfield(a, id, F_TURN) <= 511 && rfield(a, id, F_SPEED) <= 32767 &&
	rfield(a, id, F_PADDING) == 0;
}

int
sc64_pool_get_row(const Sc64Arena * a, uint8_t id, Sc64ProjectileRow * out)
{
    if (!arena_ok(a) || !out || id >= SC64_POOL_CAPACITY ||
	!row_storage_widths_valid(a, id))
	return 0;
    Sc64ProjectileRow r;
    memset(&r, 0, sizeof r);
    r.x_q8_source = (int32_t) ((uint32_t) rfield(a, id, F_XLO) |
			       ((uint32_t) rfield(a, id, F_XHI) << 16));
    r.y_q8_source = (int32_t) ((uint32_t) rfield(a, id, F_YLO) |
			       ((uint32_t) rfield(a, id, F_YHI) << 16));
    r.destination_x_source = (int16_t) rfield(a, id, F_DX);
    r.destination_y_source = (int16_t) rfield(a, id, F_DY);
    r.next_speed = rfield(a, id, F_SPEED);
    r.turn_rate_raw = rfield(a, id, F_TURN);
    r.heading = (uint8_t) rfield(a, id, F_HEADING);
    r.next_velocity_direction = (uint8_t) rfield(a, id, F_VDIR);
    r.target_ref = rfield(a, id, F_TARGET);
    r.owner_ref = (uint8_t) rfield(a, id, F_OWNER);
    r.remaining_time = (int16_t) rfield(a, id, F_TIMER);
    r.state = (uint8_t) rfield(a, id, F_STATE);
    r.order_signal_bit0 = (uint8_t) rfield(a, id, F_SIGNAL);
    r.movement_flags_low4 = (uint8_t) rfield(a, id, F_MOVE);
    r.hit_flags_low2 = (uint8_t) rfield(a, id, F_HIT);
    r.script_phase = (uint8_t) rfield(a, id, F_PHASE);
    r.raw_source_owner = (uint8_t) rfield(a, id, F_RAWOWNER);
    r.pending_damage_event = (uint8_t) rfield(a, id, F_PENDING);
    r.next_link = (uint8_t) rfield(a, id, F_NEXT);
    r.launch_layer_plus1 = (uint8_t) rfield(a, id, F_LAYER);
    *out = r;
    return 1;
}
int
sc64_pool_set_row(Sc64Arena * a, uint8_t id, const Sc64ProjectileRow * r)
{
    if (!arena_ok(a) || !r || rd(a, H_SIZE) > 100 || id >= rd(a, H_SIZE))
	return 0;
    sfield(a, id, F_XLO, (uint16_t) r->x_q8_source);
    sfield(a, id, F_XHI, (uint16_t) ((uint32_t) r->x_q8_source >> 16));
    sfield(a, id, F_YLO, (uint16_t) r->y_q8_source);
    sfield(a, id, F_YHI, (uint16_t) ((uint32_t) r->y_q8_source >> 16));
    sfield(a, id, F_DX, (uint16_t) r->destination_x_source);
    sfield(a, id, F_DY, (uint16_t) r->destination_y_source);
    sfield(a, id, F_SPEED, r->next_speed);
    sfield(a, id, F_TURN, r->turn_rate_raw);
    sfield(a, id, F_HEADING, r->heading);
    sfield(a, id, F_VDIR, r->next_velocity_direction);
    sfield(a, id, F_TARGET, r->target_ref);
    sfield(a, id, F_OWNER, r->owner_ref);
    sfield(a, id, F_TIMER, (uint16_t) r->remaining_time);
    sfield(a, id, F_STATE, r->state);
    sfield(a, id, F_SIGNAL, r->order_signal_bit0);
    sfield(a, id, F_MOVE, r->movement_flags_low4);
    sfield(a, id, F_HIT, r->hit_flags_low2);
    sfield(a, id, F_PHASE, r->script_phase);
    sfield(a, id, F_RAWOWNER, r->raw_source_owner);
    sfield(a, id, F_PENDING, r->pending_damage_event);
    sfield(a, id, F_LAYER, r->launch_layer_plus1);
    return 1;
}
static int
fits_s(int32_t v, unsigned b)
{
    int32_t	    lo = -(1 << (b - 1)), hi = (1 << (b - 1)) - 1;
    return v >= lo && v <= hi;
}
static int
validrow(const Sc64ProjectileRow * r)
{
    return fits_s(r->x_q8_source, 22) &&
	fits_s(r->y_q8_source, 22) &&
	fits_s(r->destination_x_source, 14) &&
	fits_s(r->destination_y_source, 14) &&
	r->next_speed <= 21333 && r->turn_rate_raw < 512 &&
	(r->target_ref <= 399 || r->target_ref == 511) &&
	(r->owner_ref <= 239 || r->owner_ref == 255) &&
	r->remaining_time >= -1 && r->remaining_time <= 255 &&
	r->state <= 3 && r->order_signal_bit0 <= 1 &&
	r->movement_flags_low4 <= 15 && r->hit_flags_low2 <= 3 &&
	r->script_phase <= 3 && r->raw_source_owner <= 11 &&
	r->pending_damage_event <= 1 && r->next_link <= SC64_END &&
	r->launch_layer_plus1 <= SC64_PROJECTILE_MAX_LAYER + 1u &&
	(r->state != 3 || !r->launch_layer_plus1);
}

/* Called only after verifying the active chain in pool_validate. */
static int pool_has_launch_layer(const Sc64Arena *a)
{
    for (uint8_t id = head(a, H_ACTIVE); id != SC64_END;
         id = (uint8_t)rfield(a, id, F_NEXT))
        if (rfield(a, id, F_LAYER)) return 1;
    return 0;
}

/* The live arena is already validated and exclusively owned. When there
 * is room to reuse a row, check the two list splices before mutating them.
 * Growth and suspicious links use the full validator. */
static int live_acquire_links(const Sc64Arena *a)
{
    if (!arena_ok(a)) return 0;
    unsigned size = rd(a, H_SIZE);
    unsigned active = rd(a, H_ACTIVE_COUNT);
    unsigned free = rd(a, H_FREE_COUNT);
    unsigned fh = rd(a, H_FREE), ftail = rd(a, H_FREE_TAIL);
    unsigned ah = rd(a, H_ACTIVE), atail = rd(a, H_ACTIVE_TAIL);
    Sc64ProjectileRow row;
    if (size > SC64_POOL_CAPACITY || size % SC64_POOL_GROWTH ||
        active + free != size || rd(a, H_RESERVED) || !free ||
        (free == 1 && size < SC64_POOL_CAPACITY) ||
        fh >= size || ftail >= size || (fh == ftail) != (free == 1) ||
        rfield(a, fh, F_PREV) != SC64_END ||
        rfield(a, ftail, F_NEXT) != SC64_END || rfield(a, ftail, F_STATE) != 3 ||
        !sc64_pool_get_row(a, (uint8_t)fh, &row) || !validrow(&row) || row.state != 3)
        return 0;
    unsigned next = row.next_link;
    if (free == 1 ? next != SC64_END :
        next >= size || next == fh || (next == ftail) != (free == 2) ||
        rfield(a, next, F_PREV) != fh || rfield(a, next, F_STATE) != 3)
        return 0;
    if (!active) return ah == SC64_END && atail == SC64_END;
    if (ah >= size || atail >= size || (ah == atail) != (active == 1) ||
        rfield(a, ah, F_PREV) != SC64_END || rfield(a, atail, F_NEXT) != SC64_END ||
        rfield(a, ah, F_STATE) >= 3 || rfield(a, atail, F_STATE) >= 3)
        return 0;
    next = rfield(a, ah, F_NEXT);
    if (active == 1) return next == SC64_END;
    return next < size && next != ah && (next == atail) == (active == 2) &&
        rfield(a, next, F_PREV) == ah && rfield(a, next, F_STATE) < 3;
}

int sc64_pool_acquire_live(Sc64Arena *a, uint8_t *ordinal, uint8_t *native_index)
{
    if (!ordinal || !native_index) return 0;
    if (!live_acquire_links(a)) return sc64_pool_acquire(a, ordinal, native_index);
    return pool_acquire_linked(a, ordinal, native_index);
}

/* In a validated, exclusively owned arena the rest of both chains is already
 * known-good. Check all endpoints and every link this splice will overwrite
 * before touching storage. A suspect arena takes the original strict path. */
static int live_retire_links(const Sc64Arena *a, uint8_t id)
{
    unsigned size, active, free, ah, atail, fh, ftail, prev, next, second;
    Sc64ProjectileRow row;
    if (!arena_ok(a)) return 0;
    size = rd(a, H_SIZE);
    active = rd(a, H_ACTIVE_COUNT);
    free = rd(a, H_FREE_COUNT);
    ah = rd(a, H_ACTIVE); atail = rd(a, H_ACTIVE_TAIL);
    fh = rd(a, H_FREE); ftail = rd(a, H_FREE_TAIL);
    if (size > SC64_POOL_CAPACITY || size % SC64_POOL_GROWTH || id >= size ||
        !active || active + free != size || rd(a, H_RESERVED) ||
        ah >= size || atail >= size || (ah == atail) != (active == 1) ||
        rfield(a, ah, F_PREV) != SC64_END || rfield(a, atail, F_NEXT) != SC64_END ||
        rfield(a, ah, F_STATE) >= 3 || rfield(a, atail, F_STATE) >= 3 ||
        !sc64_pool_get_row(a, id, &row) || !validrow(&row) || row.state == 3)
        return 0;
    prev = rfield(a, id, F_PREV); next = row.next_link;
    if ((prev == SC64_END) != (id == ah) || (next == SC64_END) != (id == atail) ||
        (prev != SC64_END && next != SC64_END && (prev == next || active < 3)) ||
        (prev != SC64_END && (prev >= size || prev == id ||
            rfield(a, prev, F_NEXT) != id || rfield(a, prev, F_STATE) >= 3)) ||
        (next != SC64_END && (next >= size || next == id ||
            rfield(a, next, F_PREV) != id || rfield(a, next, F_STATE) >= 3)))
        return 0;
    if (!free) return fh == SC64_END && ftail == SC64_END;
    if (fh >= size || ftail >= size || (fh == ftail) != (free == 1) ||
        rfield(a, fh, F_STATE) != 3 || rfield(a, ftail, F_STATE) != 3 ||
        rfield(a, fh, F_PREV) != SC64_END || rfield(a, ftail, F_NEXT) != SC64_END)
        return 0;
    second = rfield(a, fh, F_NEXT);
    return free == 1 ? second == SC64_END :
        second < size && second != fh && (second == ftail) == (free == 2) &&
        rfield(a, second, F_STATE) == 3 && rfield(a, second, F_PREV) == fh;
}

int sc64_pool_retire_live(Sc64Arena *a, uint8_t ordinal)
{
    if (!live_retire_links(a, ordinal)) return sc64_pool_retire(a, ordinal);
    pool_retire_linked(a, ordinal);
    return 1;
}

static int
canonical_unallocated_row(const Sc64ProjectileRow * r)
{
    return r->x_q8_source == 0 && r->y_q8_source == 0 &&
	r->destination_x_source == 0 && r->destination_y_source == 0 &&
	r->next_speed == 0 && r->turn_rate_raw == 0 && r->heading == 0 &&
	r->next_velocity_direction == 0 && r->target_ref == 511 &&
	r->owner_ref == 255 && r->remaining_time == 0 && r->state == 0 &&
	r->order_signal_bit0 == 0 && r->movement_flags_low4 == 0 &&
	r->hit_flags_low2 == 0 && r->script_phase == 0 &&
	r->raw_source_owner == 0 && r->pending_damage_event == 0 &&
	r->next_link == 0 && r->launch_layer_plus1 == 0;
}

int
sc64_pool_validate(const Sc64Arena * a)
{
    if (!arena_ok(a))
	return 0;
    if (rd(a, H_ACTIVE) > SC64_END || rd(a, H_FREE) > SC64_END ||
	rd(a, H_SIZE) > 100 || (rd(a, H_SIZE) % SC64_POOL_GROWTH) != 0 ||
	rd(a, H_ACTIVE_TAIL) > SC64_END ||
	rd(a, H_FREE_TAIL) > SC64_END || rd(a, H_ACTIVE_COUNT) > 100 ||
	rd(a, H_FREE_COUNT) > 100 || rd(a, H_RESERVED) != 0)
	return 0;
    uint8_t	    size = (uint8_t) rd(a, H_SIZE);
    uint8_t	    seen[100] = {0};
    for (unsigned li = 0; li < 2; li++) {
	uint8_t		id = head(a, li ? H_FREE : H_ACTIVE), prev = SC64_END;
	unsigned	steps = 0;
	while (id != SC64_END) {
	    if (id >= size || seen[id] || ++steps > size ||
		!row_storage_widths_valid(a, id) || rfield(a, id, F_PREV) != prev)
		return 0;
	    seen[id] = (uint8_t) (li ? 2 : 1);
	    if ((li && rfield(a, id, F_STATE) != 3) || (!li && rfield(a, id, F_STATE) == 3))
		return 0;
	    prev = id;
	    id = (uint8_t) rfield(a, id, F_NEXT);
	} if (steps != head(a, li ? H_FREE_COUNT : H_ACTIVE_COUNT) || prev != head(a, li ? H_FREE_TAIL : H_ACTIVE_TAIL))
	    return 0;
    } for (uint8_t i = 0; i < size; i++) {
	Sc64ProjectileRow r;
	if (!seen[i] || !sc64_pool_get_row(a, i, &r) || !validrow(&r) || (seen[i] == 2) != (r.state == 3))
	    return 0;
    } for (uint8_t i = size; i < 100; i++) {
	Sc64ProjectileRow r;
	if (!sc64_pool_get_row(a, i, &r))
	    return 0;		/* Unallocated storage is excluded from
				 * serialization and must be canonical. */
	if (!canonical_unallocated_row(&r) || rfield(a, i, F_PREV) != SC64_END ||
            rfield(a, i, F_PADDING) != 0)
	    return 0;
    } return 1;
}
static void
putbits(uint8_t * b, size_t * pos, uint32_t v, unsigned n)
{
    for (unsigned i = 0; i < n; i++, (*pos)++)
	if ((v >> i) & 1u)
	    b[*pos >> 3] |= (uint8_t) (1u << (*pos & 7));
}
static uint32_t getbits(const uint8_t * b, size_t * pos, unsigned n){
    uint32_t	    v = 0;
    for (unsigned i = 0; i < n; i++, (*pos)++)
	v |= (uint32_t) ((b[*pos >> 3] >> (*pos & 7)) & 1u) << i;
    return v;
}
static void
putrow(uint8_t * b, size_t * pos, const Sc64ProjectileRow * r, int layered)
{
    putbits(b, pos, (uint32_t) r->x_q8_source, 22);
    putbits(b, pos, (uint32_t) r->y_q8_source, 22);
    putbits(b, pos, (uint16_t) r->destination_x_source, 14);
    putbits(b, pos, (uint16_t) r->destination_y_source, 14);
    putbits(b, pos, r->next_speed, 15);
    putbits(b, pos, r->turn_rate_raw, 9);
    putbits(b, pos, r->heading, 8);
    putbits(b, pos, r->next_velocity_direction, 8);
    putbits(b, pos, r->target_ref, 9);
    putbits(b, pos, r->owner_ref, 8);
    putbits(b, pos, (uint16_t) r->remaining_time, 9);
    putbits(b, pos, r->state, 2);
    putbits(b, pos, r->order_signal_bit0, 1);
    putbits(b, pos, r->movement_flags_low4, 4);
    putbits(b, pos, r->hit_flags_low2, 2);
    putbits(b, pos, r->script_phase, 2);
    putbits(b, pos, r->raw_source_owner, 4);
    putbits(b, pos, r->pending_damage_event, 1);
    putbits(b, pos, r->next_link, 7);
    putbits(b, pos, r->launch_layer_plus1, layered ? 5 : 3);
}
static int32_t signv(uint32_t v, unsigned n){
    uint32_t	    sign = 1u << (n - 1);
    return (int32_t) ((v ^ sign) - sign);
}
static Sc64ProjectileRow getrow(const uint8_t * b, size_t * pos, int layered){
    Sc64ProjectileRow r;
    memset(&r, 0, sizeof r);
    r.x_q8_source = signv(getbits(b, pos, 22), 22);
    r.y_q8_source = signv(getbits(b, pos, 22), 22);
    r.destination_x_source = (int16_t) signv(getbits(b, pos, 14), 14);
    r.destination_y_source = (int16_t) signv(getbits(b, pos, 14), 14);
    r.next_speed = (uint16_t) getbits(b, pos, 15);
    r.turn_rate_raw = (uint16_t) getbits(b, pos, 9);
    r.heading = (uint8_t) getbits(b, pos, 8);
    r.next_velocity_direction = (uint8_t) getbits(b, pos, 8);
    r.target_ref = (uint16_t) getbits(b, pos, 9);
    r.owner_ref = (uint8_t) getbits(b, pos, 8);
    r.remaining_time = (int16_t) signv(getbits(b, pos, 9), 9);
    r.state = (uint8_t) getbits(b, pos, 2);
    r.order_signal_bit0 = (uint8_t) getbits(b, pos, 1);
    r.movement_flags_low4 = (uint8_t) getbits(b, pos, 4);
    r.hit_flags_low2 = (uint8_t) getbits(b, pos, 2);
    r.script_phase = (uint8_t) getbits(b, pos, 2);
    r.raw_source_owner = (uint8_t) getbits(b, pos, 4);
    r.pending_damage_event = (uint8_t) getbits(b, pos, 1);
    r.next_link = (uint8_t) getbits(b, pos, 7);
    r.launch_layer_plus1 = (uint8_t) getbits(b, pos, layered ? 5 : 3);
    return r;
}

static int fixed_read_row(const uint8_t *in, uint8_t id, int layered, Sc64ProjectileRow *out)
{
    size_t byte = 3u + (size_t)id * 21u, pos = byte * 8u;
    Sc64ProjectileRow row = getrow(in, &pos, layered);
    if ((in[byte + 20u] & (layered ? 0xc0u : 0xf0u)) ||
        (!layered && row.launch_layer_plus1) || !validrow(&row)) return 0;
    *out = row;
    return 1;
}
int
sc64_codec_encode(const Sc64Arena * a, uint8_t * out, size_t n)
{
    if (!arena_ok(a) || !out || n != SC64_CODEC_BYTES ||
        packet_overlaps_arena(a, out, n) || !sc64_pool_validate(a))
        return 0;
    memset(out, 0, n);
    size_t	    p = 0;
    putbits(out, &p, head(a, H_ACTIVE), 7);
    putbits(out, &p, head(a, H_FREE), 7);
    putbits(out, &p, head(a, H_SIZE), 7);
    int layered = pool_has_launch_layer(a);
    putbits(out, &p, (unsigned)layered, 3);
    for (uint8_t i = 0; i < 100; i++) {
	Sc64ProjectileRow r;
	if (!sc64_pool_get_row(a, i, &r))
	    return 0;
	size_t		byte = 3u + (size_t) i * 21u;
	p = byte * 8;
	putrow(out, &p, &r, layered);
	if (p != byte * 8 + (layered ? 166u : 164u))
	    return 0;
    } return 1;
}
static int
packet_preflight(const uint8_t *in, size_t n,
                   uint8_t next_ids[SC64_POOL_CAPACITY], uint8_t heads[3])
{
    if (!in || n != SC64_CODEC_BYTES)
        return 0;

    size_t	    pos = 0;
    uint8_t	    active_head = (uint8_t) getbits(in, &pos, 7);
    uint8_t	    free_head = (uint8_t) getbits(in, &pos, 7);
    uint8_t	    count = (uint8_t) getbits(in, &pos, 7);
    unsigned version = getbits(in, &pos, 3);
    if (version > 1 || count > SC64_POOL_CAPACITY ||
	(count % SC64_POOL_GROWTH) != 0)
	return 0;

    /* Bounded preflight state: 100 next IDs, states and membership marks. */
    uint8_t	    states[SC64_POOL_CAPACITY];
    uint8_t	    membership[SC64_POOL_CAPACITY] = {0};
    int have_layer = 0;
    for (uint8_t i = 0; i < SC64_POOL_CAPACITY; ++i) {
	Sc64ProjectileRow row;
	if (!fixed_read_row(in, i, version != 0, &row))
	    return 0;
	have_layer |= row.launch_layer_plus1 != 0;
	next_ids[i] = row.next_link;
	states[i] = row.state;
	if (i < count) {
	    if (row.state == 3 && row.next_link > SC64_END)
		return 0;
	} else if (!canonical_unallocated_row(&row)) {
	    return 0;
	}
    }

    if ((version != 0) != have_layer) return 0;
    for (unsigned list = 0; list < 2; ++list) {
	uint8_t		id = list ? free_head : active_head;
	unsigned	steps = 0;
	while (id != SC64_END) {
	    if (id >= count || membership[id] || ++steps > count)
		return 0;
	    membership[id] = (uint8_t) (list ? 2 : 1);
	    if ((list && states[id] != 3) || (!list && states[id] == 3))
		return 0;
	    id = next_ids[id];
	}
    }
    for (uint8_t i = 0; i < count; ++i) {
	if (!membership[i] || ((membership[i] == 2) != (states[i] == 3)))
	    return 0;
    }
    heads[0]=active_head; heads[1]=free_head; heads[2]=count;
    return 1;
}

/* All pre-write decoder checks, shared with the caller's candidate preflight. */
static int
target_preflight(const Sc64Arena *a, const uint8_t *in, size_t n,
                 uint8_t next_ids[SC64_POOL_CAPACITY], uint8_t heads[3])
{
    return arena_ok(a) && in && n==SC64_CODEC_BYTES &&
        !packet_overlaps_arena(a,in,n) && packet_preflight(in,n,next_ids,heads);
}

int
sc64_codec_validate_target(const Sc64Arena *a, const uint8_t *in, size_t n)
{
    uint8_t next_ids[SC64_POOL_CAPACITY], heads[3];
    return target_preflight(a,in,n,next_ids,heads);
}

int
sc64_codec_validate(const uint8_t *in, size_t n)
{
    uint8_t next_ids[SC64_POOL_CAPACITY], heads[3];
    return packet_preflight(in,n,next_ids,heads);
}

int
sc64_codec_allocated_count(const uint8_t *in, size_t n)
{
    if (!in || n!=SC64_CODEC_BYTES || (in[2]&FIXED_UNKNOWN_FLAGS)) return -1;
    unsigned count=((unsigned)in[1]>>6)|(((unsigned)in[2]&31u)<<2);
    return count<=SC64_POOL_CAPACITY && count%SC64_POOL_GROWTH==0 ?
        (int)count : -1;
}

int
sc64_codec_get_row(const uint8_t *in, size_t n, uint8_t id,
                    Sc64ProjectileRow *out)
{
    if (!in || !out || n!=SC64_CODEC_BYTES || id>=SC64_POOL_CAPACITY ||
        (in[2]&FIXED_UNKNOWN_FLAGS))
        return 0;
    return fixed_read_row(in, id, (in[2]&FIXED_LAYER_FLAG) != 0, out);
}

int
sc64_codec_decode(Sc64Arena * a, const uint8_t * in, size_t n)
{
    uint8_t next_ids[SC64_POOL_CAPACITY], heads[3];
    if (!target_preflight(a,in,n,next_ids,heads)) return 0;
    uint8_t active_head=heads[0], free_head=heads[1], count=heads[2];
    int layered = (in[2]&FIXED_LAYER_FLAG) != 0;

    /* Commit by decoding one local row at a time after complete preflight. */
    for (size_t i = 0; i < SC64_ARENA_WORDS; ++i)
	wr(a, i, 0);
    sethead(a, H_ACTIVE, active_head);
    sethead(a, H_FREE, free_head);
    sethead(a, H_SIZE, count);
    sethead(a, H_ACTIVE_TAIL, SC64_END);
    sethead(a, H_FREE_TAIL, SC64_END);

    for (uint8_t i = 0; i < SC64_POOL_CAPACITY; ++i) {
	size_t		row_start = (3u + (size_t) i * 21u) * 8u;
	size_t pos = row_start;
	Sc64ProjectileRow row = getrow(in, &pos, layered);
	if (i < count) {
	    (void)sc64_pool_set_row(a, i, &row);
	    sfield(a, i, F_NEXT, row.next_link);
	} else {
	    sfield(a, i, F_TARGET, 511);
	    sfield(a, i, F_OWNER, 255);
	    sfield(a, i, F_STATE, 0);
	    sfield(a, i, F_NEXT, 0);
	}
	sfield(a, i, F_PREV, SC64_END);
    }

    uint8_t	    counts[2] = {0, 0};
    for (unsigned list = 0; list < 2; ++list) {
	uint8_t		id = list ? free_head : active_head;
	uint8_t		previous = SC64_END;
	while (id != SC64_END) {
	    sfield(a, id, F_PREV, previous);
	    previous = id;
	    id = next_ids[id];
	    ++counts[list];
	}
	sethead(a, list ? H_FREE_TAIL : H_ACTIVE_TAIL, previous);
    }
    sethead(a, H_ACTIVE_COUNT, counts[0]);
    sethead(a, H_FREE_COUNT, counts[1]);
    return sc64_pool_validate(a);
}

#ifdef SC64_INTERCEPTOR_COMPACT_CODEC
/* Keep the compact bit layout independent so existing saved-game builds
 * retain the fixed codec's ARM code. The active fixture exercises both
 * implementations and the shared row domain. */
static void
putrow_fields(uint8_t * b, size_t * pos, const Sc64ProjectileRow * r, int layered)
{
    unsigned dx = (uint16_t)r->destination_x_source;
    unsigned dy = (uint16_t)r->destination_y_source;
    unsigned owner = r->raw_source_owner;
    if (layered) {
        unsigned layer = r->launch_layer_plus1;
        dx |= (layer & 3u) << 12;
        dy |= (layer & 12u) << 10;
        owner |= (layer & 16u) >> 1;
    }
    putbits(b, pos, (uint32_t) r->x_q8_source, 22);
    putbits(b, pos, (uint32_t) r->y_q8_source, 22);
    putbits(b, pos, dx, 14);
    putbits(b, pos, dy, 14);
    putbits(b, pos, r->next_speed, 15);
    putbits(b, pos, r->turn_rate_raw, 9);
    putbits(b, pos, r->heading, 8);
    putbits(b, pos, r->next_velocity_direction, 8);
    putbits(b, pos, r->target_ref, 9);
    putbits(b, pos, r->owner_ref, 8);
    putbits(b, pos, (uint16_t) r->remaining_time, 9);
    putbits(b, pos, r->state, 2);
    putbits(b, pos, r->order_signal_bit0, 1);
    putbits(b, pos, r->movement_flags_low4, 4);
    putbits(b, pos, r->hit_flags_low2, 2);
    putbits(b, pos, r->script_phase, 2);
    putbits(b, pos, owner, 4);
    putbits(b, pos, r->pending_damage_event, 1);
}
static Sc64ProjectileRow getrow_fields(const uint8_t * b, size_t * pos, int layered){
    Sc64ProjectileRow r;
    memset(&r, 0, sizeof r);
    r.x_q8_source = signv(getbits(b, pos, 22), 22);
    r.y_q8_source = signv(getbits(b, pos, 22), 22);
    unsigned dx = getbits(b, pos, 14), dy = getbits(b, pos, 14);
    r.next_speed = (uint16_t) getbits(b, pos, 15);
    r.turn_rate_raw = (uint16_t) getbits(b, pos, 9);
    r.heading = (uint8_t) getbits(b, pos, 8);
    r.next_velocity_direction = (uint8_t) getbits(b, pos, 8);
    r.target_ref = (uint16_t) getbits(b, pos, 9);
    r.owner_ref = (uint8_t) getbits(b, pos, 8);
    r.remaining_time = (int16_t) signv(getbits(b, pos, 9), 9);
    r.state = (uint8_t) getbits(b, pos, 2);
    r.order_signal_bit0 = (uint8_t) getbits(b, pos, 1);
    r.movement_flags_low4 = (uint8_t) getbits(b, pos, 4);
    r.hit_flags_low2 = (uint8_t) getbits(b, pos, 2);
    r.script_phase = (uint8_t) getbits(b, pos, 2);
    unsigned owner = getbits(b, pos, 4);
    r.pending_damage_event = (uint8_t) getbits(b, pos, 1);
    if (layered) {
        r.destination_x_source = (int16_t)(dx & COMPACT_DEST_MASK);
        r.destination_y_source = (int16_t)(dy & COMPACT_DEST_MASK);
        r.raw_source_owner = (uint8_t)(owner & 7u);
        r.launch_layer_plus1 = (uint8_t)((dx >> 12) | ((dy >> 12) << 2) | ((owner & 8u) << 1));
    } else {
        r.destination_x_source = (int16_t)signv(dx, 14);
        r.destination_y_source = (int16_t)signv(dy, 14);
        r.raw_source_owner = (uint8_t)owner;
    }
    return r;
}
/* Compact packet:
 *   allocated groups and optional layer flag (byte), active count (byte)
 *   all allocated ordinals, active list then free list (7 bits each)
 *   active row fields in that active order (154 bits each)
 *   zero high bits to byte alignment
 * The list permutation encodes both heads, tails and links losslessly.
 * Free fields are canonical. With the layer flag, the two destination fields
 * use unsigned 12-bit map coordinates and the owner uses three bits. Their
 * five excess bits carry launch depth. Unflagged packets retain the full
 * signed destination and raw-owner domains of the generic compact codec. */
enum { COMPACT_HEADER_BYTES = 2, COMPACT_ROW_BITS = 154 };

static unsigned compact_max_allocated(unsigned cap)
{
    if (!cap || cap >= SC64_POOL_CAPACITY) return cap;
    /* Growth at one remaining free slot happens before acquisition. If the
     * new active count is at most cap, the old allocated count was <= cap. */
    return (cap / SC64_POOL_GROWTH + 1u) * SC64_POOL_GROWTH;
}

static size_t compact_bytes(unsigned allocated, unsigned active)
{
    return COMPACT_HEADER_BYTES + (allocated * 7u + active * COMPACT_ROW_BITS + 7u) / 8u;
}

size_t sc64_compact_codec_max_bytes(unsigned cap)
{
    return cap <= SC64_POOL_CAPACITY ? compact_bytes(compact_max_allocated(cap), cap) : 0;
}

static int compact_acquire_allowed(const Sc64Arena *a, unsigned cap)
{
    return arena_ok(a) && cap <= SC64_POOL_CAPACITY &&
        rd(a, H_SIZE) <= compact_max_allocated(cap) && rd(a, H_ACTIVE_COUNT) < cap;
}

int sc64_compact_pool_acquire(Sc64Arena *a, unsigned cap,
                              uint8_t *ordinal, uint8_t *native_index)
{
    return compact_acquire_allowed(a, cap) &&
        sc64_pool_acquire(a, ordinal, native_index);
}

int sc64_compact_pool_acquire_live(Sc64Arena *a, unsigned cap,
                                   uint8_t *ordinal, uint8_t *native_index)
{
    return compact_acquire_allowed(a, cap) &&
        sc64_pool_acquire_live(a, ordinal, native_index);
}

static Sc64ProjectileRow compact_free_row(void)
{
    Sc64ProjectileRow row = {0};
    row.state = 3;
    row.target_ref = 511;
    row.owner_ref = 255;
    return row;
}

int sc64_compact_pool_retire(Sc64Arena *a, uint8_t ordinal)
{
    Sc64ProjectileRow row = compact_free_row();
    if (!sc64_pool_retire(a, ordinal)) return 0;
    return sc64_pool_set_row(a, ordinal, &row);
}

int sc64_compact_pool_retire_live(Sc64Arena *a, uint8_t ordinal)
{
    Sc64ProjectileRow row = compact_free_row();
    if (!sc64_pool_retire_live(a, ordinal)) return 0;
    return sc64_pool_set_row(a, ordinal, &row);
}

size_t sc64_compact_codec_encode(const Sc64Arena *a, unsigned cap,
                                  uint8_t *out, size_t out_capacity)
{
    if (!arena_ok(a) || !out || cap > SC64_POOL_CAPACITY || !sc64_pool_validate(a))
        return 0;
    unsigned allocated = head(a, H_SIZE), active = head(a, H_ACTIVE_COUNT);
    size_t n = compact_bytes(allocated, active);
    if (allocated > compact_max_allocated(cap) || active > cap || n > out_capacity ||
        packet_overlaps_arena(a, out, n))
        return 0;

    for (uint8_t id = head(a, H_FREE); id != SC64_END;
         id = (uint8_t)rfield(a, id, F_NEXT)) {
        Sc64ProjectileRow row;
        if (!sc64_pool_get_row(a, id, &row)) return 0;
        row.state = row.next_link = 0;
        if (!canonical_unallocated_row(&row)) return 0;
    }
    int layered = pool_has_launch_layer(a);
    if (layered) {
        for (uint8_t id = head(a, H_ACTIVE); id != SC64_END;
             id = (uint8_t)rfield(a, id, F_NEXT))
            if (rfield(a, id, F_DX) > COMPACT_DEST_MASK ||
                rfield(a, id, F_DY) > COMPACT_DEST_MASK ||
                rfield(a, id, F_RAWOWNER) >= 8u) return 0;
    }

    memset(out, 0, n);
    out[0] = (uint8_t)(allocated / SC64_POOL_GROWTH | (layered ? COMPACT_LAYER_FLAG : 0));
    out[1] = (uint8_t)active;
    size_t pos = COMPACT_HEADER_BYTES * 8u;
    for (unsigned list = 0; list < 2; ++list)
        for (uint8_t id = head(a, list ? H_FREE : H_ACTIVE); id != SC64_END;
             id = (uint8_t)rfield(a, id, F_NEXT))
            putbits(out, &pos, id, 7);

    for (uint8_t id = head(a, H_ACTIVE); id != SC64_END;
         id = (uint8_t)rfield(a, id, F_NEXT)) {
        Sc64ProjectileRow row;
        if (!sc64_pool_get_row(a, id, &row)) return 0;
        putrow_fields(out, &pos, &row, layered);
    }
    return n;
}

typedef struct {
    uint8_t allocated, active, layered, ids[SC64_POOL_CAPACITY];
    size_t row_pos;
} CompactPacket;

static Sc64ProjectileRow compact_active_row(const uint8_t *in, size_t *pos,
                                            const CompactPacket *packet,
                                            unsigned index)
{
    Sc64ProjectileRow row = getrow_fields(in, pos, packet->layered);
    row.next_link = index + 1u < packet->active ? packet->ids[index + 1u] : SC64_END;
    return row;
}

static int compact_preflight(const uint8_t *in, size_t n, unsigned cap,
                             CompactPacket *packet)
{
    if (!in || n < COMPACT_HEADER_BYTES || cap > SC64_POOL_CAPACITY)
        return 0;
    unsigned groups = in[0] & ~COMPACT_LAYER_FLAG;
    if (groups > SC64_POOL_CAPACITY / SC64_POOL_GROWTH) return 0;
    unsigned allocated = groups * SC64_POOL_GROWTH, active = in[1];
    if (allocated > compact_max_allocated(cap) || active > cap ||
        active > allocated || n != compact_bytes(allocated, active))
        return 0;
    packet->allocated = (uint8_t)allocated;
    packet->active = (uint8_t)active;
    packet->layered = (in[0] & COMPACT_LAYER_FLAG) != 0;

    size_t pos = COMPACT_HEADER_BYTES * 8u;
    uint8_t seen[(SC64_POOL_CAPACITY + 7u) / 8u] = {0};
    for (unsigned i = 0; i < allocated; ++i) {
        unsigned id = getbits(in, &pos, 7);
        if (id >= allocated || (seen[id / 8u] & (1u << (id % 8u)))) return 0;
        seen[id / 8u] |= (uint8_t)(1u << (id % 8u));
        packet->ids[i] = (uint8_t)id;
    }
    packet->row_pos = pos;
    int have_layer = 0;
    for (unsigned i = 0; i < active; ++i) {
        Sc64ProjectileRow row = compact_active_row(in, &pos, packet, i);
        if (row.state == 3 || !validrow(&row)) return 0;
        have_layer |= row.launch_layer_plus1 != 0;
    }
    return packet->layered == have_layer && getbits(in, &pos, (unsigned)(n * 8u - pos)) == 0;
}

int sc64_compact_codec_validate(const uint8_t *in, size_t n, unsigned cap,
                                Sc64CompactRowCheck check, void *context)
{
    CompactPacket packet;
    if (!compact_preflight(in, n, cap, &packet)) return 0;
    size_t pos = packet.row_pos;
    for (unsigned i = 0; check && i < packet.active; ++i) {
        Sc64ProjectileRow row = compact_active_row(in, &pos, &packet, i);
        if (!check(context, packet.ids[i], &row)) return 0;
    }
    return 1;
}

int sc64_compact_codec_validate_target(const Sc64Arena *a, const uint8_t *in,
                                       size_t n, unsigned cap,
                                       Sc64CompactRowCheck check, void *context)
{
    return arena_ok(a) && !packet_overlaps_arena(a, in, n) &&
        sc64_compact_codec_validate(in, n, cap, check, context);
}

int sc64_compact_codec_decode(Sc64Arena *a, const uint8_t *in, size_t n,
                              unsigned cap)
{
    CompactPacket packet;
    if (!arena_ok(a) || packet_overlaps_arena(a, in, n) ||
        !compact_preflight(in, n, cap, &packet)) return 0;

    /* Preflight completed before the first halfword write. The permutation
     * supplies every allocated slot once, so nothing can fail in commit. */
    sc64_pool_init(a);
    sethead(a, H_SIZE, packet.allocated);
    size_t pos = packet.row_pos;
    for (unsigned list = 0; list < 2; ++list) {
        unsigned first = list ? packet.active : 0;
        unsigned end = list ? packet.allocated : packet.active;
        if (first == end) continue;
        sethead(a, list ? H_FREE : H_ACTIVE, packet.ids[first]);
        sethead(a, list ? H_FREE_TAIL : H_ACTIVE_TAIL, packet.ids[end - 1u]);
        sethead(a, list ? H_FREE_COUNT : H_ACTIVE_COUNT, (uint8_t)(end - first));
        for (unsigned i = first; i < end; ++i) {
            uint8_t id = packet.ids[i];
            Sc64ProjectileRow row = list ? compact_free_row() : compact_active_row(in, &pos, &packet, i);
            sc64_pool_set_row(a, id, &row);
            sfield(a, id, F_PREV, i == first ? SC64_END : packet.ids[i - 1u]);
            sfield(a, id, F_NEXT, i + 1u == end ? SC64_END : packet.ids[i + 1u]);
        }
    }
    return 1;
}
#endif
