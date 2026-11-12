/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_RULES_H
#define SC64_NUKE_RULES_H
#include <stdint.h>

/* Stock missile facts. Supply is in the engine's doubled units. Flight
 * cadence is supplied by the caller, so source and native timing can be
 * compared without freezing guessed timings into an SRAM format. */
enum {
    SC64_NUKE_MISSILE_RAW = 14, SC64_NUKE_SILO_RAW = 108,
    SC64_NUKE_GHOST_RAW = 1, SC64_NUKE_COVERT_RAW = 117,
    SC64_NUKE_MISSILE_ORE = 200, SC64_NUKE_MISSILE_GAS = 200,
    SC64_NUKE_MISSILE_SUPPLY = 16, SC64_NUKE_MISSILE_SOURCE_TICKS = 1500,
    SC64_NUKE_SOURCE_SIGHT = 288, SC64_NUKE_OCULAR_SOURCE_SIGHT = 352,
    SC64_NUKE_RADIUS_INNER = 128, SC64_NUKE_RADIUS_MIDDLE = 192,
    SC64_NUKE_RADIUS_OUTER = 256
};

typedef struct {
    uint16_t remaining;
    uint8_t armed;
} Sc64NukeSilo;

typedef struct {
    uint16_t paint, descent, warning;
} Sc64NukeCadence;

enum { SC64_NUKE_IDLE, SC64_NUKE_PAINTING, SC64_NUKE_DESCENDING };
enum {
    SC64_NUKE_EVENT_WARNING = 1, SC64_NUKE_EVENT_RELEASE = 2,
    SC64_NUKE_EVENT_IMPACT = 4, SC64_NUKE_EVENT_ABORT = 8
};
typedef struct {
    uint16_t x, y, spotter, age;
    uint8_t owner, phase;
    Sc64NukeCadence cadence;
} Sc64NukeFlight;

static inline int
sc64_nuke_train(Sc64NukeSilo *silo, int *ore, int *gas, unsigned reserved,
                unsigned cap, unsigned ticks)
{
    if (silo->armed || silo->remaining || !ticks || ticks > UINT16_MAX ||
        *ore < SC64_NUKE_MISSILE_ORE || *gas < SC64_NUKE_MISSILE_GAS ||
        reserved > cap || cap - reserved < SC64_NUKE_MISSILE_SUPPLY)
        return 0;
    *ore -= SC64_NUKE_MISSILE_ORE;
    *gas -= SC64_NUKE_MISSILE_GAS;
    silo->remaining = (uint16_t)ticks;
    return 1;
}

static inline unsigned sc64_nuke_silo_supply(const Sc64NukeSilo *silo)
{
    return (silo->armed || silo->remaining) ? SC64_NUKE_MISSILE_SUPPLY : 0;
}

static inline int sc64_nuke_train_step(Sc64NukeSilo *silo)
{
    if (!silo->remaining || --silo->remaining) return 0;
    silo->armed = 1;
    return 1;
}

static inline int sc64_nuke_cadence_valid(Sc64NukeCadence cadence)
{
    return cadence.warning && cadence.warning <= cadence.paint &&
           cadence.descent && (unsigned)cadence.paint + cadence.descent <= UINT16_MAX;
}

/* The world adapter checks a complete attached owned silo, an eligible Ghost
 * within its sight radius, and current map coordinates before calling this.
 * Keep the missile independent of its silo as soon as it is committed. */
static inline int
sc64_nuke_launch(Sc64NukeSilo *silo, Sc64NukeFlight *flight, unsigned owner,
                 unsigned spotter, unsigned x, unsigned y, Sc64NukeCadence cadence)
{
    if (!silo->armed || silo->remaining || flight->phase != SC64_NUKE_IDLE ||
        owner >= 5 || spotter > UINT16_MAX || x > UINT16_MAX || y > UINT16_MAX ||
        !sc64_nuke_cadence_valid(cadence))
        return 0;
    silo->armed = 0;
    *flight = (Sc64NukeFlight){
        (uint16_t)x, (uint16_t)y, (uint16_t)spotter, 0,
        (uint8_t)owner, SC64_NUKE_PAINTING, cadence
    };
    return 1;
}

static inline unsigned sc64_nuke_flight_supply(const Sc64NukeFlight *flight)
{
    return flight->phase != SC64_NUKE_IDLE ? SC64_NUKE_MISSILE_SUPPLY : 0;
}

/* The caller checks slot identity, same owner, order, and ability to track.
 * A lost spotter consumes the missile while painting. Once released, the
 * descending missile is independent of the spotter and the silo. */
static inline unsigned sc64_nuke_flight_step(Sc64NukeFlight *flight, int spotter_can_track)
{
    unsigned event = 0;
    if (flight->phase == SC64_NUKE_IDLE) return 0;
    if (flight->phase == SC64_NUKE_PAINTING && !spotter_can_track) {
        flight->phase = SC64_NUKE_IDLE;
        return SC64_NUKE_EVENT_ABORT;
    }
    ++flight->age;
    if (flight->phase == SC64_NUKE_PAINTING) {
        if (flight->age == flight->cadence.warning)
            event |= SC64_NUKE_EVENT_WARNING;
        if (flight->age == flight->cadence.paint) {
            flight->phase = SC64_NUKE_DESCENDING;
            event |= SC64_NUKE_EVENT_RELEASE;
        }
    } else if (flight->age == (unsigned)flight->cadence.paint + flight->cadence.descent) {
        flight->phase = SC64_NUKE_IDLE;
        event |= SC64_NUKE_EVENT_IMPACT;
    }
    return event;
}

/* Source-pixel rectangle bounds are inclusive. Use the stock integer distance
 * approximation on nonnegative separations, not distance to the actor center. */
static inline unsigned
sc64_nuke_source_distance(int x, int y, int left, int top, int right, int bottom)
{
    unsigned a = x < left ? (unsigned)(left - x) : x > right ? (unsigned)(x - right) : 0;
    unsigned b = y < top ? (unsigned)(top - y) : y > bottom ? (unsigned)(y - bottom) : 0;
    if (a < b) { unsigned swap = a; a = b; b = swap; }
    if (a / 4 < b) return a - a / 16 - a / 64 + (3 * b) / 8 + (3 * b) / 256;
    return a;
}

/* The caller applies Explosive size scaling and armor through normal hit
 * resolution. The packet is eligible for friendly fire and air targets;
 * the verified PC reference excludes burrowed targets in the outer rings. */
static inline unsigned
sc64_nuke_damage_packet_q8(unsigned max_hp, unsigned max_shield, unsigned source_distance,
                           int burrowed)
{
    uint64_t combined = (uint64_t)max_hp + max_shield;
    uint64_t damage = combined * 2 / 3;
    if (source_distance > SC64_NUKE_RADIUS_OUTER ||
        (burrowed && source_distance > SC64_NUKE_RADIUS_INNER)) return 0;
    if (damage < 500) damage = 500;
    damage *= source_distance > SC64_NUKE_RADIUS_MIDDLE ? 64u :
              source_distance > SC64_NUKE_RADIUS_INNER ? 128u : 256u;
    return damage > UINT32_MAX ? UINT32_MAX : (unsigned)damage;
}
#endif
