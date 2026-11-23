/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WORKER_CARGO_SAVE_H
#define SC64_WORKER_CARGO_SAVE_H
#include "sc64_race_save_unit_edit.h"
#include "sc64_save_decode_context.h"
#ifndef SC64_WORKER_CARGO_KIND
#error "worker cargo save preparation needs explicit cargo semantics"
#endif

/* Normalize a private candidate before the ordinary mission validators.
 * A failed row may change the scratch candidate, never the live world. No
 * second full save or per-actor migration bank is allocated. */
static int
race_save_worker_cargo_prepare(RaceSave *candidate, unsigned width,
                                unsigned height, int legacy SC64_GCTX_DECL)
{
    if (!candidate || (legacy != 0 && legacy != 1)) return 0;
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit decoded;
        if (!SC64_GUARD_DECODE(candidate,i,width,height,&decoded)) return 0;
        if (legacy && !sc64_worker_cargo_migrate_legacy(&decoded)) return 0;
        if (!sc64_worker_cargo_state_valid(&decoded)) return 0;
        if (!legacy) continue;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
        if (!race_save_encode_unit(candidate,i,width,height,&decoded)) return 0;
#else
        /* Only flags change; preserve every historical word verbatim. */
        candidate->units[i][offsetof(Unit,flags)/sizeof(int)]=(s16)decoded.flags;
#endif
    }
    return 1;
}
#endif
