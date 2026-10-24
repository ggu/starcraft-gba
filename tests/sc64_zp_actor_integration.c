/* Dedicated Z/P actor behavior through the real engine paths. This fixture
 * does not claim campaign completion or imported-trigger fidelity. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u8 framebuffer[240 * 160] __attribute__((aligned(4)));

static void reset_world(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    path_grid_valid = 0;
    selected_building = -1;
    race_mode = 0;
    race_selected_mission = 0;
    camx = camy = 0;
    gfx_begin(C_BLACK);
    sc64_visibility_reset();
}

static void test_spawn_facts(void)
{
    int kerrigan, vulture, templar, ultralisk, revealer, beacon;
    reset_world();
    kerrigan = spawn_unit(SIDE, U_ZP_INFESTED_KERRIGAN, 80, 80);
    vulture = spawn_unit(SIDE, U_ZP_RAYNOR_VULTURE, 100, 80);
    templar = spawn_unit(SIDE, U_ZP_HIGH_TEMPLAR, 120, 80);
    ultralisk = spawn_unit(SIDE, U_ZP_ULTRALISK, 140, 80);
    revealer = spawn_unit(SIDE, U_ZP_MAP_REVEALER, 160, 80);
    beacon = spawn_build(ENEMY, B_ZP_PROTOSS_BEACON, 240, 240, 0);
    assert(kerrigan >= 0 && vulture >= 0 && templar >= 0 &&
           ultralisk >= 0 && revealer >= 0 && beacon >= 0);
    assert(u[kerrigan].raw_type == 51 && u[kerrigan].maxhp == 400 && u[kerrigan].armor == 2);
    assert(u[vulture].raw_type == 19 && u[vulture].maxhp == 300 && u[vulture].armor == 3);
    assert(u[templar].raw_type == 67 && u[templar].maxhp == 40 && u[templar].shield == 40);
    assert(u[ultralisk].raw_type == 39 && u[ultralisk].maxhp == 400 && u[ultralisk].armor == 1);
    assert(u[revealer].raw_type == 101 && u[revealer].maxhp == 1);
    assert(b[beacon].raw_type == 196 && b[beacon].maxhp == 100000 &&
           b[beacon].hp == 100000 && b[beacon].shield == 0 && invincible_buildings[beacon]);
    {
        int left, up, right, down;
        assert(sc64_game_building_bounds_source(B_ZP_PROTOSS_BEACON,
                                                &left, &up, &right, &down));
        assert(left == 48 && up == 32 && right == 47 && down == 31);
        path_grid_valid = 0;
        build_path_grid();
        int with_beacon[MAP_COLS * MAP_ROWS];
        for (int cell = 0; cell < MAP_COLS * MAP_ROWS; ++cell)
            with_beacon[cell] = SC64_GRID_GET(path_walkable, cell);
        b[beacon].alive = 0;
        path_grid_valid = 0;
        build_path_grid();
        int bounds_affected_navigation = 0;
        for (int cell = 0; cell < MAP_COLS * MAP_ROWS; ++cell)
            if (with_beacon[cell] != SC64_GRID_GET(path_walkable, cell))
                bounds_affected_navigation = 1;
        assert(bounds_affected_navigation);
    }
}

static void test_hidden_map_revealer(void)
{
    int reveal, enemy, mover;
    reset_world();
    reveal = spawn_unit(SIDE, U_ZP_MAP_REVEALER, 80, 80);
    enemy = spawn_unit(ENEMY, U_MARINE, 88, 80);
    mover = spawn_unit(SIDE, U_OVERLORD, 80, 80);
    assert(reveal >= 0 && enemy >= 0 && mover >= 0);
    u[reveal].order = ORDER_MOVE;
    u[reveal].tx = 220;
    assert(sc64_game_unit_has_flag(U_ZP_MAP_REVEALER, SC64_GAME_ACTOR_HIDDEN));
    assert(!sc64_game_unit_selectable(U_ZP_MAP_REVEALER));
    assert(!sc64_game_unit_has_flag(U_ZP_MAP_REVEALER, SC64_GAME_ACTOR_MOVABLE));
    assert(!sc64_game_unit_has_flag(U_ZP_MAP_REVEALER, SC64_GAME_ACTOR_COLLIDABLE));
    assert(!sc64_game_unit_has_flag(U_ZP_MAP_REVEALER, SC64_GAME_ACTOR_DRAWABLE));
    assert(!sc64_game_unit_has_flag(U_ZP_MAP_REVEALER, SC64_GAME_ACTOR_WEAPON));
    assert(sc64_game_unit_collision_radius(U_ZP_RAYNOR_VULTURE) == 8);
    assert(sc64_game_unit_collision_radius(U_ZP_ULTRALISK) == 10);

    cx = 80; cy = 80;
    assert(selection_target() == mover); /* Revealer cannot be selected. */
    assert(!race_can_attack(U_ZP_MAP_REVEALER, enemy));
    assert(!race_can_attack(U_MARINE, reveal));
    assert(!race_can_attack(-1, reveal)); /* Defensive target scans use this path. */
    assert(unit_step_clear_race_fallback(&u[mover], 81, 80));
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_step_clear(&u[mover], 81, 80));
    sc64_unit_buckets_end();
    move_toward(&u[reveal], 220, 80);
    assert(u[reveal].x == 80 && u[reveal].y == 80);

    sc64_visibility_reset();
    sc64_adjust_unit_visibility(u[reveal].x, u[reveal].y, u[reveal].type, 1);
    assert(SC64_GRID_GET(visible, 12 + 5 * MAP_COLS));

    gfx_begin(C_BLACK);
    draw_unit(&u[reveal]);
    assert(gfx_read_band(0, 160, framebuffer));
    assert(framebuffer[112 * 240 + 80] == C_BLACK);
    gfx_begin(C_BLACK);
    draw_minimap();
    assert(gfx_read_band(124, 34, framebuffer));
    /* A hidden actor never receives the unit dot at its coarse map point. */
    assert(framebuffer[(129 - 124) * 240 + 197 + sc64_minimap_pixel(80, MAP_COLS)] !=
           owner_color(SIDE));
}

static void test_weapon_facts_and_immortality(void)
{
    int kerrigan, vulture, templar, ultra, beacon;
    reset_world();
    kerrigan = spawn_unit(SIDE, U_ZP_INFESTED_KERRIGAN, 64, 64);
    vulture = spawn_unit(SIDE, U_ZP_RAYNOR_VULTURE, 64, 96);
    templar = spawn_unit(SIDE, U_ZP_HIGH_TEMPLAR, 64, 128);
    ultra = spawn_unit(SIDE, U_ZP_ULTRALISK, 64, 160);
    beacon = spawn_build(ENEMY, B_ZP_PROTOSS_BEACON, 240, 240, 0);
    assert(sc64_race_unit_damage_vs(u[kerrigan].type, 0) == 50);
    assert(unit_range(u[kerrigan].type) == 8); /* Source 15px -> GBA 8px. */
    assert(sc64_race_unit_cooldown_vs(u[kerrigan].type, 0) == 15);
    assert(sc64_race_unit_damage_vs(u[vulture].type, 0) == 30);
    assert(sc64_race_unit_damage_vs(u[vulture].type, 1) == 0);
    assert(sc64_race_unit_range_vs(u[vulture].type, 0) == 160);
    assert(unit_range(u[vulture].type) == 80);
    assert(sc64_race_unit_damage_vs(u[templar].type, 0) == 0);
    assert(sc64_race_unit_damage_vs(u[ultra].type, 0) == 20);
    assert(sc64_race_unit_range_vs(u[ultra].type, 0) == 25);
    assert(!race_can_attack(U_MARINE, MAX_UNITS + beacon));
    hit_target(MAX_UNITS + beacon, 50000, U_MARINE, SIDE);
    assert(b[beacon].hp == 100000);
}

static void test_source_bounds_collision(void)
{
    int ultralisk, marine;
    reset_world();
    ultralisk = spawn_unit(SIDE, U_ZP_ULTRALISK, 80, 80);
    marine = spawn_unit(ENEMY, U_MARINE, 89, 80);
    assert(ultralisk >= 0 && marine >= 0);
    assert(!unit_step_clear_race_fallback(&u[ultralisk], 81, 80));
    sc64_unit_buckets_begin();
    assert(!sc64_unit_bucket_step_clear(&u[ultralisk], 81, 80));
    sc64_unit_buckets_end();
}

int main(void)
{
    gba_init();
    test_spawn_facts();
    test_hidden_map_revealer();
    test_weapon_facts_and_immortality();
    test_source_bounds_collision();
    puts("Z/P actor engine integration PASS");
    return 0;
}
