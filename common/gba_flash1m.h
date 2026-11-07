/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GBA_FLASH1M_H
#define GBA_FLASH1M_H
#include <stdint.h>
#define GBA_FLASH1M_SIZE 131072u
#define GBA_FLASH1M_BANK_SIZE 65536u
#define GBA_FLASH1M_SECTOR_SIZE 4096u
/* Prototype transport: callbacks access one 64 KiB flash window. Operations
 * return failure on bounds or timeout; no caller may assume rollback. */
typedef struct {
 void *ctx;
 uint8_t (*read)(void *,uint16_t);
 void (*write)(void *,uint16_t,uint8_t);
 uint32_t polls;
 unsigned fault; /* sticky after an uncertain timeout; no automatic reset */
 uint32_t settle_reads; /* model-specific delay; zero for ordinary DQ6 devices */
} GbaFlash1m;
int gba_flash1m_read(GbaFlash1m *,uint32_t,void *,uint32_t);
int gba_flash1m_program(GbaFlash1m *,uint32_t,const void *,uint32_t);
int gba_flash1m_erase_sector(GbaFlash1m *,uint32_t);
#endif
