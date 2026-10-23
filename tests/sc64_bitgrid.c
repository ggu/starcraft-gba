#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_bitgrid.h"

#define MAX_CELLS 1031
#define MAX_BYTES ((MAX_CELLS + 7) / 8)

static u32 test_random(u32 *state)
{
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static void check_grid(const u8 *bits, const u8 *oracle, int count)
{
    for (int i = 0; i < count; ++i)
        assert(sc64_bitgrid_get(bits, count, i) == (oracle[i] != 0));
}

static void test_bounds(void)
{
    u8 bits[2] = {0xA5, 0x5A};
    assert(sc64_bitgrid_bytes(-1) == 0);
    assert(sc64_bitgrid_bytes(0) == 0);
    assert(sc64_bitgrid_bytes(1) == 1);
    assert(sc64_bitgrid_bytes(8) == 1);
    assert(sc64_bitgrid_bytes(9) == 2);
    assert(sc64_bitgrid_get(bits, 9, -1) == 0);
    assert(sc64_bitgrid_get(bits, 9, 9) == 0);
    assert(!sc64_bitgrid_set(bits, 9, -1, 1));
    assert(!sc64_bitgrid_set(bits, 9, 9, 1));
    assert(sc64_bitgrid_get(0, 9, 0) == 0);
    assert(!sc64_bitgrid_set(0, 9, 0, 1));
    sc64_bitgrid_clear(bits, -1);
    sc64_bitgrid_clear(bits, 0);
    assert(bits[0] == 0xA5 && bits[1] == 0x5A);
}

static void test_cell_counts(void)
{
    static const int counts[] = {1, 7, 8, 9, 15, 17, 63, 65, 127, 129, 1023, 1025, 1031};
    u8 oracle[MAX_CELLS];
    u8 buffer[MAX_BYTES + 2];

    for (unsigned c = 0; c < sizeof(counts) / sizeof(counts[0]); ++c) {
        int count = counts[c];
        unsigned bytes = sc64_bitgrid_bytes(count);
        u8 *bits = buffer + 1;
        memset(buffer, 0xA7, sizeof(buffer));
        memset(oracle, 0, (size_t)count);

        sc64_bitgrid_clear(bits, count);
        assert(buffer[0] == 0xA7 && buffer[bytes + 1] == 0xA7);
        for (int i = 0; i < count; ++i) assert(!sc64_bitgrid_get(bits, count, i));

        /* Set every cell, then clear alternating cells. The byte oracle
         * catches accidental changes to neighboring packed bits. */
        for (int i = 0; i < count; ++i) {
            assert(sc64_bitgrid_set(bits, count, i, 2));
            oracle[i] = 1;
        }
        check_grid(bits, oracle, count);
        for (int i = 0; i < count; i += 2) {
            assert(sc64_bitgrid_clear_cell(bits, count, i));
            oracle[i] = 0;
        }
        check_grid(bits, oracle, count);
        for (int i = 0; i < count; i += 2) {
            assert(sc64_bitgrid_set(bits, count, i, 1));
            oracle[i] = 1;
        }
        check_grid(bits, oracle, count);
        assert(buffer[0] == 0xA7 && buffer[bytes + 1] == 0xA7);
    }
}

static void test_random_oracle(void)
{
    const int count = 1025;
    const unsigned bytes = sc64_bitgrid_bytes(count);
    u8 storage[MAX_BYTES + 2], oracle[MAX_CELLS];
    u8 *bits = storage + 1;
    u32 seed = 0x64B17u;

    memset(storage, 0xD3, sizeof(storage));
    memset(oracle, 0, sizeof(oracle));
    sc64_bitgrid_clear(bits, count);
    for (int step = 0; step < 12000; ++step) {
        int cell = (int)(test_random(&seed) % (u32)count);
        int value = (int)(test_random(&seed) % 5u);
        assert(sc64_bitgrid_set(bits, count, cell, value));
        oracle[cell] = (u8)(value != 0);
        assert(sc64_bitgrid_get(bits, count, cell) == oracle[cell]);
        if ((step & 127) == 0) check_grid(bits, oracle, count);
    }
    check_grid(bits, oracle, count);
    assert(storage[0] == 0xD3 && storage[bytes + 1] == 0xD3);
}

static void test_pack_unpack(void)
{
    static const int counts[] = {1, 7, 8, 9, 17, 65, 129, 1031};
    u8 source[MAX_CELLS + 2], output[MAX_CELLS + 2], packed[MAX_BYTES + 2];
    u32 seed = 0xC0FFEEu;

    for (unsigned c = 0; c < sizeof(counts) / sizeof(counts[0]); ++c) {
        int count = counts[c];
        unsigned bytes = sc64_bitgrid_bytes(count);
        memset(source, 0xE1, sizeof(source));
        memset(output, 0xE2, sizeof(output));
        memset(packed, 0xE3, sizeof(packed));
        for (int i = 0; i < count; ++i)
            source[i + 1] = (u8)(test_random(&seed) & 0xFFu);

        sc64_bitgrid_pack(packed + 1, source + 1, count);
        assert(packed[0] == 0xE3 && packed[bytes + 1] == 0xE3);
        sc64_bitgrid_unpack(output + 1, packed + 1, count);
        assert(output[0] == 0xE2 && output[count + 1] == 0xE2);
        for (int i = 0; i < count; ++i)
            assert(output[i + 1] == (source[i + 1] != 0));
    }

    /* Empty and null ranges do not write into caller-owned storage. */
    memset(packed, 0xB4, sizeof(packed));
    sc64_bitgrid_pack(packed + 1, source + 1, 0);
    sc64_bitgrid_unpack(output + 1, packed + 1, 0);
    sc64_bitgrid_pack(0, source, 9);
    sc64_bitgrid_pack(packed, 0, 9);
    sc64_bitgrid_unpack(0, packed, 9);
    sc64_bitgrid_unpack(output, 0, 9);
    for (unsigned i = 0; i < sizeof(packed); ++i) assert(packed[i] == 0xB4);
}

static void test_t4_full_map_grid(void)
{
    enum { cells = 128 * 128, bytes = cells / 8 };
    u8 packed[bytes + 2], oracle[cells], unpacked[cells + 2];
    memset(packed, 0xC7, sizeof(packed));
    memset(oracle, 0, sizeof(oracle));
    memset(unpacked, 0xD8, sizeof(unpacked));
    sc64_bitgrid_clear(packed + 1, cells);
    assert(packed[0] == 0xC7 && packed[bytes + 1] == 0xC7);
    assert(sc64_bitgrid_set(packed + 1, cells, 0, 1));
    assert(sc64_bitgrid_set(packed + 1, cells, cells - 1, 1));
    oracle[0] = oracle[cells - 1] = 1;
    for (int cell = 1; cell < cells - 1; cell += 2) {
        assert(sc64_bitgrid_set(packed + 1, cells, cell, 1));
        oracle[cell] = 1;
    }
    sc64_bitgrid_unpack(unpacked + 1, packed + 1, cells);
    assert(unpacked[0] == 0xD8 && unpacked[cells + 1] == 0xD8);
    for (int cell = 0; cell < cells; ++cell)
        assert(unpacked[cell + 1] == oracle[cell]);
    assert(sc64_bitgrid_get(packed + 1, cells, cells - 1) == 1);
    assert(sc64_bitgrid_get(packed + 1, cells, cells) == 0);
    assert(!sc64_bitgrid_set(packed + 1, cells, cells, 1));
    assert(packed[0] == 0xC7 && packed[bytes + 1] == 0xC7);
}

int main(void)
{
    test_bounds();
    test_cell_counts();
    test_random_oracle();
    test_pack_unpack();
    test_t4_full_map_grid();
    puts("sc64 bitgrid: ok");
    return 0;
}
