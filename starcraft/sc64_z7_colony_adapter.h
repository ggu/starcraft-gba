#ifndef SC64_Z7_COLONY_ADAPTER_H
#define SC64_Z7_COLONY_ADAPTER_H

/*
 * Z7-only Creep Colony -> Sunken Colony morph.
 *
 * OpenBW's pinned action/building path requires a completed Creep Colony and
 * completed Spawning Pool, charges the Sunken Colony's 50 minerals at morph
 * admission, and morphs the building in place without consuming a Drone:
 * references/openbw/bwgame.h:17668-17677, 6570-6582. Its completion path
 * adds (new max HP - previous max HP) to current HP rather than healing a
 * damaged colony to full: bwgame.h:6536-6548. The authored 300-frame Sunken
 * build time is represented by the race rule and scaled to 30 Hz by the
 * existing building tick.
 *
 * The normal race_tick_buildings() path owns the 375-tick construction
 * clock. The colony remains a Creep Colony (raw 143, 400 max HP) during that
 * clock, matching OpenBW's incomplete-morph behavior. Call the pre hook
 * immediately before it and the post hook immediately after it. The pre
 * hook snapshots current HP in the otherwise-unused queue_progress field;
 * the post hook changes identity to Sunken only after generic completion
 * and applies the native HP delta. No shield field is used and no storage
 * is added.
 */

#if defined(SC64_Z7_COLONY_DEFENSE_ENABLED) && !defined(SC64_Z7_RUNTIME)
#error "Z7 colony defense requires the authenticated Z7 runtime"
#endif

#if defined(SC64_Z7_RUNTIME) && defined(SC64_Z7_COLONY_DEFENSE_ENABLED)

#define SC64_Z7_SUNKEN_MORPH_FLAG 0x8000u
#define SC64_Z7_CREEP_RAW 143
#define SC64_Z7_SUNKEN_RAW 146
#define SC64_Z7_CREEP_HP 400
#define SC64_Z7_SUNKEN_HP 300
#define SC64_Z7_SUNKEN_MORPH_TICKS ((300 * 5 + 3) / 4)

static int race_z7_colony_morph_state_valid(const Building *q)
{
    if (!q) return 0;
    if (!(q->flags & SC64_Z7_SUNKEN_MORPH_FLAG)) return 1;
    /* Candidate validation must work from HOME before a runtime/map is bound,
       so this helper validates only the serialized row itself. */
    if (!q->alive || q->owner != SIDE ||
        q->type != B_CREEP_COLONY || q->raw_type != SC64_Z7_CREEP_RAW ||
        q->raw_owner != 3 ||
        q->maxhp != SC64_Z7_CREEP_HP || q->queue_count != 0 ||
        q->queue_progress < 1 || q->queue_progress > SC64_Z7_CREEP_HP ||
        q->shield != 0 || q->shield_remainder != 0 || q->build_progress < 0 ||
        (q->under_construction &&
         q->build_progress >= SC64_Z7_SUNKEN_MORPH_TICKS) ||
        (!q->under_construction &&
         (q->build_progress != 0 || q->hp != SC64_Z7_CREEP_HP)))
        return 0;
    return 1;
}

static int race_z7_colony_morph_available(int building)
{
    const Building *q;
    if (!race_z7_active() || race_human != R_ZERG ||
        building < 0 || building >= MAX_BUILDINGS) return 0;
    q = &b[building];
    return q->alive && q->owner == SIDE && !q->under_construction &&
        q->type == B_CREEP_COLONY && q->raw_type == SC64_Z7_CREEP_RAW &&
        q->raw_owner == race_z7_product_source_owner(SIDE) &&
        q->maxhp == SC64_Z7_CREEP_HP && q->hp > 0 && q->hp <= q->maxhp &&
        q->queue_count == 0 && q->queue_progress == 0 &&
        !(q->flags & SC64_Z7_SUNKEN_MORPH_FLAG) &&
        race_owner_has_prereq(SIDE, SC_PR_SPAWNING_POOL);
}

static int race_z7_colony_start_sunken(int building)
{
    Sc64RaceBank bank;
    Building *q;
    if (!race_z7_colony_morph_available(building)) return 0;
    /* Both authored building rules use 300 source frames. Refuse the feature
       if a future rule edit makes the shared Creep construction clock differ. */
    if (sc64_race_building_rule(B_CREEP_COLONY)->build_frames !=
        sc64_race_building_rule(B_SUNKEN_COLONY)->build_frames) return 0;
    q = &b[building];
    bank.minerals = (unsigned short)race_owner_minerals(SIDE);
    bank.gas = (unsigned short)race_owner_gas(SIDE);
    if (!sc64_race_pay_building(&bank, B_SUNKEN_COLONY)) return 0;

    /* Sunken morph is in-place: retain the Creep type, raw identity, max HP,
       slot, location and owner while it is incomplete. The reserved marker
       and per-frame HP snapshot are fully retained by the packed codec. */
    minerals = bank.minerals;
    gas = bank.gas;
    q->under_construction = 1;
    q->build_progress = 0;
    q->queue_progress = q->hp;
    q->flags |= SC64_Z7_SUNKEN_MORPH_FLAG;
    invalidate_path_grid();
    return 1;
}

static int race_z7_colony_pre_building_tick(void)
{
    if (!race_z7_active()) return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        if (!(q->flags & SC64_Z7_SUNKEN_MORPH_FLAG)) continue;
        if (!q->alive) {
            /* Death before morph completion retains the dead slot's source
               identity but drops temporary morph metadata. */
            q->flags &= ~SC64_Z7_SUNKEN_MORPH_FLAG;
            q->queue_progress = 0;
            continue;
        }
        if (!race_z7_colony_morph_state_valid(q) || !q->under_construction)
            return 0;
        q->queue_progress = q->hp;
    }
    return 1;
}

static int race_z7_colony_post_building_tick(void)
{
    if (!race_z7_active()) return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        if (!(q->flags & SC64_Z7_SUNKEN_MORPH_FLAG)) continue;
        if (!q->alive) {
            q->flags &= ~SC64_Z7_SUNKEN_MORPH_FLAG;
            q->queue_progress = 0;
            continue;
        }
        if (!race_z7_colony_morph_state_valid(q)) return 0;
        if (!q->under_construction) {
            int hp = q->queue_progress + SC64_Z7_SUNKEN_HP - SC64_Z7_CREEP_HP;
            if (hp < 1) hp = 1;
            if (hp > SC64_Z7_SUNKEN_HP) hp = SC64_Z7_SUNKEN_HP;
            q->type = B_SUNKEN_COLONY;
            q->raw_type = SC64_Z7_SUNKEN_RAW;
            q->maxhp = SC64_Z7_SUNKEN_HP;
            q->hp = hp;
            q->flags &= ~SC64_Z7_SUNKEN_MORPH_FLAG;
            q->queue_progress = 0;
        }
    }
    return 1;
}

#endif /* SC64_Z7_RUNTIME && SC64_Z7_COLONY_DEFENSE_ENABLED */

#endif /* SC64_Z7_COLONY_ADAPTER_H */
