/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_BUILDING_ART_H
#define SC64_NUKE_BUILDING_ART_H
#include "sc64_nuke_building_source_generated.h"

/* Built and AlmostBuilt are the two body frames. Landing and IsWorking
 * are separate indexed overlays drawn at the same world anchor. */
static inline const Sc64SpriteFrame *
sc64_nuke_building_sprite(unsigned raw, unsigned almost_built)
{
    if (almost_built > 1) return 0;
    if (raw == 108) return &sc64_nuke_building_silo_body[almost_built];
    if (raw == 117) return &sc64_nuke_building_covert_body[almost_built];
    return 0;
}

static inline const Sc64SpriteFrame *
sc64_nuke_building_landing_sprite(unsigned raw, unsigned source_tick)
{
    unsigned frame = source_tick / SC64_NUKE_BUILDING_LANDING_HOLD;
    if (frame > 4) frame = 4;
    if (raw == 108) return &sc64_nuke_building_silo_landing[frame];
    if (raw == 117) return &sc64_nuke_building_covert_landing[frame];
    return 0;
}

static inline const Sc64SpriteFrame *
sc64_nuke_building_working_sprite(unsigned raw, unsigned source_tick)
{
    if (source_tick % (2u * SC64_NUKE_BUILDING_WORKING_HOLD) >=
        SC64_NUKE_BUILDING_WORKING_HOLD) return 0;
    if (raw == 108) return &sc64_nuke_building_silo_working[0];
    if (raw == 117) return &sc64_nuke_building_covert_working[0];
    return 0;
}
#endif
