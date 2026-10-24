#ifndef SC64_P5_COUNT_SNAPSHOT_H
#define SC64_P5_COUNT_SNAPSHOT_H

/* Ephemeral cache for the six authenticated P5 Command/Bring rows. It is
 * deliberately absent from every persisted runtime/save structure. */
typedef struct {
    u32 count[6];
    u8 active;
} Sc64P5CountSnapshot;

enum {
    SC64_P5_COUNT_CURRENT_ANY = 0,
    SC64_P5_COUNT_ALL_HERO = 1,
    SC64_P5_COUNT_CURRENT_HERO_LOC2 = 2,
    SC64_P5_COUNT_CURRENT_ZEALOT_LOC2 = 3,
    SC64_P5_COUNT_CURRENT_ANY_LOC4 = 4,
    SC64_P5_COUNT_CURRENT_ANY_LOC5 = 5
};

#endif
