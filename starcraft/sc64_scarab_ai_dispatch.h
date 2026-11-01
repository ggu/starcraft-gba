/* SPDX-License-Identifier: GPL-3.0-or-later
 * Staged, approximate owner-decision clock. No runtime hook or wire format.
 * A caller supplies a stable, authoritative owner roster for the whole world;
 * an absent owner has a zero record. Slots must never be shared by two source
 * owners, even when they map to the same live engine owner. */
#ifndef SC64_SCARAB_AI_DISPATCH_H
#define SC64_SCARAB_AI_DISPATCH_H
#include <stdint.h>

enum { SC64_SCARAB_AI_OWNER_LIMIT = 8, SC64_SCARAB_AI_PERIOD = 120 };
typedef struct {
    uint8_t active, live_owner, source_owner, remaining;
} Sc64ScarabAiDecision;
typedef struct { Sc64ScarabAiDecision owner[SC64_SCARAB_AI_OWNER_LIMIT]; } Sc64ScarabAiClock;
typedef void (*Sc64ScarabAiDecisionFn)(void *context, unsigned slot,
                                       unsigned live_owner, unsigned source_owner);

/* Validate before loading or ticking; unsupported owners must never be
 * registered by the profile-specific authority. This is not an SRAM codec. */
static inline int sc64_scarab_ai_clock_valid(const Sc64ScarabAiClock *clock)
{
    if (!clock) return 0;
    for (unsigned i=0;i<SC64_SCARAB_AI_OWNER_LIMIT;++i) {
        const Sc64ScarabAiDecision *p=&clock->owner[i];
        if (p->active>1 || (p->active && (!p->remaining || p->remaining>SC64_SCARAB_AI_PERIOD))) return 0;
        if (!p->active && (p->live_owner || p->source_owner || p->remaining)) return 0;
        if (p->active) for (unsigned j=0;j<i;++j)
            if (clock->owner[j].active && clock->owner[j].live_owner==p->live_owner &&
                clock->owner[j].source_owner==p->source_owner) return 0;
    }
    return 1;
}

/* Paused worlds do not consume decision time. One decision per due owner;
 * the callback owns parent scanning, payment and source-identity validation. */
static inline int sc64_scarab_ai_clock_tick(Sc64ScarabAiClock *clock, int paused,
                                            Sc64ScarabAiDecisionFn decision, void *context)
{
    if (!sc64_scarab_ai_clock_valid(clock) || (paused!=0 && paused!=1) || !decision) return 0;
    if (paused) return 1;
    for (unsigned i=0;i<SC64_SCARAB_AI_OWNER_LIMIT;++i) {
        Sc64ScarabAiDecision *p=&clock->owner[i];
        if (!p->active) continue;
        if (--p->remaining == 0) {
            p->remaining=SC64_SCARAB_AI_PERIOD;
            decision(context,i,p->live_owner,p->source_owner);
        }
    }
    return 1;
}
#endif
