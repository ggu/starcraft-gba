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

enum {
    SC64_P7_OBSERVER_RAW = 84,
    SC64_P7_OBSERVER_BASE_SIGHT_SOURCE_PX = 288,
    SC64_P7_OBSERVER_SENSOR_ARRAY_SIGHT_SOURCE_PX = 352,
    SC64_P7_CANNON_RAW = 162,
    SC64_P7_CANNON_DETECTION_SOURCE_PX = 224
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
    return raw_type == SC64_P7_OBSERVER_RAW;
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

static int sc64_p7_observer_target_allowed(unsigned target_raw_type,
                                           int optically_visible,
                                           int detected)
{
    if (sc64_p7_observer_is_cloaked(target_raw_type))
        return optically_visible && detected;
    return optically_visible;
}

#endif
