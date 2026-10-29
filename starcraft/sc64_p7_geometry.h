#ifndef SC64_P7_GEOMETRY_H
#define SC64_P7_GEOMETRY_H

/* Pure source-pixel geometry for P7 actors. Runtime centers are at half the
 * CHK/BWAPI pixel scale; the output is a half-open source-pixel rectangle.
 * BWAPI dimensions use inclusive offsets from the center, hence +1 on the
 * right and bottom edges. Facts come from the pinned BWAPI UnitType tables
 * (d727fed68558c506163048ea889131d8cbb33915); the small fallback tables below
 * cover mapped placement types without a profile fact row. */
#include "race_rules.h"
#include <limits.h>

#if defined(SC64_P7_RECT_TABLES) && !defined(SC64_FACT_TABLE_GENERATOR)
#if !defined(SC64_FACT_TABLES) || !defined(SC64_P7_ACTORS)
#error "SC64_P7_RECT_TABLES requires FACT_TABLES and P7 actors"
#endif
#endif
#ifdef SC64_P7_ACTORS
static int sc64_p7_actor_rect_source(unsigned raw,int x,int y,int building,
                                      int *left,int *top,int *right,int *bottom)
{
#if defined(SC64_P7_RECT_TABLES) && !defined(SC64_FACT_TABLE_GENERATOR)
    int l,u,r,d;
#else
    int type,l,u,r,d;
#endif
    /* Source centers are doubled below. Keep room for the largest supported
     * actor extents on the positive edge so malformed INT_MAX coordinates
     * cannot overflow either the multiply or the rectangle edge additions. */
    if (!left || !top || !right || !bottom || raw>227u || x<0 || y<0 ||
        x>(INT_MAX-256)/2 || y>(INT_MAX-256)/2)
        return 0;
#if defined(SC64_P7_RECT_TABLES) && !defined(SC64_FACT_TABLE_GENERATOR)
    if (!sc64_fact_p7_rect_extents(raw,building,&l,&u,&r,&d)) return 0;
#else
    type=building ? sc64_race_building_from_raw((int)raw) :
                    sc64_race_unit_from_raw((int)raw);
    if (type<0) return 0;
    if (building) {
        if (!sc64_zp_building_bounds_source(type,&l,&u,&r,&d)) {
            /* Exact pinned UnitType::dimension{Left,Up,Right,Down} values
             * for mapped structures without a ZP feature-fact row. */
            switch(raw) {
            case 154:l=56;u=39;r=56;d=39;break; /* Nexus */
            case 155:l=36;u=16;r=40;d=20;break; /* Robotics Facility */
            case 156:l=16;u=12;r=16;d=20;break; /* Pylon */
            case 157:l=48;u=32;r=48;d=24;break; /* Assimilator */
            case 160:l=48;u=32;r=48;d=40;break; /* Gateway */
            case 162:l=20;u=16;r=20;d=16;break; /* Photon Cannon */
            case 164:l=40;u=24;r=40;d=24;break; /* Cybernetics Core */
            case 166:l=36;u=24;r=40;d=18;break; /* Forge */
            case 167:l=48;u=40;r=48;d=32;break; /* Stargate */
            case 194:l=48;u=32;r=47;d=31;break; /* Special Zerg Beacon */
#ifdef SC64_P8_DATA_ENABLED
            case 168:l=64;u=48;r=63;d=47;break; /* Stasis Cell/Prison */
#endif
            default:return 0;
            }
        }
    } else if (!sc64_zp_actor_bounds_source(type,&l,&u,&r,&d)) {
        /* Exact source extents for the remaining base actors in P7. The
         * other P7 map actors use P6/Z5/ZP/P7 facts or other audited facts. */
        switch(raw) {
        case 0:l=8;u=9;r=8;d=10;break;   /* Marine */
        case 64:l=11;u=11;r=11;d=11;break; /* Probe */
        case 65:l=11;u=5;r=11;d=13;break; /* Zealot */
        case 66:l=15;u=15;r=16;d=16;break; /* Dragoon */
        case 70:l=18;u=16;r=17;d=15;break; /* Scout */
        case 77:l=11;u=5;r=11;d=13;break; /* Fenix Zealot */
        case 78:l=15;u=15;r=16;d=16;break; /* Fenix Dragoon */
        case 79:l=12;u=10;r=11;d=13;break; /* Tassadar */
        case 83:l=16;u=16;r=15;d=15;break; /* Reaver */
        default:return 0;
        }
    }
#endif
    int cx=x*2,cy=y*2;
    *left=cx-l; *top=cy-u;
    *right=cx+r+1; *bottom=cy+d+1;
    return 1;
}
#endif /* SC64_P7_ACTORS */

#endif /* SC64_P7_GEOMETRY_H */
