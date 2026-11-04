/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only corruption helpers. Never export or seed guest SRAM. */
#include "../common/gba.c"
#include <string.h>
void guardian_test_snapshot(void *dst) { memcpy(dst,host_save,sizeof(host_save)); }
void guardian_test_restore(const void *src) { memcpy(host_save,src,sizeof(host_save)); }
void guardian_test_retag(int slot,u32 tag,u32 inner)
{
    volatile u8 *p=host_save+(unsigned)slot*SAVE_SLOT_SIZE;
    u32 len=sram_get32(p+4);sram_put32(p+16,inner);
    sram_put32(p+8,hash_bytes((const u8 *)p+16,len));sram_put32(p,tag);
}
void guardian_test_sequence(int slot,u32 seq) { sram_put32(host_save+(unsigned)slot*SAVE_SLOT_SIZE+12,seq); }
void guardian_test_corrupt(int slot) { host_save[(unsigned)slot*SAVE_SLOT_SIZE+16]^=1; }
