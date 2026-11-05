/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_PAID_BUILDING_H
#define SC64_GUARDIAN_PAID_BUILDING_H
/* The bit is meaningful only after candidate authentication and source-row
 * collision preflight. Other source building types may legitimately use it. */
#ifndef SC64_GUARDIAN_PAID_BUILDING_MORPH
#define SC64_GUARDIAN_PAID_BUILDING_MORPH 0x2000u
#endif
static int race_guardian_paid_morph_active(const Building *q)
{
    if (!q || !q->alive || !q->under_construction ||
        !(q->flags & SC64_GUARDIAN_PAID_BUILDING_MORPH) ||
        (q->type!=B_HIVE && q->type!=B_GREATER_SPIRE)) return 0;
    return q->raw_type==sc64_race_raw_building(q->type);
}
static void race_guardian_paid_morph_retire(Building *q)
{
    if (race_guardian_paid_morph_active(q))
        q->flags &= (u16)~SC64_GUARDIAN_PAID_BUILDING_MORPH;
}
#endif
