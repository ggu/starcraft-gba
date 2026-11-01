"""Independent full-grid oracle for Z7 sight and incremental visibility."""

import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


C_SOURCE = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
#define GBA_HOT
#define MAP_COLS 64
#define MAP_ROWS 64
#define MAP_CELLS (MAP_COLS * MAP_ROWS)
#define MAX_UNITS 256
#define MAX_BUILDINGS 255
#define SIDE 1
#define U_SCV 41
#define B_COMMAND 200
#define B_HATCHERY 131
#define B_LAIR 132
#define B_HIVE 133
#define B_SPIRE 141
#define B_GREATER_SPIRE 137
#define B_NEXUS 201

enum { U_UNKNOWN = 1, U_ZP_SOURCE = 2 };
typedef struct { int alive, owner, x, y, type, loaded; } Unit;
typedef struct { int alive, owner, x, y, type, under_construction, flags; } Building;
static Unit u[MAX_UNITS];
static Building b[MAX_BUILDINGS];
static u8 visible[MAP_CELLS], explored[MAP_CELLS];
static int race_cols = MAP_COLS, race_rows = MAP_ROWS;
static int z7_active = 1;
static int sc64_cell_col(int cell) { return cell % MAP_COLS; }
static int sc64_cell_row(int cell) { return cell / MAP_COLS; }
static int IS_WORKER(int type) { return type == U_SCV; }
static int race_z7_active(void) { return z7_active; }
static int sc64_race_raw_unit(int type) { return type; }
static int sc64_race_raw_building(int type) { return type; }
static int sc64_race_is_zerg_townhall(int type) {
    return type == B_HATCHERY || type == B_LAIR || type == B_HIVE;
}
static int sc64_zp_actor_sight_source_pixels(int type) {
    return type == U_ZP_SOURCE ? 400 : 0;
}
static int sc64_zp_building_sight_source_pixels(int type) { (void)type; return 0; }
#define SC64_TRANSPORT_LOADED(q) ((q)->loaded)

#if TEST_EXPECT_Z7_SPANS
#ifndef SC64_Z7_SIGHT_SPANS
#error "candidate fixture must enable SC64_Z7_SIGHT_SPANS"
#endif
#else
#ifdef SC64_Z7_SIGHT_SPANS
#error "reference fixture must leave SC64_Z7_SIGHT_SPANS disabled"
#endif
#endif

#include "starcraft/sc64_visibility.h"

#ifdef SC64_Z7_SIGHT_SPANS
static int brute_halfwidth(int radius, int dy) {
    int width = 0;
    while ((width + 1) * (width + 1) + dy * dy <= radius * radius) ++width;
    return width;
}

static int check_z7_lookup(void) {
    int radius_index, dy;
    for (radius_index=0; radius_index<8; ++radius_index) {
        int radius=64+16*radius_index;
        for (dy=0; dy<=radius; ++dy)
            if (sc64_z7_visibility_halfwidth(radius_index,dy) !=
                brute_halfwidth(radius,dy)) {
                fprintf(stderr,"Z7 halfwidth mismatch radius=%d dy=%d\n",radius,dy);
                return 1;
            }
        if (sc64_z7_visibility_halfwidth(radius_index,-1) != 0 ||
            sc64_z7_visibility_halfwidth(radius_index,radius+1) != 0) return 1;
    }
    return sc64_z7_visibility_halfwidth(-1,0) != 0 ||
           sc64_z7_visibility_halfwidth(8,0) != 0;
}
#endif

#ifdef SC64_Z7_VISIBILITY_ROW_CONTEXT
static int check_prepared_rows(void) {
    static const int centers[] = {-17,-1,0,1,15,16,31,1008,1023,1039};
    Sc64Z7VisibilityRowContext ctx;
    int radius, xi, yi, row;
    if (sc64_z7_visibility_prepare_rows(&ctx,0,0,63) ||
        sc64_z7_visibility_prepare_rows(&ctx,0,0,72) ||
        sc64_z7_visibility_prepare_rows(&ctx,0,0,200)) return 1;
    for (radius=64; radius<=176; radius+=16)
        for (xi=0; xi<(int)(sizeof(centers)/sizeof(centers[0])); ++xi)
            for (yi=0; yi<(int)(sizeof(centers)/sizeof(centers[0])); ++yi) {
                int x=centers[xi], y=centers[yi], extent=(radius+23)/16;
                if (!sc64_z7_visibility_prepare_rows(&ctx,x,y,radius)) return 1;
                for (row=-14; row<MAP_ROWS+14; ++row) {
                    int first,last,want_first=1,want_last=0;
                    sc64_z7_visibility_prepared_row_span(&ctx,row,&first,&last);
                    /* Full bounded-row squared-distance oracle: no width
                     * table, label or interval formula from production. */
                    if (row>=0 && row<MAP_ROWS &&
                        row>=y/16-extent && row<=y/16+extent)
                        for (int col=0; col<MAP_COLS; ++col) {
                            int dx=col*16+8-x, dy=row*16+8-y;
                            if (col>=x/16-extent && col<=x/16+extent &&
                                dx*dx+dy*dy<=radius*radius) {
                                if (want_first>want_last) want_first=col;
                                want_last=col;
                            }
                        }
                    if (first!=want_first || last!=want_last) {
                        fprintf(stderr,"prepared row mismatch r=%d xy=%d,%d row=%d got=%d..%d want=%d..%d\n",
                            radius,x,y,row,first,last,want_first,want_last);
                        return 1;
                    }
                }
            }
    return 0;
}
#endif

static unsigned short ref_count[MAP_CELLS];
static u8 ref_visible[MAP_CELLS], ref_explored[MAP_CELLS];
static unsigned short ref_mini_visible[24 * 24], ref_mini_explored[24 * 24];

/* Independent authored test model: full-map point-to-circle tests, no spans. */
static int expected_radius(int raw) {
    switch (raw) {
    case 35: case 36: return 64;
    case 37: case 40: case 47: case 50: return 80;
    case 38: return 96;
    case 39: case 41: case 43: case 149: return 112;
    case 103: case 134: case 135: case 136: case 137: case 138: case 139:
    case 140: case 141: case 142: return 128;
    case 42: case 51: case 131: return 144;
    case 45: case 46: case 62: case 130: case 132: case 143: case 144:
    case 146: return 160;
    case 44: case 133: return 176;
    case 59: case 97: return 64;
    default: return 0;
    }
}

static int owner_unit_radius(int type) {
    if (type == U_ZP_SOURCE) return 200; /* intentionally outside Z7 LUT */
    int radius = z7_active ? expected_radius(type) : 0;
    return radius ? radius : (IS_WORKER(type) ? 56 : 72);
}

static int owner_building_radius(const Building *q) {
    int raw = q->type;
    int radius;
    if (!z7_active) raw = 0;
    else if (q->under_construction) {
        if (q->type == B_LAIR) raw = B_HATCHERY;
        else if (q->type == B_HIVE) raw = B_LAIR;
        else if (q->type == B_GREATER_SPIRE) raw = B_SPIRE;
        else return 64; /* authored unfinished, non-morphing building rule */
    }
    radius = expected_radius(raw);
    if (radius) return radius;
    return (q->type == B_COMMAND || sc64_race_is_zerg_townhall(q->type)) ? 112 : 64;
}

static int unit_explore_radius(int type) {
    if (type == U_ZP_SOURCE) return 12;
    int radius = z7_active ? expected_radius(type) : 0;
    return radius ? radius / 16 : (IS_WORKER(type) ? 4 : 5);
}

static int building_explore_radius(const Building *q) {
    int raw = q->type;
    int radius;
    if (!z7_active) raw = 0;
    else if (q->under_construction) {
        if (q->type == B_LAIR) raw = B_HATCHERY;
        else if (q->type == B_HIVE) raw = B_LAIR;
        else if (q->type == B_GREATER_SPIRE) raw = B_SPIRE;
        else return 4;
    }
    radius = expected_radius(raw);
    return radius ? radius / 16 : (q->type == B_COMMAND ? 7 : 3);
}

static void ref_add(int x, int y, int radius, int explore_radius) {
    int cell, c = x / 16, r = y / 16;
    for (cell = 0; cell < MAP_CELLS; ++cell) {
        int col = cell % MAP_COLS, row = cell / MAP_COLS;
        int dx = col * 16 + 8 - x, dy = row * 16 + 8 - y;
        int tx = col - c, ty = row - r;
        if (dx * dx + dy * dy <= radius * radius) ++ref_count[cell];
        if (tx * tx + ty * ty <= explore_radius * explore_radius)
            ref_explored[cell] = 1;
    }
}

static void compare(int step) {
    int i, cell;
    memset(ref_count, 0, sizeof(ref_count));
    memset(ref_visible, 0, sizeof(ref_visible));
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !u[i].loaded)
            ref_add(u[i].x, u[i].y, owner_unit_radius(u[i].type),
                    unit_explore_radius(u[i].type));
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE)
            ref_add(b[i].x, b[i].y, owner_building_radius(&b[i]),
                    building_explore_radius(&b[i]));
    memset(ref_mini_visible, 0, sizeof(ref_mini_visible));
    memset(ref_mini_explored, 0, sizeof(ref_mini_explored));
    for (cell = 0; cell < MAP_CELLS; ++cell) {
        int mini = (cell % MAP_COLS) * 24 / MAP_COLS +
                   ((cell / MAP_COLS) * 24 / MAP_ROWS) * 24;
        ref_visible[cell] = ref_count[cell] != 0;
        ref_mini_visible[mini] += ref_visible[cell];
        ref_mini_explored[mini] += ref_explored[cell] != 0;
        if (visible[cell] != ref_visible[cell] || explored[cell] != ref_explored[cell] ||
            sc64_visibility_source_count(cell) != ref_count[cell]) {
            fprintf(stderr, "cell/count mismatch step=%d cell=%d vis=%u/%u exp=%u/%u count=%u/%u\n",
                    step, cell, visible[cell], ref_visible[cell], explored[cell],
                    ref_explored[cell], sc64_visibility_source_count(cell), ref_count[cell]);
            exit(1);
        }
    }
    for (i = 0; i < 24 * 24; ++i)
        if (sc64_minimap_visible[i] != ref_mini_visible[i] ||
            sc64_minimap_explored[i] != ref_mini_explored[i]) {
            fprintf(stderr, "minimap mismatch step=%d cell=%d got=%u/%u want=%u/%u\n",
                    step, i, sc64_minimap_visible[i], sc64_minimap_explored[i],
                    ref_mini_visible[i], ref_mini_explored[i]);
            exit(1);
        }
}

static void update(int step) { sc64_update_exploration(); compare(step); }

static void clear_world(void) {
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    memset(visible, 0, sizeof(visible)); memset(explored, 0, sizeof(explored));
    memset(ref_explored, 0, sizeof(ref_explored));
    sc64_visibility_reset();
}

static int run_radius_matrix(void) {
    static const int types[] = {35,37,38,39,103,42,45,62,44};
    static const int moves[][2] = {{1,0},{-1,0},{0,1},{0,-1},
        {1,1},{1,-1},{-1,1},{-1,-1},{15,0},{-15,0},{0,15},{0,-15},
        {15,15},{15,-15},{-15,15},{-15,-15}};
    int ti, px, py, mi, step = 1;
    for (ti = 0; ti < (int)(sizeof(types)/sizeof(types[0])); ++ti) {
        clear_world(); u[0] = (Unit){1,SIDE,400,400,types[ti],0}; update(step++);
        for (px = 0; px < 16; ++px) for (py = 0; py < 16; ++py) {
            int x = 400 + px, y = 400 + py;
            u[0].x=x; u[0].y=y; update(step++);
            for (mi = 0; mi < (int)(sizeof(moves)/sizeof(moves[0])); ++mi) {
                u[0].x=x+moves[mi][0]; u[0].y=y+moves[mi][1]; update(step++);
                u[0].x=x; u[0].y=y; update(step++);
            }
        }
    }
    return step;
}

static int run_fallback_matrix(int step) {
    static const int types[] = {U_SCV, U_UNKNOWN};
    static const int moves[][2] = {{1,0},{-1,0},{0,1},{0,-1},
        {1,1},{1,-1},{-1,1},{-1,-1},{15,0},{-15,0},{0,15},{0,-15},
        {15,15},{15,-15},{-15,15},{-15,-15}};
    int ti, px, py, mi;
    z7_active = 0; /* Exercise unchanged 56/72 pixel fallback radii. */
    for (ti = 0; ti < (int)(sizeof(types)/sizeof(types[0])); ++ti) {
        clear_world(); u[0]=(Unit){1,SIDE,400,400,types[ti],0}; update(step++);
        for (px=0;px<16;++px) for (py=0;py<16;++py) {
            int x=400+px, y=400+py;
            u[0].x=x; u[0].y=y; update(step++);
            for (mi=0;mi<(int)(sizeof(moves)/sizeof(moves[0]));++mi) {
                u[0].x=x+moves[mi][0]; u[0].y=y+moves[mi][1]; update(step++);
                u[0].x=x; u[0].y=y; update(step++);
            }
        }
    }
    clear_world(); z7_active=1;
    return step;
}

static int run_unsupported_radius(int step) {
    static const int moves[][2]={{1,0},{-1,1},{15,0},{0,-15}};
    int active, i;
    for (active=0; active<=1; ++active) {
        clear_world(); z7_active=active;
        u[0]=(Unit){1,SIDE,400,400,U_ZP_SOURCE,0}; update(step++);
        for (i=0;i<(int)(sizeof(moves)/sizeof(moves[0]));++i) {
            u[0].x+=moves[i][0]; u[0].y+=moves[i][1]; update(step++);
        }
        u[0].x=1023; u[0].y=0; update(step++); /* top-right clipped fallback */
        u[0].alive=0; update(step++);
    }
    clear_world(); z7_active=1;
    return step;
}

static int run_lifecycle_matrix(int step) {
    clear_world();
    u[0]=(Unit){1,SIDE,0,0,38,0}; update(step++);
    u[0].x=1023; u[0].y=1023; update(step++); /* distant teleport */
    u[0].x=1023; u[0].y=0; update(step++); /* top-right clipped source */
    u[0].type=44; update(step++); /* type change */
    u[0].alive=0; update(step++);
    u[0].alive=1; u[0].owner=2; update(step++);
    u[0].owner=SIDE; u[0].loaded=1; update(step++);
    u[0].loaded=0; update(step++);
    u[0].x=0; u[0].y=1023; update(step++);
    b[0]=(Building){1,SIDE,0,0,134,0,0}; update(step++);
    b[0].under_construction=1; update(step++); /* unfinished non-morph */
    b[0].type=B_LAIR; update(step++); /* target type maps to predecessor */
    b[0].under_construction=0; update(step++); /* completion expands sight */
    b[0].type=B_HIVE; b[0].under_construction=1; update(step++);
    b[0].under_construction=0; update(step++);
    b[0].type=B_GREATER_SPIRE; b[0].under_construction=1; update(step++);
    b[0].under_construction=0; update(step++);
    b[0].owner=2; update(step++);
    b[0].alive=0; update(step++);
    clear_world(); z7_active=0;
    u[1]=(Unit){1,SIDE,511,511,35,0}; update(step++); /* inactive fallback */
    clear_world(); z7_active=1;
    u[1]=(Unit){1,SIDE,511,511,35,0}; update(step++);
    return step;
}

static int run_dense_counts(int step) {
    int i, cell = 32 + 32 * MAP_COLS;
    clear_world();
    for (i=0;i<MAX_UNITS;++i) u[i]=(Unit){1,SIDE,512,512,44,0};
    b[0]=(Building){1,SIDE,512,512,133,0,0};
    update(step++);
    if (sc64_visibility_source_count(cell) != 257) {
        fprintf(stderr,"nine-bit count did not exceed 255\n"); return 1;
    }
    for (i=0;i<MAX_UNITS;++i) { u[i].alive=0; update(step++); }
    b[0].alive=0; update(step++);
    if (sc64_visibility_source_count(cell) != 0) return 1;
    /* Fill every source slot, then cross both nine-bit boundaries. */
    for (i=0;i<MAX_UNITS;++i) u[i]=(Unit){1,SIDE,512,512,44,0};
    for (i=0;i<MAX_BUILDINGS;++i) b[i]=(Building){1,SIDE,512,512,133,0,0};
    update(step++);
    if (sc64_visibility_source_count(cell) != 511) return 1;
    u[0].alive=0; update(step++);
    if (sc64_visibility_source_count(cell) != 510) return 1;
    for (i=1;i<MAX_UNITS;++i) u[i].alive=0;
    update(step++);
    if (sc64_visibility_source_count(cell) != 255) return 1;
    for (i=0;i<MAX_BUILDINGS;++i) b[i].alive=0;
    update(step++);
    if (sc64_visibility_source_count(cell) != 0) return 1;
    return 0;
}

int main(void) {
    int step;
#ifdef SC64_Z7_SIGHT_SPANS
    if (check_z7_lookup()) return 1;
#endif
#ifdef SC64_Z7_VISIBILITY_ROW_CONTEXT
    if (check_prepared_rows()) return 1;
#endif
    clear_world();
    step = run_radius_matrix();
    step = run_fallback_matrix(step);
    step = run_unsupported_radius(step);
    step = run_lifecycle_matrix(step);
    if (run_dense_counts(step)) return 1;
    puts("Z7 sight span full-grid oracle passed");
    return 0;
}
'''


class Z7SightSpansTests(unittest.TestCase):
    def test_sight_span_gate_requires_z7_source_and_legacy_spans(self):
        include = '#include "starcraft/sc64_visibility.h"\n'
        common = ["cc", "-E", "-x", "c", "-I", str(ROOT), "-"]
        cases = (
            ("-DSC64_Z7_SIGHT_SPANS", "-DSC64_VISIBILITY_SPANS",
             "-DSC64_RACES", "-DSC64_Z7_RUNTIME"),
            ("-DSC64_Z7_SIGHT_SPANS", "-DSC64_Z7_SOURCE_SIGHT",
             "-DSC64_RACES", "-DSC64_Z7_RUNTIME"),
        )
        expected = "SC64_Z7_SIGHT_SPANS requires SC64_Z7_SOURCE_SIGHT and SC64_VISIBILITY_SPANS"
        for flags in cases:
            result = subprocess.run([*common, *flags], input=include, text=True,
                                    capture_output=True, cwd=ROOT)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(expected, result.stderr)

    def test_span_gate_matches_independent_full_grid_oracle(self):
        with tempfile.TemporaryDirectory(prefix="sc64-z7-sight-") as temp:
            source = Path(temp) / "fixture.c"
            source.write_text(C_SOURCE)
            variants = (
                ("scalar", [], 0),
                ("legacy-spans", ["-DSC64_VISIBILITY_SPANS"], 0),
                ("z7-spans", ["-DSC64_VISIBILITY_SPANS", "-DSC64_Z7_SIGHT_SPANS"], 1),
                ("z7-phase", ["-DSC64_VISIBILITY_SPANS", "-DSC64_Z7_SIGHT_SPANS",
                              "-DSC64_Z7_VISIBILITY_PHASE_REJECT"], 1),
                ("z7-row-context", ["-DSC64_VISIBILITY_SPANS", "-DSC64_Z7_SIGHT_SPANS",
                                    "-DSC64_Z7_VISIBILITY_PHASE_REJECT",
                                    "-DSC64_Z7_VISIBILITY_ROW_CONTEXT"], 1),
            )
            for suffix, extra, expect_z7 in variants:
                binary = Path(temp) / f"fixture-{suffix}"
                command = [
                    "cc", "-std=c11", "-O1", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined",
                    "-Wno-unused-variable",
                    f"-DTEST_EXPECT_Z7_SPANS={expect_z7}",
                    "-DSC64_RACES", "-DSC64_Z7_RUNTIME", "-DSC64_Z7_SOURCE_SIGHT",
                    "-DSC64_ZP_CAMPAIGN_ACTORS", "-DSC64_ZP_CAMPAIGN_STORAGE",
                    *extra, "-I", str(ROOT), str(source), "-o", str(binary),
                ]
                subprocess.run(command, check=True, cwd=ROOT)
                result = subprocess.run([str(binary)], cwd=ROOT,
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("full-grid oracle passed", result.stdout)


if __name__ == "__main__":
    unittest.main()
