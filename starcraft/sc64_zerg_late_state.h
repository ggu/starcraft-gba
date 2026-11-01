/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Compact persistent form of the Z9/Z10 scheduler, AI and crystal state.
 *
 * As with Z8 (sc64_z8_state.h), everything derivable from the immutable
 * program and the mission-start setup is rebuilt on load. Only mutable
 * fields are stored, and a restored image must pass sc64_zl_state_valid()
 * before it is used. The record fits the shared Zerg mission save union.
 */
#ifndef SC64_ZERG_LATE_STATE_H
#define SC64_ZERG_LATE_STATE_H

#include "sc64_zerg_late_triggers.h"
#include "sc64_z8_state.h" /* sc64_z8_rect_is_source() */

enum {
    /* One byte per instance: flags in the low nibble, cursor in the high. */
    SC64_ZL_SAVE_INSTANCES = 16,
    SC64_ZL_SAVE_MOVED_LOCATIONS = 2,
    SC64_ZL_AI_TOWNS = 3,
    SC64_ZL_SAVE_AI_BYTES = 24,
    SC64_ZL_NO_CARRIER = 0xff
};

typedef struct {
    u32 frames, passes, elapsed_seconds, countdown;
    u32 wait_ms[SC64_Z8_OWNER_LIMIT];
    u8 switches[SC64_Z8_SWITCH_BYTES];
    u8 instance[SC64_ZL_SAVE_INSTANCES];
    u8 outcome[SC64_Z8_OWNER_LIMIT];
    u8 waiting_mask, pass_counter, second_counter;
    /* Unit slot of the Drone carrying the Khaydarin Crystal, or NO_CARRIER. */
    u8 crystal_carrier;
    Sc64Z8Rect moved[SC64_ZL_SAVE_MOVED_LOCATIONS];
    /* Source wallets of the three scripted Protoss towns. */
    u16 ai_minerals[SC64_ZL_AI_TOWNS], ai_gas[SC64_ZL_AI_TOWNS];
    /* Authored town policy state; see sc64_zerg_late_ai.h. */
    u8 ai[SC64_ZL_SAVE_AI_BYTES];
} Sc64ZlPersistentState;

_Static_assert(sizeof(Sc64ZlPersistentState) == 160,
               "late Zerg save state fits the shared Zerg payload");

/* MoveLocation destinations (group2) are the only locations whose rectangle
 * can differ from the source. Collect them in ascending location order. */
static inline unsigned
sc64_zl_moved_locations(const Sc64Z8Program *p, u8 out[SC64_ZL_SAVE_MOVED_LOCATIONS])
{
    unsigned n = 0;
    for (unsigned loc = 1; p && loc <= p->location_count; ++loc) {
        int used = 0;
        for (unsigned i = 0; i < p->action_count && !used; ++i)
            used = p->actions[i].opcode == 38u && p->actions[i].group2 == loc;
        if (!used) continue;
        if (n >= SC64_ZL_SAVE_MOVED_LOCATIONS) return 0xffu;
        out[n++] = (u8)loc;
    }
    return n;
}

/* Pack a valid live state. Fails on any field the compact form cannot hold.
 * The AI bytes, wallets and carrier are owned by the caller. */
static inline int
sc64_zl_state_pack(Sc64ZlPersistentState *out, const Sc64ZlSchedulerState *zs,
                   const Sc64ZlProgram *zp)
{
    const Sc64Z8SchedulerState *s = zs ? &zs->core : 0;
    const Sc64Z8Program *p = zp ? &zp->rows : 0;
    u8 moved[SC64_ZL_SAVE_MOVED_LOCATIONS];
    unsigned moved_count = sc64_zl_moved_locations(p, moved);
    if (!out || !sc64_zl_state_valid(zs, zp) || moved_count == 0xffu ||
        s->instance_count > SC64_ZL_SAVE_INSTANCES) return 0;
    for (unsigned i = 0; i < s->instance_count; ++i)
        if (s->instances[i].cursor > 15u) return 0;
    for (unsigned loc = 1; loc <= p->location_count; ++loc) {
        int mutable_location = 0;
        for (unsigned m = 0; m < moved_count; ++m) mutable_location |= moved[m] == loc;
        if (!mutable_location &&
            !sc64_z8_rect_is_source(&s->locations[loc - 1], &p->locations[loc - 1]))
            return 0;
    }
    out->frames = s->frames;
    out->passes = s->passes;
    out->elapsed_seconds = s->elapsed_seconds;
    out->countdown = zs->countdown;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        out->wait_ms[o] = s->wait_ms[o];
        out->outcome[o] = s->outcome[o];
    }
    for (unsigned i = 0; i < SC64_Z8_SWITCH_BYTES; ++i) out->switches[i] = s->switches[i];
    for (unsigned i = 0; i < SC64_ZL_SAVE_INSTANCES; ++i)
        out->instance[i] = i < s->instance_count ?
            (u8)(s->instances[i].flags | (s->instances[i].cursor << 4)) : 0u;
    out->waiting_mask = s->waiting_mask;
    out->pass_counter = s->pass_counter;
    out->second_counter = s->second_counter;
    for (unsigned m = 0; m < SC64_ZL_SAVE_MOVED_LOCATIONS; ++m)
        out->moved[m] = m < moved_count ? s->locations[moved[m] - 1] : (Sc64Z8Rect){0};
    return 1;
}

/* Rebuild a full scheduler image in caller-provided scratch. `out` changes
 * only if the result validates; it may be null to validate only. */
static inline int
sc64_zl_state_unpack(Sc64ZlSchedulerState *out, const Sc64ZlPersistentState *in,
                     const Sc64ZlProgram *zp, const Sc64Z8Setup *setup,
                     Sc64ZlSchedulerState *scratch)
{
    Sc64ZlSchedulerState *const image = scratch;
    const Sc64Z8Program *p = zp ? &zp->rows : 0;
    u8 moved[SC64_ZL_SAVE_MOVED_LOCATIONS];
    unsigned moved_count = sc64_zl_moved_locations(p, moved);
    if (!image || image == out || !in || moved_count == 0xffu ||
        !sc64_zl_init(image, zp, setup) ||
        image->core.instance_count > SC64_ZL_SAVE_INSTANCES) return 0;
    for (unsigned i = image->core.instance_count; i < SC64_ZL_SAVE_INSTANCES; ++i)
        if (in->instance[i]) return 0;
    for (unsigned m = moved_count; m < SC64_ZL_SAVE_MOVED_LOCATIONS; ++m) {
        const Sc64Z8Rect *r = &in->moved[m];
        if (r->left || r->top || r->right || r->bottom) return 0;
    }
    image->core.frames = in->frames;
    image->core.passes = in->passes;
    image->core.elapsed_seconds = in->elapsed_seconds;
    image->countdown = in->countdown;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        image->core.wait_ms[o] = in->wait_ms[o];
        image->core.outcome[o] = in->outcome[o];
    }
    for (unsigned i = 0; i < SC64_Z8_SWITCH_BYTES; ++i) image->core.switches[i] = in->switches[i];
    for (unsigned i = 0; i < image->core.instance_count; ++i) {
        image->core.instances[i].flags = in->instance[i] & 0x0fu;
        image->core.instances[i].cursor = in->instance[i] >> 4;
    }
    image->core.waiting_mask = in->waiting_mask;
    image->core.pass_counter = in->pass_counter;
    image->core.second_counter = in->second_counter;
    for (unsigned m = 0; m < moved_count; ++m)
        image->core.locations[moved[m] - 1] = in->moved[m];
    if (!sc64_zl_state_valid(image, zp)) return 0;
    if (out) *out = *image;
    return 1;
}

#endif /* SC64_ZERG_LATE_STATE_H */
