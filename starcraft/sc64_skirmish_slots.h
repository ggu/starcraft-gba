/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SKIRMISH_SLOTS_H
#define SC64_SKIRMISH_SLOTS_H

enum { SC64_SKIRMISH_SEAT_COUNT = 3, SC64_SKIRMISH_SEAT_MASK = 7 };

static inline unsigned sc64_skirmish_dense_seats(int bots)
{
    return bots >= 1 && bots <= SC64_SKIRMISH_SEAT_COUNT ? (1u << bots) - 1u : 0;
}

static inline int sc64_skirmish_seat_count(unsigned seats)
{
    return (int)((seats & 1u) + ((seats >> 1) & 1u) + ((seats >> 2) & 1u));
}

/* The existing engine owner IDs are assigned in order of the occupied
 * seats. Physical map starts are numbered separately, with the human at 0. */
static inline int sc64_skirmish_bot_seat(unsigned seats, int rank)
{
    for (int seat = 0; seat < SC64_SKIRMISH_SEAT_COUNT; ++seat)
        if ((seats & (1u << seat)) && rank-- == 0) return seat;
    return -1;
}

static inline int sc64_skirmish_sparse_valid(int bots, unsigned seats)
{
    return !(seats & ~SC64_SKIRMISH_SEAT_MASK) &&
        sc64_skirmish_seat_count(seats) == bots && seats != sc64_skirmish_dense_seats(bots);
}

/* Sparse games have at most two bots. Save their physical seats in the
 * otherwise inactive third race word, retaining its low race value. Dense
 * games keep their exact old encoding. Old readers refuse the new marker. */
enum { SC64_SKIRMISH_SEAT_TAG = 0x534c0000 };

static inline int sc64_skirmish_seat_word(int last_race, unsigned sparse)
{
    return sparse ? SC64_SKIRMISH_SEAT_TAG | (int)(sparse << 2) | last_race : last_race;
}

static inline int sc64_skirmish_read_seat_word(int custom, int bots, int word,
                                               int *last_race, unsigned *sparse)
{
    if (word >= 0 && word < 3) {
        *last_race = word;
        *sparse = 0;
        return 1;
    }
    unsigned bits = (unsigned)word;
    unsigned seats = (bits >> 2) & SC64_SKIRMISH_SEAT_MASK;
    unsigned race = bits & 3u;
    if (custom != 1 || bots < 1 || bots >= SC64_SKIRMISH_SEAT_COUNT ||
        (bits & ~31u) != SC64_SKIRMISH_SEAT_TAG || race >= 3 ||
        !sc64_skirmish_sparse_valid(bots, seats)) return 0;
    *last_race = (int)race;
    *sparse = seats;
    return 1;
}
/* Blood Bath's imported geometry order is a save contract: NW, SE, NE, SW.
 * The selected base IDs are packed in owner order: human then open CPU rows. */
enum {
    SC64_SKIRMISH_BLOOD_BATH_SOLO_ORDER = (3u << 2),
    SC64_SKIRMISH_BLOOD_BATH_DENSE_ORDER = 2u | (1u << 2) | (3u << 6),
    SC64_SKIRMISH_START_TAG = 0x534d0000
};

static inline int sc64_skirmish_start_base(unsigned order, int rank)
{
    return (int)((order >> (2u * (unsigned)rank)) & 3u);
}

static inline int sc64_skirmish_start_valid(int bots, unsigned mask, unsigned order)
{
    if (bots < 1 || bots > SC64_SKIRMISH_SEAT_COUNT ||
        (mask & ~SC64_SKIRMISH_SEAT_MASK) || sc64_skirmish_seat_count(mask) != bots ||
        !order || (order >> (2u * (unsigned)(bots + 1)))) return 0;
    unsigned used = 0;
    for (int rank = 0; rank <= bots; ++rank) {
        unsigned bit = 1u << sc64_skirmish_start_base(order, rank);
        if (used & bit) return 0;
        used |= bit;
    }
    return 1;
}

static inline int sc64_skirmish_start_word(int last_race, unsigned mask, unsigned order)
{
    return SC64_SKIRMISH_START_TAG | (int)(order << 5) | (int)(mask << 2) | last_race;
}

static inline int sc64_skirmish_read_start_word(int custom, int blood_bath, int bots, int word,
                                                int *last_race, unsigned *sparse, unsigned *order)
{
    unsigned bits = (unsigned)word;
    if ((bits & ~0x1fffu) != SC64_SKIRMISH_START_TAG) {
        if (!sc64_skirmish_read_seat_word(custom, bots, word, last_race, sparse)) return 0;
        *order = 0;
        return 1;
    }
    unsigned race = bits & 3u, mask = (bits >> 2) & SC64_SKIRMISH_SEAT_MASK;
    unsigned starts = (bits >> 5) & 0xffu;
    if (custom != 1 || !blood_bath || race >= 3 || !sc64_skirmish_start_valid(bots, mask, starts))
        return 0;
    *last_race = (int)race;
    *sparse = mask == sc64_skirmish_dense_seats(bots) ? 0 : mask;
    *order = starts;
    return 1;
}
#ifdef SC64_NATIVE_MELEE_STARTS
#include "sc64_native_start_selector.h"

/* Only the new allied-CPU mode needs a new tag. Free for All keeps the
 * previous word exactly; older cartridges reject Melee rather than silently
 * turning the saved allies into enemies. */
enum { SC64_SKIRMISH_MELEE_TAG = 0x534e0000 };

static inline int sc64_skirmish_melee_word(int last_race, unsigned mask, unsigned order)
{
    return SC64_SKIRMISH_MELEE_TAG | (sc64_skirmish_start_word(last_race, mask, order) & 0x1fff);
}

static inline int sc64_skirmish_read_match_word(int custom, int blood_bath, int bots, int word,
                                                int *last_race, unsigned *sparse,
                                                unsigned *order, unsigned *melee)
{
    unsigned bits = (unsigned)word;
    unsigned allied = (bits & ~0x1fffu) == SC64_SKIRMISH_MELEE_TAG;
    if (allied) word = SC64_SKIRMISH_START_TAG | (int)(bits & 0x1fffu);
    if (!sc64_skirmish_read_start_word(custom, blood_bath, bots, word, last_race, sparse, order))
        return 0;
    *melee = allied;
    return 1;
}

static inline unsigned sc64_skirmish_blood_bath_start_order(unsigned rows,
                                                              const Sc64NativeMeleeStarts *starts)
{
    /* CHK markers are SE, SW, NW, NE. The saved GBA base IDs are NW, SE, NE, SW. */
    static const uint8_t gba_base_by_marker[] = {1, 3, 0, 2};
    unsigned order = 0, rank = 0;
    for (unsigned row = 0; row < 4; ++row)
        if (rows & (1u << row))
            order |= (unsigned)gba_base_by_marker[starts->marker_by_row[row]] << (2u * rank++);
    return order;
}
#endif
#endif
