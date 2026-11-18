/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Shared world and per-frame record of the SC64_TERRAIN_FAST_VALIDATE
 * differential test. tests/sc64_terrain_fast_validate_side.c is compiled
 * twice, without and with the flag, and both copies run the same frames. */
#ifndef SC64_TERRAIN_FAST_VALIDATE_TEST_H
#define SC64_TERRAIN_FAST_VALIDATE_TEST_H

#include <stdint.h>

#define FV_MAX_MINERALS 72
#define FV_MAX_GEYSERS 12
#define FV_MAX_BUILDINGS 80
#define FV_MAP_MAX 128
#define FV_GRID_BYTES (FV_MAP_MAX * FV_MAP_MAX)
#define FV_CELLS (17 * 9)
#define FV_DOODADS 32

typedef struct {
    int cols, rows, race_mode, mission, camx, camy;
    /* One byte per cell: bit 0 visible, bit 1 explored. */
    uint8_t fog[FV_GRID_BYTES];
    int mineral_x[FV_MAX_MINERALS], mineral_y[FV_MAX_MINERALS];
    int mineral_amount[FV_MAX_MINERALS], mineral_style[FV_MAX_MINERALS];
    int geyser_x[FV_MAX_GEYSERS], geyser_y[FV_MAX_GEYSERS], geyser_count;
    int building_x[FV_MAX_BUILDINGS], building_y[FV_MAX_BUILDINGS];
    int building_type[FV_MAX_BUILDINGS], building_owner[FV_MAX_BUILDINGS];
    int building_alive[FV_MAX_BUILDINGS], building_construction[FV_MAX_BUILDINGS];
    int building_raw[FV_MAX_BUILDINGS];
    int doodad[FV_DOODADS][2];
    /* A second draw window to query the per-tile answers with. */
    int probe_col0, probe_row0, probe_col1, probe_row1;
} FvWorld;

typedef struct {
    unsigned key;
    int hit, reused, valid, stored, mask_count, cache_camx, cache_camy;
    unsigned requests, hits, partial;
    int window_col0, window_row0, window_col1, window_row1;
    /* Per tile of the draw window, row-major: 1 + clean, then partial and
     * state for tiles that are not clean. Zero past the window. */
    uint8_t clean[FV_CELLS];
    uint8_t part[FV_CELLS];
    uint16_t state[FV_CELLS];
    uint8_t dirty[FV_CELLS];
    uint8_t part_cells[FV_CELLS];
    uint16_t mask[FV_CELLS];
    uint16_t now[FV_CELLS];
    int now_valid, now_col0, now_row0, now_cols, now_rows;
    uint32_t fp[2][16];
    /* Answers for the probe window (same layout as clean/part/state). */
    uint8_t probe_clean[FV_CELLS];
    uint8_t probe_part[FV_CELLS];
    uint16_t probe_state[FV_CELLS];
#ifdef SC64_DIAGNOSTIC_TERRAIN
    uint32_t diag[14];
#endif
} FvRecord;

/* Not compared: how the fast side answered the draw windows. */
typedef struct {
    unsigned indexed, call, none, probe_indexed, probe_call, probe_none;
} FvModes;

void fv_ref_frame(const FvWorld *world, FvRecord *record);
void fv_fast_frame(const FvWorld *world, FvRecord *record, FvModes *modes);

#endif
