/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The modal box in the shipped game loop, on the Terran product flags: rows open
 * from real mission logic, the world stands still, only A advances, an outcome
 * waits for its row, and saves never hold or replay a box. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 previous;

static void input(u16 held)
{
    game_update(held, (u16)(held & (u16)~previous));
    previous = held;
}

static u32 world_digest(void)
{
    u32 h = 2166136261u;
#define MIX(v) (h = (h ^ (u32)(v)) * 16777619u)
    MIX(frame); MIX(minerals); MIX(gas); MIX(won); MIX(lost); MIX(cx); MIX(cy);
    MIX(race_campaign.elapsed_frames); MIX(race_campaign.message_ticks);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive) {
            MIX(i); MIX(u[i].type); MIX(u[i].owner); MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp);
            MIX(u[i].order);
        }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive) { MIX(i); MIX(b[i].type); MIX(b[i].owner); MIX(b[i].hp); }
#undef MIX
    return h;
}

static void start_mission(int mission)
{
    race_mode = 0;
    race_selected_mission = mission;
    race_human = sc64_race_maps[mission].owner_races[sc64_race_maps[mission].human_slot];
    init_state();
    previous = 0;
    sc64_mission_text_interactive = 1;
}

static void run_until_box(int limit)
{
    while (limit-- > 0 && !sc64_mission_text_showing(&race_mission_text)) input(0);
    assert(sc64_mission_text_showing(&race_mission_text));
}

static void settle_box(void)
{
    while (race_mission_text.settle) input(0);
}

/* Read everything queued, without any field input in between. */
static void read_queue(void)
{
    while (sc64_mission_text_showing(&race_mission_text)) {
        settle_box();
        input(KEY_A);
        input(0);
    }
}

static void test_box_freezes_the_world(void)
{
    start_mission(SC64_RACE_MISSION_T1);
    run_until_box(100);
    /* T1's opening row: the first trigger that fires with no input. */
    int row = sc64_mission_text_row(&race_mission_text);
    assert(sc64_mission_text_rows[row].trigger == 3 && sc64_mission_text_rows[row].action == 3);
    assert(sc64_mission_text_rows[row].opcode == SC64_MISSION_TEXT_OPCODE_DISPLAY);
    assert(!strcmp(sc64_mission_text_speakers[sc64_mission_text_rows[row].speaker].name, "Advisor"));
    u32 digest = world_digest();
    int opened_at = frame;
    for (int i = 0; i < 300; ++i) input(0);
    /* Field keys reach neither the menus nor the map. */
    input(KEY_START); input(0);
    input(KEY_SELECT); input(0);
    input(KEY_B); input(0);
    input(KEY_L); input(0);
    input(KEY_R); input(0);
    input(KEY_RIGHT); input(KEY_RIGHT); input(0);
    assert(screen == SCREEN_PLAY && !menu_open && !box_state);
    assert(sc64_mission_text_row(&race_mission_text) == row);
    assert(world_digest() == digest && frame == opened_at);
    read_queue();
    assert(world_digest() == digest);
    /* A's release still belongs to the box; the next frame is the world's. */
    assert(sc64_mission_text_gated(&race_mission_text));
    input(0);
    assert(!sc64_mission_text_gated(&race_mission_text));
    assert(frame == opened_at + 1);
    /* The next row comes from the mission clock. */
    run_until_box(600);
    row = sc64_mission_text_row(&race_mission_text);
    assert(sc64_mission_text_rows[row].trigger == 7 && sc64_mission_text_rows[row].action == 0);
    assert(!strcmp(sc64_mission_text_speakers[sc64_mission_text_rows[row].speaker].name, "SCV"));
    assert(sc64_mission_text_speakers[sc64_mission_text_rows[row].speaker].portrait < 0);
}

static void test_outcome_waits_for_its_row(void)
{
    start_mission(SC64_RACE_MISSION_T1);
    run_until_box(100);
    read_queue();
    input(0);
    /* A mission can end on the tick that posts its last row. */
    race_mission_text_post_source(4, 3);
    lost = 1;
    input(0);
    assert(screen == SCREEN_PLAY && sc64_mission_text_showing(&race_mission_text));
    read_queue(); /* Its last frame is A's release, which hands over to the result. */
    assert(screen == SCREEN_RESULT && lost);
    assert(!sc64_mission_text_gated(&race_mission_text));
    /* A held dismissal cannot skip the result screen. */
    input(0);
    assert(screen == SCREEN_RESULT);
}

static void test_saves_hold_no_box(void)
{
    start_mission(SC64_RACE_MISSION_T1);
    run_until_box(100);
    read_queue();
    input(0);
    run_until_box(600);
    read_queue();
    input(0);
    for (int i = 0; i < 30; ++i) input(0);
    assert(!sc64_mission_text_gated(&race_mission_text));
    int saved_at = frame;
    u32 digest = world_digest();
    save_game();
    for (int i = 0; i < 90; ++i) input(0);
    assert(load_game());
    assert(frame == saved_at && world_digest() == digest);
    assert(!sc64_mission_text_gated(&race_mission_text));
    /* The latches re-read the world, so a line already read does not repeat. */
    for (int i = 0; i < 600; ++i) {
        input(0);
        assert(!sc64_mission_text_gated(&race_mission_text));
    }
}

/* T2's model re-runs a one-shot row while its switch re-arms. */
static void test_one_shot_row_opens_once(void)
{
    start_mission(SC64_RACE_MISSION_T2);
    run_until_box(100);
    read_queue();
    input(0);
    for (int i = 0; i < 50; ++i) bw_message(0, 12, 0);
    assert(race_mission_text.count == 1);
    read_queue();
    input(0);
    bw_message(0, 12, 0);
    assert(!sc64_mission_text_showing(&race_mission_text));
    /* The ending's five rows are distinct. */
    for (int action = 5; action <= 9; ++action) bw_message(0, 14, action);
    assert(race_mission_text.count == 5);
}

/* The table is computed at import time; this build's own palettes must agree. */
static void test_portrait_table_matches_the_live_palette(void)
{
    for (int i = 0; i < 224; ++i) {
        u16 from = sc64_briefing_palette[i];
        int r = from & 31, g = (from >> 5) & 31, b_ = (from >> 10) & 31;
        unsigned best = 0xFFFFFFFFu, got = 0xFFFFFFFFu;
        for (int j = 0; j < 224; ++j) {
            u16 to = sc64_palette[32 + j];
            int dr = r - (to & 31), dg = g - ((to >> 5) & 31), db = b_ - ((to >> 10) & 31);
            unsigned d = (unsigned)(dr * dr * 3 + dg * dg * 4 + db * db * 2);
            if (d < best) best = d;
            if (32 + j == sc64_mission_text_portrait_lut[i]) got = d;
        }
        assert(got == best);
    }
}

static void test_presented_rows_cover_every_mission(void)
{
    /* The bank is whole and its speakers resolve; Z7 alone has no rows. */
    assert(SC64_MISSION_TEXT_MISSION_COUNT == 30);
    assert(sc64_mission_text_first[30] == SC64_MISSION_TEXT_ROW_COUNT);
    for (int mission = 0; mission < 30; ++mission) {
        unsigned first = sc64_mission_text_first[mission];
        unsigned last = sc64_mission_text_first[mission + 1];
        assert((first == last) == (mission == 18));
        for (unsigned i = first; i < last; ++i) {
            const Sc64MissionTextRow *row = &sc64_mission_text_rows[i];
            const char *text = sc64_mission_text_pool + row->text;
            unsigned page = 0, rows = 1;
            assert(row->text < SC64_MISSION_TEXT_POOL_BYTES);
            assert(row->speaker < SC64_MISSION_TEXT_SPEAKER_COUNT);
            assert(row->opcode == SC64_MISSION_TEXT_OPCODE_TRANSMISSION ||
                   row->opcode == SC64_MISSION_TEXT_OPCODE_DISPLAY);
            /* Every page fits the box's text column. */
            for (const char *p = text; *p; ++p) {
                if (*p == '\f') { page = 0; rows = 1; continue; }
                if (*p == '\n') { ++rows; assert(rows <= 5); }
                ++page;
                assert(page < 256);
            }
        }
    }
}

int main(void)
{
    game_init();
    test_presented_rows_cover_every_mission();
    test_portrait_table_matches_the_live_palette();
    test_box_freezes_the_world();
    test_outcome_waits_for_its_row();
    test_saves_hold_no_box();
    test_one_shot_row_opens_once();
    puts("mission text game: ok");
    return 0;
}
