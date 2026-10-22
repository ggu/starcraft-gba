/*
 * Bounded decoder for StarCraft's 12-player PUNI availability table.
 *
 * The CHK PUNI payload is 12 x 228 per-player bytes, followed by 228 global
 * defaults and 12 x 228 use-default selectors. When a selector is nonzero,
 * the selected value is the global default; otherwise it is the player's
 * explicit byte. Any nonzero selected byte means available.
 *
 * N64 handler evidence: native-c/funcs_16.c, PUNI handler at 0x8002EC48,
 * which requires 5700 bytes and selects those arrays per player/unit. The
 * table's runtime destination address is not symbolized; its identity is an
 * inference from the handler and the OpenBW layout at bwgame.h:21522-21531.
 */
#ifndef SC64_UNIT_AVAILABILITY_H
#define SC64_UNIT_AVAILABILITY_H

#define SC64_PUNI_PLAYER_COUNT 12u
#define SC64_PUNI_UNIT_COUNT 228u
#define SC64_PUNI_BYTES 5700u
#define SC64_PUNI_PLAYER_BYTES (SC64_PUNI_PLAYER_COUNT * SC64_PUNI_UNIT_COUNT)
#define SC64_PUNI_GLOBAL_OFFSET SC64_PUNI_PLAYER_BYTES
#define SC64_PUNI_DEFAULT_SELECTOR_OFFSET \
    (SC64_PUNI_GLOBAL_OFFSET + SC64_PUNI_UNIT_COUNT)
#define SC64_PUNI_EFFECTIVE_BIT_BYTES \
    ((SC64_PUNI_PLAYER_COUNT * SC64_PUNI_UNIT_COUNT + 7u) / 8u)

/* Returns 1 on a valid lookup and writes 0/1 to `available`; otherwise
 * returns 0 without modifying the output. Inputs are fail-closed. */
static inline int sc64_puni_available(const unsigned char *puni,
                                      unsigned length,
                                      unsigned player,
                                      unsigned raw_unit,
                                      unsigned *available)
{
    unsigned player_offset;
    unsigned global_offset;
    unsigned selector_offset;
    unsigned value;

    if (!puni || !available || length != SC64_PUNI_BYTES ||
        player >= SC64_PUNI_PLAYER_COUNT || raw_unit >= SC64_PUNI_UNIT_COUNT)
        return 0;

    player_offset = player * SC64_PUNI_UNIT_COUNT + raw_unit;
    global_offset = SC64_PUNI_GLOBAL_OFFSET + raw_unit;
    selector_offset = SC64_PUNI_DEFAULT_SELECTOR_OFFSET + player_offset;
    value = puni[selector_offset] ? puni[global_offset] : puni[player_offset];
    *available = value != 0u;
    return 1;
}

/* Lookup in the generated effective bitmap (player-major, LSB-first). */
static inline int sc64_availability_bitmap_get(const unsigned char *bits,
                                                unsigned length,
                                                unsigned player,
                                                unsigned raw_unit,
                                                unsigned *available)
{
    unsigned bit_index;
    if (!bits || !available || length != SC64_PUNI_EFFECTIVE_BIT_BYTES ||
        player >= SC64_PUNI_PLAYER_COUNT || raw_unit >= SC64_PUNI_UNIT_COUNT)
        return 0;
    bit_index = player * SC64_PUNI_UNIT_COUNT + raw_unit;
    *available = (bits[bit_index >> 3] >> (bit_index & 7u)) & 1u;
    return 1;
}

#endif /* SC64_UNIT_AVAILABILITY_H */
