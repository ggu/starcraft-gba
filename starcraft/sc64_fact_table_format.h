/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_FACT_TABLE_FORMAT_H
#define SC64_FACT_TABLE_FORMAT_H

#if (defined(SC64_FACT_SIGHT_TABLES) || defined(SC64_P7_RECT_TABLES)) && \
    !defined(SC64_FACT_TABLES) && !defined(SC64_FACT_TABLE_GENERATOR)
#error "FACT feature tables require SC64_FACT_TABLES"
#endif
#if defined(SC64_P7_RECT_TABLES) && !defined(SC64_P7_ACTORS)
#error "SC64_P7_RECT_TABLES requires SC64_P7_ACTORS"
#endif

/* V1 is the legacy actor/collision table. V2 adds feature-gated payloads. */
#define SC64_FACT_TABLE_FORMAT_V1 1u
#define SC64_FACT_TABLE_FORMAT_V2 2u
#define SC64_FACT_FEATURE_SIGHT 1u
#define SC64_FACT_FEATURE_P7_RECT 2u
#define SC64_FACT_UNIT_FLAGS_MASK 0xffffu
#define SC64_FACT_UNIT_RADIUS_SHIFT 16
#define SC64_FACT_UNIT_RADIUS_MASK 0x3fu
#define SC64_FACT_UNIT_FLYING (1u << 22)
#define SC64_FACT_UNIT_SIGHT_SHIFT 23
#define SC64_FACT_UNIT_SIGHT_MASK 0xffu
#define SC64_FACT_RECT_VALID (1u << 28)
#define SC64_FACT_RECT_FIELD_MASK 0x7fu
#define SC64_FACT_RECT_UP_SHIFT 7
#define SC64_FACT_RECT_RIGHT_SHIFT 14
#define SC64_FACT_RECT_DOWN_SHIFT 21
#define SC64_FACT_RECT_RAW_COUNT 228u
#define SC64_FACT_BUILDING_FLAGS_MASK 0xffffu

#endif
