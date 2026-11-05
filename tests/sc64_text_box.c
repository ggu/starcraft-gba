/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The text-box queue and its input rule, with no game data. */
#include <assert.h>
#include <stdio.h>
#include "gba.h"
#include "sc64_text_box.h"

/* One frame, as the main loop computes it. */
static unsigned previous;
static int frame_with(unsigned held, unsigned pages)
{
    int advanced = sc64_text_box_input(held, held & ~previous, pages);
    previous = held;
    return advanced;
}

int main(void)
{
    sc64_text_box_reset();
    assert(sc64_text_box_count() == 0);
    assert(!frame_with(KEY_A, 1));
    assert(!frame_with(0, 1));

    /* Pressing arms; only the release advances. */
    assert(sc64_text_box_push(3) == 1);
    assert(sc64_text_box_count() == 1 && sc64_text_box_front() == 3);
    assert(!frame_with(KEY_A, 1));
    assert(sc64_text_box_count() == 1);
    assert(frame_with(0, 1));
    assert(sc64_text_box_count() == 0);

    /* A key already down when the box opens does not acknowledge it. */
    sc64_text_box_push(4);
    previous = KEY_A;
    assert(!frame_with(KEY_A, 1));
    assert(!frame_with(KEY_A | KEY_UP, 1));
    assert(sc64_text_box_count() == 1);
    /* Start can dismiss it while that A stays down, which is what lets a replay
     * insert a box without disturbing its own held keys. */
    assert(!frame_with(KEY_A | KEY_START, 1));
    assert(frame_with(KEY_A, 1));
    assert(sc64_text_box_count() == 0 && previous == KEY_A);

    /* Other keys do nothing. */
    sc64_text_box_push(5);
    previous = 0;
    assert(!frame_with(KEY_B, 1));
    assert(!frame_with(KEY_SELECT, 1));
    assert(!frame_with(KEY_L | KEY_R, 1));
    assert(!frame_with(0, 1));
    assert(sc64_text_box_count() == 1);

    /* Pages advance before the row closes, then the page resets. */
    sc64_text_box_reset();
    sc64_text_box_push(6);
    sc64_text_box_push(7);
    assert(!frame_with(KEY_A, 3));
    assert(frame_with(0, 3) && sc64_text_box_page() == 1 && sc64_text_box_front() == 6);
    assert(!frame_with(KEY_A, 3));
    assert(frame_with(0, 3) && sc64_text_box_page() == 2);
    assert(!frame_with(KEY_A, 3));
    assert(frame_with(0, 3) && sc64_text_box_page() == 0 && sc64_text_box_front() == 7);
    assert(sc64_text_box_count() == 1);
    assert(!frame_with(KEY_A, 1));
    assert(frame_with(0, 1) && sc64_text_box_count() == 0);

    /* Both keys in one frame: one decides, and it alone closes the box. */
    sc64_text_box_push(8);
    assert(!frame_with(KEY_A | KEY_START, 1));
    assert(frame_with(KEY_START, 1));
    assert(sc64_text_box_count() == 0);
    previous = 0;

    /* A row already waiting is not queued twice, but a closed one may repeat. */
    assert(sc64_text_box_push(9) == 1);
    assert(sc64_text_box_push(9) == 0);
    assert(sc64_text_box_count() == 1);
    assert(!frame_with(KEY_A, 1));
    assert(frame_with(0, 1));
    assert(sc64_text_box_push(9) == 1);
    sc64_text_box_reset();

    /* The queue is bounded, in order, and never touches what lies past it. */
    for (unsigned i = 0; i < SC64_TEXT_BOX_QUEUE_CAPACITY; ++i)
        assert(sc64_text_box_push(100u + i) == 1);
    assert(sc64_text_box_push(999) == 0);
    assert(sc64_text_box_count() == SC64_TEXT_BOX_QUEUE_CAPACITY);
    for (unsigned i = 0; i < SC64_TEXT_BOX_QUEUE_CAPACITY; ++i) {
        assert(sc64_text_box_front() == 100u + i);
        sc64_text_box_pop();
    }
    assert(sc64_text_box_count() == 0);
    sc64_text_box_pop();
    assert(sc64_text_box_count() == 0);

    /* A reset clears a half-acknowledged box too. */
    sc64_text_box_push(1);
    previous = 0;
    assert(!frame_with(KEY_A, 1));
    sc64_text_box_reset();
    previous = 0;
    assert(sc64_text_box_count() == 0 && sc64_text_box_page() == 0);
    sc64_text_box_push(2);
    assert(frame_with(0, 1) == 0 && sc64_text_box_count() == 1);

    puts("text box queue and input OK");
    return 0;
}
