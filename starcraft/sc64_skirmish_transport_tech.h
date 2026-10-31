/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SKIRMISH_TRANSPORT_TECH_H
#define SC64_SKIRMISH_TRANSPORT_TECH_H

#include <stdint.h>
#include <limits.h>

/* Ventral Sacs' pinned default price and 24 Hz duration, scaled to the
 * engine's 30 Hz tick. The campaign has a separate source-bound state. */
enum { SC64_SK_TRANSPORT_OWNERS = 4,
       SC64_SK_TRANSPORT_RESEARCH_TICKS = 3000,
       SC64_SK_TRANSPORT_MINERALS = 200,
       SC64_SK_TRANSPORT_GAS = 200 };
typedef struct {
    uint16_t remaining;
    uint8_t producer_plus_one;
    uint8_t researched;
} Sc64SkTransportJob;
typedef struct { Sc64SkTransportJob owner[SC64_SK_TRANSPORT_OWNERS]; } Sc64SkTransportTech;
_Static_assert(sizeof(Sc64SkTransportJob) == 4, "transport job wire budget");
_Static_assert(sizeof(Sc64SkTransportTech) == 16, "transport tech wire budget");

/* Custom-game owner IDs deliberately omit the neutral slot. */
static inline int sc64_sk_transport_owner_index(int owner)
{
    switch (owner) { case 0: return 0; case 1: return 1;
                     case 3: return 2; case 4: return 3; default: return -1; }
}

static inline int sc64_sk_transport_job_valid(const Sc64SkTransportJob *j,
                                               unsigned building_count)
{
    if (!j || building_count > 255u || j->researched > 1u ||
        j->remaining > SC64_SK_TRANSPORT_RESEARCH_TICKS ||
        j->producer_plus_one > building_count) return 0;
    if (!!j->remaining != !!j->producer_plus_one) return 0;
    return !j->researched || !j->remaining;
}

/* Explicit little-endian bytes avoid relying on compiler structure layout. */
static inline void sc64_sk_transport_encode(uint8_t out[16], const Sc64SkTransportTech *t)
{
    for (unsigned i = 0; i < SC64_SK_TRANSPORT_OWNERS; ++i) {
        const Sc64SkTransportJob *j = &t->owner[i];
        out[4*i] = (uint8_t)j->remaining;
        out[4*i+1] = (uint8_t)(j->remaining >> 8);
        out[4*i+2] = j->producer_plus_one;
        out[4*i+3] = j->researched;
    }
}

static inline int sc64_sk_transport_decode(Sc64SkTransportTech *out,
                                             const uint8_t in[16],
                                             unsigned building_count)
{
    Sc64SkTransportTech decoded;
    if (!out || !in) return 0;
    for (unsigned i = 0; i < SC64_SK_TRANSPORT_OWNERS; ++i) {
        Sc64SkTransportJob *j = &decoded.owner[i];
        j->remaining = (uint16_t)((unsigned)in[4*i] | (unsigned)in[4*i+1] << 8);
        j->producer_plus_one = in[4*i+2];
        j->researched = in[4*i+3];
        if (!sc64_sk_transport_job_valid(j, building_count)) return 0;
        for (unsigned k = 0; k < i; ++k)
            if (j->producer_plus_one &&
                j->producer_plus_one == decoded.owner[k].producer_plus_one) return 0;
    }
    *out = decoded;
    return 1;
}

/* The caller verifies a live, completed, idle Lair/Hive owned by this owner,
 * and prevents two jobs binding the same building. All payment is atomic. */
static inline int sc64_sk_transport_start(Sc64SkTransportJob *j,
    unsigned producer, unsigned building_count, int *minerals, int *gas)
{
    if (!sc64_sk_transport_job_valid(j, building_count) || !minerals || !gas ||
        producer >= building_count || j->researched || j->remaining ||
        *minerals < SC64_SK_TRANSPORT_MINERALS || *gas < SC64_SK_TRANSPORT_GAS)
        return 0;
    *minerals -= SC64_SK_TRANSPORT_MINERALS;
    *gas -= SC64_SK_TRANSPORT_GAS;
    j->remaining = SC64_SK_TRANSPORT_RESEARCH_TICKS;
    j->producer_plus_one = (uint8_t)(producer + 1u);
    return 1;
}

/* Cancellation refunds fully; destruction refunds 75%. The caller invokes
 * this once when the exact producer is canceled or removed. */
static inline int sc64_sk_transport_cancel(Sc64SkTransportJob *j, int destroyed,
                                            int *minerals, int *gas)
{
    int refund;
    if (!j || !j->remaining || !minerals || !gas ||
        (destroyed != 0 && destroyed != 1)) return 0;
    refund = destroyed ? 150 : 200;
    if (*minerals < 0 || *gas < 0 || *minerals > INT_MAX - refund ||
        *gas > INT_MAX - refund) return 0;
    *minerals += refund;
    *gas += refund;
    j->remaining = 0;
    j->producer_plus_one = 0;
    return 1;
}

/* The caller first verifies the bound producer remains valid. */
static inline void sc64_sk_transport_tick(Sc64SkTransportJob *j)
{
    if (j && j->remaining && !--j->remaining) {
        j->producer_plus_one = 0;
        j->researched = 1;
    }
}
#endif
