#ifndef SC64_Z5_COUNT_SNAPSHOT_H
#define SC64_Z5_COUNT_SNAPSHOT_H

/* Phase-local cache for authenticated numeric Z5 Command/Bring rows. This is
 * transient world-context state and is intentionally absent from save data. */
typedef struct {
    u16 count[SC64_Z5_SOURCE_CONDITION_COUNT];
    u8 valid[(SC64_Z5_SOURCE_CONDITION_COUNT + 7) / 8];
    u8 active;
} Sc64Z5CountSnapshot;
_Static_assert(sizeof(Sc64Z5CountSnapshot) <= 96,
               "Z5 count snapshot context bound");

#endif
