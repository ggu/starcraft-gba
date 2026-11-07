/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SAVE_DECODE_CONTEXT_H
#define SC64_SAVE_DECODE_CONTEXT_H
/* Standalone non-staged save fixtures use the original strict decoder.
 * game.c defines the explicit authenticated context before these headers. */
#ifndef SC64_GCTX_DECL
#define SC64_GCTX_DECL
#define SC64_GCTX_ARG
#define SC64_GUARD_DECODE(c,s,w,h,o) race_save_decode_unit(c,s,w,h,o)
#endif
#endif
