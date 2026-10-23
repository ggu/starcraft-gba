/* Asset-free, byte-exact tests for the opt-in compact Unit row. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint32_t u32;
enum { SC_RU_COUNT = 48 };
#define SC64_RACES 1
typedef struct {
    int x, y, hp, maxhp, owner, type, alive, selected, cd, order, tx, ty;
    int carry, build, work_timer, armor, weapon, target, path_timer, path_retry;
    int path_x, path_y, shield, raw_type, raw_owner, flags, shield_remainder;
} Unit;
#include "../starcraft/sc64_compact_unit_save.h"

_Static_assert(sizeof(Unit) == 27 * sizeof(int), "fixture matches RACES Unit");

static void fill_max(Unit *q)
{
    memset(q, 0, sizeof(*q));
    q->x = q->y = q->tx = q->ty = q->path_x = q->path_y = 2047;
    q->hp = -131072; q->maxhp = 131071; q->owner = 4; q->type = 47;
    q->alive = 0; q->selected = 1; q->cd = 65535; q->order = 15;
    q->carry = -65536; q->build = -512; q->work_timer = 65535;
    q->armor = 255; q->weapon = 65535; q->target = 511;
    q->path_timer = q->path_retry = 65535; q->shield = 131071;
    q->raw_type = q->raw_owner = 255; q->flags = 65535;
    q->shield_remainder = 1279;
}

static void exact_roundtrip(void)
{
    Unit before, after;
    Sc64PackedUnitSave row;
    fill_max(&before);
    assert(sc64_unit_save_encode(&row, &before, 2048, 2048));
    assert(sc64_unit_save_decode(&after, &row, 2048, 2048));
    assert(!memcmp(&before, &after, sizeof(before)));

    before.alive = 1; before.hp = 131071; before.selected = 0;
    before.carry = 65535; before.build = 511; before.target = -1;
    before.raw_type = before.raw_owner = -1;
    assert(sc64_unit_save_encode(&row, &before, 2048, 2048));
    assert(sc64_unit_save_decode(&after, &row, 2048, 2048));
    assert(!memcmp(&before, &after, sizeof(before)));
}

static void stable_dead_and_zero_slots(void)
{
    Unit dead, decoded, zero = {0};
    Sc64PackedUnitSave row;
    fill_max(&dead);
    dead.alive = 0; dead.hp = -193; dead.raw_type = dead.raw_owner = -1;
    dead.flags = 0xA55A; dead.target = -1; dead.build = -1;
    assert(sc64_unit_save_encode(&row, &dead, 2048, 2048));
    assert(sc64_unit_save_decode(&decoded, &row, 2048, 2048));
    assert(!memcmp(&dead, &decoded, sizeof(dead)));

    assert(sc64_unit_save_encode(&row, &zero, 2048, 2048));
    assert(sc64_packed_unit_is_zero(&row));
    assert(sc64_unit_save_decode(&decoded, &row, 2048, 2048));
    assert(!memcmp(&zero, &decoded, sizeof(zero)));
}

static uint32_t rng_state = 0x6d2b79f5u;
static uint32_t next_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void randomized_roundtrips(void)
{
    for (unsigned n = 0; n < 10000; ++n) {
        Unit before, after;
        Sc64PackedUnitSave row;
        memset(&before, 0, sizeof(before));
        before.x = next_random() & 2047; before.y = next_random() & 2047;
        before.tx = next_random() & 2047; before.ty = next_random() & 2047;
        before.path_x = next_random() & 2047; before.path_y = next_random() & 2047;
        before.alive = next_random() & 1;
        before.hp = before.alive ? 1 + (int)(next_random() % 131071) :
                                      (int)(next_random() % 262144) - 131072;
        before.maxhp = next_random() & 131071;
        if (before.alive && before.maxhp < before.hp) before.maxhp = before.hp;
        before.owner = next_random() % 5; before.type = next_random() % SC_RU_COUNT;
        before.selected = next_random() & 1; before.cd = next_random() & 65535;
        before.order = next_random() & 15; before.carry = (int)(next_random() & 131071) - 65536;
        before.build = (int)(next_random() & 1023) - 512;
        before.work_timer = next_random() & 65535;
        before.armor = next_random() & 255; before.weapon = next_random() & 65535;
        before.target = -1 + (int)(next_random() % 513);
        before.path_timer = next_random() & 65535;
        before.path_retry = next_random() & 65535;
        before.shield = next_random() & 131071;
        before.raw_type = (int)(next_random() % 257) - 1;
        before.raw_owner = (int)(next_random() % 257) - 1;
        before.flags = next_random() & 65535;
        before.shield_remainder = next_random() % 1280;
        assert(sc64_unit_save_encode(&row, &before, 2048, 2048));
        assert(sc64_unit_save_decode(&after, &row, 2048, 2048));
        assert(!memcmp(&before, &after, sizeof(before)));
    }
}

static void rejects_atomically(void)
{
    Unit q, out, sentinel;
    Sc64PackedUnitSave row, original;
    fill_max(&q);
    memset(&row, 0xA5, sizeof(row)); original = row;
#define REJECT(field, value) do { \
    q.field = (value); \
    assert(!sc64_unit_save_encode(&row, &q, 2048, 2048)); \
    assert(!memcmp(&row, &original, sizeof(row))); \
    fill_max(&q); \
} while (0)
    REJECT(x, 2048); REJECT(hp, 131072); REJECT(hp, -131073);
    REJECT(maxhp, 131072); REJECT(owner, 5); REJECT(type, 48);
    REJECT(alive, 2); REJECT(selected, -1); REJECT(cd, -1); REJECT(order, 16);
    REJECT(tx, 2048); REJECT(carry, 65536); REJECT(build, 512);
    REJECT(work_timer, 65536); REJECT(armor, 256); REJECT(weapon, 65536);
    REJECT(target, 512); REJECT(path_timer, 65536); REJECT(path_retry, -1);
    REJECT(path_x, 2048); REJECT(shield, 131072); REJECT(raw_type, 256);
    REJECT(raw_owner, -2); REJECT(flags, 65536); REJECT(shield_remainder, 1280);
#undef REJECT
    q.alive = 1; q.hp = -1;
    assert(!sc64_unit_save_encode(&row, &q, 2048, 2048));
    q.alive = 0; q.hp = 0;
    assert(sc64_unit_save_encode(&row, &q, 2048, 2048));
    assert(!sc64_unit_save_encode(&row, &q, 2049, 2048));

    fill_max(&q); assert(sc64_unit_save_encode(&row, &q, 2048, 2048));
    memset(&sentinel, 0x3C, sizeof(sentinel)); out = sentinel;
    row.bytes[37] |= 0x80; /* Reserved bit303. */
    assert(!sc64_unit_save_decode(&out, &row, 2048, 2048));
    assert(!memcmp(&out, &sentinel, sizeof(out)));
    assert(sc64_unit_save_encode(&row, &q, 2048, 2048));
    /* owner starts at bit57. Encoding owner5 is invalid and must not apply. */
    row.bytes[7] = (u8)((row.bytes[7] & (u8)~0x0Eu) | 0x0Au);
    out = sentinel;
    assert(!sc64_unit_save_decode(&out, &row, 2048, 2048));
    assert(!memcmp(&out, &sentinel, sizeof(out)));
    assert(!sc64_unit_save_decode(&out, &row, 0, 2048));
}

int main(void)
{
    assert(sizeof(Sc64PackedUnitSave) == 38);
    exact_roundtrip();
    stable_dead_and_zero_slots();
    randomized_roundtrips();
    rejects_atomically();
    puts("compact Unit codec ok (38 bytes, 303 field bits, lossless slots)");
    return 0;
}
