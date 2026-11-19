/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Layer facts from the authenticated USA unit table, 00C/001. Ordinary
 * units, finished buildings and mineral fields share layer 4. */
static unsigned
sc64_world_actor_layer(int raw, unsigned fallback)
{
    switch (raw) {
    case 194: case 195: case 196: case 197: case 198: case 199:
        return 0;
    case 191: case 192: case 193: case 204: case 214:
        return 1;
    case 35: case 142:
        return 2;
    case 125:
        return 3;
    case 9: case 11: case 12: case 22: case 27: case 28: case 29:
    case 42: case 57: case 58: case 71: case 72: case 82: case 86: case 102:
        return 16;
    case 8: case 14: case 21: case 33: case 43: case 44: case 45: case 47:
    case 49: case 55: case 56: case 59: case 60: case 62: case 69: case 70:
    case 73: case 80: case 84: case 88: case 91: case 92: case 94: case 98: case 101:
        return 18;
    case 105: case 202:
        return 19;
    default:
        return (unsigned)raw < 228u ? 4u : fallback;
    }
}

static unsigned
sc64_building_world_layer(unsigned slot)
{
    const Building *q = &b[slot];
    /* Native liftoff puts the building between ground units and flyers.
     * Use the live flight state, including during travel to a landing site. */
    if (q->flags & 4) {
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (race_late_airborne((int)slot)) return 12;
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        if (race_bc_facility_airborne((int)slot)) return 12;
#endif
    }
    int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
    return sc64_world_actor_layer(raw, 4);
}

enum {
    DEPTH_MINERAL, DEPTH_GEYSER, DEPTH_BUILDING, DEPTH_UNIT,
    DEPTH_SLOT_BITS = 10,
    DEPTH_SLOT_MASK = (1 << DEPTH_SLOT_BITS) - 1,
    DEPTH_Y_BITS = DEPTH_SLOT_BITS + 2,
    DEPTH_WORLD_Y_BITS = 15,
    DEPTH_LAYER_BITS = DEPTH_Y_BITS + DEPTH_WORLD_Y_BITS,
};

_Static_assert(MAX_UNITS <= DEPTH_SLOT_MASK + 1 &&
               MAX_BUILDINGS <= DEPTH_SLOT_MASK + 1 &&
               MAX_MINERALS <= DEPTH_SLOT_MASK + 1 &&
               MAX_RACE_GEYSERS <= DEPTH_SLOT_MASK + 1,
               "painter keys must hold every actor slot");

/* Ground depth uses the world anchor. Air ties use kind and slot.
 * Camera position and animation offsets leave this ordering stable. */
static void
sc64_depth_push(u32 *heap, unsigned *size, unsigned layer, int y,
                unsigned kind, unsigned slot)
{
    unsigned row = layer <= 4 ? (unsigned)limit(y, 0, (1 << DEPTH_WORLD_Y_BITS) - 1) : 0;
    u32 key = (layer << DEPTH_LAYER_BITS) | (row << DEPTH_Y_BITS) |
              (kind << DEPTH_SLOT_BITS) | slot;
    unsigned at = (*size)++;
    while (at) {
        unsigned parent = (at - 1) / 2;
        if (heap[parent] <= key) break;
        heap[at] = heap[parent];
        at = parent;
    }
    heap[at] = key;
}

static u32
sc64_depth_pop(u32 *heap, unsigned *size)
{
    u32 first = heap[0], key = heap[--*size];
    unsigned at = 0;
    while (at * 2 + 1 < *size) {
        unsigned child = at * 2 + 1;
        if (child + 1 < *size && heap[child + 1] < heap[child]) ++child;
        if (key <= heap[child]) break;
        heap[at] = heap[child];
        at = child;
    }
    heap[at] = key;
    return first;
}

static void __attribute__((noinline))
sc64_draw_depth_ordered_actors(void)
{
    u32 heap[MAX_UNITS + MAX_BUILDINGS + MAX_MINERALS + MAX_RACE_GEYSERS];
    unsigned size = 0;
    for (unsigned i = 0; i < MAX_MINERALS; ++i) {
        if (m[i].amount <= 0) continue;
        int x = m[i].x - camx, y = m[i].y - camy + 12;
        if (x > -6 && x < 245 && y > 7 && y < 123)
            sc64_depth_push(heap, &size, 4, m[i].y, DEPTH_MINERAL, i);
    }
    for (int i = 0; i < geyser_count; ++i) {
        int x = geysers[i].x - camx, y = geysers[i].y - camy + 12;
        if (geyser_in_view(x, y))
            sc64_depth_push(heap, &size, 4, geysers[i].y, DEPTH_GEYSER, (unsigned)i);
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive) continue;
        int x = b[i].x - camx, y = b[i].y - camy + 12;
        if (x < -128 || x > 368 || y < -128 || y > 248) continue;
        sc64_depth_push(heap, &size, sc64_building_world_layer(i), b[i].y, DEPTH_BUILDING, i);
    }
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (!q->alive) continue;
        int x = q->x - camx, y = q->y - camy + 12;
        if (!sc64_unit_anchor_in_view(q, x, y)) continue;
        int raw = sc64_unit_art_raw(q);
        unsigned fallback = sc64_race_unit_flying(q->type) ? 16u : 4u;
        sc64_depth_push(heap, &size, sc64_world_actor_layer(raw, fallback), q->y, DEPTH_UNIT, i);
    }
    while (size) {
        u32 key = sc64_depth_pop(heap, &size);
        unsigned i = key & DEPTH_SLOT_MASK;
        switch ((key >> DEPTH_SLOT_BITS) & 3u) {
        case DEPTH_MINERAL:
            draw_map_mineral((int)i);
            break;
        case DEPTH_GEYSER:
            draw_map_geyser((int)i);
            break;
        case DEPTH_BUILDING:
            draw_building_art(&b[i]);
            draw_building_overlay(&b[i]);
            break;
        case DEPTH_UNIT:
            if (u[i].selected) draw_unit_pass(&u[i], 1);
            draw_unit(&u[i]);
            break;
        }
    }
}
