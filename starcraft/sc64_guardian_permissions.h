/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GUARDIAN_PERMISSIONS_H
#define SC64_GUARDIAN_PERMISSIONS_H
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
