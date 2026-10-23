#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/sc64_unit_availability.h"

int main(void)
{
    unsigned char table[SC64_PUNI_BYTES];
    unsigned value;
    unsigned p, u;

    memset(table, 0, sizeof(table));
    /* Global raw 5 enabled, but player 0 explicitly disables it. */
    table[SC64_PUNI_GLOBAL_OFFSET + 5] = 1;
    table[0 * SC64_PUNI_UNIT_COUNT + 5] = 0;
    table[SC64_PUNI_DEFAULT_SELECTOR_OFFSET + 5] = 0;
    table[SC64_PUNI_DEFAULT_SELECTOR_OFFSET + SC64_PUNI_UNIT_COUNT + 5] = 1;
    /* Player 1 explicitly enables raw 1 while the global default is off. */
    table[SC64_PUNI_UNIT_COUNT + 1] = 0x80;
    table[SC64_PUNI_DEFAULT_SELECTOR_OFFSET + SC64_PUNI_UNIT_COUNT + 1] = 0;

    for (p = 0; p < SC64_PUNI_PLAYER_COUNT; ++p) {
        for (u = 0; u < SC64_PUNI_UNIT_COUNT; ++u) {
            unsigned expected = table[SC64_PUNI_DEFAULT_SELECTOR_OFFSET +
                                      p * SC64_PUNI_UNIT_COUNT + u]
                ? table[SC64_PUNI_GLOBAL_OFFSET + u] != 0
                : table[p * SC64_PUNI_UNIT_COUNT + u] != 0;
            value = 99;
            assert(sc64_puni_available(table, sizeof(table), p, u, &value));
            assert(value == expected);
        }
    }
    assert(sc64_puni_available(table, sizeof(table), 0, 5, &value) && value == 0);
    assert(sc64_puni_available(table, sizeof(table), 1, 5, &value) && value == 1);
    assert(sc64_puni_available(table, sizeof(table), 1, 1, &value) && value == 1);

    value = 0xA5u;
    assert(!sc64_puni_available(NULL, sizeof(table), 0, 0, &value));
    assert(value == 0xA5u);
    assert(!sc64_puni_available(table, sizeof(table) - 1, 0, 0, &value));
    assert(value == 0xA5u);
    assert(!sc64_puni_available(table, sizeof(table), 12, 0, &value));
    assert(value == 0xA5u);
    assert(!sc64_puni_available(table, sizeof(table), 0, 228, &value));
    assert(value == 0xA5u);
    assert(!sc64_puni_available(table, sizeof(table), 0, 0, NULL));

    {
        unsigned char bits[SC64_PUNI_EFFECTIVE_BIT_BYTES];
        unsigned first = 7;
        unsigned second = 7;
        memset(bits, 0, sizeof(bits));
        /* Adjacent flat bits straddle player 0/1 at this odd row stride. */
        bits[(227u >> 3)] |= (unsigned char)(1u << (227u & 7u));
        bits[(228u >> 3)] |= (unsigned char)(1u << (228u & 7u));
        bits[(11u * 228u + 227u) >> 3] |=
            (unsigned char)(1u << ((11u * 228u + 227u) & 7u));
        assert(sc64_availability_bitmap_get(bits, sizeof(bits), 0, 227, &first));
        assert(sc64_availability_bitmap_get(bits, sizeof(bits), 1, 0, &second));
        assert(first == 1 && second == 1);
        assert(sc64_availability_bitmap_get(bits, sizeof(bits), 11, 227, &value));
        assert(value == 1);
        value = 0xA5u;
        assert(!sc64_availability_bitmap_get(bits, sizeof(bits) - 1, 11, 227, &value));
        assert(value == 0xA5u);
        assert(!sc64_availability_bitmap_get(bits, sizeof(bits), 12, 0, &value));
        assert(value == 0xA5u);
    }

    puts("PUNI player/default selection and bounds passed");
    return 0;
}
