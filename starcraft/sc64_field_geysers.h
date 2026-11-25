/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FIELD_GEYSERS_H
#define SC64_FIELD_GEYSERS_H

/* Geyser rows stored by campaign journals describe neutral map objects.
 * Placement and rendering also use sites under preplaced gas buildings. */
#ifdef SC64_SOURCE_GAS
enum { SC64_FIELD_GEYSER_LIMIT = 2 * MAX_RACE_GEYSERS };
typedef struct {
    u16 x, y, initial;
} Sc64GasSite;
static Sc64GasSite sc64_gas_sites[SC64_FIELD_GEYSER_LIMIT];
static int sc64_gas_site_count;
#define field_geysers sc64_gas_sites
#define field_geyser_count sc64_gas_site_count
#else
#define SC64_FIELD_GEYSER_LIMIT MAX_RACE_GEYSERS
#define field_geysers geysers
#define field_geyser_count geyser_count
#endif
#endif
