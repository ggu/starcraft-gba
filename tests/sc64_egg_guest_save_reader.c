/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Offline inspection of a copy of guest-exported SRAM using the game loader.
 * Never exports or modifies the input image; this is not guest emulation. */
#include "../starcraft/game.c"
#include <stdio.h>
#include <stdlib.h>
extern int sc64_validator_import_sram(const char *path);

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    gba_init(); game_init();
    if (!sc64_validator_import_sram(argv[1]) || !race_load_game()) {
        fputs("guest SRAM rejected by game loader\n", stderr); return 1;
    }
    fprintf(stderr, "loaded version=%08x expected=%08x mode=%d human=%d bots=%d won=%d lost=%d\n",
            (unsigned)race_data.version, (unsigned)SC64_SHARED_EGG_SAVE_VERSION,
            race_mode, race_human, race_bots, won, lost);
    if (race_data.version != SC64_SHARED_EGG_SAVE_VERSION || race_mode != 1 ||
        race_human != R_ZERG || race_bots != 1 || won || lost) return 1;
    int eggs = 0, slot = -1, zerglings = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE) continue;
        if (u[i].type == U_ZERGLING) ++zerglings;
        if (u[i].type == SC64_EGG_ACTOR) {
            if (u[i].target != U_ZERGLING || u[i].work_timer <= 0 ||
                u[i].work_timer > sc64_z7_egg_duration(U_ZERGLING)) return 1;
            ++eggs; slot = i;
        }
    }
    fprintf(stderr, "eggs=%d zerglings=%d supply=%d cap=%d\n", eggs, zerglings, race_owner_supply(SIDE), race_owner_supply_cap(SIDE));
    if (eggs != 1 || zerglings != 0 || race_owner_supply(SIDE) > race_owner_supply_cap(SIDE)) return 1;
    printf("{\"version\":%u,\"frame\":%d,\"egg_slot\":%d,\"egg_timer\":%d,\"egg_type\":%d,\"zergling_type\":%d,\"side\":%d,\"supply\":%d,\"supply_cap\":%d,\"minerals\":%d,\"gas\":%d,\"bots\":%d}\n",
           (unsigned)race_data.version, frame, slot, u[slot].work_timer,
           SC64_EGG_ACTOR, U_ZERGLING, SIDE, race_owner_supply(SIDE), race_owner_supply_cap(SIDE), minerals, gas, race_bots);
    return 0;
}
