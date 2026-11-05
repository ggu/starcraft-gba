/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Modal text boxes on the real engine, in the Terran product. The mission is
 * launched from the ordinary menu and every key goes through game_update(). */
#define SC64_TEXT_BOX_HOST_MODAL
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_TEXT_BOXES
#error "Compile this fixture with the Terran product flags"
#endif

volatile u16 demo_keys;
static unsigned held_keys;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    game_render();
    held_keys = held;
}

static void press(unsigned key) { input(key); input(0); }

static void launch_t1(void)
{
    game_init();
    held_keys = 0;
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_TERRAN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == SC64_RACE_MISSION_T1);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && race_selected_mission == SC64_RACE_MISSION_T1);
    assert(sc64_text_box_count() == 0);
}

static u32 world_digest(void)
{
    u32 h = 2166136261u;
#define MIX(v) do { u32 x_ = (u32)(v); for (int k_ = 0; k_ < 4; ++k_) { h ^= (x_ >> (8 * k_)) & 255u; h *= 16777619u; } } while (0)
    for (int i = 0; i < MAX_UNITS; ++i) {
        MIX(u[i].alive);
        if (!u[i].alive) continue;
        MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp); MIX(u[i].type); MIX(u[i].owner); MIX(u[i].order);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        MIX(b[i].alive);
        if (b[i].alive) { MIX(b[i].hp); MIX(b[i].queue_progress); }
    }
    MIX(frame); MIX(minerals); MIX(gas); MIX(race_campaign.elapsed_frames);
    MIX(race_campaign.objective_flags);
#undef MIX
    return h;
}

static void run_to_box(int limit)
{
    while (!sc64_text_box_count()) {
        assert(limit-- > 0);
        input(0);
    }
}

static unsigned gold_on_panel_top(void)
{
    unsigned n = 0;
    for (int x = SC64_TEXT_BOX_PANEL_X; x < SC64_TEXT_BOX_PANEL_X + SC64_TEXT_BOX_PANEL_W; ++x)
        n += gfx_test_pixel(x, SC64_TEXT_BOX_PANEL_Y) == C_GOLD;
    return n;
}

/* Every generated row must be drawable within its bounds. */
static void check_table(void)
{
    unsigned rows = 0;
    for (unsigned m = 0; m < SC64_TEXT_BOX_MISSIONS; ++m) {
        const Sc64TextBoxMission *mission = &sc64_text_box_missions[m];
        assert(mission->row_count == 0 || mission->rows);
        assert(mission->row_count <= 0xffffu);
        for (unsigned i = 0; i < mission->row_count; ++i) {
            const Sc64TextBoxRow *row = &mission->rows[i];
            ++rows;
            assert(row->head < SC64_TEXT_BOX_HEAD_COUNT);
            assert(row->speaker < sizeof(sc64_text_box_speakers) / sizeof(sc64_text_box_speakers[0]));
            assert(row->page_count >= 1 && row->page_count <= 9);
            assert(sc64_font_width(sc64_text_box_speakers[row->speaker]) <= SC64_TEXT_BOX_TEXT_WIDTH);
            for (unsigned p = 0; p < row->page_count; ++p) {
                const char *page = sc64_text_box_pages[row->first_page + p];
                unsigned lines = 1, length = 0;
                char line[128];
                unsigned used = 0;
                for (const char *c = page;; ++c) {
                    if (*c == '\n' || !*c) {
                        line[used] = 0;
                        assert(sc64_font_width(line) <= SC64_TEXT_BOX_TEXT_WIDTH);
                        used = 0;
                        if (!*c) break;
                        ++lines;
                    } else {
                        assert(used + 1 < sizeof(line));
                        line[used++] = *c;
                    }
                    ++length;
                }
                assert(lines <= SC64_TEXT_BOX_TEXT_ROWS);
                assert(length < SC64_FONT_TEXT_LIMIT);
            }
        }
    }
    assert(rows > 300);
}

int main(int argc, char **argv)
{
    const char *shot = argc > 1 ? argv[1] : 0;
    u32 digest;
    int box_frame, opening_box_frame;
    unsigned first_row;

    gba_init();
    check_table();
    launch_t1();

    /* T1 trigger 3: a DisplayText box from the Advisor, at one source second. */
    run_to_box(120);
    box_frame = opening_box_frame = frame;
    assert(box_frame >= 30 && box_frame < 64);
    first_row = sc64_text_box_front();
    assert(race_text_box_front_row() && race_text_box_front_row()->trigger == 3 &&
           race_text_box_front_row()->action == 3);

    /* The mission stops, and no other key reaches the battlefield. */
    digest = world_digest();
    for (int i = 0; i < 120; ++i) input(0);
    assert(frame == box_frame && world_digest() == digest);
    input(KEY_B); input(0); input(KEY_L); input(0); input(KEY_R); input(0);
    input(KEY_SELECT); input(0); input(KEY_LEFT); input(KEY_UP); input(0);
    assert(frame == box_frame && world_digest() == digest);
    assert(screen == SCREEN_PLAY && !menu_open && sc64_text_box_count() == 1);

    /* The box covers the map and stays out of the cached HUD bands. */
    game_render();
    assert(gold_on_panel_top() == SC64_TEXT_BOX_PANEL_W);
    if (shot) gfx_dump_ppm(shot);

    /* A press only arms it; the world is still stopped until the release. A long
     * row advances a page at a time, then closes. */
    for (unsigned page = 0; page < race_text_box_front_row()->page_count; ++page) {
        assert(sc64_text_box_page() == page);
        input(KEY_A);
        assert(sc64_text_box_count() == 1 && frame == box_frame);
        input(0);
        if (sc64_text_box_count()) assert(sc64_text_box_page() == page + 1u);
        else break;
    }
    assert(sc64_text_box_count() == 0 && frame == box_frame && world_digest() == digest);

    /* The box leaves no pixels behind, on either page. */
    input(0);
    assert(gold_on_panel_top() < 40u);
    input(0);
    assert(gold_on_panel_top() < 40u);
    assert(frame == box_frame + 2);

    /* T1 trigger 7: the next box, and Start dismisses it without pausing. */
    run_to_box(300);
    assert(sc64_text_box_front() != first_row);
    assert(race_text_box_front_row()->trigger == 7);
    box_frame = frame;
    input(KEY_START);
    assert(screen == SCREEN_PLAY && sc64_text_box_count() == 1);
    input(0);
    assert(screen == SCREEN_PLAY && sc64_text_box_count() == 0 && frame == box_frame);

    /* A pause-menu save, then a load in a fresh game: no line repeats, since
     * the once-only bits are saved, and the world continues identically. */
    for (int i = 0; i < 30; ++i) input(0);
    assert(sc64_text_box_count() == 0);
    press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    digest = world_digest();
    box_frame = frame;
    press(KEY_B);
    assert(screen == SCREEN_PAUSE);
    press(KEY_A);
    assert(screen == SCREEN_PLAY);
    for (int i = 0; i < 200; ++i) { input(0); assert(!sc64_text_box_count()); }
    {
        u32 later = world_digest();
        int later_frame = frame;
        launch_t1();
        assert(frame < box_frame);
        assert(load_game());
        assert(frame == box_frame && world_digest() == digest);
        assert(sc64_text_box_count() == 0);
        while (frame < later_frame) { input(0); assert(!sc64_text_box_count()); }
        assert(world_digest() == later);
    }

    /* A new game clears a box, even one left open by a host fixture. */
    sc64_text_box_push(0);
    assert(sc64_text_box_count() == 1);
    assert(load_game());
    assert(sc64_text_box_count() == 0);
    sc64_text_box_push(0);
    launch_t1();
    assert(sc64_text_box_count() == 0);

    /* Direct saves must refuse an unserialized box and preserve the earlier
     * acknowledged pause-menu save. */
    run_to_box(120);
    assert(frame == opening_box_frame);
    assert(!save_game());
    assert(sc64_text_box_count() == 1 && frame == opening_box_frame);
    assert(load_game());
    assert(sc64_text_box_count() == 0 && frame == box_frame);

    /* A skirmish posts nothing. */
    {
        int saved_mode = race_mode;
        race_mode = 1;
        sc64_text_box_reset();
        race_text_box_post_row(3, 3);
        assert(sc64_text_box_count() == 0);
        race_mode = saved_mode;
    }

    puts("text boxes on the engine OK");
    return 0;
}
