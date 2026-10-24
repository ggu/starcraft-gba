/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual Probe-command path for a powered Photon Cannon. Economy and site
 * setup isolate the UI/building regression; this is not a campaign win test. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned held_keys;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key)
{
    input(key);
    input(0);
}

static int find_probe(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE) return i;
    return -1;
}

static int find_site(int powered, int require_rules, int near_x, int near_y,
                     int *out_x, int *out_y)
{
    int best = 0x7fffffff, found = 0;
    for (int y = 48; y < WORLD_H - 48; y += 8)
        for (int x = 48; x < WORLD_W - 48; x += 8) {
            if (!!race_site_powered(SIDE, x, y) != !!powered ||
                (require_rules && !race_can_site(SIDE, B_PHOTON_CANNON, x, y)) ||
                !can_place(B_PHOTON_CANNON, x, y)) continue;
            int d = dist2(x, y, near_x, near_y);
            if (d < best) {
                best = d; *out_x = x; *out_y = y; found = 1;
            }
        }
    return found;
}

static int side_building_count(int type)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        count += b[i].alive && b[i].owner == SIDE && b[i].type == type;
    return count;
}

static void select_probe_and_open_menu(int probe)
{
    clear_selection();
    cx = u[probe].x; cy = u[probe].y;
    press(KEY_A);
    assert(u[probe].selected);
    assert(race_probe_cannon_command_available());
    press(KEY_B);
    assert(menu_open);
}

static void select_cannon_row(void)
{
    assert(field_menu_count() == MENU_STOP + 3);
    for (int i = 0; i < MENU_STOP + 2; ++i) press(KEY_DOWN);
    assert(menu_cursor == MENU_STOP + 2);
    static const char *labels[MENU_STOP + 1] = {
        "MOVE ORDER", "ATTACK MOVE", "GATEWAY", "PYLON", "ASSIMILATOR",
        "CYBERNETICS", "FORGE", "WEAPONS", "ARMOR", "STOP / HOLD"
    };
    assert(strcmp(field_menu_base_label(menu_cursor, labels), "CANNON") == 0);
    game_render(); /* Scrolls the eleventh/twelfth rows through the menu panel. */
    press(KEY_A);
    assert(!menu_open && mode == MODE_BUILD && menu == B_PHOTON_CANNON);
}

static void test_context_and_menu(void)
{
    int probe, zealot;
    held_keys = 0;
    gba_init();
    game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1;
    race_skirmish_init(); screen = SCREEN_PLAY;
    probe = find_probe(); assert(probe >= 0);
    select_probe_and_open_menu(probe);
    select_cannon_row();
    mode = MODE_NORMAL;

    /* A mixed selection must not gain a Probe-only build command. */
    zealot = spawn_unit(SIDE, U_ZEALOT, u[probe].x + 8, u[probe].y);
    assert(zealot >= 0);
    clear_selection(); u[probe].selected = 1; u[zealot].selected = 1;
    assert(!race_probe_cannon_command_available());
    assert(field_menu_count() == MENU_STOP + 1);

    /* A selected building, spell-actor, or transport context is also kept
     * separate from the Probe command rows. */
    clear_selection(); u[probe].selected = 1; selected_building = 0;
    assert(!race_probe_cannon_command_available());
    selected_building = -1;
    clear_selection(); u[zealot].selected = 1;
    assert(!race_probe_cannon_command_available());
    clear_selection(); u[probe].selected = 1;
}

static void test_prerequisite_power_payment_and_completion(void)
{
    int probe, pylon = -1, forge_x = -1, forge_y = -1;
    int powered_x = -1, powered_y = -1, unpowered_x = -1, unpowered_y = -1;
    held_keys = 0;
    gba_init(); game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1;
    race_skirmish_init(); screen = SCREEN_PLAY;
    probe = find_probe(); assert(probe >= 0);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_PYLON) {
            pylon = i; break;
        }
    assert(pylon >= 0 && building_complete(pylon));
    assert(find_site(1, 0, u[probe].x, u[probe].y, &powered_x, &powered_y));

    /* Forge is absent from the ordinary Protoss skirmish start. The command
     * remains selectable, but placement fails without charging the player. */
    select_probe_and_open_menu(probe); select_cannon_row();
    int before = side_building_count(B_PHOTON_CANNON);
    minerals = 500; cx = powered_x; cy = powered_y;
    press(KEY_A);
    assert(mode == MODE_BUILD && minerals == 500);
    assert(side_building_count(B_PHOTON_CANNON) == before);

    /* Fixture-only setup supplies a completed Forge; normal UI still pays
     * and places the Cannon. */
    int forge_found = 0;
    for (int y = 48; y < WORLD_H - 48 && !forge_found; y += 8)
        for (int x = 48; x < WORLD_W - 48; x += 8)
            if (race_can_site(SIDE, B_FORGE, x, y) && can_place(B_FORGE, x, y)) {
                forge_x = x; forge_y = y; forge_found = 1; break;
            }
    assert(forge_found);
    int forge = spawn_build(SIDE, B_FORGE, forge_x, forge_y, 0);
    assert(forge >= 0 && building_complete(forge));
    assert(find_site(1, 1, u[probe].x, u[probe].y, &powered_x, &powered_y));
    assert(find_site(0, 0, u[probe].x, u[probe].y, &unpowered_x, &unpowered_y));

    /* Power is checked at the selected site, independently of Forge. */
    minerals = 500; cx = unpowered_x; cy = unpowered_y;
    press(KEY_A);
    assert(mode == MODE_BUILD && minerals == 500);
    assert(side_building_count(B_PHOTON_CANNON) == before);

    cx = powered_x; cy = powered_y;
    minerals = 149; press(KEY_A);
    assert(mode == MODE_BUILD && minerals == 149);
    assert(side_building_count(B_PHOTON_CANNON) == before);

    minerals = 150; press(KEY_A);
    assert(mode == MODE_NORMAL && minerals == 0);
    int cannon = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_PHOTON_CANNON &&
            b[i].under_construction) { cannon = i; break; }
    assert(cannon >= 0);
    if (u[probe].order == ORDER_BUILD) assert(u[probe].target == cannon);
    assert(b[cannon].alive && b[cannon].owner == SIDE &&
           b[cannon].type == B_PHOTON_CANNON && b[cannon].under_construction);
    assert(b[cannon].maxhp == 100 && b[cannon].shield == 100 &&
           b[cannon].build_progress >= 0);
    assert(side_building_count(B_PHOTON_CANNON) == before + 1);

    /* Reach the site and verify the Protoss worker is released immediately. */
    if (u[probe].order == ORDER_BUILD) {
        u[probe].x = b[cannon].x; u[probe].y = b[cannon].y;
        assert(race_builder_arrived(probe));
    }
    assert(u[probe].alive && u[probe].order == ORDER_IDLE && u[probe].target == -1);
    int duration = (sc64_race_building_rules[B_PHOTON_CANNON].build_frames * 5 + 3) / 4;
    int remaining = duration - b[cannon].build_progress;
    assert(remaining > 0);
    for (int tick = 0; tick < remaining - 1; ++tick) race_tick_buildings();
    assert(!building_complete(cannon));
    race_tick_buildings();
    assert(building_complete(cannon));
    assert(race_site_powered(SIDE, b[cannon].x, b[cannon].y));
}

static void test_cannon_targets_ground_and_air(void)
{
    int cannon = -1, ground, air;
    held_keys = 0;
    gba_init(); game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1;
    race_skirmish_init(); screen = SCREEN_PLAY;
    int pylon = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_PYLON) pylon = i;
    assert(pylon >= 0);
    int forge = spawn_build(SIDE, B_FORGE, b[pylon].x + 32, b[pylon].y + 32, 0);
    assert(forge >= 0);
    int x = -1, y = -1;
    assert(find_site(1, 1, b[pylon].x, b[pylon].y, &x, &y));
    cannon = spawn_build(SIDE, B_PHOTON_CANNON, x, y, 0);
    assert(cannon >= 0 && building_complete(cannon));

    ground = spawn_unit(ENEMY, U_ZERGLING, b[cannon].x + 24, b[cannon].y);
    air = spawn_unit(ENEMY, U_MUTALISK, b[cannon].x + 32, b[cannon].y);
    assert(ground >= 0 && air >= 0);
    int ground_hp = u[ground].hp, air_hp = u[air].hp;
    race_tick_defenses();
    assert(u[ground].hp < ground_hp);
    assert(u[air].hp == air_hp);

    u[ground].alive = 0;
    b[cannon].queue_progress = 0;
    race_tick_defenses();
    assert(u[air].hp < air_hp);

    /* An unpowered completed Cannon does not fire. */
    int powered_hp = u[air].hp;
    b[pylon].alive = 0;
    b[cannon].queue_progress = 0;
    race_tick_defenses();
    assert(u[air].hp == powered_hp);
}

int main(void)
{
    test_context_and_menu();
    test_prerequisite_power_payment_and_completion();
    test_cannon_targets_ground_and_air();
    puts("Probe Photon Cannon UI/payment/warp-in/defense checks passed");
    return 0;
}
