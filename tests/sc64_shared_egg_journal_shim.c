/* SPDX-License-Identifier: GPL-3.0-or-later */
/* This translation unit exists only in the HOST journal fixture. */
#include <assert.h>
#include "../common/gba.c"
void fixture_set_host_sequence(int slot,u32 sequence)
{
    assert(slot>=0 && slot<2);
    sram_put32(host_save+(unsigned)slot*SAVE_SLOT_SIZE+12u,sequence);
}
