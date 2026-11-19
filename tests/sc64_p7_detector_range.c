/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "../starcraft/sc64_p7_observer_rules.h"

/* The unchanged scalar distance decides pairs that survive the cull. The
 * reference below independently uses it without checking either axis first. */
static unsigned sc64_p7_observer_xy_length(unsigned x, unsigned y)
{
    unsigned major = x > y ? x : y;
    unsigned minor = x > y ? y : x;
    if (major / 4u < minor)
        major = major - major / 16u + minor * 3u / 8u - major / 64u +
                minor * 3u / 256u;
    return major;
}

#include "../starcraft/sc64_p7_detector_range.h"

static void check(unsigned dx, unsigned dy, unsigned range)
{
    const int right = 1, bottom = 3;
    const int left = dx ? (int)dx : right - 1;
    const int top = dy ? (int)dy + bottom - 1 : bottom - 1;
    const int right2 = left + 12, bottom2 = top + 17;
    int distance = (int)sc64_p7_observer_xy_length(dx, dy);
    int want = distance >= 0 && (unsigned)distance <= range;

    assert(sc64_p7_detector_rect_in_range(-16, -17, right, bottom,
        left, top, right2, bottom2, range) == want);
    assert(sc64_p7_detector_rect_in_range(left, top, right2, bottom2,
        -16, -17, right, bottom, range) == want);
}

int main(void)
{
    const unsigned ranges[] = {
        SC64_P7_CANNON_DETECTION_SOURCE_PX,
        SC64_SPORE_DETECTION_SOURCE_PX,
        SC64_P7_OBSERVER_BASE_SIGHT_SOURCE_PX,
        SC64_OVERLORD_DETECTION_SOURCE_PX
    };
    const unsigned outliers[] = {
        0, 1, 224, 288, 352, 353, 65535, UINT_MAX / 3u - 1u,
        UINT_MAX / 3u, UINT_MAX / 3u + 1u, INT_MAX - 256u
    };

    for (unsigned i = 0; i < sizeof(ranges) / sizeof(ranges[0]); ++i) {
        for (unsigned dx = 0; dx <= 384; ++dx)
            for (unsigned dy = 0; dy <= 384; ++dy)
                check(dx, dy, ranges[i]);
        for (unsigned x = 0; x < sizeof(outliers) / sizeof(outliers[0]); ++x)
            for (unsigned y = 0; y < sizeof(outliers) / sizeof(outliers[0]); ++y)
                check(outliers[x], outliers[y], ranges[i]);
    }
    puts("Detector axis culling matches the original inclusive range.");
    return 0;
}
