#ifndef SC64_UNIT_INDEX_H
#define SC64_UNIT_INDEX_H

/* Stored unit slot numbers.
 *
 * Tables that link or list unit slots (collision buckets, per-owner target
 * lists, effect slot lists) store a slot number and a "none" sentinel. While
 * the unit table has fewer than 255 slots, a byte holds every slot plus the
 * historical 0xff sentinel, so those profiles keep their byte layout, RAM and
 * generated code exactly. A larger table switches the same storage to 16-bit
 * numbers with 0xffff as the sentinel.
 *
 * Code that walks such a list compares against SC64_UNIT_IX_NONE, never a
 * literal 0xff, so both widths read the same. memset(..., 0xff, ...) still
 * fills either width with the sentinel. */
#if !defined(MAX_UNITS) || MAX_UNITS < 255
/* Standalone fixtures without a unit table use the byte layout. */
typedef u8 Sc64UnitIx;
#define SC64_UNIT_IX_NONE 0xff
#else
typedef u16 Sc64UnitIx;
#define SC64_UNIT_IX_NONE 0xffff
#endif
#ifdef MAX_UNITS
#define SC64_UNIT_IX_WIDE (MAX_UNITS >= 255)
_Static_assert(MAX_UNITS > 0 && MAX_UNITS < SC64_UNIT_IX_NONE,
               "unit slot numbers must fit below the stored sentinel");
#else
#define SC64_UNIT_IX_WIDE 0
#endif
_Static_assert(sizeof(Sc64UnitIx) == (SC64_UNIT_IX_WIDE ? 2u : 1u),
               "unit slot storage width follows the table size");

#endif
