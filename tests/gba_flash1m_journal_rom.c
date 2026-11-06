/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Standalone experiment, not a game save backend. */
#include "../common/gba_flash1m.h"
#include "../common/gba_flash_journal.h"
__attribute__((used,section(".rodata.gba_save_signature")))
const char gba_save_signature[]="FLASH1M_V103";
volatile unsigned flash_journal_status;
__attribute__((noinline)) void flash_journal_done(void) { __asm__ volatile ("" ::: "memory"); }
static uint8_t rd(void *c,uint16_t p){(void)c;return ((volatile uint8_t*)0x0e000000)[p];}
static void wr(void *c,uint16_t p,uint8_t v){(void)c;((volatile uint8_t*)0x0e000000)[p]=v;}
static int read_io(void *c,uint32_t p,void *d,uint32_t n){return gba_flash1m_read(c,p,d,n);}
static int write_io(void *c,uint32_t p,const void *d,uint32_t n){return gba_flash1m_program(c,p,d,n);}
static int erase_io(void *c,uint32_t p){return gba_flash1m_erase_sector(c,p);}
static void stop(unsigned s){flash_journal_status=s;flash_journal_done();for(;;);}
static uint8_t payload[GBA_FJ_PAYLOAD_MAX];
int main(void) {
 /* Pinned mGBA's fixed 650-cycle busy byte needs a test-model delay. Not a
  * physical Flash timing assertion. */
 GbaFlash1m flash={0,rd,wr,500000,0,256};GbaFlashJournalIo io={&flash,read_io,erase_io,write_io};
 GbaFlashJournalRecord previous,one,two;int result;
 for(unsigned i=0;i<sizeof payload;++i)payload[i]=0x5f;
 result=gba_fj_select(&io,0,0,&previous);if(result!=GBA_FJ_OK)stop(1);
#ifndef FLASH_JOURNAL_READ_ONLY
 if(previous.index>=2)stop(2);
 if(gba_fj_write_parts(&io,payload,sizeof payload,0,0,0x56465a60,0,0,&previous,&one)!=GBA_FJ_OK)stop(3);
 if(gba_fj_write_parts(&io,payload,sizeof payload,0,0,0x56465a60,0,0,&one,&two)!=GBA_FJ_OK)stop(4);
 if(two.index!=3)stop(5);
#else
 (void)one;(void)two;
 if(previous.index!=3 || previous.length!=sizeof payload || previous.tag!=0x56465a60)stop(6);
 uint8_t chunk[64];for(unsigned i=0;i<sizeof payload;i+=sizeof chunk) {
  unsigned n=sizeof payload-i;if(n>sizeof chunk)n=sizeof chunk;
  if(gba_fj_read_payload(&io,&previous,i,chunk,n)!=GBA_FJ_OK)stop(7);
  for(unsigned j=0;j<n;++j)if(chunk[j]!=0x5f)stop(8);
 }
#endif
 stop(0x600d);return 0;
}
