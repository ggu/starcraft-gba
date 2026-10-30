#ifndef SC64_P7_OBSERVER_RULES_H
#define SC64_P7_OBSERVER_RULES_H

/* P7 Observer cloak/detection policy. Numeric ranges and lifecycle rules are
 * transcribed from the pinned BWAPI/OpenBW facts; runtime state not modeled
 * by this game is supplied as false by the caller. */
#if defined(SC64_P7_OBSERVER_ENABLED) && \
    (!defined(SC64_RACES) || !defined(SC64_P7_DATA_ENABLED) || \
     !defined(SC64_P7_RUNTIME) || !defined(SC64_P7_ACTORS) || \
     !defined(SC64_ZP_CAMPAIGN_ACTORS))
#error "P7 Observer behavior requires P7 data/runtime, race actors, and Z/P facts"
#endif

/* Aura, intrinsic cloaks, and cached detection share one opt-in profile so
 * no half-enabled combination can silently use a different visibility path. */
#if defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED) && \
    (!defined(SC64_P7_OBSERVER_ENABLED) || !defined(SC64_RACES) || \
     !defined(SC64_P7_DATA_ENABLED) || !defined(SC64_P7_RUNTIME) || \
     !defined(SC64_P7_ACTORS) || !defined(SC64_ZP_CAMPAIGN_ACTORS))
#error "P7 Arbiter cloak requires P7 Observer, actors, runtime, and campaign facts"
#endif
#if defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED) && \
    !defined(SC64_P7_BATTERY_ENABLED)
#error "P7 cloak cache requires P7 persistent state"
#endif
#if defined(SC64_P7_ARBITER_CLOAK_ENABLED) && \
    !defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED)
#error "Use the unified P7 Arbiter cloak/cache feature gate"
#endif

/* Geometry hoisting changes only how the cache reuses validated source
 * rectangles. Keep it behind the complete cache profile. */
#if defined(SC64_P7_GEOMETRY_RECT_HOIST_ENABLED) && \
    !defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED)
#error "P7 geometry hoist requires the Arbiter cloak cache"
#endif

enum {
    SC64_P7_OBSERVER_RAW = 84,
    SC64_P7_HERO_DARK_TEMPLAR_RAW = 74,
    SC64_P7_ZERATUL_RAW = 75,
    SC64_P7_OBSERVER_BASE_SIGHT_SOURCE_PX = 288,
    SC64_P7_OBSERVER_SENSOR_ARRAY_SIGHT_SOURCE_PX = 352,
    SC64_P7_CANNON_RAW = 162,
    SC64_P7_CANNON_DETECTION_SOURCE_PX = 224,
    /* Zerg detectors (pinned BWAPI sight ranges; structures use 7 tiles). */
    SC64_OVERLORD_RAW = 42,
    SC64_OVERLORD_DETECTION_SOURCE_PX = 352,
    SC64_SPORE_COLONY_RAW = 144,
    SC64_SPORE_DETECTION_SOURCE_PX = 224
};

typedef struct {
    unsigned completed : 1;
    unsigned enabled : 1;
    unsigned blinded : 1;
    unsigned powered : 1;
    unsigned hallucinated : 1;
} Sc64P7DetectorState;

static int sc64_p7_observer_is_cloaked(unsigned raw_type)
{
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    return raw_type == SC64_P7_OBSERVER_RAW ||
           raw_type == SC64_P7_HERO_DARK_TEMPLAR_RAW ||
           raw_type == SC64_P7_ZERATUL_RAW;
#else
    return raw_type == SC64_P7_OBSERVER_RAW;
#endif
}

static int sc64_p7_detector_usable(const Sc64P7DetectorState *state,
                                   int is_building)
{
    return state && state->completed && state->enabled && !state->blinded &&
           !state->hallucinated && (!is_building || state->powered);
}

static int sc64_p7_detector_range_source_pixels(unsigned raw_type,
                                                int grounded,
                                                int sensor_array_level)
{
    if (raw_type == SC64_P7_CANNON_RAW && grounded)
        return SC64_P7_CANNON_DETECTION_SOURCE_PX;
    if (raw_type == SC64_P7_OBSERVER_RAW)
        return sensor_array_level > 0 ?
            SC64_P7_OBSERVER_SENSOR_ARRAY_SIGHT_SOURCE_PX :
            SC64_P7_OBSERVER_BASE_SIGHT_SOURCE_PX;
    return 0;
}

#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
static int sc64_p7_observer_target_allowed_cloaked(int cloaked,
                                                   int optically_visible,
                                                   int detected)
{
    if (cloaked)
        return optically_visible && detected;
    return optically_visible;
}
static int sc64_p7_observer_target_allowed(unsigned target_raw_type,
                                           int optically_visible,
                                           int detected)
{
    return sc64_p7_observer_target_allowed_cloaked(
        sc64_p7_observer_is_cloaked(target_raw_type), optically_visible,
        detected);
}
#else
static int sc64_p7_observer_target_allowed(unsigned target_raw_type,
                                           int optically_visible,
                                           int detected)
{
    if (sc64_p7_observer_is_cloaked(target_raw_type))
        return optically_visible && detected;
    return optically_visible;
}
#endif

#endif
