/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ORIGINAL_SKIRMISH_MAP_H
#define ORIGINAL_SKIRMISH_MAP_H

/* Original terrain layouts. Coordinates are 16-pixel cells; rectangles are
 * inclusive. All four starting areas have the same resource clearances. */
enum { ORIGINAL_MAP_COUNT = 2, ORIGINAL_MAP_SIZE = 64, ORIGINAL_MAP_MAX_OBSTACLES = 8 };

typedef struct { unsigned char left, top, right, bottom; } OriginalMapObstacle;
typedef struct {
    const char *title;
    int start_x[4], start_y[4];
    unsigned char obstacle_count;
    OriginalMapObstacle obstacles[ORIGINAL_MAP_MAX_OBSTACLES];
} OriginalSkirmishMap;

static const OriginalSkirmishMap original_skirmish_maps[ORIGINAL_MAP_COUNT] = {
    {
        "TRANSIT YARD", {160, 864, 864, 160}, {160, 864, 160, 864}, 4,
        {{30, 4, 33, 22}, {30, 41, 33, 59},
         {4, 30, 22, 33}, {41, 30, 59, 33}}
    },
    {
        "STONE GARDENS", {160, 864, 864, 160}, {160, 864, 160, 864}, 4,
        {{22, 22, 27, 27}, {36, 22, 41, 27},
         {22, 36, 27, 41}, {36, 36, 41, 41}}
    }
};

static inline int original_skirmish_blocked(int map, int col, int row)
{
    unsigned int i;
    const OriginalSkirmishMap *layout;
    if (map < 0 || map >= ORIGINAL_MAP_COUNT || col < 0 || row < 0 ||
        col >= ORIGINAL_MAP_SIZE || row >= ORIGINAL_MAP_SIZE) return 1;
    layout = &original_skirmish_maps[map];
    for (i = 0; i < layout->obstacle_count; ++i) {
        const OriginalMapObstacle *o = &layout->obstacles[i];
        if (col >= o->left && col <= o->right && row >= o->top && row <= o->bottom)
            return 1;
    }
    return 0;
}
#endif
