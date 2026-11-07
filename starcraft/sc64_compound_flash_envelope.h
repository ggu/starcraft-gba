/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_COMPOUND_FLASH_ENVELOPE_H
#define SC64_COMPOUND_FLASH_ENVELOPE_H
#include <stdint.h>
/* Fixed bounded header. The journal checksum covers header and all sections;
 * it detects corruption, not a maliciously forged record. */
enum { SC64_CF_HEADER=32, SC64_CF_LIMIT=28656,
       SC64_CF_DEBT=1, SC64_CF_CONTROLLER=2, SC64_CF_PAYER=4, SC64_CF_FLIGHT=8 };
#define SC64_CF_MAGIC 0x31464353u
#define SC64_CF_FEATURES (SC64_CF_DEBT|SC64_CF_CONTROLLER|SC64_CF_PAYER|SC64_CF_FLIGHT)
typedef struct {
    uint8_t profile,mode;uint16_t mission;
    uint32_t features,base_length,codec;
    uint16_t debts,controllers,payers,flights;
} Sc64CompoundFlashHeader;
static uint16_t sc64_cf_u16(const uint8_t *p){return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
static uint32_t sc64_cf_u32(const uint8_t *p){return (uint32_t)sc64_cf_u16(p)|((uint32_t)sc64_cf_u16(p+2)<<16);}
static void sc64_cf_put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void sc64_cf_put32(uint8_t *p,uint32_t v){sc64_cf_put16(p,(uint16_t)v);sc64_cf_put16(p+2,(uint16_t)(v>>16));}
/* Exact supported tuple comes from the caller's authenticated product context.
 * No in-payload field can expand this set. */
static int sc64_cf_parse(const uint8_t *p,uint32_t length,const Sc64CompoundFlashHeader *expected,
                         Sc64CompoundFlashHeader *out)
{
    Sc64CompoundFlashHeader v;uint32_t total;
    if(!p || !expected || !out || length<SC64_CF_HEADER || length>SC64_CF_LIMIT ||
       sc64_cf_u32(p)!=SC64_CF_MAGIC || sc64_cf_u16(p+4)!=2 || sc64_cf_u16(p+10))return 0;
    v.profile=p[6];v.mode=p[7];v.mission=sc64_cf_u16(p+8);v.features=sc64_cf_u32(p+12);
    v.base_length=sc64_cf_u32(p+16);v.debts=sc64_cf_u16(p+20);v.controllers=sc64_cf_u16(p+22);
    v.payers=sc64_cf_u16(p+24);v.flights=sc64_cf_u16(p+26);v.codec=sc64_cf_u32(p+28);
    if(v.profile!=expected->profile || v.mode!=expected->mode || v.mission!=expected->mission ||
       v.features!=expected->features || (v.features&~SC64_CF_FEATURES) || v.codec!=expected->codec ||
       v.debts!=expected->debts || v.controllers!=expected->controllers || v.payers!=expected->payers || v.flights!=expected->flights ||
       !v.base_length || v.base_length>SC64_CF_LIMIT-SC64_CF_HEADER) return 0;
    total=SC64_CF_HEADER+v.base_length;
    if(v.debts>SC64_CF_LIMIT-total)return 0;
    total+=v.debts;
    if(v.controllers>SC64_CF_LIMIT-total)return 0;
    total+=v.controllers;
    if(v.payers>SC64_CF_LIMIT-total)return 0;
    total+=v.payers;
    if(v.flights>SC64_CF_LIMIT-total)return 0;
    total+=v.flights;
    if(total!=length)return 0;
    *out=v;return 1;
}
static void sc64_cf_write(uint8_t p[SC64_CF_HEADER],const Sc64CompoundFlashHeader *v)
{
    for(unsigned i=0;i<SC64_CF_HEADER;++i)p[i]=0;
    sc64_cf_put32(p,SC64_CF_MAGIC);sc64_cf_put16(p+4,2);p[6]=v->profile;p[7]=v->mode;
    sc64_cf_put16(p+8,v->mission);sc64_cf_put32(p+12,v->features);sc64_cf_put32(p+16,v->base_length);
    sc64_cf_put16(p+20,v->debts);sc64_cf_put16(p+22,v->controllers);sc64_cf_put16(p+24,v->payers);sc64_cf_put16(p+26,v->flights);sc64_cf_put32(p+28,v->codec);
}
#endif
