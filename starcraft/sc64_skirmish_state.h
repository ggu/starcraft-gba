/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SKIRMISH_STATE_H
#define SC64_SKIRMISH_STATE_H

#ifdef SC64_SHARED_TRANSPORT
_Static_assert(MAX_BUILDINGS <= 255, "transport research producer index exceeds one byte");
static Sc64SkTransportTech race_skirmish_transport_tech;
static Sc64SkTransportTech race_skirmish_transport_candidate;
#ifdef SC64_NATIVE_SKIRMISH_ELIMINATION
static u8 race_skirmish_neutral_ventral;
static u8 race_skirmish_neutral_ventral_candidate;
#endif
#endif
static int race_skirmish_map;
static int race_bot_races[3]={R_ZERG,R_PROTOSS,R_TER};
#ifdef SC64_IMPORTED_SKIRMISH
static unsigned race_skirmish_slots; /* 0 keeps the legacy dense seating */
static unsigned race_skirmish_start_order; /* 0 keeps the legacy physical starts */
#ifdef SC64_NATIVE_MELEE_STARTS
static unsigned race_skirmish_melee; /* CPU players allied in the native Blood Bath preview */
#ifdef SC64_NATIVE_SKIRMISH_ELIMINATION
static u8 race_skirmish_eliminated; /* mask of occupied CPU ranks, not lobby rows */
#endif
#endif
#endif

static inline void race_skirmish_reset_elimination(void)
{
#ifdef SC64_NATIVE_SKIRMISH_ELIMINATION
    race_skirmish_eliminated=0;
#ifdef SC64_SHARED_TRANSPORT
    race_skirmish_neutral_ventral=0;
#endif
#endif
}

#endif
