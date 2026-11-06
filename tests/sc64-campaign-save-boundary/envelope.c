/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Metadata only: successful probes do not admit or decode any actor. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../starcraft/sc64_z7_legacy_envelope_probe.h"
static _Alignas(Sc64Z7LegacyEnvelopeProbe) uint8_t bytes[SC64_Z7_V36_SLOT_BYTES];
static uint8_t before[SC64_Z7_V36_SLOT_BYTES];
static const int good_map=18, bad_map=18;
static int valid_map(const void *map) { return map==&good_map; }
static void put32(unsigned offset,uint32_t n) { for(unsigned i=0;i<4;i++)bytes[offset+i]=(uint8_t)(n>>(i*8)); }
static uint32_t checksum(void) {
    uint32_t h=UINT32_C(2166136261), len=sc64_z7_v36_read32(bytes+4);
    assert(len<=SC64_Z7_V36_SLOT_BYTES-16u);
    for(unsigned i=0;i<len;i++)h=(h^bytes[16+i])*UINT32_C(16777619);
    return h;
}
static void fix_checksum(void) { put32(8,checksum()); }
static void blank(void) {
    memset(bytes,0,sizeof(bytes));put32(0,SC64_Z7_V36_OUTER);
    put32(4,SC64_Z7_V36_PAYLOAD_BASE+SC64_Z7_V36_TRAILER_BYTES);put32(12,1);
    put32(16,SC64_Z7_V36_INNER);put32(16+SC64_Z7_V36_STATE_OFFSET+4,1);
    put32(16+SC64_Z7_V36_STATE_OFFSET+8,18);bytes[16+SC64_Z7_V36_CAMPAIGN_MISSION_OFFSET]=18;
    fix_checksum();
}
static int probe(const void *map,Sc64Z7LegacyEnvelopeProbe *out) {
    const Sc64Z7LegacyMapAuthority authority={&good_map,valid_map};
    return sc64_z7_v36_probe_context(bytes,sizeof(bytes),map,&authority,out);
}
static void reject(Sc64Z7LegacyEnvelopeProbe *out) {
    *out=(Sc64Z7LegacyEnvelopeProbe){bytes,1,1,1};
    memcpy(before,bytes,sizeof(bytes));assert(!probe(&good_map,out));
    assert(!out->slot && !out->length && !out->checksum && !out->sequence);
    assert(!memcmp(before,bytes,sizeof(bytes)));
}
int main(int argc,char **argv) {
    Sc64Z7LegacyEnvelopeProbe out;
    blank();memcpy(before,bytes,sizeof(bytes));assert(probe(&good_map,&out));
    assert(out.slot==bytes && out.length==16312 && out.sequence==1 && !memcmp(before,bytes,sizeof(bytes)));
    assert(!probe(&bad_map,&out) && !out.slot);
    { const Sc64Z7LegacyMapAuthority authority={&good_map,valid_map};
      memcpy(before,bytes,sizeof(bytes));
      assert(!sc64_z7_v36_probe_context(bytes,sizeof(bytes),&good_map,&authority,
             (Sc64Z7LegacyEnvelopeProbe *)(void *)bytes));
      assert(!memcmp(before,bytes,sizeof(bytes))); }
    blank();put32(0,0x56435a56u);reject(&out); /* wrong product/generation */
    blank();put32(4,16369);reject(&out);blank();put32(4,16311);reject(&out);
    blank();put32(12,0);reject(&out);blank();bytes[100]^=1;reject(&out);
    blank();put32(4,16326);fix_checksum();reject(&out); /* malformed IMP1 */
    blank();put32(4,16326);memcpy(bytes+16+16296,"IMP1",4);
    bytes[16+16296+4]=1;fix_checksum();assert(probe(&good_map,&out));
    blank();put32(16,0x52435a56u);fix_checksum();reject(&out);
    blank();put32(16+SC64_Z7_V36_STATE_OFFSET,1);fix_checksum();reject(&out);
    blank();put32(16+SC64_Z7_V36_STATE_OFFSET+4,2);fix_checksum();reject(&out);
    blank();put32(16+SC64_Z7_V36_STATE_OFFSET+8,19);fix_checksum();reject(&out);
    blank();bytes[16+SC64_Z7_V36_CAMPAIGN_MISSION_OFFSET]=19;fix_checksum();reject(&out);
    /* Actor-row bytes are opaque at this boundary. Even a checksummed row
     * change yields only metadata, never permission to install the world. */
    blank();bytes[16+4+31]^=0x80;fix_checksum();assert(probe(&good_map,&out));
    if(argc==2) {
        FILE *f=fopen(argv[1],"rb");assert(f);assert(fread(bytes,1,sizeof(bytes),f)==sizeof(bytes));assert(!fclose(f));
        memcpy(before,bytes,sizeof(bytes));assert(probe(&good_map,&out));assert(!memcmp(before,bytes,sizeof(bytes)));
        printf("genuine predecessor metadata accepted; no actor decode or install\n");
    }
    puts("Z7 predecessor metadata boundary passed");return 0;
}
