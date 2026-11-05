/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_WIRE_H
#define SC64_GUARDIAN_WIRE_H
/* Serialization-only candidate conversion. A caller must authenticate the
 * journal discriminator before passing staged=1. This does not publish saves. */
#ifdef SC64_GUARDIAN_COCOON_STAGE
static int sc64_guardian_wire_encode(const Unit *in, Unit *out, int staged)
{
    if (!in || !out) return 0;
#ifdef SC64_GUARDIAN_DISTINCT_STAGE
    if (in->type == U_GUARDIAN_STAGE) {
        if (!staged || in->raw_type != 44 || (unsigned)in->alive > 1u ||
            in->maxhp != 150 || in->armor != 2 || in->shield ||
            in->order < ORDER_IDLE || in->order > ORDER_ATTACK_TARGET ||
            in->target < -1 || in->target >= MAX_UNITS + MAX_BUILDINGS ||
            (in->alive ? (in->hp < 1 || in->hp > 150) : in->hp > 0)) return 0;
        *out = *in; return 1;
    }
#endif
    if (in->type != U_GUARDIAN_COCOON) {
        if (in->raw_type == 59) return 0;
        *out = *in; return 1;
    }
    if (!staged || in->raw_type != 59 || (unsigned)in->alive > 1u ||
        (in->alive ? (in->hp < 1 || in->hp > 200 || in->work_timer < 1) :
                     (in->hp > 0 || in->hp < -131072)) ||
        in->maxhp != 200 || in->target != 44 || (unsigned)in->work_timer > 750u ||
        in->build != -1 || in->carry || in->flags ||
        in->order != ORDER_IDLE || in->armor || in->shield)
        return 0;
    *out = *in; out->type = U_OVERLORD; return 1;
}
static int sc64_guardian_wire_decode(const Unit *in, Unit *out, int staged)
{
    Unit decoded, check;
    if (!in || !out) return 0;
#ifdef SC64_GUARDIAN_DISTINCT_STAGE
    if (in->type == U_GUARDIAN_STAGE) return sc64_guardian_wire_encode(in,out,staged);
#endif
    if (in->raw_type != 59) { *out = *in; return 1; }
    if (!staged || in->type != U_OVERLORD) return 0;
    decoded = *in; decoded.type = U_GUARDIAN_COCOON;
    if (!sc64_guardian_wire_encode(&decoded, &check, 1)) return 0;
    *out = decoded; return 1;
}
#endif
#endif
