/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GAS_SERVICE_QUEUE_H
#define SC64_GAS_SERVICE_QUEUE_H
#include <stdint.h>
/* An isolated FIFO primitive for the gas-service experiment. The caller owns
 * eligibility, timers and journal authentication. IDs are one-based. */
_Static_assert(MAX_UNITS > 0 && MAX_UNITS <= 255, "unit ID must fit");
_Static_assert(MAX_BUILDINGS > 0 && MAX_BUILDINGS <= 255, "building ID must fit");
typedef struct {
    uint8_t head[MAX_BUILDINGS], tail[MAX_BUILDINGS];
    uint8_t next[MAX_UNITS], site[MAX_UNITS];
} Sc64GasServiceQueue;
static inline void sc64_gas_service_remove(Sc64GasServiceQueue *q, unsigned unit)
{
    if (unit >= MAX_UNITS || !q->site[unit]) return;
    unsigned site = q->site[unit] - 1u, prev = 0;
    for (unsigned n = q->head[site]; n; n = q->next[n - 1u]) {
        unsigned at = n - 1u;
        if (at == unit) {
            if (prev) q->next[prev - 1u] = q->next[at];
            else q->head[site] = q->next[at];
            if (q->tail[site] == n) q->tail[site] = (uint8_t)prev;
            q->next[at] = q->site[at] = 0;
            return;
        }
        prev = n;
    }
}
static inline int sc64_gas_service_join(Sc64GasServiceQueue *q, unsigned site, unsigned unit)
{
    if (!q || site >= MAX_BUILDINGS || unit >= MAX_UNITS) return 0;
    if (q->site[unit] == site + 1u) return 1;
    sc64_gas_service_remove(q, unit);
    if (q->site[unit]) return 0;
    unsigned id = unit + 1u;
    if (q->tail[site]) q->next[q->tail[site] - 1u] = (uint8_t)id;
    else q->head[site] = (uint8_t)id;
    q->tail[site] = (uint8_t)id;
    q->site[unit] = (uint8_t)(site + 1u);
    q->next[unit] = 0;
    return 1;
}
/* The experiment's detached rank snapshot is not a cartridge save format.
 * Restore checks duplicates, holes and bounds before publishing any state. */
static inline void sc64_gas_service_ranks(const Sc64GasServiceQueue *q, uint8_t ranks[MAX_UNITS])
{
    memset(ranks, 0, MAX_UNITS);
    for (unsigned s = 0; s < MAX_BUILDINGS; ++s) {
        unsigned rank = 0;
        for (unsigned n = q->head[s]; n; n = q->next[n - 1u]) ranks[n - 1u] = (uint8_t)++rank;
    }
}
static inline int sc64_gas_service_restore(Sc64GasServiceQueue *out,
                                            const uint8_t sites[MAX_UNITS],
                                            const uint8_t ranks[MAX_UNITS])
{
    Sc64GasServiceQueue candidate = {0};
    uint8_t ordered[MAX_UNITS];
    for (unsigned s = 0; s < MAX_BUILDINGS; ++s) {
        unsigned count = 0;
        memset(ordered, 0, sizeof(ordered));
        for (unsigned u = 0; u < MAX_UNITS; ++u) {
            if (sites[u] > MAX_BUILDINGS || (!!sites[u] != !!ranks[u])) return 0;
            if (sites[u] != s + 1u) continue;
            unsigned r = ranks[u];
            if (r > MAX_UNITS || ordered[r - 1u]) return 0;
            ordered[r - 1u] = (uint8_t)(u + 1u);
            ++count;
        }
        for (unsigned r = 0; r < count; ++r) {
            if (!ordered[r] || !sc64_gas_service_join(&candidate, s, ordered[r] - 1u)) return 0;
        }
    }
    *out = candidate;
    return 1;
}
#endif
