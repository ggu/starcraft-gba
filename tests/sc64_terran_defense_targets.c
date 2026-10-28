/* Differential coverage for the late-Terran defense owner-list query. */
#include <assert.h>
#include "../starcraft/game.c"

static int reference_target(const Building *q, int ground, int air, int range)
{
    int best = -1, bestd = range * range;
    for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive) {
        if (FAST_ABS(q->x - u[j].x) >= range || FAST_ABS(q->y - u[j].y) >= range) continue;
        if (!owners_hostile(q->owner, u[j].owner) || (u[j].flags & 16)) continue;
        if (q->owner == SIDE && !currently_visible(u[j].x, u[j].y)) continue;
        int flying = sc64_race_unit_flying(u[j].type);
        if ((flying && !air) || (!flying && !ground) || !race_can_attack(-1, j)) continue;
        int d = dist2(q->x, q->y, u[j].x, u[j].y);
        if (d < bestd) { bestd = d; best = j; }
    }
    return best;
}

static void build_target_lists(u8 head[5][2], u8 next[MAX_UNITS], u8 hostile[5])
{
    memset(head, 0xff, sizeof(u8) * 5 * 2);
    for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive) {
        int owner = u[j].owner;
        if (owner < 0 || owner >= 5) continue;
        int flying = sc64_race_unit_flying(u[j].type);
        next[j] = head[owner][flying];
        head[owner][flying] = (u8)j;
    }
    for (int owner = 0; owner < 5; ++owner) {
        hostile[owner] = 0;
        for (int other = 0; other < 5; ++other)
            if (owners_hostile(owner, other)) hostile[owner] |= (u8)(1u << other);
    }
}

int main(void)
{
    u8 head[5][2], next[MAX_UNITS], hostile[5];
    Building tower;
    gba_init(); game_init();
    race_mode = 1; race_human = R_TER; race_bots = 2;
    race_skirmish_init(); screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    for (int cell = 0; cell < MAP_CELLS; ++cell) SC64_GRID_SET(visible, cell, 1);

    /* Dense mixed-owner candidates, including slot-order ties, strict range
     * boundaries, hidden/invincible flags, and both attack classes. */
    for (int j = 0; j < MAX_UNITS; ++j) {
        u[j].alive = 1;
        u[j].owner = (j % 4 == 0) ? SIDE : (j % 4 == 1) ? ENEMY :
                     (j % 4 == 2) ? 4 : RESCUABLE;
        u[j].type = (j % 3 == 0) ? U_WRAITH : U_MARINE;
        u[j].x = 320 + (j % 16) * 11;
        u[j].y = 320 + (j / 16) * 9;
        u[j].flags = (j % 23 == 0) ? 16 : 0;
    }
    u[1].x = u[2].x = 400; u[1].y = u[2].y = 400;
    u[1].flags = u[2].flags = 0;
    u[MAX_UNITS - 1].x = 512; u[MAX_UNITS - 1].y = 400;
    u[MAX_UNITS - 1].flags = 0;
    build_target_lists(head, next, hostile);

    memset(&tower, 0, sizeof(tower));
    tower.owner = SIDE; tower.x = 400; tower.y = 400;
    for (int owner = 0; owner < 5; ++owner) {
        tower.owner = owner;
        for (int mode = 0; mode < 3; ++mode) {
            int ground = mode != 2, air = mode != 1;
            for (int range = 1; range <= 128; range += 17) {
                int expected = reference_target(&tower, ground, air, range);
                int actual = race_defense_target_from_owner_lists(
                    &tower, ground, air, range, head, next, hostile[owner]);
                assert(actual == expected);
            }
        }
    }

    /* Class lists keep air-only and ground-only defenses exact while ties
     * across classes still choose the lower global unit slot. */
    memset(u, 0, sizeof(u));
    u[0].alive = u[1].alive = 1;
    u[0].owner = u[1].owner = ENEMY;
    u[0].type = U_WRAITH; u[1].type = U_MARINE;
    u[0].x = u[1].x = 400; u[0].y = u[1].y = 400;
    build_target_lists(head, next, hostile);
    assert(race_defense_target_from_owner_lists(&tower, 0, 1, 112, head, next,
                                                hostile[SIDE]) == 0);
    assert(race_defense_target_from_owner_lists(&tower, 1, 0, 112, head, next,
                                                hostile[SIDE]) == 1);
    assert(race_defense_target_from_owner_lists(&tower, 1, 1, 112, head, next,
                                                hostile[SIDE]) == 0);
    u[0].alive = 0;
    assert(race_defense_target_from_owner_lists(&tower, 0, 1, 112, head, next,
                                                hostile[SIDE]) == -1);

    /* A preceding defense can kill the indexed winner. The next lookup must
     * observe live state immediately and select the next target. */
    tower.owner = SIDE; tower.x = 400; tower.y = 400;
    int first = race_defense_target_from_owner_lists(&tower, 1, 1, 112, head, next,
                                                      hostile[SIDE]);
    assert(first >= 0);
    u[first].alive = 0;
    assert(race_defense_target_from_owner_lists(&tower, 1, 1, 112, head, next,
                                                hostile[SIDE]) ==
           reference_target(&tower, 1, 1, 112));

    /* Human defense acquisition obeys current fog in the owner-list backend;
     * other owners continue using their own target/visibility model. */
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    u[0] = (Unit){ .x=416, .y=400, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_MARINE, .alive=1 };
    u[1] = (Unit){ .x=448, .y=400, .hp=100, .maxhp=100,
        .owner=ENEMY, .type=U_MARINE, .alive=1 };
    SC64_GRID_CLEAR(visible);
    SC64_GRID_SET(visible, 448 / 16 + (400 / 16) * MAP_COLS, 1);
    build_target_lists(head, next, hostile);
    tower.owner = SIDE; tower.x = 400; tower.y = 400;
    assert(race_defense_target_from_owner_lists(&tower, 1, 0, 112, head, next,
                                                hostile[SIDE]) == 1);
    SC64_GRID_SET(visible, 448 / 16 + (400 / 16) * MAP_COLS, 0);
    assert(race_defense_target_from_owner_lists(&tower, 1, 0, 112, head, next,
                                                hostile[SIDE]) == -1);
    tower.owner = ENEMY;
    u[2] = (Unit){ .x=416, .y=400, .hp=100, .maxhp=100,
        .owner=SIDE, .type=U_MARINE, .alive=1 };
    build_target_lists(head, next, hostile);
    assert(race_defense_target_from_owner_lists(&tower, 1, 0, 112, head, next,
                                                hostile[ENEMY]) == 2);
    tower.owner = SIDE;
    for (int cell = 0; cell < MAP_CELLS; ++cell) SC64_GRID_SET(visible, cell, 1);

    /* Owner changes remain guarded by current ownership and hostility. */
    int moved_owner = head[ENEMY][0];
    if (moved_owner == 0xff) moved_owner = head[ENEMY][1];
    assert(moved_owner != 0xff);
    u[moved_owner].owner = SIDE;
    assert(race_defense_target_from_owner_lists(&tower, 1, 1, 112, head, next,
                                                hostile[SIDE]) ==
           reference_target(&tower, 1, 1, 112));

    /* Full-tick differential cases cover every late campaign mission and
     * skirmish diplomacy. Each run restores the same randomized world before
     * the scan oracle and indexed tick, then compares all combat mutations. */
    {
        static Unit units_before[MAX_UNITS], units_after[MAX_UNITS];
        static Building buildings_before[MAX_BUILDINGS], buildings_after[MAX_BUILDINGS];
        static Tracer tracers_before[MAX_TRACERS], tracers_after[MAX_TRACERS];
        static Sc64TerranLateMissionState late_before, late_after;
        int supply_after;
        static u32 rng = 0x91e10da5u;
        const int missions[] = {10, 11, 12, 13, -1};
        for (unsigned test = 0; test < sizeof(missions) / sizeof(missions[0]); ++test) {
            race_mode = missions[test] < 0;
            race_selected_mission = missions[test] < 0 ? 0 : missions[test];
            memset(&race_late, 0, sizeof(race_late));
            sc64_terran_late_build_hostility_cache();
            memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
            memset(invincible_units, 0, sizeof(invincible_units));
            memset(invincible_buildings, 0, sizeof(invincible_buildings));
            memset(tracers, 0, sizeof(tracers)); supply_used = 0;
            for (int j = 0; j < MAX_UNITS; ++j) {
                rng = rng * 1664525u + 1013904223u;
                if ((rng >> 28) == 0) continue;
                Unit *p = &u[j]; p->alive = 1;
                p->owner = (u8[]){ SIDE, ENEMY, 4, RESCUABLE }[(rng >> 8) & 3u];
                p->type = (j % 37 == 0) ? U_MAP_REVEALER :
                          (j % 41 == 0) ? U_DATA_DISK :
                          (j % 3 == 0) ? U_WRAITH : U_MARINE;
                p->x = 180 + (int)((rng >> 16) % 900u);
                rng = rng * 1664525u + 1013904223u;
                p->y = 180 + (int)((rng >> 16) % 900u);
                p->hp = (j % 13 == 0) ? 1 : 24 + (int)(rng % 60u);
                p->maxhp = 100; p->shield = (j % 5 == 0) ? 8 : 0;
                p->armor = (u8)(j % 4); p->order = (j % 31 == 0) ? ORDER_TRANSPORTED : 0;
                p->flags = (j % 17 == 0) ? 16 : 0;
                invincible_units[j] = (u8)(j % 19 == 0);
            }
            for (int i = 0; i < 32; ++i) {
                rng = rng * 1664525u + 1013904223u;
                Building *p = &b[i]; p->alive = 1; p->under_construction = 0;
                p->owner = (u8[]){ SIDE, ENEMY, 4, RESCUABLE }[(rng >> 8) & 3u];
                p->type = (i % 4 == 0) ? B_PHOTON_CANNON :
                          (i % 4 == 1) ? B_SUNKEN_COLONY :
                          (i % 4 == 2) ? B_SPORE_COLONY : B_MISSILE_TURRET;
                p->x = 180 + (int)((rng >> 16) % 900u);
                p->y = 180 + (int)((rng >> 1) % 900u);
                p->hp = p->maxhp = 100; p->shield = (i % 2) ? 0 : 6;
                p->queue_progress = i % 5 == 0 ? 0 : i % 3;
                b[32 + i].alive = p->type == B_PHOTON_CANNON;
                b[32 + i].under_construction = 0;
                b[32 + i].owner = p->owner; b[32 + i].type = B_PYLON;
                b[32 + i].x = p->x; b[32 + i].y = p->y;
            }
            /* Force two nearby defenses to share a low-health first target;
             * the second query must see its death and select the next live unit. */
            u[0].alive = u[1].alive = 1;
            u[0].owner = u[1].owner = ENEMY; u[0].type = u[1].type = U_MARINE;
            u[0].x = u[1].x = 400; u[0].y = u[1].y = 400;
            u[0].hp = 1; u[1].hp = 90; u[0].shield = u[1].shield = 0;
            u[0].flags = u[1].flags = 0; invincible_units[0] = invincible_units[1] = 0;
            b[0].alive = b[1].alive = 1;
            b[0].owner = b[1].owner = SIDE;
            b[0].type = b[1].type = B_SUNKEN_COLONY;
            b[0].x = b[1].x = 400; b[0].y = b[1].y = 400;
            b[0].queue_progress = b[1].queue_progress = 0;
            if (test == 0) {
                for (int j = 2; j < MAX_UNITS; ++j) u[j].alive = 0;
                for (int i = 2; i < MAX_BUILDINGS; ++i) b[i].alive = 0;
                b[0].type = b[1].type = B_SPORE_COLONY;
                u[0].hp = u[1].hp = 90;
            }

            memcpy(units_before, u, sizeof(units_before));
            memcpy(buildings_before, b, sizeof(buildings_before));
            memcpy(tracers_before, tracers, sizeof(tracers_before));
            late_before = race_late;
            int supply_before = supply_used;
            race_tick_defenses_fullscan();
            if (test == 0) assert(b[0].queue_progress == 0 && b[1].queue_progress == 0);
            memcpy(units_after, u, sizeof(units_after));
            memcpy(buildings_after, b, sizeof(buildings_after));
            memcpy(tracers_after, tracers, sizeof(tracers_after));
            late_after = race_late; supply_after = supply_used;
            memcpy(u, units_before, sizeof(units_before));
            memcpy(b, buildings_before, sizeof(buildings_before));
            memcpy(tracers, tracers_before, sizeof(tracers_before));
            race_late = late_before; supply_used = supply_before;
            race_tick_defenses_late_targets();
            assert(memcmp(units_after, u, sizeof(units_after)) == 0);
            assert(memcmp(buildings_after, b, sizeof(buildings_after)) == 0);
            assert(memcmp(tracers_after, tracers, sizeof(tracers_after)) == 0);
            assert(memcmp(&late_after, &race_late, sizeof(race_late)) == 0);
            assert(supply_after == supply_used);
        }
    }
    return 0;
}
