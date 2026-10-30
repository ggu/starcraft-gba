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
        v->progress > 104 || (v->active_slot == SC64_SCARAB_FIELD_NO_SLOT && v->progress) ||
        (v->active_slot != SC64_SCARAB_FIELD_NO_SLOT && !v->queued)) return 0;
    unsigned bits = v->queued | (v->active_slot << 3) | (v->cooldown << 11);
    *carry = (int)bits - 65536; *timer = (int)v->progress; return 1;
}
static inline int sc64_scarab_parent_fields_unpack(int carry, int timer, Sc64ScarabParentFields *out)
{
    if (!out || carry < -65536 || carry > 65535 || timer < 0 || timer > 104) return 0;
    unsigned bits = (unsigned)(carry + 65536); Sc64ScarabParentFields v = {bits & 7u, (bits >> 3) & 255u, bits >> 11, (unsigned)timer};
    int c,t; if (!sc64_scarab_parent_fields_pack(&v,&c,&t)) return 0; *out=v; return 1;
}
static inline int sc64_scarab_child_fields_pack(const Sc64ScarabChildFields *v, int *carry, int *timer)
{
    if (!v || !carry || !timer || v->parent_slot >= SC64_SCARAB_FIELD_SLOT_LIMIT || v->phase > 3 || v->life > 90 || v->no_collision > 7 || v->no_collision > v->life) return 0;
    *carry = (int)(v->parent_slot | (v->phase << 8)); *timer = (int)(v->life | (v->no_collision << 7)); return 1;
}
static inline int sc64_scarab_child_fields_unpack(int carry, int timer, Sc64ScarabChildFields *out)
{
    if (!out || carry < 0 || carry > 1023 || timer < 0 || timer > 1023) return 0;
    unsigned bits=(unsigned)carry, t=(unsigned)timer; Sc64ScarabChildFields v={bits & 255u,bits >> 8,t & 127u,t >> 7};
    int c,n; if (!sc64_scarab_child_fields_pack(&v,&c,&n)) return 0; *out=v; return 1;
}
#endif
