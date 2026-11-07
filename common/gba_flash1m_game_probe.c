/* SPDX-License-Identifier: GPL-3.0-or-later
 * Resource-only ARM bridge. No cartridge or timing qualification is implied. */
#if !defined(SC64_Q8_FLASH_ARM_PROBE) || defined(GBA_HOST)
#error "This bridge is exclusively for the explicit ARM Flash probe"
#endif
#include "gba_flash1m.h"
#include "gba_flash_journal.h"
#define REG16(a) (*(volatile uint16_t *)(a))
#define FLASH_WINDOW ((volatile uint8_t *)0x0e000000u)
static uint8_t window_read(void *c,uint16_t p) {(void)c;return FLASH_WINDOW[p];}
static void window_write(void *c,uint16_t p,uint8_t v) {(void)c;FLASH_WINDOW[p]=v;}
/* Bound is a diagnostic resource limit, not a device-specific timing claim. */
static GbaFlash1m flash={0,window_read,window_write,100000u,0,0};
static int quiescent(void) {
    /* Do not change interrupt or DMA state behind the game's back. */
    return !(REG16(0x04000208u)&1u) && !(REG16(0x040000c6u)&0x8000u) &&
           !(REG16(0x040000d2u)&0x8000u);
}
static int read_flash(void *c,uint32_t p,void *out,uint32_t n) {
    return quiescent() && gba_flash1m_read(c,p,out,n);
}
static int erase_flash(void *c,uint32_t p) {
    return p>=32768u && quiescent() && gba_flash1m_erase_sector(c,p);
}
static int program_flash(void *c,uint32_t p,const void *data,uint32_t n) {
    return p>=32768u && quiescent() && gba_flash1m_program(c,p,data,n);
}
const GbaFlashJournalIo *sc64_q8_flash_arm_io(void) {
    static const GbaFlashJournalIo io={&flash,read_flash,erase_flash,program_flash};
    return &io;
}
