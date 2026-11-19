/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_DETECTOR_RANGE_H
#define SC64_P7_DETECTOR_RANGE_H

/* Validated half-open rectangles use inclusive endpoint gaps. For the
 * detector ranges (at most 352), a gap outside the range cannot pass the
 * legacy metric. In its diagonal branch the minor-axis terms compensate
 * for the major-axis deductions whenever the multiplication fits. If it
 * overflows, the remaining major-axis distance alone exceeds 352. */
static inline int
sc64_p7_detector_rect_in_range(int al, int at, int ar, int ab,
                               int bl, int bt, int br, int bb,
                               unsigned range)
{
    unsigned dx = al >= br ? al - (br - 1) : bl >= ar ? bl - (ar - 1) : 0;
    if (dx > range) return 0;
    unsigned dy = at >= bb ? at - (bb - 1) : bt >= ab ? bt - (ab - 1) : 0;
    if (dy > range) return 0;
    return sc64_p7_observer_xy_length(dx, dy) <= range;
}

#endif
