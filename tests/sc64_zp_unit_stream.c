/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Unit-stream save rows (starcraft/sc64_zp_unit_stream.h): exact round
 * trips of random and crowd-shaped unit tables at every field's range
 * limits, canonical-form checks, corruption rejection and overflow refusal. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
#define SC64_RACES 1
#define MAX_UNITS 408
#define SC_RU_COUNT 60
#define SC64_ZPS_DATA_BYTES 7500
typedef struct {
    int hp, maxhp, carry, shield;
    s16 x, y, tx, ty, build, target, path_x, path_y, raw_type, raw_owner;
    u16 cd, work_timer, weapon, path_timer, path_retry, flags, shield_remainder;
    u8 owner, type, alive, selected, order, armor;
} Unit;
#include "../starcraft/sc64_zp_unit_stream.h"

enum { W = 1024, H = 1024 };
static u32 rng = 0x2545f491u;
static u32 next(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }
static int pick(int lo, int hi) { return lo + (int)(next() % (u32)(hi - lo + 1)); }

static Unit table[MAX_UNITS], decoded[MAX_UNITS];
static Sc64ZpUnitStream stream, copy;

static void random_unit(Unit *q)
{
    memset(q, 0, sizeof(*q));
    q->alive = (u8)pick(0, 1);
    q->hp = q->alive ? pick(0, 131071) : pick(-131072, 131071);
    q->maxhp = pick(0, 131071); q->carry = pick(-65536, 65535);
    q->shield = pick(0, 131071);
    q->x = (s16)pick(0, W - 1); q->y = (s16)pick(0, H - 1);
    q->tx = (s16)pick(0, 2047); q->ty = (s16)pick(0, 2047);
    q->build = (s16)pick(-1024, 1023); q->target = (s16)pick(-1, 1023);
    q->path_x = (s16)pick(0, 2047); q->path_y = (s16)pick(0, 2047);
    q->raw_type = (s16)pick(-1, 255); q->raw_owner = (s16)pick(-1, 255);
    q->cd = (u16)pick(0, 65535); q->work_timer = (u16)pick(0, 65535);
    q->weapon = (u16)pick(0, 65535); q->path_timer = (u16)pick(0, 24);
    q->path_retry = (u16)pick(0, 12); q->flags = (u16)pick(0, 65535);
    q->shield_remainder = (u16)pick(0, 1279);
    q->owner = (u8)pick(0, 4); q->type = (u8)pick(0, SC_RU_COUNT - 1);
    q->selected = (u8)pick(0, 1); q->order = (u8)pick(0, 15); q->armor = (u8)pick(0, 255);
}

static void zergling(Unit *q, int owner, int i)
{
    memset(q, 0, sizeof(*q));
    q->alive = 1; q->owner = (u8)owner; q->type = 4; q->hp = q->maxhp = 35;
    q->raw_type = 37; q->raw_owner = -1; q->build = -1; q->target = -1;
    q->order = 2; q->tx = 795; q->ty = 780;
    q->x = (s16)(96 + (i % 20) * 12 + pick(0, 300)); q->y = (s16)(240 + (i / 20) * 12 + pick(0, 300));
    q->path_x = (s16)pick(400, 800); q->path_y = (s16)pick(400, 800);
    q->path_timer = (u16)pick(0, 24); q->path_retry = (u16)(pick(0, 3) * 3);
    if (pick(0, 3) == 0) q->cd = (u16)pick(1, 12);
    if (pick(0, 5) == 0) q->hp = pick(1, 34);
}

static void round_trip(const char *label, size_t *bytes)
{
    assert(sc64_zp_stream_encode(&stream, table, W, H));
    assert(sc64_zp_stream_canonical(&stream));
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        assert(sc64_zp_stream_decode(&stream, i, W, H, &decoded[i]));
        assert(!memcmp(&decoded[i], &table[i], sizeof(Unit)));
    }
    if (bytes) *bytes = stream.used;
    if (label) printf("%s: %u data bytes, %u templates\n", label, stream.used, stream.template_count);
}

static void corruption_is_rejected(void)
{
    /* Flip bits of the used data in turn. Each flip must be rejected by the
     * structure check or a slot decode, or decode a different world: no two
     * streams encode the same table. (SRAM records also carry a checksum.) */
    unsigned rejected = 0, flips = 0;
    for (unsigned byte = 0; byte < stream.used; byte += 7) {
        for (unsigned bit = 0; bit < 8; ++bit, ++flips) {
            int ok = 1;
            copy = stream;
            copy.data[byte] ^= (u8)(1u << bit);
            if (!sc64_zp_stream_canonical(&copy)) { ++rejected; continue; }
            for (unsigned i = 0; i < MAX_UNITS && ok; ++i) {
                Unit q;
                if (!sc64_zp_stream_decode(&copy, i, W, H, &q)) ok = 0;
                else if (memcmp(&q, &table[i], sizeof(q))) ok = 0;
            }
            if (ok) { printf("same table after flip byte %u bit %u (used %u)\n", byte, bit, stream.used); abort(); }
            ++rejected;
        }
    }
    copy = stream; copy.used++;
    assert(!sc64_zp_stream_canonical(&copy));
    copy = stream; copy.data[stream.used] = 1;
    assert(!sc64_zp_stream_canonical(&copy));
    copy = stream; copy.template_count = SC64_ZPS_TEMPLATES + 1;
    assert(!sc64_zp_stream_canonical(&copy));
    copy = stream;
    for (unsigned i = 0; i < MAX_UNITS; ++i) if (copy.offset[i] != SC64_ZPS_NO_ROW) {
        copy.offset[i] = SC64_ZPS_NO_ROW; break;
    }
    assert(!sc64_zp_stream_canonical(&copy));
    printf("corruption: %u single-bit flips, each rejected or decoded differently\n", flips);
    (void)rejected;
}

int main(void)
{
    size_t crowd_bytes, mixed_bytes;
    memset(table, 0, sizeof(table));
    round_trip("empty table", 0);
    assert(stream.used == 0);

    /* 400 Zerglings for owner 0 plus eight Terran units. */
    for (int i = 0; i < 400; ++i) zergling(&table[i], 0, i);
    for (int i = 400; i < MAX_UNITS; ++i) random_unit(&table[i]);
    round_trip("400 Zerglings + 8 random", &crowd_bytes);
    assert(crowd_bytes <= 400u * 16u);
    corruption_is_rejected();

    /* Dead slots with retained fields round-trip exactly. */
    for (int i = 0; i < 400; i += 3) { table[i].alive = 0; table[i].hp = -1; }
    round_trip("crowd with retained dead rows", 0);

    /* Mixed armies: five owners, a handful of types each. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        zergling(&table[i], i % 5, i);
        table[i].type = (u8)(i % 7);
        if (i % 11 == 0) random_unit(&table[i]);
    }
    round_trip("mixed five-owner table", &mixed_bytes);

    /* Fully random rows overflow the record: the encoder refuses. */
    for (int i = 0; i < MAX_UNITS; ++i) random_unit(&table[i]);
    assert(!sc64_zp_stream_encode(&stream, table, W, H));
    /* Out-of-range fields are refused, never truncated. */
    memset(table, 0, sizeof(table));
    zergling(&table[5], 1, 5);
    table[5].target = 1024;
    assert(!sc64_zp_stream_encode(&stream, table, W, H));
    table[5].target = 1023; table[5].x = W;
    assert(!sc64_zp_stream_encode(&stream, table, W, H));
    table[5].x = W - 1;
    round_trip(0, 0);

    /* Field extremes at both ends, one field at a time from a template. */
    for (unsigned f = 0; f < SC64_ZPS_FIELDS; ++f) {
        memset(table, 0, sizeof(table));
        zergling(&table[0], 2, 0); table[1] = table[0];
        int lo = -sc64_zps_bias[f], hi = (1 << sc64_zps_width[f]) - 1 - sc64_zps_bias[f];
        if (f == 15) lo = -1;                       /* target */
        if (f == 16) hi = 24;                       /* path_timer */
        if (f == 17) hi = 12;                       /* path_retry */
        if (f == 24) hi = 1279;                     /* shield_remainder */
        if (f == 0) lo = 0;                         /* alive hp */
        if (f == 1 || f == 20) hi = 131071;
        if (f == 2) hi = 4;
        if (f == 3) hi = SC_RU_COUNT - 1;
        if (f == 21 || f == 22) hi = 255;
        sc64_zps_set_field(&table[1], f, lo); round_trip(0, 0);
        sc64_zps_set_field(&table[1], f, hi); round_trip(0, 0);
    }
    printf("unit stream: crowd %zu B, mixed %zu B of %u; all round trips exact\n",
           crowd_bytes, mixed_bytes, (unsigned)SC64_ZPS_DATA_BYTES);
    return 0;
}
