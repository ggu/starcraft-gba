#ifndef SC64_Z7_COLONY_ADAPTER_H
#define SC64_Z7_COLONY_ADAPTER_H

/*
 * Z7-only Creep Colony -> Sunken/Spore Colony morphs.
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
 * The normal race_tick_buildings() path owns the 375-tick construction.
 * Spore Colony has the same 300 source-frame morph time, 50-mineral cost and
 * 400 HP, but requires a completed Evolution Chamber. Its existing authored
 * combat rule is air-only, 15 damage, 112 range and 19 tick cooldown. The
 * current engine has no building detector path, so Spore cloak detection is
 * not provided. The colony remains a Creep Colony (raw 143, 400 max HP) during
 * that clock, matching OpenBW's incomplete-morph behavior. Call the pre hook
 * immediately before it and the post hook immediately after it. The pre
 * hook snapshots current HP in the otherwise-unused queue_progress field;
 * the post hook changes identity only after generic completion and applies
 * the Sunken native HP delta or preserves Spore HP. No shield field is used and no storage
 * is added.
 */

#if defined(SC64_Z7_COLONY_DEFENSE_ENABLED) && !defined(SC64_Z7_RUNTIME)
#error "Z7 colony defense requires the authenticated Z7 runtime"
#endif

#if defined(SC64_Z7_RUNTIME) && defined(SC64_Z7_COLONY_DEFENSE_ENABLED)

#define SC64_Z7_CREEP_RAW 143
#define SC64_Z7_SUNKEN_RAW 146
#define SC64_Z7_SPORE_RAW 144
#define SC64_Z7_CREEP_HP 400
#define SC64_Z7_SUNKEN_HP 300
#define SC64_Z7_SPORE_HP 400
#define SC64_Z7_SUNKEN_MORPH_TICKS ((300 * 5 + 3) / 4)
#define SC64_Z7_SPORE_MORPH_TICKS ((300 * 5 + 3) / 4)

static int race_z7_colony_has_evolution_chamber(void)
{
    for (int i=0; i<MAX_BUILDINGS; ++i)
        if (building_complete(i) && b[i].owner==SIDE &&
            b[i].type==B_EVOLUTION_CHAMBER) return 1;
    return 0;
}

static int race_z7_colony_target_prerequisite(int target_type)
{
    if (target_type==B_SUNKEN_COLONY)
        return race_owner_has_prereq(SIDE,SC_PR_SPAWNING_POOL);
    if (target_type==B_SPORE_COLONY)
        return race_z7_colony_has_evolution_chamber();
    return 0;
}

static int race_z7_colony_morph_state_valid(const Building *q)
{
    unsigned morph;
    int target_ticks;
    if (!q) return 0;
    morph=q->flags & (SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG);
    if (!morph) return 1;
    if (morph==SC64_Z7_SUNKEN_MORPH_FLAG)
        target_ticks=SC64_Z7_SUNKEN_MORPH_TICKS;
    else if (morph==SC64_Z7_SPORE_MORPH_FLAG)
        target_ticks=SC64_Z7_SPORE_MORPH_TICKS;
    else return 0;
    /* Candidate validation must work from HOME before a runtime/map is bound,
       so this helper validates only the serialized row itself. */
    if (!q->alive || q->owner != SIDE ||
        q->type != B_CREEP_COLONY || q->raw_type != SC64_Z7_CREEP_RAW ||
        q->raw_owner != 3 ||
        q->maxhp != SC64_Z7_CREEP_HP || q->queue_count != 0 ||
        q->queue_progress < 1 || q->queue_progress > SC64_Z7_CREEP_HP ||
        q->shield != 0 || q->shield_remainder != 0 || q->build_progress < 0 ||
        (q->under_construction &&
         q->build_progress >= target_ticks) ||
        (!q->under_construction &&
         (q->build_progress != 0 ||
#ifdef SC64_CONSTRUCTION_HP
          q->hp != q->queue_progress
#else
          q->hp != SC64_Z7_CREEP_HP
#endif
          )))
        return 0;
    return 1;
}

static int race_z7_colony_morph_available_for(int building,int target_type);

static int race_z7_colony_morph_available(int building)
{ return race_z7_colony_morph_available_for(building,B_SUNKEN_COLONY); }

static int race_z7_colony_morph_available_for(int building,int target_type)
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
        !(q->flags & (SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG)) &&
        race_z7_colony_target_prerequisite(target_type);
}

static int race_z7_colony_start_morph(int building,int target_type)
{
    unsigned morph_flag;
    Sc64RaceBank bank;
    Building *q;
    if (!race_z7_colony_morph_available_for(building,target_type)) return 0;
    if (target_type==B_SUNKEN_COLONY) morph_flag=SC64_Z7_SUNKEN_MORPH_FLAG;
    else if (target_type==B_SPORE_COLONY) morph_flag=SC64_Z7_SPORE_MORPH_FLAG;
    else return 0;
    /* Both authored building rules use 300 source frames. Refuse the feature
       if a future rule edit makes the shared Creep construction clock differ. */
    if (sc64_race_building_rule(B_CREEP_COLONY)->build_frames !=
        sc64_race_building_rule(target_type)->build_frames) return 0;
    q = &b[building];
    bank.minerals = (unsigned short)race_owner_minerals(SIDE);
    bank.gas = (unsigned short)race_owner_gas(SIDE);
    if (!sc64_race_pay_building(&bank, target_type)) return 0;

    /* Both morphs are in-place: retain the Creep type, raw identity, max HP,
       slot, location and owner while it is incomplete. The reserved marker
       and per-frame HP snapshot are fully retained by the packed codec. */
    minerals = bank.minerals;
    gas = bank.gas;
    q->under_construction = 1;
    q->build_progress = 0;
    q->queue_progress = q->hp;
    q->flags |= morph_flag;
    invalidate_path_grid();
    return 1;
}

static int race_z7_colony_start_sunken(int building)
{ return race_z7_colony_start_morph(building,B_SUNKEN_COLONY); }

static int race_z7_colony_start_spore(int building)
{ return race_z7_colony_start_morph(building,B_SPORE_COLONY); }

static int race_z7_colony_pre_building_tick(void)
{
    if (!race_z7_active()) return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        if (!(q->flags & (SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG))) continue;
        if (!q->alive) {
            /* Death before morph completion retains the dead slot's source
               identity but drops temporary morph metadata. */
            q->flags &= ~(SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG);
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
        if (!(q->flags & (SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG))) continue;
        if (!q->alive) {
            q->flags &= ~(SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG);
            q->queue_progress = 0;
            continue;
        }
        if (!race_z7_colony_morph_state_valid(q)) return 0;
        if (!q->under_construction) {
            int spore=(q->flags & SC64_Z7_SPORE_MORPH_FLAG)!=0;
            int target_hp=spore ? SC64_Z7_SPORE_HP : SC64_Z7_SUNKEN_HP;
            int hp = spore ? q->queue_progress :
                q->queue_progress + SC64_Z7_SUNKEN_HP - SC64_Z7_CREEP_HP;
            if (hp < 1) hp = 1;
            if (hp > target_hp) hp = target_hp;
            q->type = spore ? B_SPORE_COLONY : B_SUNKEN_COLONY;
            q->raw_type = spore ? SC64_Z7_SPORE_RAW : SC64_Z7_SUNKEN_RAW;
            q->maxhp = target_hp;
            q->hp = hp;
            q->flags &= ~(SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG);
            q->queue_progress = 0;
        }
    }
    return 1;
}

#endif /* SC64_Z7_RUNTIME && SC64_Z7_COLONY_DEFENSE_ENABLED */

#endif /* SC64_Z7_COLONY_ADAPTER_H */
