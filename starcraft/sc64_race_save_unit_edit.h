/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RACE_SAVE_UNIT_EDIT_H
#define SC64_RACE_SAVE_UNIT_EDIT_H

/* Re-encode a row in its existing payload layout after a detached migration. */
static inline int
race_save_encode_unit(RaceSave *candidate, unsigned slot, unsigned width,
                      unsigned height, const Unit *unit)
{
    if (!candidate || !unit || slot >= MAX_UNITS) return 0;
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_encode(&candidate->units[slot], unit, width, height);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_unit_encode(&candidate->units[slot], unit, width, height);
#else
    return sc64_zp_unit_save_encode(&candidate->units[slot], unit, width, height);
#endif
#elif defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return sc64_unit_save_encode(&candidate->units[slot], unit, width, height);
#else
    (void)width;
    (void)height;
    for (unsigned word = 0; word < RACE_UNIT_WORDS; ++word) {
        int value;
        memcpy(&value, (const u8 *)unit + word * sizeof(value), sizeof(value));
        candidate->units[slot][word] = (s16)value;
    }
    return 1;
#endif
}

#endif
