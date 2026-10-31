/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private fixture: exposes only the host backend's in-memory SRAM for a
 * separate-process round trip. This file is not part of the GBA product. */
#include "../common/gba.c"
#include <stdio.h>
/* Fixture-only: replace one existing envelope while preserving its sequence. */
int sc64_test_replace_payload(u32 tag,const void *data,u32 len)
{
    int selected=-1;u32 sequence=0;
    for (int slot=0;slot<2;slot++) {
        volatile u8 *p=host_save+(unsigned)slot*SAVE_SLOT_SIZE;
        if (sram_get32(p)!=tag || sram_get32(p+4)!=len) continue;
        if (selected<0 || (s32)(sram_get32(p+12)-sequence)>0) { selected=slot;sequence=sram_get32(p+12); }
    }
    if (selected>=0) {
        volatile u8 *p=host_save+(unsigned)selected*SAVE_SLOT_SIZE;
        const u8 *src=data;
        for (u32 i=0;i<len;i++) p[16+i]=src[i];
        sram_put32(p+8,hash_bytes(src,len));
        return 1;
    }
    return 0;
}
int sc64_test_remove_tag(u32 tag)
{
    for (int slot=0;slot<2;slot++) {
        volatile u8 *p=host_save+(unsigned)slot*SAVE_SLOT_SIZE;
        if (sram_get32(p)==tag) { sram_put32(p,0); return 1; }
    }
    return 0;
}
int sc64_test_remove_sequence(u32 sequence)
{
    for (int slot=0;slot<2;slot++) {
        volatile u8 *p=host_save+(unsigned)slot*SAVE_SLOT_SIZE;
        if (sram_get32(p+12)==sequence) { sram_put32(p,0); return 1; }
    }
    return 0;
}
int sc64_test_set_sequence(u32 tag,u32 sequence)
{
    for (int slot=0;slot<2;slot++) {
        volatile u8 *p=host_save+(unsigned)slot*SAVE_SLOT_SIZE;
        if (sram_get32(p)==tag) { sram_put32(p+12,sequence); return 1; }
    }
    return 0;
}
int sc64_test_sram_transfer(const char *path,int write_mode)
{
    FILE *f=fopen(path,write_mode?"wb":"rb"); size_t n;
    if (!f) return 0;
    n=write_mode?fwrite(host_save,1,sizeof host_save,f):fread(host_save,1,sizeof host_save,f);
    if (fclose(f)) return 0;
    return n==sizeof host_save;
}
