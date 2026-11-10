/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GBA_FLASH_AUDIO_TRANSACTION_H
#define GBA_FLASH_AUDIO_TRANSACTION_H
#include <stdint.h>
typedef struct {
 void *context;
 uint16_t (*read16)(void *,uint32_t);
 void (*write16)(void *,uint32_t,uint16_t);
 void (*write32)(void *,uint32_t,uint32_t);
 uint32_t mix,music;
 uint16_t timer_reload,timer_control,sound;
 unsigned imported,active;
} GbaFlashAudioTransaction;
static int gba_flash_audio_stop(GbaFlashAudioTransaction *a){
 uint16_t d1=a->read16(a->context,0x040000c6),d2=a->read16(a->context,0x040000d2);
 a->active=0;
 if(a->imported && d1==0xb640 && d2==0xb640 && a->read16(a->context,0x04000102)==0x0080){
  a->active=1;a->timer_control=a->read16(a->context,0x04000102);a->sound=a->read16(a->context,0x04000082);
  a->write16(a->context,0x04000102,0);a->write32(a->context,0x040000c4,0);a->write32(a->context,0x040000d0,0);return 1;
 }
 return !(d1&0x8000) && !(d2&0x8000);
}
static void gba_flash_audio_restore(GbaFlashAudioTransaction *a){
 if(!a->active)return;
 a->write16(a->context,0x04000082,a->sound|0x8800);
 a->write32(a->context,0x040000bc,a->mix);a->write32(a->context,0x040000c4,0xb6400000u);
 a->write32(a->context,0x040000c8,a->music);a->write32(a->context,0x040000d0,0xb6400000u);
 a->write16(a->context,0x04000100,a->timer_reload);a->write16(a->context,0x04000102,a->timer_control);a->active=0;
}
#endif
