/* SPDX-License-Identifier: GPL-3.0-or-later
 * Synthetic test-only policy; no game-derived permission data. */
#ifndef SC64_EARLY_ZERG_PERMISSIONS_GENERATED_H
#define SC64_EARLY_ZERG_PERMISSIONS_GENERATED_H
typedef struct { unsigned char mission, owner_kind[12], source_race[12], bits[342]; } Sc64EarlyZergPermission;
static const Sc64EarlyZergPermission sc64_early_zerg_permissions[4] = {
    { .mission=0 }, { .mission=1 }, { .mission=14 },
    /* Synthetic Z4 source 3 allows raw 37 and denies raw 41. */
    { .mission=15, .bits={[90]=2} }
};
#endif
