#ifndef SC64_Z5_ENSNARE_H
#define SC64_Z5_ENSNARE_H

#include "../common/gba.h"

#if defined(SC64_Z5_ENSNARE_ENABLED)
#if !defined(SC64_Z5_RUNTIME)
#error "SC64_Z5_ENSNARE_ENABLED requires SC64_Z5_RUNTIME"
#endif

enum {
    SC64_Z5_ENSNARE_HERO_RAW_TYPE = 51,
    SC64_Z5_ENSNARE_TECH_ID = 17,
    SC64_Z5_ENSNARE_COST_Q8 = 75 * 256,
    SC64_Z5_ENSNARE_RANGE_SOURCE_PIXELS = 288,
    SC64_Z5_ENSNARE_AREA_HALF_WIDTH_SOURCE_PIXELS = 64,
    SC64_Z5_ENSNARE_STATUS_DURATION = 75,
    SC64_Z5_ENSNARE_STATUS_FRAMES = 8,
    SC64_Z5_ENSNARE_NOMINAL_NATIVE_FRAMES = 600,
    SC64_Z5_ENSNARE_NOMINAL_GBA_TICKS_30HZ = 750,
    SC64_Z5_ENSNARE_MOVEMENT_SPEED_NUMERATOR = 1,
    SC64_Z5_ENSNARE_MOVEMENT_SPEED_DENOMINATOR = 2,
    SC64_Z5_ENSNARE_WEAPON_COOLDOWN_NUMERATOR = 5,
    SC64_Z5_ENSNARE_WEAPON_COOLDOWN_DENOMINATOR = 4,
    SC64_Z5_ENSNARE_GBA_SOURCE_FRAME_NUMERATOR = 4,
    SC64_Z5_ENSNARE_GBA_SOURCE_FRAME_DENOMINATOR = 5,
    SC64_Z5_ENSNARE_MAX_ACTORS = 512,
    SC64_Z5_ENSNARE_ERROR = -1,
    SC64_Z5_ENSNARE_UNCHANGED = 0,
    SC64_Z5_ENSNARE_CAST = 1,
    SC64_Z5_ENSNARE_ADVANCED = 2
};

typedef struct Sc64Z5EnsnareActor {
    int x;
    int y;
    /* Source unit-finder bounding box endpoints. `right` and `bottom` use
     * the same endpoint convention as the pinned engine's stored bounds. */
    int bbox_left;
    int bbox_top;
    int bbox_right;
    int bbox_bottom;
    int energy_q8;
    u16 raw_type;
    u8 owner;
    u8 alive;
    u8 completed;
    u8 hallucinated;
    u8 disabled;
    u8 building;
    u8 invincible;
    u8 burrowed;
    u8 hidden;
} Sc64Z5EnsnareActor;

typedef struct Sc64Z5EnsnareStatus {
    u8 timer;
    u8 cycle_counter;
} Sc64Z5EnsnareStatus;

_Static_assert(sizeof(Sc64Z5EnsnareStatus) == 2, "Z5 Ensnare status size");

static inline int sc64_z5_ensnare_actor_valid(const Sc64Z5EnsnareActor *a)
{
    return a && a->x >= -1000000 && a->x <= 1000000 &&
           a->y >= -1000000 && a->y <= 1000000 &&
           a->bbox_left >= -1000000 && a->bbox_left <= 1000000 &&
           a->bbox_top >= -1000000 && a->bbox_top <= 1000000 &&
           a->bbox_right >= -1000000 && a->bbox_right <= 1000000 &&
           a->bbox_bottom >= -1000000 && a->bbox_bottom <= 1000000 &&
           a->owner < 12 && a->energy_q8 >= 0 &&
           a->energy_q8 <= 250 * 256 && a->bbox_left <= a->x &&
           a->bbox_top <= a->y && a->bbox_right > a->x &&
           a->bbox_bottom > a->y && a->bbox_right - a->bbox_left <= 4096 &&
           a->bbox_bottom - a->bbox_top <= 4096 && a->alive <= 1 &&
           a->completed <= 1 && a->hallucinated <= 1 && a->disabled <= 1 &&
           a->building <= 1 && a->invincible <= 1 && a->burrowed <= 1 &&
           a->hidden <= 1;
}

static inline int sc64_z5_ensnare_status_valid(const Sc64Z5EnsnareStatus *s)
{
    return s && s->timer <= SC64_Z5_ENSNARE_STATUS_DURATION &&
           s->cycle_counter < SC64_Z5_ENSNARE_STATUS_FRAMES;
}

/* Native order_Spell moves to weapon range before firing. The caller supplies
 * engine-computed integer distance and halt distance; no map or floating-point
 * geometry is duplicated here. */
static inline int sc64_z5_ensnare_in_range(int bbox_distance,
                                            int caster_halt_distance)
{
    return bbox_distance >= 0 && bbox_distance <= 1000000 &&
           caster_halt_distance >= 0 && caster_halt_distance <= 1000000 &&
           bbox_distance <= SC64_Z5_ENSNARE_RANGE_SOURCE_PIXELS +
                                caster_halt_distance;
}

/* Resolve one native square_at(pos,64)/find_units_noexpand query against the
 * supplied actor array, refreshing eligible targets' status timer to 75. The
 * rectangle uses the pinned unit-finder endpoint rules. No owner filter is
 * applied; this intentionally models native friendly fire. */
static inline int sc64_z5_ensnare_apply(Sc64Z5EnsnareActor *actors,
                                         Sc64Z5EnsnareStatus *statuses,
                                         unsigned count,
                                         unsigned caster_index,
                                         int tech_available,
                                         int cast_x,
                                         int cast_y,
                                         int cast_distance,
                                         int caster_halt_distance)
{
    const Sc64Z5EnsnareActor *caster;
    if (!actors || !statuses || count == 0 ||
        count > SC64_Z5_ENSNARE_MAX_ACTORS || caster_index >= count ||
        tech_available < 0 || tech_available > 1 || cast_x < -1000000 ||
        cast_x > 1000000 || cast_y < -1000000 || cast_y > 1000000 ||
        cast_distance < 0 || cast_distance > 1000000 ||
        caster_halt_distance < 0)
        return SC64_Z5_ENSNARE_ERROR;
    for (unsigned i = 0; i < count; ++i) {
        if (!sc64_z5_ensnare_actor_valid(&actors[i]) ||
            !sc64_z5_ensnare_status_valid(&statuses[i]))
            return SC64_Z5_ENSNARE_ERROR;
    }
    caster = &actors[caster_index];
    if (!tech_available || caster->raw_type != SC64_Z5_ENSNARE_HERO_RAW_TYPE ||
        !caster->alive || !caster->completed || caster->hallucinated ||
        caster->disabled || caster->burrowed ||
        caster->energy_q8 < SC64_Z5_ENSNARE_COST_Q8 ||
        !sc64_z5_ensnare_in_range(cast_distance, caster_halt_distance))
        return SC64_Z5_ENSNARE_UNCHANGED;

    int left = cast_x - SC64_Z5_ENSNARE_AREA_HALF_WIDTH_SOURCE_PIXELS;
    int top = cast_y - SC64_Z5_ENSNARE_AREA_HALF_WIDTH_SOURCE_PIXELS;
    int right = cast_x + SC64_Z5_ENSNARE_AREA_HALF_WIDTH_SOURCE_PIXELS;
    int bottom = cast_y + SC64_Z5_ENSNARE_AREA_HALF_WIDTH_SOURCE_PIXELS;
    for (unsigned i = 0; i < count; ++i) {
        const Sc64Z5EnsnareActor *a = &actors[i];
        if (i == caster_index || !a->alive || a->building || a->invincible ||
            a->burrowed || a->bbox_left >= right || a->bbox_top >= bottom ||
            a->bbox_right < left || a->bbox_bottom < top)
            continue;
        statuses[i].timer = SC64_Z5_ENSNARE_STATUS_DURATION;
    }
    /* Source charges energy on firing even when the area contains no targets. */
    actors[caster_index].energy_q8 -= SC64_Z5_ENSNARE_COST_Q8;
    return SC64_Z5_ENSNARE_CAST;
}

/* Advance one 30Hz runtime tick. Phase represents 4/5 of a native simulation
 * frame per project tick. Each source frame updates a unit's timer only when
 * the native `completed || !hidden` condition holds. User pause freezes both
 * the conversion phase and status clocks. */
static inline int sc64_z5_ensnare_tick_30hz(Sc64Z5EnsnareActor *actors,
                                             Sc64Z5EnsnareStatus *statuses,
                                             unsigned count,
                                             u8 *frame_phase,
                                             int paused)
{
    unsigned phase, native_frames;
    if (!actors || !statuses || !frame_phase || count > SC64_Z5_ENSNARE_MAX_ACTORS ||
        *frame_phase >= SC64_Z5_ENSNARE_GBA_SOURCE_FRAME_DENOMINATOR ||
        paused < 0 || paused > 1)
        return SC64_Z5_ENSNARE_ERROR;
    for (unsigned i = 0; i < count; ++i) {
        if (!sc64_z5_ensnare_actor_valid(&actors[i]) ||
            !sc64_z5_ensnare_status_valid(&statuses[i]))
            return SC64_Z5_ENSNARE_ERROR;
    }
    if (paused) return SC64_Z5_ENSNARE_UNCHANGED;
    phase = *frame_phase + SC64_Z5_ENSNARE_GBA_SOURCE_FRAME_NUMERATOR;
    native_frames = phase / SC64_Z5_ENSNARE_GBA_SOURCE_FRAME_DENOMINATOR;
    phase %= SC64_Z5_ENSNARE_GBA_SOURCE_FRAME_DENOMINATOR;
    for (unsigned frame = 0; frame < native_frames; ++frame) {
        for (unsigned i = 0; i < count; ++i) {
            if (!actors[i].completed && actors[i].hidden) continue;
            if (++statuses[i].cycle_counter >= SC64_Z5_ENSNARE_STATUS_FRAMES) {
                statuses[i].cycle_counter = 0;
                if (statuses[i].timer) --statuses[i].timer;
            }
        }
    }
    *frame_phase = (u8)phase;
    return native_frames ? SC64_Z5_ENSNARE_ADVANCED : SC64_Z5_ENSNARE_UNCHANGED;
}

#endif /* SC64_Z5_ENSNARE_ENABLED */
#endif
