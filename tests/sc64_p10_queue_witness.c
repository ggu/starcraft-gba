/* SPDX-License-Identifier: GPL-3.0-or-later
 * P10-feature build of the shared L gesture and queued-supply witness. */
#define SC64_QUEUE_GESTURE_ONLY
#define main sc64_selection_other_main
#include "sc64_selection_run.c"
#undef main
int main(void)
{
    game_init();
    if (queue_cancel_tests()) return fail(1, "P10 shared queue gesture");
    fixture();
    b[0].owner=SIDE; b[0].type=B_GATEWAY; b[0].alive=1;
    b[0].queue[0]=U_DRAGOON; b[0].queue_count=1;
    selected_building=0;
    int prior=race_owner_queued_supply(SIDE);
    if (prior != unit_supply(U_DRAGOON)) return fail(2, "P10 queued supply setup");
    tap(KEY_L);
    if (race_owner_queued_supply(SIDE)!=0 || b[0].queue_count ||
        minerals!=500+unit_cost(U_DRAGOON) || gas!=unit_gas_cost(U_DRAGOON))
        return fail(3, "P10 queued supply cancellation");
    puts("PASS P10 queue gesture and reservation");
    return 0;
}
