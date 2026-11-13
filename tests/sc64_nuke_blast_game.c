/* SPDX-License-Identifier: GPL-3.0-or-later */
#define SC64_SIEGE_SPLASH 1
#define SC64_NUKE_BLAST_PROBE 1
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

static int add_unit(int owner, int raw, int x, int y, int hp)
{
    int slot = spawn_unit(owner, sc64_race_unit_from_raw(raw), x, y);
    assert(slot >= 0);
    u[slot].raw_type = raw;
    u[slot].hp = u[slot].maxhp = hp;
    u[slot].armor = u[slot].shield = 0;
    return slot;
}

int main(void)
{
    gba_init();
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    race_mode = 1;
    race_cols = race_rows = 64;
    int friendly = add_unit(SIDE, 0, 200, 100, 500);
    int air = add_unit(ENEMY, 8, 200, 100, 500);
    int middle = add_unit(ENEMY, 0, 280, 100, 500);
    int outer = add_unit(RESCUABLE, 0, 320, 100, 500);
    int outside = add_unit(ENEMY, 0, 333, 100, 500);
    int buried = add_unit(ENEMY, 37, 280, 100, 500);
    int inner_buried = add_unit(ENEMY, 37, 200, 100, 500);
    int excluded = add_unit(SIDE, 0, 200, 100, 500);
    int invincible = add_unit(ENEMY, 0, 200, 100, 500);
    int nexus = spawn_build(ENEMY, B_NEXUS, 200, 100, 0);
    assert(nexus >= 0);
    b[nexus].raw_type = 154;
    assert(b[nexus].hp == 750 && b[nexus].shield == 750);
    u[buried].flags |= 2u;
    u[inner_buried].flags |= 2u;
    invincible_units[invincible] = 1;
    race_nuke_apply_blast(200, 100, SIDE, excluded);
    assert(u[friendly].hp == 250 && u[air].hp < 500);
    assert(u[middle].hp == 375 && u[outer].hp == 438 && u[outside].hp == 500);
    assert(u[buried].hp == 500 && u[inner_buried].hp == 250);
    assert(u[excluded].hp == 500 && u[invincible].hp == 500);
    assert(b[nexus].shield == 0 && b[nexus].hp == 501);
    puts("nuclear impact: ground, air, friends, rings, shields, armor, burrow and immunity");
    return 0;
}
