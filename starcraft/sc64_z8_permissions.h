/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Z8_PERMISSIONS_H
#define SC64_Z8_PERMISSIONS_H

#include <stdint.h>

/* PUNI indexes source slots, not live engine owners. */
#define SC64_Z8_PERMISSION_OWNER_COUNT 12u
#define SC64_Z8_PERMISSION_RAW_COUNT 228u
#define SC64_Z8_PERMISSION_BYTES 342u

typedef struct {
    uint8_t bits[SC64_Z8_PERMISSION_BYTES];
} Sc64Z8PermissionTable;

static inline int sc64_z8_permission_available(const Sc64Z8PermissionTable *table,
                                                unsigned owner, unsigned raw)
{
    unsigned bit;
    if (!table || owner >= SC64_Z8_PERMISSION_OWNER_COUNT ||
        raw >= SC64_Z8_PERMISSION_RAW_COUNT) return 0;
    bit = owner * SC64_Z8_PERMISSION_RAW_COUNT + raw;
    return (table->bits[bit >> 3] >> (bit & 7u)) & 1u;
}
#endif
