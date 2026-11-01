/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Prepared Z7 production flags use uncached tracer lifetimes. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

#ifdef SC64_TRACE_DURATION_CACHE
#error "Z7 lifecycle fixture must exercise the production uncached path"
#endif

int main(void)
{
    unsigned checked = 0, long_checked = 0;
    for (unsigned n = 0; n < SC64_WEAPON_SOURCE_PROFILE_COUNT; ++n) {
        const Sc64WeaponSourceAttackProfile *p = &sc64_weapon_source_profiles[n];
        Sc64WeaponSourceVfxKey key = {.actor_raw=p->actor_raw, .weapon_id=p->weapon_id,
                                     .target_air=p->mode, .variant=p->variant, .direction=8};
        uint32_t packed;
        if (!sc64_weapon_source_vfx_pack(key, &packed)) continue;
        int expected = sc64_trace_duration((int)packed, 100, 100, 160, 100);
        if (expected <= 0) continue;
        clear_tracers();
        add_tracer(100, 100, 160, 100, (int)packed);
        assert(tracers[0].life == expected + 1);
        for (int tick = 0; tick < expected; ++tick) {
            assert(sc64_tracer_duration(0) == expected);
            update_tracers();
        }
        assert(tracers[0].life == 1);
        update_tracers();
        assert(tracers[0].life == 0);
        ++checked;
        if (!long_checked) {
            expected = sc64_trace_duration((int)packed, 100, 100, 30000, 100);
            if (expected > 255 && expected <= 65535) {
                clear_tracers();
                add_tracer(100, 100, 30000, 100, (int)packed);
                assert(tracers[0].life == expected + 1);
                for (int tick = 0; tick < expected; ++tick) {
                    assert(sc64_tracer_duration(0) == expected);
                    update_tracers();
                }
                assert(tracers[0].life == 1);
                update_tracers();
                assert(tracers[0].life == 0);
                ++long_checked;
            }
        }
    }
    assert(checked && long_checked);
    printf("Z7 uncached tracer lifecycle: %u source profiles, %u long duration\n",
           checked, long_checked);
    return 0;
}
