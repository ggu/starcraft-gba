/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GHOST_TECH_RUNTIME_H
#define SC64_GHOST_TECH_RUNTIME_H

enum { SC64_GHOST_LIVE_UPGRADES = 12, SC64_GHOST_T4 = 7, SC64_GHOST_T5 = 8,
       SC64_GHOST_T6 = 9, SC64_GHOST_T7 = 10, SC64_GHOST_T10 = 13,
       SC64_GHOST_WORLD_JOB_LIMIT = 2 * SC64_GHOST_TECH_OWNER_COUNT };
typedef struct { Sc64GhostTechJob paid; u8 bank, slot_plus_one, parent_plus_one; } Sc64GhostCovertJob;
_Static_assert(MAX_BUILDINGS < 255, "Ghost tech uses one-byte slot handles");
static Sc64GhostCovertJob sc64_ghost_jobs[SC64_GHOST_WORLD_JOB_LIMIT];
static Sc64GhostTechState sc64_ghost_tech;
static u32 sc64_ghost_initial_tech;
static int sc64_ghost_tech_fault;

/* CHK UPGR/PUPx are default costs throughout the original campaign.
 * Exceptions below are owner restrictions/seeds, not cost overrides. */
static unsigned race_ghost_tech_policy(unsigned key)
{
    if (key >= SC64_GHOST_TECH_OWNER_COUNT) return 0;
    if (race_mode) return SC64_GHOST_LIVE_UPGRADES;
    int mission = race_selected_mission;
    if (mission >= 4 && mission < SC64_GHOST_T4 && key == 1) return 0;
    if (mission == SC64_GHOST_T5) {
        if (key == 0 || key == 3) return 0;
        if (key == 6) return 4;
    }
    if ((mission == SC64_GHOST_T6 || mission == SC64_GHOST_T7) && !key) return 0;
    if (mission == SC64_GHOST_T10 && (key == 0 || key == 6)) return 0;
    return SC64_GHOST_LIVE_UPGRADES;
}

static void race_ghost_tech_reset(void)
{
    memset(sc64_ghost_jobs, 0, sizeof sc64_ghost_jobs);
    sc64_ghost_tech = (Sc64GhostTechState){0};
    sc64_ghost_tech_fault = 0;
    if (!race_mode && race_selected_mission == SC64_GHOST_T5)
        (void)sc64_ghost_tech_seed(&sc64_ghost_tech, 6, 8);
    if (!race_mode && race_selected_mission == SC64_GHOST_T10) {
        (void)sc64_ghost_tech_seed(&sc64_ghost_tech, 0, SC64_GHOST_LIVE_UPGRADES);
        (void)sc64_ghost_tech_seed(&sc64_ghost_tech, 6, SC64_GHOST_LIVE_UPGRADES);
    }
    sc64_ghost_initial_tech = sc64_ghost_tech.completed;
}

static int race_ghost_tech_save_safe(void)
{
    return !sc64_ghost_tech_fault && !sc64_ghost_tech.pending &&
        sc64_ghost_tech.completed == sc64_ghost_initial_tech;
}

static int race_ghost_tech_key(int owner, int raw_owner)
{
    return owner == NEUTRAL ? -1 :
        sc64_battlecruiser_owner_key(race_mode, race_selected_mission, owner, raw_owner);
}

static int race_ghost_max_energy(const Unit *q)
{
    return sc64_ghost_energy_cap(&sc64_ghost_tech,
        (unsigned)race_ghost_tech_key(q->owner, q->raw_owner));
}

static void race_ghost_birth(Unit *q, int placed)
{
    if (!sc64_race_ordinary_ghost(q->type)) return;
    /* Every placed raw-1 row in the supported original campaign census
     * sets the energy-valid bit and 100%; newly trained units start at 25%. */
    int max = race_ghost_max_energy(q);
    q->carry = placed ? max : max / 4;
}

static void race_ghost_tick_energy(Unit *q)
{
    int key = race_ghost_tech_key(q->owner, q->raw_owner);
    if (key < 0) return;
    int cap = sc64_ghost_energy_cap(&sc64_ghost_tech, (unsigned)key);
    /* A transferred Ghost uses its new owner's cap. */
    if (q->carry > cap) q->carry = cap;
    (void)sc64_ghost_energy_tick(&sc64_ghost_tech, (unsigned)key, &q->carry, 0, 0, 1);
}

static int race_ghost_sight(const Unit *q)
{
    return sc64_ghost_sight_source_pixels(&sc64_ghost_tech,
        (unsigned)race_ghost_tech_key(q->owner, q->raw_owner));
}

static Sc64GhostCovertJob *race_ghost_job_at(int slot)
{
    if (!sc64_ghost_tech.pending || (unsigned)slot >= MAX_BUILDINGS) return 0;
    for (unsigned i = 0; i < SC64_GHOST_WORLD_JOB_LIMIT; ++i)
        if (sc64_ghost_jobs[i].slot_plus_one == slot + 1) return &sc64_ghost_jobs[i];
    return 0;
}

static int race_ghost_covert_parent(int slot, int cached)
{
    if (!race_nuke_raw_building(slot, SC64_NUKE_COVERT_RAW)) return -1;
    int parent = race_nuke_parent_matches(slot, cached, 116) ? cached : race_nuke_parent_slot(slot, 116);
    if (parent < 0) return -1;
    int bank = b[slot].owner, key = race_ghost_tech_key(bank, b[slot].raw_owner);
    return key >= 0 && key == race_ghost_tech_key(bank, b[parent].raw_owner) ? parent : -1;
}

static int race_ghost_tech_start(int slot, unsigned kind)
{
    if (sc64_ghost_tech_fault || !(kind == SC64_GHOST_TECH_OCULAR ||
         kind == SC64_GHOST_TECH_REACTOR) || race_ghost_job_at(slot)) return 8;
    int parent = race_ghost_covert_parent(slot, -1);
    if (parent < 0) return 8;
    const Sc64GhostTechFact *fact = sc64_ghost_tech_fact(kind);
    int bank = b[slot].owner, key = race_ghost_tech_key(bank, b[slot].raw_owner);
    unsigned policy = race_ghost_tech_policy((unsigned)key);
    int ore = race_owner_minerals(bank), vespene = race_owner_gas(bank);
    if (ore < fact->minerals) return 1;
    if (vespene < fact->gas) return 12;
    for (unsigned i = 0; i < SC64_GHOST_WORLD_JOB_LIMIT; ++i) {
        Sc64GhostCovertJob *job = &sc64_ghost_jobs[i];
        if (job->slot_plus_one) continue;
        if (!sc64_ghost_tech_begin(&sc64_ghost_tech, &job->paid, (unsigned)key,
                                  kind, policy, 1, &ore, &vespene)) return 8;
        job->bank = (u8)bank;
        job->slot_plus_one = (u8)(slot + 1);
        job->parent_plus_one = (u8)(parent + 1);
        race_add_owner_resources(bank, -(int)fact->minerals, -(int)fact->gas);
        return 0;
    }
    return 8;
}

static int race_ghost_tech_cancel(int slot, int destroyed)
{
    if (!race_nuke_raw_building(slot, SC64_NUKE_COVERT_RAW)) return 0;
    Sc64GhostCovertJob *job = race_ghost_job_at(slot);
    if (!job) return 0;
    int bank = job->bank;
    if (!job->paid.kind || bank == NEUTRAL || (unsigned)bank >= 5u) return 0;
    if (!destroyed && (b[slot].owner != bank ||
        race_ghost_tech_key(bank, b[slot].raw_owner) != job->paid.payer)) return 0;
    int ore = race_owner_minerals(bank), vespene = race_owner_gas(bank);
    int old_ore = ore, old_vespene = vespene;
    if (!sc64_ghost_tech_cancel(&sc64_ghost_tech, &job->paid,
            job->paid.payer, destroyed, &ore, &vespene)) return 0;
    race_add_owner_resources(bank, ore - old_ore, vespene - old_vespene);
    *job = (Sc64GhostCovertJob){0};
    return 1;
}

static void race_ghost_tech_destroy(int slot)
{
    Sc64GhostCovertJob *job = race_ghost_job_at(slot);
    if (job && !race_ghost_tech_cancel(slot, 1)) {
        sc64_ghost_tech_fault = 1;
        *job = (Sc64GhostCovertJob){0};
    }
}

static void race_ghost_tech_tick(void)
{
    if (!sc64_ghost_tech.pending || sc64_ghost_tech_fault ||
        !sc64_battlecruiser_source_tick_due()) return;
    for (unsigned i = 0; i < SC64_GHOST_WORLD_JOB_LIMIT; ++i) {
        Sc64GhostCovertJob *job = &sc64_ghost_jobs[i];
        int slot = (int)job->slot_plus_one - 1;
        if (slot < 0) continue;
        int key = race_ghost_tech_key(b[slot].owner, b[slot].raw_owner);
        int parent = job->bank == b[slot].owner && key == job->paid.payer ?
            race_ghost_covert_parent(slot, (int)job->parent_plus_one - 1) : -1;
        if (parent >= 0) job->parent_plus_one = (u8)(parent + 1);
        int result = sc64_ghost_tech_tick(&sc64_ghost_tech, &job->paid, (unsigned)key, parent >= 0, 1);
        if (result == SC64_GHOST_JOB_COMPLETE) {
            if (job->bank == SIDE) game_audio_cue(GAME_AUDIO_CUE_RESEARCH);
            *job = (Sc64GhostCovertJob){0};
        } else if (result == SC64_GHOST_JOB_ERROR) sc64_ghost_tech_fault = 1;
    }
}

/* Menus use the same filtered action iterator for width, label and dispatch. */
static int race_ghost_tech_menu_at(int index)
{
    int slot = selected_building;
    if (sc64_ghost_tech_fault || !race_nuke_raw_building(slot, SC64_NUKE_COVERT_RAW) ||
        b[slot].owner != SIDE) return 0;
    int key = race_ghost_tech_key(SIDE, b[slot].raw_owner);
    const Sc64GhostCovertJob *job = race_ghost_job_at(slot);
    if (job)
        return job->bank == SIDE && key == job->paid.payer ?
            (index < 0 ? 1 : index == 0 ? SC64_GHOST_MENU_CANCEL : 0) : 0;
    if (race_ghost_covert_parent(slot, -1) < 0) return 0;
    int count = 0;
    unsigned policy = race_ghost_tech_policy((unsigned)key);
    for (int kind = SC64_GHOST_TECH_OCULAR; kind <= SC64_GHOST_TECH_REACTOR; ++kind) {
        u32 bit = sc64_ghost_tech_bit((unsigned)key, (unsigned)kind);
        if ((policy & (1u << (kind - 1))) &&
            !((sc64_ghost_tech.completed | sc64_ghost_tech.pending) & bit)) {
            if (count == index) return kind;
            ++count;
        }
    }
    return index < 0 ? count : 0;
}

static int race_ghost_tech_menu_width(void) { return race_ghost_tech_menu_at(-1); }
static int race_ghost_tech_menu_kind(int row)
{
    if (!race_nuke_raw_building(selected_building, SC64_NUKE_COVERT_RAW)) return 0;
    int offset = row - race_ghost_tech_command_index();
    return offset < 0 ? 0 : race_ghost_tech_menu_at(offset);
}

static int race_ghost_tech_menu_choose(int kind)
{
    return kind == SC64_GHOST_MENU_CANCEL ?
        (race_ghost_tech_cancel(selected_building, 0) ? 0 : 8) :
        race_ghost_tech_start(selected_building, (unsigned)kind);
}

static const char *race_ghost_tech_menu_label(int kind)
{
    if (kind == SC64_GHOST_MENU_CANCEL) return "CANCEL RESEARCH";
    return kind == SC64_GHOST_TECH_OCULAR ? "OCULAR IMPLANTS" : "MOEBIUS REACTOR";
}
#endif
