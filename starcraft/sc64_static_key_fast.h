/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STATIC_KEY_FAST_H
#define SC64_STATIC_KEY_FAST_H

/* SC64_TERRAIN_FAST_VALIDATE variant of the culled static-object key in
 * sc64_static_key.h. It returns the same generation on every call: the
 * generation advances exactly when the reference would see one of its
 * compared fields change.
 *
 * The reference keeps one "drawn" field per slot (0, or 1/3 for a building
 * drawn while complete/under construction) and the payload fields of the
 * slot as of the last frame it was drawn. Here the drawn fields are bit
 * words (one bit per slot, plus a construction bit for buildings that is set
 * only for drawn slots), so an undrawn slot costs no field-array access and a
 * whole word of undrawn slots is checked with one compare. A drawn-field
 * mismatch in the reference is a bit mismatch here, and payload fields are
 * compared and stored with the reference's exact rules. Payload storage
 * starts at zero, as in the reference, and is only touched for drawn slots.
 *
 * Visibility is the same test as currently_visible() with the map size read
 * once per call. */

#ifndef SC64_STATIC_KEY_CULL
#error "SC64_TERRAIN_FAST_VALIDATE provides only the SC64_STATIC_KEY_CULL key"
#endif

static inline int
sc64_static_key_visible(int x, int y, int cols, int rows)
{
    int col = x / 16, row = y / 16;
    if (col < 0 || row < 0 || col >= cols || row >= rows)
        return 0;
    return SC64_GRID_GET(visible, col + row * cols);
}

#define SC64_STATIC_KEY_WORDS(count) (((count) + 31) / 32)

static unsigned
sc64_static_objects_key(void)
{
#ifdef SC64_ZP_FULL_STORAGE
    typedef s16 Field;
#else
    typedef int Field;
#endif
    static Field mineral_fields[MAX_MINERALS * 3];
    static Field geyser_fields[MAX_RACE_GEYSERS * 2];
    static Field building_fields[MAX_BUILDINGS * 5];
    static u32 mineral_drawn[SC64_STATIC_KEY_WORDS(MAX_MINERALS)];
    static u32 geyser_drawn[SC64_STATIC_KEY_WORDS(MAX_RACE_GEYSERS)];
    static u32 building_drawn[SC64_STATIC_KEY_WORDS(MAX_BUILDINGS)];
    static u32 building_built[SC64_STATIC_KEY_WORDS(MAX_BUILDINGS)];
#if defined(SC64_T4_SCENARIO_RUNTIME) || defined(SC64_Z5_RUNTIME) || defined(SC64_P6_RUNTIME)
    enum { EXTRA_FIELDS = 0
#ifdef SC64_T4_SCENARIO_RUNTIME
                          + 32 * 2
#endif
#ifdef SC64_Z5_RUNTIME
                          + SC64_Z5_UNIT_BACKED_DOODADS * 2
#endif
#ifdef SC64_P6_RUNTIME
                          + SC64_P6_DOODAD_SOURCE_COUNT * 2
#endif
    };
    static Field extra_fields[EXTRA_FIELDS];
    int n = 0;
#endif
    static unsigned generation;
    const int cols = MAP_COLS, rows = MAP_ROWS;
    const int view_x = camx, view_y = camy;
    int changed = 0, w;
#ifdef SC64_ZP_FULL_STORAGE
#define SC64_STATIC_FAST_FIELD(slot, value) do { \
    int next_value = (value); \
    if (next_value < -32768 || next_value > 32767) { \
        (slot) = 0; changed = 1; \
    } else if ((slot) != next_value) { \
        (slot) = (Field)next_value; changed = 1; \
    } \
} while (0)
#else
#define SC64_STATIC_FAST_FIELD(slot, value) do { \
    int next_value = (value); \
    if ((slot) != next_value) { \
        (slot) = next_value; changed = 1; \
    } \
} while (0)
#endif
    /* An undrawn slot costs its presence test and the loop step; the slot's
     * bit and fields are derived from its index only when it is drawn. */
#define SC64_STATIC_FAST_MINERAL(p) \
    if ((p)->amount > 0) { \
        int x = (p)->x - view_x, y = (p)->y - view_y + 12; \
        if (x > -6 && x < 245 && y > 7 && y < 123 && \
            sc64_static_key_visible((p)->x, (p)->y, cols, rows)) { \
            Field *f = mineral_fields + 3 * ((p) - m); \
            drawn |= 1u << ((p) - first); \
            SC64_STATIC_FAST_FIELD(f[0], (p)->x); \
            SC64_STATIC_FAST_FIELD(f[1], (p)->y); \
            SC64_STATIC_FAST_FIELD(f[2], (p)->style); \
        } \
    }
    {
        const Mineral *p = m;
        for (w = 0; w < SC64_STATIC_KEY_WORDS(MAX_MINERALS); ++w) {
            const Mineral *first = p;
            const Mineral *end = p + (MAX_MINERALS - w * 32 < 32 ?
                                      MAX_MINERALS - w * 32 : 32);
            u32 drawn = 0;
            for (; p < end; ++p) {
                SC64_STATIC_FAST_MINERAL(p)
            }
            if (mineral_drawn[w] != drawn) {
                mineral_drawn[w] = drawn;
                changed = 1;
            }
        }
    }
#undef SC64_STATIC_FAST_MINERAL
    {
        Field *f = geyser_fields;
        int i = 0;
        for (w = 0; w < SC64_STATIC_KEY_WORDS(MAX_RACE_GEYSERS); ++w) {
            int end = MAX_RACE_GEYSERS - w * 32 < 32 ? MAX_RACE_GEYSERS : i + 32;
            u32 drawn = 0, bit = 1;
            for (; i < end; ++i, f += 2, bit <<= 1) {
                int x = geysers[i].x - view_x, y = geysers[i].y - view_y + 12;
                if (!(i < geyser_count && geyser_in_view(x, y) &&
                      sc64_static_key_visible(geysers[i].x, geysers[i].y, cols, rows)))
                    continue;
                drawn |= bit;
                SC64_STATIC_FAST_FIELD(f[0], geysers[i].x);
                SC64_STATIC_FAST_FIELD(f[1], geysers[i].y);
            }
            if (geyser_drawn[w] != drawn) {
                geyser_drawn[w] = drawn;
                changed = 1;
            }
        }
    }
#ifdef SC64_RACES
#define SC64_STATIC_FAST_RAW(q) (q)->raw_type
#else
#define SC64_STATIC_FAST_RAW(q) 0
#endif
#define SC64_STATIC_FAST_BUILDING(q) \
    if ((q)->alive) { \
        int x = (q)->x - view_x, y = (q)->y - view_y + 12; \
        if (building_art_in_view((q), x, y) && \
            ((q)->owner == SIDE || sc64_static_key_visible((q)->x, (q)->y, cols, rows))) { \
            u32 bit = 1u << ((q) - first); \
            Field *f = building_fields + 5 * ((q) - b); \
            drawn |= bit; \
            if ((q)->under_construction) built |= bit; \
            SC64_STATIC_FAST_FIELD(f[0], (q)->x); \
            SC64_STATIC_FAST_FIELD(f[1], (q)->y); \
            SC64_STATIC_FAST_FIELD(f[2], (q)->type); \
            SC64_STATIC_FAST_FIELD(f[3], (q)->owner); \
            SC64_STATIC_FAST_FIELD(f[4], SC64_STATIC_FAST_RAW(q)); \
        } \
    }
    {
        const Building *q = b;
        for (w = 0; w < SC64_STATIC_KEY_WORDS(MAX_BUILDINGS); ++w) {
            const Building *first = q;
            const Building *end = q + (MAX_BUILDINGS - w * 32 < 32 ?
                                       MAX_BUILDINGS - w * 32 : 32);
            u32 drawn = 0, built = 0;
            for (; q < end; ++q) {
                SC64_STATIC_FAST_BUILDING(q)
            }
            if (building_drawn[w] != drawn || building_built[w] != built) {
                building_drawn[w] = drawn;
                building_built[w] = built;
                changed = 1;
            }
        }
    }
#undef SC64_STATIC_FAST_BUILDING
#undef SC64_STATIC_FAST_RAW
#ifdef SC64_T4_SCENARIO_RUNTIME
    for (int i = 0; i < 32; ++i) {
        SC64_STATIC_FAST_FIELD(extra_fields[n], race_t4.doodad_slot[i]); ++n;
        SC64_STATIC_FAST_FIELD(extra_fields[n], race_t4.doodad_enabled[i]); ++n;
    }
#endif
#ifdef SC64_Z5_RUNTIME
    for (int i = 0; i < SC64_Z5_UNIT_BACKED_DOODADS; ++i) {
        SC64_STATIC_FAST_FIELD(extra_fields[n], race_z5_prop_cache_field((unsigned)i, 0)); ++n;
        SC64_STATIC_FAST_FIELD(extra_fields[n], race_z5_prop_cache_field((unsigned)i, 1)); ++n;
    }
#endif
#ifdef SC64_P6_RUNTIME
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i) {
        SC64_STATIC_FAST_FIELD(extra_fields[n], race_p6_prop_cache_field((unsigned)i, 0)); ++n;
        SC64_STATIC_FAST_FIELD(extra_fields[n], race_p6_prop_cache_field((unsigned)i, 1)); ++n;
    }
#endif
#undef SC64_STATIC_FAST_FIELD
    if (changed) ++generation;
    return generation;
}

#undef SC64_STATIC_KEY_WORDS
#endif
