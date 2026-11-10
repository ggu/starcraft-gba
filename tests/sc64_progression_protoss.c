/* SPDX-License-Identifier: GPL-3.0-or-later */
/* P5 is won by its ordinary-input controller; A on the result screen must
 * start P6 in the same process. Build with the Protoss campaign flags. */
#define main controller_main
#include "sc64_p5_campaign_run.c"
#undef main
#define PROGRESSION_FACTION SC64_FACTION_PROTOSS
#define PROGRESSION_MISSION SC64_RACE_MISSION_P5
#define PROGRESSION_NEXT 25
#include "sc64_campaign_progression.h"

int main(void)
{
    char *args[] = { "controller", "--limit", "180000", "--attempt-tag", "progression", 0 };
    if (controller_main(5, args)) { fprintf(stderr, "P5 controller did not win\n"); return 3; }
    return progression_after_win();
}
