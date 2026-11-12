/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST fixture for the real P10 custom journal and its public predecessors.
 * Fixtures use the actual paid admission and save routines; HOST images
 * are never installed on a cartridge or used as ARM input. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"

#if !defined(GBA_HOST) || !defined(SC64_P10_RUNTIME)
#error "Use the prepared P10 HOST product vectors"
#endif

enum {
    F_FRAME, F_MINERALS, F_GAS, F_ENEMY_ORE, F_ENEMY_GAS, F_SUPPLY,
    F_LEGACY, F_EGG, F_TIMER, F_HALL, F_LARVA, F_FORGE, F_PROGRESS,
    F_LEVEL, F_COUNT
};

typedef struct {
    unsigned tag;
    int fields[F_COUNT];
} Witness;

static int owner_slot(int owner, int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type)
            return i;
    return -1;
}

static int find_actor(int owner, int type, int raw)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == type && u[i].raw_type == raw)
            return i;
    return -1;
}

static int egg_slot(void)
{
#ifdef SC64_SHARED_EGG_CUSTOM_SERVICE
    return find_actor(ENEMY, SC64_EGG_ACTOR, 36);
#else
    return -1;
#endif
}

static void make_custom_world(void)
{
    memset(host_save, 0xff, sizeof(host_save));
    gba_init();
    game_init();
    race_mode = 1;
    race_human = R_PROTOSS;
    race_bots = 1;
    race_skirmish_map = 0;
    race_bot_races[0] = R_ZERG;
    init_state();
    assert(screen == SCREEN_PLAY && race_player[ENEMY] == R_ZERG);

    int py = spawn_build(SIDE, B_PYLON, 240, 240, 0);
    int forge = spawn_build(SIDE, B_FORGE, 240, 240, 0);
    int hall = owner_slot(ENEMY, B_HATCHERY);
    int legacy = find_actor(ENEMY, U_OVERLORD, 42);
    if (legacy < 0)
        legacy = spawn_unit(ENEMY, U_OVERLORD, 140, 100);
    assert(py >= 0 && forge >= 0 && hall >= 0 && legacy >= 0);
    u[legacy].raw_type = 36; /* Same source value as the actual Egg; different type. */

    minerals = gas = 1000;
    race_minerals[ENEMY] = 100;
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    Sc64ProtossResearchQuote quote;
    assert(sc64_pr_quote_v45(SC64_PR_SHIELDS, 0, &quote));
    assert(sc64_pr_stage_begin_slot(forge, SC64_PR_SHIELDS));
    assert(b[forge].queue_progress == quote.ticks && b[forge].queue_count == 0);
    assert(minerals == 1000 - quote.mineral && gas == 1000 - quote.gas);
    /* Finish the paid shields job through its production clock, leaving one
     * real level in padding and a second, distinct family still in flight. */
    b[forge].queue_progress = 1;
    sc64_pr_stage_tick();
    assert(sc64_experimental_research_levels.level[SIDE][SC64_PR_SHIELDS] == 1);
    assert(sc64_pr_quote_v45(SC64_PR_GROUND_WEAPONS, 0, &quote));
    assert(sc64_pr_stage_begin_slot(forge, SC64_PR_GROUND_WEAPONS));
    assert(b[forge].queue_progress == quote.ticks && b[forge].queue_count == 0);
#endif
#ifdef SC64_SHARED_EGG_CUSTOM_SERVICE
    int larva = b[hall].larva;
    assert(larva > 0 && egg_slot() < 0);
    assert(race_queue_unit(ENEMY, hall, U_DRONE));
    int egg = egg_slot();
    assert(egg >= 0 && egg != legacy && u[egg].target == U_DRONE && u[egg].raw_owner == -1);
    assert(b[hall].larva == larva - 1);
    assert(race_minerals[ENEMY] == 100 - sc64_race_unit_rule(U_DRONE)->minerals);
#endif
}

static Witness observe(unsigned tag)
{
    Witness out = { .tag = tag };
    int *f = out.fields;
    f[F_FRAME] = frame;
    f[F_MINERALS] = minerals;
    f[F_GAS] = gas;
    f[F_ENEMY_ORE] = race_minerals[ENEMY];
    f[F_ENEMY_GAS] = race_gas[ENEMY];
    f[F_SUPPLY] = race_owner_supply(ENEMY);
    f[F_LEGACY] = find_actor(ENEMY, U_OVERLORD, 36);
    f[F_EGG] = egg_slot();
    f[F_TIMER] = f[F_EGG] < 0 ? 0 : u[f[F_EGG]].work_timer;
    f[F_HALL] = owner_slot(ENEMY, B_HATCHERY);
    assert(f[F_LEGACY] >= 0 && f[F_HALL] >= 0);
    f[F_LARVA] = b[f[F_HALL]].larva;
    f[F_FORGE] = owner_slot(SIDE, B_FORGE);
    assert(f[F_FORGE] >= 0);
    f[F_PROGRESS] = b[f[F_FORGE]].queue_progress;
#ifdef SC64_RESEARCH_SRAM_PRODUCT
    f[F_LEVEL] = sc64_experimental_research_levels.level[SIDE][SC64_PR_SHIELDS];
#endif
    return out;
}

static unsigned current_tag(void)
{
#ifdef SC64_P10_RESEARCH_EGG_SRAM_PROBE
    return SC64_RESEARCH_EGG_TRANSPORT_SAVE_VERSION;
#elif defined(SC64_SHARED_EGG_CUSTOM_SERVICE)
    return SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION;
#elif defined(SC64_RESEARCH_SRAM_PRODUCT)
    return SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION;
#else
    return SC64_TRANSPORT_SAVE_VERSION;
#endif
}

static GbaSaveSelection select_save(unsigned expected)
{
    GbaSaveSelection selected;
    RaceLoadPlan plan;
    assert(race_worker_cargo_select(&plan, &selected));
    assert(selected.tag == expected && selected.length <= SAVE_PAYLOAD_MAX);
    return selected;
}

static void write_fixture(const char *path)
{
    make_custom_world();
    Witness want = observe(current_tag());
    assert(race_save_game());
    GbaSaveSelection selected = select_save(want.tag);
    char state[512];
    assert(snprintf(state, sizeof(state), "%s.state", path) < (int)sizeof(state));
    FILE *f = fopen(path, "wbx");
    assert(f);
    assert(fwrite(host_save, 1, sizeof(host_save), f) == sizeof(host_save));
    assert(!fclose(f));
    f = fopen(state, "wx");
    assert(f);
    assert(fprintf(f, "%u\n", want.tag) > 0);
    for (int i = 0; i < F_COUNT; ++i)
        assert(fprintf(f, "%d\n", want.fields[i]) > 0);
    assert(!fclose(f));
    printf("wrote P10 custom tag %08x, %u payload bytes\n", selected.tag, selected.length);
}

#ifdef SC64_P10_RESEARCH_EGG_SRAM_PROBE
static void check_state(const Witness *want)
{
    Witness got = observe(want->tag);
    assert(!memcmp(got.fields, want->fields, sizeof(got.fields)));
    assert(screen == SCREEN_PLAY && race_mode == 1 && race_human == R_PROTOSS &&
           race_bots == 1 && race_player[ENEMY] == R_ZERG);
    if (got.fields[F_EGG] >= 0) {
        const Unit *egg = &u[got.fields[F_EGG]];
        assert(egg->target == U_DRONE && egg->raw_owner == -1 && egg->owner == ENEMY);
        assert(got.fields[F_EGG] != got.fields[F_LEGACY] && egg->work_timer > 0);
    }
    if (got.fields[F_PROGRESS]) {
        Sc64PrPaidReceipt receipt;
        assert(sc64_pr_stage_receipt(got.fields[F_FORGE], &receipt));
        assert(receipt.live && receipt.payer == SIDE &&
               receipt.family == SC64_PR_GROUND_WEAPONS && receipt.target == 1);
        assert(sc64_pr_stage_jobs == 1 && sc64_pr_stage_levels_any);
    } else {
        assert(!sc64_pr_stage_jobs && !sc64_pr_stage_levels_any);
    }
}

static void load_fixture(const char *path, int migrate)
{
    char state[512];
    assert(snprintf(state, sizeof(state), "%s.state", path) < (int)sizeof(state));
    FILE *f = fopen(state, "r");
    Witness want;
    assert(f && fscanf(f, "%u", &want.tag) == 1);
    for (int i = 0; i < F_COUNT; ++i)
        assert(fscanf(f, "%d", &want.fields[i]) == 1);
    assert(!fclose(f));

    gba_init();
    game_init();
    f = fopen(path, "rb");
    assert(f);
    assert(fread(host_save, 1, sizeof(host_save), f) == sizeof(host_save));
    assert(fgetc(f) == EOF && !fclose(f));
    static u8 before[sizeof(host_save)];
    memcpy(before, host_save, sizeof(before));
    assert(race_load_game());
    assert(!memcmp(before, host_save, sizeof(before)));
    check_state(&want);
    GbaSaveSelection old = select_save(want.tag);
    if (migrate) {
        assert(want.tag != current_tag());
        assert(race_save_game());
        GbaSaveSelection replacement = select_save(current_tag());
        assert(old.slot != replacement.slot && old.length == replacement.length &&
               replacement.sequence == old.sequence + 1);
        unsigned offset = (unsigned)old.slot * SAVE_SLOT_SIZE;
        assert(!memcmp(before + offset, host_save + offset, SAVE_SLOT_SIZE));
        assert(race_load_game());
        check_state(&want);
    }
    int egg = want.fields[F_EGG], forge = want.fields[F_FORGE];
    int progress = want.fields[F_PROGRESS];
    game_update(0, 0);
    if (progress)
        assert(b[forge].queue_progress == progress - 1);
    if (egg >= 0) {
        assert(u[egg].work_timer == want.fields[F_TIMER] - 1);
        int ticks = 1;
        while (u[egg].alive && u[egg].type == SC64_EGG_ACTOR &&
               ticks++ <= want.fields[F_TIMER])
            game_update(0, 0);
        assert(u[egg].alive && u[egg].owner == ENEMY && u[egg].type == U_DRONE);
    }
    if (progress) {
        int ore = minerals, vespene = gas;
        Sc64ProtossResearchQuote quote;
        assert(sc64_pr_quote_v45(SC64_PR_GROUND_WEAPONS, 0, &quote));
        assert(sc64_pr_stage_cancel_slot(forge, 0));
        assert(minerals == ore + quote.mineral && gas == vespene + quote.gas);
    }
    printf("fresh P10 load%s: tag %08x, Egg %s, paid research %s\n",
           migrate ? " and migration" : "", want.tag, egg >= 0 ? "born" : "absent",
           progress ? "continued and refunded" : "absent");
}

static void campaign_roundtrip(void)
{
    memset(host_save, 0xff, sizeof(host_save));
    gba_init();
    game_init();
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    game_update(KEY_A, KEY_A);
    game_update(0, 0);
    assert(race_front.page == SC_FRONT_MISSION);
    game_update(KEY_A, KEY_A);
    game_update(0, 0);
    assert(race_front.page == SC_FRONT_BRIEFING);
    game_update(KEY_START, KEY_START);
    assert(screen == SCREEN_PLAY && race_mode == 0 && race_human == R_PROTOSS);
    int mission = race_selected_mission, saved_frame = frame;
    assert(!sc64_pr_stage_jobs && !sc64_pr_stage_levels_any);
    assert(race_save_game());
    GbaSaveSelection selected = select_save(SC64_TRANSPORT_SAVE_VERSION);
    assert(sram_get32(host_save + (unsigned)selected.slot * SAVE_SLOT_SIZE + 16) == SAVE_VERSION);
    game_update(0, 0);
    assert(race_load_game() && !race_mode && race_selected_mission == mission);
    assert(frame == saved_frame && !sc64_pr_stage_jobs && !sc64_pr_stage_levels_any);
    puts("P10 campaign still uses the original inner and outer tags");
}

static void rewrite_checksum(u8 *record)
{
    u32 length = sram_get32(record + 4);
    assert(length <= SAVE_PAYLOAD_MAX);
    sram_put32(record + 8, hash_bytes(record + 16, length));
}

static void semantic_fallback(int retag)
{
    make_custom_world();
    Witness older = observe(current_tag());
    assert(race_save_game());
    GbaSaveSelection old = select_save(older.tag);
    minerals -= 23;
    assert(race_save_game());
    GbaSaveSelection newer = select_save(older.tag);
    assert(newer.slot != old.slot && newer.length == old.length);
    u8 *record = host_save + (unsigned)newer.slot * SAVE_SLOT_SIZE;
    if (retag == 1) {
        /* Exact 0x45 research tag/inner pair, but the payload contains an
         * Egg identity. A reader must not infer Egg permission from rows. */
        sram_put32(record, SC64_RESEARCH_JOBS_TRANSPORT_SAVE_VERSION);
        sram_put32(record + 16, SAVE_VERSION);
    } else if (retag == 2) {
        /* The Egg-only pair may not grant research padding or paid jobs. */
        sram_put32(record, SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION);
    } else {
        /* Add an authenticated malformed extra bit after the packed level
         * payload. This remains a valid outer record/checksum. */
        size_t padding = offsetof(RaceSave, buildings) + 40 * SC64_PR_ROW_WIDTH - 1;
        assert(padding < newer.length);
        record[16 + padding] |= 0x80;
    }
    rewrite_checksum(record);
    assert(valid_record(record, newer.length, sram_get32(record)));

    static u8 before[sizeof(host_save)];
    memcpy(before, host_save, sizeof(before));
    assert(race_load_game());
    check_state(&older);
    assert(select_save(older.tag).slot == old.slot);
    assert(!memcmp(before, host_save, sizeof(before)));

    /* Remove the valid fallback, then check failure cannot publish any
     * actors, balances, research or physical SRAM changes. */
    sram_put32(host_save + (unsigned)old.slot * SAVE_SLOT_SIZE, 0);
    memcpy(before, host_save, sizeof(before));
    static Unit units_before[MAX_UNITS];
    static Building buildings_before[MAX_BUILDINGS];
    memcpy(units_before, u, sizeof(u));
    memcpy(buildings_before, b, sizeof(b));
    Sc64ProtossResearchLevels levels = sc64_experimental_research_levels;
    assert(!race_load_game());
    check_state(&older);
    assert(!memcmp(units_before, u, sizeof(u)) && !memcmp(buildings_before, b, sizeof(b)));
    assert(!memcmp(&levels, &sc64_experimental_research_levels, sizeof(levels)));
    assert(!memcmp(before, host_save, sizeof(before)));
}
#endif

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "write")) {
        write_fixture(argv[2]);
        return 0;
    }
#ifdef SC64_P10_RESEARCH_EGG_SRAM_PROBE
    if (argc == 3 && (!strcmp(argv[1], "read") || !strcmp(argv[1], "migrate"))) {
        load_fixture(argv[2], !strcmp(argv[1], "migrate"));
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "campaign")) {
        campaign_roundtrip();
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "reject")) {
        semantic_fallback(0);
        semantic_fallback(1);
        semantic_fallback(2);
        puts("P10 compound semantic fallback and atomic rejection passed");
        return 0;
    }
#endif
    return 2;
}
