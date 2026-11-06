/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../tools/sc64_headless_read_bounds.h"
int main(void)
{
    const size_t rom = 0x2000000u;
    const uint32_t starts[] = {0x02000000u,0x03000000u,0x04000000u,0x05000000u,0x06000000u,0x08000000u};
    const uint32_t ends[] = {0x02040000u,0x03008000u,0x04000002u,0x05000400u,0x06018000u,0x0a000000u};
    for (unsigned i=0;i<sizeof(starts)/sizeof(starts[0]);++i) {
        assert(sc64_headless_read_allowed(starts[i],1,rom));
        assert(sc64_headless_read_allowed(ends[i]-1,1,rom));
        assert(!sc64_headless_read_allowed(starts[i],0,rom));
        assert(!sc64_headless_read_allowed(ends[i]-1,2,rom));
        assert(!sc64_headless_read_allowed(ends[i],1,rom));
        assert(!sc64_headless_read_allowed(starts[i],UINT32_MAX,rom));
    }
    assert(!sc64_headless_read_allowed(0x04000002u,1,rom));
    assert(!sc64_headless_read_allowed(0x04000100u,2,rom));
    assert(!sc64_headless_read_allowed(0x07000000u,1,rom));
    assert(!sc64_headless_read_allowed(0x0e000000u,1,rom));
    assert(!sc64_headless_read_allowed(0xffffffffu,2,rom));
    assert(sc64_headless_read_allowed(0x08000fffu,1,0x1000u));
    assert(!sc64_headless_read_allowed(0x08000fffu,2,0x1000u));
    assert(!sc64_headless_write_allowed(1));
    assert(sc64_headless_write_allowed(0));
    puts("headless read bounds and physical write denial: PASS");
    return 0;
}
