/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Normal-menu canonical race-skirmish SRAM resume and deterministic continuation.
 * Build this source once for each canonical Terran, Z6 and P5 runtime. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/game.c"

#if !defined(SC64_UNIFIED) || !defined(SC64_IMPORTED_SKIRMISH) || !defined(SC64_CAMPAIGN_RACE)
#error "Build the resume fixture with a canonical race profile"
#endif

enum { ECONOMY_TICKS = 1800, CONTINUE_TICKS = 900 };

typedef struct {
    uint32_t hash;
    int frame, minerals, gas;
    int bank_minerals[5], bank_gas[5];
    int active_minerals, remaining_minerals, units, buildings;
} WorldMark;

static u16 previous;
static void input(u16 held)
{
    game_update(held, held & (u16)~previous);
    previous = held;
}
static void tap(u16 key) { input(key); input(0); }
static void neutral_ticks(int count)
{
    for (int i = 0; i < count; ++i) input(0);
}
static void to_row(int row)
{
    for (int step = 0; race_front.row != row && step < 16; ++step)
        tap(race_front.row < row ? KEY_DOWN : KEY_UP);
    assert(race_front.row == row);
}
static void set_field(int *field, int value)
{
    for (int i = 0; *field != value && i < 3; ++i) tap(KEY_RIGHT);
    assert(*field == value);
}
static int bot_race(int human, int slot) { return (human + 1 + slot) % 3; }

static void launch(int map, int human, int bots, int forced_bot_race)
{
    game_init(); previous = 0;
    tap(KEY_DOWN); tap(KEY_A); /* HOME -> Custom Game -> Skirmish */
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_SKIRMISH);
    to_row(0); set_field(&race_front.map, map);
    to_row(1); set_field(&race_front.race, human);
    to_row(2); set_field(&race_front.bots, bots);
    for (int i = 0; i < bots; ++i) {
        to_row(3 + i);
        set_field(&race_front.bot_races[i], forced_bot_race >= 0 ? forced_bot_race : bot_race(human, i));
    }
    to_row(3 + bots); tap(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode);
    assert(race_skirmish_map == map && race_human == human && race_bots == bots);
    for (int i = 0; i < bots; ++i)
        assert(race_bot_races[i] == (forced_bot_race >= 0 ? forced_bot_race : bot_race(human, i)));
}

static uint32_t hash_word(uint32_t h, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) { h ^= (uint8_t)v; h *= 16777619u; v >>= 8; }
    return h;
}
static WorldMark mark_world(void)
{
    WorldMark w = {0};
    uint32_t h = 2166136261u;
#define MIX(v) (h = hash_word(h, (uint32_t)(v)))
    w.frame = frame; w.minerals = minerals; w.gas = gas;
    MIX(frame); MIX(seed); MIX(minerals); MIX(gas); MIX(supply_used); MIX(supply_cap);
    MIX(train_type); MIX(weapons_level); MIX(armor_level);
    MIX(weapons_research); MIX(armor_research);
    MIX(geyser_count);
    for (int i = 0; i < geyser_count; ++i) {
        MIX(geysers[i].x); MIX(geysers[i].y);
    }
    /* Compare semantic explored cells, independent of profile bit packing.
       Current visibility is reconstructed rather than persisted. */
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        MIX(SC64_GRID_GET(explored, cell));
    for (int p = 0; p < 5; ++p) {
        w.bank_minerals[p] = race_owner_minerals(p); w.bank_gas[p] = race_owner_gas(p);
        MIX(race_player[p]); MIX(w.bank_minerals[p]); MIX(w.bank_gas[p]);
    }
    for (int i = 0; i < MAX_MINERALS; ++i) {
        MIX(m[i].x); MIX(m[i].y); MIX(m[i].amount);
        w.active_minerals += m[i].amount > 0;
        w.remaining_minerals += m[i].amount;
    }
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        MIX(q->alive);
        if (!q->alive) continue;
        MIX(q->owner); MIX(q->type); MIX(q->x); MIX(q->y);
        MIX(q->hp); MIX(q->maxhp); MIX(q->order); MIX(q->tx); MIX(q->ty); MIX(q->carry);
        MIX(q->build); MIX(q->work_timer); MIX(q->target); MIX(q->path_x); MIX(q->path_y);
        MIX(q->cd); MIX(q->weapon); MIX(q->armor); MIX(q->path_timer); MIX(q->path_retry);
#ifdef SC64_RACES
        MIX(q->shield); MIX(q->raw_type); MIX(q->raw_owner); MIX(q->flags);
        MIX(q->shield_remainder);
#endif
        w.units += q->alive != 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        MIX(q->alive);
        if (!q->alive) continue;
        MIX(q->owner); MIX(q->type); MIX(q->x); MIX(q->y);
        MIX(q->hp); MIX(q->maxhp); MIX(q->under_construction); MIX(q->build_progress);
        MIX(q->level); MIX(q->queue_count); MIX(q->queue_progress);
        for (int j = 0; j < q->queue_count && j < 5; ++j)
            MIX(q->queue[(q->queue_head + j) % 5]);
#ifdef SC64_RACES
        MIX(q->shield); MIX(q->raw_type); MIX(q->raw_owner); MIX(q->flags);
        MIX(q->larva); MIX(q->larva_timer); MIX(q->shield_remainder);
#endif
        w.buildings += q->alive != 0;
    }
    w.hash = h;
#undef MIX
    return w;
}
static void same_world(WorldMark a, WorldMark b)
{
    assert(a.hash == b.hash && a.frame == b.frame);
    assert(a.minerals == b.minerals && a.gas == b.gas);
    assert(a.active_minerals == b.active_minerals);
    assert(a.remaining_minerals == b.remaining_minerals);
    assert(a.units == b.units && a.buildings == b.buildings);
    for (int p = 0; p < 5; ++p) {
        assert(a.bank_minerals[p] == b.bank_minerals[p]);
        assert(a.bank_gas[p] == b.bank_gas[p]);
    }
}

static void verify_resume(int map, int human, int bots, int forced_bot_race)
{
    launch(map, human, bots, forced_bot_race);
    int expected_bot_races[3];
    for (int i = 0; i < 3; ++i) expected_bot_races[i] = race_bot_races[i];
    int bank_before = race_owner_minerals(SIDE);
    int resource_before = 0;
    for (int i = 0; i < MAX_MINERALS; ++i) resource_before += m[i].amount;
    neutral_ticks(ECONOMY_TICKS);
    if (!(frame >= ECONOMY_TICKS && race_owner_minerals(SIDE) > bank_before))
        fprintf(stderr, "economy map%d human%d bots%d frame%d bank%d->%d minerals%d\n",
                map, human, bots, frame, bank_before, race_owner_minerals(SIDE), minerals);
    assert(frame >= ECONOMY_TICKS && race_owner_minerals(SIDE) > bank_before);
    int resource_after = 0;
    for (int i = 0; i < MAX_MINERALS; ++i) resource_after += m[i].amount;
    assert(resource_after < resource_before);

    /* Save through ordinary pause UI after worker mining and bot economy run. */
    tap(KEY_START); assert(screen == SCREEN_PAUSE);
    input(KEY_B); /* SRAM write via the pause menu; no play tick after save. */
    WorldMark saved = mark_world();

    /* Unpause the saved process and advance it to form the reference path.
       The release edge is the first play tick; loading from HOME returns to
       play directly, so the resumed path starts with its first neutral tick. */
    input(KEY_A);
    input(0);
    neutral_ticks(CONTINUE_TICKS - 1);
    WorldMark reference = mark_world();
    assert(reference.frame == saved.frame + CONTINUE_TICKS);
    assert(reference.active_minerals <= saved.active_minerals);
    assert(reference.remaining_minerals < saved.remaining_minerals);
    assert(reference.bank_minerals[SIDE] > saved.bank_minerals[SIDE]);

    /* Reinitialize actors and the frontend while retaining host SRAM. This is
       an in-process reload, not a fresh emulator/process SRAM qualification. */
    game_init(); previous = 0;
    input(KEY_L); /* load action returns directly to play without a tick */
    assert(screen == SCREEN_PLAY && race_mode);
    assert(race_skirmish_map == map && race_human == human && race_bots == bots);
    for (int i = 0; i < 3; ++i) assert(race_bot_races[i] == expected_bot_races[i]);
    WorldMark restored = mark_world();
    same_world(saved, restored);

    neutral_ticks(CONTINUE_TICKS);
    WorldMark resumed = mark_world();
    same_world(reference, resumed);
    printf("resume map=%d human=%d bots=%d bot_race=%d frame=%d->%d ore=%d->%d bank=%d->%d units=%d buildings=%d hash=%08x\n",
           map, human, bots, forced_bot_race, saved.frame, resumed.frame, saved.remaining_minerals,
           resumed.remaining_minerals, saved.bank_minerals[SIDE],
           resumed.bank_minerals[SIDE], resumed.units, resumed.buildings, resumed.hash);
}

int main(void)
{
    gba_init();
    for (int map = 0; map <= SC64_SKIRMISH_IMPORTED_COUNT; ++map)
        for (int human = R_TER; human <= R_PROTOSS; ++human)
            for (int bots = 1; bots <= 3; ++bots)
                verify_resume(map, human, bots, -1);
    /* Three focused full-lobby compositions per map exercise each race's
       economy AI with a different human faction. */
    for (int map = 0; map <= SC64_SKIRMISH_IMPORTED_COUNT; ++map)
        for (int bot = R_TER; bot <= R_PROTOSS; ++bot)
            verify_resume(map, (bot + 1) % 3, 3, bot);
    puts("canonical skirmish save/reload continuation: ok");
    return 0;
}
