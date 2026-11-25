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
static inline unsigned sc64_p10_pi_get(const uint8_t *p,unsigned at,unsigned n) { unsigned x=0; for(unsigned i=0;i<n;i++) x|=((p[(at+i)/8]>>((at+i)%8))&1u)<<i; return x; }
static inline void sc64_p10_pi_put(uint8_t *p,unsigned at,unsigned n,unsigned x) { for(unsigned i=0;i<n;i++) { unsigned k=at+i; p[k/8]=(uint8_t)((p[k/8]&~(1u<<(k%8)))|(((x>>i)&1u)<<(k%8))); } }
/* Sideband bits: unit 222..223; building 193..194. Research 195..199 unchanged. */
static inline int sc64_p10_pi_unit(uint8_t out[28],const uint8_t in[28],int encode) {
 uint8_t t[28]={0}; unsigned o,tm,rt,rem,v, a=0,b=0;
 if(encode) {
  o=sc64_p10_pi_get(in,40,3);tm=sc64_p10_pi_get(in,146,5);rt=sc64_p10_pi_get(in,151,4);rem=sc64_p10_pi_get(in,212,11);
  if(o>=5||tm>=25||rt>=13||rem>=1280)return 0;
  v=(((o*25+tm)*13+rt)*1280+rem);
  for(a=0;a<224;) {
   if(a==40){sc64_p10_pi_put(t,b,21,v);b+=21;a+=3;}
   else if(a==146)a+=5; else if(a==151)a+=4; else if(a==212)a+=11;
   else {sc64_p10_pi_put(t,b++,1,sc64_p10_pi_get(in,a++,1));}
  }
  if (b!=222) return 0;
 } else {
  v=sc64_p10_pi_get(in,40,21);if(v>=2080000)return 0;
  rem=v%1280;v/=1280;rt=v%13;v/=13;tm=v%25;v/=25;o=v;
  for(a=0;a<224;) {
   if(a==40){sc64_p10_pi_put(t,a,3,o);a+=3;b+=21;}
   else if(a==146){sc64_p10_pi_put(t,a,5,tm);a+=5;} else if(a==151){sc64_p10_pi_put(t,a,4,rt);a+=4;} else if(a==212){sc64_p10_pi_put(t,a,11,rem);a+=11;}
   else {sc64_p10_pi_put(t,a++,1,sc64_p10_pi_get(in,b++,1));}
  }
  if (b!=222) return 0;
 }
 memcpy(out,t,28);return 1;
}
static inline int sc64_p10_pi_building(uint8_t out[25],const uint8_t in[25],int encode) {
 uint8_t t[25]={0};unsigned o,rem,head,count,v,w,a=0,b=0;
 if(encode) {
  o=sc64_p10_pi_get(in,40,3);rem=sc64_p10_pi_get(in,182,11);head=sc64_p10_pi_get(in,80,3);count=sc64_p10_pi_get(in,83,3);
  if(o>=5||rem>=1280||head>=5||count>=6)return 0;
  v=o*1280+rem;w=head*6+count;
  for(a=0;a<195;) {
   if(a==40){sc64_p10_pi_put(t,b,13,v);b+=13;a+=3;}
   else if(a==80){sc64_p10_pi_put(t,b,5,w);b+=5;a+=3;}
   else if(a==83)a+=3;else if(a==182)a+=11;
   else {sc64_p10_pi_put(t,b++,1,sc64_p10_pi_get(in,a++,1));}
  }
  if (b!=193) return 0;
 } else {
  v=sc64_p10_pi_get(in,40,13);w=sc64_p10_pi_get(in,90,5);if(v>=6400||w>=30)return 0;
  o=v/1280;rem=v%1280;head=w/6;count=w%6;
  for(a=0;a<195;) {
   if(a==40){sc64_p10_pi_put(t,a,3,o);a+=3;b+=13;}
   else if(a==80){sc64_p10_pi_put(t,a,3,head);a+=3;b+=5;}
   else if(a==83){sc64_p10_pi_put(t,a,3,count);a+=3;}
   else if(a==182){sc64_p10_pi_put(t,a,11,rem);a+=11;}
   else {sc64_p10_pi_put(t,a++,1,sc64_p10_pi_get(in,b++,1));}
  }
  if (b!=193) return 0;
 }
 sc64_p10_pi_put(t,195,5,sc64_p10_pi_get(in,195,5));memcpy(out,t,25);return 1;
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
    uint8_t packet[SC64_P10_PI_PACKET]={0}, tmp[28]; unsigned bit;
    if (!save || !tail || !rows || !count || count>SC64_P10_PI_ROWS) return 0;
    for (unsigned i=0;i<count;i++)
        if (!sc64_impact_event_valid(&rows[i],MAX_UNITS+MAX_BUILDINGS,5,SC_RU_COUNT)) return 0;
    for (unsigned i=0;i<MAX_UNITS;i++)
        if (!sc64_p10_pi_unit(tmp,save->units[i].bytes,1)) return 0;
    for (unsigned i=0;i<MAX_BUILDINGS;i++) {
        if (!sc64_p10_pi_building(tmp,save->buildings[i].bytes,1)) return 0;
        for (unsigned j=0;j<5;j++)
            if (i*5+j>=96 && sc64_p10_pi_get(save->buildings[i].bytes,195+j,1)) return 0;
    }
    sc64_impact_encode(rows,count,packet);
    for (unsigned i=0;i<MAX_UNITS;i++) {
        if (!sc64_p10_pi_unit(tmp,save->units[i].bytes,1)) return 0;
        memcpy(save->units[i].bytes,tmp,28);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;i++) {
        if (!sc64_p10_pi_building(tmp,save->buildings[i].bytes,1)) return 0;
        memcpy(save->buildings[i].bytes,tmp,25);
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
    uint8_t tmp[28]; unsigned bit;
    if (!save) return 0;
    for (unsigned n=0;n<SC64_P10_PI_BITS;n++) {
        uint8_t *p=sc64_p10_pi_side(save,n,&bit);
        sc64_p10_pi_put(p,bit,1,0);
    }
    for (unsigned i=0;i<MAX_UNITS;i++) {
        if (!sc64_p10_pi_unit(tmp,save->units[i].bytes,0)) return 0;
        memcpy(save->units[i].bytes,tmp,28);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;i++) {
        if (!sc64_p10_pi_building(tmp,save->buildings[i].bytes,0)) return 0;
        memcpy(save->buildings[i].bytes,tmp,25);
    }
    return 1;
}
/* Extract packet and normalize detached rows. Research bits are untouched.
 * The caller must subsequently validate the ordinary world and research.
 * Tail length is exact; unused sideband and packet bytes must be zero. */
static inline int sc64_p10_pi_decode(RaceSave *save,const uint8_t *tail,unsigned tail_len,
                                      Sc64ImpactEvent *candidate,int *count)
{
    uint8_t packet[SC64_P10_PI_PACKET]={0}; unsigned bit;
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
    *count=decoded; return 1;
}
#endif
