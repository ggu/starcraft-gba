/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GHOST_TECH_HOOKS_H
#define SC64_GHOST_TECH_HOOKS_H
#include "sc64_ghost_tech.h"

enum { SC64_GHOST_MENU_CANCEL = -1, SC64_GHOST_OCULAR_SIGHT_KEY = -101 };
static void race_ghost_tech_reset(void), race_ghost_tech_tick(void);
static void race_ghost_tech_destroy(int slot);
static int race_ghost_tech_save_safe(void), race_ghost_tech_menu_width(void);
static int race_ghost_tech_command_index(void), race_ghost_tech_menu_kind(int row);
static int race_ghost_tech_menu_choose(int kind);
static const char *race_ghost_tech_menu_label(int kind);
static void race_ghost_birth(Unit *q, int placed);
static void race_ghost_tick_energy(Unit *q);
static int race_ghost_max_energy(const Unit *q), race_ghost_sight(const Unit *q);
#endif
