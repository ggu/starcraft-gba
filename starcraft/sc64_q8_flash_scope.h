/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compile-time scope for the isolated HOST Flash/Q8 experiment. */
#ifndef SC64_Q8_FLASH_SCOPE_H
#define SC64_Q8_FLASH_SCOPE_H
#if defined(SC64_Q8_FLASH_HOST_STAGE) && \
    (!defined(GBA_HOST) || !defined(SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT) || \
     !defined(SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL) || \
     defined(SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL) || \
     !defined(SC64_P7_TYPED_COMPACT_ENABLED) || !defined(SC64_P10_RUNTIME))
#error "Q8 Flash stage requires HOST P10 typed custom jobs and excludes campaign"
#endif
#if defined(SC64_Q8_FLASH_ARM_PROBE) && (defined(GBA_HOST) || defined(SC64_Q8_FLASH_HOST_STAGE) || !defined(SC64_RESEARCH_BOUNDED_ARM_PROBE) || !defined(SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT) || !defined(SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL) || defined(SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL) || !defined(SC64_P7_TYPED_COMPACT_ENABLED) || !defined(SC64_P10_RUNTIME))
#error "ARM Q8 Flash probe requires P10 typed custom research and excludes HOST and campaign"
#endif
#endif
