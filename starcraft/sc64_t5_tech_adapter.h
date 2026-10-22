#ifndef SC64_T5_TECH_ADAPTER_H
#define SC64_T5_TECH_ADAPTER_H

/* T5 Terran tech path only. The Control Tower is represented as a normal
 * building-pool record attached to its Starport; Building.level stores the
 * parent slot plus one so zero remains the unattached sentinel. */
#ifdef SC64_T5_RUNTIME

/* BWAPI's Starport tile footprint is 4x3 and Control Tower is 2x2.
   The verified addon anchor is TilePosition(4,1) from the Starport top-left.
   Convert anchor to center delta: (4+1-2, 1+1-1.5) tiles = (96,16)
   source pixels = (48,8) GBA pixels at the 2:1 world scale. */
enum { SC64_T5_ADDON_X_OFFSET = 48, SC64_T5_ADDON_Y_OFFSET = 8 };

static inline int sc64_race_t5_building_art_raw(int building_type)
{
    /* The private graphics set has Starport art but no Control Tower frame.
       Keep raw_type=115 for identity/save semantics; draw hook may alias 114. */
    return building_type == B_CONTROL_TOWER ? 114 :
           sc64_race_raw_building(building_type);
}

static int race_t5_starport_has_control_tower(int producer)
{
    if (producer < 0 || producer >= MAX_BUILDINGS || !b[producer].alive ||
        b[producer].type != B_STARPORT || !building_complete(producer)) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_CONTROL_TOWER &&
            b[i].owner == b[producer].owner && b[i].level == producer + 1 &&
            b[i].x == b[producer].x + SC64_T5_ADDON_X_OFFSET &&
            b[i].y == b[producer].y + SC64_T5_ADDON_Y_OFFSET &&
            building_complete(i)) return 1;
    return 0;
}

static int race_t5_has_attached_control_tower(int producer)
{
    if (producer < 0 || producer >= MAX_BUILDINGS) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_CONTROL_TOWER &&
            b[i].level == producer + 1) return 1;
    return 0;
}

static int race_t5_start_control_tower(int producer)
{
    int x, y, slot;
    Sc64RaceBank bank;
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(B_CONTROL_TOWER);
    if (!race_t5_active() || !rule || producer < 0 || producer >= MAX_BUILDINGS ||
        !b[producer].alive || b[producer].owner != SIDE ||
        b[producer].type != B_STARPORT || !building_complete(producer) ||
        !race_t5_building_product_available(SIDE, b[producer].raw_owner,
                                            B_CONTROL_TOWER) ||
        !race_owner_has_prereq(SIDE, SC_PR_FACTORY) ||
        race_t5_has_attached_control_tower(producer)) return 0;
    x = b[producer].x + SC64_T5_ADDON_X_OFFSET;
    y = b[producer].y + SC64_T5_ADDON_Y_OFFSET;
    if (x <= 12 || y <= 18 || x >= WORLD_W - 12 || y >= WORLD_H - 12 ||
        !race_can_site(SIDE, B_CONTROL_TOWER, x, y) || !can_place(B_CONTROL_TOWER, x, y))
        return 0;
    /* can_place checks the parent using ordinary building clearance. Also
       reject any competing building in that clearance band explicitly. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (i != producer && b[i].alive &&
            dist2(x, y, b[i].x, b[i].y) < 24 * 24) return 0;
    slot = find_build_slot();
    if (slot < 0) return 0;
    bank.minerals = (unsigned short)race_owner_minerals(SIDE);
    bank.gas = (unsigned short)race_owner_gas(SIDE);
    if (!sc64_race_pay_building(&bank, B_CONTROL_TOWER)) return 0;
    slot = spawn_build(SIDE, B_CONTROL_TOWER, x, y, 1);
    if (slot < 0) return 0; /* The pool was checked above; no state can race here. */
    b[slot].level = producer + 1;
    b[slot].raw_type = 115;
    b[slot].raw_owner = 0;
    minerals = bank.minerals;
    gas = bank.gas;
    return 1;
}

/* Add-ons have no SCV order. Advance them in the T5-only tech phase at the
   same 5/4 game-tick conversion used by the other authored race rules. A
   destroyed/captured parent leaves a harmless orphan with level=0; this avoids
   accidentally attaching it to a later Starport that reuses the pool slot. */
static void race_t5_tech_tick(void)
{
    if (!race_t5_active()) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *tower = &b[i];
        int parent;
        if (!tower->alive || tower->type != B_CONTROL_TOWER) continue;
        parent = tower->level - 1;
        if (parent < 0 || parent >= MAX_BUILDINGS || !b[parent].alive ||
            b[parent].type != B_STARPORT || b[parent].owner != tower->owner ||
            tower->x != b[parent].x + SC64_T5_ADDON_X_OFFSET ||
            tower->y != b[parent].y + SC64_T5_ADDON_Y_OFFSET) {
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

#endif /* SC64_T5_RUNTIME */
#endif /* SC64_T5_TECH_ADAPTER_H */
