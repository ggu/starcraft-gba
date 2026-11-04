/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Mission text box model: lookup, the queue, paging, the input gate and key
 * history across a box. Uses a synthetic bank, so no game data is needed. */
#include <assert.h>
#include <string.h>
#include "sc64_mission_text.h"

static const char pool[] =
    "first\frow two" "\0"
    "second" "\0"
    "third";
static const Sc64MissionTextRow rows[] = {
    {40, 19, 0, 2, 1, SC64_MISSION_TEXT_OPCODE_TRANSMISSION, 0},
    {41, 0, 14, 2, 5, SC64_MISSION_TEXT_OPCODE_DISPLAY, 1},
    {42, 7, 21, 7, 0, SC64_MISSION_TEXT_OPCODE_TRANSMISSION, 2},
    /* The same string spoken by another unit is another row. */
    {42, 32, 21, 8, 0, SC64_MISSION_TEXT_OPCODE_TRANSMISSION, 3},
};
enum { ROWS = sizeof(rows) / sizeof(rows[0]) };

static int
frame(Sc64MissionText *t, u16 held, u16 *previous, u16 *out)
{
    u16 pressed = (u16)(held & (u16)~*previous);
    int taken = sc64_mission_text_input(t, rows, pool, held, &pressed);
    *previous = held;
    if (out) *out = pressed;
    return taken;
}

static void
settle(Sc64MissionText *t, u16 *previous)
{
    while (t->settle) assert(frame(t, *previous, previous, 0));
}

static void
test_lookup(void)
{
    assert(sc64_mission_text_find_string(rows, 0, ROWS, 7, 40, 19) == 0);
    assert(sc64_mission_text_find_string(rows, 0, ROWS, 7, 40, 20) == -1);
    assert(sc64_mission_text_find_string(rows, 0, ROWS, 9, 41, 123) == 1); /* No speaker. */
    assert(sc64_mission_text_find_string(rows, 0, ROWS, 7, 42, 7) == 2);
    assert(sc64_mission_text_find_string(rows, 0, ROWS, 7, 42, 32) == 3);
    assert(sc64_mission_text_find_string(rows, 0, ROWS, 7, 70000u, 7) == -1);
    assert(sc64_mission_text_find_string(rows, 1, 2, 7, 40, 19) == -1); /* Other mission. */
    assert(sc64_mission_text_find_source(rows, 0, ROWS, 8, 0) == 3);
    assert(sc64_mission_text_find_source(rows, 0, ROWS, 8, 1) == -1);
    assert(!strcmp(sc64_mission_text_next_page(pool), "row two"));
    assert(!sc64_mission_text_next_page(pool + rows[2].text));
}

static void
test_pages_and_queue(void)
{
    Sc64MissionText t;
    u16 previous = 0, out = 0;
    sc64_mission_text_reset(&t);
    assert(!sc64_mission_text_gated(&t));
    assert(!frame(&t, KEY_LEFT, &previous, &out) && out == KEY_LEFT);
    previous = 0;
    sc64_mission_text_post(&t, rows, 0);
    sc64_mission_text_post(&t, rows, 1);
    assert(sc64_mission_text_showing(&t) && sc64_mission_text_row(&t) == 0);
    /* A press during the settle frames does not dismiss an unread line. */
    assert(frame(&t, KEY_A, &previous, 0));
    assert(t.page == rows[0].text);
    assert(frame(&t, 0, &previous, 0));
    settle(&t, &previous);
    /* Only A advances. */
    assert(frame(&t, KEY_START, &previous, 0) && t.page == rows[0].text);
    assert(frame(&t, KEY_B, &previous, 0) && t.page == rows[0].text);
    assert(frame(&t, 0, &previous, 0));
    /* A turns the row's second page, then its next row. */
    assert(frame(&t, KEY_A, &previous, 0));
    assert(!strcmp(pool + t.page, "row two") && sc64_mission_text_row(&t) == 0);
    assert(frame(&t, 0, &previous, 0));
    settle(&t, &previous);
    assert(frame(&t, KEY_A, &previous, 0));
    assert(sc64_mission_text_row(&t) == 1 && t.page == rows[1].text);
    /* Holding A does not run through the queue. */
    while (t.settle) assert(frame(&t, KEY_A, &previous, 0));
    assert(frame(&t, KEY_A, &previous, 0) && sc64_mission_text_row(&t) == 1);
    assert(frame(&t, 0, &previous, 0));
    /* The last row closes on the press and hands back after the release. */
    assert(frame(&t, KEY_A, &previous, 0));
    assert(!sc64_mission_text_showing(&t) && sc64_mission_text_gated(&t));
    assert(sc64_mission_text_row(&t) == -1);
    assert(frame(&t, KEY_A, &previous, 0) && t.state == SC64_MISSION_TEXT_RELEASE);
    assert(frame(&t, 0, &previous, 0) && t.state == SC64_MISSION_TEXT_RESUME);
    assert(!frame(&t, KEY_RIGHT, &previous, &out) && out == KEY_RIGHT);
    assert(!sc64_mission_text_gated(&t));
    assert(t.posted == 2 && !t.dropped);
}

static void
test_overflow_is_bounded(void)
{
    Sc64MissionText t;
    sc64_mission_text_reset(&t);
    for (int i = 0; i < SC64_MISSION_TEXT_QUEUE + 5; ++i)
        sc64_mission_text_post(&t, rows, i % ROWS);
    assert(t.count == SC64_MISSION_TEXT_QUEUE && t.dropped == 5);
    assert(sc64_mission_text_row(&t) == 0);
    /* A row the bank does not present neither opens nor disturbs a box. */
    Sc64MissionText empty;
    sc64_mission_text_reset(&empty);
    sc64_mission_text_post(&empty, rows, -1);
    assert(!sc64_mission_text_gated(&empty) && empty.posted == 1);
    /* The ring wraps without losing order. */
    sc64_mission_text_reset(&t);
    u16 previous = 0;
    for (int round = 0; round < 3 * SC64_MISSION_TEXT_QUEUE; ++round) {
        sc64_mission_text_post(&t, rows, 1);
        sc64_mission_text_post(&t, rows, 2);
        for (int n = 0; n < 2; ++n) {
            assert(sc64_mission_text_row(&t) == 1 + n);
            settle(&t, &previous);
            assert(frame(&t, KEY_A, &previous, 0));
            assert(frame(&t, 0, &previous, 0));
        }
        assert(!frame(&t, 0, &previous, 0));
    }
}

/* The press history a tape replays must survive: a key held across a box is not
 * a new press afterwards, and a fresh press right after the box still counts. */
static void
test_key_history(void)
{
    Sc64MissionText t;
    u16 previous = KEY_A | KEY_LEFT, out = 0;
    sc64_mission_text_reset(&t);
    sc64_mission_text_post(&t, rows, 2);
    assert(frame(&t, KEY_A | KEY_LEFT, &previous, 0));
    assert(t.held_at_open == (KEY_A | KEY_LEFT));
    previous = KEY_LEFT;
    assert(frame(&t, KEY_LEFT, &previous, 0));
    settle(&t, &previous);
    assert(frame(&t, KEY_LEFT | KEY_A, &previous, 0));
    assert(frame(&t, KEY_LEFT, &previous, 0));
    /* A drag held before the box, resumed after it, gives no new A press. */
    assert(!frame(&t, KEY_LEFT | KEY_A, &previous, &out));
    assert(out == 0);

    previous = 0;
    sc64_mission_text_reset(&t);
    sc64_mission_text_post(&t, rows, 2);
    assert(frame(&t, 0, &previous, 0));
    settle(&t, &previous);
    assert(frame(&t, KEY_A, &previous, 0));
    assert(frame(&t, 0, &previous, 0));
    /* A press that was not held before the box stays a press. */
    assert(!frame(&t, KEY_A, &previous, &out));
    assert(out == KEY_A);
}

int
main(void)
{
    test_lookup();
    test_pages_and_queue();
    test_overflow_is_bounded();
    test_key_history();
    return 0;
}
