#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef struct { int x, y, amount, style; } Mineral;

#include "../starcraft/sc64_compact_mineral_save.h"

static void same(const Mineral *a, const Mineral *b)
{
    assert(a->x == b->x && a->y == b->y &&
           a->amount == b->amount && a->style == b->style);
}

static void roundtrip_style(int style)
{
    Mineral in = {2047, 2046, 65535, style};
    Mineral out = {-1, -1, -1, -1};
    Sc64PackedMineralSave row;
    assert(sc64_mineral_save_encode(&row, &in, 2048, 2048));
    assert(sizeof(row) == 5);
    assert(sc64_mineral_save_decode(&out, &row, 2048, 2048));
    same(&in, &out);
}

int main(void)
{
    static const int styles[] = {0, 176, 177, 178};
    Sc64PackedMineralSave row, before;
    Mineral in = {0, 0, 0, 0};
    Mineral out, unchanged;

    for (unsigned i = 0; i < sizeof(styles) / sizeof(styles[0]); ++i)
        roundtrip_style(styles[i]);

    /* Canonical unused records remain all-zero and decode losslessly. */
    memset(&row, 0, sizeof(row));
    memset(&out, 0xa5, sizeof(out));
    assert(sc64_mineral_save_decode(&out, &row, 2048, 2048));
    same(&in, &out);
    assert(sc64_mineral_save_encode(&row, &in, 2048, 2048));
    for (unsigned i = 0; i < sizeof(row.bytes); ++i) assert(row.bytes[i] == 0);

    /* Invalid source values and geometry must not modify the destination. */
    memset(&row, 0x5a, sizeof(row));
    before = row;
    in = (Mineral){1, 2, 3, 179};
    assert(!sc64_mineral_save_encode(&row, &in, 2048, 2048));
    assert(memcmp(&row, &before, sizeof(row)) == 0);
    in = (Mineral){2048, 0, 0, 176};
    assert(!sc64_mineral_save_encode(&row, &in, 2048, 2048));
    assert(memcmp(&row, &before, sizeof(row)) == 0);
    in = (Mineral){0, 0, 65536, 176};
    assert(!sc64_mineral_save_encode(&row, &in, 2048, 2048));
    assert(memcmp(&row, &before, sizeof(row)) == 0);
    in = (Mineral){0, 0, 0, 176};
    assert(!sc64_mineral_save_encode(&row, &in, 0, 2048));
    assert(memcmp(&row, &before, sizeof(row)) == 0);

    /* A row legal on a 128x128 map must reject coordinates outside a smaller
     * active world transactionally during decode. */
    in = (Mineral){1024, 16, 500, 177};
    assert(sc64_mineral_save_encode(&row, &in, 2048, 2048));
    unchanged = (Mineral){91, 92, 93, 178};
    out = unchanged;
    assert(!sc64_mineral_save_decode(&out, &row, 1024, 2048));
    same(&out, &unchanged);

    puts("compact mineral save tests passed");
    return 0;
}
