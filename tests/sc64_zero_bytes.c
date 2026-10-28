/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_zero_bytes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned rng = 0x7495e1b3u;
static unsigned cases;
static unsigned next_random(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}

static void compare(const void *object, size_t length)
{
    static const unsigned char zero[512] = {0};
    assert(length <= sizeof(zero));
    int expected = object && memcmp(object, zero, length) == 0;
    assert(sc64_bytes_are_zero(object, length) == expected);
    ++cases;
}

int main(void)
{
    static const unsigned char values[] = {1, 0x80, 0xff};
    compare(NULL, 0);
    compare(NULL, 56);
    for (unsigned offset = 0; offset < 8; ++offset)
        for (size_t length = 0; length <= 256; ++length) {
            /* The tested range ends exactly at the allocation's ASan redzone. */
            unsigned char *allocation = malloc(offset + length + (length == 0));
            assert(allocation);
            unsigned char *object = allocation + offset;
            memset(object, 0, length);
            compare(object, length);
            for (size_t byte = 0; byte < length; ++byte) {
                for (unsigned v = 0; v < sizeof(values); ++v) {
                    object[byte] = values[v];
                    compare(object, length);
                }
                object[byte] = 0;
            }
            free(allocation);
        }
    for (unsigned trial = 0; trial < 80000; ++trial) {
        size_t length = next_random() % 513u;
        unsigned offset = next_random() % 8u;
        unsigned char *allocation = malloc(offset + length + 1);
        assert(allocation);
        unsigned char *object = allocation + offset;
        for (size_t i = 0; i < length; ++i) object[i] = (unsigned char)next_random();
        compare(object, length);
        memset(object, 0, length);
        compare(object, length);
        free(allocation);
    }
    printf("zero bytes: %u memcmp-oracle cases, alignments, exact ends and null pass\n", cases);
    return 0;
}
