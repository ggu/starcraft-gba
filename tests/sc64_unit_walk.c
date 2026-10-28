/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>

typedef unsigned char u8;
typedef struct { int x, y, path_timer; } Unit;
#define MAX_UNITS 4
static Unit u[MAX_UNITS];
static int pixels;
static void gfx_pixel(int x, int y, u8 color) { (void)x; (void)y; (void)color; ++pixels; }

#include "sc64_unit_walk.h"

static int failures;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

/* move_toward() decrements the path timer once per tick and repaths at 0. */
static void
walk_tick(Unit *q)
{
    if (q->path_timer <= 0) q->path_timer = 24;
    q->path_timer--;
}

int
main(void)
{
    int seen[5] = {0}, phase;
    sc64_walk_reset_all();
    CHECK(sc64_walk_track(&u[0]) == 0);   /* a fresh slot stands */
    CHECK(sc64_walk_track(&u[0]) == 0);

    /* Every tick a unit is moved registers, including the 0 -> 23 repath,
     * and a full path leg visits every pose. */
    for (int i = 0; i < 48; ++i) {
        walk_tick(&u[0]);
        phase = sc64_walk_track(&u[0]);
        CHECK(phase >= 1 && phase <= 4);
        if (phase >= 1 && phase <= 4) seen[phase] = 1;
    }
    CHECK(seen[1] && seen[2] && seen[3] && seen[4]);

    /* One tick without a step stays in stride; a real stop stands. */
    CHECK(sc64_walk_track(&u[0]) != 0);
    CHECK(sc64_walk_track(&u[0]) == 0);
    CHECK(sc64_walk_track(&u[0]) == 0);

    /* A blocked unit is held at path_timer 1 and stands after one tick. */
    u[1].path_timer = 7;
    sc64_walk_track(&u[1]);
    u[1].path_timer = 1;
    sc64_walk_track(&u[1]);
    sc64_walk_track(&u[1]);
    CHECK(sc64_walk_track(&u[1]) == 0);

    /* Resets clear only the stride bits, leaving the low five for facing. */
    SC64_WALK_BITS[2] = 0xff;
    sc64_walk_reset_slot(2);
    CHECK(SC64_WALK_BITS[2] == 0x1f);
    sc64_walk_reset_slot(-1);              /* out of range is ignored */
    sc64_walk_reset_slot(MAX_UNITS);

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
