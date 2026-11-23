/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored mineral queue and journal scene; no campaign assets required. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

/* Link tests/sc64_guardian_journal_shim.c in place of common/gba.c. */
void guardian_test_snapshot(void *dst);
void guardian_test_corrupt(int slot);
void guardian_test_reset(void);

#ifndef SC64_MINERAL_FIFO
#error "compile this fixture with SC64_MINERAL_FIFO"
#endif

static void reset_scene(void)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    memset(visible, 0xff, sizeof(visible));
    memset(explored, 0xff, sizeof(explored));
    path_grid_valid = 1;
    frame = 0;
    b[0].alive = 1; b[0].owner = SIDE; b[0].type = B_COMMAND;
    b[0].x = 64; b[0].y = 128; b[0].hp = b[0].maxhp = 1000;
    m[0] = (Mineral){ .x = 128, .y = 128, .amount = 1000 };
    supply_cap = 20; train_type = U_MARINE;
    assert(mineral_fifo_enabled);
}

static int miner(int x, int patch)
{
    int slot = spawn_unit(SIDE, U_SCV, x, m[patch].y);
    assert(slot >= 0);
    u[slot].order = ORDER_MINE; u[slot].target = patch;
    u[slot].build = -1; u[slot].work_timer = 0;
    return slot;
}

static void tick(void) { update_orders(); ++frame; }

static void queue_is(int patch, const int *slots, unsigned count)
{
    unsigned link = mineral_fifo.head[patch];
    for (unsigned i = 0; i < count; ++i) {
        assert(link == (unsigned)slots[i] + 1u);
        assert(mineral_fifo.patch[slots[i]] == (unsigned)patch + 1u);
        link = mineral_fifo.next[slots[i]];
    }
    assert(link == 0);
    assert(mineral_fifo.tail[patch] == (count ? (unsigned)slots[count-1] + 1u : 0));
}

/* The lower slot arrives last; serialization must preserve arrival order. */
static void arrival_and_handoff(void)
{
    reset_scene();
    int late = miner(30, 0), first = miner(128, 0), second = miner(128, 0);
    tick();
    u[late].x = 128; u[late].y = 128;
    tick();
    const int expected[] = { first, second, late };
    queue_is(0, expected, 3);
    assert(u[first].work_timer > 0 && !u[second].work_timer && !u[late].work_timer);
    u[first].work_timer = 1;
    tick();
    assert(u[first].carry == 8 && u[second].work_timer == 0);
    tick();
    assert(u[second].work_timer > 0 && !u[late].work_timer);
    const int remaining[] = { second, late };
    queue_is(0, remaining, 2);
}

static void cancellation_and_reuse(void)
{
    reset_scene();
    int first = miner(128, 0), middle = miner(128, 0), last = miner(128, 0);
    tick();
    u[middle].order = ORDER_IDLE;
    tick();
    const int expected[] = { first, last };
    queue_is(0, expected, 2);
    /* Reusing a dead slot must append, never inherit its old position. */
    u[middle].alive = 0;
    int reused = miner(128, 0);
    assert(reused == middle);
    tick();
    const int after_reuse[] = { first, last, reused };
    queue_is(0, after_reuse, 3);

    /* A depleted field removes old membership when workers retarget. */
    m[1] = (Mineral){ .x = 160, .y = 128, .amount = 100 };
    m[0].amount = 0;
    tick();
    assert(u[first].target == 1 && u[last].target == 1 && u[reused].target == 1);
    queue_is(0, 0, 0);
}

#if !defined(SC64_RACES)
static void journal_continuation(void)
{
    guardian_test_reset();
    reset_scene();
    int late = miner(30, 0), first = miner(128, 0), second = miner(128, 0);
    tick(); u[late].x = 128; u[late].y = 128; tick();
    const int expected[] = { first, second, late };
    queue_is(0, expected, 3);
    assert(save_game());
    assert(save_read(&data, sizeof(data), SC64_MINERAL_FIFO_SAVE_VERSION));
    Unit uninterrupted[MAX_UNITS]; Mineral uninterrupted_minerals[MAX_MINERALS];
    for (int i = 0; i < 50; ++i) tick();
    memcpy(uninterrupted, u, sizeof(uninterrupted));
    memcpy(uninterrupted_minerals, m, sizeof(uninterrupted_minerals));
    int uninterrupted_frame = frame;
    /* A fresh runtime must derive all queue state from the saved record. */
    memset(&mineral_fifo, 0, sizeof(mineral_fifo));
    mineral_fifo_enabled = 0;
    assert(load_game());
    assert(mineral_fifo_enabled);
    queue_is(0, expected, 3);
    for (int i = 0; i < 50; ++i) tick();
    assert(frame == uninterrupted_frame);
    assert(!memcmp(uninterrupted, u, sizeof(uninterrupted)));
    assert(!memcmp(uninterrupted_minerals, m, sizeof(uninterrupted_minerals)));
}

static void legacy_parallel_timers(void)
{
    guardian_test_reset();
    reset_scene();
    int a = miner(128, 0), bslot = miner(128, 0);
    u[a].work_timer = 3; u[bslot].work_timer = 5;
    mineral_fifo_enabled = 0;
    assert(save_game());
    mineral_fifo_enabled = 1;
    assert(load_game());
    assert(!mineral_fifo_enabled);
    tick();
    assert(u[a].work_timer == 2 && u[bslot].work_timer == 4);
    assert(save_game());
    assert(save_read(&data, sizeof(data), SAVE_VERSION));
    assert(data.version == SAVE_VERSION);
}

static void malformed_journal(void)
{
    guardian_test_reset();
    reset_scene();
    int first = miner(128, 0), second = miner(128, 0);
    tick();
    assert(save_game());
    GbaSaveSelection older, winner;
    assert(worker_mineral_fifo_select(&older));
    assert(save_read(&data, sizeof(data), SC64_MINERAL_FIFO_SAVE_VERSION));
    /* Both rows claim rank 1: checksum-valid but semantically invalid. */
    data.units[second].carry = data.units[first].carry;
    assert(save_write_selected(&data, sizeof(data), SC64_MINERAL_FIFO_SAVE_VERSION,
                               &older, mineral_fifo_save_tags, 2));
    assert(worker_mineral_fifo_select(&winner));
    assert(winner.slot == older.slot && winner.sequence == older.sequence);
    memset(&mineral_fifo, 0, sizeof(mineral_fifo));
    assert(load_game());
    const int expected[] = { first, second };
    queue_is(0, expected, 2);

    /* A queue marker on a gas order cannot become live cargo. */
    data.units[first].order = ORDER_GAS;
    data.units[first].work_timer = 0;
    data.units[first].carry = MINERAL_CARGO_MAX + 1;
    assert(save_write_selected(&data, sizeof(data), SC64_MINERAL_FIFO_SAVE_VERSION,
                               &older, mineral_fifo_save_tags, 2));
    assert(worker_mineral_fifo_select(&winner));
    assert(winner.slot == older.slot && winner.sequence == older.sequence);

    /* Eliminate the only valid predecessor, preserving the malformed record. */
    guardian_test_corrupt(older.slot);
    unsigned char sram_before[32768], sram_after[32768];
    Unit units_before[MAX_UNITS]; Mineral minerals_before[MAX_MINERALS];
    Sc64MineralFifo fifo_before = mineral_fifo;
    memcpy(units_before, u, sizeof(units_before));
    memcpy(minerals_before, m, sizeof(minerals_before));
    int saved_frame = frame, saved_mode = mineral_fifo_enabled;
    guardian_test_snapshot(sram_before);
    assert(!load_game());
    guardian_test_snapshot(sram_after);
    assert(!memcmp(sram_before, sram_after, sizeof(sram_before)));
    assert(!memcmp(units_before, u, sizeof(units_before)));
    assert(!memcmp(minerals_before, m, sizeof(minerals_before)));
    assert(!memcmp(&fifo_before, &mineral_fifo, sizeof(fifo_before)));
    assert(frame == saved_frame && mineral_fifo_enabled == saved_mode);
}

#endif

int main(void)
{
    arrival_and_handoff();
    cancellation_and_reuse();
#if !defined(SC64_RACES)
    journal_continuation();
    legacy_parallel_timers();
    malformed_journal();
#endif
    puts("Mineral FIFO ordering, cancellation and journal continuation passed.");
    return 0;
}
