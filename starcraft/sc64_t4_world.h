#ifndef SC64_T4_WORLD_H
#define SC64_T4_WORLD_H

/* T4-only source-slot policy helpers. They describe campaign ownership and
 * rescue decisions; callers perform all world/list mutations. */
enum {
    SC64_T4_OWNER_SIDE = 0,
    SC64_T4_OWNER_ENEMY = 1,
    SC64_T4_OWNER_NEUTRAL = 2,
    SC64_T4_OWNER_RESCUABLE = 3,
    SC64_T4_OWNER_FORCE4 = 4,
    SC64_T4_FORCE_NONE = 0,
    SC64_T4_FORCE1 = 1,
    SC64_T4_FORCE2 = 2,
    SC64_T4_FORCE3 = 3,
    SC64_T4_FORCE4 = 4,
    SC64_T4_RESCUE_SOURCE_HALF_EXTENT = 64
};

/* T4's OWNR slots are 0:Human, 1/2/7:Rescue Passive, 3/4/6:Computer,
 * 5:Unused, and 11:Neutral. Keep Force 2 and Force 4 in different runtime
 * owners even though both use the same OWNR computer controller. */
static int
sc64_t4_runtime_owner(int raw_owner)
{
    switch (raw_owner) {
    case 0: return SC64_T4_OWNER_SIDE;
    case 1: case 2: case 7: return SC64_T4_OWNER_RESCUABLE;
    case 3: return SC64_T4_OWNER_ENEMY;
    case 4: case 6: return SC64_T4_OWNER_FORCE4;
    case 5: return SC64_T4_OWNER_NEUTRAL;
    case 11: return SC64_T4_OWNER_NEUTRAL;
    default: return -1;
    }
}

/* FORC assignments are zero-based on disk. Add one to obtain source force
 * numbers: bytes 00,02,02,01,03,03,03,02 => forces 1,3,3,2,4,4,4,3. */
static int
sc64_t4_source_force(int raw_owner)
{
    static const unsigned char force_by_slot[8] = {1, 3, 3, 2, 4, 4, 4, 3};
    if (raw_owner < 0 || raw_owner >= 8) return SC64_T4_FORCE_NONE;
    return force_by_slot[raw_owner];
}

static int
sc64_t4_force_contains(int force, int raw_owner)
{
    return force > SC64_T4_FORCE_NONE && force <= SC64_T4_FORCE4 &&
           sc64_t4_source_force(raw_owner) == force;
}

/* Source force membership policy: force 2 and force 4 are hostile to the
 * human force; Rescue Passive force 3 remains eligible for rescue. This
 * helper classifies opposition to the player, not force-2-vs-force-4
 * diplomacy; the adapter should keep those owners distinct. */
static int
sc64_t4_force_hostile_to_human(int force)
{
    return force == SC64_T4_FORCE2 || force == SC64_T4_FORCE4;
}

static int
sc64_t4_owner_hostile_to_human(int runtime_owner)
{
    return runtime_owner == SC64_T4_OWNER_ENEMY ||
           runtime_owner == SC64_T4_OWNER_FORCE4;
}

/* Translate current runtime ownership back to the source player slot used by
 * force selectors. raw_owner remains provenance on the actor: after rescue,
 * current SIDE ownership must count as slot 0/Force 1. */
static int
sc64_t4_current_source_owner(int runtime_owner, int original_raw_owner)
{
    switch (runtime_owner) {
    case SC64_T4_OWNER_SIDE:
        return original_raw_owner == 0 || original_raw_owner == 1 ||
               original_raw_owner == 2 || original_raw_owner == 7 ? 0 : -1;
    case SC64_T4_OWNER_ENEMY:
        return original_raw_owner == 3 ? 3 : -1;
    case SC64_T4_OWNER_NEUTRAL:
        return original_raw_owner == 5 || original_raw_owner == 11 ?
               original_raw_owner : -1;
    case SC64_T4_OWNER_RESCUABLE:
        return original_raw_owner == 1 || original_raw_owner == 2 ||
               original_raw_owner == 7 ? original_raw_owner : -1;
    case SC64_T4_OWNER_FORCE4:
        return original_raw_owner == 4 || original_raw_owner == 6 ?
               original_raw_owner : -1;
    default:
        return -1;
    }
}

/* The retained FORC payload assigns distinct active forces 1, 2, and 4;
 * T4 has no SetAllianceStatus trigger actions. OpenBW's controller policy
 * keeps Rescue Passive and Neutral allied with everyone, while distinct
 * active forces oppose one another. Runtime owner 4 retains slots 4/6 as a
 * single force and therefore remains allied internally. */
static int
sc64_t4_owners_hostile(int owner_a, int owner_b)
{
    int active_a, active_b;
    if (owner_a < SC64_T4_OWNER_SIDE || owner_a > SC64_T4_OWNER_FORCE4 ||
        owner_b < SC64_T4_OWNER_SIDE || owner_b > SC64_T4_OWNER_FORCE4)
        return 0;
    active_a = owner_a == SC64_T4_OWNER_SIDE ||
               owner_a == SC64_T4_OWNER_ENEMY ||
               owner_a == SC64_T4_OWNER_FORCE4;
    active_b = owner_b == SC64_T4_OWNER_SIDE ||
               owner_b == SC64_T4_OWNER_ENEMY ||
               owner_b == SC64_T4_OWNER_FORCE4;
    return active_a && active_b && owner_a != owner_b;
}

/* The audited hook's candidate test requires a Human controller and a
 * non-null sprite. The caller supplies presence/sprite state because imported
 * map revealers and stale slots are not candidates. */
static int
sc64_t4_rescue_candidate_present(int live_owner, int has_sprite)
{
    return live_owner == SC64_T4_OWNER_SIDE && has_sprite;
}

/* GPTP evidence establishes a +/-64 pixel rectangle around the Rescue
 * Passive actor and selects a Human-controller candidate. The binary query's
 * edge convention/list order is unknown, so this deterministic adaptation
 * uses an inclusive source-pixel center rectangle as a deterministic
 * approximation. T4 has exactly one human slot, removing candidate
 * selection ambiguity. The caller filters for present/sprite-bearing actors;
 * the hook's opaque bounds helper and candidate ordering are not reproduced. */
static int
sc64_t4_rescue_contact(int rescue_order_active, int rescue_raw_owner,
                       int rescue_source_x, int rescue_source_y,
                       int candidate_live_owner,
                       int candidate_x, int candidate_y)
{
    int dx, dy;
    if (!rescue_order_active ||
        (rescue_raw_owner != 1 && rescue_raw_owner != 2 && rescue_raw_owner != 7) ||
        candidate_live_owner != SC64_T4_OWNER_SIDE)
        return 0;
    dx = candidate_x - rescue_source_x;
    dy = candidate_y - rescue_source_y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx <= SC64_T4_RESCUE_SOURCE_HALF_EXTENT &&
           dy <= SC64_T4_RESCUE_SOURCE_HALF_EXTENT;
}

/* Rescue Passive units transfer individually. A completed grounded resource
 * depot instead transfers every live actor belonging to the rescue owner's
 * slot. Source linked-list ordering and the full native transfer side effects
 * are intentionally left to the caller. */
static int
sc64_t4_rescue_depot_group(int completed, int grounded, int resource_depot)
{
    return completed && grounded && resource_depot;
}

static int
sc64_t4_rescue_transfer_candidate(int depot_group, int candidate_alive,
                                  int candidate_raw_owner,
                                  int rescue_raw_owner,
                                  int is_trigger_actor)
{
    if (!candidate_alive ||
        (rescue_raw_owner != 1 && rescue_raw_owner != 2 && rescue_raw_owner != 7))
        return 0;
    if (depot_group) return candidate_raw_owner == rescue_raw_owner;
    return is_trigger_actor && candidate_raw_owner == rescue_raw_owner;
}

#endif
