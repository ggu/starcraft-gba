/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CUSTOM_LARVA_EGGS_H
#define SC64_CUSTOM_LARVA_EGGS_H

/* Custom Game Zerg production: Larva at every Hatchery, Lair and Hive, and one
 * independently timed Egg per order, for the human and for computer opponents.
 *
 * SC64_CUSTOM_LARVA_EGGS is part of SC64_SHARED_ENGINE_FLAGS, so every
 * race-engine build receives it, and so does every tool that derives tables
 * from the unit roster. It takes effect in each build that has Custom Games
 * (SC64_IMPORTED_SKIRMISH) in the shared transport save envelope, which is
 * every delivered cartridge (Terran, Z7 to Z10, P7, P10) and Showcase. The
 * older chapter profiles (T3, Z3, P3, Z4, P6 and the full-Terran and Zerg
 * canonical profiles) have no such envelope: their records are raw word rows
 * with no Egg admission, so they keep the serial Hatchery queue. It selects
 * the existing shared Egg
 * producer and the Egg-aware unit codecs. It does not select a save journal:
 * a Custom Game record may hold Egg rows under the product's own tags, and a
 * campaign record may not (see race_load_preflight_candidate). Campaign
 * missions keep their own production: the producer below is active only
 * while race_mode is set, and Z7 keeps its mission Eggs.
 *
 * The explicit staged Egg, Guardian and Scarab-namespace experiments own
 * their own identities and journals, so the capability stays off in them. */
#if defined(SC64_CUSTOM_LARVA_EGGS) && defined(SC64_RACES) && \
    defined(SC64_IMPORTED_SKIRMISH) && defined(SC64_SHARED_SCOURGE_COMBAT) && \
    defined(SC64_SHARED_TRANSPORT) && \
    !defined(SC64_SHARED_EGG_CUSTOM_SERVICE) && !defined(SC64_SHARED_EGG_LATE_SERVICE) && \
    !defined(SC64_SHARED_EGG_HOST_STAGE) && !defined(SC64_SHARED_EGG_JOURNAL_HOST_STAGE) && \
    !defined(SC64_SHARED_EGG_WIRE_HOST_STAGE) && !defined(SC64_SHARED_EGG_PRODUCER_HOST_STAGE) && \
    !defined(SC64_GUARDIAN_COCOON_STAGE) && !defined(SC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE) && \
    !defined(SC64_SCARAB_NAMESPACE_STAGE) && !defined(SC64_COMPOUND_SCARAB_CONTEXT_HOST_STAGE) && \
    !defined(SC64_COMPOUND_FLASH_Z10_HOST_STAGE)
#define SC64_CUSTOM_EGGS_ACTIVE 1
#define SC64_SHARED_EGG_WIRE_HOST_STAGE 1
#define SC64_SHARED_EGG_PRODUCER_HOST_STAGE 1
#endif

/* Larva: one per LARVA_PERIOD updates at each completed town hall while it
 * holds fewer than LARVA_CAP. The pinned OpenBW revision reloads a timer of
 * 37 on a secondary order that runs every ninth frame and stops at three
 * connected larvae (bwgame.h 8273-8289): 38 * 9 = 342 frames, which is 428
 * updates at this engine's 5:4 time scale. */
#define SC64_LARVA_CAP 3
#define SC64_LARVA_PERIOD 428

/* Eggs stand in a row of fixed places across the hall that ordered them,
 * this far below the place its units appear at. */
#define SC64_EGG_PLACES 6
#define SC64_EGG_ROW_DROP 10

#endif
