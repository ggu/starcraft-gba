#ifndef SC64_Z6_CLOAK_ADAPTER_H
#define SC64_Z6_CLOAK_ADAPTER_H

#if defined(SC64_Z6_CLOAK_ENABLED)
#if !defined(SC64_Z6_RUNTIME) || !defined(SC64_Z5_CLOAK_ENABLED)
#error "Z6 cloak adapter requires Z6 runtime and the shared bounded cloak model"
#endif

#include "sc64_z5_detection.h"
#include "sc64_z6_abilities_generated.h"

enum {
    SC64_Z6_AUTH_PTEC_PERSONNEL_CLOAKING = 10,
    SC64_Z6_AUTH_KERRIGAN_RAW_TYPE = 51,
    SC64_Z6_AUTH_KERRIGAN_SOURCE_OWNER = 3,
    SC64_Z6_AUTH_OBSERVER_RAW_TYPE = 84,
    SC64_Z6_AUTH_PHOTON_CANNON_RAW_TYPE = 162,
    SC64_Z6_AUTH_KERRIGAN_INITIAL_ENERGY_PERCENT = 100,
    SC64_Z6_AUTH_OBSERVER_SIGHT_SOURCE_PX = 9 * 32,
    /* Pinned OpenBW caps a grounded building's detection at seven tiles,
     * independently of Photon Cannon's eleven-tile sight table entry. */
    SC64_Z6_AUTH_PHOTON_CANNON_DETECT_SOURCE_PX = 7 * 32
};

_Static_assert(SC64_Z6_ABILITIES_COMPLETE == 1u &&
               SC64_Z6_ABILITIES_SOURCE_GLOBAL_ID == SC64_Z6_GLOBAL_ID,
               "Z6 cloak metadata must match authenticated active campaign");
_Static_assert(SC64_Z6_ABILITIES_PERSONNEL_CLOAK_TECH_ID ==
               SC64_Z6_AUTH_PTEC_PERSONNEL_CLOAKING &&
               SC64_Z6_ABILITIES_PERSONNEL_CLOAK_AVAILABLE == 1u &&
               SC64_Z6_ABILITIES_PERSONNEL_CLOAK_RESEARCHED == 0u &&
               SC64_Z6_ABILITIES_PERSONNEL_CLOAK_HERO_RESEARCH_EXEMPTION == 1u,
               "Z6 source must grant raw51 its available, unresearched cloak");
_Static_assert(SC64_Z6_ABILITIES_CONSUME_AVAILABLE == 0u &&
               SC64_Z6_ABILITIES_CONSUME_RESEARCHED == 0u,
               "Z6 source does not grant Consume");
_Static_assert(SC64_Z6_ABILITIES_HERO_RAW_TYPE ==
               SC64_Z6_AUTH_KERRIGAN_RAW_TYPE &&
               SC64_Z6_ABILITIES_HERO_INITIAL_ENERGY_PERCENT ==
               SC64_Z6_AUTH_KERRIGAN_INITIAL_ENERGY_PERCENT &&
               SC64_Z6_ABILITIES_HERO_INITIAL_CLOAKED == 0u,
               "Z6 source Kerrigan energy and initial cloak state changed");
_Static_assert(SC64_Z6_ABILITIES_OBSERVER_RAW_TYPE ==
               SC64_Z6_AUTH_OBSERVER_RAW_TYPE &&
               SC64_Z6_ABILITIES_OBSERVER_SIGHT_TILES == 9u &&
               SC64_Z6_ABILITIES_PHOTON_CANNON_RAW_TYPE ==
               SC64_Z6_AUTH_PHOTON_CANNON_RAW_TYPE &&
               SC64_Z6_ABILITIES_PHOTON_CANNON_SIGHT_TILES == 11u,
               "Z6 cloak detector facts changed");

static int race_z6_cloak_source_ability_valid(void)
{
    return race_z6_active() && SC64_Z6_ABILITIES_COMPLETE == 1u &&
        SC64_Z6_ABILITIES_PERSONNEL_CLOAK_AVAILABLE == 1u &&
        SC64_Z6_ABILITIES_CONSUME_AVAILABLE == 0u;
}

/* The source has one owner-3 raw51 Infested Kerrigan. Resolve by both source
 * provenance and the imported type; never attach ability energy to a reused
 * or duplicate slot. Slot 3 maps to the human SIDE in this mission. */
static int race_z6_cloak_hero_identity_slot(void)
{
    int found=-1;
    if (!race_z6_active() || race_z6_source_owner(SC64_Z6_AUTH_KERRIGAN_SOURCE_OWNER) != SIDE)
        return -1;
    for (int i=0;i<MAX_UNITS;++i) if (
        u[i].raw_type == SC64_Z6_AUTH_KERRIGAN_RAW_TYPE &&
        u[i].raw_owner == SC64_Z6_AUTH_KERRIGAN_SOURCE_OWNER &&
        u[i].owner == SIDE) {
        if (found>=0) return -1;
        found=i;
    }
    return found;
}

static int race_z6_cloak_actor(const Unit *q)
{
    return race_z6_active() && q &&
        q->raw_type == SC64_Z6_AUTH_KERRIGAN_RAW_TYPE &&
        q->raw_owner == SC64_Z6_AUTH_KERRIGAN_SOURCE_OWNER && q->owner == SIDE;
}

static int race_z6_cloak_hero_slot(void)
{
    int slot=race_z6_cloak_hero_identity_slot();
    return slot>=0 && u[slot].alive ? slot : -1;
}

/* Called only after the authenticated map import. Save rebinding must preserve
 * carry energy and race_z6.cloak rather than resetting them. */
static int race_z6_cloak_initialize_source_units(void)
{
    int hero;
    if (!race_z6_cloak_source_ability_valid()) return 0;
    hero=race_z6_cloak_hero_slot();
    if (hero<0 || u[hero].type != U_ZP_INFESTED_KERRIGAN ||
        u[hero].maxhp != sc64_zp_actor_default_hp(u[hero].type)) return 0;
    race_z6.cloak.active=0;
    race_z6.cloak.regen_remainder=0;
    u[hero].carry=(SC64_Z5_CLOAK_MAX_ENERGY_Q8 *
        SC64_Z6_ABILITIES_HERO_INITIAL_ENERGY_PERCENT) / 100;
    return 1;
}

static int race_z6_cloak_menu_available(void)
{
    int hero=race_z6_cloak_hero_slot();
    return race_z6_cloak_source_ability_valid() && hero>=0 &&
        selected_building<0 && u[hero].selected;
}

static int race_z6_cloak_toggle_selected(void)
{
    int hero;
    if (!race_z6_cloak_menu_available()) return SC64_Z5_CLOAK_ERROR;
    hero=race_z6_cloak_hero_slot();
    return sc64_z5_cloak_toggle(&race_z6.cloak,&u[hero].carry);
}

/* Called once per active engine update; callers pass all pause states so
 * source presentations and user pause both freeze drain/regeneration. */
static int race_z6_cloak_tick(int paused)
{
    int hero;
    if (!race_z6_active()) return 1;
    if (paused<0 || paused>1) return 0;
    if (paused) return 1;
    if (race_z6.runtime.model.outcome != SC64_Z6_RUNNING) return 1;
    hero=race_z6_cloak_hero_identity_slot();
    if (hero<0) return 0;
    if (!u[hero].alive) {
        race_z6.cloak.active=0;
        race_z6.cloak.regen_remainder=0;
        return 1;
    }
    return sc64_z5_cloak_tick(&race_z6.cloak,&u[hero].carry,0)>=0;
}

static int race_z6_cloak_world_state_valid(void)
{
    int hero;
    if (!race_z6_active() || !race_z6_cloak_source_ability_valid() ||
        !sc64_z5_cloak_state_valid(&race_z6.cloak)) return 0;
    hero=race_z6_cloak_hero_identity_slot();
    if (hero<0) return 0;
    return u[hero].carry>=0 && u[hero].carry<=SC64_Z5_CLOAK_MAX_ENERGY_Q8 &&
        (u[hero].alive || (!race_z6.cloak.active && !race_z6.cloak.regen_remainder));
}

static int race_z6_cloak_target_box(const Unit *q, Sc64Z5DetectBox *box)
{
    if (!q || !box || q->x<0 || q->x>=WORLD_W || q->y<0 || q->y>=WORLD_H)
        return 0;
    *box=(Sc64Z5DetectBox){q->x*2,q->y*2,7,10,7,11};
    return sc64_z5_detect_box_valid(box);
}

static int race_z6_cloak_observer_box(const Unit *q, Sc64Z5DetectBox *box)
{
    if (!q || !box || q->x<0 || q->x>=WORLD_W || q->y<0 || q->y>=WORLD_H)
        return 0;
    *box=(Sc64Z5DetectBox){q->x*2,q->y*2,16,16,15,15};
    return sc64_z5_detect_box_valid(box);
}

static int race_z6_cloak_cannon_box(const Building *q, Sc64Z5DetectBox *box)
{
    if (!q || !box || q->x<0 || q->x>=WORLD_W || q->y<0 || q->y>=WORLD_H)
        return 0;
    *box=(Sc64Z5DetectBox){q->x*2,q->y*2,20,16,20,16};
    return sc64_z5_detect_box_valid(box);
}

static int race_z6_cloak_enemy_detector_sees(const Sc64Z5DetectBox *hero_box)
{
    Sc64Z5DetectBox detector_box;
    if (!hero_box || !sc64_z5_detect_box_valid(hero_box)) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive &&
        u[i].raw_type==SC64_Z6_AUTH_OBSERVER_RAW_TYPE &&
        owners_hostile(SIDE,u[i].owner) &&
        race_z6_cloak_observer_box(&u[i],&detector_box) &&
        sc64_z5_box_distance(hero_box,&detector_box)<=
            SC64_Z6_AUTH_OBSERVER_SIGHT_SOURCE_PX) return 1;
    for (int i=0;i<MAX_BUILDINGS;++i) if (building_complete(i) && b[i].hp>0 &&
        b[i].raw_type==SC64_Z6_AUTH_PHOTON_CANNON_RAW_TYPE &&
        b[i].type==B_PHOTON_CANNON && owners_hostile(SIDE,b[i].owner) &&
        race_z6_cloak_cannon_box(&b[i],&detector_box) &&
        sc64_z5_box_distance(hero_box,&detector_box)<=
            SC64_Z6_AUTH_PHOTON_CANNON_DETECT_SOURCE_PX) return 1;
    return 0;
}

/* Only the source hero is hidden by this cloak policy. Live hostile Observer
 * and Photon Cannon arrays provide detection; this intentionally excludes
 * unrelated map doodads and does not turn generic sight into detection. */
static int __attribute__((noinline))
race_z6_cloaked_target_attackable(int target_slot)
{
    Sc64Z5DetectBox hero_box;
    int hero;
    if (target_slot<0 || target_slot>=MAX_UNITS) return 1;
    if (!race_z6_cloak_actor(&u[target_slot]) || !u[target_slot].alive ||
        !race_z6_active() || !race_z6.cloak.active) return 1;
    hero=race_z6_cloak_hero_slot();
    if (hero<0 || target_slot!=hero || !race_z6_cloak_target_box(&u[target_slot],&hero_box))
        return 1;
    return race_z6_cloak_enemy_detector_sees(&hero_box);
}

#endif /* SC64_Z6_CLOAK_ENABLED */

#endif
