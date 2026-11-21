/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    death_vfx_reset();
    death_vfx_test_storage_guards_init();
    for (unsigned layer = 0; layer <= 31; ++layer) {
        for (unsigned source = 0; source <= 1; ++source) {
            for (unsigned color = 0; color <= 255; color += 255) {
                Sc64DeathVfx effect = {
                    .x = -11, .y = 257, .birth = 0x12345678u, .raw = 42,
                    .burst_su = 27, .remnant_start = 27, .total_su = 27,
                    .style = SC64_DEATH_VFX_ZERG_AIR, .extent = 16,
                    .source = source, .actor_layer_plus1 = layer, .color = color
                };
                for (int slot = 0; slot < SC64_DEATH_VFX_POOL; slot += SC64_DEATH_VFX_POOL - 1) {
                    death_vfx_test_store_row(slot, &effect);
                    Sc64DeathVfx out = { .actor_layer_plus1 = layer ^ 31u };
                    death_vfx_test_load_row(slot, &out);
                    assert(out.actor_layer_plus1 == layer);
                    assert(out.source == source && out.color == color);
                    assert(out.x == effect.x && out.y == effect.y && out.birth == effect.birth);
                    assert(out.raw == effect.raw && out.style == effect.style && out.extent == effect.extent);
                    assert(out.burst_su == effect.burst_su && out.remnant_start == effect.remnant_start &&
                           out.total_su == effect.total_su);
                    assert(SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_ROW_WORDS + 9] == 0);
                    assert(death_vfx_test_storage_guards_ok());
                }
            }
        }
    }
    puts("PASS VRAM death-effect rows preserve every depth, source and color boundary");
    return 0;
}
