/* Source facts and compact per-owner completed levels. The runtime adapter must
 * authenticate producer/job state and requirements. No journal tag is chosen.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SC64_PROTOSS_RESEARCH_H
#define SC64_PROTOSS_RESEARCH_H
#include <stdint.h>
#include <string.h>

enum { SC64_PR_OWNER_COUNT=5, SC64_PR_FAMILY_COUNT=6, SC64_PR_LEVEL_WIRE_BYTES=8 };
enum { SC64_PR_GROUND_ARMOR, SC64_PR_AIR_ARMOR, SC64_PR_GROUND_WEAPONS,
       SC64_PR_AIR_WEAPONS, SC64_PR_SHIELDS, SC64_PR_SINGULARITY };
typedef struct { uint8_t level[SC64_PR_OWNER_COUNT][SC64_PR_FAMILY_COUNT]; } Sc64ProtossResearchLevels;
typedef struct { uint16_t mineral, gas, ticks; uint8_t max_level; } Sc64ProtossResearchQuote;

/* Source 00C/006 SHA-256 5c7a104f...; 5:4 timing is the project's policy,
 * not a measured N64 rate. `level` is the completed level before starting. */
static inline int sc64_pr_quote(unsigned family, unsigned level, Sc64ProtossResearchQuote *out)
{
    static const uint16_t base[]={100,150,100,100,200,150};
    static const uint16_t factor[]={75,75,50,75,100,0};
    unsigned max, cost, source_time;
    if (!out || family>=SC64_PR_FAMILY_COUNT) return 0;
    max=family==SC64_PR_SINGULARITY?1u:3u;
    if (level>=max) return 0;
    cost=base[family]+factor[family]*level;
    source_time=family==SC64_PR_SINGULARITY?2500u:4000u+480u*level;
    out->mineral=(uint16_t)cost; out->gas=(uint16_t)cost;
    out->ticks=(uint16_t)((source_time*5u+3u)/4u); out->max_level=(uint8_t)max;
    return 1;
}

static inline int sc64_pr_levels_valid(const Sc64ProtossResearchLevels *levels)
{
    if (!levels) return 0;
    for (unsigned o=0;o<SC64_PR_OWNER_COUNT;++o)
        for (unsigned f=0;f<SC64_PR_FAMILY_COUNT;++f)
            if (levels->level[o][f]>(f==SC64_PR_SINGULARITY?1u:3u)) return 0;
    return 1;
}

/* Stable compact levels, two bits each. Remaining high four bits are zero.
 * Producer-bound active jobs need a separately authenticated building codec. */
static inline int sc64_pr_levels_encode(uint8_t out[SC64_PR_LEVEL_WIRE_BYTES],
                                         const Sc64ProtossResearchLevels *levels)
{
    if (!out || !sc64_pr_levels_valid(levels)) return 0;
    memset(out,0,SC64_PR_LEVEL_WIRE_BYTES);
    for (unsigned o=0;o<SC64_PR_OWNER_COUNT;++o)
        for (unsigned f=0;f<SC64_PR_FAMILY_COUNT;++f) {
            unsigned n=o*SC64_PR_FAMILY_COUNT+f;
            out[n>>2]|=(uint8_t)(levels->level[o][f]<<((n&3u)*2u));
        }
    return 1;
}
static inline int sc64_pr_levels_decode(Sc64ProtossResearchLevels *out,
                                        const uint8_t in[SC64_PR_LEVEL_WIRE_BYTES])
{
    Sc64ProtossResearchLevels candidate;
    if (!out || !in || (in[7]&0xf0u)) return 0;
    for (unsigned o=0;o<SC64_PR_OWNER_COUNT;++o)
        for (unsigned f=0;f<SC64_PR_FAMILY_COUNT;++f) {
            unsigned n=o*SC64_PR_FAMILY_COUNT+f;
            candidate.level[o][f]=(uint8_t)((in[n>>2]>>((n&3u)*2u))&3u);
        }
    if (!sc64_pr_levels_valid(&candidate)) return 0;
    *out=candidate; return 1;
}

/* Caller must verify completed powered producer, owner, no conflicting job,
 * level permissions and the exact prerequisite before invoking this helper. */
static inline int sc64_pr_begin(unsigned family, unsigned level, int ready,
                                int *minerals, int *gas, uint16_t *remaining)
{
    Sc64ProtossResearchQuote q;
    if (!minerals || !gas || !remaining || *remaining || !ready ||
        !sc64_pr_quote(family,level,&q) || *minerals<q.mineral || *gas<q.gas) return 0;
    *minerals-=q.mineral; *gas-=q.gas; *remaining=q.ticks; return 1;
}

/* The adapter authenticates producer binding before each call. A paused job
 * stays paid and bound; destroyed/canceled behavior is intentionally external. */
static inline int sc64_pr_tick(unsigned family, uint8_t *level,
                               uint16_t *remaining, int active)
{
    Sc64ProtossResearchQuote q;
    if (!level || !remaining || !*remaining || !active ||
        !sc64_pr_quote(family,*level,&q) || *remaining>q.ticks) return 0;
    if (--*remaining) return 0;
    ++*level; return 1;
}
#endif
