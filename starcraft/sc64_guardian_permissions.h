/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_PERMISSIONS_H
#define SC64_GUARDIAN_PERMISSIONS_H
#ifdef SC64_EARLY_ZERG_PERMISSIONS
#include "sc64_early_zerg_permissions.h"
/* Z1-Z3 use the source map descriptor's already-normalized live owner map.
 * Z4 has an explicitly authored source mapping. Raw CHK SIDE is not an
 * engine race enum and is deliberately not interpreted here. */
static int race_guardian_early_source_owner(int mission,int source)
{
    if ((unsigned)source>=8u || (unsigned)mission>=SC64_RACE_MAP_COUNT ||
        !sc64_early_zerg_permission_source((unsigned)mission)) return -1;
    if (mission==15) {
        if (source==1) return ENEMY;
        if (source==2) return 4;
        if (source==3) return SIDE;
        return -1;
    }
    const Sc64RaceMap *map=&sc64_race_maps[mission];
    if (!map->owner_modes || !map->units) return -1;
    return map->owner_modes[source];
}
#endif
/* PUNI is source-slot scoped: validate that provenance before reading the
 * authenticated source table. A missing profile is denied, never defaulted. */
static int race_guardian_campaign_puni(const unsigned char *puni,int source,int raw)
{
    if (!puni || (unsigned)source>=12u || (unsigned)raw>=228u) return 0;
    unsigned cell=(unsigned)source*228u+(unsigned)raw;
    return puni[2964u+cell] ? puni[2736u+(unsigned)raw]!=0 : puni[cell]!=0;
}

static int race_guardian_campaign_allowed(int owner,int source,int raw)
{
    if (race_mode || (unsigned)owner>=5u || (unsigned)source>=8u || (unsigned)raw>=228u) return 0;
#ifdef SC64_EARLY_ZERG_PERMISSIONS
    if (sc64_early_zerg_permission_source((unsigned)race_selected_mission)) {
        int mapped=race_guardian_early_source_owner(race_selected_mission,source);
        if (mapped!=owner) return 0; /* also checks immutable map bounds */
        const Sc64RaceMap *map=&sc64_race_maps[race_selected_mission];
        return map->owner_races && map->owner_races[source]==R_ZERG &&
            sc64_early_zerg_permission_available((unsigned)race_selected_mission,(unsigned)source,(unsigned)raw);
    }
#endif
#ifdef SC64_Z8_RUNTIME
    if (race_z8_active()) return race_z8_guardian_product_allowed(owner,source,raw);
#endif
#ifdef SC64_Z9_RUNTIME
    if (race_zl_active()) return race_zl_guardian_product_allowed(owner,source,raw);
#endif
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) return race_z7_source_owner((unsigned)source)==owner &&
        race_guardian_campaign_puni(sc64_z7_source_puni,source,raw);
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) return race_z6_source_owner((unsigned)source)==owner &&
        race_guardian_campaign_puni(sc64_z6_source_puni,source,raw);
#endif
#ifdef SC64_Z5_RUNTIME
    if (race_z5_active()) return race_z5_source_owner((unsigned)source)==owner &&
        race_guardian_campaign_puni(sc64_z5_source_puni,source,raw);
#endif
    return 0;
}
#endif
