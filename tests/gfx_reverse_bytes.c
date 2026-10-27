/* SPDX-License-Identifier: GPL-3.0-or-later */
/* gfx_reverse_bytes matches a byte-by-byte reversal for every length and
 * source alignment, and writes nothing past the requested length. */
#include <assert.h>
#include <stdio.h>
#include "../common/gba.h"

int main(void)
{
    static u8 src[4 + 300] __attribute__((aligned(4)));
    static u8 dst[300 + 8];
    for (unsigned i = 0; i < sizeof(src); ++i) src[i] = (u8)(i * 37u + 11u);
    unsigned cases = 0;
    for (unsigned offset = 0; offset < 4; ++offset)
        for (unsigned n = 0; n <= 300; ++n) {
            for (unsigned i = 0; i < sizeof(dst); ++i) dst[i] = 0xa5;
            gfx_reverse_bytes(dst, src + offset, n);
            for (unsigned i = 0; i < n; ++i) assert(dst[i] == src[offset + n - 1 - i]);
            for (unsigned i = n; i < sizeof(dst); ++i) assert(dst[i] == 0xa5);
            ++cases;
        }
    printf("gfx_reverse_bytes: %u cases match a byte-by-byte reversal\n", cases);
    return 0;
}
