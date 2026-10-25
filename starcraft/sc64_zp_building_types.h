#ifndef SC64_ZP_BUILDING_TYPES_H
#define SC64_ZP_BUILDING_TYPES_H

/* Proposed runtime-only compact Building record for a future ZP storage
 * profile. The legacy save schema and live Building ABI are unchanged. Keep
 * HP/max HP/shield as int because combat takes int pointers to these members;
 * keep build_progress as int because the construction path mutates it through
 * the full accepted -1..65534 range. Narrow coordinates/raw IDs only after
 * the source/save validator has checked their existing bounds. */
typedef struct {
    int hp, maxhp, shield, build_progress;
    s16 x, y, raw_type, raw_owner;
    u16 queue_progress, flags, larva_timer, shield_remainder;
    u8 owner, type, alive, queue[5];
    u8 queue_head, queue_count, under_construction, level, larva;
} Sc64ZpCompactBuilding;

_Static_assert(sizeof(Sc64ZpCompactBuilding) == 48,
               "compact ZP Building runtime record budget");

#endif
