/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CARRIER_RELEASE_H
#define SC64_CARRIER_RELEASE_H

/* Include before profile-sensitive declarations in every translation unit.
 * The release capability only selects existing, mutually exclusive journal
 * adapters. Generation fields, actor IDs and tag definitions stay there. */
#ifdef SC64_CARRIER_RELEASE
#if defined(SC64_TERRAN_CARRIER_PROBE) || defined(SC64_ZERG_CARRIER_PROBE) || \
    defined(SC64_SHOWCASE_CARRIER_PROBE)
#error "Use the Carrier release selector or a direct preview gate, not both"
#endif

#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 0
#define SC64_CARRIER_RELEASE_TERRAN 1
#else
#define SC64_CARRIER_RELEASE_TERRAN 0
#endif
#if defined(SC64_Z10_RUNTIME) && defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 1
#define SC64_CARRIER_RELEASE_Z10 1
#else
#define SC64_CARRIER_RELEASE_Z10 0
#endif
#if defined(SC64_P10_RUNTIME) && defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 2
#define SC64_CARRIER_RELEASE_P10 1
#else
#define SC64_CARRIER_RELEASE_P10 0
#endif

#if SC64_CARRIER_RELEASE_TERRAN + SC64_CARRIER_RELEASE_Z10 + SC64_CARRIER_RELEASE_P10 + \
    defined(SC64_SHOWCASE_ENABLED) != 1
#error "Carrier release requires exactly one supported product profile"
#endif

#if SC64_CARRIER_RELEASE_TERRAN
#define SC64_TERRAN_CARRIER_PROBE 1
#elif SC64_CARRIER_RELEASE_Z10
#define SC64_ZERG_CARRIER_PROBE 1
#elif defined(SC64_SHOWCASE_ENABLED)
#define SC64_SHOWCASE_CARRIER_PROBE 1
#define SC64_P7_RECT_TABLES 1
#endif

/* P10 already uses the native Carrier service and fixed journal. */
#if !SC64_CARRIER_RELEASE_P10
#define SC64_INTERCEPTOR_COMPACT_CODEC 1
#define SC64_P7_PROJECTILES_ENABLED 1
#define SC64_P7_CARRIER_TRAINING_ENABLED 1
#define SC64_P7_INTERCEPTOR_FLIGHT_ENABLED 1
#endif
#undef SC64_CARRIER_RELEASE_TERRAN
#undef SC64_CARRIER_RELEASE_Z10
#undef SC64_CARRIER_RELEASE_P10
#endif
#endif
