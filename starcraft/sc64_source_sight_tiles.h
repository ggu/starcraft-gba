/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Numeric source facts only: BWAPI UnitType.cpp sightRangeTiles at revision
 * d727fed68558c506163048ea889131d8cbb33915 (UnitType::sightRange() returns
 * tiles * 32 source pixels). Indexed by public raw UnitType id 0..227; the
 * pseudo-types after 227 have no sight.
 */
#ifndef SC64_SOURCE_SIGHT_TILES_H
#define SC64_SOURCE_SIGHT_TILES_H

static const unsigned char sc64_source_sight_tiles[228] = {
    /*   0 */  7,  9,  8,  8,  8, 10, 10,  7,  7, 10,  7,  8, 11,  3,  3,  7, 11,  8,  8,
    /*  19 */  8,  7,  7, 10, 10, 10, 10, 10,  8, 11, 11, 10, 10,  7, 10,  9,  4,  4,  5,
    /*  38 */  6,  7,  5,  7,  9,  7, 11, 10, 10,  5,  7, 10,  5,  9, 10,  8,  5,  7, 11,
    /*  57 */ 11,  8,  4,  9,  7, 10, 10,  8,  7,  8,  7,  8,  8,  8,  9, 11,  6,  7,  7,
    /*  76 */  8,  7,  8,  7, 10, 10,  9, 10,  9,  5,  9,  7, 10,  7,  7,  8,  7,  7,  7,
    /*  95 */  7,  7,  4,  9, 10, 11, 10, 11,  8, 11,  8, 10, 10,  8,  8,  8,  8,  8,  8,
    /* 114 */ 10,  8, 10,  8,  8,  8,  8,  8,  8,  8, 11, 10, 10,  8,  5,  5, 10,  9, 10,
    /* 133 */ 11,  8,  8,  8,  8,  8,  8,  8,  8,  8, 10, 10,  7, 10,  8,  8,  7,  8,  8,
    /* 152 */  8,  7, 11, 10,  8, 10,  7, 10, 10, 10, 11, 10, 10, 10, 10, 10,  8, 10, 10,
    /* 171 */ 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  8,
    /* 190 */ 10,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8, 10,  8,  6,  7,  1,  1,  1,  1,
    /* 209 */  6,  6,  3,  6,  3,  1,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
};

/* Source pixels for a raw UnitType id, or 0 when the id is unknown. */
static inline int sc64_source_sight_raw_pixels(int raw)
{
    return raw >= 0 && raw < (int)sizeof(sc64_source_sight_tiles) ?
        sc64_source_sight_tiles[raw] * 32 : 0;
}

#endif /* SC64_SOURCE_SIGHT_TILES_H */
