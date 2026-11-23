/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PACKED_MINERAL_FIFO_TAGS_H
#define SC64_PACKED_MINERAL_FIFO_TAGS_H

#include <stddef.h>
#include "sc64_save_tag_namespace.h"
#include "sc64_stage_save_auth.h"
#define SC64_FIFO_TRANSPORT_TAG(ns) ((SC64_TRANSPORT_SAVE_VERSION & ~SC64_SAVE_TAG_NAMESPACE_MASK) | (ns))
#define SC64_FIFO_BASE_TAG SC64_FIFO_TRANSPORT_TAG(SC64_SAVE_TAG_PACKED_FIFO_BASE_NAMESPACE)
#if defined(SC64_TERRAN_CARRIER_PROBE)
#define SC64_FIFO_CUSTOM_TAG SC64_FIFO_TRANSPORT_TAG(SC64_SAVE_TAG_PACKED_FIFO_TERRAN_NAMESPACE)
#define SC64_FIFO_CUSTOM_ORIGINAL SC64_TERRAN_CARRIER_TAG
#elif defined(SC64_ZERG_CARRIER_PROBE)
#define SC64_FIFO_CUSTOM_TAG SC64_FIFO_TRANSPORT_TAG(SC64_SAVE_TAG_PACKED_FIFO_ZERG_NAMESPACE)
#define SC64_FIFO_CUSTOM_ORIGINAL SC64_ZERG_CARRIER_TAG
#elif defined(SC64_RESEARCH_SRAM_PRODUCT)
#define SC64_FIFO_CUSTOM_TAG SC64_FIFO_TRANSPORT_TAG(SC64_SAVE_TAG_PACKED_FIFO_RESEARCH_NAMESPACE)
#define SC64_FIFO_CUSTOM_ORIGINAL SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION
#endif
static const u32 race_fifo_tags[] = {
    SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION, SC64_TRANSPORT_SAVE_VERSION,
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    SC64_RESEARCH_TRANSPORT_SAVE_VERSION,
#endif
#ifdef SC64_FIFO_CUSTOM_TAG
    SC64_FIFO_CUSTOM_ORIGINAL, SC64_FIFO_CUSTOM_TAG,
#endif
    SC64_FIFO_BASE_TAG
};
_Static_assert(sizeof(race_fifo_tags) / sizeof(race_fifo_tags[0]) <= GBA_SAVE_MAX_ACCEPTED_TAGS,
               "packed FIFO tag list exceeds selector capacity");

/* Preserve the predecessor payload layout. Only this journal tag selects
 * ranked carry semantics, including in worlds with an empty queue. */
static u32 race_mineral_fifo_write_tag(u32 tag)
{
    if (!mineral_fifo_enabled) return tag;
#ifdef SC64_FIFO_CUSTOM_TAG
    if (tag == SC64_FIFO_CUSTOM_ORIGINAL) return SC64_FIFO_CUSTOM_TAG;
#endif
    return tag == SC64_TRANSPORT_SAVE_VERSION ? SC64_FIFO_BASE_TAG : tag;
}

static int race_mineral_fifo_authenticate(Sc64StageSaveAuth *auth, const RaceSave *candidate,
                                          u32 tag, u32 length)
{
    const Sc64StageTagPair pairs[] = {
        {SC64_FIFO_BASE_TAG, SAVE_VERSION, SC64_STAGE_SAVE_MINERAL_FIFO},
#ifdef SC64_FIFO_CUSTOM_TAG
        {SC64_FIFO_CUSTOM_TAG, SAVE_VERSION, SC64_STAGE_SAVE_MINERAL_FIFO},
#endif
    };
    if (!auth) return 0;
    *auth = (Sc64StageSaveAuth){0};
    if (!candidate || length < offsetof(RaceSave,state) + sizeof(candidate->state[0]) ||
        (unsigned)candidate->state[0] > 1u) return 0;
#ifdef SC64_FIFO_CUSTOM_TAG
    if (candidate->state[0] != (tag == SC64_FIFO_CUSTOM_TAG)) return 0;
#endif
    return sc64_stage_save_authenticate(auth, candidate, tag, candidate->version,
                                       pairs, sizeof(pairs) / sizeof(pairs[0]));
}

#endif
