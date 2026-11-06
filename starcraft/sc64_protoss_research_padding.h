/* Experimental row-padding codec. Never selects or authenticates a journal.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PROTOSS_RESEARCH_PADDING_H
#define SC64_PROTOSS_RESEARCH_PADDING_H
#include "sc64_protoss_research.h"
#include <stddef.h>
#define SC64_PR_PAD_TUPLE_VALID(rows,width,used) \
    (((width)==28 && (used)==1 && ((rows)==96 || (rows)==160)) || \
     ((width)==25 && (used)==3 && (rows)==160))
/* Caller must authenticate version and context before calling. Decode checks
 * the entire wire before mutating candidate; candidate must be detached from
 * SRAM and the live world. Existing row validators must run after normalize. */
static inline int sc64_pr_padding_read(uint8_t *candidate, size_t rows, size_t width,
                                       unsigned used, unsigned owners,
                                       Sc64ProtossResearchLevels *out)
{
    uint8_t packed[SC64_PR_LEVEL_WIRE_BYTES]={0};
    Sc64ProtossResearchLevels value;
    if (!candidate || !out || !SC64_PR_PAD_TUPLE_VALID(rows,width,used)) return 0;
    for (size_t i=0;i<rows;i++) {
        uint8_t x=candidate[i*width+width-1];
        for (unsigned bit=used;bit<8;bit++) {
            size_t n=i*(8-used)+(bit-used);
            if (n<96) packed[n/8]|=(uint8_t)(((x>>bit)&1u)<<(n%8));
            else if ((x>>bit)&1u) return 0;
        }
    }
    if (!sc64_pr_levels_decode(&value,packed,owners)) return 0;
    for (size_t i=0;i<rows;i++) candidate[i*width+width-1]&=(uint8_t)((1u<<used)-1u);
    *out=value;
    return 1;
}
/* Writer only touches detached, already validated/encoded candidate rows. */
static inline int sc64_pr_padding_write(uint8_t *candidate, size_t rows, size_t width,
                                        unsigned used, unsigned owners,
                                        const Sc64ProtossResearchLevels *levels)
{
    uint8_t packed[SC64_PR_LEVEL_WIRE_BYTES];
    if (!candidate || !SC64_PR_PAD_TUPLE_VALID(rows,width,used) ||
        !sc64_pr_levels_encode(packed,levels,owners)) return 0;
    for (size_t i=0;i<rows;i++)
        if (candidate[i*width+width-1] & (uint8_t)~((1u<<used)-1u)) return 0;
    for (size_t i=0;i<rows;i++) {
        uint8_t *x=&candidate[i*width+width-1];
        for (unsigned bit=used;bit<8;bit++) {
            size_t n=i*(8-used)+(bit-used);
            if (n<96) *x|=(uint8_t)(((packed[n/8]>>(n%8))&1u)<<bit);
        }
    }
    return 1;
}
#endif
