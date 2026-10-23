#ifndef SC64_T6_TECH_ADAPTER_H
#define SC64_T6_TECH_ADAPTER_H

#if defined(SC64_T6_RUNTIME) && defined(SC64_T5_RUNTIME)

/* T6 has the same source-backed Starport/Control Tower footprint as T5. The
 * adapter keeps its availability and active-mission checks separate so T6
 * support cannot change earlier campaign behavior. */
static int race_t6_active(void);
static int race_t6_building_product_available(int owner, int raw_owner,
                                               int type);
static int race_t6_unit_product_available(int owner, int raw_owner, int type);
enum { SC64_T6_ADDON_X_OFFSET = 48, SC64_T6_ADDON_Y_OFFSET = 8 };

static inline int race_t6_tech_active(void)
{
    return race_t6_active()
#ifdef SC64_TERRAN_LATE_RUNTIME
        || race_late_active()
#endif
        ;
}

static int
race_t6_starport_has_control_tower(int producer)
{
    if (producer < 0 || producer >= MAX_BUILDINGS || !b[producer].alive ||
        b[producer].type != B_STARPORT || !building_complete(producer)) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_CONTROL_TOWER &&
            b[i].owner == b[producer].owner && b[i].level == producer + 1 &&
            b[i].x == b[producer].x + SC64_T6_ADDON_X_OFFSET &&
            b[i].y == b[producer].y + SC64_T6_ADDON_Y_OFFSET &&
            building_complete(i)) return 1;
    return 0;
}

static int
race_t6_has_attached_control_tower(int producer)
{
    if (producer < 0 || producer >= MAX_BUILDINGS) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_CONTROL_TOWER &&
            b[i].level == producer + 1) return 1;
    return 0;
}

static int
race_t6_can_train_dropship(int producer)
{
    if (!race_t6_tech_active() || producer < 0 || producer >= MAX_BUILDINGS ||
        !b[producer].alive || b[producer].owner != SIDE ||
        b[producer].type != B_STARPORT || !building_complete(producer) ||
        !race_t6_starport_has_control_tower(producer)) return 0;
    return race_t6_unit_product_available(SIDE, b[producer].raw_owner,
                                           U_DROPSHIP);
}

static int
race_t6_start_control_tower(int producer)
{
    int x, y, slot;
    Sc64RaceBank bank;
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(B_CONTROL_TOWER);
    if (!race_t6_tech_active() || !rule || producer < 0 ||
        producer >= MAX_BUILDINGS || !b[producer].alive ||
        b[producer].owner != SIDE || b[producer].type != B_STARPORT ||
        !building_complete(producer) ||
        !race_t6_building_product_available(SIDE, b[producer].raw_owner,
                                            B_CONTROL_TOWER) ||
        !race_owner_has_prereq(SIDE, SC_PR_FACTORY) ||
        race_t6_has_attached_control_tower(producer)) return 0;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (!race_late_building_product_available(SIDE, b[producer].raw_owner,
                                               B_CONTROL_TOWER)) return 0;
#endif
    x = b[producer].x + SC64_T6_ADDON_X_OFFSET;
    y = b[producer].y + SC64_T6_ADDON_Y_OFFSET;
    if (x <= 12 || y <= 18 || x >= WORLD_W - 12 || y >= WORLD_H - 12 ||
        !race_can_site(SIDE, B_CONTROL_TOWER, x, y) ||
        !can_place(B_CONTROL_TOWER, x, y)) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (i != producer && b[i].alive && dist2(x, y, b[i].x, b[i].y) < 24 * 24)
            return 0;
    slot = find_build_slot();
    if (slot < 0) return 0;
    bank.minerals = (unsigned short)race_owner_minerals(SIDE);
    bank.gas = (unsigned short)race_owner_gas(SIDE);
    if (!sc64_race_pay_building(&bank, B_CONTROL_TOWER)) return 0;
    slot = spawn_build(SIDE, B_CONTROL_TOWER, x, y, 1);
    if (slot < 0) return 0;
    b[slot].level = producer + 1;
    b[slot].raw_type = 115;
    b[slot].raw_owner =
#ifdef SC64_TERRAN_LATE_RUNTIME
        race_late_active() ? sc64_terran_late_product_source_owner(SIDE) :
#endif
        0;
    minerals = bank.minerals;
    gas = bank.gas;
    return 1;
}

static void
race_t6_tech_tick(void)
{
    if (!race_t6_tech_active()) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *tower = &b[i];
        int parent;
        if (!tower->alive || tower->type != B_CONTROL_TOWER) continue;
        parent = tower->level - 1;
        if (parent < 0 || parent >= MAX_BUILDINGS || !b[parent].alive ||
            b[parent].type != B_STARPORT || b[parent].owner != tower->owner ||
            tower->x != b[parent].x + SC64_T6_ADDON_X_OFFSET ||
            tower->y != b[parent].y + SC64_T6_ADDON_Y_OFFSET) {
            tower->level = 0;
            continue;
        }
        if (!tower->under_construction) continue;
        if (++tower->build_progress >=
            (sc64_race_building_rule(B_CONTROL_TOWER)->build_frames * 5 + 3) / 4) {
            tower->under_construction = 0;
            tower->hp = tower->maxhp;
            tower->build_progress = 0;
        }
    }
}

#endif /* SC64_T6_RUNTIME && SC64_T5_RUNTIME */
#endif /* SC64_T6_TECH_ADAPTER_H */
