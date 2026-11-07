/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_VISUAL_CAPTURE_H
#define SC64_VISUAL_CAPTURE_H
/* Visual-only frontend never accepts the actor observer's RAM request (2). */
static inline bool sc64_visual_capture_allowed(int capture) { return capture==0 || capture==1; }
#endif
