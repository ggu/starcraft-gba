/* Headless navigation and palette smoke test for the optional Races frontend. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "../starcraft/game.c"

static u16 previous;

static void controller_frame(u16 held)
{
    game_update(held, held & (u16)~previous);
    previous = held;
}

static void press(u16 key)
{
    controller_frame(key);
    controller_frame(0);
}

static int check(int ok, const char *message)
{
    if (!ok) fprintf(stderr, "Races menu: %s\n", message);
    return ok;
}

static int capture(const char *name)
{
    char path[256];
    if (mkdir("build/menupreview", 0777) != 0 && errno != EEXIST) {
        perror("build/menupreview");
        return 0;
    }
    if (screen == SCREEN_TITLE)
        for (int i = 0; i < 80; ++i) game_update(0, 0);
    snprintf(path, sizeof(path), "build/menupreview/%s.ppm", name);
    game_render();
    gfx_dump_ppm(path);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Races menu: cannot write %s\n", path);
        return 0;
    }
    fclose(f);
    return 1;
}

static int same_file(const char *a, const char *b)
{
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    int ok = fa && fb;
    while (ok) {
        unsigned char ba[1024], bb[1024];
        size_t na = fread(ba, 1, sizeof(ba), fa);
        size_t nb = fread(bb, 1, sizeof(bb), fb);
        if (na != nb || memcmp(ba, bb, na)) ok = 0;
        if (na < sizeof(ba) || nb < sizeof(bb)) break;
    }
    if (fa) fclose(fa);
    if (fb) fclose(fb);
    return ok;
}

/* Independent fixture for the StarCraft display palette. Keep the channel
   curve literal here so this test detects a missing or incorrect install. */
static void install_expected_display_palette(void)
{
    static const u8 curve[32] = {
        0,3,5,6,7,9,10,11,12,13,14,15,16,17,18,19,
        20,20,21,22,23,24,24,25,26,27,27,28,29,30,30,31
    };
    u16 palette[224];
    for (int i = 0; i < 224; ++i) {
        u16 c = sc64_palette[32 + i];
        palette[i] = (u16)(curve[c & 31] | (curve[(c >> 5) & 31] << 5) |
                           (curve[(c >> 10) & 31] << 10));
    }
    gfx_set_palette(32, 224, palette);
    static const struct { u8 index, r, g, b; } ui[] = {
        {C_DIM,21,23,25}, {C_EDGE,13,18,21}, {C_GOLD_DARK,20,15,5},
        {C_FOG,5,7,10}, {C_GRASS,8,15,9}, {C_TERRAN,11,21,31},
        {C_ZERG,25,11,22}
    };
    for (unsigned i = 0; i < sizeof(ui) / sizeof(ui[0]); ++i) {
        u16 c = (u16)(ui[i].r | (ui[i].g << 5) | (ui[i].b << 10));
        gfx_set_palette(ui[i].index, 1, &c);
    }
}

static int display_palette_is_active(const char *capture_name)
{
    char expected[96], raw[96], original_path[128], expected_path[128], raw_path[128];
    snprintf(expected, sizeof(expected), "%s_expected", capture_name);
    snprintf(raw, sizeof(raw), "%s_raw", capture_name);
    snprintf(original_path, sizeof(original_path), "build/menupreview/%s.ppm", capture_name);
    snprintf(expected_path, sizeof(expected_path), "build/menupreview/%s.ppm", expected);
    snprintf(raw_path, sizeof(raw_path), "build/menupreview/%s.ppm", raw);
    /* The fixture must differ from the unmodified game palette. */
    gfx_set_palette(32, 224, sc64_palette + 32);
    if (!capture(raw) || same_file(original_path, raw_path)) return 0;
    install_expected_display_palette();
    return capture(expected) && same_file(original_path, expected_path);
}

int main(void)
{
    gba_init();
    game_init();
    if (!capture("home")) return 1;
    if (!check(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME,
               "game_init did not show HOME")) return 1;

    press(KEY_A);
    if (!check(race_front.page == SC_FRONT_CAMPAIGN, "HOME A did not open CAMPAIGN")) return 1;
#ifdef SC64_UNIFIED
    while (race_front.row != 1) press(KEY_RIGHT); /* Zerg campaign row */
#endif
    if (!capture("campaign_zerg")) return 1;
    press(KEY_A);
    if (!check(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_ZERG,
               "default campaign faction is not Zerg")) return 1;
    if (!capture("mission_zerg")) return 1;
    press(KEY_A);
    if (!check(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 0,
               "Z1 did not open its briefing")) return 1;
    if (!capture("briefing_z1")) return 1;

    press(KEY_B);
    if (!check(race_front.page == SC_FRONT_MISSION, "briefing B did not return to missions")) return 1;
    press(KEY_B);
    press(KEY_RIGHT);
    if (!check(race_front.page == SC_FRONT_CAMPAIGN && race_front.faction == SC_FRONT_PROTOSS,
               "campaign Right did not select Protoss")) return 1;
    if (!capture("campaign_protoss")) return 1;
    press(KEY_A);
    press(KEY_DOWN);
    if (!check(race_front.page == SC_FRONT_MISSION && race_front.row == 1,
               "Protoss second mission selection is wrong")) return 1;
    if (!capture("mission_p2")) return 1;
    press(KEY_A);
    if (!check(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 3,
               "Protoss second mission did not open its briefing")) return 1;
    if (!capture("briefing_p2")) return 1;
    press(KEY_A);
    if (!check(screen == SCREEN_PLAY && race_campaign.mission == 3 &&
               race_selected_mission == 3 && race_human == R_PROTOSS && !race_mode,
               "P2 launch did not select the Protoss campaign")) return 1;
    if (!capture("p2_launch")) return 1;
    if (!display_palette_is_active("p2_launch"))
        return check(0, "launch did not retain the transformed world palette");

    /* Exercise title B using a real save made through the pause controls. */
    press(KEY_START);
    press(KEY_B);
    if (!check(screen == SCREEN_PAUSE, "pause B did not save without leaving pause")) return 1;
    game_init();
    previous = 0;
    press(KEY_B);
    if (!check(screen == SCREEN_PLAY && race_campaign.mission == 3 &&
               race_human == R_PROTOSS && !race_mode,
               "title B did not reload the saved P2 campaign")) return 1;
    if (!capture("p2_load")) return 1;
    if (!display_palette_is_active("p2_load"))
        return check(0, "title B load did not retain the transformed world palette");

    /* HOME → CUSTOM GAME; race, opponents, launch. */
    game_init();
    previous = 0;
    press(KEY_DOWN);
    press(KEY_A);
    if (!check(race_front.page == SC_FRONT_SKIRMISH, "HOME Down/A did not open custom game")) return 1;
    if (!capture("custom_game")) return 1;
    while (race_front.race != R_PROTOSS) press(KEY_RIGHT);
    press(KEY_DOWN);
    press(KEY_DOWN);
    if (!check(race_front.page == SC_FRONT_SKIRMISH && race_front.row == 2,
               "custom game row navigation failed")) return 1;
    if (!capture("custom_game_launch")) return 1;
    press(KEY_A);
    if (!check(screen == SCREEN_PLAY && race_mode && race_human == R_PROTOSS && race_bots == 2,
               "custom game launch did not use the selected race and opponents")) return 1;
    puts("Races menu navigation, load, and palette restoration: ok");
    return 0;
}
