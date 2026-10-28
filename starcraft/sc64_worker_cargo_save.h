/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WORKER_CARGO_SAVE_H
#define SC64_WORKER_CARGO_SAVE_H
#ifndef SC64_WORKER_CARGO_KIND
#error "worker cargo save preparation needs explicit cargo semantics"
#endif

/* Normalize a private candidate before the ordinary mission validators.
 * A failed row may change the scratch candidate, never the live world. No
 * second full save or per-actor migration bank is allocated. */
static int
race_save_worker_cargo_prepare(RaceSave *candidate, unsigned width,
                                unsigned height, int legacy)
{
    if (!candidate || (legacy != 0 && legacy != 1)) return 0;
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit decoded;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
        if (!race_save_decode_unit(candidate,i,width,height,&decoded)) return 0;
#else
        (void)width; (void)height;
        for (unsigned j=0;j<RACE_UNIT_WORDS;++j) {
            int value=j==offsetof(Unit,flags)/sizeof(int)
                ? (u16)candidate->units[i][j] : candidate->units[i][j];
            memcpy((u8 *)&decoded+j*sizeof(int),&value,sizeof(value));
        }
#endif
        if (legacy && !sc64_worker_cargo_migrate_legacy(&decoded)) return 0;
        if (!sc64_worker_cargo_state_valid(&decoded)) return 0;
        if (!legacy) continue;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
        if (!sc64_p7_typed_unit_encode(&candidate->units[i],&decoded,width,height)) return 0;
#elif defined(SC64_Z7_DERIVED_SAVE)
        if (!sc64_derived_unit_encode(&candidate->units[i],&decoded,width,height)) return 0;
#else
        if (!sc64_zp_unit_save_encode(&candidate->units[i],&decoded,width,height)) return 0;
#endif
#else
        if (!sc64_unit_save_encode(&candidate->units[i],&decoded,width,height)) return 0;
#endif
#else
        /* Only flags change; preserve every historical word verbatim. */
        candidate->units[i][offsetof(Unit,flags)/sizeof(int)]=(s16)decoded.flags;
#endif
    }
    return 1;
}
#endif
