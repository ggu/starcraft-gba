/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include "../common/gba.h"

typedef struct { u32 version; unsigned char padding[128]; int state[2]; } RaceSave;
#define SAVE_VERSION 0x12430001u
#define LEGACY_INNER 0x22430005u
#define SC64_TRANSPORT_SAVE_VERSION 0x13430001u
#define SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION 0x14430001u
#define SC64_TERRAN_CARRIER_TAG 0x134d0001u
#define SC64_ZERG_CARRIER_TAG 0x134e0001u
#define SC64_RESEARCH_TRANSPORT_SAVE_VERSION 0x13440001u
#define SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION 0x13450001u
static int mineral_fifo_enabled;
#include "../starcraft/sc64_save_tag_namespace.h"
#include "../starcraft/sc64_packed_mineral_fifo_tags.h"

static int prepare(const void *record, u32 length, u32 tag, void *context)
{
    Sc64StageSaveAuth *auth = context;
    *auth = (Sc64StageSaveAuth){0};
    if (length != sizeof(RaceSave)) return 0;
    const RaceSave *candidate = record;
    if (tag == SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION)
        return candidate->version == LEGACY_INNER;
    return race_mineral_fifo_authenticate(auth, candidate, tag, length);
}

static void check_journal(void)
{
    const unsigned count = sizeof(race_fifo_tags) / sizeof(race_fifo_tags[0]);
    const u32 legacy_tag = SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION;
    RaceSave legacy = {.version = LEGACY_INNER};
    RaceSave packed = {.version = SAVE_VERSION};
    RaceSave scratch;
    Sc64StageSaveAuth auth;
    GbaSaveSelection previous, selected;
    assert(save_write_first(&legacy, sizeof(legacy), legacy_tag));
    assert(save_select_prepared(&scratch, sizeof(scratch), race_fifo_tags,
                                count, prepare, &auth, &previous));
    assert(previous.tag == legacy_tag && !auth.features);
#ifdef SC64_FIFO_CUSTOM_TAG
    packed.state[0] = 1;
    const u32 fifo_tag = SC64_FIFO_CUSTOM_TAG;
#else
    const u32 fifo_tag = SC64_FIFO_BASE_TAG;
#endif
    /* A checksum-valid FIFO tag paired with the wrong inner version must
     * preserve the predecessor, including with six accepted P10 tags. */
    packed.version = LEGACY_INNER;
    assert(save_write_selected(&packed, sizeof(packed), fifo_tag, &previous,
                               race_fifo_tags, count));
    assert(save_select_prepared(&scratch, sizeof(scratch), race_fifo_tags,
                                count, prepare, &auth, &selected));
    assert(selected.tag == legacy_tag && selected.slot == previous.slot && !auth.features);

    packed.version = SAVE_VERSION;
    assert(save_write_selected(&packed, sizeof(packed), fifo_tag, &previous,
                               race_fifo_tags, count));
    assert(save_select_prepared(&scratch, sizeof(scratch), race_fifo_tags,
                                count, prepare, &auth, &selected));
    assert(selected.tag == fifo_tag && selected.slot != previous.slot);
    assert(sc64_stage_save_auth_matches(&auth, &scratch, SAVE_VERSION, SC64_STAGE_SAVE_MINERAL_FIFO));
}

int main(void)
{
    RaceSave candidate = {.version = SAVE_VERSION};
    Sc64StageSaveAuth auth;
    const unsigned count = sizeof(race_fifo_tags) / sizeof(race_fifo_tags[0]);
    for (unsigned i = 0; i < count; ++i)
        for (unsigned j = 0; j < i; ++j) assert(race_fifo_tags[i] != race_fifo_tags[j]);
    assert((SC64_FIFO_BASE_TAG & SC64_SAVE_TAG_NAMESPACE_MASK) == SC64_SAVE_TAG_PACKED_FIFO_BASE_NAMESPACE);
    mineral_fifo_enabled = 0;
    for (unsigned i = 0; i < count; ++i)
        assert(race_mineral_fifo_write_tag(race_fifo_tags[i]) == race_fifo_tags[i]);
    mineral_fifo_enabled = 1;
    assert(race_mineral_fifo_write_tag(SC64_TRANSPORT_SAVE_VERSION) == SC64_FIFO_BASE_TAG);
    assert(race_mineral_fifo_write_tag(SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION) == SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION);
    assert(race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_BASE_TAG, sizeof(candidate)));
    assert(sc64_stage_save_auth_matches(&auth, &candidate, SAVE_VERSION, SC64_STAGE_SAVE_MINERAL_FIFO));
    assert(!race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_BASE_TAG, sizeof(candidate.version)-1));
    assert(!sc64_stage_save_auth_matches(&auth, &candidate, SAVE_VERSION, SC64_STAGE_SAVE_MINERAL_FIFO));
    assert(!race_mineral_fifo_authenticate(&auth, &candidate, SC64_TRANSPORT_SAVE_VERSION, sizeof(candidate)));
    candidate.version++;
    assert(!race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_BASE_TAG, sizeof(candidate)));
    candidate.version = SAVE_VERSION;
#ifdef SC64_FIFO_CUSTOM_TAG
#if defined(SC64_TERRAN_CARRIER_PROBE)
    assert((SC64_FIFO_CUSTOM_TAG & SC64_SAVE_TAG_NAMESPACE_MASK) == SC64_SAVE_TAG_PACKED_FIFO_TERRAN_NAMESPACE);
#elif defined(SC64_ZERG_CARRIER_PROBE)
    assert((SC64_FIFO_CUSTOM_TAG & SC64_SAVE_TAG_NAMESPACE_MASK) == SC64_SAVE_TAG_PACKED_FIFO_ZERG_NAMESPACE);
#else
    assert((SC64_FIFO_CUSTOM_TAG & SC64_SAVE_TAG_NAMESPACE_MASK) == SC64_SAVE_TAG_PACKED_FIFO_RESEARCH_NAMESPACE);
#endif
    assert(race_mineral_fifo_write_tag(SC64_FIFO_CUSTOM_ORIGINAL) == SC64_FIFO_CUSTOM_TAG);
    assert(!race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_CUSTOM_TAG, sizeof(candidate)));
    assert(!race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_BASE_TAG, offsetof(RaceSave, state)));
    candidate.state[0] = 1;
    assert(race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_CUSTOM_TAG, sizeof(candidate)));
    assert(!race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_BASE_TAG, sizeof(candidate)));
    candidate.state[0] = 2;
    assert(!race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_CUSTOM_TAG, sizeof(candidate)));
#else
    candidate.state[0] = 1;
    assert(race_mineral_fifo_authenticate(&auth, &candidate, SC64_FIFO_BASE_TAG, sizeof(candidate)));
#endif
    check_journal();
    printf("packed mineral FIFO tag policy: %u tags and SRAM fallback ok\n", count);
    return 0;
}
