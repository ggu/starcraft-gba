/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_TERRAN_CARRIER_ENVELOPE_H
#define SC64_TERRAN_CARRIER_ENVELOPE_H

/* Opt-in custom journal. Include after the Terran RaceSave and impact types.
 * The ordinary Terran writer and loader still use the legacy payload.
 * The caller must authenticate the outer SRAM header and run the existing
 * complete game preflight on the reconstructed ordinary payload. */
#include "sc64_interceptor_pool_codec.h"

enum {
    SC64_TC_PREFIX = offsetof(RaceSave,invincible_units),
    SC64_TC_INV_COUNT = MAX_UNITS + MAX_BUILDINGS,
    SC64_TC_INV_BYTES = (SC64_TC_INV_COUNT + 7u) / 8u,
    SC64_TC_OMITTED = SC64_TC_PREFIX + SC64_TC_INV_COUNT,
    SC64_TC_BASE = offsetof(RaceSave,battlecruiser_shots),
    SC64_TC_TAIL_MIN = SC64_TRANSPORT_TRAILER_BYTES,
    SC64_TC_TAIL_MAX = SC64_BC_SAVE_EXTRA_BYTES + sizeof(Sc64ImpactSaveHeader) +
        RACE_IMPACT_ROWS * sizeof(Sc64ImpactEvent) + SC64_TRANSPORT_TRAILER_BYTES,
    SC64_TC_PACKET_MAX = 2015,
    SC64_TC_COUNTS = SC64_TC_PREFIX + SC64_TC_INV_BYTES,
    SC64_TC_HEADER = SC64_TC_COUNTS + 4,
    SC64_TC_WIRE_MAX = SC64_TC_HEADER + SC64_TC_PACKET_MAX + SC64_TC_TAIL_MAX
};
_Static_assert(sizeof(RaceSave)==15856 && offsetof(RaceSave,campaign)==13536 &&
    SC64_TC_BASE==15816,"Terran save layout changed");
_Static_assert(MAX_UNITS==192 && MAX_BUILDINGS==96 &&
    RACE_IMPACT_SAVE_CAPACITY==32 && SC64_BC_SHOT_LIMIT==2,
    "retain the full Terran actor, Yamato and impact capacities");
_Static_assert(offsetof(RaceSave,invincible_buildings)==SC64_TC_PREFIX+MAX_UNITS &&
    SC64_TC_OMITTED<SC64_TC_BASE,"Terran invincibility layout changed");
_Static_assert(SC64_TC_WIRE_MAX==15929 && SC64_TC_WIRE_MAX<=RACE_IMPACT_RECORD_MAX,
    "Terran compact journal budget");

/* Little-endian lengths: projectile packet then untouched old wire tail.
 * Keep the complete map/fog, wallets, campaign and Backwater words. Only
 * T3/late-mission storage can be elided; writer checks every omitted byte. */
static unsigned sc64_tc_get16(const u8 *b)
{ return b[0] | ((unsigned)b[1]<<8); }

static void sc64_tc_put16(u8 *b,unsigned n)
{ b[0]=(u8)n; b[1]=(u8)(n>>8); }

static int sc64_tc_prefix_valid(const u8 *base)
{
    const u32 version=SAVE_VERSION;
    const int custom=1;
    return !memcmp(base+offsetof(RaceSave,version),&version,sizeof(version)) &&
           !memcmp(base+offsetof(RaceSave,state),&custom,sizeof(custom));
}

/* Base and out may be identical; packet must be disjoint. */
static unsigned sc64_tc_pack(const u8 *base,unsigned len,const u8 *packet,
                              unsigned packet_len,u8 *out,unsigned capacity)
{
    if (!base || !packet || !out || len<SC64_TC_BASE+SC64_TC_TAIL_MIN ||
        len>SC64_TC_BASE+SC64_TC_TAIL_MAX || packet_len>SC64_TC_PACKET_MAX ||
        !sc64_tc_prefix_valid(base) ||
        !sc64_compact_codec_validate(packet,packet_len,SC64_POOL_CAPACITY,0,0)) return 0;
    for (unsigned i=SC64_TC_PREFIX;i<SC64_TC_OMITTED;++i)
        if (base[i]>1) return 0;
    for (unsigned i=SC64_TC_OMITTED;i<SC64_TC_BASE;++i)
        if (base[i]) return 0;
    unsigned tail=len-SC64_TC_BASE, result=SC64_TC_HEADER+packet_len+tail;
    if (capacity<result) return 0;
    u8 inv[SC64_TC_INV_BYTES]={0};
    for (unsigned i=0;i<SC64_TC_INV_COUNT;++i)
        inv[i/8] |= (u8)(base[SC64_TC_PREFIX+i]<<(i%8));
    if (out!=base) memcpy(out,base,SC64_TC_PREFIX);
    memmove(out+SC64_TC_HEADER+packet_len,base+SC64_TC_BASE,tail);
    memcpy(out+SC64_TC_PREFIX,inv,sizeof(inv));
    sc64_tc_put16(out+SC64_TC_COUNTS,packet_len);
    sc64_tc_put16(out+SC64_TC_COUNTS+2,tail);
    memcpy(out+SC64_TC_HEADER,packet,packet_len);
    return result;
}

/* Detached format check and exact reconstruction, including in place. Copy
 * the packet out before expanding in place; its world links still require
 * validation against the reconstructed candidate. */
static unsigned sc64_tc_unpack_packet(const u8 *wire,unsigned len,u8 *base,
                                      unsigned capacity,u8 *saved_packet,u16 *saved_size)
{
    if (!wire || !base || len<SC64_TC_HEADER || len>SC64_TC_WIRE_MAX ||
        !sc64_tc_prefix_valid(wire)) return 0;
    unsigned packet=sc64_tc_get16(wire+SC64_TC_COUNTS);
    unsigned tail=sc64_tc_get16(wire+SC64_TC_COUNTS+2);
    if (packet>SC64_TC_PACKET_MAX || tail<SC64_TC_TAIL_MIN || tail>SC64_TC_TAIL_MAX ||
        len!=SC64_TC_HEADER+packet+tail || capacity<SC64_TC_BASE+tail) return 0;
    if (SC64_TC_INV_COUNT%8 &&
        (wire[SC64_TC_COUNTS-1]>>(SC64_TC_INV_COUNT%8))) return 0;
    if (!sc64_compact_codec_validate(wire+SC64_TC_HEADER,packet,SC64_POOL_CAPACITY,0,0))
        return 0;
    if (saved_packet) {
        memcpy(saved_packet,wire+SC64_TC_HEADER,packet);
        *saved_size=(u16)packet;
    }
    u8 inv[SC64_TC_INV_BYTES];
    memcpy(inv,wire+SC64_TC_PREFIX,sizeof(inv));
    if (base!=wire) memcpy(base,wire,SC64_TC_PREFIX);
    memmove(base+SC64_TC_BASE,wire+SC64_TC_HEADER+packet,tail);
    for (unsigned i=0;i<SC64_TC_INV_COUNT;++i)
        base[SC64_TC_PREFIX+i]=(inv[i/8]>>(i%8))&1u;
    memset(base+SC64_TC_OMITTED,0,SC64_TC_BASE-SC64_TC_OMITTED);
    return SC64_TC_BASE+tail;
}
#ifndef SC64_TERRAN_CARRIER_PROBE
static unsigned sc64_tc_unpack(const u8 *wire,unsigned len,u8 *base,unsigned capacity)
{ return sc64_tc_unpack_packet(wire,len,base,capacity,0,0); }
#endif
#endif
