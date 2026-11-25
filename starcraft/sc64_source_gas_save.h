#ifndef SC64_SOURCE_GAS_SAVE_H
#define SC64_SOURCE_GAS_SAVE_H

/* The upper halves of the existing signed coordinate words carry source-site
 * depletion in source ordinal order. A legacy in-range coordinate has zero
 * upper bits. The authenticated RaceSave size and trailer remain unchanged. */
#ifdef SC64_SOURCE_GAS
typedef unsigned (*Sc64GasSiteCapacity)(int, int, int, unsigned);
_Static_assert(sizeof(int) == sizeof(u32), "gas save words must be 32 bits");

static inline int sc64_gas_save_coordinate(int word)
{
    return (int)((unsigned)word & 0xffffu);
}

static inline unsigned sc64_gas_save_debt(const Geyser *rows, unsigned ordinal)
{
    unsigned row = ordinal >> 1;
    int word = (ordinal & 1u) ? rows[row].y : rows[row].x;
    return (unsigned)word >> 16;
}

static inline int sc64_gas_save_debts_valid(const Geyser *rows, int mode, int map,
                                      int mission, Sc64GasSiteCapacity capacity)
{
    for (unsigned i = 0; i < 2u * MAX_RACE_GEYSERS; ++i) {
        unsigned used = sc64_gas_save_debt(rows, i);
        if (used && used > capacity(mode, map, mission, i)) return 0;
    }
    return 1;
}

static inline int sc64_gas_save_pack(Geyser *rows, const u16 *used, int mode,
                               int map, int mission, Sc64GasSiteCapacity capacity)
{
    /* Validate everything before changing the candidate. */
    for (unsigned i = 0; i < MAX_RACE_GEYSERS; ++i)
        if (rows[i].x < 0 || rows[i].x > 65535 ||
            rows[i].y < 0 || rows[i].y > 65535) return 0;
    for (unsigned i = 0; i < 2u * MAX_RACE_GEYSERS; ++i)
        if (used[i] && used[i] > capacity(mode, map, mission, i)) return 0;
    for (unsigned i = 0; i < MAX_RACE_GEYSERS; ++i) {
        u32 x = (u32)rows[i].x | ((u32)used[2u*i] << 16);
        u32 y = (u32)rows[i].y | ((u32)used[2u*i+1u] << 16);
        memcpy(&rows[i].x, &x, sizeof x);
        memcpy(&rows[i].y, &y, sizeof y);
    }
    return 1;
}

static inline void sc64_gas_save_publish(const Geyser *rows, Geyser *out, u16 *used)
{
    for (unsigned i = 0; i < MAX_RACE_GEYSERS; ++i) {
        used[2u*i] = (u16)sc64_gas_save_debt(rows, 2u*i);
        used[2u*i+1u] = (u16)sc64_gas_save_debt(rows, 2u*i+1u);
        out[i].x = (int)sc64_gas_save_coordinate(rows[i].x);
        out[i].y = (int)sc64_gas_save_coordinate(rows[i].y);
    }
}
#else
static inline int sc64_gas_save_coordinate(int word)
{
    return word;
}
#endif
#endif
