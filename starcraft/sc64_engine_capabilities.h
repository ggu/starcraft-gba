/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ENGINE_CAPABILITIES_H
#define SC64_ENGINE_CAPABILITIES_H

/* Mechanics belong to the engine, not the selected mission or menu.
 * This contract is checked by game.c and by probes of real Make expansions.
 * Storage placement, asset banks and mission objectives are separate concerns.
 * The small initial set does not claim all historical mission services have
 * already been made mode-independent; see docs/SHARED-ENGINE.md. */
#define SC64_ENGINE_CAP_CONSTRUCTION_HP 1u
#define SC64_ENGINE_CAP_SIEGE_SPLASH 2u
#define SC64_ENGINE_CAP_BATTERY 4u
#define SC64_ENGINE_CORE_EXPECTED 7u

#if defined(SC64_SHARED_BATTERY) || defined(SC64_P7_BATTERY_ENABLED)
#define SC64_BATTERY_SERVICE_ENABLED
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
#include "sc64_p7_battery_rules.h"
#endif

#ifdef SC64_CONSTRUCTION_HP
#define SC64_ENGINE_HAS_CONSTRUCTION_HP SC64_ENGINE_CAP_CONSTRUCTION_HP
#else
#define SC64_ENGINE_HAS_CONSTRUCTION_HP 0u
#endif
#ifdef SC64_SIEGE_SPLASH
#define SC64_ENGINE_HAS_SIEGE_SPLASH SC64_ENGINE_CAP_SIEGE_SPLASH
#else
#define SC64_ENGINE_HAS_SIEGE_SPLASH 0u
#endif
#ifdef SC64_SHARED_BATTERY
#define SC64_ENGINE_HAS_BATTERY SC64_ENGINE_CAP_BATTERY
#else
#define SC64_ENGINE_HAS_BATTERY 0u
#endif
#define SC64_ENGINE_CORE_CAPABILITIES \
    (SC64_ENGINE_HAS_CONSTRUCTION_HP | SC64_ENGINE_HAS_SIEGE_SPLASH | SC64_ENGINE_HAS_BATTERY)

#ifdef SC64_ENGINE_CORE_REQUIRED
#ifndef SC64_RACES
#error "Shared race-engine mechanics require SC64_RACES"
#endif
#if SC64_ENGINE_CORE_CAPABILITIES != SC64_ENGINE_CORE_EXPECTED
#error "Production race-engine builds must include every shared core mechanic"
#endif
#endif

#endif
