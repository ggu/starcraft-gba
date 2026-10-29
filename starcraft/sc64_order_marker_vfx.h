/* ROM cursor-marker candidate: source frame sequence and centered canvas.
 * PC reference identifies destination feedback; N64 order UI is untraced.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ORDER_MARKER_VFX_H
#define SC64_ORDER_MARKER_VFX_H
#include <stdint.h>
#include "../common/gba.h"
#define SC64_ORDER_MARKER_SOURCE_UPDATES 5u
#define SC64_ORDER_MARKER_GBA_TICKS 7u

typedef struct {
    uint8_t x, y, w, h;
    const unsigned char *pixels;
} Sc64OrderMarkerFrame;

/* Convert a seven-tick 30-Hz remaining lifetime to the source 24-Hz phase. */
static inline int sc64_order_marker_phase(unsigned remaining, unsigned *phase)
{
    if (!phase || !remaining || remaining > SC64_ORDER_MARKER_GBA_TICKS) return 0;
    *phase = (SC64_ORDER_MARKER_GBA_TICKS - remaining) * 4u / 5u;
    return *phase < SC64_ORDER_MARKER_SOURCE_UPDATES;
}
#ifdef SC64_ORDER_MARKER_ART
#ifndef SC64_ORDER_MARKER_BANK_HEADER
#define SC64_ORDER_MARKER_BANK_HEADER "sc64_order_marker_generated.h"
#endif
#include SC64_ORDER_MARKER_BANK_HEADER
/* Source marker runs five source updates then hides; x,y are camera-adjusted
 * screen pixels. Clip to the battlefield even without global scene clipping. */
static inline int sc64_order_marker_draw(int x, int y, uint32_t age_source_updates)
{
    if (age_source_updates >= SC64_ORDER_MARKER_SOURCE_UPDATES) return 0;
    const Sc64OrderMarkerFrame *f = &sc64_order_marker_frames[age_source_updates];
    int left = x - 11 + f->x, top = y - 7 + f->y;
    int col0 = left < 0 ? -left : 0;
    int col1 = left + f->w > 240 ? 240 - left : f->w;
    int row0 = top < 12 ? 12 - top : 0;
    int row1 = top + f->h > 120 ? 120 - top : f->h;
    if (col0 >= col1 || row0 >= row1) return 1;
    if (col0 == 0 && col1 == f->w && row0 == 0 && row1 == f->h)
        gfx_blit_sprite(left, top, f->w, f->h, f->pixels);
    else for (int row = row0; row < row1; ++row)
        gfx_blit_sprite(left + col0, top + row, col1 - col0, 1,
                        f->pixels + row * f->w + col0);
    return 1;
}
#endif
#endif
