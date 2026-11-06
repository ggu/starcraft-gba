/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Z7_LEGACY_ENVELOPE_PROBE_H
#define SC64_Z7_LEGACY_ENVELOPE_PROBE_H
#include <stdint.h>
#include <stddef.h>

/* Narrow, read-only metadata probe for the independently witnessed Z7
 * generation-36 cargo/Transport1 format. It does not decode actor rows,
 * authenticate their identities, select a journal, migrate or admit a save.
 * The descriptor and callback must come from trusted, immutable game data;
 * a future caller must also perform the full predecessor and mission checks. */
enum { SC64_Z7_V36_SLOT_BYTES=16384u, SC64_Z7_V36_PAYLOAD_BASE=16296u,
       SC64_Z7_V36_TRAILER_BYTES=16u, SC64_Z7_V36_HEADER_BYTES=16u,
       SC64_Z7_V36_STATE_OFFSET=15652u, SC64_Z7_V36_CAMPAIGN_MISSION_OFFSET=15954u,
       SC64_Z7_V36_MISSION=18u, SC64_Z7_V36_ZERG=1u };
#define SC64_Z7_V36_OUTER UINT32_C(0x56435a54)
#define SC64_Z7_V36_INNER UINT32_C(0x52435a54)
typedef struct {
    const void *expected_map;
    int (*validate_map)(const void *map);
} Sc64Z7LegacyMapAuthority;
typedef struct {
    const volatile uint8_t *slot;
    uint32_t length, checksum, sequence;
} Sc64Z7LegacyEnvelopeProbe;
static uint32_t sc64_z7_v36_read32(const volatile uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int sc64_z7_v36_probe_context(const volatile uint8_t *slot, size_t slot_size,
                                     const void *candidate_map,
                                     const Sc64Z7LegacyMapAuthority *authority,
                                     Sc64Z7LegacyEnvelopeProbe *result)
{
    uint32_t length, checksum, sequence, hash=UINT32_C(2166136261);
    if (!result) return 0;
    /* Never let an accidentally aliased output write to the examined SRAM. */
    if (slot) {
        uintptr_t r=(uintptr_t)result, s=(uintptr_t)slot;
        if ((r>=s && r-s<slot_size) || (s>=r && s-r<sizeof(*result))) return 0;
    }
    *result=(Sc64Z7LegacyEnvelopeProbe){0};
    if (!slot || slot_size!=SC64_Z7_V36_SLOT_BYTES || !authority ||
        !authority->expected_map || candidate_map!=authority->expected_map ||
        !authority->validate_map || !authority->validate_map(candidate_map)) return 0;
    if (sc64_z7_v36_read32(slot)!=SC64_Z7_V36_OUTER) return 0;
    length=sc64_z7_v36_read32(slot+4);checksum=sc64_z7_v36_read32(slot+8);
    sequence=sc64_z7_v36_read32(slot+12);
    if (length<SC64_Z7_V36_PAYLOAD_BASE+SC64_Z7_V36_TRAILER_BYTES ||
        length>SC64_Z7_V36_SLOT_BYTES-SC64_Z7_V36_HEADER_BYTES || !sequence) return 0;
    for (uint32_t i=0;i<length;++i) hash=(hash ^ slot[SC64_Z7_V36_HEADER_BYTES+i])*UINT32_C(16777619);
    if (hash!=checksum) return 0;
    const volatile uint8_t *payload=slot+SC64_Z7_V36_HEADER_BYTES;
    uint32_t extra=length-(SC64_Z7_V36_PAYLOAD_BASE+SC64_Z7_V36_TRAILER_BYTES);
    if (extra) {
        const volatile uint8_t *extension=payload+SC64_Z7_V36_PAYLOAD_BASE;
        if (extra<8 || extension[0]!='I' || extension[1]!='M' ||
            extension[2]!='P' || extension[3]!='1' || extension[4]<1 ||
            extension[4]>8 || extension[5] || extension[6] || extension[7] ||
            extra!=8u+6u*extension[4]) return 0;
    }
    if (sc64_z7_v36_read32(payload)!=SC64_Z7_V36_INNER ||
        sc64_z7_v36_read32(payload+SC64_Z7_V36_STATE_OFFSET)!=0 ||
        sc64_z7_v36_read32(payload+SC64_Z7_V36_STATE_OFFSET+4)!=SC64_Z7_V36_ZERG ||
        sc64_z7_v36_read32(payload+SC64_Z7_V36_STATE_OFFSET+8)!=SC64_Z7_V36_MISSION ||
        payload[SC64_Z7_V36_CAMPAIGN_MISSION_OFFSET]!=SC64_Z7_V36_MISSION) return 0;
    *result=(Sc64Z7LegacyEnvelopeProbe){slot,length,checksum,sequence};
    return 1;
}
#endif
