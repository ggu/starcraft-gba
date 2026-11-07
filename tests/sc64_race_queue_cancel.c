/* SPDX-License-Identifier: GPL-3.0-or-later
 * Run the ordinary-queue L gesture against the race-engine product vector. */
#define SC64_QUEUE_GESTURE_ONLY
#define main sc64_selection_other_main
#include "sc64_selection_run.c"
#undef main
int main(void)
{
    int result;
    game_init();
    result = queue_cancel_tests();
    if (result) return fail(result, "race queue cancellation");
    puts("PASS race queue cancellation");
    return 0;
}
