/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GAS_SERVICE_SAVE_H
#define SC64_GAS_SERVICE_SAVE_H

/* Detached queue codec for an already authenticated journal candidate.
 * The marker belongs to state[16] (see sc64_source_gas_save.h), whose value
 * is covered by the journal checksum. Callers MUST operate on disposable
 * candidate storage: a failed write callback can leave that scratch altered.
 * Never publish the candidate or queue until all checks and writes succeed.
 * Legacy, unmarked records must bypass this codec entirely. */
#include "sc64_gas_service_queue.h"

enum { SC64_GAS_QUEUE_CARRY_BITS = 4, SC64_GAS_QUEUE_CARRY_MAX = 8,
       SC64_GAS_QUEUE_MAX_TIMER = 60 };

typedef struct {
    int alive, worker, gas_order, target, carry, timer;
} Sc64GasQueueSaveRow;
typedef int (*Sc64GasQueueReadRow)(void *, unsigned, Sc64GasQueueSaveRow *);
typedef int (*Sc64GasQueueWriteCarry)(void *, unsigned, int);
typedef int (*Sc64GasQueueEligible)(void *, unsigned, unsigned, const Sc64GasQueueSaveRow *);

/* Verify both directions of the live list without trusting its links. */
static inline int
sc64_gas_queue_save_ranks(const Sc64GasServiceQueue *queue, uint8_t *sites, uint8_t *ranks)
{
    uint8_t seen[MAX_UNITS] = {0};
    if (!queue || !sites || !ranks) return 0;
    memset(sites, 0, MAX_UNITS); memset(ranks, 0, MAX_UNITS);
    for (unsigned site = 0; site < MAX_BUILDINGS; ++site) {
        unsigned rank = 0, last = 0;
        for (unsigned link = queue->head[site]; link;) {
            if (link > MAX_UNITS || seen[link - 1u] || ++rank > MAX_UNITS) return 0;
            unsigned slot = link - 1u;
            if (queue->site[slot] != site + 1u) return 0;
            seen[slot] = 1; sites[slot] = (uint8_t)(site + 1u); ranks[slot] = (uint8_t)rank;
            last = link; link = queue->next[slot];
        }
        if (queue->tail[site] != last) return 0;
    }
    for (unsigned i = 0; i < MAX_UNITS; ++i)
        if (!seen[i] && (queue->site[i] || queue->next[i])) return 0;
    return 1;
}

/* Keep rank scratch off the journal-selection stack while other campaign
 * validators run. The cartridge stack guard checks both call paths. */
static __attribute__((noinline)) int
sc64_gas_queue_save_encode(void *candidate, const Sc64GasServiceQueue *queue,
                           Sc64GasQueueReadRow read, Sc64GasQueueWriteCarry write,
                           Sc64GasQueueEligible eligible)
{
    uint8_t sites[MAX_UNITS], ranks[MAX_UNITS];
    if (!candidate || !read || !write || !eligible ||
        !sc64_gas_queue_save_ranks(queue, sites, ranks)) return 0;
    /* First pass validates every row before any scratch write. */
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Sc64GasQueueSaveRow row;
        if (!read(candidate, i, &row)) return 0;
        if (!ranks[i]) continue;
        unsigned site = sites[i] - 1u;
        if (!row.alive || !row.worker || !row.gas_order || row.target != (int)site ||
            row.carry < 0 || row.carry > SC64_GAS_QUEUE_CARRY_MAX ||
            row.timer < 0 || row.timer > SC64_GAS_QUEUE_MAX_TIMER ||
            !eligible(candidate, i, site, &row)) return 0;
    }
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Sc64GasQueueSaveRow row;
        if (!ranks[i]) continue;
        if (!read(candidate, i, &row) ||
            !write(candidate, i, ((int)ranks[i] << SC64_GAS_QUEUE_CARRY_BITS) | row.carry)) return 0;
    }
    return 1;
}

/* Authenticate the marker and payload BEFORE calling. Scratch queue and rows
 * remain detached through all checks; the output queue is assigned last. */
static __attribute__((noinline)) int
sc64_gas_queue_save_decode(void *candidate, Sc64GasServiceQueue *out,
                           Sc64GasQueueReadRow read, Sc64GasQueueWriteCarry write,
                           Sc64GasQueueEligible eligible)
{
    uint8_t sites[MAX_UNITS] = {0}, ranks[MAX_UNITS] = {0};
    uint8_t counts[MAX_BUILDINGS] = {0}, starts[MAX_BUILDINGS], ordered[MAX_UNITS] = {0};
    unsigned total = 0;
    if (!candidate || !out || !read || !write || !eligible) return 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Sc64GasQueueSaveRow row;
        if (!read(candidate, i, &row)) return 0;
        if (!row.alive || !row.worker || !row.gas_order) continue;
        if (row.carry < 0) return 0;
        if (row.carry <= SC64_GAS_QUEUE_CARRY_MAX) continue;
        unsigned rank = (unsigned)row.carry >> SC64_GAS_QUEUE_CARRY_BITS;
        unsigned cargo = (unsigned)row.carry & ((1u << SC64_GAS_QUEUE_CARRY_BITS) - 1u);
        if (!rank || rank > MAX_UNITS || cargo > SC64_GAS_QUEUE_CARRY_MAX ||
            row.target < 0 || row.target >= MAX_BUILDINGS ||
            row.timer < 0 || row.timer > SC64_GAS_QUEUE_MAX_TIMER) return 0;
        row.carry = (int)cargo;
        if (!eligible(candidate, i, (unsigned)row.target, &row)) return 0;
        sites[i] = (uint8_t)(row.target + 1); ranks[i] = (uint8_t)rank;
        ++counts[row.target];
    }
    for (unsigned site = 0; site < MAX_BUILDINGS; ++site) {
        starts[site] = (uint8_t)total;
        total += counts[site];
    }
    if (total > MAX_UNITS) return 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        if (!ranks[i]) continue;
        unsigned site = sites[i] - 1u;
        if (ranks[i] > counts[site]) return 0;
        unsigned pos = starts[site] + ranks[i] - 1u;
        if (pos >= total || ordered[pos]) return 0;
        ordered[pos] = (uint8_t)(i + 1u);
    }
    for (unsigned i = 0; i < total; ++i) if (!ordered[i]) return 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Sc64GasQueueSaveRow row;
        if (ranks[i] && (!read(candidate, i, &row) ||
            !write(candidate, i, row.carry & ((1 << SC64_GAS_QUEUE_CARRY_BITS) - 1)))) return 0;
    }
    /* No failure is possible after this point, so out changes atomically. */
    memset(out, 0, sizeof(*out));
    for (unsigned site = 0; site < MAX_BUILDINGS; ++site) {
        unsigned start = starts[site], count = counts[site];
        if (!count) continue;
        out->head[site] = ordered[start]; out->tail[site] = ordered[start + count - 1u];
        for (unsigned at = 0; at < count; ++at) {
            unsigned slot = ordered[start + at] - 1u;
            out->site[slot] = (uint8_t)(site + 1u);
            out->next[slot] = at + 1u < count ? ordered[start + at + 1u] : 0;
        }
    }
    return 1;
}
#endif
