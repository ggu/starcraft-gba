/* T5 Control Tower prerequisite/building tests. Fixture state is constructed
 * directly to isolate paid tech behavior; this is not a controller proof. */
#include "../starcraft/game.c"
#include "../starcraft/sc64_t5_tech_adapter.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int find_starport_site(int *out_x, int *out_y)
{
    for (int y = 64; y < WORLD_H - 64; y += 16)
        for (int x = 64; x < WORLD_W - 96; x += 16)
            if (can_place(B_STARPORT, x, y) &&
                can_place(B_CONTROL_TOWER, x + SC64_T5_ADDON_X_OFFSET,
                          y + SC64_T5_ADDON_Y_OFFSET)) {
                *out_x = x;
                *out_y = y;
                return 1;
            }
    return 0;
}

static int make_building(int owner, int type, int x, int y, int complete)
{
    int slot = spawn_build(owner, type, x, y, !complete);
    assert(slot >= 0);
    if (complete) {
        b[slot].under_construction = 0;
        b[slot].hp = b[slot].maxhp;
    }
    return slot;
}

static void reset_fixture(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active());
    memset(b, 0, sizeof(b));
    memset(u, 0, sizeof(u));
    for (int i = 0; i < MAX_BUILDINGS; ++i) b[i].level = 0;
    minerals = 1000;
    gas = 1000;
    supply_cap = 200;
    supply_used = 0;
    invalidate_path_grid();
}

int main(void)
{
    int x, y, factory, command, starport, other_starport, tower;
    int before_ore, before_gas;

    assert(sc64_race_building_from_raw(115) == B_CONTROL_TOWER);
    assert(sc64_race_raw_building(B_CONTROL_TOWER) == 115);
    assert(!strcmp(sc64_race_building_name(B_CONTROL_TOWER), "CONTROL TOWER"));
    assert(sc64_race_t5_building_art_raw(B_CONTROL_TOWER) == 114);
    assert(sc64_race_building_max_hp(B_CONTROL_TOWER) == 500);
    assert(sc64_race_building_armor(B_CONTROL_TOWER) == 1);
    assert(sc64_race_building_rule(B_CONTROL_TOWER)->minerals == 50);
    assert(sc64_race_building_rule(B_CONTROL_TOWER)->gas == 50);
    assert(sc64_race_building_rule(B_CONTROL_TOWER)->build_frames == 600);
    assert(sc64_race_building_rule(B_CONTROL_TOWER)->worker == 255);
    assert(sc64_race_unit_from_raw(11) == U_DROPSHIP);
    assert(sc64_race_raw_unit(U_DROPSHIP) == 11);
    assert(sc64_race_unit_rule(U_DROPSHIP)->prereq & SC_PR_CONTROL_TOWER);

    reset_fixture();
    assert(find_starport_site(&x, &y));
    /* A Starport requires a completed Factory in this profile. */
    assert(!race_can_site(SIDE, B_STARPORT, x, y));
    assert(!race_can_site(SIDE, B_FACTORY, x - 96, y + 96));
    make_building(SIDE, B_BARRACKS, x - 192, y + 160, 1);
    assert(race_can_site(SIDE, B_FACTORY, x - 96, y + 96));
    factory = make_building(SIDE, B_FACTORY, x - 96, y + 96, 0);
    assert(!race_can_site(SIDE, B_STARPORT, x, y));
    b[factory].under_construction = 0;
    assert(race_can_site(SIDE, B_STARPORT, x, y));
    command = make_building(SIDE, B_COMMAND, x - 96, y - 96, 1);
    (void)command;
    starport = make_building(SIDE, B_STARPORT, x, y, 1);
    assert(b[starport].raw_type == 114 && b[starport].raw_owner == 0);
    other_starport = make_building(SIDE, B_STARPORT, x + 144, y, 1);

    /* Dropships require an attached, completed tower; an unrelated tower or
       a tower on another Starport cannot satisfy the prerequisite. */
    before_ore = minerals;
    before_gas = gas;
    assert(!race_queue_unit(SIDE, starport, U_DROPSHIP));
    assert(minerals == before_ore && gas == before_gas && !b[starport].queue_count);
    assert(race_queue_unit(SIDE, starport, U_WRAITH));
    b[starport].queue_count = 0;
    b[starport].queue_head = 0;
    assert(!race_t5_starport_has_control_tower(starport));

    /* Wrong producer, wrong owner, unfinished producer, occupied add-on site,
       insufficient bank, and full building pool all fail without charging. */
    before_ore = minerals; before_gas = gas;
    assert(!race_t5_start_control_tower(factory));
    assert(!race_t5_start_control_tower(other_starport + MAX_BUILDINGS));
    b[other_starport].owner = ENEMY;
    assert(!race_t5_start_control_tower(other_starport));
    b[other_starport].owner = SIDE;
    b[other_starport].under_construction = 1;
    assert(!race_t5_start_control_tower(other_starport));
    b[other_starport].under_construction = 0;
    tower = make_building(ENEMY, B_DEPOT, x + SC64_T5_ADDON_X_OFFSET,
                          y + SC64_T5_ADDON_Y_OFFSET, 1);
    assert(!race_t5_start_control_tower(starport));
    b[tower].alive = 0;
    assert(minerals == before_ore && gas == before_gas);
    minerals = 49;
    assert(!race_t5_start_control_tower(starport));
    assert(minerals == 49 && gas == before_gas);
    minerals = before_ore; gas = 49;
    assert(!race_t5_start_control_tower(starport));
    assert(minerals == before_ore && gas == 49);
    gas = before_gas;
    for (int i = 0; i < MAX_BUILDINGS; ++i) b[i].alive = 1;
    assert(!race_t5_start_control_tower(starport));
    assert(minerals == before_ore && gas == before_gas);

    reset_fixture();
    assert(find_starport_site(&x, &y));
    factory = make_building(SIDE, B_FACTORY, x - 96, y + 96, 1);
    command = make_building(SIDE, B_COMMAND, x - 96, y - 96, 1);
    starport = make_building(SIDE, B_STARPORT, x, y, 1);
    (void)factory; (void)command;
    before_ore = minerals; before_gas = gas;
    assert(!race_start_build(SIDE, -1, B_CONTROL_TOWER,
                             x + SC64_T5_ADDON_X_OFFSET,
                             y + SC64_T5_ADDON_Y_OFFSET));
    assert(race_t5_start_control_tower(starport));
    assert(minerals == before_ore - 50 && gas == before_gas - 50);
    tower = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_CONTROL_TOWER) tower = i;
    assert(tower >= 0 && b[tower].under_construction && b[tower].hp == 1);
    assert(b[tower].maxhp == 500 && b[tower].level == starport + 1);
    assert(b[tower].raw_type == 115 && b[tower].raw_owner == 0);
    assert(b[tower].x == b[starport].x + 48 && b[tower].y == b[starport].y + 8);
    assert(!race_t5_starport_has_control_tower(starport));
    assert(!race_t5_starport_has_control_tower(other_starport));
    for (int i = 0; i < 749; ++i) race_t5_tech_tick();
    assert(b[tower].under_construction && b[tower].build_progress == 749);
    race_t5_tech_tick();
    assert(!b[tower].under_construction && b[tower].build_progress == 0 && b[tower].hp == 500);
    assert(race_t5_starport_has_control_tower(starport));
    assert(!race_t5_start_control_tower(starport));

    /* A completed attached tower permits Dropship production and charges the
       normal unit cost; the other Starport still lacks the local attachment. */
    before_ore = minerals; before_gas = gas;
    assert(!race_queue_unit(SIDE, other_starport, U_DROPSHIP));
    assert(race_queue_unit(SIDE, starport, U_DROPSHIP));
    assert(minerals == before_ore - 100 && gas == before_gas - 100);
    assert(b[starport].queue_count == 1 && b[starport].queue[0] == U_DROPSHIP);

    /* Paid products receive canonical source identities before import/save
       or cargo code observes them. The importer may override these defaults. */
    int ship = spawn_unit(SIDE, U_DROPSHIP, x, y);
    int marine = spawn_unit(SIDE, U_MARINE, x, y);
    assert(ship >= 0 && marine >= 0);
    assert(u[ship].raw_type == 11 && u[ship].raw_owner == 0);
    assert(u[marine].raw_type == 0 && u[marine].raw_owner == 0);
    assert(race_transport_carrier_valid(ship));
    assert(race_transport_request_board(marine, ship));

    /* A reused pool slot at another location cannot borrow the old tower,
       even before the next tech phase has detached that stale reference. */
    b[starport].x += 160;
    assert(!race_t5_starport_has_control_tower(starport));
    before_ore = minerals; before_gas = gas;
    assert(!race_queue_unit(SIDE, starport, U_DROPSHIP));
    assert(minerals == before_ore && gas == before_gas);
    race_t5_tech_tick();
    assert(b[tower].level == 0);
    b[starport].x -= 160;
    b[tower].level = starport + 1; /* restore explicit fixture linkage */

    /* Destroying the parent must not leave a stale slot link that becomes a
       valid attachment if a new Starport later reuses that pool index. */
    b[starport].alive = 0;
    race_t5_tech_tick();
    assert(b[tower].level == 0);
    b[starport].alive = 1;
    b[starport].owner = SIDE;
    b[starport].type = B_STARPORT;
    b[starport].under_construction = 0;
    assert(!race_t5_starport_has_control_tower(starport));

    puts("T5 tech: Control Tower attachment, prerequisites, payment, progress, and Dropship gating passed");
    return 0;
}
