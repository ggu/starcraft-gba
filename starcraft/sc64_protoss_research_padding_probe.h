/* Standalone research journal experiment; not a runtime save format.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PROTOSS_RESEARCH_PADDING_PROBE_H
#define SC64_PROTOSS_RESEARCH_PADDING_PROBE_H
#ifndef GBA_HOST
#error This experimental codec is restricted to HOST fixtures
#endif
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "sc64_protoss_research.h"

typedef enum { SC64_PR_PAD_COMPACT, SC64_PR_PAD_TYPED } Sc64PrPadFormat;
static inline int sc64_pr_pad_layout(Sc64PrPadFormat f, size_t rows,
                                     size_t *width, unsigned *used)
{
    if (f==SC64_PR_PAD_COMPACT && (rows==96 || rows==160)) {
        *width=28; *used=1; return 1; /* byte 27: bit 0 is data */
    }
    if (f==SC64_PR_PAD_TYPED && rows==160) {
        *width=25; *used=3; return 1; /* byte 24: bits 0..2 are data */
    }
    return 0;
}
static inline int sc64_pr_pad_overlap(const void *a,size_t an,const void *b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if (!an || !bn) return 0;
    return x<=y ? y-x<an : x-y<bn;
}
/* A caller supplies an explicitly distinct candidate tag. This helper never
 * selects a production tag or authorizes a journal write. */
static inline int sc64_pr_pad_tag(uint32_t selected,uint32_t candidate,
                                  const uint32_t *old,size_t old_count)
{
    if (!candidate || selected!=candidate || !old || !old_count || old_count>5) return 0;
    for (size_t i=0;i<old_count;i++) {
        if (!old[i] || old[i]==candidate) return 0;
        for (size_t j=0;j<i;j++) if (old[i]==old[j]) return 0;
    }
    return 1;
}
static inline int sc64_pr_pad_transform(uint8_t *out,size_t out_len,
    Sc64ProtossResearchLevels *decoded,const uint8_t *in,size_t in_len,
    const Sc64ProtossResearchLevels *levels,unsigned owners,Sc64PrPadFormat format,
    size_t rows,uint32_t selected,uint32_t candidate,const uint32_t *old,
    size_t old_count,int encode)
{
    size_t width; unsigned used; uint8_t packed[SC64_PR_LEVEL_WIRE_BYTES]={0};
    Sc64ProtossResearchLevels value;
    if (!out || !in || !sc64_pr_pad_layout(format,rows,&width,&used) ||
        in_len!=rows*width || out_len!=in_len ||
        !sc64_pr_pad_tag(selected,candidate,old,old_count) ||
        sc64_pr_pad_overlap(out,out_len,in,in_len) ||
        (old_count && sc64_pr_pad_overlap(out,out_len,old,old_count*sizeof(*old)))) return 0;
    if (encode) {
        if (!levels || sc64_pr_pad_overlap(out,out_len,levels,sizeof(*levels)) ||
            !sc64_pr_levels_encode(packed,levels,owners)) return 0;
    } else {
        if (!decoded || sc64_pr_pad_overlap(decoded,sizeof(*decoded),in,in_len) ||
            sc64_pr_pad_overlap(decoded,sizeof(*decoded),out,out_len) ||
            (old_count && sc64_pr_pad_overlap(decoded,sizeof(*decoded),old,old_count*sizeof(*old)))) return 0;
    }
    for (size_t row=0;row<rows;row++) {
        uint8_t byte=in[row*width+width-1];
        for (unsigned bit=used;bit<8;bit++) {
            size_t n=row*(8-used)+(bit-used);
            unsigned set=(byte>>bit)&1u;
            if (encode) { if (set) return 0; }
            else if (n<96) packed[n/8]|=(uint8_t)(set<<(n%8));
            else if (set) return 0;
        }
    }
    if (!encode && !sc64_pr_levels_decode(&value,packed,owners)) return 0;
    memcpy(out,in,in_len);
    for (size_t row=0;row<rows;row++) {
        uint8_t *byte=&out[row*width+width-1];
        *byte&=(uint8_t)((1u<<used)-1u);
        if (encode) for (unsigned bit=used;bit<8;bit++) {
            size_t n=row*(8-used)+(bit-used);
            if (n<96) *byte|=(uint8_t)(((packed[n/8]>>(n%8))&1u)<<bit);
        }
    }
    if (!encode) *decoded=value;
    return 1;
}
#endif
