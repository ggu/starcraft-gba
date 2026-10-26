/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FACT_TABLE_FORMAT_H
#define SC64_FACT_TABLE_FORMAT_H

/* Packed per-type fact words shared by tools/sc64_fact_tables_gen.c and the
 * SC64_FACT_TABLES lookups. A unit word holds the raw game actor flags (zero
 * still means "no authored policy"), the collision radius and flight; a
 * building half-word holds the raw building actor flags. */
#define SC64_FACT_UNIT_FLAGS_MASK 0xffffu
#define SC64_FACT_UNIT_RADIUS_SHIFT 16
#define SC64_FACT_UNIT_RADIUS_MASK 0x3fu
#define SC64_FACT_UNIT_FLYING (1u << 22)
#define SC64_FACT_BUILDING_FLAGS_MASK 0xffffu

#endif
