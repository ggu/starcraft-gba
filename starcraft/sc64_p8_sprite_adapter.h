/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_SPRITE_ADAPTER_H
#define SC64_P8_SPRITE_ADAPTER_H

/* Native raw12 and raw28 resolve through the same N64 flingy/sprite/image/
 * GRP chain. Reuse the existing canonical raw12 descriptor without changing
 * logical actor identity. This does not imply raw12 gameplay equivalence. */
static inline const Sc64SpriteFrame *
sc64_p8_unit_sprite_for_raw(unsigned short raw_type)
{
#ifdef SC64_P8_DATA_ENABLED
    if (raw_type == 28u) raw_type = 12u;
#endif
    return sc64_race_unit_sprite(raw_type);
}

#endif
