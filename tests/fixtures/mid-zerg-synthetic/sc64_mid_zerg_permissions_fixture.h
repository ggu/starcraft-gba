/* SPDX-License-Identifier: GPL-3.0-or-later
 * Synthetic test-only policy; no game-derived permission data. */
#ifndef SC64_MID_ZERG_PERMISSIONS_GENERATED_H
#define SC64_MID_ZERG_PERMISSIONS_GENERATED_H
typedef struct { unsigned char mission, owner_kind[12], source_race[12], bits[342]; } Sc64MidZergPermission;
static const Sc64MidZergPermission sc64_mid_zerg_permissions[2] = {
    { .mission=16 },
    /* Synthetic Z6 source 3 allows raw 37 and denies raw 41. */
    { .mission=17, .bits={[90]=2} }
};
#endif
