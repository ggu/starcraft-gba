/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-button custom Zerg checkpoint. All world changes use game_update. */
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main

static int live_egg(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == SC64_EGG_ACTOR &&
            u[i].target == U_ZERGLING) return i;
    return -1;
}

int main(int argc, char **argv)
{
    if (argc != 2 || !(trace = fopen(argv[1], "w"))) return 2;
    fputs("{\"profile\":\"z7-custom-egg\",\"events\":[\n", trace);
    option_map = 0; option_human = R_ZERG; option_bots = 1;
    option_bot_races[0] = R_TER; match_limit = 30000;
    gba_init();
    if (!launch_match()) goto fail;
    while (screen == SCREEN_PLAY && !won && !lost && host_ticks < match_limit &&
           minerals < sc64_race_building_rule(B_SPAWNING_POOL)->minerals) input(0);
    if (!build_barracks()) goto fail;
    while (screen == SCREEN_PLAY && !won && !lost && host_ticks < match_limit &&
           !(combat_build.state == BUILD_FOUNDATION && combat_build.complete)) input(0);
    if (!(combat_build.state == BUILD_FOUNDATION && combat_build.complete)) goto fail;
    int hall = -1;
    while (screen == SCREEN_PLAY && !won && !lost && host_ticks < match_limit) {
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE && sc64_race_is_zerg_townhall(b[i].type) &&
                building_complete(i) && b[i].larva > 0) { hall = i; break; }
        if (hall >= 0 && minerals >= sc64_race_unit_rule(U_ZERGLING)->minerals) break;
        input(0);
    }
    if (hall < 0 || !select_building(hall)) goto fail;
    for (int n = 0; n < 16 && train_type != U_ZERGLING; ++n) tap(KEY_R);
    if (train_type != U_ZERGLING || live_egg() >= 0) goto fail;
    int before_minerals = minerals, before_larva = b[hall].larva;
    int before_units = count_units(SIDE, U_ZERGLING);
    tap(KEY_A);
    int egg = live_egg();
    if (egg < 0 || b[hall].larva != before_larva - 1 ||
        minerals > before_minerals - sc64_race_unit_rule(U_ZERGLING)->minerals) goto fail;
    trace_record("{\"kind\":\"paid_egg\",\"host_tick\":%u,\"frame\":%d,\"slot\":%d,\"timer\":%d,\"minerals_before\":%d,\"minerals_after\":%d,\"larva_before\":%d,\"larva_after\":%d,\"zerglings_before\":%d}",
                 host_ticks, frame, egg, u[egg].work_timer, before_minerals, minerals,
                 before_larva, b[hall].larva, before_units);
    tap(KEY_START);
    if (screen != SCREEN_PAUSE) goto fail;
    tap(KEY_B);
    if ((pause_notice & 0xc0) != UI_NOTICE_SAVED || live_egg() != egg) goto fail;
    trace_record("{\"kind\":\"save\",\"host_tick\":%u,\"frame\":%d,\"slot\":%d,\"timer\":%d}", host_ticks, frame, egg, u[egg].work_timer);
    tap(KEY_START);
    while (screen == SCREEN_PLAY && !won && !lost && host_ticks < match_limit && live_egg() >= 0) input(0);
    if (live_egg() >= 0 || count_units(SIDE, U_ZERGLING) != before_units + 2) goto fail;
    trace_record("{\"kind\":\"birth\",\"host_tick\":%u,\"frame\":%d,\"zerglings\":%d}", host_ticks, frame, count_units(SIDE, U_ZERGLING));
    fputs("\n]}\n", trace); fclose(trace); return 0;
fail:
    fprintf(stderr, "checkpoint failed tick=%u screen=%d won=%d lost=%d bank=%d\n", host_ticks, screen, won, lost, minerals);
    fputs("\n]}\n", trace); fclose(trace); return 1;
}
