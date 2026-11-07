/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compile-time scope for the isolated HOST Flash/Q8 experiment. */
#ifndef SC64_Q8_FLASH_SCOPE_H
#define SC64_Q8_FLASH_SCOPE_H
#if defined(SC64_Q8_FLASH_HOST_STAGE) && \
    (!defined(GBA_HOST) || !defined(SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT) || \
     !defined(SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL) || \
     (defined(SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL) && !defined(SC64_CONTROLLER_FLASH_P1_HOST_STAGE)) || \
     !defined(SC64_P7_TYPED_COMPACT_ENABLED) || !defined(SC64_P10_RUNTIME))
#error "Q8 Flash stage requires HOST P10 typed custom jobs and excludes campaign"
#endif
#if defined(SC64_Q8_FLASH_ARM_PROBE) && (defined(GBA_HOST) || defined(SC64_Q8_FLASH_HOST_STAGE) || !defined(SC64_RESEARCH_BOUNDED_ARM_PROBE) || !defined(SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT) || !defined(SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL) || defined(SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL) || !defined(SC64_RACES))
#error "ARM Q8 Flash probe requires shared custom research and excludes HOST and campaign"
#endif
#if defined(SC64_Q8_FLASH_ALL_CUSTOM_HOST) && (!defined(SC64_Q8_FLASH_HOST_STAGE) || !defined(GBA_HOST) || defined(SC64_Q8_FLASH_ARM_PROBE))
#error "All-races Flash admission is a separate HOST-only experiment"
#endif
#if defined(SC64_Q8_FLASH_ALL_CUSTOM_ARM_PROBE) && \
    (!defined(SC64_Q8_FLASH_ARM_PROBE) || defined(GBA_HOST) || \
     defined(SC64_Q8_FLASH_HOST_STAGE) || defined(SC64_Q8_FLASH_ALL_CUSTOM_HOST))
#error "All-races ARM Flash admission requires the explicit ARM transaction probe"
#endif
#if defined(SC64_CONTROLLER_FLASH_P1_HOST_STAGE) && \
    (!defined(GBA_HOST) || !defined(SC64_Q8_FLASH_HOST_STAGE) || \
     !defined(SC64_CAMPAIGN_CONTROLLER_HOST_STAGE) || \
     !defined(SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL) || \
     defined(SC64_SCARAB_NAMESPACE_STAGE))
#error "P1 controller Flash stage requires HOST campaign research and excludes Scarab"
#endif
#endif
