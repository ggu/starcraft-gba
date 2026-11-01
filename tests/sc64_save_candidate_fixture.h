/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Preserve the pending-impact portion of a saved snapshot when injecting a
 * candidate. The production writer constructs the current record envelope;
 * its reader must accept the envelope and return the exact candidate. */
#ifndef SC64_SAVE_CANDIDATE_FIXTURE_H
#define SC64_SAVE_CANDIDATE_FIXTURE_H
#ifdef SC64_IMPACT_EVENTS
static Sc64ImpactEvent fixture_saved_impacts[RACE_IMPACT_CAPACITY];
static u8 fixture_saved_impact_count;
#endif
static void fixture_capture_save(RaceSave *candidate)
{
#ifdef SC64_IMPACT_EVENTS
    int count = -1;
    assert(race_impact_save_read(fixture_saved_impacts, &count));
    assert(count >= 0 && count <= RACE_IMPACT_CAPACITY);
    fixture_saved_impact_count = (u8)count;
#endif
    *candidate = race_data;
}
static void write_candidate_record(const RaceSave *candidate)
{
    static RaceSave expected;
    expected = *candidate; /* Keep an independent copy even if candidate aliases scratch. */
#ifdef SC64_IMPACT_EVENTS
    Sc64ImpactEvent live[RACE_IMPACT_CAPACITY], decoded[RACE_IMPACT_CAPACITY];
    u8 live_count = race_impact_count;
    int count = -1;
    memcpy(live, race_impact_rows, sizeof(live));
    memcpy(race_impact_rows, fixture_saved_impacts, sizeof(fixture_saved_impacts));
    race_impact_count = fixture_saved_impact_count;
#ifdef SC64_WORKER_CARGO_KIND
    /* Isolate this intentionally malformed candidate: an older valid record
     * would make the journal correctly fall back and mask rejection. */
#ifdef SC64_SHARED_TRANSPORT
    const u32 tags[] = { SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,
                         SC64_TRANSPORT_SAVE_VERSION };
#elif defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT)
    const u32 tags[] = { SC64_LEGACY_SAVE_VERSION,
                         SC64_WORKER_CARGO_LEGACY_SAVE_VERSION, SAVE_VERSION };
#else
    const u32 tags[] = { SC64_WORKER_CARGO_LEGACY_SAVE_VERSION, SAVE_VERSION };
#endif
    for (unsigned i = 0; i < sizeof(tags) / sizeof(tags[0]); ++i) {
        save_test_corrupt_latest(0, tags[i]);
        save_test_corrupt_latest(0, tags[i]);
    }
    race_data = expected;
    assert(race_impact_save_write(NULL));
#else
    race_data = expected;
    assert(race_impact_save_write());
#endif
    memcpy(race_impact_rows, live, sizeof(live));
    race_impact_count = live_count;
    assert(race_impact_save_read(decoded, &count));
    assert(count == fixture_saved_impact_count);
    assert(memcmp(decoded, fixture_saved_impacts, (size_t)count * sizeof(*decoded)) == 0);
    assert(memcmp(&race_data, &expected, sizeof(expected)) == 0);
#else
    static RaceSave readback;
#ifdef SC64_SHARED_TRANSPORT
    u8 trailer[SC64_TRANSPORT_TRAILER_BYTES], read_trailer[SC64_TRANSPORT_TRAILER_BYTES];
    sc64_sk_transport_encode(trailer, &race_skirmish_transport_candidate);
    assert(save_write_parts(&expected, sizeof(expected), trailer, sizeof(trailer),
                            SC64_TRANSPORT_SAVE_VERSION));
    assert(save_read_parts(&readback, sizeof(readback), read_trailer, sizeof(read_trailer),
                           SC64_TRANSPORT_SAVE_VERSION));
    assert(memcmp(trailer, read_trailer, sizeof(trailer)) == 0);
#else
    save_write(&expected, sizeof(expected), SAVE_VERSION);
    assert(save_read(&readback, sizeof(readback), SAVE_VERSION));
#endif
    assert(memcmp(&readback, &expected, sizeof(expected)) == 0);
    race_data = readback;
#endif
}
#endif
