/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only journal inspection and file transfer; never copy to guest SRAM. */
#include "sc64_guardian_journal_shim.c"
#include <stdio.h>
int sc64_test_sram_transfer(const char *path,int write_mode)
{
    FILE *f=fopen(path,write_mode?"wb":"rb");size_t n;
    if (!f) return 0;
    n=write_mode?fwrite(host_save,1,sizeof(host_save),f):fread(host_save,1,sizeof(host_save),f);
    return fclose(f)==0 && n==sizeof(host_save);
}
