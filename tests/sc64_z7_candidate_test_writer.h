/* SPDX-License-Identifier: GPL-3.0-or-later
 * Host-only candidate mutation helper. Preserve the current normal envelope's
 * impact rows and Transport1 trailer; replace only the RaceSave prefix. */
#ifndef SC64_Z7_CANDIDATE_TEST_WRITER_H
#define SC64_Z7_CANDIDATE_TEST_WRITER_H
#ifndef GBA_HOST
#error "candidate test writer is host-only"
#endif
#if defined(SC64_SHARED_TRANSPORT) && defined(SC64_BATTLECRUISER_YAMATO_EXPERIMENT)
#error "this test helper does not support the variable Yamato prefix"
#endif
static void sc64_z7_test_write_candidate(const RaceSave *candidate)
{
#ifdef SC64_SHARED_TRANSPORT
    static unsigned char envelope[16384]; u32 length=0;
    /* The most recent ordinary save supplies the exact opaque suffix. Every
     * subsequent candidate write preserves it byte for byte. */
    assert(save_read_latest(envelope,sizeof(envelope),SC64_TRANSPORT_SAVE_VERSION,&length));
    assert(length>=sizeof(*candidate)+16u);
    memcpy(envelope,candidate,sizeof(*candidate));
    save_write(envelope,length,SC64_TRANSPORT_SAVE_VERSION);
#else
    save_write(candidate,sizeof(*candidate),SAVE_VERSION);
#endif
}
#endif
