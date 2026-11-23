/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MINERAL_FIFO_H
#define SC64_MINERAL_FIFO_H

#include <stdint.h>

/* IDs stored in the arrays are zero for none, otherwise index plus one.
 * Each unit belongs to at most one patch; head and tail are both zero for
 * an empty patch, and the tail's next is zero. All lists are acyclic. */
_Static_assert(MAX_UNITS > 0 && MAX_UNITS <= 255, "unit IDs must fit in a byte");
_Static_assert(MAX_MINERALS > 0 && MAX_MINERALS <= 255, "patch IDs must fit in a byte");

typedef struct {
    uint8_t head[MAX_MINERALS];
    uint8_t tail[MAX_MINERALS];
    uint8_t next[MAX_UNITS];
    uint8_t patch[MAX_UNITS];
} Sc64MineralFifo;

/* The caller has already found this member and its predecessor. */
static inline void
sc64_mineral_fifo_unlink(Sc64MineralFifo *fifo, unsigned patch,
                         unsigned unit, unsigned previous)
{
    uint8_t next = fifo->next[unit];
    if (previous) fifo->next[previous - 1u] = next;
    else fifo->head[patch] = next;
    if (!next) fifo->tail[patch] = (uint8_t)previous;
    fifo->next[unit] = 0;
    fifo->patch[unit] = 0;
}

/* Removing a head is constant time; other removals scan only its patch. */
static inline void
sc64_mineral_fifo_remove(Sc64MineralFifo *fifo, unsigned unit)
{
    unsigned patch, id, previous;
    if (!fifo || unit >= MAX_UNITS || !fifo->patch[unit]) return;
    patch = (unsigned)fifo->patch[unit] - 1u;
    if (patch >= MAX_MINERALS) return;
    id = unit + 1u;
    previous = 0;
    for (unsigned current = fifo->head[patch]; current; current = fifo->next[current - 1u]) {
        if (current == id) {
            sc64_mineral_fifo_unlink(fifo, patch, unit, previous);
            return;
        }
        previous = current;
    }
}

/* Repeated enqueue leaves the worker's position unchanged. A move removes
 * the old membership before appending to the new patch's tail. */
static inline int
sc64_mineral_fifo_append(Sc64MineralFifo *fifo, unsigned patch, unsigned unit)
{
    unsigned id;
    if (!fifo || patch >= MAX_MINERALS || unit >= MAX_UNITS) return 0;
    if (fifo->patch[unit] == patch + 1u) return 1;
    if (fifo->patch[unit]) sc64_mineral_fifo_remove(fifo, unit);
    if (fifo->patch[unit]) return 0;
    id = unit + 1u;
    fifo->next[unit] = 0;
    fifo->patch[unit] = (uint8_t)(patch + 1u);
    if (fifo->tail[patch]) fifo->next[fifo->tail[patch] - 1u] = (uint8_t)id;
    else fifo->head[patch] = (uint8_t)id;
    fifo->tail[patch] = (uint8_t)id;
    return 1;
}

#endif
