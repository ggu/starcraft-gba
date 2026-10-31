/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Compact persistent form of the Z8 scheduler state.
 *
 * The live Sc64Z8SchedulerState is 876 bytes, which does not fit the shared
 * Zerg save union. Everything derivable from the immutable program and the
 * mission-start setup is rebuilt on load: the version, owner set, forces,
 * speed entry, counts, each instance's trigger and owner, and every location
 * that no MoveLocation row can change. Only mutable fields are stored, and a
 * restored image must pass sc64_z8_state_valid() before it is used.
 */
#ifndef SC64_Z8_STATE_H
#define SC64_Z8_STATE_H

#include "sc64_z8_triggers.h"

enum {
    /* One byte per instance: flags in the low nibble, cursor in the high. */
    SC64_Z8_SAVE_INSTANCES = 40,
    SC64_Z8_SAVE_MOVED_LOCATIONS = 2,
    SC64_Z8_SAVE_AI_BYTES = 20
};

typedef struct {
    u32 frames, passes, elapsed_seconds;
    u32 wait_ms[SC64_Z8_OWNER_LIMIT];
    u8 switches[SC64_Z8_SWITCH_BYTES];
    u8 instance[SC64_Z8_SAVE_INSTANCES];
    u8 outcome[SC64_Z8_OWNER_LIMIT];
    u8 waiting_mask, pass_counter, second_counter, reserved;
    Sc64Z8Rect moved[SC64_Z8_SAVE_MOVED_LOCATIONS];
    /* Source wallets of the two scripted Protoss owners (slots 1 and 2). */
    u32 ai_minerals[2], ai_gas[2];
    /* Authored town and raid policy state; see sc64_z8_ai.h. */
    u8 ai[SC64_Z8_SAVE_AI_BYTES];
} Sc64Z8PersistentState;

_Static_assert(sizeof(Sc64Z8PersistentState) == 180,
               "Z8 save state fills the shared Zerg payload exactly");

/* MoveLocation destinations (group2) are the only locations whose rectangle
 * can differ from the source. Collect them in ascending location order. */
static inline unsigned
sc64_z8_moved_locations(const Sc64Z8Program *p,
                        u8 out[SC64_Z8_SAVE_MOVED_LOCATIONS])
{
    unsigned n = 0;
    for (unsigned loc = 1; p && loc <= p->location_count; ++loc) {
        int used = 0;
        for (unsigned i = 0; i < p->action_count && !used; ++i)
            used = p->actions[i].opcode == 38u && p->actions[i].group2 == loc;
        if (!used) continue;
        if (n >= SC64_Z8_SAVE_MOVED_LOCATIONS) return 0xffu;
        out[n++] = (u8)loc;
    }
    return n;
}

static inline int
sc64_z8_rect_is_source(const Sc64Z8Rect *r, const Sc64Z8SourceLocation *src)
{
    return r->left == (s16)src->left && r->top == (s16)src->top &&
           r->right == (s16)src->right && r->bottom == (s16)src->bottom;
}

/* Pack a valid live state. Fails on any field the compact form cannot hold,
 * so a save never silently drops scheduler state. The AI bytes and wallets
 * are owned by the caller and left untouched. */
static inline int
sc64_z8_state_pack(Sc64Z8PersistentState *out, const Sc64Z8SchedulerState *s,
                   const Sc64Z8Program *p)
{
    u8 moved[SC64_Z8_SAVE_MOVED_LOCATIONS];
    unsigned moved_count = sc64_z8_moved_locations(p, moved);
    if (!out || !sc64_z8_state_valid(s, p) || moved_count == 0xffu ||
        s->instance_count > SC64_Z8_SAVE_INSTANCES) return 0;
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
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        out->wait_ms[o] = s->wait_ms[o];
        out->outcome[o] = s->outcome[o];
    }
    for (unsigned i = 0; i < SC64_Z8_SWITCH_BYTES; ++i) out->switches[i] = s->switches[i];
    for (unsigned i = 0; i < SC64_Z8_SAVE_INSTANCES; ++i)
        out->instance[i] = i < s->instance_count ?
            (u8)(s->instances[i].flags | (s->instances[i].cursor << 4)) : 0u;
    out->waiting_mask = s->waiting_mask;
    out->pass_counter = s->pass_counter;
    out->second_counter = s->second_counter;
    out->reserved = 0;
    for (unsigned m = 0; m < SC64_Z8_SAVE_MOVED_LOCATIONS; ++m)
        out->moved[m] = m < moved_count ? s->locations[moved[m] - 1] : (Sc64Z8Rect){0};
    return 1;
}

/* Rebuild a full scheduler image from the program, the mission-start setup
 * and a compact record in caller-provided scratch (the engine lends a
 * transient buffer rather than reserving a second image). `out` changes
 * only if the result validates; `out` may be null to validate only. */
static inline int
sc64_z8_state_unpack(Sc64Z8SchedulerState *out, const Sc64Z8PersistentState *in,
                     const Sc64Z8Program *p, const Sc64Z8Setup *setup,
                     Sc64Z8SchedulerState *scratch)
{
    Sc64Z8SchedulerState *const image = scratch;
    u8 moved[SC64_Z8_SAVE_MOVED_LOCATIONS];
    unsigned moved_count = sc64_z8_moved_locations(p, moved);
    if (!image || image == out || !in || in->reserved || moved_count == 0xffu ||
        !sc64_z8_init(image, p, setup) ||
        image->instance_count > SC64_Z8_SAVE_INSTANCES) return 0;
    for (unsigned i = image->instance_count; i < SC64_Z8_SAVE_INSTANCES; ++i)
        if (in->instance[i]) return 0;
    for (unsigned m = moved_count; m < SC64_Z8_SAVE_MOVED_LOCATIONS; ++m) {
        const Sc64Z8Rect *r = &in->moved[m];
        if (r->left || r->top || r->right || r->bottom) return 0;
    }
    image->frames = in->frames;
    image->passes = in->passes;
    image->elapsed_seconds = in->elapsed_seconds;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) {
        image->wait_ms[o] = in->wait_ms[o];
        image->outcome[o] = in->outcome[o];
    }
    for (unsigned i = 0; i < SC64_Z8_SWITCH_BYTES; ++i) image->switches[i] = in->switches[i];
    for (unsigned i = 0; i < image->instance_count; ++i) {
        image->instances[i].flags = in->instance[i] & 0x0fu;
        image->instances[i].cursor = in->instance[i] >> 4;
    }
    image->waiting_mask = in->waiting_mask;
    image->pass_counter = in->pass_counter;
    image->second_counter = in->second_counter;
    for (unsigned m = 0; m < moved_count; ++m) image->locations[moved[m] - 1] = in->moved[m];
    if (!sc64_z8_state_valid(image, p)) return 0;
    if (out) *out = *image;
    return 1;
}

#endif /* SC64_Z8_STATE_H */
