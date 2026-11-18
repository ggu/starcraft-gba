/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CONTACT_FILTER_CENSUS_H
#define SC64_CONTACT_FILTER_CENSUS_H

/* The game defines its frame counter after including the collision code. */
static int frame;

enum {
    CONTACT_CENSUS_QUERIES,
    CONTACT_CENSUS_EMPTY,
    CONTACT_CENSUS_OCCUPIED,
    CONTACT_CENSUS_FULL_NODES,
    CONTACT_CENSUS_VISITED_NODES,
    CONTACT_CENSUS_BOUNDARY,
    CONTACT_CENSUS_ADMITTED_HEADS,
    CONTACT_CENSUS_ADMITTED_NONEMPTY_HEADS,
    CONTACT_CENSUS_EXCLUDED_NODES,
    CONTACT_CENSUS_OUTSIDE_BUCKETS,
    CONTACT_CENSUS_OUTSIDE_BUCKET_NODES,
    CONTACT_CENSUS_COINCIDENT_BUCKETS,
    CONTACT_CENSUS_COINCIDENT_BUCKET_NODES,
    CONTACT_CENSUS_COINCIDENT_SUBCELL_NODES,
    CONTACT_CENSUS_EXACT_BOUNDS_BUCKETS,
    CONTACT_CENSUS_EXACT_BOUNDS_NODES,
    CONTACT_CENSUS_UNSAFE_BUCKETS,
    CONTACT_CENSUS_EXCLUDED_SMALL_NODES,
    CONTACT_CENSUS_OUTSIDE_SMALL_BUCKETS,
    CONTACT_CENSUS_OUTSIDE_SMALL_NODES,
    CONTACT_CENSUS_FIELDS
};
static volatile u32 sc64_contact_filter_counts[CONTACT_CENSUS_FIELDS];

static void __attribute__((noinline))
sc64_contact_count_bucket(const Unit *q, unsigned head, int col, int row,
                          int reach, int speed)
{
    unsigned occupancy = 0, admitted = 0, nodes = 0, outside = 0, outside_small = 0;
    unsigned coincident = 0, noncoincident = 0;
    int least_x = WORLD_W, least_y = WORLD_H, greatest_x = 0, greatest_y = 0;
    int left = q->x - reach, right = q->x + reach;
    int top = q->y - reach, bottom = q->y + reach;
    int x0 = col * 16, y0 = row * 16;
    unsigned same = ((q->x & 8) != 0) + 2u * ((q->y & 8) != 0);
    int valid = 1;

    if (head == 0xffu) {
        ++sc64_contact_filter_counts[CONTACT_CENSUS_EMPTY];
        return;
    }
    ++sc64_contact_filter_counts[CONTACT_CENSUS_OCCUPIED];
    for (unsigned cell = 0; cell != 4u; ++cell) {
        int x = x0 + ((cell & 1u) ? 8 : 0);
        int y = y0 + ((cell & 2u) ? 8 : 0);
        if (x <= right && left <= x + 7 && y <= bottom && top <= y + 7)
            admitted |= 1u << cell;
    }
    for (unsigned slot = head; slot != 0xffu && nodes < MAX_UNITS;
         slot = sc64_unit_bucket_next[slot]) {
        const Unit *actor = &u[slot];
        int x = actor->x, y = actor->y;
        unsigned cell = ((x & 8) != 0) + 2u * ((y & 8) != 0);
        unsigned bit = 1u << cell;
        ++nodes;
        if (x < x0 || x > x0 + 15 || y < y0 || y > y0 + 15)
            valid = 0;
        occupancy |= bit;
        outside += (admitted & bit) == 0;
        int sub_x = x & ~3, sub_y = y & ~3;
        outside_small += sub_x > right || sub_x + 3 < left ||
                         sub_y > bottom || sub_y + 3 < top;
        if (cell == same) {
            if (x == q->x && y == q->y) ++coincident;
            else ++noncoincident;
        }
        if (x < least_x) least_x = x;
        if (x > greatest_x) greatest_x = x;
        if (y < least_y) least_y = y;
        if (y > greatest_y) greatest_y = y;
    }
    sc64_contact_filter_counts[CONTACT_CENSUS_FULL_NODES] += nodes;
    if (!valid) {
        ++sc64_contact_filter_counts[CONTACT_CENSUS_UNSAFE_BUCKETS];
        return;
    }
    sc64_contact_filter_counts[CONTACT_CENSUS_EXCLUDED_SMALL_NODES] += outside_small;
    if (outside_small == nodes) {
        ++sc64_contact_filter_counts[CONTACT_CENSUS_OUTSIDE_SMALL_BUCKETS];
        sc64_contact_filter_counts[CONTACT_CENSUS_OUTSIDE_SMALL_NODES] += nodes;
    }
    if (admitted != 15u) {
        ++sc64_contact_filter_counts[CONTACT_CENSUS_BOUNDARY];
        sc64_contact_filter_counts[CONTACT_CENSUS_ADMITTED_HEADS] +=
            (admitted & 1u) + ((admitted >> 1) & 1u) +
            ((admitted >> 2) & 1u) + (admitted >> 3);
        unsigned nonempty = admitted & occupancy;
        sc64_contact_filter_counts[CONTACT_CENSUS_ADMITTED_NONEMPTY_HEADS] +=
            (nonempty & 1u) + ((nonempty >> 1) & 1u) +
            ((nonempty >> 2) & 1u) + (nonempty >> 3);
        sc64_contact_filter_counts[CONTACT_CENSUS_EXCLUDED_NODES] += outside;
        if (!nonempty) {
            ++sc64_contact_filter_counts[CONTACT_CENSUS_OUTSIDE_BUCKETS];
            sc64_contact_filter_counts[CONTACT_CENSUS_OUTSIDE_BUCKET_NODES] += nodes;
        }
    }
    if (speed > 0 && speed <= 255 && !noncoincident) {
        sc64_contact_filter_counts[CONTACT_CENSUS_COINCIDENT_SUBCELL_NODES] += coincident;
        if (coincident == nodes) {
            ++sc64_contact_filter_counts[CONTACT_CENSUS_COINCIDENT_BUCKETS];
            sc64_contact_filter_counts[CONTACT_CENSUS_COINCIDENT_BUCKET_NODES] += nodes;
        }
    }
    if (least_x > right || greatest_x < left || least_y > bottom || greatest_y < top) {
        ++sc64_contact_filter_counts[CONTACT_CENSUS_EXACT_BOUNDS_BUCKETS];
        sc64_contact_filter_counts[CONTACT_CENSUS_EXACT_BOUNDS_NODES] += nodes;
    }
}
#endif
