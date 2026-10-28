/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free oracle for the generated CONTACT geometry table. */
#include <assert.h>
#include <stdio.h>

typedef unsigned char u8;
#include "sc64_contact_mask_table_generated.h"

#define RADIUS_MIN 6
#define RADIUS_MAX 19
#define SPEED_MAX 14
#define OFFSET_MIN -32
#define OFFSET_MAX 32
#define TABLE_BYTES ((RADIUS_MAX - RADIUS_MIN + 1) * (SPEED_MAX + 1) * 65 * 65)

_Static_assert(sizeof(sc64_contact_mask_table_data) == TABLE_BYTES,
               "generated CONTACT mask table size changed");

static unsigned
arithmetic_oracle(int radius, int speed, int ox, int oy)
{
    static const int dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    static const int dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    unsigned mask = 0;
    int origin_distance = ox * ox + oy * oy;
    for (int k = 0; k < 8; ++k) {
        int x = ox - dx[k] * speed;
        int y = oy - dy[k] * speed;
        if (x <= -radius || x >= radius || y <= -radius || y >= radius)
            continue;
        int distance = x * x + y * y;
        if (distance < radius * radius && distance <= origin_distance)
            mask |= 1u << k;
    }
    return mask;
}

int
main(void)
{
    unsigned tuples = 0;
    assert(SC64_CONTACT_MASK_TABLE_RADIUS_MIN == RADIUS_MIN);
    assert(SC64_CONTACT_MASK_TABLE_RADIUS_COUNT == RADIUS_MAX - RADIUS_MIN + 1);
    assert(SC64_CONTACT_MASK_TABLE_SPEED_COUNT == SPEED_MAX + 1);
    assert(SC64_CONTACT_MASK_TABLE_SIDE == OFFSET_MAX - OFFSET_MIN + 1);

    for (int radius = RADIUS_MIN; radius <= RADIUS_MAX; ++radius)
        for (int speed = 0; speed <= SPEED_MAX; ++speed)
            for (int oy = OFFSET_MIN; oy <= OFFSET_MAX; ++oy)
                for (int ox = OFFSET_MIN; ox <= OFFSET_MAX; ++ox) {
                    unsigned ri = (unsigned)(radius - RADIUS_MIN);
                    unsigned oi = (unsigned)(oy - OFFSET_MIN);
                    unsigned xi = (unsigned)(ox - OFFSET_MIN);
                    unsigned index = (((ri * (SPEED_MAX + 1u) + (unsigned)speed) * 65u + oi) * 65u + xi);
                    assert(sc64_contact_mask_table_data[index] ==
                           arithmetic_oracle(radius, speed, ox, oy));
                    ++tuples;
                }
    assert(tuples == TABLE_BYTES);
    puts("CONTACT generated geometry table: 887,250 arithmetic comparisons PASS");
    return 0;
}
