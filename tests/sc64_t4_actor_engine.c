/* T4 actor policy tests through the actual game engine helpers. These check
 * selection, movement, collision, sight and damage rules; they do not claim
 * campaign mission completion or trigger fidelity. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void clear_actors(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(path_walkable, 0, sizeof(path_walkable));
    selected_building = -1;
    path_grid_valid = 0;
    race_mode = 0;
    race_selected_mission = SC64_T4_GLOBAL_ID;
    memset(sc64_unit_bucket_head, 0xff, sizeof(sc64_unit_bucket_head));
    sc64_unit_bucket_initialized = 1;
    sc64_unit_bucket_dirty_count = 0;
    sc64_unit_bucket_active = 0;
    sc64_visibility_reset();
}

static int put_unit(int owner, int type, int raw, int x, int y)
{
    int i = spawn_unit(owner, type, x, y);
    assert(i >= 0);
    u[i].raw_type = raw;
    u[i].flags = 0;
    u[i].order = ORDER_IDLE;
    return i;
}

static void test_select_and_orders(void)
{
    int disk, rhyno, reveal, civ, ghost, picked;
    clear_actors();
    disk = put_unit(SIDE, U_DATA_DISK, 218, 80, 80);
    rhyno = put_unit(SIDE, U_RHYNADON, 89, 90, 80);
    reveal = put_unit(SIDE, U_MAP_REVEALER, 101, 95, 85);
    civ = put_unit(SIDE, U_CIVILIAN, 15, 100, 80);
    ghost = put_unit(SIDE, U_GHOST, 1, 105, 80);
    cx = 80; cy = 80;
    assert(selection_target() == civ || selection_target() == ghost);
    assert(selection_target() != disk && selection_target() != rhyno &&
           selection_target() != reveal);
    picked = select_at();
    assert(picked == civ || picked == ghost);
    select_visible_type(U_GHOST);
    assert(u[ghost].selected && !u[civ].selected && !u[disk].selected);
    box_anchor_x = 75; box_anchor_y = 75; box_end_x = 145; box_end_y = 90;
    select_box_units();
    assert(u[civ].selected && u[ghost].selected && !u[disk].selected &&
           !u[rhyno].selected && !u[reveal].selected);
    clear_selection();
    cycle_selection();
    assert(u[civ].selected || u[ghost].selected);
    select_combat_group();
    assert(u[ghost].selected && !u[civ].selected && !u[rhyno].selected);
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    u[disk].selected = u[reveal].selected = 1;
    issue_order(ORDER_MOVE, 200, 200);
    assert(u[disk].order == ORDER_IDLE && u[reveal].order == ORDER_IDLE);
    assert(sc64_race_t4_actor_flags(u[rhyno].type)&SC64_T4_ACTOR_MOVABLE);
    u[civ].selected = 1;
    issue_order(ORDER_MOVE, 200, 200);
    assert(u[civ].order == ORDER_MOVE);
}

static void test_collision_and_nonblocking_building(void)
{
    int civilian, disk, beast;
    init_state();
    clear_actors();
    memset(path_walkable, 0xff, sizeof(path_walkable));
    civilian = put_unit(SIDE, U_CIVILIAN, 15, 80, 80);
    disk = put_unit(NEUTRAL, U_DATA_DISK, 218, 84, 80);
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_step_clear(&u[civilian], 81, 80));
    u[disk].alive = 0;
    beast = put_unit(NEUTRAL, U_RHYNADON, 89, 84, 80);
    (void)beast;
    sc64_unit_buckets_end();
    /* The uncached fallback must also ignore the non-solid Data Disk. */
    u[disk].alive = 1;
    u[beast].alive = 0;
    assert(unit_step_clear_race_fallback(&u[civilian], 81, 80));
    u[beast].alive = 1;
    assert(!unit_step_clear_race_fallback(&u[civilian], 81, 80));

    clear_actors();
    memset(path_walkable, 0xff, sizeof(path_walkable));
    b[0].alive = 1; b[0].type = B_TERRAN_BEACON; b[0].x = 80; b[0].y = 80;
    b[0].raw_type = 195;
    build_path_grid();
    assert(SC64_GRID_GET(path_walkable, 5 + 5 * MAP_COLS));
    {
        int found = 0;
        for (int y = 32; y < WORLD_H-32 && !found; y += 16)
            for (int x = 32; x < WORLD_W-32 && !found; x += 16)
                if (can_place(B_DEPOT, x, y)) {
                    b[0].alive = 0;
                    assert(can_place(B_DEPOT, x, y));
                    found = 1;
                }
        assert(found);
    }
}

static void test_sight_and_hidden_render(void)
{
    int reveal;
    u32 before = 0, after = 0;
    clear_actors();
    reveal = put_unit(SIDE, U_MAP_REVEALER, 101, 320, 320);
    assert(!target_alive(reveal));
    sc64_update_exploration();
    assert(SC64_GRID_GET(visible, (470 / 16) + (320 / 16) * MAP_COLS));
    assert(!SC64_GRID_GET(visible, (500 / 16) + (320 / 16) * MAP_COLS));
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x)
        before = (before ^ gfx_test_pixel(x, y)) * 16777619u;
    draw_unit(&u[reveal]);
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x)
        after = (after ^ gfx_test_pixel(x, y)) * 16777619u;
    assert(before == after);
}

static void check_exact_t4_source(int is_building, int type)
{
    static u8 expected_visible[MAP_CELLS], expected_explored[MAP_CELLS];
    static u16 expected_mini_visible[24*24], expected_mini_explored[24*24];
    int source=is_building ? sc64_race_t4_building_sight_source_pixels(type) :
                             sc64_race_t4_sight_source_pixels(type);
    int radius=source/2, tile_radius=source/32, x=900, y=900;
    assert(source>0 && radius>0 && tile_radius>0);
    clear_actors();
    SC64_GRID_CLEAR(explored);
    sc64_visibility_reset();
    memset(expected_visible,0,sizeof(expected_visible));
    memset(expected_explored,0,sizeof(expected_explored));
    memset(expected_mini_visible,0,sizeof(expected_mini_visible));
    memset(expected_mini_explored,0,sizeof(expected_mini_explored));
    if (is_building) assert(spawn_build(SIDE,type,x,y,0)>=0);
    else assert(put_unit(SIDE,type, type==U_GHOST?1:type==U_CIVILIAN?15:
                         type==U_RHYNADON?89:type==U_RAGNASAUR?95:
                         type==U_MAP_REVEALER?101:218,x,y)>=0);
    sc64_update_exploration();
    for (int row=0;row<MAP_ROWS;++row) for (int col=0;col<MAP_COLS;++col) {
        int dx=col*16+8-x,dy=row*16+8-y,cell=col+row*MAP_COLS;
        int tx=col-x/16,ty=row-y/16;
        expected_visible[cell]=(u8)(dx*dx+dy*dy<=radius*radius);
        expected_explored[cell]=(u8)(tx*tx+ty*ty<=tile_radius*tile_radius);
        int mini=sc64_minimap_cell(cell);
        expected_mini_visible[mini]+=expected_visible[cell];
        expected_mini_explored[mini]+=expected_explored[cell];
        assert(SC64_GRID_GET(visible,cell)==expected_visible[cell]);
        assert(SC64_GRID_GET(explored,cell)==expected_explored[cell]);
    }
    for(int i=0;i<24*24;++i) {
        assert(expected_mini_visible[i]==sc64_minimap_visible[i]);
        assert(expected_mini_explored[i]==sc64_minimap_explored[i]);
    }
}

static void test_all_real_t4_sight_rules(void)
{
    static const int actors[]={U_GHOST,U_CIVILIAN,U_RHYNADON,U_RAGNASAUR,
                               U_MAP_REVEALER,U_DATA_DISK};
    for (unsigned i=0;i<sizeof(actors)/sizeof(actors[0]);++i)
        check_exact_t4_source(0,actors[i]);
    check_exact_t4_source(1,B_TERRAN_BEACON);
}

static void test_ghost_and_intrinsic_immunity(void)
{
    int ghost, small, medium, large, air, disk, beacon;
    clear_actors();
    weapons_level = armor_level = 0;
    ghost = put_unit(SIDE, U_GHOST, 1, 40, 40);
    small = put_unit(ENEMY, U_MARINE, 0, 80, 40);
    u[small].armor = 2;
    hit_target(small, 10, U_GHOST, SIDE);
    assert(race_can_attack(U_GHOST, small));
    assert(u[small].hp == u[small].maxhp - 8); /* upgraded 10, minus 2 armor */
    medium = put_unit(ENEMY, U_VULTURE, 2, 80, 60);
    u[medium].shield = 0;
    u[medium].armor = 0;
    hit_target(medium, 10, U_GHOST, SIDE);
    assert(u[medium].hp == u[medium].maxhp - 5);
    large = put_unit(ENEMY, U_GOLIATH, 3, 80, 80);
    hit_target(large, 10, U_GHOST, SIDE);
    assert(u[large].hp == u[large].maxhp - 2);
    air = put_unit(ENEMY, U_WRAITH, 8, 80, 100);
    u[air].armor = 0; u[air].shield = 0;
    assert(sc64_race_unit_flying(u[air].type) && race_can_attack(U_GHOST, air));
    hit_target(air, 10, U_GHOST, SIDE);
    assert(u[air].hp == u[air].maxhp - 2);
    u[small].shield = 4;
    hit_target(small, 10, U_GHOST, SIDE);
    assert(u[small].shield == 0 && u[small].hp == u[small].maxhp - 12);
    disk = put_unit(ENEMY, U_DATA_DISK, 218, 100, 40);
    assert(invincible_units[disk]);
    assert(!race_can_attack(U_GHOST, disk));
    assert(!SC64_CAN_ATTACK_FROM(U_GHOST, disk, SIDE));
    { int hp = u[disk].hp; hit_target(disk, 100, U_GHOST, SIDE); assert(u[disk].hp == hp); }
    beacon = spawn_build(SIDE, B_TERRAN_BEACON, 100, 120, 0);
    assert(beacon >= 0 && b[beacon].maxhp == 100000 && b[beacon].hp == 100000);
    assert(invincible_buildings[beacon]);
    assert(!race_can_attack(U_GHOST, MAX_UNITS + beacon));
    assert(!SC64_CAN_ATTACK_FROM(U_GHOST, MAX_UNITS + beacon, SIDE));
    { int hp = b[beacon].hp; hit_target(MAX_UNITS + beacon, 100, U_GHOST, SIDE); assert(b[beacon].hp == hp); }
    assert(sc64_race_t4_actor_flags(U_CIVILIAN) & SC64_T4_ACTOR_SELECTABLE);
    assert(!(sc64_race_t4_actor_flags(U_RHYNADON) & SC64_T4_ACTOR_SELECTABLE));
    assert(sc64_race_t4_actor_flags(U_DATA_DISK) & SC64_T4_ACTOR_INVINCIBLE);
    assert(sc64_race_t4_building_flags(B_TERRAN_BEACON) & SC64_T4_ACTOR_INVINCIBLE);
    (void)ghost;
}

static void test_ghost_upgrades_shields_and_hidden_target(void)
{
    clear_actors();
    weapons_level = 2; armor_level = 0;
    int ghost = put_unit(SIDE, U_GHOST, 1, 40, 40);
    int target = put_unit(ENEMY, U_MARINE, 0, 80, 40);
    u[target].armor = 0; u[target].shield = 3;
    tick_combat(&u[ghost], target);
    assert(u[target].shield == 0 && u[target].hp == u[target].maxhp - 9);
    assert(u[ghost].cd == 28); /* Existing GBA 5/4 cooldown cadence. */
    int air = put_unit(ENEMY, U_WRAITH, 8, 80, 60);
    u[air].armor = 0; u[air].shield = 4; u[air].raw_type = -1;
    u[ghost].cd = 0;
    tick_combat(&u[ghost], air);
    assert(u[air].shield == 0 && u[air].hp == u[air].maxhp - 2);
    weapons_level = 0;
    int command = spawn_build(ENEMY, B_COMMAND, 80, 100, 0);
    assert(command >= 0);
    b[command].raw_type = -1; /* Produced buildings use their canonical raw size. */
    u[ghost].cd = 0;
    tick_combat(&u[ghost], MAX_UNITS + command);
    assert(b[command].hp == b[command].maxhp - 2);
    int hidden = put_unit(ENEMY, U_MAP_REVEALER, 101, 80, 80);
    assert(!race_can_attack(U_GHOST, hidden) && !target_alive(hidden));
    u[ghost].cd = 0;
    tick_combat(&u[ghost], hidden);
    assert(u[hidden].hp == 1 && u[ghost].cd == 0);
}

int main(void)
{
    test_select_and_orders();
    test_collision_and_nonblocking_building();
    test_sight_and_hidden_render();
    test_all_real_t4_sight_rules();
    test_ghost_and_intrinsic_immunity();
    test_ghost_upgrades_shields_and_hidden_target();
    puts("T4 actor engine policy tests passed");
    return 0;
}
