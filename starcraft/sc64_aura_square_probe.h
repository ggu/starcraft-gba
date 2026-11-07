/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_AURA_SQUARE_PROBE_H
#define SC64_AURA_SQUARE_PROBE_H
/* Source-pixel finder bounds, held only for the current Arbiter iteration. */
typedef struct { int left, top, right, bottom; } Sc64AuraSquare;
static inline Sc64AuraSquare sc64_aura_square_make(int x, int y, int width, int height)
{
    int cx=x*2, cy=y*2;
    Sc64AuraSquare box={cx-160,cy-160,cx+160,cy+160};
    if(box.left<0)box.left=0;
    if(box.top<0)box.top=0;
    if(box.right>width)box.right=width;
    if(box.bottom>height)box.bottom=height;
    return box;
}
static inline int sc64_aura_square_contains(const Sc64AuraSquare *box,
                                            int left,int top,int right,int bottom)
{
    return left<box->right && right-1>=box->left &&
           top<box->bottom && bottom-1>=box->top;
}
#endif
