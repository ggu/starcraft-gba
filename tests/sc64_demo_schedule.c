#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "common/demo_schedule.h"

enum { PAGE_TICKS = 1000, TEST_TICKS = 50000, EVENT_CAPACITY = 5000 };

typedef struct {
    u32 tick;
    u32 held;
} Event;

static Event events[EVENT_CAPACITY];
static volatile u32 page[DEMO_SCHEDULE_WINDOW_CAPACITY][2];
static u16 whole_held[TEST_TICKS], whole_pressed[TEST_TICKS];
static u16 pages_held[TEST_TICKS], pages_pressed[TEST_TICKS];

static u32 random_state = 0x9137c0deu;
static u32 random32(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

static u32 append_event(u32 count, u32 tick, u16 held)
{
    assert(count < EVENT_CAPACITY);
    events[count].tick = tick;
    events[count].held = held;
    return count + 1;
}

/* Independent oracle: reproduce the original preload loop directly rather
 * than calling the new page helper. */
static void run_whole_stream(u32 count, u32 ticks)
{
    u32 cursor = 0;
    u16 held = 0, previous = 0;
    for (u32 tick = 0; tick < ticks; ++tick) {
        while (cursor < count && cursor < EVENT_CAPACITY &&
               events[cursor].tick == tick) {
            held = (u16)events[cursor].held;
            ++cursor;
        }
        whole_held[tick] = held;
        whole_pressed[tick] = held & (u16)~previous;
        previous = held;
    }
    assert(cursor == count);
}

static void run_windowed(u32 count, u32 ticks)
{
    u32 event_index = 0;
    u16 held = 0, previous = 0;
    for (u32 start = 0; start < ticks; start += PAGE_TICKS) {
        u32 end = start + PAGE_TICKS;
        u32 page_count = 0;
        volatile u32 cursor = 0;
        if (end > ticks) end = ticks;
        while (event_index < count && events[event_index].tick < start)
            assert(!"page refill skipped an event");
        while (event_index < count && events[event_index].tick < end) {
            assert(page_count < DEMO_SCHEDULE_WINDOW_CAPACITY);
            page[page_count][0] = events[event_index].tick;
            page[page_count][1] = events[event_index].held;
            ++event_index;
            ++page_count;
        }
        assert(demo_schedule_window_valid(page, page_count, start, end));
        for (u32 tick = start; tick < end; ++tick) {
            held = demo_schedule_window_held(page, page_count, &cursor, tick,
                                             &held);
            pages_held[tick] = held;
            pages_pressed[tick] = held & (u16)~previous;
            previous = held;
        }
        /* This is the observer contract: all page rows before a refill have
         * been consumed, even when the current page is empty. */
        assert(cursor == page_count);
    }
    assert(event_index == count);
    assert(memcmp(whole_held, pages_held, ticks * sizeof(whole_held[0])) == 0);
    assert(memcmp(whole_pressed, pages_pressed,
                  ticks * sizeof(whole_pressed[0])) == 0);
}

static void compare_streams(u32 count, u32 ticks)
{
    run_whole_stream(count, ticks);
    run_windowed(count, ticks);
}

static void test_boundaries_and_empty_pages(void)
{
    volatile u32 cursor = 0;
    u16 held = 0, previous = 0;
    u32 count = 0;
    count = append_event(count, 999, 1);
    count = append_event(count, 1000, 0);
    count = append_event(count, 1001, 1);
    count = append_event(count, 5000, 0);
    count = append_event(count, 7000, 2);
    assert(demo_schedule_window_valid(page, 0, 0, PAGE_TICKS));
    page[0][0] = 1000;
    page[0][1] = 0;
    assert(!demo_schedule_window_valid(page, 1, 0, PAGE_TICKS));
    assert(demo_schedule_window_valid(page, 1, PAGE_TICKS, 2 * PAGE_TICKS));

    compare_streams(count, 8000);

    cursor = 0;
    held = 0;
    page[0][0] = 1000;
    page[0][1] = 0;
    assert(demo_schedule_window_held(page, 1, &cursor, 1000, &held) == 0);
    assert(cursor == 1);
    previous = 0;
    assert((held & (u16)~previous) == 0); /* release does not create a press */
    page[0][0] = 1001;
    page[0][1] = 1;
    cursor = 0;
    held = 0;
    held = demo_schedule_window_held(page, 1, &cursor, 1001, &held);
    assert((held & (u16)~previous) == 1); /* next-tick re-press is preserved */
}

static void test_page_validation(void)
{
    volatile u32 too_many[DEMO_SCHEDULE_WINDOW_CAPACITY + 1][2];
    for (u32 i = 0; i <= DEMO_SCHEDULE_WINDOW_CAPACITY; ++i) {
        too_many[i][0] = i;
        too_many[i][1] = 0;
    }
    assert(!demo_schedule_window_valid(too_many,
                                      DEMO_SCHEDULE_WINDOW_CAPACITY + 1,
                                      0, 1000));
    too_many[0][0] = 20;
    too_many[1][0] = 19;
    assert(!demo_schedule_window_valid(too_many, 2, 0, 1000));
    too_many[0][0] = 5;
    too_many[0][1] = 1024;
    assert(!demo_schedule_window_valid(too_many, 1, 0, 1000));
    too_many[0][0] = 1000;
    too_many[0][1] = 0;
    assert(!demo_schedule_window_valid(too_many, 1, 0, 1000));

    /* The runtime helper itself is bounded even if a debugger writes a bad
     * count. The final row is a guard that must remain unconsumed. */
    volatile u32 cursor = 0;
    u16 held = 0;
    for (u32 i = 0; i <= DEMO_SCHEDULE_WINDOW_CAPACITY; ++i) {
        too_many[i][0] = 0;
        too_many[i][1] = (i == DEMO_SCHEDULE_WINDOW_CAPACITY) ? 1023u : i;
    }
    held = demo_schedule_window_held(too_many,
                                     DEMO_SCHEDULE_WINDOW_CAPACITY + 1,
                                     &cursor, 0, &held);
    assert(cursor == DEMO_SCHEDULE_WINDOW_CAPACITY);
    assert(held == DEMO_SCHEDULE_WINDOW_CAPACITY - 1);
}

static void test_randomized_page_equivalence(void)
{
    for (u32 trial = 0; trial < 40; ++trial) {
        u32 count = 0;
        u16 held = 0;
        for (u32 tick = 0; tick < TEST_TICKS; ++tick) {
            if ((random32() & 31u) == 0) {
                held = (u16)(random32() & 1023u);
                count = append_event(count, tick, held);
            }
        }
        assert(count > DEMO_SCHEDULE_WINDOW_CAPACITY);
        assert(count < EVENT_CAPACITY);
        compare_streams(count, TEST_TICKS);
    }
}

int main(void)
{
    test_boundaries_and_empty_pages();
    test_page_validation();
    test_randomized_page_equivalence();
    puts("whole and windowed controller streams match held/pressed inputs");
    return 0;
}
