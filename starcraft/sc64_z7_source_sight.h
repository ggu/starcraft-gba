#ifndef SC64_Z7_SOURCE_SIGHT_H
#define SC64_Z7_SOURCE_SIGHT_H

#if defined(SC64_Z7_SOURCE_SIGHT)
#if !defined(SC64_RACES) || !defined(SC64_Z7_RUNTIME)
#error "Z7 source sight requires the imported Z7 runtime"
#endif

/* BWAPI d727fed68558c506163048ea889131d8cbb33915 UnitType::sightRange
 * values, expressed here in source pixels
 * (tiles * 32).  The GBA map is half scale; sc64_visibility converts these
 * radii at the point where it rasterizes cell centers.  Keep this table
 * sparse and keyed by authenticated raw UnitType ids. */
static int
sc64_z7_source_unit_sight_raw(int raw)
{
    switch (raw) {
    case 35: case 36: return 128; /* Larva, Egg: 4 tiles */
    case 37: case 47: return 160; /* Zergling, Scourge: 5 tiles */
    case 38: return 192;          /* Hydralisk: 6 */
    case 39: case 43: case 149: return 224; /* Ultra, Muta, Extractor: 7 */
    case 40: return 160;          /* Broodling: 5 */
    case 41: return 224;          /* Drone: 7 */
    case 42: return 288;          /* Overlord: 9 */
    case 44: return 352;          /* Guardian: 11 */
    case 45: case 46: case 130: return 320; /* Queen, Defiler, Infested CC: 10 */
    case 50: return 160;          /* Infested Terran: 5 */
    case 51: return 288;          /* Infested Kerrigan: 9 */
    case 59: case 97: return 128; /* Cocoon, Lurker Egg: 4 */
    case 62: return 320;          /* Devourer: 10 */
    case 103: return 256;         /* Lurker: 8 */
    case 131: return 288;         /* Hatchery: 9 */
    case 132: return 320;         /* Lair: 10 */
    case 133: return 352;         /* Hive: 11 */
    case 134: case 135: case 136: case 137: case 138: case 139:
    case 140: case 141: case 142: return 256; /* Zerg tech buildings: 8 */
    case 143: case 144: case 146: return 320; /* Creep/Spore/Sunken: 10 */
    default: return 0;
    }
}

static int
sc64_z7_source_unit_sight_pixels(int type)
{
    if (!race_z7_active()) return 0;
    return sc64_z7_source_unit_sight_raw(sc64_race_raw_unit(type));
}

static int
sc64_z7_source_building_sight_pixels(int type)
{
    if (!race_z7_active()) return 0;
/* BW uses four tiles for an unfinished, grounded, non-morphing building.
   The visibility snapshot encodes that state as -(type+1), so removal
   uses the same radius even if the building completes later in the tick. */
    if (type < 0) return 128;
    return sc64_z7_source_unit_sight_raw(sc64_race_raw_building(type));
}

static int __attribute__((noinline))
sc64_z7_building_visibility_key(const Building *q)
{
    int morphing;
    if (!q || !q->alive || (!race_z7_active()
#ifdef SC64_Z8_RUNTIME
        && !race_z8_active()
#endif
        )) return q ? q->type : 0;
    /* Native morphs keep the source UnitType until completion. The bounded
       runtime changes q->type to its target immediately, so recover the
       known predecessor while under construction. */
    if (q->under_construction) {
        if (q->type == B_LAIR) return B_HATCHERY;
        if (q->type == B_HIVE) return B_LAIR;
        if (q->type == B_GREATER_SPIRE) return B_SPIRE;
    }
    morphing = 0;
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    morphing |= !!(q->flags &
        (SC64_Z7_SUNKEN_MORPH_FLAG | SC64_Z7_SPORE_MORPH_FLAG));
#endif
    if (q->under_construction && !morphing) return -(q->type + 1);
    return q->type;
}
#endif /* SC64_Z7_SOURCE_SIGHT */

#endif /* SC64_Z7_SOURCE_SIGHT_H */
