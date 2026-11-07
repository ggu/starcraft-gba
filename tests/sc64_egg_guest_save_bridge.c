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
int sc64_validator_fresh_envelope(u32 outer, u32 inner)
{
    if (sram_get32(host_save) != outer || sram_get32(host_save + 16) != inner ||
        sram_get32(host_save + 12) != 1) return 0;
    for (unsigned i = SAVE_SLOT_SIZE; i < sizeof(host_save); ++i)
        if (host_save[i] != 0xff) return 0;
    return 1;
}
int sc64_validator_export_sram(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(host_save, 1, sizeof(host_save), f) == sizeof(host_save);
    return fclose(f) == 0 && ok;
}
