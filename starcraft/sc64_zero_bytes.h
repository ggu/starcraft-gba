/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ZERO_BYTES_H
#define SC64_ZERO_BYTES_H

#include <stddef.h>
#include <stdint.h>

/* GCC must not specialize the alignment test using a typed caller's pointer.
 * Host Clang keeps the same byte-oriented interface for sanitizer checks. */
#if defined(__GNUC__) && !defined(__clang__)
#define SC64_ZERO_BYTES_ATTR __attribute__((noipa))
#elif defined(__clang__)
#define SC64_ZERO_BYTES_ATTR __attribute__((noinline))
#else
#define SC64_ZERO_BYTES_ATTR
#endif

#if defined(__GNUC__) || defined(__clang__)
typedef uint32_t Sc64ZeroAlias32 __attribute__((may_alias));
#endif

/* Test the entire object representation, including padding. NULL is false,
 * even for an empty range. Non-NULL empty ranges are true. No overreads. */
static int SC64_ZERO_BYTES_ATTR
sc64_bytes_are_zero(const void *object, size_t length)
{
    const unsigned char *p = (const unsigned char *)object;
    if (!p) return 0;
#if defined(__GNUC__) || defined(__clang__)
    while (length && ((uintptr_t)p & 3u)) {
        if (*p++) return 0;
        --length;
    }
    while (length >= sizeof(Sc64ZeroAlias32)) {
        if (*(const Sc64ZeroAlias32 *)p) return 0;
        p += sizeof(Sc64ZeroAlias32);
        length -= sizeof(Sc64ZeroAlias32);
    }
#endif
    while (length) {
        if (*p++) return 0;
        --length;
    }
    return 1;
}

#undef SC64_ZERO_BYTES_ATTR
#endif
