/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private process-local copy only. The original guest image is opened read-only. */
#include "../common/gba.c"
#include <stdio.h>
int sc64_validator_import_sram(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t n = fread(host_save, 1, sizeof(host_save), f);
    int end = fgetc(f), failed = ferror(f);
    fclose(f);
    return n == sizeof(host_save) && end == EOF && !failed;
}
int sc64_validator_export_sram(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(host_save, 1, sizeof(host_save), f) == sizeof(host_save);
    return fclose(f) == 0 && ok;
}
