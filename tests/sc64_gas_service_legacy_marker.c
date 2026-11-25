/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A reader without the queue experiment must reject its authenticated marker. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint16_t u16; typedef uint32_t u32;
typedef struct { int x, y; } Geyser;
#define MAX_RACE_GEYSERS 12
#define SC64_SOURCE_GAS 1
#include "../starcraft/sc64_source_gas_save.h"
int main(void)
{
    assert(sc64_gas_save_count_valid(3));
    assert(!sc64_gas_save_count_valid(0x20000 | 3));
    assert(!sc64_gas_save_count_valid(0x30000 | 3));
    return 0;
}
