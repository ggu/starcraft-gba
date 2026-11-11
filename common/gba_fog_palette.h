/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GBA_FOG_PALETTE_H
#define GBA_FOG_PALETTE_H

#include "gba.h"

/* Build a destination-index lookup that moves every installed RGB555 color
 * toward 60% channel brightness. Candidates may never be brighter by the
 * game's 3R+6G+B luma measure; ties select the lowest palette index. A black
 * target selects the darkest non-brightening palette color available. */
static inline void
gba_fog_palette_make(const u16 palette[256], u8 lookup[256])
{
    u8 red[256], green[256], blue[256];
    u16 luma[256];

    for (unsigned i = 0; i < 256; ++i) {
        red[i] = (u8)(palette[i] & 31u);
        green[i] = (u8)((palette[i] >> 5) & 31u);
        blue[i] = (u8)((palette[i] >> 10) & 31u);
        luma[i] = (u16)(3u * red[i] + 6u * green[i] + blue[i]);
    }

    for (unsigned src = 0; src < 256; ++src) {
        unsigned target_r = (red[src] * 3u) / 5u;
        unsigned target_g = (green[src] * 3u) / 5u;
        unsigned target_b = (blue[src] * 3u) / 5u;
        unsigned best = src, best_distance = ~0u;
        unsigned darkest_luma = ~0u;
        int black_target = !(target_r | target_g | target_b);

        for (unsigned dst = 0; dst < 256; ++dst) {
            unsigned dr, dg, db, distance;
            if (luma[dst] > luma[src]) continue;
            if (black_target) {
                if (luma[dst] < darkest_luma) {
                    darkest_luma = luma[dst];
                    best = dst;
                }
                continue;
            }
            /* Absolute differences avoid unsigned wraparound. */
            dr = red[dst] > target_r ? red[dst] - target_r : target_r - red[dst];
            dg = green[dst] > target_g ? green[dst] - target_g : target_g - green[dst];
            db = blue[dst] > target_b ? blue[dst] - target_b : target_b - blue[dst];
            distance = dr * dr + dg * dg + db * db;
            if (distance < best_distance) {
                best_distance = distance;
                best = dst;
            }
        }
        lookup[src] = (u8)best;
    }
}

#endif
