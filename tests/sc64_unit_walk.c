/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <string.h>

typedef unsigned char u8;
typedef struct { int x, y; } Unit;
#define MAX_UNITS 4
static Unit u[MAX_UNITS];
static int pixels;
static void gfx_pixel(int x, int y, u8 color) { (void)x; (void)y; (void)color; ++pixels; }

#include "sc64_unit_walk.h"

static int failures;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

int
main(void)
{
    int seen[5] = {0}, phase;
    sc64_walk_reset_all();
    u[0].x = 100; u[0].y = 80;
    CHECK(sc64_walk_track(&u[0]) == 0);   /* first observation only primes */
    CHECK(sc64_walk_track(&u[0]) == 0);   /* standing still */

    /* Horizontal, vertical, and diagonal steps all register as walking and
     * the phase visits every pose. */
    for (int i = 0; i < 24; ++i) {
        u[0].x += i < 8 ? 2 : i < 16 ? 0 : 1;
        u[0].y += i < 8 ? 0 : i < 16 ? -2 : 1;
        phase = sc64_walk_track(&u[0]);
        CHECK(phase >= 1 && phase <= 4);
        if (phase >= 1 && phase <= 4) seen[phase] = 1;
    }
    CHECK(seen[1] && seen[2] && seen[3] && seen[4]);

    /* A slow unit that skips a tick stays in stride... */
    CHECK(sc64_walk_track(&u[0]) != 0);
    /* ...but settles into the standing pose once it really stops. */
    for (int i = 0; i < SC64_WALK_HOLD; ++i) phase = sc64_walk_track(&u[0]);
    CHECK(phase == 0);

    /* A reused slot primes from its new position instead of striding. */
    sc64_walk_reset_slot(0);
    u[0].x = 10; u[0].y = 10;
    CHECK(sc64_walk_track(&u[0]) == 0);
    sc64_walk_reset_slot(-1);              /* out of range is ignored */

    CHECK(SC64_WALK_LIFT(0) == 0 && SC64_WALK_LIFT(1) == 0 && SC64_WALK_LIFT(3) == 0);
    CHECK(SC64_WALK_LIFT(2) == 1 && SC64_WALK_LIFT(4) == 1);

    for (phase = 1; phase <= 4; ++phase) {
        pixels = 0; sc64_walk_legs(50, 50, phase, 1); CHECK(pixels >= 2);
        pixels = 0; sc64_walk_scuttle(50, 50, 4, phase, 1); CHECK(pixels == 4);
        pixels = 0; sc64_walk_treads(50, 50, phase, 1); CHECK(pixels >= 3);
    }

    if (failures) return 1;
    puts("unit walk cycle: ok");
    return 0;
}
