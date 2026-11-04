/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_WIRE_H
#define SC64_GUARDIAN_WIRE_H
/* Serialization-only candidate conversion. A caller must authenticate the
 * journal discriminator before passing staged=1. This does not publish saves. */
#ifdef SC64_GUARDIAN_COCOON_STAGE
static int sc64_guardian_wire_encode(const Unit *in, Unit *out, int staged)
{
    if (!in || !out) return 0;
    if (in->type != U_GUARDIAN_COCOON) {
        if (in->raw_type == 59) return 0;
        *out = *in; return 1;
    }
    if (!staged || in->raw_type != 59 || in->hp < 1 || in->hp > 200 ||
        in->maxhp != 200 || in->target != 44 || in->work_timer < 1 ||
        in->work_timer > 750 || in->build != -1 || in->carry || in->flags ||
        in->order != ORDER_IDLE || in->armor || in->shield || !in->alive)
        return 0;
    *out = *in; out->type = U_OVERLORD; return 1;
}
static int sc64_guardian_wire_decode(const Unit *in, Unit *out, int staged)
{
    Unit decoded, check;
    if (!in || !out) return 0;
    if (in->raw_type != 59) { *out = *in; return 1; }
    if (!staged || in->type != U_OVERLORD) return 0;
    decoded = *in; decoded.type = U_GUARDIAN_COCOON;
    if (!sc64_guardian_wire_encode(&decoded, &check, 1)) return 0;
    *out = decoded; return 1;
}
#endif
#endif
