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
#endif
