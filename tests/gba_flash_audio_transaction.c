/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include "../common/gba_flash_audio_transaction.h"
typedef struct {uint16_t d1,d2,timer,sound,reload;uint32_t mix,music;unsigned writes;} Regs;
static uint16_t rd(void *p,uint32_t a){Regs *r=p;switch(a){case 0x040000c6:return r->d1;case 0x040000d2:return r->d2;case 0x04000102:return r->timer;case 0x04000082:return r->sound;default:assert(0);return 0;}}
static void wr16(void *p,uint32_t a,uint16_t v){Regs *r=p;++r->writes;switch(a){case 0x04000102:r->timer=v;break;case 0x04000082:r->sound=v;break;case 0x04000100:r->reload=v;break;default:assert(0);}}
static void wr32(void *p,uint32_t a,uint32_t v){Regs *r=p;++r->writes;switch(a){case 0x040000c4:r->d1=v>>16;break;case 0x040000d0:r->d2=v>>16;break;case 0x040000bc:r->mix=v;break;case 0x040000c8:r->music=v;break;default:assert(0);}}
int main(void){Regs r={0xb640,0xb640,0x80,0x3306,0,0,0,0};GbaFlashAudioTransaction a={&r,rd,wr16,wr32,0x6009600,0x60097a0,0xf9c4,0,0,1,0};
 assert(gba_flash_audio_stop(&a));assert(!r.timer&&!r.d1&&!r.d2&&a.active);gba_flash_audio_restore(&a);
 assert(r.timer==0x80&&r.d1==0xb640&&r.d2==0xb640&&r.reload==0xf9c4&&r.mix==a.mix&&r.music==a.music&&!a.active);
 a.imported=0;r.writes=0;assert(!gba_flash_audio_stop(&a));assert(!r.writes);a.imported=1;
 r.d1=0x8000;r.d2=0;r.writes=0;assert(!gba_flash_audio_stop(&a));assert(!r.writes);
 r.d1=r.d2=0;r.writes=0;assert(gba_flash_audio_stop(&a));gba_flash_audio_restore(&a);assert(!r.writes);
 return 0;}
