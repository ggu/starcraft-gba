/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P10_PADDED_IMPACT_PAYLOAD_H
#define SC64_P10_PADDED_IMPACT_PAYLOAD_H
/* Detached P10 typed rows only. Include after RaceSave and impact types.
 * The outer journal authenticates the distinct format and supplies a detached
 * candidate. These routines do not select tags, publish SRAM or validate the
 * reconstructed world. Decode/normalize failure may mutate detached inputs. */
#include <stdint.h>
#include <string.h>
#include "sc64_impact_events.h"
enum { SC64_P10_PI_ROWS=32, SC64_P10_PI_EMBED=180,
       SC64_P10_PI_PACKET=200, SC64_P10_PI_TAIL=20,
       SC64_P10_PI_BITS=1504 };
_Static_assert(MAX_UNITS==240 && MAX_BUILDINGS==160,"P10 row counts");
_Static_assert(sizeof(((RaceSave *)0)->units[0])==28 &&
               sizeof(((RaceSave *)0)->buildings[0])==25,"P10 typed rows");
_Static_assert(sizeof(Sc64ImpactEvent)==6 && sizeof(Sc64ImpactSaveHeader)==8,
               "P10 impact packet layout");
static inline unsigned sc64_p10_pi_get(const uint8_t *p, unsigned at, unsigned n)
{
    unsigned value = 0;
    for (unsigned i = 0; i < n; ++i)
        value |= ((p[(at + i) / 8] >> ((at + i) % 8)) & 1u) << i;
    return value;
}

static inline void sc64_p10_pi_put(uint8_t *p, unsigned at, unsigned n, unsigned value)
{
    for (unsigned i = 0; i < n; ++i) {
        unsigned bit = at + i;
        unsigned mask = 1u << (bit % 8);
        p[bit / 8] = (uint8_t)((p[bit / 8] & ~mask) |
                               (((value >> i) & 1u) * mask));
    }
}

static inline void sc64_p10_pi_copy(uint8_t *out, unsigned to,
                                    const uint8_t *in, unsigned from, unsigned n)
{
    for (unsigned i = 0; i < n; ++i)
        sc64_p10_pi_put(out, to + i, 1, sc64_p10_pi_get(in, from + i, 1));
}

/* The unchanged prefix has five complete bytes in both layouts. Unit
 * padding occupies bits 222 and 223 of the expanded journal row. */
static inline int sc64_p10_pi_unit(uint8_t out[28], const uint8_t in[28], int encode)
{
    uint8_t row[28] = {0};
    memcpy(row, in, 5);
    if (encode) {
        unsigned owner = sc64_p10_pi_get(in, 40, 3);
        unsigned path_timer = sc64_p10_pi_get(in, 146, 5);
        unsigned path_retry = sc64_p10_pi_get(in, 151, 4);
        unsigned shield_remainder = sc64_p10_pi_get(in, 212, 11);
        if (owner >= 5 || path_timer >= 25 || path_retry >= 13 ||
            shield_remainder >= 1280) return 0;
        unsigned path = (owner * 25 + path_timer) * 13 + path_retry;
        sc64_p10_pi_put(row, 40, 21, path * 1280 + shield_remainder);
        sc64_p10_pi_copy(row, 61, in, 43, 103);
        sc64_p10_pi_copy(row, 164, in, 155, 57);
        sc64_p10_pi_copy(row, 221, in, 223, 1);
    } else {
        unsigned packed = sc64_p10_pi_get(in, 40, 21);
        if (packed >= 5u * 25u * 13u * 1280u) return 0;
        unsigned path = packed / 1280;
        sc64_p10_pi_put(row, 40, 3, path / (25 * 13));
        sc64_p10_pi_copy(row, 43, in, 61, 103);
        sc64_p10_pi_put(row, 146, 5, (path / 13) % 25);
        sc64_p10_pi_put(row, 151, 4, path % 13);
        sc64_p10_pi_copy(row, 155, in, 164, 57);
        sc64_p10_pi_put(row, 212, 11, packed % 1280);
        sc64_p10_pi_copy(row, 223, in, 221, 1);
    }
    memcpy(out, row, sizeof(row));
    return 1;
}

/* Building padding occupies bits 193 and 194 of the expanded journal row.
 * The research padding keeps its original positions at bits 195 to 199. */
static inline int sc64_p10_pi_building(uint8_t out[25], const uint8_t in[25], int encode)
{
    uint8_t row[25] = {0};
    memcpy(row, in, 5);
    if (encode) {
        unsigned owner = sc64_p10_pi_get(in, 40, 3);
        unsigned shield_remainder = sc64_p10_pi_get(in, 182, 11);
        unsigned queue_head = sc64_p10_pi_get(in, 80, 3);
        unsigned queue_count = sc64_p10_pi_get(in, 83, 3);
        if (owner >= 5 || shield_remainder >= 1280 ||
            queue_head >= 5 || queue_count >= 6) return 0;
        sc64_p10_pi_put(row, 40, 13, owner * 1280 + shield_remainder);
        sc64_p10_pi_copy(row, 53, in, 43, 37);
        sc64_p10_pi_put(row, 90, 5, queue_head * 6 + queue_count);
        sc64_p10_pi_copy(row, 95, in, 86, 96);
        sc64_p10_pi_copy(row, 191, in, 193, 2);
    } else {
        unsigned packed_shields = sc64_p10_pi_get(in, 40, 13);
        unsigned packed_queue = sc64_p10_pi_get(in, 90, 5);
        if (packed_shields >= 5u * 1280u || packed_queue >= 5u * 6u) return 0;
        sc64_p10_pi_put(row, 40, 3, packed_shields / 1280);
        sc64_p10_pi_copy(row, 43, in, 53, 37);
        sc64_p10_pi_put(row, 80, 3, packed_queue / 6);
        sc64_p10_pi_put(row, 83, 3, packed_queue % 6);
        sc64_p10_pi_copy(row, 86, in, 95, 96);
        sc64_p10_pi_put(row, 182, 11, packed_shields % 1280);
        sc64_p10_pi_copy(row, 193, in, 191, 2);
    }
    sc64_p10_pi_copy(row, 195, in, 195, 5);
    memcpy(out, row, sizeof(row));
    return 1;
}

/* First 96 of the 800 building padding bits belong to research. */
static inline uint8_t *sc64_p10_pi_side(RaceSave *save,unsigned n,unsigned *bit)
{
    if (n<480) { *bit=222+n%2; return save->units[n/2].bytes; }
    n-=480;
    if (n<320) { *bit=193+n%2; return save->buildings[n/2].bytes; }
    n-=320; n+=96;
    *bit=195+n%5; return save->buildings[n/5].bytes;
}
/* Output tail is always exactly 20 bytes, zero-padded for shorter packets.
 * Requires canonical detached rows, including zero unused research padding.
 * A rejected encode must not be published; candidate may already be changed. */
static inline int sc64_p10_pi_encode(RaceSave *save,uint8_t tail[SC64_P10_PI_TAIL],
                                      const Sc64ImpactEvent *rows,unsigned count)
{
    uint8_t packet[SC64_P10_PI_PACKET] = {0};
    uint8_t row[28];
    unsigned bit;
    if (!save || !tail || !rows || !count || count>SC64_P10_PI_ROWS) return 0;
    for (unsigned i=0;i<count;i++)
        if (!sc64_impact_event_valid(&rows[i],MAX_UNITS+MAX_BUILDINGS,5,SC_RU_COUNT)) return 0;
    for (unsigned i=0;i<MAX_UNITS;i++)
        if (!sc64_p10_pi_unit(row,save->units[i].bytes,1)) return 0;
    for (unsigned i=0;i<MAX_BUILDINGS;i++) {
        if (!sc64_p10_pi_building(row,save->buildings[i].bytes,1)) return 0;
        for (unsigned j=0;j<5;j++)
            if (i*5+j>=96 && sc64_p10_pi_get(save->buildings[i].bytes,195+j,1)) return 0;
    }
    sc64_impact_encode(rows,count,packet);
    for (unsigned i=0;i<MAX_UNITS;i++) {
        if (!sc64_p10_pi_unit(row,save->units[i].bytes,1)) return 0;
        memcpy(save->units[i].bytes,row,28);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;i++) {
        if (!sc64_p10_pi_building(row,save->buildings[i].bytes,1)) return 0;
        memcpy(save->buildings[i].bytes,row,25);
    }
    for (unsigned n=0;n<SC64_P10_PI_EMBED*8;n++) {
        uint8_t *p=sc64_p10_pi_side(save,n,&bit);
        sc64_p10_pi_put(p,bit,1,sc64_p10_pi_get(packet,n,1));
    }
    memcpy(tail,packet+SC64_P10_PI_EMBED,SC64_P10_PI_TAIL);
    return 1;
}
/* Restore rows after a successful or failed journal write. This removes all
 * sideband bits and leaves the first 96 research bits unchanged. Caller must
 * pass a fully encoded detached save; failure may mutate that save. */
static inline int sc64_p10_pi_normalize_rows(RaceSave *save)
{
    uint8_t row[28];
    unsigned bit;
    if (!save) return 0;
    for (unsigned n=0;n<SC64_P10_PI_BITS;n++) {
        uint8_t *p=sc64_p10_pi_side(save,n,&bit);
        sc64_p10_pi_put(p,bit,1,0);
    }
    for (unsigned i=0;i<MAX_UNITS;i++) {
        if (!sc64_p10_pi_unit(row,save->units[i].bytes,0)) return 0;
        memcpy(save->units[i].bytes,row,28);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;i++) {
        if (!sc64_p10_pi_building(row,save->buildings[i].bytes,0)) return 0;
        memcpy(save->buildings[i].bytes,row,25);
    }
    return 1;
}
/* Extract packet and normalize detached rows. Research bits are untouched.
 * The caller must subsequently validate the ordinary world and research.
 * Tail length is exact; unused sideband and packet bytes must be zero. */
static inline int sc64_p10_pi_decode(RaceSave *save,const uint8_t *tail,unsigned tail_len,
                                      Sc64ImpactEvent *candidate,int *count)
{
    uint8_t packet[SC64_P10_PI_PACKET] = {0};
    unsigned bit;
    if (!save || !tail || tail_len!=SC64_P10_PI_TAIL || !candidate || !count) return 0;
    for (unsigned n=0;n<SC64_P10_PI_BITS;n++) {
        uint8_t *p=sc64_p10_pi_side(save,n,&bit);
        unsigned v=sc64_p10_pi_get(p,bit,1);
        if (n<SC64_P10_PI_EMBED*8) sc64_p10_pi_put(packet,n,1,v);
        else if (v) return 0;
    }
    memcpy(packet+SC64_P10_PI_EMBED,tail,SC64_P10_PI_TAIL);
    unsigned n=packet[4];
    if (!n || n>SC64_P10_PI_ROWS) return 0;
    unsigned len=sc64_impact_save_bytes(n);
    for (unsigned i=len;i<SC64_P10_PI_PACKET;i++) if (packet[i]) return 0;
    int decoded=sc64_impact_decode(packet,len,SC64_P10_PI_ROWS,
                                   MAX_UNITS+MAX_BUILDINGS,5,SC_RU_COUNT,candidate);
    if (decoded<0) return 0;
    if (!sc64_p10_pi_normalize_rows(save)) return 0;
    *count = decoded;
    return 1;
}
#endif
