/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "gba_flash1m.h"
static int valid(GbaFlash1m *f,uint32_t p,uint32_t n) {
 return f && f->read && f->write && f->polls && !f->fault && p<=GBA_FLASH1M_SIZE && n<=GBA_FLASH1M_SIZE-p;
}
static void unlock(GbaFlash1m *f) {
 f->write(f->ctx,0x5555,0xaa);f->write(f->ctx,0x2aaa,0x55);
}
static void bank(GbaFlash1m *f,unsigned b) {
 unlock(f);f->write(f->ctx,0x5555,0xb0);f->write(f->ctx,0,(uint8_t)b);
}
static int wait_byte(GbaFlash1m *f,uint16_t p,uint8_t value) {
 for(uint32_t i=0;i<f->polls;++i) if(f->read(f->ctx,p)==value)return 1;
 return 0;
}
int gba_flash1m_read(GbaFlash1m *f,uint32_t p,void *dst,uint32_t n) {
 uint8_t *d=dst;if(!valid(f,p,n) || (!dst && n))return 0;
 while(n) { unsigned b=p>>16;uint32_t count=GBA_FLASH1M_BANK_SIZE-(p&65535u);if(count>n)count=n;bank(f,b);
  for(uint32_t i=0;i<count;++i)d[i]=f->read(f->ctx,(uint16_t)(p+i));
  p+=count;d+=count;n-=count;
 } bank(f,0);return 1;
}
int gba_flash1m_program(GbaFlash1m *f,uint32_t p,const void *src,uint32_t n) {
 const uint8_t *s=src;if(!valid(f,p,n) || (!src && n))return 0;
 while(n) {unsigned b=p>>16;uint32_t count=GBA_FLASH1M_BANK_SIZE-(p&65535u);if(count>n)count=n;bank(f,b);
  for(uint32_t i=0;i<count;++i) {uint16_t at=(uint16_t)(p+i);uint8_t old=f->read(f->ctx,at),v=s[i];
   if((old&v)!=v){bank(f,0);return 0;} if(old==v)continue;
   unlock(f);f->write(f->ctx,0x5555,0xa0);f->write(f->ctx,at,v);
   if(!wait_byte(f,at,v)){f->fault=1;return 0;}
  } p+=count;s+=count;n-=count;
 }bank(f,0);return 1;
}
int gba_flash1m_erase_sector(GbaFlash1m *f,uint32_t p) {
 if(!valid(f,p,GBA_FLASH1M_SECTOR_SIZE) || (p&(GBA_FLASH1M_SECTOR_SIZE-1u)))return 0;
 bank(f,p>>16);unlock(f);f->write(f->ctx,0x5555,0x80);unlock(f);f->write(f->ctx,(uint16_t)p,0x30);
 if(!wait_byte(f,(uint16_t)p,0xff)){f->fault=1;return 0;}
 for(uint32_t i=0;i<GBA_FLASH1M_SECTOR_SIZE;++i)if(f->read(f->ctx,(uint16_t)(p+i))!=0xff){bank(f,0);return 0;}
 bank(f,0);return 1;
}
