/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Z2 is won by its ordinary-input controller; A on the result screen must
 * start Z3 in the same process. Build with the Zerg campaign flags. */
#define main controller_main
#include "race_campaign_run.c"
#undef main
#define PROGRESSION_FACTION SC64_FACTION_ZERG
#define PROGRESSION_MISSION SC64_RACE_MISSION_Z2
#define PROGRESSION_NEXT 14
#include "sc64_campaign_progression.h"

int main(void)
{
    char *args[] = { "controller", "--controller", "1", 0 };
    if (controller_main(3, args)) { fprintf(stderr, "Z2 controller did not win\n"); return 3; }
    return progression_after_win();
}
