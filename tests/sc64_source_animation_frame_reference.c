/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compare the renderer's actual frame selection with the original forward
 * scan, including malformed ranges and non-monotone event timestamps. */
#define main sc64_motion_reuse_render_fixture_main
#include "sc64_motion_reuse_render.c"
#undef main

static uint16_t reference_frame(const Sc64WeaponSourceAnimation *anim,
                                uint32_t tick, uint16_t fallback)
{
    if (!anim || anim->event_first > SC64_WEAPON_SOURCE_EVENT_COUNT ||
        anim->event_count > SC64_WEAPON_SOURCE_EVENT_COUNT - anim->event_first)
        return fallback;
    if (anim->loop_pc != UINT32_MAX && anim->loop_source_tick < anim->duration_ticks &&
        tick >= anim->loop_source_tick) {
        uint32_t length = anim->duration_ticks - anim->loop_source_tick;
        if (length) tick = anim->loop_source_tick + (tick - anim->loop_source_tick) % length;
    }
    if (anim->duration_ticks && tick >= anim->duration_ticks)
        tick = anim->duration_ticks - 1u;
    for (uint32_t i = 0; i < anim->event_count; ++i) {
        const Sc64WeaponSourceEvent *event = &sc64_weapon_source_events[anim->event_first + i];
        if (event->opcode == 0 && event->source_tick <= tick) fallback = event->source_frame;
    }
    return fallback;
}

static uint32_t random_state = 1234567;
static uint32_t random_value(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state >> 8;
}

int main(void)
{
    unsigned checks = 0;
    assert(sc64_weapon_source_vfx_animation_frame(NULL, 3, 9) == 9);
    for (unsigned trial = 0; trial < 20000; ++trial) {
        for (unsigned i = 0; i < SC64_WEAPON_SOURCE_EVENT_COUNT; ++i)
            sc64_weapon_source_events[i] = (Sc64WeaponSourceEvent){
                .source_tick = random_value() % 8, .source_frame = random_value() % 20,
                .opcode = random_value() % 3};
        Sc64WeaponSourceAnimation anim = {
            .loop_pc = random_value() % 2 ? UINT32_MAX : 0,
            .loop_source_tick = random_value() % 10, .duration_ticks = random_value() % 12,
            .event_first = random_value() % 5, .event_count = random_value() % 5};
        for (uint32_t tick = 0; tick < 16; ++tick) {
            uint16_t fallback = (uint16_t)(random_value() % 30);
            assert(sc64_weapon_source_vfx_animation_frame(&anim, tick, fallback) ==
                   reference_frame(&anim, tick, fallback));
            ++checks;
        }
    }
    printf("source animation frame reference: %u comparisons passed\n", checks);
    return 0;
}
