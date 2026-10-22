/* Engine-level checks for the bounded, explicitly authored T5 world policy. */
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"

static void
clear_t5_world(void)
{
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b)); memset(m, 0, sizeof(m));
    memset(geysers, 0, sizeof(geysers)); geyser_count = 0;
    memset(&race_t5, 0, sizeof(race_t5));
    memset(race_minerals, 0, sizeof(race_minerals));
    memset(race_gas, 0, sizeof(race_gas));
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = R_TER; race_player[ENEMY] = R_TER;
    race_mode = 0; race_selected_mission = SC64_T5_GLOBAL_ID;
    selected_building = -1; won = lost = 0;
}

static int
make_t5_script(u32 id, unsigned owner, int with_area,
               const Sc64T4Location *area)
{
    Sc64T4Action action;
    memset(&action, 0, sizeof(action));
    action.opcode = with_area ? 16 : 15;
    action.player = 0; /* Native T5 rows retain this unused value. */
    action.argument = id;
    action.location = with_area ? 1 : 0;
    return sc64_t5_ai_action(&race_t5.ai, &action, owner, area,
                             &race_t5_ai_ops, 0);
}

static int
put_raw_unit(int owner, int raw_owner, int type, int x, int y)
{
    int i = spawn_unit(owner, type, x, y);
    assert(i >= 0);
    u[i].raw_type = sc64_race_raw_unit(type);
    u[i].raw_owner = raw_owner;
    u[i].order = ORDER_IDLE;
    return i;
}

static int
put_raw_building(int owner, int raw_owner, int type, int x, int y)
{
    int i = spawn_build(owner, type, x, y, 0);
    assert(i >= 0);
    b[i].raw_type = sc64_race_raw_building(type);
    b[i].raw_owner = raw_owner;
    return i;
}

static void
test_rscu_fallback_and_contact(void)
{
    int near, far, other;
    clear_t5_world();
    near = put_raw_unit(NEUTRAL, 3, U_MARINE, 100, 100);
    far = put_raw_unit(NEUTRAL, 3, U_MARINE, 400, 400);
    other = put_raw_unit(NEUTRAL, 5, U_MARINE, 102, 100);
    assert(make_t5_script(SC64_T5_SCRIPT_RSCU, 3, 0, 0) == 1);
    assert(u[near].owner == RESCUABLE && u[far].owner == RESCUABLE);
    assert(u[near].raw_owner == 3 && u[far].raw_owner == 3);
    assert(u[other].owner == NEUTRAL);
    assert(race_t5.ai.requested_scripts[3] & SC64_T5_SCRIPT_BIT_RSCU);

    put_raw_unit(SIDE, 0, U_SCV, 108, 100);
    race_t5_world_tick();
    assert(u[near].owner == SIDE && u[near].raw_owner == 3);
    assert(u[far].owner == RESCUABLE);
    assert(u[other].owner == NEUTRAL);
}

static void
test_nearby_human_depot_does_not_group_transfer(void)
{
    int group_a, group_b;
    clear_t5_world();
    put_raw_building(SIDE, 0, B_COMMAND, 500, 500);
    group_a = put_raw_unit(RESCUABLE, 3, U_MARINE, 520, 500);
    group_b = put_raw_unit(RESCUABLE, 3, U_SCV, 700, 700);
    race_t5_world_tick();
    assert(u[group_a].owner == SIDE);
    assert(u[group_b].owner == RESCUABLE);
    assert(u[group_b].raw_owner == 3);
}

static void
test_rescued_completed_command_center_transfers_group(void)
{
    int cc, group_a, group_b, other_group, human_attacker;
    clear_t5_world();
    put_raw_unit(SIDE, 0, U_SCV, 520, 500);
    cc = put_raw_building(RESCUABLE, 3, B_COMMAND, 500, 500);
    group_a = put_raw_unit(RESCUABLE, 3, U_MARINE, 520, 500);
    group_b = put_raw_unit(RESCUABLE, 3, U_SCV, 700, 700);
    other_group = put_raw_unit(RESCUABLE, 2, U_MARINE, 700, 700);
    human_attacker = put_raw_unit(SIDE, 0, U_MARINE, 530, 500);
    u[group_a].selected = 1;
    u[human_attacker].order = ORDER_ATTACK_TARGET;
    u[human_attacker].target = group_a;
    u[group_a].path_timer = 7; u[group_a].path_retry = 3;
    u[group_a].path_x = 99; u[group_a].path_y = 88;

    race_t5_world_tick();
    assert(b[cc].owner == SIDE && b[cc].raw_owner == 3);
    assert(u[group_a].owner == SIDE && u[group_b].owner == SIDE);
    assert(u[other_group].owner == RESCUABLE);
    assert(u[group_a].raw_owner == 3 && u[group_b].raw_owner == 3);
    assert(u[group_a].order == ORDER_IDLE && u[group_a].target == -1);
    assert(!u[group_a].selected && u[group_a].tx == u[group_a].x &&
           u[group_a].ty == u[group_a].y);
    assert(u[group_a].path_timer == 0 && u[group_a].path_retry == 0 &&
           u[group_a].path_x == u[group_a].x && u[group_a].path_y == u[group_a].y);
    /* The current attack target became friendly; clear that stale lock. */
    assert(u[human_attacker].order == ORDER_IDLE && u[human_attacker].target == -1);
}

static void
test_enbk_and_clrc_areas(void)
{
    const Sc64T4Location area = {
        .left = 200, .top = 200, .right = 300, .bottom = 300
    };
    int guard, outside, attack_inside, attack_outside;
    clear_t5_world();
    guard = put_raw_unit(ENEMY, 6, U_MARINE, 120, 120);
    outside = put_raw_unit(ENEMY, 6, U_MARINE, 200, 200);
    assert(make_t5_script(SC64_T5_SCRIPT_ENBK, 6, 1, &area) == 1);
    assert(u[guard].order == ORDER_ATTACK_MOVE);
    assert(u[guard].tx == 120 && u[guard].ty == 120);
    assert(u[outside].order == ORDER_IDLE);

    attack_inside = put_raw_unit(ENEMY, 6, U_MARINE, 124, 130);
    attack_outside = put_raw_unit(ENEMY, 6, U_MARINE, 190, 150);
    u[attack_inside].order = u[attack_outside].order = ORDER_ATTACK_TARGET;
    u[attack_inside].target = u[attack_outside].target = 0;
    assert(make_t5_script(SC64_T5_SCRIPT_CLRC, 6, 1, &area) == 1);
    assert(u[attack_inside].order == ORDER_IDLE && u[attack_inside].target == -1);
    assert(u[attack_outside].order == ORDER_ATTACK_TARGET &&
           u[attack_outside].target == 0);
}

static void
test_paid_backend_and_timing(void)
{
    const Sc64T4Location area = {
        .left = 10, .top = 10, .right = 60, .bottom = 60
    };
    int cc, barracks, depot, scv, marine, other_marine;
    clear_t5_world();
    race_minerals[ENEMY] = 6000;
    race_gas[ENEMY] = 6000;
    cc = put_raw_building(ENEMY, 6, B_COMMAND, 400, 400);
    barracks = put_raw_building(ENEMY, 6, B_BARRACKS, 450, 400);
    depot = put_raw_building(ENEMY, 6, B_DEPOT, 500, 400);
    (void)cc; (void)barracks; (void)depot;
    scv = put_raw_unit(ENEMY, 6, U_SCV, 400, 430);
    marine = put_raw_unit(ENEMY, 6, U_MARINE, 450, 430);
    other_marine = put_raw_unit(ENEMY, 6, U_MARINE, 460, 430);
    m[0].x = 420; m[0].y = 440; m[0].amount = 500;

    assert(make_t5_script(SC64_T5_SCRIPT_TER5, 6, 1, &area) == 1);
    assert(make_t5_script(SC64_T5_SCRIPT_TE5H, 6, 1, &area) == 1);
    int before_minerals = race_minerals[ENEMY];
    assert(race_t5_ai_backend_tick(0, 0));
    assert(race_minerals[ENEMY] == before_minerals);
    assert(b[cc].queue_count == 0 && b[barracks].queue_count == 0);
    assert(u[scv].order == ORDER_IDLE);

    assert(race_t5_ai_backend_tick(0, SC64_T5_AUTHORED_AI_PERIOD));
    assert(u[scv].order == ORDER_MINE && u[scv].target == 0);
    assert(b[cc].queue_count == 1 &&
           b[cc].queue[(b[cc].queue_head) % 5] == U_SCV);
    assert(b[barracks].queue_count == 1 &&
           b[barracks].queue[b[barracks].queue_head] == U_MARINE);
    assert(race_minerals[ENEMY] < before_minerals);

    u[marine].order = u[other_marine].order = ORDER_IDLE;
    put_raw_unit(SIDE, 0, U_SCV, 800, 800);
    assert(race_t5_ai_backend_tick(0, SC64_T5_AUTHORED_ATTACK_PERIOD));
    assert(u[marine].order == ORDER_ATTACK_MOVE ||
           u[other_marine].order == ORDER_ATTACK_MOVE);
}

int
main(void)
{
    test_rscu_fallback_and_contact();
    test_nearby_human_depot_does_not_group_transfer();
    test_rescued_completed_command_center_transfers_group();
    test_enbk_and_clrc_areas();
    test_paid_backend_and_timing();
    return 0;
}
