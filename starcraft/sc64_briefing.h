/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BRIEFING_H
#define SC64_BRIEFING_H

/* Campaign dialogue data is generated as static, prewrapped pages.  Keeping
 * this model pointer-based lets asset-free builds omit generated content
 * entirely while using the same bounded frontend. */
typedef struct Sc64BriefingPage {
    const char *speaker;
    const char *text;       /* Prewrapped to 36 columns by at most 7 rows. */
    int portrait;           /* Generated portrait ID, or -1 for objectives. */
} Sc64BriefingPage;

typedef struct Sc64Briefing {
    const Sc64BriefingPage *pages;
    unsigned page_count;
} Sc64Briefing;

#define SC64_BRIEFING_COLUMNS 36u
#define SC64_BRIEFING_ROWS 7u
#define SC64_BRIEFING_MAX_PAGES 255u
#define SC64_BRIEFING_MAX_TEXT_BYTES (SC64_BRIEFING_COLUMNS * SC64_BRIEFING_ROWS + \
                                      SC64_BRIEFING_ROWS - 1u)

#endif
