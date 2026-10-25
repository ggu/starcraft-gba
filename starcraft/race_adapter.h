#ifndef SC64_RACE_ADAPTER_H
#define SC64_RACE_ADAPTER_H

#include "race_skirmish.h"
#ifdef SC64_P3_PRODUCTION_EXIT
#ifndef SC64_P3_RUNTIME
#error "SC64_P3_PRODUCTION_EXIT requires the P3 campaign runtime"
#endif
#include "sc64_production_exit.h"
#endif
#ifdef SC64_Z5_RUNTIME
#include "sc64_z5_hp_regen.h"
#endif
#ifdef SC64_P7_BATTERY_ENABLED
#include "sc64_p7_battery_rules.h"
#include "sc64_p7_power_geometry.h"
#endif

#ifdef SC64_T5_RUNTIME
static int race_t5_active(void);
static int race_t5_starport_has_control_tower(int producer);
#endif

/* Live owner-independent race adapter. Include from game.c after engine
 * helpers and before the frame/update hooks that call these functions. */
static const int race_bot_owner_ids[3] = { ENEMY, RESCUABLE, 4 };
static int race_base_x[4] = { 160, 864, 864, 160 };
static int race_base_y[4] = { 160, 864, 160, 864 };

static void race_skirmish_geometry(void)
{
    static const int original_x[4]={160,864,864,160};
    static const int original_y[4]={160,864,160,864};
    race_cols=race_rows=64;
    for(int i=0;i<4;i++){race_base_x[i]=original_x[i];race_base_y[i]=original_y[i];}
#ifdef SC64_IMPORTED_SKIRMISH
    race_skirmish_map=limit(race_skirmish_map,0,SC64_SKIRMISH_IMPORTED_COUNT);
    if(race_skirmish_map>0){
        const Sc64SkirmishMap *map=&sc64_skirmish_maps[race_skirmish_map-1];
        race_cols=map->width;race_rows=map->height;
        for(int i=0;i<4;i++){race_base_x[i]=map->start_x[i];race_base_y[i]=map->start_y[i];}
    }
#else
    race_skirmish_map=0;
#endif
}

static int race_bot_index(int owner)
{
    int i;
    for (i = 0; i < race_bots && i < 3; ++i)
        if (race_bot_owner_ids[i] == owner) return i;
    return -1;
}

static int race_owner_identity(int owner)
{ return owner >= 0 && owner < 5 ? race_player[owner] : -1; }

static int race_owner_minerals(int owner)
{
    return owner == SIDE ? minerals : owner >= 0 && owner < 5 ? race_minerals[owner] : 0;
}

static int race_owner_gas(int owner)
{
    return owner == SIDE ? gas : owner >= 0 && owner < 5 ? race_gas[owner] : 0;
}

static void race_add_owner_resources(int owner, int ore, int vespene)
{
    int value;
    if (owner == SIDE) {
        minerals += ore; gas += vespene;
        if (minerals < 0) minerals = 0;
        if (gas < 0) gas = 0;
    } else if (owner >= 0 && owner < 5) {
        value = race_minerals[owner] + ore;
        race_minerals[owner] = value < 0 ? 0 : value > 65535 ? 65535 : value;
        value = race_gas[owner] + vespene;
        race_gas[owner] = value < 0 ? 0 : value > 65535 ? 65535 : value;
    }
}

static int race_owner_supply(int owner)
{
    int i, total = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) {
            const Sc64RaceUnitRule *r = sc64_race_unit_rule(u[i].type);
#ifdef SC64_Z7_EGG_PRODUCTION
            if (r) total += unit_slot_supply(&u[i]);
#else
            if (r) total += r->supply;
#endif
        }
    return total;
}

static int race_owner_supply_cap(int owner)
{
    int i, total = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) && b[i].owner == owner) {
        const Sc64RaceBuildingRule *r = sc64_race_building_rule(b[i].type);
        if (r) total += r->supply_provided;
    }
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner)
        total += sc64_race_unit_supply_provided(u[i].type);
    return total;
}

static int race_owner_queued_supply(int owner)
{
    int i, j, total = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == owner)
        for (j = 0; j < b[i].queue_count; ++j) {
            const Sc64RaceUnitRule *r = sc64_race_unit_rule(b[i].queue[(b[i].queue_head + j) % 5]);
            if (r) total += r->supply * (b[i].queue[(b[i].queue_head + j) % 5] == U_ZERGLING ? 2 : 1);
        }
    return total;
}

static int race_owner_reserved_supply(int owner)
{
    return race_owner_supply(owner) + race_owner_queued_supply(owner);
}

#ifdef SC64_Z7_EGG_PRODUCTION
/* Zerg production is attributed to the town hall in this adapter, while
 * Brood War applies products to individual larvae. Larvae persist through a
 * Lair/Hive morph; unfinished new Hatcheries still have no usable larvae. */
static int race_z7_morphing_egg_hall(int owner, int producer)
{
    return race_z7_active() && owner >= 0 && owner < 5 &&
           race_owner_identity(owner) == R_ZERG &&
           producer >= 0 && producer < MAX_BUILDINGS &&
           b[producer].alive && b[producer].owner == owner &&
           b[producer].under_construction &&
           (b[producer].type == B_LAIR || b[producer].type == B_HIVE);
}

/* Egg rows are independent timers. The product and remaining clock live in
 * fields already serialized for every Unit, so no producer link or extra
 * mission state is needed. Morphing keeps the Egg slot and its HP fraction. */
static int __attribute__((noinline)) race_z7_egg_tick_unit(int slot)
{
    Unit *q;
    const Sc64RaceUnitRule *product;
    int old_hp, new_hp, old_supply, owner, x, y, second;
    if (slot < 0 || slot >= MAX_UNITS) return 1;
    q = &u[slot];
    if (!q->alive || q->type != U_Z7_EGG) return 1;
    if (!race_z7_active() || !sc64_z7_egg_product_valid(q->target) ||
        q->work_timer > sc64_z7_egg_duration(q->target)) {
        /* Invalid transient data is never allowed to turn into a free unit. */
        if (q->owner == SIDE) supply_used -= unit_slot_supply(q);
        q->alive = 0;
        q->selected = 0;
        return 1;
    }
    if (q->work_timer > 0) --q->work_timer;
    if (q->work_timer > 0) return 1;

    product = sc64_race_unit_rule(q->target);
    owner = q->owner;
    x = q->x;
    y = q->y;
    old_hp = q->hp;
    old_supply = unit_slot_supply(q);
    new_hp = product->hp > 0 ? (old_hp * product->hp) / 200 : 1;
    if (new_hp < 1) new_hp = 1;

    q->type = q->target;
#ifdef SC64_COLLISION_CLASS_CACHE
    /* The cached class is keyed by unit slot, while this Egg keeps its slot
     * and changes type at birth. */
    sc64_unit_collision_class_invalidate(slot);
#endif
    q->maxhp = product->hp;
    q->hp = new_hp > q->maxhp ? q->maxhp : new_hp;
    q->shield = product->shields;
    q->shield_remainder = 0;
    q->armor = sc64_race_unit_armor(q->type);
    q->weapon = owner == SIDE ? weapons_level : 0;
    q->raw_type = sc64_race_raw_unit(q->type);
    q->raw_owner = race_z7_product_source_owner(owner);
    q->order = ORDER_IDLE;
    q->target = -1;
    q->tx = x;
    q->ty = y;
    q->carry = 0;
    q->build = -1;
    q->work_timer = 0;
    q->path_timer = 0;
    q->path_retry = 0;
    q->path_x = x;
    q->path_y = y;
    q->flags = 0;
    if (owner == SIDE) supply_used += unit_supply(q->type) - old_supply;

    /* Brood War attempts the paired Ling as one birth event. If the unit pool
     * is full, it still completes the first Ling and does not retry later. */
    if (sc64_z7_egg_product_count(q->type) == 2) {
        second = spawn_unit(owner, q->type, x + 8 < WORLD_W - 8 ? x + 8 : x - 8, y);
        (void)second;
    }
    return 1;
}

static int race_z7_egg_cancel_available(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && u[i].owner == SIDE &&
            u[i].type == U_Z7_EGG && sc64_z7_egg_product_valid(u[i].target))
            return 1;
    return 0;
}

static int race_z7_egg_cancel_selected(void)
{
    int canceled = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        const Sc64RaceUnitRule *product;
        if (!q->alive || !q->selected || q->owner != SIDE ||
            q->type != U_Z7_EGG || !sc64_z7_egg_product_valid(q->target))
            continue;
        product = sc64_race_unit_rule(q->target);
        race_add_owner_resources(q->owner, product->minerals, product->gas);
        supply_used -= unit_slot_supply(q);
        q->alive = 0;
        q->selected = 0;
        ++canceled;
    }
    return canceled;
}
#endif

static int race_owner_has_prereq(int owner, unsigned short prereq)
{
    int i;
    if (!prereq) return 1;
#ifdef SC64_Z7_EGG_PRODUCTION
    for (i = 0; i < MAX_BUILDINGS; ++i) if (
        (building_complete(i) || race_z7_morphing_egg_hall(owner, i)) &&
        b[i].owner == owner) {
#else
    for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) && b[i].owner == owner) {
#endif
        switch (b[i].type) {
        case B_HATCHERY: case B_LAIR: case B_HIVE:
            if (prereq & SC_PR_HATCHERY) prereq &= (unsigned short)~SC_PR_HATCHERY;
#ifdef SC64_ZP_CAMPAIGN_STORAGE
            /* A morphing Lair still has the Hatchery source tier; a Hive
             * morph still has the completed Lair source tier. */
#ifdef SC64_Z7_EGG_PRODUCTION
            if (b[i].type != B_HATCHERY &&
                (!b[i].under_construction || b[i].type == B_HIVE))
                prereq &= (unsigned short)~SC_PR_LAIR;
#else
            if (b[i].type != B_HATCHERY) prereq &= (unsigned short)~SC_PR_LAIR;
#endif
#endif
            break;
#ifdef SC64_ZP_CAMPAIGN_STORAGE
        case B_SPIRE: case B_GREATER_SPIRE: prereq &= (unsigned short)~SC_PR_SPIRE; break;
#endif
        case B_SPAWNING_POOL: if (prereq & SC_PR_SPAWNING_POOL) prereq &= (unsigned short)~SC_PR_SPAWNING_POOL; break;
        case B_HYDRALISK_DEN: if (prereq & SC_PR_HYDRALISK_DEN) prereq &= (unsigned short)~SC_PR_HYDRALISK_DEN; break;
        case B_NEXUS: if (prereq & SC_PR_NEXUS) prereq &= (unsigned short)~SC_PR_NEXUS; break;
        case B_GATEWAY: if (prereq & SC_PR_GATEWAY) prereq &= (unsigned short)~SC_PR_GATEWAY; break;
        case B_CYBERNETICS_CORE: if (prereq & SC_PR_CYBERNETICS_CORE) prereq &= (unsigned short)~SC_PR_CYBERNETICS_CORE; break;
        case B_PYLON: if (prereq & SC_PR_PYLON) prereq &= (unsigned short)~SC_PR_PYLON; break;
        case B_FORGE: if (prereq & SC_PR_FORGE) prereq &= (unsigned short)~SC_PR_FORGE; break;
        case B_DEPOT: if (prereq & SC_PR_DEPOT) prereq &= (unsigned short)~SC_PR_DEPOT; break;
        case B_BARRACKS: if (prereq & SC_PR_BARRACKS) prereq &= (unsigned short)~SC_PR_BARRACKS; break;
#ifdef SC64_T5_RUNTIME
        case B_FACTORY: if (prereq & SC_PR_FACTORY) prereq &= (unsigned short)~SC_PR_FACTORY; break;
        case B_CONTROL_TOWER: if (prereq & SC_PR_CONTROL_TOWER) prereq &= (unsigned short)~SC_PR_CONTROL_TOWER; break;
#endif
        }
    }
    return prereq == 0;
}

static int race_owner_has_type(int owner, int type)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return 1;
    return 0;
}

static int race_owner_refinery(int owner)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (building_complete(i) && b[i].owner == owner &&
            (b[i].type == B_REFINERY || b[i].type == B_EXTRACTOR || b[i].type == B_ASSIMILATOR)) return i;
    return -1;
}

static int race_site_powered(int owner, int x, int y)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (building_complete(i) && b[i].owner == owner && b[i].type == B_PYLON &&
            dist2(x, y, b[i].x, b[i].y) <= 144 * 144) return 1;
    return 0;
}

#ifdef SC64_P7_BATTERY_ENABLED
static int race_p7_battery_site_powered(int owner,int x,int y)
{
    if (!race_p7_active() || owner!=SIDE) return 0;
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (building_complete(i) && b[i].type==B_PYLON &&
            b[i].raw_type==156 && b[i].owner==owner && b[i].raw_owner==7 &&
            sc64_p7_in_psi_range(x-b[i].x,y-b[i].y)) return 1;
    return 0;
}
#endif

static int race_site_creep(int owner, int x, int y)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
#ifdef SC64_Z7_EGG_PRODUCTION
        if ((building_complete(i) || race_z7_morphing_egg_hall(owner, i)) &&
#else
        if (building_complete(i) &&
#endif
            b[i].owner == owner &&
            sc64_race_is_zerg_townhall(b[i].type) &&
            dist2(x, y, b[i].x, b[i].y) <= 160 * 160) return 1;
    return 0;
}

static int race_can_site(int owner, int type, int x, int y)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
    int race = race_owner_identity(owner);
#ifdef SC64_Z7_EGG_PRODUCTION
    unsigned short prereq = r ? r->prereq : 0;
    if (race_z7_active() && race == R_ZERG && type == B_EXTRACTOR)
        prereq &= (unsigned short)~SC_PR_HATCHERY;
#endif
    if (!r || r->race != race ||
#ifdef SC64_Z7_EGG_PRODUCTION
        !race_owner_has_prereq(owner, prereq)) return 0;
#else
        !race_owner_has_prereq(owner, r->prereq)) return 0;
#endif
    /* Imported Lairs/Hives and Greater Spires are recognized structures,
       but this adapter does not implement their morph/build operations. */
    if (type == B_LAIR || type == B_HIVE || type == B_GREATER_SPIRE) return 0;
#ifdef SC64_P7_BATTERY_ENABLED
    if ((r->flags & SC_RACE_F_REQUIRES_POWER) && race_p7_active() &&
        type==B_P7_SHIELD_BATTERY) {
        if (!race_p7_battery_site_powered(owner,x,y)) return 0;
    } else
#endif
    if ((r->flags & SC_RACE_F_REQUIRES_POWER) && !race_site_powered(owner, x, y)) return 0;
    if (race == R_ZERG && type != B_HATCHERY && type != B_EXTRACTOR && !race_site_creep(owner, x, y)) return 0;
    if (type == B_EXTRACTOR || type == B_ASSIMILATOR) {
        if (geyser_at(x, y) < 0) return 0;
    }
    return 1;
}

static int race_owner_worker(int owner, int type, int x, int y)
{
    int i, best = -1, d, best_d = 0x7fffffff;
    for (i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != owner || u[i].type != type || u[i].order == ORDER_BUILD) continue;
        d = dist2(x, y, u[i].x, u[i].y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

static int race_queue_unit(int owner, int producer, int type)
{
    int tail, count, reserved, cap;
#ifdef SC64_Z7_EGG_PRODUCTION
    int egg_job = 0;
#endif
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(type);
    Sc64RaceBank bank;
#ifdef SC64_Z7_SCOURGE_COMBAT
    /* This staged producer uses Z7's paired Eggs. Other modes retain their
       existing product list until independent production is qualified there. */
    if (type == U_SCOURGE && !race_z7_active()) return 0;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_airborne(producer)) return 0;
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    egg_job = race_z7_active() && owner >= 0 && owner < 5 &&
        race_owner_identity(owner) == R_ZERG && sc64_z7_egg_product_valid(type) &&
        (owner != ENEMY || type == U_ZERGLING);
    if (!r || producer < 0 || producer >= MAX_BUILDINGS || r->race != race_owner_identity(owner) ||
        (!building_complete(producer) && !(egg_job && race_z7_morphing_egg_hall(owner, producer))) ||
        b[producer].owner != owner || !sc64_race_producer_compatible(b[producer].type, r->producer) ||
        !race_owner_has_prereq(owner, r->prereq)) return 0;
    if (!egg_job && b[producer].queue_count >= 5) return 0;
#else
    if (!r || producer < 0 || producer >= MAX_BUILDINGS || r->race != race_owner_identity(owner) || !building_complete(producer) ||
        b[producer].owner != owner || !sc64_race_producer_compatible(b[producer].type, r->producer) ||
        b[producer].queue_count >= 5 || !race_owner_has_prereq(owner, r->prereq)) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (!race_t5_unit_product_available(owner, b[producer].raw_owner, type)) return 0;
    if (type == U_DROPSHIP && race_t5_active() &&
        !race_t5_starport_has_control_tower(producer)) return 0;
#endif
#ifdef SC64_T6_RUNTIME
    if (!race_t6_unit_product_available(owner, b[producer].raw_owner, type)) return 0;
    if (type == U_DROPSHIP && race_t6_active() &&
        !race_t6_can_train_dropship(producer)) return 0;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (!race_late_unit_product_available(owner, b[producer].raw_owner, type)) return 0;
    if (type == U_DROPSHIP && race_late_active() &&
        !race_t6_can_train_dropship(producer)) return 0;
#endif
    {
        const Sc64RaceBuildingRule *br = sc64_race_building_rule(b[producer].type);
        if (br && (br->flags & SC_RACE_F_REQUIRES_POWER) &&
            !race_site_powered(owner, b[producer].x, b[producer].y)) return 0;
    }
    if ((r->flags & SC_RACE_F_LARVA) && b[producer].larva < 1) return 0;
    count = type == U_ZERGLING ? 2 : 1;
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (race_p7_active() && !race_p7_carrier_training_slots_available((unsigned)count)) return 0;
#endif
#ifdef SC64_P4_RUNTIME
    if (!race_mode && race_selected_mission == SC64_P4_GLOBAL_ID) {
        unsigned committed = race_p4_reserved_units();
        unsigned free_slots = 0;
        for (int slot = 0; slot < MAX_UNITS; ++slot) free_slots += !u[slot].alive;
        for (int slot = 0; slot < MAX_BUILDINGS; ++slot) if (b[slot].alive)
            for (int q = 0; q < b[slot].queue_count; ++q) {
                int queued = b[slot].queue[(b[slot].queue_head + q) % 5];
                committed += queued == U_ZERGLING ? 2u : 1u;
            }
        if (committed + (unsigned)count > free_slots) return 0;
    }
#endif
#ifdef SC64_P5_RUNTIME
    if (!race_mode && race_selected_mission == SC64_P5_GLOBAL_ID) {
        unsigned committed = race_p5_reserved_units();
        unsigned free_slots = 0;
        for (int slot = 0; slot < MAX_UNITS; ++slot) free_slots += !u[slot].alive;
        for (int slot = 0; slot < MAX_BUILDINGS; ++slot) if (b[slot].alive)
            for (int q = 0; q < b[slot].queue_count; ++q) {
                int queued = b[slot].queue[(b[slot].queue_head + q) % 5];
                committed += queued == U_ZERGLING ? 2u : 1u;
            }
        if (committed + (unsigned)count > free_slots) return 0;
    }
#endif
    reserved = race_owner_reserved_supply(owner);
    cap = race_owner_supply_cap(owner);
    /* Zero-supply units (notably Overlords) must remain producible while
       over the current cap so the player can restore supply. */
#ifdef SC64_Z7_EGG_PRODUCTION
    if (egg_job) count = sc64_z7_egg_product_count(type);
#endif
    if (r->supply && reserved + r->supply * count > cap) return 0;
#ifdef SC64_Z7_EGG_PRODUCTION
    if (egg_job) {
        int free_slots = 0;
        for (int slot = 0; slot < MAX_UNITS; ++slot) free_slots += !u[slot].alive;
        if (!free_slots) return 0;
    }
#endif
    bank.minerals = (unsigned short)race_owner_minerals(owner);
    bank.gas = (unsigned short)race_owner_gas(owner);
    if (!sc64_race_pay_unit(&bank, type)) return 0;
    if (owner == SIDE) { minerals = bank.minerals; gas = bank.gas; }
    else { race_minerals[owner] = bank.minerals; race_gas[owner] = bank.gas; }
    if (r->flags & SC_RACE_F_LARVA) --b[producer].larva;
#ifdef SC64_Z7_EGG_PRODUCTION
    if (egg_job) {
        int x = b[producer].x + 20, y = b[producer].y + 4;
        int slot;
        if (x >= WORLD_W - 8) x = b[producer].x - 20;
        slot = spawn_unit(owner, U_Z7_EGG, x, y);
        if (slot < 0) {
            /* Admission checked for a slot before debiting the bank. This is
             * defensive against a future synchronous spawn side effect. */
            race_add_owner_resources(owner, r->minerals, r->gas);
            ++b[producer].larva;
            return 0;
        }
        u[slot].target = type;
        u[slot].work_timer = (u16)sc64_z7_egg_duration(type);
        u[slot].raw_type = 36;
        u[slot].maxhp = 200;
        u[slot].hp = 200;
        u[slot].armor = 10;
        if (owner == SIDE) supply_used += sc64_z7_egg_reserved_supply(type);
        return 1;
    }
#endif
    tail = (b[producer].queue_head + b[producer].queue_count) % 5;
    b[producer].queue[tail] = type;
    ++b[producer].queue_count;
    return 1;
}

static int race_start_build(int owner, int worker, int type, int x, int y)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
    Sc64RaceBank bank;
    int slot;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (!race_late_building_product_available(owner,
            worker >= 0 && worker < MAX_UNITS ? u[worker].raw_owner : -1,
            type)) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (type == B_CONTROL_TOWER) return 0; /* Starport-only add-on path. */
    if (!race_t5_building_product_available(owner,
            worker >= 0 && worker < MAX_UNITS ? u[worker].raw_owner : -1,
            type)) return 0;
#endif
#ifdef SC64_T6_RUNTIME
    if (!race_t6_building_product_available(owner,
            worker >= 0 && worker < MAX_UNITS ? u[worker].raw_owner : -1,
            type)) return 0;
#endif
    if (!r || r->race != race_owner_identity(owner) || worker < 0 || worker >= MAX_UNITS ||
        !u[worker].alive || u[worker].owner != owner || u[worker].type != r->worker ||
        !race_can_site(owner, type, x, y) || !can_place(type, x, y)) return 0;
    slot = find_build_slot();
    if (slot < 0) return 0;
    bank.minerals = (unsigned short)race_owner_minerals(owner);
    bank.gas = (unsigned short)race_owner_gas(owner);
    if (!sc64_race_pay_building(&bank, type)) return 0;
    if (owner == SIDE) { minerals = bank.minerals; gas = bank.gas; }
    else { race_minerals[owner] = bank.minerals; race_gas[owner] = bank.gas; }
    slot = spawn_build(owner, type, x, y, 1);
    if (slot < 0) return 0;
    /* Non-Terran construction starts only after the worker reaches the site. */
    if (race_owner_identity(owner) != R_TER) b[slot].build_progress = -1;
#ifdef SC64_T6_RUNTIME
    if (u[worker].order == ORDER_REPAIR) {
        u[worker].carry = 0; u[worker].work_timer = 0;
    }
#endif
    u[worker].order = ORDER_BUILD;
    u[worker].target = slot;
    u[worker].tx = x;
    u[worker].ty = y;
    u[worker].path_timer = 0;
    u[worker].path_retry = 0;
    return 1;
}

#ifdef SC64_ZP_CAMPAIGN_STORAGE
/* In-place Lair upgrade: keep source provenance, position and larva slots.
 * The existing bounded construction clock supplies the authored 30 Hz timing.
 * An empty queue avoids silently discarding a paid larva product. */
static int race_start_lair_morph(int building)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(B_LAIR);
    Sc64RaceBank bank;
    if (race_human != R_ZERG || building < 0 || building >= MAX_BUILDINGS ||
        !building_complete(building) || b[building].owner != SIDE ||
        b[building].type != B_HATCHERY || b[building].queue_count ||
        !race_owner_has_prereq(SIDE, r->prereq)) return 0;
    bank.minerals = (unsigned short)minerals;
    bank.gas = (unsigned short)gas;
    if (!sc64_race_pay_building(&bank, B_LAIR)) return 0;
    minerals = bank.minerals; gas = bank.gas;
    b[building].type = B_LAIR;
    b[building].raw_type = sc64_race_raw_building(B_LAIR);
    b[building].maxhp = r->hp;
    b[building].under_construction = 1;
    b[building].build_progress = 0;
    invalidate_path_grid();
    return 1;
}
#endif

/* Called by the worker's build-arrival branch. Protoss warp-in returns the
 * Probe immediately. Zerg construction consumes the Drone and its supply. */
#ifdef SC64_Z6_RUNTIME
static int __attribute__((noinline)) race_builder_arrived(int worker)
#else
static int race_builder_arrived(int worker)
#endif
{
    int building;
    if (worker < 0 || worker >= MAX_UNITS || !u[worker].alive || u[worker].order != ORDER_BUILD) return 0;
    building = u[worker].target;
    if (building < 0 || building >= MAX_BUILDINGS || !b[building].alive) return 0;
    if (race_owner_identity(u[worker].owner) == R_TER) return 0;
    if (dist2(u[worker].x, u[worker].y, b[building].x, b[building].y) > 36 * 36) return 0;
    if (b[building].build_progress < 0) b[building].build_progress = 0;
    if (race_owner_identity(u[worker].owner) == R_ZERG) {
        int supply = unit_supply(u[worker].type);
        u[worker].carry = 0;
#ifdef SC64_Z6_RUNTIME
        if (race_z6_active() && u[worker].owner==SIDE &&
            u[worker].raw_type==41 && u[worker].raw_owner==3 &&
            race_z6.human_drone_morphs<4u) ++race_z6.human_drone_morphs;
#endif
        if (u[worker].owner == SIDE) supply_used -= supply;
        u[worker].alive = 0;
    } else {
        u[worker].order = ORDER_IDLE;
        u[worker].target = -1;
    }
    return 1;
}

static void race_tick_state(void)
{
    int i,used=0,cap=0;
#ifdef SC64_Z5_RUNTIME
    int z5_regeneration_active = race_z5_active();
#endif
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        (!b[i].under_construction ||
#ifdef SC64_Z7_EGG_PRODUCTION
         race_z7_morphing_egg_hall(b[i].owner, i) ||
#endif
         0) &&
        b[i].owner >= 0 && b[i].owner < 5 && race_owner_identity(b[i].owner) == R_ZERG &&
        sc64_race_is_zerg_townhall(b[i].type)) {
        if (b[i].larva < 3 && ++b[i].larva_timer >= 428) {
            b[i].larva_timer = 0;
            if (b[i].larva < 3) ++b[i].larva;
        }
    }
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].type >= 0 && u[i].type < SC_RU_COUNT) {
        const Sc64RaceUnitRule *r = sc64_race_unit_rule(u[i].type);
#ifdef SC64_Z5_RUNTIME
        if (z5_regeneration_active) {
            int raw_type = -1;
            if (u[i].type == U_ZERGLING)
                raw_type = SC64_Z5_HP_REGEN_RAW_ZERGLING;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
            else if (u[i].type == U_ZP_INFESTED_KERRIGAN)
                raw_type = SC64_Z5_HP_REGEN_RAW_INFESTED_KERRIGAN;
#endif
            if (raw_type >= 0)
                (void)sc64_z5_hp_regen_tick(raw_type, &u[i].hp, u[i].maxhp,
                                             &u[i].shield_remainder, 0);
        }
#endif
        if(u[i].owner==SIDE){used+=unit_slot_supply(&u[i]);cap+=sc64_race_unit_supply_provided(u[i].type);}
        if (r->shields && u[i].shield < r->shields) {
            unsigned short value = (unsigned short)u[i].shield;
            unsigned short rem = (unsigned short)u[i].shield_remainder;
#ifdef SC64_P7_BATTERY_ENABLED
            if (race_p7_active()) {
                unsigned shield_value=value, shield_rem=rem;
                sc64_p7_battery_passive_shield_tick(&shield_value,r->shields,
                                                     &shield_rem);
                value=(unsigned short)shield_value;
                rem=(unsigned short)shield_rem;
            } else
#endif
                sc64_race_shield_tick_30fps(&value, r->shields, &rem);
            u[i].shield = value;
            u[i].shield_remainder = rem;
        }
    }
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if(b[i].owner==SIDE&&!b[i].under_construction)cap+=sc64_race_building_rules[b[i].type].supply_provided;
        unsigned short max = sc64_race_building_shields(b[i].type);
        if (max && b[i].shield < max) {
            unsigned short value = (unsigned short)b[i].shield;
            unsigned short rem = (unsigned short)b[i].shield_remainder;
            sc64_race_shield_tick_30fps(&value, max, &rem);
            b[i].shield = value;
            b[i].shield_remainder = rem;
        }
#ifdef SC64_P7_BATTERY_ENABLED
        if (race_p7_active() && b[i].type==B_P7_SHIELD_BATTERY &&
            !b[i].under_construction && b[i].queue_count==0) {
            unsigned energy=(unsigned)b[i].queue_progress;
            unsigned phase=(unsigned)b[i].level;
            sc64_p7_battery_regenerate(&energy,&phase);
            b[i].queue_progress=(int)energy;
            b[i].level=(int)phase;
        }
#endif
    }
    supply_used=used;supply_cap=cap;
}

#ifdef SC64_CONSTRUCTION_HP
static int race_construction_growth_is_morph(const Building *building)
{
    if (building->type == B_LAIR || building->type == B_HIVE ||
        building->type == B_GREATER_SPIRE) return 1;
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    if (building->flags & (SC64_Z7_SUNKEN_MORPH_FLAG |
                           SC64_Z7_SPORE_MORPH_FLAG)) return 1;
#endif
    return 0;
}

static int race_construction_morph_source_max(int type)
{
    int source = -1;
    if (type == B_LAIR) source = B_HATCHERY;
    else if (type == B_HIVE) source = B_LAIR;
    else if (type == B_GREATER_SPIRE) source = B_SPIRE;
    return source >= 0 ? sc64_race_building_rule(source)->hp : 0;
}

/* Keep optional integer construction math out of ARM hot-path inlining. */
static void __attribute__((noinline))
race_construction_hp_progress(int i, int progress, int total)
{
    Building *building;
    int gain, shield_max;
    if (i < 0 || i >= MAX_BUILDINGS || progress <= 0 || total <= 0) return;
    building = &b[i];
    if (!building->alive || !building->under_construction ||
        building->hp <= 0 || race_construction_growth_is_morph(building)) return;
    gain = sc64_construction_hp_gain(building->maxhp, progress, total);
    if (gain > 0) {
        building->hp += gain;
        if (building->hp > building->maxhp) building->hp = building->maxhp;
    }
    shield_max = sc64_race_building_shields(building->type);
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    {
        const Sc64ZpBuildingFact *fact = sc64_zp_building_fact(building->type);
        if (fact) shield_max = fact->shields;
    }
#endif
    if (shield_max > building->shield && shield_max > 0) {
        gain = sc64_construction_shield_gain(shield_max, progress, total);
        building->shield += gain;
        if (building->shield > shield_max) building->shield = shield_max;
    }
}
#endif

static void race_complete_building(int i)
{
    const Sc64RaceBuildingRule *r;
    if (i < 0 || i >= MAX_BUILDINGS || !b[i].alive || !b[i].under_construction) return;
    r = sc64_race_building_rule(b[i].type);
    b[i].under_construction = 0;
#ifdef SC64_CONSTRUCTION_HP
    {
        int old_max = race_construction_morph_source_max(b[i].type);
        if (old_max > 0) {
            b[i].hp = sc64_construction_morph_hp(b[i].hp, old_max, b[i].maxhp);
        }
        else if (!race_construction_growth_is_morph(&b[i]) &&
                 b[i].hp > b[i].maxhp)
            b[i].hp = b[i].maxhp;
    }
#else
    b[i].hp = b[i].maxhp;
#endif
    b[i].build_progress = 0;
#ifdef SC64_NEAREST_RESOURCE_DEPOT
    if (race_resource_depot_type(b[i].type)) race_depot_cache_clear_no_home();
#endif
    if (r && r->supply_provided && b[i].owner == SIDE) supply_cap += r->supply_provided;
    if (sc64_race_is_zerg_townhall(b[i].type)) b[i].larva = 3;
}

#ifdef SC64_P3_PRODUCTION_EXIT
typedef struct {
    int producer_type;
    int reserved;
    int reserved_x, reserved_y, reserved_type;
} Sc64P3ProductionExitContext;

static int sc64_p3_exit_in_bounds(void *opaque, int x, int y)
{
    (void)opaque;
    return x >= 0 && y >= 0 && x < WORLD_W && y < WORLD_H;
}

static int sc64_p3_exit_terrain_fits(void *opaque, int x, int y)
{
    (void)opaque;
    return !terrain_blocked(x / 16, y / 16);
}

static int sc64_p3_exit_radius(int type)
{
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    return sc64_game_unit_collision_radius(type);
#else
    (void)type;
    return 6;
#endif
}

static int sc64_p3_exit_building_blocked(void *opaque, int x, int y, int flying)
{
    int radius = sc64_p3_exit_radius(((Sc64P3ProductionExitContext *)opaque)->producer_type);
    /* Ground structures block ground exits, but a flying producer can emerge
     * over them. Unit collision remains flight-class-aware below. */
    if (flying) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive) continue;
        if (b[i].type == B_ZERG_BEACON) continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (!sc64_t4_building_collidable(i)) continue;
#endif
        if (dist2(x, y, b[i].x, b[i].y) < (radius + 14) * (radius + 14)) return 1;
    }
    return 0;
}

static int sc64_p3_exit_unit_blocked(void *opaque, int x, int y, int flying)
{
    Sc64P3ProductionExitContext *ctx = (Sc64P3ProductionExitContext *)opaque;
    int radius = sc64_p3_exit_radius(ctx->producer_type);
    if (ctx->reserved &&
        sc64_race_unit_flying(ctx->reserved_type) == flying &&
        dist2(x, y, ctx->reserved_x, ctx->reserved_y) < radius * radius)
        return 1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i])) continue;
        if (sc64_race_unit_flying(u[i].type) != flying) continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (!sc64_game_unit_collidable(u[i].type)) continue;
#endif
        int other_radius = sc64_p3_exit_radius(u[i].type);
        int required = other_radius > radius ? other_radius : radius;
        if (dist2(x, y, u[i].x, u[i].y) < required * required) return 1;
    }
    return 0;
}

static int sc64_p3_exit_reachable(void *opaque, int from_x, int from_y, int x, int y)
{
    Sc64P3ProductionExitContext *ctx = (Sc64P3ProductionExitContext *)opaque;
    int next_x, next_y;
    if (sc64_race_unit_flying(ctx->producer_type)) return 1;
    /* path_next_tile treats blocked goals as approach targets. A production
     * point must be open itself, or it could teleport into a sealed tile. */
    if (!is_traversable(x / 16, y / 16, -1)) return 0;
    return path_next_tile(from_x, from_y, x, y, &next_x, &next_y);
}

static int sc64_p3_find_production_exit(int producer, int type,
                                        int preferred_x, int preferred_y,
                                        int *out_x, int *out_y,
                                        Sc64P3ProductionExitContext *ctx)
{
    Sc64ProductionExitOps ops;
    ctx->producer_type = type;
    ops.context = ctx;
    ops.in_bounds = sc64_p3_exit_in_bounds;
    ops.terrain_fits = sc64_p3_exit_terrain_fits;
    ops.building_blocked = sc64_p3_exit_building_blocked;
    ops.unit_blocked = sc64_p3_exit_unit_blocked;
    ops.reachable = sc64_p3_exit_reachable;
    return sc64_production_exit_find(&ops, b[producer].x, b[producer].y,
                                     preferred_x, preferred_y,
                                     sc64_race_unit_flying(type), out_x, out_y);
}
#endif

static void race_tick_buildings(void)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (race_late_airborne(i)) continue;
#endif
        int type, frames, x, y, supply, owner;
        const Sc64RaceBuildingRule *br;
        if (b[i].under_construction) {
            br = sc64_race_building_rule(b[i].type);
#ifdef SC64_CONSTRUCTION_HP
            if (br && (br->race == R_ZERG || br->race == R_PROTOSS) &&
                b[i].build_progress >= 0) {
                int total = (br->build_frames * 5 + 3) / 4;
                int progress = ++b[i].build_progress;
                race_construction_hp_progress(i, progress, total);
                if (progress >= total) race_complete_building(i);
            }
#else
            if (br && (br->race == R_ZERG || br->race == R_PROTOSS) && b[i].build_progress >= 0 && ++b[i].build_progress >= (br->build_frames * 5 + 3) / 4)
                race_complete_building(i);
#endif
            continue;
        }
        if (!b[i].queue_count) continue;
        owner = b[i].owner;
        type = b[i].queue[b[i].queue_head];
        {
            const Sc64RaceUnitRule *ur = sc64_race_unit_rule(type);
            if (!ur) continue;
            const Sc64RaceBuildingRule *pr = sc64_race_building_rule(b[i].type);
            frames = (ur->build_frames * 5 + 3) / 4;
            if (pr && (pr->flags & SC_RACE_F_REQUIRES_POWER) && !race_site_powered(owner, b[i].x, b[i].y)) continue;
        }
        if (++b[i].queue_progress < frames) continue;
#ifdef SC64_P4_RUNTIME
        if (!race_mode && race_selected_mission == SC64_P4_GLOBAL_ID) {
            unsigned free_slots = 0;
            for (int slot = 0; slot < MAX_UNITS; ++slot) free_slots += !u[slot].alive;
            if (free_slots < race_p4_reserved_units() + (type == U_ZERGLING ? 2u : 1u)) {
                b[i].queue_progress = frames - 1;
                continue;
            }
        }
#endif
        x = b[i].x + 20; y = b[i].y + 4;
        if (x >= WORLD_W - 8) x = b[i].x - 20;
#ifdef SC64_P3_PRODUCTION_EXIT
        if (!race_mode && race_selected_mission == SC64_P3_GLOBAL_ID) {
            Sc64P3ProductionExitContext ctx;
            int first_x = x, first_y = y;
            if (type == U_ZERGLING) {
                int second_x, second_y, free_slots = 0;
                ctx.reserved = 0;
                for (int slot = 0; slot < MAX_UNITS; ++slot) free_slots += !u[slot].alive;
                if (free_slots < 2 ||
                    !sc64_p3_find_production_exit(i, type, x, y, &first_x, &first_y, &ctx)) {
                    b[i].queue_progress = frames > 8 ? frames - 8 : 0;
                    continue;
                }
                ctx.reserved = 1;
                ctx.reserved_x = first_x; ctx.reserved_y = first_y; ctx.reserved_type = type;
                if (!sc64_p3_find_production_exit(i, type, first_x + 8, first_y,
                                                  &second_x, &second_y, &ctx)) {
                    b[i].queue_progress = frames > 8 ? frames - 8 : 0;
                    continue;
                }
                int first_slot = spawn_unit(owner, U_ZERGLING, first_x, first_y);
                if (first_slot < 0 || spawn_unit(owner, U_ZERGLING, second_x, second_y) < 0) {
                    /* Capacity was checked before planning and no world state
                     * changes between the paired spawns; this is defensive. */
                    if (first_slot >= 0) u[first_slot].alive = 0;
                    b[i].queue_progress = frames > 8 ? frames - 8 : 0;
                    continue;
                }
            } else {
                int spawned;
                ctx.reserved = 0;
                if (!sc64_p3_find_production_exit(i, type, x, y, &first_x, &first_y, &ctx)) {
                    b[i].queue_progress = frames > 8 ? frames - 8 : 0;
                    continue;
                }
                spawned = spawn_unit(owner, type, first_x, first_y);
                if (spawned < 0) {
                    b[i].queue_progress = frames > 8 ? frames - 8 : 0;
                    continue;
                }
                if (sc64_race_is_worker(type)) {
                    int patch = nearest_mineral(first_x, first_y);
                    if (patch >= 0 && patch < MAX_MINERALS && m[patch].amount > 0) {
                        u[spawned].order = ORDER_MINE; u[spawned].target = patch; u[spawned].work_timer = 0;
                    }
                }
            }
            supply = sc64_race_unit_supply_provided(type);
            if (supply && owner == SIDE) supply_cap += supply;
            b[i].queue_head = (b[i].queue_head + 1) % 5;
            --b[i].queue_count;
            b[i].queue_progress = 0;
            continue;
        }
#endif
        if (type == U_ZERGLING) {
            int free_slots=0;
            for(int slot=0;slot<MAX_UNITS;slot++)if(!u[slot].alive)free_slots++;
            if(free_slots<2){b[i].queue_progress=frames-1;continue;}
            if (spawn_unit(owner, U_ZERGLING, x, y) < 0 || spawn_unit(owner, U_ZERGLING, x + 8, y) < 0) {
                b[i].queue_progress = frames - 1;
                continue;
            }
        } else {
            int spawned = spawn_unit(owner, type, x, y);
            if (spawned >= 0 && sc64_race_is_worker(type)) {
                int patch = nearest_mineral(x,y);
#ifndef SC64_IMPORTED_SKIRMISH
                if(race_mode){int base=owner==SIDE?0:owner==ENEMY?1:owner==RESCUABLE?2:3;
                    patch=base*4+((owner+frame/600)&3);}
#endif
                if (patch >= 0 && patch < MAX_MINERALS && m[patch].amount > 0) {
                    u[spawned].order = ORDER_MINE; u[spawned].target = patch; u[spawned].work_timer = 0;
                }
            }
            if (spawned < 0) {
            b[i].queue_progress = frames - 1;
            continue;
            }
        }
        supply = sc64_race_unit_supply_provided(type);
        if (supply && owner == SIDE) supply_cap += supply;
        b[i].queue_head = (b[i].queue_head + 1) % 5;
        --b[i].queue_count;
        b[i].queue_progress = 0;
    }
}

static void race_skirmish_init(void)
{
    static const int mdx[4] = { 56, 42, 56, 42 };
    static const int mdy[4] = { 24, 40, 8, 20 };
    int i, j, owner, worker_type, townhall, supply_building;
#ifdef SC64_P7_PROJECTILES_ENABLED
    race_p7_projectile_reset();
#endif
#ifdef SC64_ZP_FULL_STORAGE
    if (race_skirmish_map > 0) {
        const Sc64SkirmishMap *map;
        race_skirmish_geometry();
        map = &sc64_skirmish_maps[race_skirmish_map - 1];
        if (map->geyser_count > MAX_RACE_GEYSERS) {
            screen = SCREEN_PAUSE;
            toast(14);
            return;
        }
        for (unsigned g = 0; g < map->geyser_count; ++g)
            if (map->geysers[g].x >= race_cols * 16 ||
                map->geysers[g].y >= race_rows * 16) {
                screen = SCREEN_PAUSE;
                toast(14);
                return;
            }
    }
#endif
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b)); memset(m, 0, sizeof(m));
    memset(race_player, 0xff, sizeof(race_player));
    memset(race_minerals, 0, sizeof(race_minerals)); memset(race_gas, 0, sizeof(race_gas));
    race_skirmish_geometry();
    frame = 0; won = lost = 0; supply_used = 0; supply_cap = 0;
    minerals = 200; gas = 0; geyser_count = 4;
    for (i = 0; i < 4; ++i) {
        int sx = (i == 0 || i == 3) ? -1 : 1;
        int sy = (i == 0 || i == 2) ? 1 : -1;
        for (j = 0; j < 4; ++j) {
            int idx = i * 4 + j;
            m[idx].x = race_base_x[i] + sx * mdx[j];
            m[idx].y = race_base_y[i] + sy * mdy[j];
            m[idx].amount = 1500;
        }
        geysers[i].x = race_base_x[i] + (sx < 0 ? 64 : -64);
        geysers[i].y = race_base_y[i] + (sy < 0 ? 56 : -56);
    }
#ifdef SC64_IMPORTED_SKIRMISH
    if(race_skirmish_map>0){
        const Sc64SkirmishMap *map=&sc64_skirmish_maps[race_skirmish_map-1];
        memset(m,0,sizeof(m));
        for(i=0;i<map->minerals_count&&i<MAX_MINERALS;i++){
            m[i].x=map->minerals[i].x;m[i].y=map->minerals[i].y;
            m[i].amount=map->minerals[i].amount;m[i].style=176;
        }
        geyser_count=map->geyser_count;
        for(i=0;i<geyser_count;i++){geysers[i].x=map->geysers[i].x;geysers[i].y=map->geysers[i].y;}
    }
#endif
    for (i = 0; i < race_bots && i < 3; ++i)
        race_player[race_bot_owner_ids[i]] =
#ifdef SC64_IMPORTED_SKIRMISH
            limit(race_bot_races[i],0,2);
#else
            (race_human + i + 1) % 3;
#endif
    race_player[SIDE] = race_human;
    for (i = 0; i < 1 + race_bots && i < 4; ++i) {
        int base = i;
        owner = i == 0 ? SIDE : race_bot_owner_ids[i - 1];
        if (owner != SIDE) race_minerals[owner] = 200;
        switch (race_owner_identity(owner)) {
        case R_ZERG: townhall = B_HATCHERY; supply_building = -1; worker_type = U_DRONE; break;
        case R_PROTOSS: townhall = B_NEXUS; supply_building = B_PYLON; worker_type = U_PROBE; break;
        default: townhall = B_COMMAND; supply_building = B_DEPOT; worker_type = U_SCV; break;
        }
        spawn_build(owner, townhall, race_base_x[base], race_base_y[base], 0);
        if (supply_building >= 0)
            spawn_build(owner, supply_building, race_base_x[base] + ((base & 1) ? 48 : -48), race_base_y[base], 0);
        if (race_owner_identity(owner) == R_ZERG)
            spawn_unit(owner, U_OVERLORD, race_base_x[base] - 24, race_base_y[base] - 18);
        for (j = 0; j < 4; ++j) {
            int idx = base * 4 + j;
#ifdef SC64_IMPORTED_SKIRMISH
            if(race_skirmish_map>0){
                int picked[4]={-1,-1,-1,-1};
                for(int k=0;k<=j;k++){
                    int best=0x7fffffff;idx=-1;
                    for(int p=0;p<MAX_MINERALS;p++)if(m[p].amount>0){
                        int used=0;for(int q=0;q<k;q++)if(picked[q]==p)used=1;
                        int distance=dist2(m[p].x,m[p].y,race_base_x[base],race_base_y[base]);
                        if(!used&&distance<best){best=distance;idx=p;}
                    }
                    picked[k]=idx;
                }
                if(idx<0)continue;
            }
#endif
            int wi = spawn_unit(owner, worker_type, m[idx].x + 4, m[idx].y + 4);
            if (wi >= 0) {
                u[wi].order = ORDER_MINE;
                u[wi].target = idx;
                u[wi].work_timer = 0;
            }
        }
    }
    supply_cap = race_owner_supply_cap(SIDE);
    cx = race_base_x[0]; cy = race_base_y[0]; camx = camy = 0;
    selected_building = -1; mode = MODE_NORMAL; menu_open = 0;
    enemy_timer = 0; wave = 0; invalidate_path_grid(); update_exploration();
}

static int race_skirmish_site_for(int owner, int type, int *x, int *y)
{
    int base = owner == SIDE ? 0 : owner == ENEMY ? 1 : owner == RESCUABLE ? 2 : owner == 4 ? 3 : -1;
    static const int dx[8] = { 48,-48,0,0,48,-48,48,-48 };
    static const int dy[8] = { 0,0,48,-48,48,48,-48,-48 };
    int i;
    if (base < 0) return 0;
    for (int ring=0;ring<3;ring++) for (i = 0; i < 8; ++i) {
        int px = race_base_x[base] + dx[i]*(48+ring*32)/48;
        int py = race_base_y[base] + dy[i]*(48+ring*32)/48;
        int reserved_gas=0;
        for(int g=0;g<geyser_count;g++)if(dist2(px,py,geysers[g].x,geysers[g].y)<48*48)reserved_gas=1;
        if(!reserved_gas&&race_can_site(owner, type, px, py) && can_place(type, px, py)) { *x = px; *y = py; return 1; }
    }
    return 0;
}

static void race_tick_ai(void)
{
    int bot, owner, race, i, workers, army, producer, build_type, unit_type, x, y;
    if (frame % 60) return;
    for (bot = 0; bot < race_bots; ++bot) {
        owner = race_bot_owner_ids[bot]; race = race_owner_identity(owner);
        workers = army = 0; producer = -1;
        for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == owner) {
            if (sc64_race_is_worker(u[i].type)) ++workers;
            else if (sc64_race_unit_damage(u[i].type) > 0) ++army;
        }
        for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) && b[i].owner == owner &&
            sc64_race_producer_compatible(b[i].type,
                race == R_ZERG ? B_HATCHERY : race == R_PROTOSS ? B_NEXUS : B_COMMAND)) { producer = i; break; }
        /* Supply expansion is paid and finished before it raises the cap.
           Do not let worker queues permanently starve the Zerg combat army. */
        if(race_owner_supply_cap(owner)-race_owner_reserved_supply(owner)<=4) {
            int pending=0;
            if(race==R_ZERG&&producer>=0) {
                for(int j=0;j<b[producer].queue_count;j++)
                    if(b[producer].queue[(b[producer].queue_head+j)%5]==U_OVERLORD)pending=1;
                if(!pending)race_queue_unit(owner,producer,U_OVERLORD);
            } else {
                int supply_type=race==R_PROTOSS?B_PYLON:B_DEPOT;
                for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==owner&&
                    b[j].type==supply_type&&b[j].under_construction)pending=1;
                if(!pending&&race_skirmish_site_for(owner,supply_type,&x,&y)) {
                    int worker=race_owner_worker(owner,race==R_PROTOSS?U_PROBE:U_SCV,x,y);
                    if(worker>=0)race_start_build(owner,worker,supply_type,x,y);
                }
            }
        }
        /* Put two actual workers on a completed gas structure, or build one
         * on the owner's geyser after the first combat structure is started. */
        {
            int ref = race_owner_refinery(owner), gx, gy, gi, gas_workers = 0;
            int gas_type=race==R_ZERG?B_EXTRACTOR:race==R_PROTOSS?B_ASSIMILATOR:B_REFINERY;
            for (i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == owner && u[i].order == ORDER_GAS) ++gas_workers;
            if (ref >= 0 && gas_workers < 2) {
                int w=-1,best=0x7fffffff;
                for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==owner&&
                    sc64_race_is_worker(u[j].type)&&u[j].order!=ORDER_GAS&&u[j].order!=ORDER_BUILD) {
                    int d=dist2(u[j].x,u[j].y,b[ref].x,b[ref].y);
                    if(d<best){best=d;w=j;}
                }
                if (w >= 0) { u[w].order = ORDER_GAS; u[w].target = ref; u[w].carry = 0; u[w].work_timer = 0; }
            } else if (ref < 0 && !race_owner_has_type(owner,gas_type) && (race_owner_has_type(owner, race == R_ZERG ? B_SPAWNING_POOL : race == R_PROTOSS ? B_GATEWAY : B_BARRACKS)) &&
                       owner != SIDE) {
                build_type = gas_type;
                int base=owner==ENEMY?1:owner==RESCUABLE?2:3,best=0x7fffffff;
                gi=-1;
                for(int g=0;g<geyser_count;g++){
                    int distance=dist2(geysers[g].x,geysers[g].y,race_base_x[base],race_base_y[base]);
                    if(distance<best&&race_can_site(owner,build_type,geysers[g].x,geysers[g].y)&&can_place(build_type,geysers[g].x,geysers[g].y)){gi=g;best=distance;}
                }
                if(gi>=0){
                    gx = geysers[gi].x; gy = geysers[gi].y;
                    int w = race_owner_worker(owner, race == R_ZERG ? U_DRONE : race == R_PROTOSS ? U_PROBE : U_SCV, gx, gy);
                    if (w >= 0) race_start_build(owner, w, build_type, gx, gy);
                }
            }
        }
        build_type = race == R_ZERG ? B_SPAWNING_POOL : race == R_PROTOSS ? B_PYLON : B_BARRACKS;
        if (!race_owner_has_type(owner, build_type) && race_skirmish_site_for(owner, build_type, &x, &y)) {
            int worker = race_owner_worker(owner, race == R_ZERG ? U_DRONE : race == R_PROTOSS ? U_PROBE : U_SCV, x, y);
            if (worker >= 0) { race_start_build(owner, worker, build_type, x, y); continue; }
        }
        build_type = race == R_ZERG ? B_HYDRALISK_DEN : race == R_PROTOSS ? B_GATEWAY : B_BARRACKS;
        if (race_owner_has_type(owner, race == R_ZERG ? B_SPAWNING_POOL : race == R_PROTOSS ? B_PYLON : B_DEPOT) &&
            !race_owner_has_type(owner, build_type) && race_skirmish_site_for(owner, build_type, &x, &y)) {
            int worker = race_owner_worker(owner, race == R_ZERG ? U_DRONE : race == R_PROTOSS ? U_PROBE : U_SCV, x, y);
            if (worker >= 0) { race_start_build(owner, worker, build_type, x, y); continue; }
        }
        build_type = B_CYBERNETICS_CORE;
        if (race == R_PROTOSS && race_owner_has_type(owner, B_GATEWAY) &&
            !race_owner_has_type(owner, build_type) && race_skirmish_site_for(owner, build_type, &x, &y)) {
            int worker = race_owner_worker(owner, U_PROBE, x, y);
            if (worker >= 0) { race_start_build(owner, worker, build_type, x, y); continue; }
        }
        /* Reserve the first gas structure's price before queuing units.
         * Marines and workers otherwise spend each mining trip before a
         * refinery's full price can accumulate. Construction above spends
         * this reserve normally; no resources are granted to the AI. */
        int gas_type=race==R_ZERG?B_EXTRACTOR:race==R_PROTOSS?B_ASSIMILATOR:B_REFINERY;
        int reserve=race_owner_has_type(owner,gas_type)?0:sc64_race_building_rule(gas_type)->minerals;
        int worker_type=race==R_ZERG?U_DRONE:race==R_PROTOSS?U_PROBE:U_SCV;
        if (producer >= 0 && workers < 8 &&
            race_owner_minerals(owner)-reserve >= sc64_race_unit_rule(worker_type)->minerals)
            race_queue_unit(owner, producer, worker_type);
        unit_type = race == R_ZERG ?
            (race_owner_has_type(owner, B_HYDRALISK_DEN) ? U_HYDRALISK : U_ZERGLING) :
            race == R_PROTOSS ? (race_owner_has_type(owner, B_CYBERNETICS_CORE) ? U_DRAGOON : U_ZEALOT) : U_MARINE;
        for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) && b[i].owner == owner &&
            sc64_race_producer_compatible(b[i].type, sc64_race_unit_rule(unit_type)->producer) && b[i].queue_count < 2) {
            if(race_owner_minerals(owner)-reserve >= sc64_race_unit_rule(unit_type)->minerals)
                race_queue_unit(owner, i, unit_type);
            break;
        }
        if (army >= 4) {
            int tx = -1, ty = -1, target = -1, best = 0x7fffffff;
            for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && owners_hostile(owner, b[i].owner)) {
                int d = dist2(race_base_x[owner == SIDE ? 0 : owner == ENEMY ? 1 : owner == RESCUABLE ? 2 : 3],
                              race_base_y[owner == SIDE ? 0 : owner == ENEMY ? 1 : owner == RESCUABLE ? 2 : 3],
                              b[i].x, b[i].y);
                if (d < best) { best = d; target = i; }
            }
            if (target >= 0) { tx = b[target].x; ty = b[target].y; }
            for (i = 0; i < MAX_UNITS; ++i) if (tx >= 0 && u[i].alive && u[i].owner == owner &&
                !sc64_race_is_worker(u[i].type) && sc64_race_unit_damage(u[i].type) > 0) {
                u[i].order = ORDER_ATTACK_MOVE; u[i].tx = tx; u[i].ty = ty;
            }
        }
    }
}

static void race_skirmish_outcome(void)
{
    int i, human_buildings = 0, enemy_buildings = 0;
    if (!race_mode || won || lost) return;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].owner == SIDE) ++human_buildings;
        else if (race_bot_index(b[i].owner) >= 0) ++enemy_buildings;
    }
    if (!human_buildings) { lost = 1; screen = SCREEN_RESULT; result_timer = 0; }
    else if (!enemy_buildings) { won = 1; screen = SCREEN_RESULT; result_timer = 0; }
}

#endif
