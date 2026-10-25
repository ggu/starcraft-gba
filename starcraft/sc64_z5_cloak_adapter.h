#ifndef SC64_Z5_CLOAK_ADAPTER_H
#define SC64_Z5_CLOAK_ADAPTER_H

#if defined(SC64_Z5_CLOAK_ENABLED)
#include "sc64_z5_detection.h"
#include "sc64_z5_abilities_generated.h"

enum {
    SC64_Z5_AUTH_PTEC_PERSONNEL_CLOAKING = 10,
    SC64_Z5_AUTH_PTEC_PSIONIC_STORM = 19,
    SC64_Z5_AUTH_KERRIGAN_RAW_TYPE = 51,
    SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER = 3
};

_Static_assert(SC64_Z5_CLOAK_TECH_ID ==
               SC64_Z5_AUTH_PTEC_PERSONNEL_CLOAKING,
               "Z5 source PTEC must enable Personnel Cloaking only");
_Static_assert(SC64_Z5_CLOAK_RAW_TYPE == SC64_Z5_AUTH_KERRIGAN_RAW_TYPE,
               "Z5 source cloak actor must remain Infested Kerrigan");
_Static_assert(SC64_Z5_ABILITIES_COMPLETE == 1u,
               "Z5 cloak requires complete authenticated ability metadata");
_Static_assert(SC64_Z5_ABILITIES_SOURCE_GLOBAL_ID == SC64_Z5_GLOBAL_ID,
               "Z5 cloak ability metadata must match the active campaign");
_Static_assert(SC64_Z5_ABILITIES_PERSONNEL_CLOAK_TECH_ID ==
               SC64_Z5_AUTH_PTEC_PERSONNEL_CLOAKING &&
               SC64_Z5_ABILITIES_PERSONNEL_CLOAK_AVAILABLE == 1u &&
               SC64_Z5_ABILITIES_PERSONNEL_CLOAK_RESEARCHED == 0u &&
               SC64_Z5_ABILITIES_PERSONNEL_CLOAK_HERO_RESEARCH_EXEMPTION == 1u,
               "Z5 source PTEC must grant Kerrigan Personnel Cloaking");
_Static_assert(SC64_Z5_ABILITIES_PSIONIC_STORM_TECH_ID ==
               SC64_Z5_AUTH_PTEC_PSIONIC_STORM &&
               SC64_Z5_ABILITIES_PSIONIC_STORM_AVAILABLE == 0u &&
               SC64_Z5_ABILITIES_PSIONIC_STORM_RESEARCHED == 0u,
               "Z5 source PTEC must not grant Psionic Storm");
_Static_assert(SC64_Z5_ABILITIES_SOURCE_OWNER ==
               SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER &&
               SC64_Z5_ABILITIES_KERRIGAN_RAW_TYPE ==
               SC64_Z5_AUTH_KERRIGAN_RAW_TYPE &&
               SC64_Z5_ABILITIES_KERRIGAN_ENERGY_VALID == 1u &&
               SC64_Z5_ABILITIES_KERRIGAN_INITIAL_CLOAKED == 0u,
               "Z5 cloak identity and initial state must match source metadata");

/* PTEC source audit for authenticated map 008/018: Personnel Cloaking is
 * enabled for the source hero; Psionic Storm is not. These constants name
 * audited CHK facts, not a general technology system. */
static int race_z5_cloak_source_ability_valid(void)
{
    return SC64_Z5_ABILITIES_COMPLETE == 1u &&
           SC64_Z5_ABILITIES_PERSONNEL_CLOAK_AVAILABLE == 1u &&
           SC64_Z5_ABILITIES_PSIONIC_STORM_AVAILABLE == 0u;
}

/* The authored source identity is one raw-51 hero owned by source slot 3,
 * which maps to SIDE in the Z5 campaign. Reject duplicates or a reused raw
 * slot instead of applying cloak energy to an arbitrary actor. */
static int race_z5_cloak_hero_identity_slot(void)
{
    int found = -1;
    if (!race_z5_active()) return -1;
    for (int i=0; i<MAX_UNITS; ++i) if (
        u[i].raw_type == SC64_Z5_AUTH_KERRIGAN_RAW_TYPE &&
        u[i].raw_owner == SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER &&
        u[i].owner == SIDE) {
        if (found >= 0) return -1;
        found = i;
    }
    return found;
}

static int race_z5_cloak_actor(const Unit *q)
{
    return race_z5_active() && q &&
        q->raw_type == SC64_Z5_AUTH_KERRIGAN_RAW_TYPE &&
        q->raw_owner == SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER &&
        q->owner == SIDE;
}

static int race_z5_cloak_hero_slot(void)
{
    int slot=race_z5_cloak_hero_identity_slot();
    return slot >= 0 && u[slot].alive ? slot : -1;
}

/* Call once after fresh source-unit placement, never when rebinding a save. */
static int race_z5_cloak_initialize_source_units(void)
{
    int hero;
    if (!race_z5_active() || !race_z5_cloak_source_ability_valid()) return 0;
    hero = race_z5_cloak_hero_slot();
    if (hero < 0 || u[hero].type != U_ZP_INFESTED_KERRIGAN ||
        u[hero].maxhp != sc64_zp_actor_default_hp(u[hero].type)) return 0;
    race_z5.cloak.active = 0;
    race_z5.cloak.regen_remainder = 0;
    u[hero].carry = (SC64_Z5_CLOAK_MAX_ENERGY_Q8 *
        SC64_Z5_ABILITIES_KERRIGAN_INITIAL_ENERGY_PERCENT) / 100;
    return 1;
}

static int race_z5_cloak_menu_available(void)
{
    int hero = race_z5_cloak_hero_slot();
    return race_z5_cloak_source_ability_valid() && hero >= 0 &&
           selected_building < 0 &&
           u[hero].selected;
}

/* Returns the model status so the normal UI can distinguish denied energy
 * from a successful toggle without introducing a new action path. */
static int race_z5_cloak_toggle_selected(void)
{
    int hero;
    if (!race_z5_cloak_menu_available()) return SC64_Z5_CLOAK_ERROR;
    hero = race_z5_cloak_hero_slot();
    return sc64_z5_cloak_toggle(&race_z5.cloak, &u[hero].carry);
}

/* Invoke exactly once per live 30Hz game update. Result/pause screens and
 * the unacknowledged source presentation pause do not drain or regenerate. */
static int race_z5_cloak_tick(void)
{
    int hero;
    if (!race_z5_active()) return 1;
    if (screen != SCREEN_PLAY || race_z5_world_paused()) return 1;
    hero = race_z5_cloak_hero_identity_slot();
    if (hero < 0) return 0;
    if (!u[hero].alive) {
        race_z5.cloak.active=0;
        race_z5.cloak.regen_remainder=0;
        return 1;
    }
    return sc64_z5_cloak_tick(&race_z5.cloak, &u[hero].carry, 0) >= 0;
}

static int race_z5_cloak_world_state_valid(void)
{
    int hero;
    if (!race_z5_active() || !race_z5_cloak_source_ability_valid() ||
        !sc64_z5_persistent_state_valid(&race_z5)) return 0;
    hero=race_z5_cloak_hero_identity_slot();
    if (hero < 0) return 0;
    return u[hero].carry >= 0 &&
        u[hero].carry <= SC64_Z5_CLOAK_MAX_ENERGY_Q8 &&
        (u[hero].alive || (!race_z5.cloak.active && !race_z5.cloak.regen_remainder));
}

/* A cloak hides only this side-owned source hero. For the current Z5 authored
 * policy, a live enabled enemy detector prop makes her targetable. Traps are
 * stored in the Building pool by the adapter even though their native source
 * UnitType is not Building; use source fact extents and map coordinates. */
static int race_z5_cloaked_target_attackable(int target_slot)
{
    const Unit *target;
    int hero;
    const Sc64ZpActorFact *actor;
    Sc64Z5DetectBox target_box;
    if (target_slot < 0 || target_slot >= MAX_UNITS) return 1;
    target=&u[target_slot];
    if (!race_z5_cloak_actor(target) || !target->alive ||
        !race_z5_active()) return 1;
    if (!sc64_z5_cloak_state_valid(&race_z5.cloak)) return 1;
    if (!race_z5.cloak.active) return 1;
    hero = race_z5_cloak_hero_slot();
    if (hero < 0 || target_slot != hero) return 1;
    actor = sc64_zp_actor_fact(target->type);
    if (!actor || actor->raw != SC64_Z5_CLOAK_RAW_TYPE) return 1;
    target_box=(Sc64Z5DetectBox){target->x*2,target->y*2,
        actor->left,actor->up,actor->right,actor->down};
    for (unsigned i=0; i<race_z5_world.doodad_count; ++i) {
        const Sc64Z5DoodadState *d=&race_z5_doodads[i];
        const Sc64ZpBuildingFact *policy;
        const Sc64Z5BuildingFact *source;
        Sc64Z5TrapDetector detector;
        const Building *trap;
        int live_owner;
        if (!d->enabled || d->building_slot >= MAX_BUILDINGS ||
            !sc64_z5_detector_raw(d->raw_type)) continue;
        policy=sc64_zp_building_fact(sc64_race_building_from_raw(d->raw_type));
        source=sc64_z5_building_fact(d->raw_type);
        trap=&b[d->building_slot];
        if (!policy || !source || !(source->flags & SC64_Z5_SOURCE_DETECTOR) ||
            (source->flags & SC64_Z5_SOURCE_BUILDING) || !trap->alive || trap->hp<=0 ||
            trap->raw_type != d->raw_type || trap->raw_owner != d->source_owner)
            continue;
        live_owner=race_z5_source_owner(d->source_owner);
        if (live_owner < 0 || live_owner == target->owner) continue;
        detector.raw_type=d->raw_type;
        detector.source_flags=source->flags;
        detector.box=(Sc64Z5DetectBox){trap->x*2,trap->y*2,
            source->left,source->up,source->right,source->down};
        detector.owner=d->source_owner;
        detector.alive=1;
        detector.enabled=d->enabled;
        detector.completed=1;
        detector.blinded=0;
        if (sc64_z5_trap_detects(&target_box,
                                 SC64_Z5_AUTH_KERRIGAN_SOURCE_OWNER,
                                 &detector)) return 1;
    }
    return 0;
}
#endif /* SC64_Z5_CLOAK_ENABLED */

#endif
