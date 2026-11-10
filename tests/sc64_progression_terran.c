/* SPDX-License-Identifier: GPL-3.0-or-later */
/* T4 is won by its ordinary-input controller; A on the result screen must
 * start T5 in the same process. Build with the full Terran campaign flags. */
#define main controller_main
#include "sc64_t4_campaign_run.c"
#undef main
#define PROGRESSION_FACTION SC64_FACTION_TERRAN
#define PROGRESSION_MISSION SC64_T4_GLOBAL_ID
#define PROGRESSION_NEXT SC64_T5_GLOBAL_ID
#include "sc64_campaign_progression.h"

int main(void)
{
    char *args[] = { "controller", "--limit", "30000", "--trace", ".local/sc64/terran-progression.json", 0 };
    if (controller_main(5, args)) { fprintf(stderr, "T4 controller did not win\n"); return 3; }
    return progression_after_win();
}
