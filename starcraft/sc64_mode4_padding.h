/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MODE4_PADDING_H
#define SC64_MODE4_PADDING_H

/* Padding after the second 240x160 framebuffer. The optional VRAM
 * death pool and extra terrain rows take turns owning the first range. */
#define SC64_MODE4_PADDING_BASE  0x06013600u
#define SC64_MODE4_TEXT_BOX_BASE 0x06013fe0u
#define SC64_MODE4_PADDING_END   0x06014000u

#endif
