/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private fixture: exposes only the host backend's in-memory SRAM for a
 * separate-process round trip. This file is not part of the GBA product. */
#include "../common/gba.c"
#include <stdio.h>
int sc64_test_sram_transfer(const char *path,int write_mode)
{
    FILE *f=fopen(path,write_mode?"wb":"rb"); size_t n;
    if (!f) return 0;
    n=write_mode?fwrite(host_save,1,sizeof host_save,f):fread(host_save,1,sizeof host_save,f);
    if (fclose(f)) return 0;
    return n==sizeof host_save;
}
