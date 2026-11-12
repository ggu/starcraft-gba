/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { STOCK_BANK = 2000, FIRST_SILO_X = 368, SECOND_SILO_X = 688,
       SILO_Y = 408, FORGE_X = 664, FORGE_Y = 736 };

static int fixture_build(int owner, int raw, int x, int y)
{
    int type = sc64_race_building_from_raw(raw);
    assert(type >= 0);
    int slot = spawn_build(owner, type, x, y, 0);
    assert(slot >= 0 && sc64_race_raw_building(b[slot].type) == raw);
    b[slot].raw_type = raw;
    return slot;
}

static int fixture_find(int raw, int x, int y)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw && b[i].x == x && b[i].y == y) return i;
    assert(!"fixture building disappeared");
    return -1;
}

static u32 base_tag(void)
{
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    return SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION;
#else
    return SC64_TRANSPORT_SAVE_VERSION;
#endif
}

static void fixture_start(void)
{
    gba_init(); game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1;
    race_bot_races[0] = R_PROTOSS; race_skirmish_map = 0;
    init_state();
    assert(screen == SCREEN_PLAY && race_player[SIDE] == R_TER && race_player[ENEMY] == R_PROTOSS);
    minerals = gas = STOCK_BANK;
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    fixture_build(ENEMY, 156, FORGE_X - 28, FORGE_Y);
    int forge = fixture_build(ENEMY, 166, FORGE_X, FORGE_Y);
    race_minerals[ENEMY] = race_gas[ENEMY] = 1000;
    assert(sc64_pr_stage_begin_slot(forge, SC64_PR_GROUND_WEAPONS));
    sc64_experimental_research_levels.level[ENEMY][SC64_PR_AIR_WEAPONS] = 2;
    sc64_pr_stage_resync();
#endif
}

static void fixture_ticks(unsigned count)
{
    while (count--) { ++frame; race_nuke_tick(); }
}

static void fixture_read(const char *path)
{
    FILE *f = fopen(path, "rb"); assert(f);
    assert(fread(host_save, 1, sizeof host_save, f) == sizeof host_save);
    assert(fgetc(f) == EOF && !fclose(f));
}

static void fixture_write(const char *path)
{
    FILE *f = fopen(path, "wb"); assert(f);
    assert(fwrite(host_save, 1, sizeof host_save, f) == sizeof host_save);
    assert(!fclose(f));
}

static GbaSaveSelection fixture_select(u32 expected)
{
    RaceLoadPlan plan;
    GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan, &selected) && selected.tag == expected);
    return selected;
}

static void fixture_assert_research(void)
{
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    Sc64PrPaidReceipt paid;
    assert(sc64_pr_stage_receipt(fixture_find(166, FORGE_X, FORGE_Y), &paid));
    assert(paid.live && paid.payer == ENEMY && paid.family == SC64_PR_GROUND_WEAPONS &&
           paid.target == 1 && sc64_pr_stage_jobs == 1);
    assert(sc64_experimental_research_levels.level[ENEMY][SC64_PR_AIR_WEAPONS] == 2);
#endif
}

static void write_receipts(const char *path)
{
    fixture_start();
    assert(race_save_game());
    GbaSaveSelection base = fixture_select(base_tag());
    assert(base.sequence == 1);
    int first_parent = fixture_build(SIDE, 106, FIRST_SILO_X - 48, SILO_Y - 8);
    int first = fixture_build(SIDE, 108, FIRST_SILO_X, SILO_Y);
    fixture_build(SIDE, 106, SECOND_SILO_X - 48, SILO_Y - 8);
    int second = fixture_build(SIDE, 108, SECOND_SILO_X, SILO_Y);
    int ghost = spawn_unit(SIDE, sc64_race_unit_from_raw(1), 720, SILO_Y + 8);
    assert(ghost >= 0 && sc64_race_ordinary_ghost(u[ghost].type));
    assert(race_nuke_arm(first, SIDE) == 4);
    fixture_ticks(1875);
    assert(race_nuke_silos[first].ammo.armed);
    race_nuke_detach_addon(first_parent);
    assert(b[first].owner == NEUTRAL && race_nuke_silos[first].payer == SIDE);
    assert(race_nuke_arm(second, SIDE) == 4);
    fixture_ticks(25);
    assert(race_nuke_silos[second].ammo.remaining == 1480);
    assert(race_nuke_reserved_supply(SIDE) == 32 && race_nuke_paid_silos == 2);
    assert(race_save_game());
    GbaSaveSelection stage = fixture_select(SC64_NUKE_SILO_TRANSPORT_TAG);
    assert(stage.sequence == 2 && stage.slot != base.slot && stage.length == base.length);
    assert(b[first].queue_progress == 0 && b[first].level == 0 &&
           b[second].queue_progress == 0 && b[second].level == 0);
    fixture_write(path);
    printf("wrote baseline %08x and Silo %08x, both %u bytes\n",
           base.tag, stage.tag, stage.length);
}

static void read_receipts(const char *path)
{
    gba_init(); game_init(); fixture_read(path);
    assert(race_load_game() && race_mode == 1 && race_human == R_TER && frame == 1900);
    int first = fixture_find(108, FIRST_SILO_X, SILO_Y);
    int second = fixture_find(108, SECOND_SILO_X, SILO_Y);
    assert(b[first].owner == NEUTRAL && race_nuke_silos[first].ammo.armed &&
           race_nuke_silos[first].payer == SIDE && !race_nuke_silo_online(first, SIDE));
    assert(race_nuke_silos[second].ammo.remaining == 1480 && race_nuke_silo_online(second, SIDE));
    assert(race_nuke_paid_silos == 2 && race_nuke_reserved_supply(SIDE) == 32 &&
           minerals == STOCK_BANK - 400 && gas == STOCK_BANK - 400);
    assert(!b[first].queue_progress && !b[first].level && !b[second].queue_progress && !b[second].level);
    fixture_assert_research();
    assert(race_save_game() && fixture_select(SC64_NUKE_SILO_TRANSPORT_TAG).sequence == 3);
    fixture_ticks(5);
    assert(race_nuke_silos[second].ammo.remaining == 1476);
    assert(race_nuke_cancel_arm(second, SIDE));
    assert(race_nuke_paid_silos == 1 && race_nuke_reserved_supply(SIDE) == 16 &&
           minerals == STOCK_BANK - 200 && gas == STOCK_BANK - 200);
    int parent = fixture_find(106, FIRST_SILO_X - 48, SILO_Y - 8);
    race_nuke_relink_neutral_addon(parent);
    assert(race_nuke_armed_slot(SIDE) == first);
    int ghost = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && sc64_race_ordinary_ghost(u[i].type)) ghost = i;
    assert(ghost >= 0);
    static u8 prior[sizeof host_save];
    memcpy(prior, host_save, sizeof prior);
    assert(race_nuke_issue(ghost, SIDE, u[ghost].x - 220, u[ghost].y));
    assert(race_nuke_track(ghost)->approaching && !race_save_game() &&
           memcmp(prior, host_save, sizeof prior) == 0);
    assert(race_nuke_abort(ghost) && race_nuke_armed_slot(SIDE) == first);
    assert(race_nuke_issue(ghost, SIDE, u[ghost].x + 16, u[ghost].y));
    assert(race_nuke_ghost_locked(ghost) && !race_save_game() &&
           memcmp(prior, host_save, sizeof prior) == 0);
    while (race_nuke_ghost_locked(ghost)) fixture_ticks(1);
    assert(race_nuke_reserved_supply(SIDE) == 16 && !race_save_game() &&
           memcmp(prior, host_save, sizeof prior) == 0);
    puts("fresh process restored queued and detached armed Silo; active flight preserved SRAM");
}

static void bad_payer(const char *path)
{
    gba_init(); game_init(); fixture_read(path);
    RaceLoadPlan plan;
    GbaSaveSelection stage;
    assert(race_worker_cargo_select(&plan, &stage) && stage.tag == SC64_NUKE_SILO_TRANSPORT_TAG);
    int row_slot = -1;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i)
        if (plan.nuke_silo_receipts[i] == 1480) row_slot = (int)i;
    assert(row_slot >= 0);
    Building row;
    assert(race_save_decode_building(&race_data, (unsigned)row_slot, &row));
    row.level = ENEMY + 1;
    const Sc64NukeSaveWorld world = race_nuke_save_world(1, 1, 1024, 1024);
    assert(race_nuke_save_replace_building(&race_data, (unsigned)row_slot, &row, &world));
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    assert(sc64_pr_padding_write((u8 *)race_data.buildings, MAX_BUILDINGS, SC64_PR_ROW_WIDTH,
                                  SC64_PR_USED_BITS, 5, &plan.research_levels));
#endif
    unsigned old_slot = 1u - (unsigned)stage.slot;
    const u8 *old = host_save + old_slot * SAVE_SLOT_SIZE;
    GbaSaveSelection predecessor = {base_tag(), sram_get32(old + 4), sram_get32(old + 12),
                                     sram_get32(old + 8), (int)old_slot};
    assert(race_impact_save_write(&predecessor, 1));
    assert(valid_record(host_save + stage.slot * SAVE_SLOT_SIZE, stage.length, stage.tag));
    assert(fixture_select(base_tag()).sequence == 1);
    assert(race_load_game() && !race_nuke_paid_silos && minerals == STOCK_BANK);
    fixture_assert_research();
    puts("checksummed authenticated record with a foreign queued payer rejected");
}

static void fallback(const char *path)
{
    gba_init(); game_init(); fixture_read(path);
    GbaSaveSelection latest = fixture_select(SC64_NUKE_SILO_TRANSPORT_TAG);
    static u8 original[sizeof host_save];
    memcpy(original, host_save, sizeof original);
    u8 *bad = host_save + latest.slot * SAVE_SLOT_SIZE;
    /* A clean checksum does not authorize the wrong inner generation. */
    sram_put32(bad + 16, SAVE_VERSION + 1);
    sram_put32(bad + 8, hash_bytes(bad + 16, latest.length));
    assert(valid_record(bad, latest.length, latest.tag));
    assert(fixture_select(base_tag()).sequence == 1);
    assert(race_load_game() && frame == 0 && !race_nuke_paid_silos && minerals == STOCK_BANK);
    fixture_assert_research();
    /* Keep the real payload and recompute nothing: the base outer tag must
     * not make the same receipt (or its appended actor IDs) meaningful. */
    memcpy(host_save, original, sizeof original);
    sram_put32(bad, base_tag());
    assert(valid_record(bad, latest.length, base_tag()));
    assert(fixture_select(base_tag()).sequence == 1);
    assert(race_load_game() && frame == 0 && !race_nuke_paid_silos && minerals == STOCK_BANK);
    /* If both slots are invalid, a failed load must preserve live state. */
    memset(host_save + (1 - latest.slot) * SAVE_SLOT_SIZE, 0xff, SAVE_SLOT_SIZE);
    frame = 7; minerals = 5;
    assert(!race_load_game() && frame == 7 && minerals == 5 && !race_nuke_paid_silos);
    puts("checked semantic fallback from checksum-valid wrong inner and old-tag payload");
}

static void legacy(const char *path)
{
    gba_init(); game_init(); fixture_read(path);
    RaceLoadPlan plan;
    GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan, &selected) &&
           selected.tag != SC64_NUKE_SILO_TRANSPORT_TAG);
    assert(race_load_game() && !race_nuke_paid_silos && !race_nuke_active_flights);
    printf("loaded existing predecessor %08x (%u bytes) at frame %d\n",
           selected.tag, selected.length, frame);
}

#ifdef SC64_TERRAN_LATE_RUNTIME
static void campaign_write(const char *path)
{
    gba_init(); game_init();
    race_mode = 0; race_selected_mission = SC64_TERRAN_LATE_GLOBAL_T8;
    init_state();
    int silo = -1, command = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (race_nuke_raw_building(i, 108) && b[i].owner == NEUTRAL) silo = i;
        if (race_nuke_raw_building(i, 106) && b[i].owner == SIDE && race_late_airborne(i))
            command = i;
    }
    assert(silo >= 0 && command >= 0 && race_late_active());
    int x = b[silo].x - 48, y = b[silo].y - 8;
    b[command].x = x; b[command].y = y;
    assert(race_late_flight_order(command, x, y, 1));
    game_update(0, 0);
    assert(b[silo].owner == SIDE && b[silo].raw_owner == b[command].raw_owner);
    fixture_build(SIDE, 109, x - 96, y);
    minerals = gas = 500;
    assert(race_nuke_arm(silo, SIDE) == 4 && race_nuke_silos[silo].ammo.remaining == 1500);
    assert(race_save_game() && fixture_select(SC64_NUKE_SILO_TRANSPORT_TAG).sequence == 1);
    fixture_write(path);
}

static void campaign_read(const char *path)
{
    gba_init(); game_init(); fixture_read(path);
    assert(race_load_game() && race_late_active() &&
           race_selected_mission == SC64_TERRAN_LATE_GLOBAL_T8);
    int silo = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (race_nuke_raw_building(i, 108) && b[i].owner == SIDE) silo = i;
    assert(silo >= 0 && race_nuke_silo_online(silo, SIDE) &&
           race_nuke_silos[silo].ammo.remaining == 1500 &&
           race_nuke_silos[silo].payer == SIDE && race_nuke_paid_silos == 1 &&
           race_nuke_reserved_supply(SIDE) == 16 && minerals == 300 && gas == 300);
    game_update(0, 0);
    assert(race_nuke_silos[silo].ammo.remaining == 1499);
    puts("T8 campaign landing, source-permitted Silo queue, and campaign clock restored");
}
#endif

int main(int argc, char **argv)
{
    assert(argc == 3);
    if (!strcmp(argv[1], "write")) write_receipts(argv[2]);
    else if (!strcmp(argv[1], "read")) read_receipts(argv[2]);
    else if (!strcmp(argv[1], "fallback")) fallback(argv[2]);
    else if (!strcmp(argv[1], "bad-payer")) bad_payer(argv[2]);
    else if (!strcmp(argv[1], "legacy")) legacy(argv[2]);
#ifdef SC64_TERRAN_LATE_RUNTIME
    else if (!strcmp(argv[1], "campaign-write")) campaign_write(argv[2]);
    else if (!strcmp(argv[1], "campaign-read")) campaign_read(argv[2]);
#endif
    else assert(!"unknown test operation");
    return 0;
}
