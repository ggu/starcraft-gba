/* Optional host-side normal-input controller for reproducible skirmish runs.
 * Observations may guide decisions; gameplay changes go through game_update(). */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_MATCH_LIMIT 60000u

static int option_map = 1;
static int option_human = R_TER;
static int option_bots = 3;
static int option_bot_races[3] = { R_TER, R_ZERG, R_PROTOSS };
static unsigned match_limit = DEFAULT_MATCH_LIMIT;
static u16 prior_keys;
static unsigned host_ticks;
static FILE *trace;
static int trace_first = 1;

enum build_state { BUILD_NONE, BUILD_PENDING, BUILD_FOUNDATION };
static struct {
    enum build_state state;
    int worker, type, x, y, building, complete;
} combat_build;

static void observe_combat_build(void);

static void trace_record(const char *format, ...)
{
    if (!trace) return;
    if (!trace_first) fputs(",\n", trace);
    trace_first = 0;
    va_list args;
    va_start(args, format);
    vfprintf(trace, format, args);
    va_end(args);
}

static void input(u16 held)
{
    u16 pressed = (u16)(held & (u16)~prior_keys);
    int changed = held != prior_keys;
    if (screen == SCREEN_RESULT || won || lost || host_ticks >= match_limit) return;
    game_update(held, pressed);
    prior_keys = held;
    ++host_ticks;
    if (trace && changed) {
        trace_record("{\"kind\":\"input\",\"host_tick\":%u,\"frame\":%d,\"held\":%u,\"pressed\":%u}",
                     host_ticks, frame, (unsigned)held, (unsigned)pressed);
    }
    observe_combat_build();
}

static void tap(u16 key) { input(key); input(0); }

static int launch_match(void)
{
    game_init();
    prior_keys = 0;
    tap(KEY_DOWN); /* HOME: Custom game */
    tap(KEY_A);
    if (screen != SCREEN_TITLE || race_front.page != SC_FRONT_SKIRMISH) return 0;
    /* Configure the requested imported map, human race, and bot races. */
    while (race_front.row > 0) tap(KEY_UP);
    for (int n = 0; race_front.map != option_map && n <= SC64_SKIRMISH_IMPORTED_COUNT; ++n)
        tap(KEY_RIGHT);
    if (race_front.map != option_map) return 0;
    tap(KEY_DOWN);
    for (int n = 0; race_front.race != option_human && n < 3; ++n) tap(KEY_RIGHT);
    if (race_front.race != option_human) return 0;
    tap(KEY_DOWN);
    for (int n = 0; race_front.bots != option_bots && n < 3; ++n) tap(KEY_RIGHT);
    if (race_front.bots != option_bots) return 0;
    for (int i = 0; i < option_bots; ++i) {
        tap(KEY_DOWN);
        for (int n = 0; race_front.bot_races[i] != option_bot_races[i] && n < 3; ++n)
            tap(KEY_RIGHT);
        if (race_front.bot_races[i] != option_bot_races[i]) return 0;
    }
    while (race_front.row < 3 + option_bots) tap(KEY_DOWN);
    tap(KEY_A);
    if (screen != SCREEN_PLAY || !race_mode || race_human != option_human ||
        race_bots != option_bots || race_skirmish_map != option_map) return 0;
    for (int i = 0; i < option_bots; ++i)
        if (race_bot_races[i] != option_bot_races[i]) return 0;
    if (trace) {
        trace_record("{\"kind\":\"start\",\"host_tick\":%u,\"frame\":%d,\"map\":%d,\"human_race\":%d,\"bots\":[",
                     host_ticks, frame, race_skirmish_map, race_human);
        for (int i = 0; i < race_bots; ++i)
            fprintf(trace, "%s{\"owner\":%d,\"race\":%d}", i ? "," : "",
                    race_bot_owner_ids[i], race_bot_races[i]);
        fputs("]}", trace);
    }
    return 1;
}

static void move_cursor(int x, int y);
static void clear_building_selection(void);

static int select_unit(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE) return 0;
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) tap(KEY_B);
    clear_building_selection();
    if (u[slot].selected && selected_count() == 1) return 1;
    for (int attempt = 0; attempt < 4 && u[slot].alive && screen == SCREEN_PLAY; ++attempt) {
        unsigned guard = 0;
        /* A worker may move while the cursor travels. Follow its current
         * position and click only while the UI would select that worker. */
        while (u[slot].alive && selection_target() != slot && guard++ < 6000u &&
               host_ticks < match_limit && screen == SCREEN_PLAY && !won && !lost) {
            int dx = u[slot].x - cx, dy = u[slot].y - cy;
            if (abs(dx) <= 1 && abs(dy) <= 1) break;
            u16 held = (abs(dx) >= 4 && (abs(dy) >= 4 || abs(dy) <= 1)) ||
                       (abs(dy) >= 4 && abs(dx) <= 1) ? KEY_R : 0;
            if (dx > 1) held |= KEY_RIGHT;
            if (dx < -1) held |= KEY_LEFT;
            if (dy > 1) held |= KEY_DOWN;
            if (dy < -1) held |= KEY_UP;
            input(held);
        }
        if (selection_target() != slot) break;
        tap(KEY_A);
        if (u[slot].selected && selected_count() == 1) return 1;
    }
    fprintf(stderr, "unit select miss slot%d raw%d xy%d,%d cursor%d,%d selected%d count%d target%d\n",
            slot, u[slot].raw_type, u[slot].x, u[slot].y, cx, cy,
            u[slot].selected, selected_count(), selection_target());
    return 0;
}

static int select_building(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive || b[slot].owner != SIDE) return 0;
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) tap(KEY_B);
    clear_building_selection();
    move_cursor(b[slot].x, b[slot].y);
    tap(KEY_A);
    return selected_building == slot;
}

static void move_cursor(int x, int y)
{
    unsigned guard = 0;
    while ((abs(cx - x) > 1 || abs(cy - y) > 1) && guard++ < 6000u && host_ticks < match_limit && screen == SCREEN_PLAY &&
           !won && !lost) {
        int dx = x - cx, dy = y - cy;
        u16 held = (abs(dx) >= 4 && (abs(dy) >= 4 || abs(dy) <= 1)) ||
                   (abs(dy) >= 4 && abs(dx) <= 1) ? KEY_R : 0;
        if (dx > 1) held |= KEY_RIGHT;
        if (dx < -1) held |= KEY_LEFT;
        if (dy > 1) held |= KEY_DOWN;
        if (dy < -1) held |= KEY_UP;
        input(held);
    }
    input(0);
}

static void clear_building_selection(void)
{
    if (selected_building < 0) return;
    move_cursor(900, 700);
    input(KEY_L);
    unsigned guard = 0;
    while ((cx < 916 || cy < 716) && guard++ < 1000u && host_ticks < match_limit && screen == SCREEN_PLAY) {
        u16 held = KEY_L;
        if (cx < 916) held |= KEY_RIGHT;
        if (cy < 716) held |= KEY_DOWN;
        input(held);
    }
    input(0);
}

static int choose_menu_command(int command)
{
    if (menu_open) tap(KEY_B);
    tap(KEY_B);
    for (int guard = 0; menu_open && menu_cursor != command && guard < 32; ++guard) {
#ifdef SC64_COMMAND_ICONS
        if (card_fits()) {
            int slot = card_command_slot(menu_cursor);
            int target_col = card_command_slot(command) % CARD_COLUMNS;
            int row_length = field_menu_count() - (slot / CARD_COLUMNS) * CARD_COLUMNS;
            if (row_length > CARD_COLUMNS) row_length = CARD_COLUMNS;
            if (slot % CARD_COLUMNS != target_col) {
                /* A short last row may not contain the target column. */
                tap(target_col >= row_length ? KEY_UP : KEY_RIGHT);
            } else {
                tap(KEY_DOWN);
            }
        } else
#endif
            tap(KEY_DOWN);
    }
    if (!menu_open || menu_cursor != command) return 0;
    tap(KEY_A);
    return 1;
}

static int count_units(int owner, int type)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        count += u[i].alive && u[i].owner == owner && (type < 0 || u[i].type == type);
    return count;
}

static int count_buildings(int owner, int type)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        count += b[i].alive && b[i].owner == owner && (type < 0 || b[i].type == type);
    return count;
}

static int closest_hostile_building(void)
{
    int best = -1, best_distance = 0x7fffffff;
    int sx = race_base_x[0], sy = race_base_y[0];
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && owners_hostile(SIDE, b[i].owner)) {
        int d = dist2(sx, sy, b[i].x, b[i].y);
        if (d < best_distance) { best_distance = d; best = i; }
    }
    return best;
}

static int choose_build_site(int type, int *out_x, int *out_y)
{
    int best_distance = 0x7fffffff, found = 0;
    for (int ring = 0; ring < 3; ++ring)
        for (int y = race_base_y[0] - 96 - ring * 32; y <= race_base_y[0] + 96 + ring * 32; y += 16)
            for (int x = race_base_x[0] - 96 - ring * 32; x <= race_base_x[0] + 96 + ring * 32; x += 16) {
                int site_x = x, site_y = y;
#ifdef SC64_FIELD_CONTROLS
                field_ui_site(type, &site_x, &site_y);
#endif
                if (site_x < 24 || site_y < 24 || site_x >= WORLD_W - 24 || site_y >= WORLD_H - 24 ||
                    !race_can_site(SIDE, type, site_x, site_y) || !can_place(type, site_x, site_y)) continue;
                int d = dist2(race_base_x[0], race_base_y[0], site_x, site_y);
                if (d < best_distance) { best_distance = d; *out_x = site_x; *out_y = site_y; found = 1; }
            }
    return found;
}

/* Observe on every tick, including ticks spent navigating menus. A Drone may
 * disappear and a Probe may be idle immediately after creating a foundation. */
static void observe_combat_build(void)
{
    if (combat_build.state == BUILD_NONE) return;
    if (combat_build.state == BUILD_PENDING) {
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
            if (b[i].alive && b[i].owner == SIDE && b[i].type == combat_build.type &&
                b[i].x == combat_build.x && b[i].y == combat_build.y) {
                combat_build.state = BUILD_FOUNDATION;
                combat_build.building = i;
                fprintf(stderr, "build: foundation tick%u slot%d type%d bank%d/%d\n",
                        host_ticks, i, combat_build.type, minerals, gas);
                break;
            }
        }
        if (combat_build.state == BUILD_PENDING &&
            (!u[combat_build.worker].alive || u[combat_build.worker].owner != SIDE ||
             u[combat_build.worker].order != ORDER_BUILD_PENDING ||
             u[combat_build.worker].target != combat_build.type ||
             u[combat_build.worker].tx != combat_build.x ||
             u[combat_build.worker].ty != combat_build.y)) {
            fprintf(stderr, "build: request ended without foundation tick%u worker%d\n",
                    host_ticks, combat_build.worker);
            combat_build.state = BUILD_NONE;
        }
    }
    if (combat_build.state == BUILD_FOUNDATION) {
        int i = combat_build.building;
        if (!b[i].alive || b[i].owner != SIDE || b[i].type != combat_build.type ||
            b[i].x != combat_build.x || b[i].y != combat_build.y) {
            fprintf(stderr, "build: foundation lost tick%u slot%d\n", host_ticks, i);
            combat_build.state = BUILD_NONE;
        } else if (!combat_build.complete && building_complete(i)) {
            combat_build.complete = 1;
            fprintf(stderr, "build: complete tick%u slot%d type%d\n", host_ticks, i, combat_build.type);
        }
    }
}

static int build_barracks(void)
{
    int worker = -1, x = -1, y = -1;
    int build_type = option_human == R_ZERG ? B_SPAWNING_POOL :
                     option_human == R_PROTOSS ? B_GATEWAY : B_BARRACKS;
    int worker_type = option_human == R_ZERG ? U_DRONE :
                      option_human == R_PROTOSS ? U_PROBE : U_SCV;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (build_worker_available(i, SIDE, worker_type)) {
            worker = i; break;
        }
    if (worker < 0) { fprintf(stderr, "build: no free worker type%d\n", worker_type); return 0; }
    if (!choose_build_site(build_type, &x, &y)) {
        fprintf(stderr, "build: no legal site type%d minerals%d\n", build_type, minerals); return 0;
    }
    if (!select_unit(worker)) {
        fprintf(stderr, "build: worker selection failed slot%d xy%d,%d mode%d menu%d\n",
                worker, u[worker].x, u[worker].y, mode, menu_open); return 0;
    }
    if (!choose_menu_command(MENU_BARRACKS)) {
        fprintf(stderr, "build: cannot navigate command menu cursor%d\n", menu_cursor);
        return 0;
    }
    if (mode != MODE_BUILD || menu != build_type) {
        fprintf(stderr, "build: expected menu type%d got mode%d type%d selected%d\n",
                build_type, mode, menu, selected_count()); return 0;
    }
    move_cursor(x, y);
    if (screen != SCREEN_PLAY || won || lost || host_ticks >= match_limit ||
        mode != MODE_BUILD || abs(cx - x) > 1 || abs(cy - y) > 1 ||
        !u[worker].alive || !u[worker].selected) return 0;
    /* Track the snapped UI site, which can differ from the cursor position. */
    x = cx; y = cy;
#ifdef SC64_FIELD_CONTROLS
    field_ui_site(build_type, &x, &y);
#endif
    int minerals_before = minerals, gas_before = gas;
    combat_build.state = BUILD_PENDING;
    combat_build.worker = worker; combat_build.type = build_type;
    combat_build.x = x; combat_build.y = y; combat_build.building = -1;
    combat_build.complete = 0;
    unsigned before_click = host_ticks;
    input(KEY_A);
    int accepted = host_ticks > before_click && mode == MODE_NORMAL && combat_build.state != BUILD_NONE;
    if (!accepted) combat_build.state = BUILD_NONE;
    input(0);
    fprintf(stderr, "build: issued type%d at%d,%d worker%d order%d mode%d bank%d/%d\n",
            build_type, x, y, worker, u[worker].order, mode, minerals, gas);
    if (accepted)
        trace_record("{\"kind\":\"action\",\"action\":\"build\",\"host_tick\":%u,\"frame\":%d,\"type\":%d,\"worker\":%d,\"x\":%d,\"y\":%d,\"minerals_before\":%d,\"minerals_after\":%d,\"gas_before\":%d,\"gas_after\":%d}",
                     host_ticks, frame, build_type, worker, x, y, minerals_before, minerals,
                     gas_before, gas);
    return accepted;
}

static int human_combat_building(void)
{
    return option_human == R_ZERG ? B_SPAWNING_POOL :
           option_human == R_PROTOSS ? B_GATEWAY : B_BARRACKS;
}

static int human_producer_type(void)
{
    return option_human == R_ZERG ? B_HATCHERY :
           option_human == R_PROTOSS ? B_GATEWAY : B_BARRACKS;
}

static int human_combat_unit(void)
{
    return option_human == R_ZERG ? U_ZERGLING :
           option_human == R_PROTOSS ? U_ZEALOT : U_MARINE;
}

static int record_attack_result(int x, int y, int issued)
{
    if (mode != MODE_NORMAL || issued <= 0) return 0;
    trace_record("{\"kind\":\"action\",\"action\":\"attack_move\",\"host_tick\":%u,\"frame\":%d,\"x\":%d,\"y\":%d,\"selected_issued\":%d}",
                 host_ticks, frame, x, y, issued);
    return 1;
}

static int attack_order(int x, int y)
{
    if (menu_open) tap(KEY_B);
    if (mode != MODE_NORMAL) tap(KEY_B);
    int seed = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !sc64_race_is_worker(u[i].type) &&
            u[i].type != U_OVERLORD) { seed = i; break; }
    if (seed < 0 || !select_unit(seed)) return 0;
    tap(KEY_R);
    if (selected_count() <= 0) return 0;
    if (!choose_menu_command(MENU_ATTACK)) return 0;
    if (mode != MODE_ATTACK) return 0;
    move_cursor(x, y);
    /* Cursor travel can end the match before the target click. Existing
     * attack-move orders then say nothing about this attempted gesture. */
    if (screen != SCREEN_PLAY || won || lost || host_ticks >= match_limit ||
        mode != MODE_ATTACK || abs(cx - x) > 1 || abs(cy - y) > 1) return 0;
    unsigned before_click = host_ticks;
    tap(KEY_A);
    if (host_ticks == before_click) return 0;
    int issued = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        issued += u[i].alive && u[i].owner == SIDE && u[i].selected &&
                  u[i].order == ORDER_ATTACK_MOVE;
    return record_attack_result(x, y, issued);
}

static void write_progress(void)
{
    fprintf(stderr, "tick=%u frame=%d result=%d won=%d lost=%d bank=%d/%d ",
            host_ticks, frame, screen, won, lost, minerals, gas);
    static const int owners[4] = { SIDE, ENEMY, RESCUABLE, 4 };
    for (int p = 0; p < 4; ++p) {
        int owner = owners[p];
        fprintf(stderr, "owner%d(u%d,b%d,m%d,g%d) ", owner,
                count_units(owner, -1), count_buildings(owner, -1),
                race_owner_minerals(owner), race_owner_gas(owner));
    }
    fputc('\n', stderr);
}

int main(int argc, char **argv)
{
    const char *trace_path;
    char *end = NULL;
    long value;
    if (argc < 5 || argc > 6) {
        fprintf(stderr, "usage: %s MAP HUMAN_RACE BOT_COUNT TRACE_PATH [MAX_TICKS]\n"
                        "  MAP: 0 or 1; HUMAN_RACE: 0 Terran, 1 Zerg, 2 Protoss; BOT_COUNT: 1..3\n",
                argv[0]);
        return 2;
    }
    value = strtol(argv[1], &end, 10);
    if (!end || *end || value < 0 || value > SC64_SKIRMISH_IMPORTED_COUNT) goto invalid_args;
    option_map = (int)value;
    end = NULL;
    value = strtol(argv[2], &end, 10);
    if (!end || *end || value < R_TER || value > R_PROTOSS) goto invalid_args;
    option_human = (int)value;
    end = NULL;
    value = strtol(argv[3], &end, 10);
    if (!end || *end || value < 1 || value > 3) goto invalid_args;
    option_bots = (int)value;
    for (int i = 0; i < option_bots; ++i)
        option_bot_races[i] = (option_human + 1 + i) % 3;
    trace_path = argv[4];
    if (argc == 6) {
        end = NULL;
        value = strtol(argv[5], &end, 10);
        if (!end || *end || value < 1 || value > 1000000L) goto invalid_args;
        match_limit = (unsigned)value;
    }
    trace = fopen(trace_path, "w");
    if (!trace) { perror(trace_path); return 2; }
    fputs("{\"profile\":\"unified-race-skirmish\",\"events\":[\n", trace);
    gba_init();
    if (!launch_match()) {
        fprintf(stderr, "normal-menu launch failed: screen=%d page=%d map=%d human=%d bots=%d\n",
                screen, race_front.page, race_skirmish_map, race_human, race_bots);
        fclose(trace); return 1;
    }
    fprintf(stderr, "launched map%d human race%d vs %d bots [", race_skirmish_map, race_human, race_bots);
    for (int i = 0; i < race_bots; ++i) fprintf(stderr, "%s%d", i ? "," : "", race_bot_races[i]);
    fprintf(stderr, "], bases=(%d,%d),(%d,%d),(%d,%d),(%d,%d)\n",
            race_base_x[0], race_base_y[0], race_base_x[1], race_base_y[1],
            race_base_x[2], race_base_y[2], race_base_x[3], race_base_y[3]);

    unsigned next_production = 1800, next_attack = 4200, next_report = 3000;
    while (host_ticks < match_limit && screen == SCREEN_PLAY && !won && !lost) {
        input(0);
        const Sc64RaceBuildingRule *combat_building = sc64_race_building_rule(human_combat_building());
        if (combat_build.state == BUILD_NONE && combat_building && minerals >= combat_building->minerals && host_ticks >= 1200) {
            int barracks_requested = build_barracks();
            fprintf(stderr, "barracks attempt tick%u accepted%d buildingcount%d\n",
                    host_ticks, barracks_requested, count_buildings(SIDE, human_combat_building()));
        }
        if (host_ticks >= next_production && combat_build.state == BUILD_FOUNDATION &&
            combat_build.complete) {
            int producer = -1;
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (b[i].alive && b[i].owner == SIDE && b[i].type == human_producer_type() &&
                    building_complete(i) && b[i].queue_count < 5 &&
                    (option_human != R_ZERG || b[i].larva > 0)) { producer = i; break; }
            if (producer >= 0 && minerals >= 50 && select_building(producer)) {
                int wanted = human_combat_unit();
                for (int step = 0; train_type != wanted && step < 5; ++step) tap(KEY_R);
                fprintf(stderr, "production tick%u producer%d type%d train%d wanted%d queue%d larva%d bank%d/%d\n",
                        host_ticks, producer, b[producer].type, train_type, wanted,
                        b[producer].queue_count, b[producer].larva, minerals, gas);
                unsigned before_click = host_ticks;
                int minerals_before = minerals, gas_before = gas;
                int queue_before = b[producer].queue_count;
                int attempted = train_type == wanted && screen == SCREEN_PLAY && !won && !lost &&
                    host_ticks < match_limit && b[producer].alive && b[producer].owner == SIDE &&
                    b[producer].type == human_producer_type() && selected_building == producer &&
                    minerals >= sc64_race_unit_rule(wanted)->minerals;
                if (attempted)
                    tap(KEY_A);
                if (attempted && host_ticks > before_click && b[producer].queue_count > queue_before)
                    trace_record("{\"kind\":\"action\",\"action\":\"queue\",\"host_tick\":%u,\"frame\":%d,\"producer\":%d,\"type\":%d,\"minerals_before\":%d,\"minerals_after\":%d,\"gas_before\":%d,\"gas_after\":%d}",
                                 host_ticks, frame, producer, wanted, minerals_before, minerals,
                                 gas_before, gas);
            }
            next_production += 900;
        }
        if (host_ticks >= next_attack) {
            int target = closest_hostile_building();
            if (target >= 0) {
                fprintf(stderr, "human attack order tick%u target slot%d owner%d type%d xy%d,%d combat%d\n",
                        host_ticks, target, b[target].owner, b[target].type, b[target].x, b[target].y,
                        count_units(SIDE, human_combat_unit()));
                (void)attack_order(b[target].x, b[target].y);
            }
            next_attack += 2400;
        }
        if (host_ticks >= next_report) {
            write_progress();
            next_report += 3000;
        }
    }
    write_progress();
    const char *outcome = won ? "won" : lost ? "lost" : "limit";
    fprintf(trace, "\n],\"terminal\":{\"kind\":\"%s\",\"host_tick\":%u,\"frame\":%d,\"won\":%d,\"lost\":%d,"
            "\"map\":%d,\"human_race\":%d,\"bot_count\":%d,\"human\":{\"owner\":%d,\"race\":%d,\"units\":%d,\"buildings\":%d},\"bots\":[",
            outcome, host_ticks, frame, won, lost, race_skirmish_map, race_human,
            race_bots, SIDE, race_human, count_units(SIDE, -1), count_buildings(SIDE, -1));
    for (int i = 0; i < race_bots; ++i) {
        int owner = race_bot_owner_ids[i];
        fprintf(trace, "%s{\"owner\":%d,\"race\":%d,\"units\":%d,\"buildings\":%d}",
                i ? "," : "", owner, race_bot_races[i], count_units(owner, -1),
                count_buildings(owner, -1));
    }
    fputs("]}}\n", trace);
    fclose(trace);
    fprintf(stderr, "terminal=%s trace=%s\n", outcome, trace_path);
    return won ? 0 : lost ? 3 : 4;

invalid_args:
    fprintf(stderr, "invalid arguments; usage: %s MAP HUMAN_RACE BOT_COUNT TRACE_PATH [MAX_TICKS]\n",
            argv[0]);
    return 2;
}
