/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba_flash1m.h"
typedef struct { uint8_t bytes[131072];unsigned bank,state,cmd,busy,stuck,bad,stall_program; } Model;
static uint8_t rd(void *c,uint16_t p) { Model *m=c;if(m->stuck)return 0x5f;if(m->busy){--m->busy;return 0x5f;}return m->bytes[m->bank*65536u+p]; }
static void wr(void *c,uint16_t p,uint8_t v) {
 Model *m=c;if(m->stuck||m->busy){m->bad++;return;}
 if(m->state==0 && m->cmd==0xa0){unsigned a=m->bank*65536u+p;assert((m->bytes[a]&v)==v);m->bytes[a]&=v;m->cmd=0;if(m->stall_program)m->stuck=1;else m->busy=2;return;}
 if(m->state==0 && m->cmd==0xb0){if(p==0&&v<2)m->bank=v;else m->bad++;m->cmd=0;return;}
 if(m->state==0){if(p==0x5555&&v==0xaa)m->state=1;else m->bad++;return;}
 if(m->state==1){if(p==0x2aaa&&v==0x55)m->state=2;else{m->bad++;m->state=0;}return;}
 m->state=0;if(m->cmd==0x80 && v==0x30 && !(p&4095u)) {memset(m->bytes+m->bank*65536u+p,0xff,4096);m->cmd=0;m->busy=3;return;}
 if(p==0x5555 && (v==0x80||v==0xa0||v==0xb0)){m->cmd=v;return;}
 m->bad++;
}
int main(void) {
 static Model m;memset(m.bytes,0xff,sizeof(m.bytes));GbaFlash1m f={&m,rd,wr,20,0};
 uint8_t v[4]={0x12,0x34,0x56,0x78},r[4]={0};
 assert(gba_flash1m_program(&f,65534,v,4));assert(m.bank==0 && !m.bad);
 assert(gba_flash1m_read(&f,65534,r,4)&&!memcmp(r,v,4)&&m.bank==0);
 assert(!gba_flash1m_program(&f,65534,"\xff",1));assert(m.bytes[65534]==0x12 && m.bank==0);
 assert(gba_flash1m_erase_sector(&f,61440));assert(m.bytes[65534]==0xff && m.bytes[65536]==0x56 && m.bank==0);
 assert(!gba_flash1m_erase_sector(&f,1));assert(!gba_flash1m_read(&f,131071,r,2));assert(!gba_flash1m_program(&f,131072,v,1));
 m.stall_program=1;unsigned prev=m.bad;assert(!gba_flash1m_program(&f,0,v,1));assert(m.bad==prev && f.fault); assert(!gba_flash1m_read(&f,0,r,1)); assert(m.bad==prev); /* no commands issued after timeout */
 puts("flash bank crossing, erase isolation, 1-to-0, bounds and timeout passed");return 0;
}
