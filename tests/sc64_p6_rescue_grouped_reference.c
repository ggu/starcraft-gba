/* Independent copy of the pre-optimization nested rescue scan. It operates on
 * compact row views so the oracle is separately compiled and cannot call the
 * production adapter or share its temporary owner list. */
#include <stddef.h>

typedef struct {
    unsigned char alive;
    int owner, x, y, target, tx, ty;
    unsigned char selected, order;
} RescueOracleUnit;
typedef struct { unsigned char alive; int owner, x, y; } RescueOracleBuilding;

enum { ORACLE_SIDE = 0, ORACLE_RESCUABLE = 3, ORACLE_IDLE = 0 };

int sc64_p6_rescue_fullscan_oracle(RescueOracleUnit *units, unsigned unit_count,
                                    RescueOracleBuilding *buildings,
                                    unsigned building_count)
{
    int changed = 0;
    for (unsigned i = 0; i < unit_count; ++i)
        if (units[i].alive && units[i].owner == ORACLE_RESCUABLE) {
            for (unsigned j = 0; j < unit_count; ++j)
                if (units[j].alive && units[j].owner == ORACLE_SIDE) {
                    int dx = units[i].x - units[j].x;
                    int dy = units[i].y - units[j].y;
                    if (dx * dx + dy * dy <= 32 * 32) {
                        units[i].owner = ORACLE_SIDE;
                        units[i].selected = 0;
                        units[i].target = -1;
                        units[i].order = ORACLE_IDLE;
                        units[i].tx = units[i].x;
                        units[i].ty = units[i].y;
                        changed = 1;
                        break;
                    }
                }
        }
    for (unsigned i = 0; i < building_count; ++i)
        if (buildings[i].alive && buildings[i].owner == ORACLE_RESCUABLE)
            for (unsigned j = 0; j < unit_count; ++j)
                if (units[j].alive && units[j].owner == ORACLE_SIDE) {
                    int dx = buildings[i].x - units[j].x;
                    int dy = buildings[i].y - units[j].y;
                    if (dx * dx + dy * dy <= 32 * 32) {
                        buildings[i].owner = ORACLE_SIDE;
                        changed = 1;
                        break;
                    }
                }
    return changed;
}
