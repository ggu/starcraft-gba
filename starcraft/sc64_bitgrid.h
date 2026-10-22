#ifndef SC64_BITGRID_H
#define SC64_BITGRID_H

#include "gba.h"

/* Bounded bit access for byte arrays whose cells are stored least-significant
 * bit first. Callers pass the logical cell count so the final partial byte
 * and invalid indices cannot touch adjacent state. */
static inline unsigned sc64_bitgrid_bytes(int cell_count)
{
    return cell_count <= 0 ? 0u : ((unsigned)cell_count + 7u) / 8u;
}

static inline int sc64_bitgrid_get(const u8 *bits, int cell_count, int cell)
{
    unsigned mask;
    if (!bits || cell < 0 || cell >= cell_count) return 0;
    mask = 1u << ((unsigned)cell & 7u);
    return (bits[(unsigned)cell >> 3] & mask) != 0;
}

/* Returns 1 when the requested cell was in range, 0 otherwise. A nonzero
 * value sets the cell; zero clears only that cell and preserves its peers. */
static inline int sc64_bitgrid_set(u8 *bits, int cell_count, int cell, int value)
{
    unsigned byte_index, mask;
    if (!bits || cell < 0 || cell >= cell_count) return 0;
    byte_index = (unsigned)cell >> 3;
    mask = 1u << ((unsigned)cell & 7u);
    if (value) bits[byte_index] = (u8)(bits[byte_index] | mask);
    else bits[byte_index] = (u8)(bits[byte_index] & ~mask);
    return 1;
}

static inline int sc64_bitgrid_clear_cell(u8 *bits, int cell_count, int cell)
{
    return sc64_bitgrid_set(bits, cell_count, cell, 0);
}

/* Clear exactly ceil(cell_count / 8) bytes. Guard data after the grid is not
 * touched; high unused bits in the final byte are cleared too. */
static inline void sc64_bitgrid_clear(u8 *bits, int cell_count)
{
    unsigned i, bytes;
    if (!bits) return;
    bytes = sc64_bitgrid_bytes(cell_count);
    for (i = 0; i < bytes; ++i) bits[i] = 0;
}

/* Convert byte-per-cell truth values to/from packed storage. Both routines
 * touch only the logical range and leave caller guard bytes intact. */
static inline void sc64_bitgrid_pack(u8 *packed, const u8 *cells, int cell_count)
{
    int i;
    if (!packed || !cells || cell_count <= 0) return;
    sc64_bitgrid_clear(packed, cell_count);
    for (i = 0; i < cell_count; ++i)
        if (cells[i]) (void)sc64_bitgrid_set(packed, cell_count, i, 1);
}

static inline void sc64_bitgrid_unpack(u8 *cells, const u8 *packed, int cell_count)
{
    int i;
    if (!packed || !cells || cell_count <= 0) return;
    for (i = 0; i < cell_count; ++i)
        cells[i] = (u8)sc64_bitgrid_get(packed, cell_count, i);
}

#endif
