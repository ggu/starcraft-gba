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
    F_RESERVED = 22, F_PADDING = 23
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
int
sc64_pool_acquire(Sc64Arena * a, uint8_t * ordinal, uint8_t * native_index)
{
    if (!arena_ok(a) || !ordinal || !native_index || !sc64_pool_validate(a))
	return 0;
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
sc64_pool_retire(Sc64Arena * a, uint8_t id)
{
    if (!arena_ok(a) || !sc64_pool_validate(a) || id >= head(a, H_SIZE) || rfield(a, id, F_STATE) == 3)
	return 0;
    list_erase(a, H_ACTIVE, H_ACTIVE_TAIL, H_ACTIVE_COUNT, id);
    sfield(a, id, F_STATE, 3);
    list_insert_after(a, H_FREE, H_FREE_TAIL, H_FREE_COUNT, id);
    return 1;
}
static int
row_storage_widths_valid(const Sc64Arena * a, uint8_t id)
{
    static const unsigned byte_fields[] = {
	F_STATE, F_HEADING, F_VDIR, F_OWNER, F_SIGNAL, F_MOVE, F_HIT,
	F_PHASE, F_RAWOWNER, F_PENDING, F_NEXT, F_RESERVED
    };
    for (size_t i = 0; i < sizeof byte_fields / sizeof byte_fields[0]; ++i) {
	if (rfield(a, id, byte_fields[i]) > 255)
	    return 0;
    }
    return rfield(a, id, F_STATE) <= 3 &&
	rfield(a, id, F_SIGNAL) <= 1 && rfield(a, id, F_MOVE) <= 15 &&
	rfield(a, id, F_HIT) <= 3 && rfield(a, id, F_PHASE) <= 3 &&
	rfield(a, id, F_PENDING) <= 1 && rfield(a, id, F_NEXT) <= SC64_END &&
	rfield(a, id, F_RESERVED) <= 7 && rfield(a, id, F_TARGET) <= 511 &&
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
    r.reserved_snapshot_bits = (uint8_t) rfield(a, id, F_RESERVED);
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
    sfield(a, id, F_RESERVED, r->reserved_snapshot_bits);
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
	r->reserved_snapshot_bits == 0;
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
	r->next_link == 0 && r->reserved_snapshot_bits == 0;
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
putrow(uint8_t * b, size_t * pos, const Sc64ProjectileRow * r)
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
    putbits(b, pos, r->reserved_snapshot_bits, 3);
}
static int32_t signv(uint32_t v, unsigned n){
    uint32_t	    sign = 1u << (n - 1);
    return (int32_t) ((v ^ sign) - sign);
}
static Sc64ProjectileRow getrow(const uint8_t * b, size_t * pos){
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
    r.reserved_snapshot_bits = (uint8_t) getbits(b, pos, 3);
    return r;
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
    putbits(out, &p, 0, 3);
    for (uint8_t i = 0; i < 100; i++) {
	Sc64ProjectileRow r;
	if (!sc64_pool_get_row(a, i, &r))
	    return 0;
	size_t		byte = 3u + (size_t) i * 21u;
	p = byte * 8;
	putrow(out, &p, &r);
	if (p != byte * 8 + 164)
	    return 0;
    } return 1;
}
int
sc64_codec_decode(Sc64Arena * a, const uint8_t * in, size_t n)
{
    if (!arena_ok(a) || !in || n != SC64_CODEC_BYTES ||
        packet_overlaps_arena(a, in, n))
        return 0;

    size_t	    pos = 0;
    uint8_t	    active_head = (uint8_t) getbits(in, &pos, 7);
    uint8_t	    free_head = (uint8_t) getbits(in, &pos, 7);
    uint8_t	    count = (uint8_t) getbits(in, &pos, 7);
    if (getbits(in, &pos, 3) != 0 || count > SC64_POOL_CAPACITY ||
	(count % SC64_POOL_GROWTH) != 0)
	return 0;

    /* Bounded preflight state: 100 next IDs, states and membership marks. */
    uint8_t	    next_ids[SC64_POOL_CAPACITY];
    uint8_t	    states[SC64_POOL_CAPACITY];
    uint8_t	    membership[SC64_POOL_CAPACITY] = {0};
    for (uint8_t i = 0; i < SC64_POOL_CAPACITY; ++i) {
	size_t		row_start = (3u + (size_t) i * 21u) * 8u;
	pos = row_start;
	Sc64ProjectileRow row = getrow(in, &pos);
	if (pos != row_start + 164u ||
	    (in[3u + (size_t) i * 21u + 20u] & 0xf0u) != 0 ||
	    !validrow(&row))
	    return 0;
	next_ids[i] = row.next_link;
	states[i] = row.state;
	if (i < count) {
	    if (row.state == 3 && row.next_link > SC64_END)
		return 0;
	} else if (!canonical_unallocated_row(&row)) {
	    return 0;
	}
    }

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
	pos = row_start;
	Sc64ProjectileRow row = getrow(in, &pos);
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
