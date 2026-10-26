/* T5 compile/runtime check for the generic raw-type facing fallback. */
#include <stdio.h>
#include "../starcraft/game.c"

static unsigned hash_screen(void)
{
    unsigned hash = 2166136261u;
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            hash = (hash ^ gfx_test_pixel(x, y)) * 16777619u;
    return hash;
}

int main(void)
{
    gba_init();
    game_init();
    memset(u, 0, sizeof(u));
    camx = camy = 0;
    u[0].alive = 1;
    u[0].owner = SIDE;
    u[0].type = U_MARINE;
    u[0].raw_type = -1;
    u[0].hp = u[0].maxhp = 40;
    u[0].x = 120;
    u[0].y = 70;
    int raw = sc64_race_raw_unit(U_MARINE);
    if (raw < 0 || !sc64_unit_facing_frames((unsigned short)raw)) return 1;
    gfx_begin(0);
    draw_unit(&u[0]);
    unsigned fallback = hash_screen();
    u[0].raw_type = raw;
    gfx_begin(0);
    draw_unit(&u[0]);
    unsigned explicit_raw = hash_screen();
    if (fallback != explicit_raw) {
        fputs("T5 raw_type=-1 facing fallback differs from explicit raw sprite\n", stderr);
        return 2;
    }
    puts("T5 generic raw-type facing fallback: ok");
    return 0;
}
