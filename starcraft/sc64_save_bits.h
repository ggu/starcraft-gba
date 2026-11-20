/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SAVE_BITS_H
#define SC64_SAVE_BITS_H

/* Known field positions let the compiler omit unused byte loads. The caller
 * checks the row bounds and that the field spans no more than four bytes. */
static inline __attribute__((always_inline)) u32
sc64_save_read_bits_fixed(const u8 *bytes, unsigned bit, unsigned width)
{
    unsigned index = bit >> 3;
    unsigned shift = bit & 7u;
    u32 value = bytes[index];
    if (shift + width > 8) value |= (u32)bytes[index + 1] << 8;
    if (shift + width > 16) value |= (u32)bytes[index + 2] << 16;
    if (shift + width > 24) value |= (u32)bytes[index + 3] << 24;
    return width == 32 ? value : (value >> shift) & (((u32)1 << width) - 1);
}

/* Packed save fields are least-significant bit first. Read only the bytes
 * spanned by the field, including fields at the end of an unaligned row. */
static inline int
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
