/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_EARLY_ZERG_PERMISSIONS_H
#define SC64_EARLY_ZERG_PERMISSIONS_H
#ifndef SC64_EARLY_ZERG_PERMISSIONS
#error "Early Zerg permissions require explicitly prepared private inputs"
#endif
#include "sc64_early_zerg_permissions_generated.h"
/* This reads source PUNI only. Callers must separately validate source-owner
 * mapping and the saved/current controller. source_race stores raw CHK SIDE
 * values, not the engine's R_* enumeration. No live world is read here. */
static const Sc64EarlyZergPermission *
sc64_early_zerg_permission_source(unsigned mission)
{
    for (unsigned i=0; i<4u; ++i)
        if (sc64_early_zerg_permissions[i].mission==mission)
            return &sc64_early_zerg_permissions[i];
    return 0;
}
static int
sc64_early_zerg_permission_available(unsigned mission,unsigned source_slot,unsigned raw)
{
    if (source_slot>=12u || raw>=228u) return 0;
    const Sc64EarlyZergPermission *source=sc64_early_zerg_permission_source(mission);
    if (!source) return 0;
    unsigned bit=source_slot*228u+raw;
    return (source->bits[bit >> 3] >> (bit & 7u)) & 1u;
}
#endif
