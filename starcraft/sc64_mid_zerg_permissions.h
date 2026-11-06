/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_MID_ZERG_PERMISSIONS_H
#define SC64_MID_ZERG_PERMISSIONS_H
#ifndef SC64_MID_ZERG_PERMISSIONS
#error "Mid Zerg permissions require explicitly prepared private inputs"
#endif
#include "sc64_mid_zerg_permissions_generated.h"
/* This is immutable source PUNI, not a current-controller binding. */
static int sc64_mid_zerg_permission_available(unsigned mission,unsigned source,unsigned raw)
{
    if (source>=12u || raw>=228u) return 0;
    for (unsigned i=0;i<2u;++i) if (sc64_mid_zerg_permissions[i].mission==mission) {
        unsigned bit=source*228u+raw;
        return (sc64_mid_zerg_permissions[i].bits[bit>>3]>>(bit&7u))&1u;
    }
    return 0;
}
#endif
