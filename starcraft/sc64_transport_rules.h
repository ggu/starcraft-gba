#ifndef SC64_TRANSPORT_RULES_H
#define SC64_TRANSPORT_RULES_H

/* Transport constants transcribed from the pinned BWAPI UnitType tables:
 * UnitType.cpp unitSpaceRequired/unitSpaceProvided (raw UnitType IDs).
 * BWAPI defines required-space 255 as not transportable. This file encodes
 * immutable type facts only; runtime ownership, completion, movement, range,
 * loading, unloading, and death behavior remain caller responsibilities. */
enum {
    SC64_TRANSPORT_DROPSHIP_RAW = 11,
    SC64_TRANSPORT_SHUTTLE_RAW = 69,
    SC64_TRANSPORT_OVERLORD_RAW = 42,
    SC64_TRANSPORT_DROPSHIP_SPACE = 8,
    SC64_TRANSPORT_SHUTTLE_SPACE = 8,
    SC64_TRANSPORT_OVERLORD_SPACE = 8,
    SC64_TRANSPORT_SPACE_UNAVAILABLE = 255,
    SC64_TRANSPORT_MAX_PASSENGERS = 8
};

/* Number of passenger space points provided by a supported raw transport. */
static inline unsigned sc64_transport_space_provided_raw(unsigned raw_type)
{
    if (raw_type == SC64_TRANSPORT_DROPSHIP_RAW)
        return SC64_TRANSPORT_DROPSHIP_SPACE;
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (raw_type == SC64_TRANSPORT_SHUTTLE_RAW)
        return SC64_TRANSPORT_SHUTTLE_SPACE;
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (raw_type == SC64_TRANSPORT_OVERLORD_RAW)
        return SC64_TRANSPORT_OVERLORD_SPACE;
#endif
    return 0u;
}

/* Return 1 for raw types with verified BWAPI spaceRequired data, including
 * known non-transportable types (output 255); return 0 for unknown IDs. */
static inline int sc64_transport_space_required_raw(unsigned raw_type,
                                                    unsigned *out_space)
{
    unsigned value;
    if (!out_space) return 0;
    switch (raw_type) {
    /* T5 Terran actors plus commonly retained T5 Zerg ground units. */
    case 0: case 1: case 7: case 10: case 15: case 16: case 20:
    case 32: case 34: case 37: case 41:
        value = 1; break;
    case 2: case 3: case 19: case 38:
        value = 2; break;
    case 5: case 23:
        value = 4; break;
#ifdef SC64_Z6_TRANSPORT_ENABLED
    /* Authenticated Z6 ground cargo. raw51 is Kerrigan; raw39 is Ultralisk. */
    case 39: value = 4; break;
    case 51: value = 1; break;
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    /* P5's authenticated Protoss escort passengers. Values are pinned
     * UnitType::unitSpaceRequired entries, not estimates from actor size. */
    case 64: value = 1; break; /* Probe */
    case 65: case 67: case 79: value = 2; break; /* Zealot, Templar, Tassadar */
    case 66: value = 4; break; /* Dragoon */
#endif
    /* Verified non-transportable types from the same BWAPI table. */
    case 4: case 6: case 8: case 9: case 11: case 12: case 13: case 14:
    case 17: case 18: case 21: case 22: case 24: case 25: case 26:
    case 27: case 28: case 29: case 30: case 31:
    case 33: case 35: case 36: case 42: case 43: case 45: case 47:
#ifdef SC64_P5_SHUTTLE_ENABLED
    case 69:
#endif
    /* Terran, Zerg, and Protoss building IDs used by the scenario adapters. */
    case 106: case 109: case 110: case 111: case 112: case 113: case 114:
    case 115:
    case 122: case 124: case 125: case 130: case 131: case 132: case 133:
    case 135: case 137: case 139: case 141: case 142: case 143: case 144:
    case 146: case 149: case 150: case 151: case 154: case 155: case 156:
    case 157: case 160: case 162: case 164: case 166: case 167: case 194:
    case 195: case 203: case 205: case 206: case 207: case 208: case 209:
    case 211: case 212:
        value = SC64_TRANSPORT_SPACE_UNAVAILABLE; break;
    default:
        return 0;
    }
    *out_space = value;
    return 1;
}

#ifdef SC64_SHARED_TRANSPORT
/* Shared transport uses the same immutable capacity facts in every mode. */
static inline int sc64_transport_unconditional_carrier_raw(unsigned raw_type)
{
    return raw_type == SC64_TRANSPORT_DROPSHIP_RAW ||
           raw_type == SC64_TRANSPORT_SHUTTLE_RAW;
}
static inline int sc64_transport_shared_space_required_raw(unsigned raw_type,
                                                           unsigned *out_space)
{
    unsigned value;
    if (!out_space) return 0;
    /* Pinned unitSpaceRequired numeric defaults for all source unit IDs.
     * Unknown/out-of-range IDs do not become eligible passengers. */
    if (raw_type >= 228u) return 0;
    switch (raw_type) {
    case 0: case 1: case 7: case 10: case 15: case 16: case 20:
    case 32: case 34: case 37: case 40: case 41: case 50: case 51:
    case 54: case 64: case 99: case 100: case 104: value = 1; break;
    case 2: case 3: case 17: case 19: case 38: case 46: case 52:
    case 53: case 61: case 65: case 67: case 74: case 75: case 77:
    case 79: case 87: value = 2; break;
    case 5: case 23: case 39: case 48: case 63: case 66: case 68:
    case 76: case 78: case 81: case 83: case 103: value = 4; break;
    default: value = SC64_TRANSPORT_SPACE_UNAVAILABLE; break;
    }
    *out_space = value;
    return 1;
}
static inline unsigned sc64_transport_shared_space_provided_raw(unsigned raw_type)
{
    return raw_type == SC64_TRANSPORT_DROPSHIP_RAW ||
           raw_type == SC64_TRANSPORT_SHUTTLE_RAW ||
           raw_type == SC64_TRANSPORT_OVERLORD_RAW ? 8u : 0u;
}
#endif

static inline int sc64_transport_passenger_eligible_raw(unsigned passenger_raw)
{
    unsigned required;
    return sc64_transport_space_required_raw(passenger_raw, &required) &&
           required != SC64_TRANSPORT_SPACE_UNAVAILABLE;
}

/* Exact type-capacity accounting for the private transport runtime. The
 * manifest lists existing passenger raw types; runtime eligibility checks
 * (same owner, live, not burrowed, completed/enabled carrier, proximity) must
 * be performed separately. Invalid/unknown cargo rejects the whole query. */
static inline int sc64_transport_load_fits_raw(unsigned transport_raw,
                                               const unsigned short *loaded_raw,
                                               unsigned loaded_count,
                                               unsigned passenger_raw)
{
    unsigned capacity = sc64_transport_space_provided_raw(transport_raw);
    unsigned used = 0, required;
    if (!capacity || loaded_count > SC64_TRANSPORT_MAX_PASSENGERS ||
        (loaded_count && !loaded_raw) ||
        !sc64_transport_space_required_raw(passenger_raw, &required) ||
        required == SC64_TRANSPORT_SPACE_UNAVAILABLE) return 0;
    for (unsigned i = 0; i < loaded_count; ++i) {
        unsigned space;
        if (!sc64_transport_space_required_raw(loaded_raw[i], &space) ||
            space == SC64_TRANSPORT_SPACE_UNAVAILABLE) return 0;
        used += space;
        if (used > capacity) return 0;
    }
    return used + required <= capacity;
}

#endif
