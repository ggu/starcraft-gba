/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_z8_clock.h"

int main(void)
{
    unsigned native = 0;
    for (unsigned frame = 1; frame <= 3000; ++frame) {
        native += (unsigned)sc64_z8_native_step(frame);
        assert(native == (4u * frame) / 5u);
    }
    assert(native == 2400u);
    assert(!sc64_z8_native_step(0));
    puts("Z8 clock: 2400 native steps per 3000 active engine updates");
    return 0;
}
