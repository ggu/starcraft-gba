#ifndef SC64_STATIC_KEY_H
#define SC64_STATIC_KEY_H

/* Track only art that the static pass can draw into this viewport. Payload
 * changes outside that set cannot affect the cached pixels; entering/leaving
 * the set is itself a key change. Keep exact values rather than a hash. */
static unsigned
sc64_static_objects_key(void)
{
    enum { FIELDS = MAX_MINERALS * 4 + MAX_RACE_GEYSERS * 3 + MAX_BUILDINGS * 6
#ifdef SC64_T4_SCENARIO_RUNTIME
                    + 32 * 2
#endif
#ifdef SC64_Z5_RUNTIME
                    + SC64_Z5_UNIT_BACKED_DOODADS * 2
#endif
    };
#ifdef SC64_ZP_FULL_STORAGE
    /* Coordinates, raw IDs, owner/type and visibility fit signed 16 bits.
     * Preserve exact valid values while reserving RAM for later map rosters. */
    static s16 previous[FIELDS];
#else
    static int previous[FIELDS];
#endif
    static unsigned generation;
    int n = 0, changed = 0;
#ifdef SC64_ZP_FULL_STORAGE
#define SC64_STATIC_FIELD(value) do { \
    int next_value = (value); \
    if (next_value < -32768 || next_value > 32767) { \
        previous[n] = 0; changed = 1; \
    } else if (previous[n] != next_value) { \
        previous[n] = (s16)next_value; changed = 1; \
    } \
    ++n; \
} while (0)
#else
#define SC64_STATIC_FIELD(value) do { \
    int next_value = (value); \
    if (previous[n] != next_value) { \
        previous[n] = next_value; changed = 1; \
    } \
    ++n; \
} while (0)
#endif
    for (int i = 0; i < MAX_MINERALS; ++i) {
        int x = m[i].x - camx, y = m[i].y - camy + 12;
        int drawn = m[i].amount > 0 && x > -6 && x < 245 &&
                    y > 7 && y < 123 && currently_visible(m[i].x, m[i].y);
        SC64_STATIC_FIELD(drawn);
        if (drawn) {
            SC64_STATIC_FIELD(m[i].x);
            SC64_STATIC_FIELD(m[i].y);
            SC64_STATIC_FIELD(m[i].style);
        } else n += 3;
    }
    for (int i = 0; i < MAX_RACE_GEYSERS; ++i) {
        int x = geysers[i].x - camx, y = geysers[i].y - camy + 12;
        int drawn = i < geyser_count && x > 0 && x < 240 &&
                    y > 12 && y < 120 && currently_visible(geysers[i].x, geysers[i].y);
        SC64_STATIC_FIELD(drawn);
        if (drawn) {
            SC64_STATIC_FIELD(geysers[i].x);
            SC64_STATIC_FIELD(geysers[i].y);
        } else n += 2;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        int x = q->x - camx, y = q->y - camy + 12;
        int drawn = q->alive && x >= -20 && x <= 260 && y >= 2 && y <= 130 &&
                    (q->owner == SIDE || currently_visible(q->x, q->y));
        SC64_STATIC_FIELD(drawn);
        if (drawn) {
            SC64_STATIC_FIELD(q->x);
            SC64_STATIC_FIELD(q->y);
            SC64_STATIC_FIELD(q->type);
            SC64_STATIC_FIELD(q->owner);
#ifdef SC64_RACES
            SC64_STATIC_FIELD(q->raw_type);
#else
            SC64_STATIC_FIELD(0);
#endif
        } else n += 5;
    }
#ifdef SC64_T4_SCENARIO_RUNTIME
    /* Prop state changes the endpoint/visibility drawn into this cache.
     * Include the slot binding too, so restart/load cannot reuse art captured
     * for a previous mapping. */
    for (int i = 0; i < 32; ++i) {
        SC64_STATIC_FIELD(race_t4.doodad_slot[i]);
        SC64_STATIC_FIELD(race_t4.doodad_enabled[i]);
    }
#endif
#ifdef SC64_Z5_RUNTIME
    for (int i = 0; i < SC64_Z5_UNIT_BACKED_DOODADS; ++i) {
        SC64_STATIC_FIELD(race_z5_prop_cache_field((unsigned)i, 0));
        SC64_STATIC_FIELD(race_z5_prop_cache_field((unsigned)i, 1));
    }
#endif
#undef SC64_STATIC_FIELD
    if (changed) ++generation;
    return generation;
}

#endif
