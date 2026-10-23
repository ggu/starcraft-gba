#ifndef SC64_T6_ACTOR_RULES_H
#define SC64_T6_ACTOR_RULES_H

/* Numeric identity/default facts from pinned BWAPI d727fed68558c506163048ea889131d8cbb33915:
 * UnitType.h and UnitType.cpp's defaultMaxHP, defaultArmorAmount,
 * defaultOreCost, defaultGasCost, defaultTimeCost, sightRangeTiles,
 * unitDimensions, unitFlags and unitWeapons tables. No implementation copied.
 * Raw 126 is Special_Crashed_Norad_II, a stationary, unarmed structure;
 * Hero_Norad_II is the different raw-29 flying Battlecruiser.
 * These facts do not enable a T6 scenario or add construction menu entries. */
enum {
    SC64_T6_CRASHED_NORAD_RAW = 126,
    SC64_T6_QUEENS_NEST_RAW = 138,
    SC64_T6_CRASHED_NORAD_HP = 700,
    SC64_T6_QUEENS_NEST_HP = 850,
    SC64_T6_QUEENS_NEST_MINERALS = 150,
    SC64_T6_QUEENS_NEST_GAS = 100,
    SC64_T6_QUEENS_NEST_BUILD_FRAMES = 900
};

typedef struct {
    unsigned short raw_type, hp, minerals, gas, build_frames;
    unsigned short sight_pixels;
    unsigned char armor, tile_width, tile_height;
    unsigned char left, up, right, down; /* source pixels at original scale */
    unsigned char flying, ground_weapon, air_weapon;
} Sc64T6BuildingFact;

static const Sc64T6BuildingFact sc64_t6_building_facts[] = {
    { SC64_T6_CRASHED_NORAD_RAW, SC64_T6_CRASHED_NORAD_HP, 800, 600, 4800,
      320, 1, 3, 2, 48, 32, 47, 31, 0, 0, 0 },
    { SC64_T6_QUEENS_NEST_RAW, SC64_T6_QUEENS_NEST_HP,
      SC64_T6_QUEENS_NEST_MINERALS, SC64_T6_QUEENS_NEST_GAS,
      SC64_T6_QUEENS_NEST_BUILD_FRAMES,
      256, 1, 3, 2, 38, 28, 32, 28, 0, 0, 0 }
};

static inline const Sc64T6BuildingFact *sc64_t6_building_fact(unsigned raw)
{
    for (unsigned i = 0; i < sizeof(sc64_t6_building_facts) /
                                    sizeof(sc64_t6_building_facts[0]); ++i)
        if (sc64_t6_building_facts[i].raw_type == raw)
            return &sc64_t6_building_facts[i];
    return 0;
}

#endif
