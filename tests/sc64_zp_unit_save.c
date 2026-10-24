/* New-generation ZP Unit codec: compare all unchanged fields against the
 * established 38-byte codec while enforcing movement timer bounds. */
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
#include "../starcraft/sc64_zp_unit_save.h"

_Static_assert(sizeof(Sc64ZpPackedUnitSave) == 35, "new row is 35 bytes");

static void set_bits(u8 *bytes, unsigned start, unsigned width, unsigned value)
{
    for (unsigned i = 0; i < width; ++i) {
        unsigned mask = 1u << ((start + i) & 7u);
        if ((value >> i) & 1u) bytes[(start + i) >> 3] |= (u8)mask;
        else bytes[(start + i) >> 3] &= (u8)~mask;
    }
}

static void same_fields_roundtrip(void)
{
    Unit src = {0}, old_out, new_out;
    Sc64PackedUnitSave old_row;
    Sc64ZpPackedUnitSave new_row;
    src.x = 2047; src.y = 1701; src.hp = 131071; src.maxhp = 131071;
    src.owner = 4; src.type = 47; src.alive = 1; src.selected = 1;
    src.cd = 65535; src.order = 15; src.tx = 2047; src.ty = 2046;
    src.carry = -65536; src.build = 511; src.work_timer = 65535;
    src.armor = 255; src.weapon = 65535; src.target = 511;
    src.path_timer = 24; src.path_retry = 12; src.path_x = 2044; src.path_y = 2043;
    src.shield = 131071; src.raw_type = 255; src.raw_owner = -1;
    src.flags = 65535; src.shield_remainder = 1279;
    assert(sc64_unit_save_encode(&old_row, &src, 2048, 2048));
    assert(sc64_zp_unit_save_encode(&new_row, &src, 2048, 2048));
    assert(sc64_unit_save_decode(&old_out, &old_row, 2048, 2048));
    assert(sc64_zp_unit_save_decode(&new_out, &new_row, 2048, 2048));
    assert(!memcmp(&src, &old_out, sizeof(src)));
    assert(!memcmp(&src, &new_out, sizeof(src)));
}

static uint32_t rng = 0xa43c1f29u;
static uint32_t next_random(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng;
}

static void randomized_differential(void)
{
    for (unsigned i = 0; i < 20000; ++i) {
        Unit src = {0}, old_out, new_out;
        Sc64PackedUnitSave old_row;
        Sc64ZpPackedUnitSave new_row;
        src.x = next_random() & 2047; src.y = next_random() & 2047;
        src.hp = (int)(next_random() % 262144) - 131072;
        src.alive = src.hp > 0 && (next_random() & 1u);
        if (src.alive && src.hp < 1) src.hp = 1;
        src.maxhp = next_random() & 131071;
        if (src.alive && src.maxhp < src.hp) src.maxhp = src.hp;
        src.owner = next_random() % 5; src.type = next_random() % SC_RU_COUNT;
        src.selected = next_random() & 1; src.cd = next_random() & 65535;
        src.order = next_random() & 15; src.tx = next_random() & 2047;
        src.ty = next_random() & 2047;
        src.carry = (int)(next_random() & 131071) - 65536;
        src.build = (int)(next_random() & 1023) - 512;
        src.work_timer = next_random() & 65535; src.armor = next_random() & 255;
        src.weapon = next_random() & 65535; src.target = (int)(next_random() % 513) - 1;
        src.path_timer = next_random() % 25; src.path_retry = next_random() % 13;
        src.path_x = next_random() & 2047; src.path_y = next_random() & 2047;
        src.shield = next_random() & 131071;
        src.raw_type = (int)(next_random() % 257) - 1;
        src.raw_owner = (int)(next_random() % 257) - 1;
        src.flags = next_random() & 65535; src.shield_remainder = next_random() % 1280;
        assert(sc64_unit_save_encode(&old_row, &src, 2048, 2048));
        assert(sc64_zp_unit_save_encode(&new_row, &src, 2048, 2048));
        assert(sc64_unit_save_decode(&old_out, &old_row, 2048, 2048));
        assert(sc64_zp_unit_save_decode(&new_out, &new_row, 2048, 2048));
        assert(!memcmp(&src, &old_out, sizeof(src)));
        assert(!memcmp(&src, &new_out, sizeof(src)));
        assert(old_out.path_timer == new_out.path_timer);
        assert(old_out.path_retry == new_out.path_retry);
    }
}

static void rejects_invalid_paths_and_decode_atomically(void)
{
    Unit q = {0}, out, sentinel;
    Sc64ZpPackedUnitSave row, before;
    q.x = q.y = 1; q.maxhp = 100; q.owner = 1; q.type = 2;
    q.alive = 1; q.hp = 100; q.path_timer = 24; q.path_retry = 12;
    assert(sc64_zp_unit_save_encode(&row, &q, 64, 64));
    memset(&sentinel, 0x5a, sizeof(sentinel)); out = sentinel;
    before = row; q.path_timer = 25;
    assert(!sc64_zp_unit_save_encode(&row, &q, 64, 64));
    assert(!memcmp(&row, &before, sizeof(row)));
    q.path_timer = 24; q.path_retry = 13;
    assert(!sc64_zp_unit_save_encode(&row, &q, 64, 64));
    assert(!memcmp(&row, &before, sizeof(row)));
    q.path_retry = 12; assert(sc64_zp_unit_save_encode(&row, &q, 64, 64));
    set_bits(row.bytes, 187, 5, 25); /* noncanonical path_timer */
    assert(!sc64_zp_unit_save_decode(&out, &row, 64, 64));
    assert(!memcmp(&out, &sentinel, sizeof(out)));
    assert(sc64_zp_unit_save_encode(&row, &q, 64, 64));
    set_bits(row.bytes, 192, 4, 13); /* noncanonical path_retry */
    assert(!sc64_zp_unit_save_decode(&out, &row, 64, 64));
    assert(!memcmp(&out, &sentinel, sizeof(out)));
}

static void zero_slots_and_world_bounds(void)
{
    Unit zero = {0}, out;
    Sc64ZpPackedUnitSave row;
    assert(sc64_zp_unit_save_encode(&row, &zero, 128, 128));
    assert(sc64_zp_unit_is_zero(&row));
    assert(sc64_zp_unit_save_decode(&out, &row, 128, 128));
    assert(!memcmp(&out, &zero, sizeof(out)));
    assert(!sc64_zp_unit_save_decode(&out, &row, 0, 128));
    assert(!memcmp(&out, &zero, sizeof(out)));
}

int main(void)
{
    same_fields_roundtrip();
    randomized_differential();
    rejects_invalid_paths_and_decode_atomically();
    zero_slots_and_world_bounds();
    puts("ZP unit codec tests passed (35 bytes, 280 bits)");
    return 0;
}
