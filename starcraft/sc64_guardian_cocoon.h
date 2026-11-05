/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_COCOON_H
#define SC64_GUARDIAN_COCOON_H

/* Isolated candidate facts and bounded normalization. This header does not
 * enable a morph command or change any serialized production record. */
enum { SC64_GUARDIAN_RAW = 44, SC64_COCOON_RAW = 59,
       SC64_COCOON_HP = 200, SC64_GUARDIAN_TICKS = 750 };
typedef struct {
    int type, raw_type, hp, maxhp, target, work_timer, order, carry, alive;
} Sc64GuardianCocoonRow;

static int sc64_guardian_cocoon_valid(const Sc64GuardianCocoonRow *r,
                                      int guardian_wire_type)
{
    return r && guardian_wire_type >= 0 && guardian_wire_type < 64 &&
        r->type == guardian_wire_type && r->raw_type == SC64_COCOON_RAW &&
        r->alive == 1 && r->hp > 0 && r->hp <= SC64_COCOON_HP &&
        r->maxhp == SC64_COCOON_HP && r->target == SC64_GUARDIAN_RAW &&
        r->work_timer > 0 && r->work_timer <= SC64_GUARDIAN_TICKS &&
        r->order == 0 && r->carry == 0;
}

static int sc64_guardian_scaled_hp(int hp, int old_max, int new_max)
{
    if (hp <= 0 || old_max <= 0 || new_max <= 0 || hp > old_max ||
        old_max > 200 || new_max > 200) return 0;
    int result = hp * new_max / old_max;
    return result ? result : 1;
}
#endif
