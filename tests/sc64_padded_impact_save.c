/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Synthetic save with the standard Z10 building offsets and row size. */
typedef uint8_t u8;
#define MAX_UNITS 240
#define MAX_BUILDINGS 160
#define SC_RU_COUNT 64
#define SC64_PACKED_BUILDING_SAVE_BYTES 28
#define SC64_PACKED_BUILDING_SAVE_USED_BITS 217
#define SC64_BC_SAVE_EXTRA_BYTES 0
#define SC64_ZP_FULL_STORAGE
#define SC64_ZP_CAMPAIGN_STORAGE
#define SC64_SHARED_TRANSPORT
#define SC64_WORKER_CARGO_KIND
#define SC64_BUILDING_PAD_IMPACT_SAVE
typedef struct {
    u8 before_buildings[8404];
    struct { u8 bytes[SC64_PACKED_BUILDING_SAVE_BYTES]; } buildings[MAX_BUILDINGS];
    u8 after_buildings[3412];
} RaceSave;

#include "../starcraft/sc64_impact_save_types.h"
#include "../starcraft/sc64_padded_impact_payload.h"

_Static_assert(sizeof(RaceSave) == 16296 && RACE_IMPACT_RAW_SAVE_CAPACITY == 8,
               "fixture must use the existing standard save layout");
_Static_assert(RACE_IMPACT_SAVE_CAPACITY == 32 && sizeof(RaceImpactRecord) == 16368,
               "new body must preserve journal size and cover the full queue");

int main(void)
{
    RaceSave original;
    memset(&original, 0xad, sizeof(original));
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i)
        original.buildings[i].bytes[SC64_PACKED_BUILDING_SAVE_BYTES - 1] = (u8)(i & 1u);

    Sc64ImpactEvent rows[RACE_IMPACT_ROWS], decoded[RACE_IMPACT_ROWS];
    for (unsigned i = 0; i < RACE_IMPACT_ROWS; ++i) {
        Sc64ImpactHit hit = {(int)(i * 29u % 400u), (int)(i * 217u % 4096u),
                             (int)(i % 16u + 1u), (int)(i % 5u),
                             (int)(i % 8u) - 1, (int)(i * 23u % 64u)};
        assert(sc64_impact_pack(&rows[i], &hit, i * 8u + 1u));
    }

    for (unsigned count = 9; count <= RACE_IMPACT_ROWS; ++count) {
        RaceSave packed = original;
        u8 tail[SC64_BUILDING_PAD_IMPACT_TAIL_MAX + 1];
        memset(tail, 0xac, sizeof(tail));
        unsigned bytes = sc64_padded_impact_encode(&packed, tail, rows, count);
        unsigned excess = count > 23 ? count * 6u - 140u : 0;
        assert(bytes == 4u + excess && tail[bytes] == 0xacu);
        assert(!memcmp(tail, "IPB", 3) && tail[3] == count);
        RaceSave malformed = packed;
        assert(sc64_padded_impact_decode(&malformed, tail, bytes - 1, decoded) == -1);
        assert(!memcmp(&malformed, &packed, sizeof(packed)));
        tail[3] = 33;
        assert(sc64_padded_impact_decode(&malformed, tail, bytes, decoded) == -1);
        tail[3] = (u8)count;
        if (count <= 23) {
            malformed.buildings[MAX_BUILDINGS - 1].bytes[27] |= 0x80u;
            assert(sc64_padded_impact_decode(&malformed, tail, bytes, decoded) == -1);
        } else {
            tail[bytes - 1] |= 0x80u; /* Invalid source type in the last event. */
            assert(sc64_padded_impact_decode(&malformed, tail, bytes, decoded) == -1);
            tail[bytes - 1] &= 0x7fu;
        }
        assert(sc64_padded_impact_decode(&packed, tail, bytes, decoded) == (int)count);
        assert(!memcmp(decoded, rows, count * sizeof(*rows)));
        assert(!memcmp(&packed, &original, sizeof(packed)));
    }

    u8 tail[SC64_BUILDING_PAD_IMPACT_TAIL_MAX];
    RaceSave invalid = original;
    invalid.buildings[55].bytes[27] |= 0x80u;
    RaceSave unchanged = invalid;
    assert(!sc64_padded_impact_encode(&invalid, tail, rows, 9));
    assert(!memcmp(&invalid, &unchanged, sizeof(invalid)));
    rows[0].steps = 0;
    assert(!sc64_padded_impact_encode(&original, tail, rows, 9));
    puts("padded impact payload: exact round-trip of 9 through 32 events");
}
