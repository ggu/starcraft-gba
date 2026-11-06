/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_HEADLESS_READ_BOUNDS_H
#define SC64_HEADLESS_READ_BOUNDS_H
#include <stddef.h>
#include <stdint.h>
/* Observational memory regions only. The sole permitted MMIO register is the
 * two-byte display control used to identify the visible Mode 4 page. */
static int sc64_headless_read_allowed(uint32_t address, uint32_t length, size_t rom_size)
{
    return length && ((address >= 0x04000000 && address < 0x04000002 && length <= 0x04000002 - address) ||
                      (address >= 0x05000000 && address < 0x05000400 && length <= 0x05000400 - address) ||
                      (address >= 0x06000000 && address < 0x06018000 && length <= 0x06018000 - address) ||
                      (address >= 0x02000000 && address < 0x02040000 && length <= 0x02040000 - address) ||
                      (address >= 0x03000000 && address < 0x03008000 && length <= 0x03008000 - address) ||
                      (address >= 0x08000000 && address < 0x0a000000 && length <= 0x0a000000 - address &&
                       (uint64_t)(address - 0x08000000) + length <= rom_size));
}
static int sc64_headless_write_allowed(int physical) { return !physical; }
#endif
