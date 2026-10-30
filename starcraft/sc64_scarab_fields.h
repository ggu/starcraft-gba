/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_FIELDS_H
#define SC64_SCARAB_FIELDS_H
#include <stdint.h>

/* Inactive field-format prototype. Values alone do not establish actor
 * identity, ownership, lifecycle validity or save compatibility. */
enum { SC64_SCARAB_FIELD_SLOT_LIMIT = 240, SC64_SCARAB_FIELD_NO_SLOT = 255 };
typedef struct { unsigned queued, active_slot, cooldown, progress; } Sc64ScarabParentFields;
typedef struct { unsigned parent_slot, phase, life, no_collision; } Sc64ScarabChildFields;

static inline int sc64_scarab_parent_fields_pack(const Sc64ScarabParentFields *v, int *carry, int *timer)
{
    if (!v || !carry || !timer || v->queued > 5 || v->cooldown > 60 ||
        (v->active_slot >= SC64_SCARAB_FIELD_SLOT_LIMIT && v->active_slot != SC64_SCARAB_FIELD_NO_SLOT) ||
        v->progress > 105 || (v->active_slot == SC64_SCARAB_FIELD_NO_SLOT && v->progress) ||
        (v->active_slot != SC64_SCARAB_FIELD_NO_SLOT && !v->queued)) return 0;
    unsigned bits = v->queued | (v->active_slot << 3) | (v->cooldown << 11);
    /* The legacy race serializer writes each Unit field as signed 16-bit.
     * Store the low 16 bits with a signed bias; timer bit 7 holds bit 16,
     * leaving bits 0..6 for the 0..105 progress value. */
    *carry = (int)(bits & 65535u) - 32768;
    *timer = (int)(v->progress | ((bits >> 16) << 7)); return 1;
}
static inline int sc64_scarab_parent_fields_unpack(int carry, int timer, Sc64ScarabParentFields *out)
{
    if (!out || carry < -32768 || carry > 32767 || timer < 0 || timer > 255) return 0;
    unsigned bits = (unsigned)(carry + 32768) | (((unsigned)timer >> 7) << 16);
    Sc64ScarabParentFields v = {bits & 7u, (bits >> 3) & 255u, bits >> 11, (unsigned)timer & 127u};
    int c,t;
    if (!sc64_scarab_parent_fields_pack(&v,&c,&t) || c != carry || t != timer) return 0;
    *out=v; return 1;
}
/* Caller must authenticate the record generation independently; row contents
 * cannot identify the format. This only converts a known-empty historical
 * raw-83 parent, not arbitrary historical actors or whole save records. */
enum { SC64_SCARAB_PARENT_LEGACY = 1, SC64_SCARAB_PARENT_PACKED = 2 };
static inline int sc64_scarab_parent_convert(unsigned format, int raw_type,
                                             int carry, int timer, int *out_carry, int *out_timer)
{
    Sc64ScarabParentFields p;
    int c, t;
    if (!out_carry || !out_timer || out_carry == out_timer || raw_type != 83) return 0;
    if (format == SC64_SCARAB_PARENT_LEGACY) {
        if (carry != 0 || timer != 0) return 0;
        p = (Sc64ScarabParentFields){0,SC64_SCARAB_FIELD_NO_SLOT,0,0};
        if (!sc64_scarab_parent_fields_pack(&p,&c,&t)) return 0;
    } else if (format == SC64_SCARAB_PARENT_PACKED) {
        if (!sc64_scarab_parent_fields_unpack(carry,timer,&p)) return 0;
        c=carry; t=timer;
    } else return 0;
    *out_carry=c; *out_timer=t; return 1;
}
static inline int sc64_scarab_child_fields_pack(const Sc64ScarabChildFields *v, int *carry, int *timer)
{
    if (!v || !carry || !timer || v->phase > 3 ||
        (v->parent_slot >= SC64_SCARAB_FIELD_SLOT_LIMIT &&
         !(v->parent_slot == SC64_SCARAB_FIELD_NO_SLOT && v->phase >= 2)) ||
        v->life > 90 || v->no_collision > 7 || v->no_collision > v->life) return 0;
    *carry = (int)(v->parent_slot | (v->phase << 8)); *timer = (int)(v->life | (v->no_collision << 7)); return 1;
}
static inline int sc64_scarab_child_fields_unpack(int carry, int timer, Sc64ScarabChildFields *out)
{
    if (!out || carry < 0 || carry > 1023 || timer < 0 || timer > 1023) return 0;
    unsigned bits=(unsigned)carry, t=(unsigned)timer; Sc64ScarabChildFields v={bits & 255u,bits >> 8,t & 127u,t >> 7};
    int c,n; if (!sc64_scarab_child_fields_pack(&v,&c,&n)) return 0; *out=v; return 1;
}
#endif
