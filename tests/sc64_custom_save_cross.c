/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST cross-build save check for custom games. The same source is compiled in
 * an unchanged default-branch checkout (writer) and in this tree (reader):
 *   write FILE   start a Zerg custom game, play 120 ticks, save through the pause
 *                menu, write the SRAM image and print the named state
 *   read FILE    fresh process: SRAM image in, title quick-load, print the state
 * The two FP lines must match, or the reader must refuse the slot and stay on
 * the title screen (printed as LOADED screen=0). */
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <string.h>
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *dst);
extern void guardian_test_restore(const void *src);

static void fingerprint(void)
{
    int units = 0, buildings = 0;
    for (int i = 0; i < MAX_UNITS; i++) units += u[i].alive;
    for (int i = 0; i < MAX_BUILDINGS; i++) buildings += b[i].alive;
    /* The quick-load tap runs one update, so only tick-independent state is compared. */
    printf("FP units=%d buildings=%d supply=%d/%d race_mode=%d human=%d\n",
           units, buildings, supply_used, supply_cap, race_mode, race_human);
}

int main(int argc, char **argv)
{
    static u8 image[32768];
    FILE *f;
    if (argc < 3) return 2;
    if (!strcmp(argv[1], "write")) {
        guardian_test_reset();
        option_map = 0; option_human = R_ZERG; option_bots = 1;
        option_bot_races[0] = R_TER; match_limit = 100000;
        if (!launch_match()) return 3;
        for (int t = 0; t < 120; t++) input(0);
        tap(KEY_START);
        if (screen != SCREEN_PAUSE) return 4;
        tap(KEY_B);
        if ((pause_notice & 0xc0) != UI_NOTICE_SAVED) return 5;
        fingerprint();
        guardian_test_snapshot(image);
        f = fopen(argv[2], "wb");
        if (!f || fwrite(image, 1, sizeof image, f) != sizeof image) return 6;
        fclose(f);
        return 0;
    }
    f = fopen(argv[2], "rb");
    if (!f || fread(image, 1, sizeof image, f) != sizeof image) return 6;
    fclose(f);
    game_init();
    guardian_test_restore(image);
    prior_keys = 0;
    tap(KEY_L);
    printf("LOADED screen=%d\n", screen == SCREEN_PLAY ? 1 : 0);
    if (screen == SCREEN_PLAY) fingerprint();
    return 0;
}
