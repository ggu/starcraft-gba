/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../common/gba_flash1m.h"
__attribute__((used,section(".rodata.gba_save_signature")))
const char gba_save_signature[]="FLASH1M_V103";
volatile unsigned flash_test_status;
__attribute__((noinline)) void flash_test_done(void) { __asm__ volatile ("" ::: "memory"); }
static uint8_t mmio_read(void *unused,uint16_t p) { (void)unused;return ((volatile uint8_t *)0x0e000000)[p]; }
static void mmio_write(void *unused,uint16_t p,uint8_t v) { (void)unused;((volatile uint8_t *)0x0e000000)[p]=v; }
int main(void) {
 GbaFlash1m f={0,mmio_read,mmio_write,500000,0,256};
 static const uint8_t mark[4]={0x5f,0x34,0x56,0x78};uint8_t got[4];
 flash_test_status=1;
#ifndef FLASH_READ_ONLY
 if(!gba_flash1m_erase_sector(&f,61440)){flash_test_status=2;for(;;);}
 if(!gba_flash1m_erase_sector(&f,65536)){flash_test_status=3;for(;;);}
 if(!gba_flash1m_program(&f,65534,mark,4)){flash_test_status=4;for(;;);}
#endif
 if(!gba_flash1m_read(&f,65534,got,4)){flash_test_status=5;for(;;);}
 for(int i=0;i<4;++i)if(got[i]!=mark[i]){flash_test_status=6;for(;;);}
 flash_test_status=0x600d;flash_test_done();for(;;);
}
