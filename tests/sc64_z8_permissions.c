/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include "../starcraft/sc64_z8_permissions.h"

int main(void)
{
    Sc64Z8PermissionTable table = {{0}};
    const unsigned bit = 3u * SC64_Z8_PERMISSION_RAW_COUNT + 143u;
    table.bits[bit >> 3] = (uint8_t)(1u << (bit & 7u));
    table.bits[SC64_Z8_PERMISSION_BYTES - 1u] |= 0x80u;
    assert(sc64_z8_permission_available(&table, 3, 143));
    assert(!sc64_z8_permission_available(&table, 3, 144));
    assert(sc64_z8_permission_available(&table, 11, 227));
    assert(!sc64_z8_permission_available(&table, 12, 0));
    assert(!sc64_z8_permission_available(&table, 0, 228));
    assert(!sc64_z8_permission_available(&table, UINT_MAX, UINT_MAX));
    assert(!sc64_z8_permission_available(0, 3, 143));
    return 0;
}
