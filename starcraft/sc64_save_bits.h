/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SAVE_BITS_H
#define SC64_SAVE_BITS_H

#if defined(SC64_LATE_TERRAN_STORAGE) && \
    !defined(SC64_COMPACT_BUILDING_LEGACY_READER) && \
    !defined(SC64_COMPACT_BUILDING_FIXED_READER)
#define SC64_COMPACT_BUILDING_FIXED_READER 1
#endif

/* Packed save fields are least-significant bit first. Read only the bytes
 * spanned by the field, including fields at the end of an unaligned row. */
static inline
#ifdef SC64_COMPACT_BUILDING_FIXED_READER
__attribute__((always_inline))
#endif
int
sc64_save_read_bits(const u8 *bytes, unsigned row_bits, unsigned *bit,
                    unsigned width, u32 *value)
{
    if (!bytes || !bit || !value || !width || width > 32 ||
        *bit > row_bits || width > row_bits - *bit) return 0;

    unsigned start = *bit;
    unsigned end = start + width;
    unsigned index = start >> 3;
    unsigned last = (end - 1) >> 3;
    unsigned offset = start & 7u;
    u32 result = (u32)bytes[index] >> offset;
    for (unsigned shift = 8u - offset; index < last; shift += 8u)
        result |= (u32)bytes[++index] << shift;
    if (width < 32) result &= ((u32)1u << width) - 1;

    *bit = end;
    *value = result;
    return 1;
}

#endif
