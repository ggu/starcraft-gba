/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Strict command-level model + actual transport + journal. */
#define main flash_model_existing_main
#include "gba_flash1m_model.c"
#undef main
#include "../common/gba_flash_journal.h"
static int tr_read(void *c,uint32_t p,void *out,uint32_t n)
{return gba_flash1m_read(c,p,out,n);}
static int tr_program(void *c,uint32_t p,const void *in,uint32_t n)
{return gba_flash1m_program(c,p,in,n);}
static int tr_erase(void *c,uint32_t p)
{return gba_flash1m_erase_sector(c,p);}
int main(void) {
 static Model model;static uint8_t legacy[32768],payload[GBA_FJ_PAYLOAD_MAX];
 memset(model.bytes,0xff,sizeof model.bytes);memset(payload,0x5f,sizeof payload);
 /* Real journal encoding, with an old-layout predecessor. */
 uint8_t *h=model.bytes;gba_fj_put32(h,0x56435a60);gba_fj_put32(h+4,32);
 memset(h+16,0x31,32);gba_fj_put32(h+8,gba_fj_hash(2166136261u,h+16,32));gba_fj_put32(h+12,42);
 memcpy(legacy,model.bytes,sizeof legacy);
 GbaFlash1m flash={&model,rd,wr,20,0,0};GbaFlashJournalIo io={&flash,tr_read,tr_erase,tr_program};
 GbaFlashJournalRecord old,a,b,winner;
 assert(gba_fj_select(&io,0,0,&old)==GBA_FJ_OK && old.index==0);
 assert(gba_fj_write_parts(&io,payload,sizeof payload,0,0,0x56465a60,0,0,&old,&a)==GBA_FJ_OK);
 assert(a.index==2 && a.length==sizeof payload && !memcmp(model.bytes,legacy,sizeof legacy));
 assert(gba_fj_write_parts(&io,payload,sizeof payload,0,0,0x56465a60,0,0,&a,&b)==GBA_FJ_OK);
 assert(b.index==3 && gba_fj_select(&io,0,0,&winner)==GBA_FJ_OK && gba_fj_same_record(&b,&winner));
 assert(!memcmp(model.bytes,legacy,sizeof legacy) && !model.bad && !flash.fault);
 puts("strict Flash transport + full journal: two slots and legacy preserved PASS");return 0;
}
