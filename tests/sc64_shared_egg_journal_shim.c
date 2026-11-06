/* SPDX-License-Identifier: GPL-3.0-or-later */
/* This translation unit exists only in the HOST journal fixture. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
void fixture_set_host_sequence(int slot,u32 sequence)
{
    assert(slot>=0 && slot<2);
    sram_put32(host_save+(unsigned)slot*SAVE_SLOT_SIZE+12u,sequence);
}
/* Private HOST-only cartridge images, never imported into a guest. */
int fixture_export_host_save(const char *path)
{
    FILE *f=fopen(path,"wbx");if(!f)return 0;
    int ok=fwrite(host_save,1,sizeof(host_save),f)==sizeof(host_save);
    return fclose(f)==0 && ok;
}
int fixture_import_host_save(const char *path)
{
    u8 data[sizeof(host_save)];FILE *f=fopen(path,"rb");if(!f)return 0;
    int ok=fread(data,1,sizeof(data),f)==sizeof(data) && fgetc(f)==EOF;
    if(fclose(f)!=0)ok=0;
    if(ok)memcpy(host_save,data,sizeof(data));
    return ok;
}
int fixture_host_save_equals(const char *path)
{
    u8 data[sizeof(host_save)];FILE *f=fopen(path,"rb");if(!f)return 0;
    int ok=fread(data,1,sizeof(data),f)==sizeof(data) && fgetc(f)==EOF &&
           !memcmp(host_save,data,sizeof(data));
    return fclose(f)==0 && ok;
}
