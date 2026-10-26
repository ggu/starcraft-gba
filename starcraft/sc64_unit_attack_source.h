/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional source-authored full attack-Init body timeline bank. */
#ifndef SC64_UNIT_ATTACK_SOURCE_H
#define SC64_UNIT_ATTACK_SOURCE_H

#if defined(SC64_UNIT_ATTACK_POSES_ENABLED)
#include "sc64_unit_attack_source_generated.h"
#if !defined(SC64_UNIT_ATTACK_POSE_BANK_VERSION) || \
    SC64_UNIT_ATTACK_POSE_BANK_VERSION != 1 || \
    !defined(SC64_UNIT_ATTACK_POSE_METADATA)
#error "SC64_UNIT_ATTACK_POSES_ENABLED requires generated attack-pose metadata v1"
#endif
#else
static inline unsigned char
sc64_unit_attack_source_release_tick(unsigned short raw_type, int target_air)
{
    (void)raw_type; (void)target_air;
    return 0xffu;
}

static inline unsigned char
sc64_unit_attack_source_duration(unsigned short raw_type, int target_air)
{
    (void)raw_type; (void)target_air;
    return 0u;
}

static inline unsigned short
sc64_unit_attack_source_full_duration(unsigned short raw_type, int target_air)
{
    (void)raw_type; (void)target_air;
    return 0u;
}

static inline const Sc64SpriteFrame *
sc64_unit_attack_source_frame(unsigned short raw_type, int target_air,
                              unsigned char heading32,
                              unsigned int post_release_tick)
{
    (void)raw_type; (void)target_air; (void)heading32;
    (void)post_release_tick;
    return 0;
}

static inline const Sc64SpriteFrame *
sc64_unit_attack_source_full_frame(unsigned short raw_type, int target_air,
                                   unsigned char heading32,
                                   unsigned int attack_tick)
{
    (void)raw_type; (void)target_air; (void)heading32; (void)attack_tick;
    return 0;
}
#endif

#endif
