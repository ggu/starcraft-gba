/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ZERG_CARRIER_ENVELOPE_H
#define SC64_ZERG_CARRIER_ENVELOPE_H

/* Detached storage study for the two current 64x64 Z10 custom maps.
 * Include after the normal RaceSave, impact and transport codecs. The outer
 * journal is not wired into the shipping reader or writer. */
#include "sc64_interceptor_pool_codec.h"

enum {
    SC64_ZC_UNIT_BITS = 280,
    SC64_ZC_BUILDING_BITS = 224,
    SC64_ZC_BUILDING_USED = 217,
    SC64_ZC_UNIT_OWNER = 244,
    SC64_ZC_BUILDING_OWNER = 169,
    SC64_ZC_OWNER_BITS = 4, /* raw owner + 1, including unset and neutral 11 */
    SC64_ZC_STRIP_PER_ROW = 2 + (9 - SC64_ZC_OWNER_BITS),
    SC64_ZC_UNIT_WIRE_BITS = SC64_ZC_UNIT_BITS - SC64_ZC_STRIP_PER_ROW,
    SC64_ZC_BUILDING_WIRE_BITS = SC64_ZC_BUILDING_USED - SC64_ZC_STRIP_PER_ROW,
    SC64_ZC_UNITS = offsetof(RaceSave, units),
    SC64_ZC_BUILDINGS = SC64_ZC_UNITS + MAX_UNITS * SC64_ZC_UNIT_WIRE_BITS / 8,
    SC64_ZC_MINERALS = SC64_ZC_BUILDINGS + MAX_BUILDINGS * SC64_ZC_BUILDING_WIRE_BITS / 8,
    SC64_ZC_FOG = SC64_ZC_MINERALS + sizeof(((RaceSave *)0)->minerals_map),
    SC64_ZC_FOG_BYTES = 64 * 64 / 8,
    SC64_ZC_MISC = SC64_ZC_FOG + SC64_ZC_FOG_BYTES,
    SC64_ZC_MISC_BYTES = offsetof(RaceSave, z4) - offsetof(RaceSave, state),
    SC64_ZC_COUNTS = SC64_ZC_MISC + SC64_ZC_MISC_BYTES,
    SC64_ZC_HEADER = SC64_ZC_COUNTS + 4,
    SC64_ZC_PACKET_MAX = 2015, /* full 100-row compact Interceptor pool */
    SC64_ZC_TAIL_MAX = sizeof(Sc64ImpactSaveHeader) +
        RACE_IMPACT_ROWS * sizeof(Sc64ImpactEvent) + SC64_TRANSPORT_TRAILER_BYTES,
    SC64_ZC_LOGICAL_MAX = sizeof(RaceSave) + SC64_ZC_TAIL_MAX,
    SC64_ZC_PACKET_WORK = SC64_ZC_LOGICAL_MAX,
    SC64_ZC_WORK_BYTES = SC64_ZC_PACKET_WORK + SC64_ZC_PACKET_MAX,
    SC64_ZC_WIRE_MAX = SC64_ZC_HEADER + SC64_ZC_PACKET_MAX + SC64_ZC_TAIL_MAX
};
_Static_assert(MAX_UNITS == 240 && MAX_BUILDINGS == 160 &&
    SC64_BC_SAVE_EXTRA_BYTES == 0 && SC64_TRANSPORT_TRAILER_BYTES == 16,
    "Z10 custom envelope layout");
_Static_assert(sizeof(RaceSave) == 16296 && SC64_ZC_UNITS == 4 &&
    offsetof(RaceSave, buildings) == 8404 &&
    offsetof(RaceSave, minerals_map) == 12884 &&
    offsetof(RaceSave, fog) == 13604 &&
    offsetof(RaceSave, state) == 15652 && offsetof(RaceSave, z4) == 16076,
    "keep the authenticated Z10 predecessor offsets");
_Static_assert(sizeof(((RaceSave *)0)->fog) == 2048 &&
    sizeof(((RaceSave *)0)->units[0]) * 8 == SC64_ZC_UNIT_BITS &&
    sizeof(((RaceSave *)0)->buildings[0]) * 8 == SC64_ZC_BUILDING_BITS &&
    (MAX_UNITS * SC64_ZC_UNIT_WIRE_BITS) % 8 == 0 &&
    (MAX_BUILDINGS * SC64_ZC_BUILDING_WIRE_BITS) % 8 == 0,
    "do not change predecessor row widths or use fractional sections");
_Static_assert(SC64_ZC_HEADER == 14054 && SC64_ZC_WIRE_MAX == 16285 &&
    SC64_ZC_WIRE_MAX <= RACE_IMPACT_RECORD_MAX, "full custom journal fits");
_Static_assert(SC64_ZC_WORK_BYTES == 18527 && SC64_ZC_WORK_BYTES <= 32768,
    "custom save and packet share existing Z10 route/save scratch");

static unsigned sc64_zc_get16(const u8 *b)
{ return b[0] | ((unsigned)b[1] << 8); }

static void sc64_zc_put16(u8 *b, unsigned n)
{ b[0] = (u8)n; b[1] = (u8)(n >> 8); }

static int sc64_zc_custom(const u8 *version, const u8 *state)
{
    const u32 expected = SAVE_VERSION;
    int mode, map;
    memcpy(&mode,state,sizeof(mode));
    memcpy(&map,state+20*sizeof(int),sizeof(map));
    if (memcmp(version,&expected,sizeof(expected)) || mode != 1) return 0;
    if (map == 0) return 1; /* authored Outpost geometry */
    return map == SC64_MAP_BLOOD_BATH && map <= SC64_SKIRMISH_IMPORTED_COUNT &&
        sc64_skirmish_maps[map-1].width == 64 && sc64_skirmish_maps[map-1].height == 64;
}

/* A compact row keeps x:10, y:10, every bit up through the low four raw-owner
 * bits, then every bit through the normal used end. The old coordinate MSBs
 * cannot be set on these maps; the top five raw-owner bits and the building
 * reserved bits are checked individually before any output is written. */
static int sc64_zc_zero_bits(const u8 *src, unsigned lo, unsigned hi)
{
    for (unsigned bit = lo; bit < hi; ++bit)
        if ((src[bit >> 3] >> (bit & 7u)) & 1u) return 0;
    return 1;
}

static int sc64_zc_row_valid(const u8 *row, unsigned owner, unsigned used, unsigned total)
{
    return sc64_zc_zero_bits(row,10,11) && sc64_zc_zero_bits(row,21,22) &&
        sc64_zc_zero_bits(row,owner+SC64_ZC_OWNER_BITS,owner+9) &&
        sc64_zc_zero_bits(row,used,total);
}

/* Copy bits between disjoint buffers, taking up to one destination and one
 * source byte at a time. Only the requested bits are replaced, including
 * byte edges shared with adjacent slot records. */
static void sc64_zc_copy(u8 *dst,unsigned d,const u8 *src,unsigned s,unsigned n)
{
    while (n) {
        unsigned dl = d & 7u, sl = s & 7u;
        unsigned step = 8u - (dl > sl ? dl : sl);
        if (step > n) step = n;
        unsigned mask = (1u << step) - 1u;
        unsigned bits = (src[s >> 3] >> sl) & mask;
        dst[d >> 3] = (u8)((dst[d >> 3] & ~(mask << dl)) | (bits << dl));
        d += step; s += step; n -= step;
    }
}

static void sc64_zc_row(u8 *out,unsigned d,const u8 *in,unsigned s,
                        unsigned owner,unsigned used,int restore)
{
    unsigned split = owner + SC64_ZC_OWNER_BITS;
    unsigned middle = split - 22, last = used - (owner + 9);
#define SC64_ZC_SLICE(base,wire,count) do { \
        if (restore) sc64_zc_copy(out,d+(base),in,s+(wire),(count)); \
        else sc64_zc_copy(out,d+(wire),in,s+(base),(count)); \
    } while (0)
    SC64_ZC_SLICE(0,0,10);
    SC64_ZC_SLICE(11,10,10);
    SC64_ZC_SLICE(22,20,middle);
    SC64_ZC_SLICE(owner+9,20+middle,last);
#undef SC64_ZC_SLICE
}

/* Validate the intact old impact/transport block independently of the legacy
 * SRAM capacity. The study keeps all live impacts; the normal recorder only
 * has room for eight. Runtime publication must additionally validate actor
 * and projectile ownership against the candidate world. */
static int sc64_zc_tail(const u8 *tail,unsigned size)
{
    if (size < SC64_TRANSPORT_TRAILER_BYTES || size > SC64_ZC_TAIL_MAX) return 0;
    unsigned impact = size - SC64_TRANSPORT_TRAILER_BYTES;
    Sc64SkTransportTech tech;
    if (!sc64_sk_transport_decode(&tech,tail+impact,MAX_BUILDINGS)) return 0;
    Sc64ImpactEvent rows[RACE_IMPACT_ROWS];
    return !impact || sc64_impact_decode(tail,impact,RACE_IMPACT_ROWS,
        MAX_UNITS+MAX_BUILDINGS,5,SC_RU_COUNT,rows)>0;
}

/* base includes the unchanged impact/transport tail. base and out may
 * be the same buffer, or disjoint. Packet must be disjoint from both
 * occupied spans (in shared scratch, place it at SC64_ZC_PACKET_WORK).
 * Zero is a transactional format rejection. */
static unsigned sc64_zc_pack(const u8 *base,unsigned len,const u8 *packet,
                             unsigned packet_len,u8 *out,unsigned capacity)
{
    if (!base || !packet || !out || len < sizeof(RaceSave) + SC64_TRANSPORT_TRAILER_BYTES ||
        len > SC64_ZC_LOGICAL_MAX || packet_len > SC64_ZC_PACKET_MAX) return 0;
    unsigned tail = len - sizeof(RaceSave), total = SC64_ZC_HEADER + packet_len + tail;
    if (total > capacity || !sc64_zc_custom(base,base+offsetof(RaceSave,state)) ||
        !sc64_zc_tail(base+sizeof(RaceSave),tail) ||
        !sc64_compact_codec_validate(packet,packet_len,SC64_POOL_CAPACITY,0,0)) return 0;
    for (unsigned i=offsetof(RaceSave,fog)+SC64_ZC_FOG_BYTES;i<offsetof(RaceSave,state);++i)
        if (base[i]) return 0;
    for (unsigned i=offsetof(RaceSave,z4);i<sizeof(RaceSave);++i)
        if (base[i]) return 0;
    for (unsigned i=0;i<MAX_UNITS;++i)
        if (!sc64_zc_row_valid(base+offsetof(RaceSave,units)+i*(SC64_ZC_UNIT_BITS/8),
                              SC64_ZC_UNIT_OWNER,SC64_ZC_UNIT_BITS,SC64_ZC_UNIT_BITS)) return 0;
    for (unsigned i=0;i<MAX_BUILDINGS;++i)
        if (!sc64_zc_row_valid(base+offsetof(RaceSave,buildings)+i*(SC64_ZC_BUILDING_BITS/8),
                              SC64_ZC_BUILDING_OWNER,SC64_ZC_BUILDING_USED,
                              SC64_ZC_BUILDING_BITS)) return 0;
    int inplace=out==base;
    u8 stage[SC64_ZC_UNIT_BITS/8];
    if (!inplace) {
        memcpy(out,base,SC64_ZC_UNITS);
        memset(out+SC64_ZC_UNITS,0,SC64_ZC_MINERALS-SC64_ZC_UNITS);
    }
    for (unsigned i=0;i<MAX_UNITS;++i) {
        const u8 *row=base+offsetof(RaceSave,units)+i*(SC64_ZC_UNIT_BITS/8);
        if (inplace) { memcpy(stage,row,SC64_ZC_UNIT_BITS/8); row=stage; }
        sc64_zc_row(out+SC64_ZC_UNITS,i*SC64_ZC_UNIT_WIRE_BITS,row,0,
                    SC64_ZC_UNIT_OWNER,SC64_ZC_UNIT_BITS,0);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        const u8 *row=base+offsetof(RaceSave,buildings)+i*(SC64_ZC_BUILDING_BITS/8);
        if (inplace) { memcpy(stage,row,SC64_ZC_BUILDING_BITS/8); row=stage; }
        sc64_zc_row(out+SC64_ZC_BUILDINGS,i*SC64_ZC_BUILDING_WIRE_BITS,row,0,
                    SC64_ZC_BUILDING_OWNER,SC64_ZC_BUILDING_USED,0);
    }
    memmove(out+SC64_ZC_MINERALS,base+offsetof(RaceSave,minerals_map),
            SC64_ZC_MISC-SC64_ZC_MINERALS);
    memmove(out+SC64_ZC_MISC,base+offsetof(RaceSave,state),SC64_ZC_MISC_BYTES);
    /* The ordinary trailer lies after all source rows. Move it before the
     * new lengths and packet can claim any lower bytes. */
    memmove(out+SC64_ZC_HEADER+packet_len,base+sizeof(RaceSave),tail);
    sc64_zc_put16(out+SC64_ZC_COUNTS,packet_len);
    sc64_zc_put16(out+SC64_ZC_COUNTS+2,tail);
    memcpy(out+SC64_ZC_HEADER,packet,packet_len);
    return total;
}

static int sc64_zc_open(const u8 *wire,unsigned len,unsigned *packet,unsigned *tail)
{
    if (!wire || len < SC64_ZC_HEADER || len > SC64_ZC_WIRE_MAX ||
        !sc64_zc_custom(wire,wire+SC64_ZC_MISC)) return 0;
    unsigned p=sc64_zc_get16(wire+SC64_ZC_COUNTS), t=sc64_zc_get16(wire+SC64_ZC_COUNTS+2);
    if (p > SC64_ZC_PACKET_MAX || t > SC64_ZC_TAIL_MAX ||
        len != SC64_ZC_HEADER+p+t || !sc64_zc_tail(wire+SC64_ZC_HEADER+p,t) ||
        !sc64_compact_codec_validate(wire+SC64_ZC_HEADER,p,SC64_POOL_CAPACITY,0,0)) return 0;
    *packet=p; *tail=t;
    return 1;
}

/* Detached output is disjoint from the immutable input. */
static unsigned sc64_zc_unpack(const u8 *wire,unsigned len,u8 *base,unsigned capacity)
{
    unsigned packet,tail;
    if (!base || !sc64_zc_open(wire,len,&packet,&tail) ||
        capacity < sizeof(RaceSave)+tail) return 0;
    memset(base,0,sizeof(RaceSave));
    memcpy(base,wire,SC64_ZC_UNITS);
    for (unsigned i=0;i<MAX_UNITS;++i)
        sc64_zc_row(base+offsetof(RaceSave,units)+i*(SC64_ZC_UNIT_BITS/8),0,
                    wire+SC64_ZC_UNITS,i*SC64_ZC_UNIT_WIRE_BITS,
                    SC64_ZC_UNIT_OWNER,SC64_ZC_UNIT_BITS,1);
    for (unsigned i=0;i<MAX_BUILDINGS;++i)
        sc64_zc_row(base+offsetof(RaceSave,buildings)+i*(SC64_ZC_BUILDING_BITS/8),0,
                    wire+SC64_ZC_BUILDINGS,i*SC64_ZC_BUILDING_WIRE_BITS,
                    SC64_ZC_BUILDING_OWNER,SC64_ZC_BUILDING_USED,1);
    memcpy(base+offsetof(RaceSave,minerals_map),wire+SC64_ZC_MINERALS,
           SC64_ZC_MISC-SC64_ZC_MINERALS);
    memcpy(base+offsetof(RaceSave,state),wire+SC64_ZC_MISC,SC64_ZC_MISC_BYTES);
    memcpy(base+sizeof(RaceSave),wire+SC64_ZC_HEADER+packet,tail);
    return sizeof(RaceSave)+tail;
}

/* Expand right to left inside a single save/scratch buffer. Save the
 * independently checked packet above the largest logical world so candidate
 * preflight can still validate it after rebuilding the predecessor rows. */
static unsigned sc64_zc_unpack_in_place(u8 *base,unsigned len,unsigned capacity,
                                        unsigned *packet_size)
{
    unsigned packet,tail;
    if (!packet_size || capacity < SC64_ZC_WORK_BYTES ||
        !sc64_zc_open(base,len,&packet,&tail)) return 0;
    memcpy(base+SC64_ZC_PACKET_WORK,base+SC64_ZC_HEADER,packet);
    memmove(base+sizeof(RaceSave),base+SC64_ZC_HEADER+packet,tail);
    memset(base+offsetof(RaceSave,z4),0,sizeof(RaceSave)-offsetof(RaceSave,z4));
    memmove(base+offsetof(RaceSave,state),base+SC64_ZC_MISC,SC64_ZC_MISC_BYTES);
    memset(base+offsetof(RaceSave,fog)+SC64_ZC_FOG_BYTES,0,
           sizeof(((RaceSave *)0)->fog)-SC64_ZC_FOG_BYTES);
    memmove(base+offsetof(RaceSave,minerals_map),base+SC64_ZC_MINERALS,
            SC64_ZC_MISC-SC64_ZC_MINERALS);
    u8 stage[SC64_ZC_UNIT_BITS/8];
    for (unsigned i=MAX_BUILDINGS;i>0;) {
        unsigned bit=(--i)*SC64_ZC_BUILDING_WIRE_BITS, phase=bit&7u;
        memcpy(stage,base+SC64_ZC_BUILDINGS+bit/8,(phase+SC64_ZC_BUILDING_WIRE_BITS+7)/8);
        u8 *row=base+offsetof(RaceSave,buildings)+i*(SC64_ZC_BUILDING_BITS/8);
        memset(row,0,SC64_ZC_BUILDING_BITS/8);
        sc64_zc_row(row,0,stage,phase,SC64_ZC_BUILDING_OWNER,SC64_ZC_BUILDING_USED,1);
    }
    for (unsigned i=MAX_UNITS;i>0;) {
        unsigned bit=(--i)*SC64_ZC_UNIT_WIRE_BITS, phase=bit&7u;
        memcpy(stage,base+SC64_ZC_UNITS+bit/8,(phase+SC64_ZC_UNIT_WIRE_BITS+7)/8);
        u8 *row=base+offsetof(RaceSave,units)+i*(SC64_ZC_UNIT_BITS/8);
        memset(row,0,SC64_ZC_UNIT_BITS/8);
        sc64_zc_row(row,0,stage,phase,SC64_ZC_UNIT_OWNER,SC64_ZC_UNIT_BITS,1);
    }
    *packet_size=packet;
    return sizeof(RaceSave)+tail;
}

#endif
