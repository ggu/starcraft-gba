/* SPDX-License-Identifier: GPL-3.0-or-later
 * A fresh host cartridge starts erased and survives repeated game boots. */
#include <assert.h>
#include <string.h>
#include "../common/gba.c"

int main(void)
{
    static const u8 input[] = {1, 2, 3, 4};
    u8 output[sizeof(input)] = {0};
    for (unsigned i = 0; i < sizeof(host_save); ++i) assert(host_save[i] == 0xff);
    /* Staging a save before gba_init and rebooting must both preserve it. */
    save_write(input, sizeof(input), 0x12345678u);
    gba_init();
    assert(save_read(output, sizeof(output), 0x12345678u));
    assert(!memcmp(input, output, sizeof(input)));
    for (unsigned i = 16384; i < sizeof(host_save); ++i) assert(host_save[i] == 0xff);
    gba_init();
    memset(output, 0, sizeof(output));
    assert(save_read(output, sizeof(output), 0x12345678u));
    assert(!memcmp(input, output, sizeof(input)));
    return 0;
}
