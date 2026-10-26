/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SC64_FACT_TABLES oracle: compile with the target build's actor gates,
 * -DSC64_FACT_TABLES and the generated table directory. Every table-backed
 * lookup must equal its *_reference function for every in-range type, a
 * margin outside both rosters and the integer extremes, including the
 * derived collidable/movable/drawable/hidden/selectable policy tests. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "race_rules.h"
#include "sc64_game_actor_facts.h"

#ifndef SC64_FACT_TABLES
#error "Build this oracle with SC64_FACT_TABLES"
#endif

static unsigned long checked;

static void expect(int ok, const char *what, int type)
{
    ++checked;
    if (!ok) {
        fprintf(stderr, "fact table mismatch: %s type %d\n", what, type);
        exit(1);
    }
}

static int policy(unsigned flags, unsigned bit) { return !flags || (flags & bit); }

static void unit(int type)
{
    unsigned table = sc64_game_unit_actor_flags(type);
    unsigned reference = sc64_game_unit_actor_flags_reference(type);
    expect(table == reference, "unit flags", type);
    expect(sc64_game_unit_collision_radius(type) ==
           sc64_game_unit_collision_radius_reference(type), "radius", type);
    expect(sc64_race_unit_flying(type) == sc64_race_unit_flying_reference(type),
           "flying", type);
    expect(policy(table, SC64_GAME_ACTOR_COLLIDABLE) ==
           policy(reference, SC64_GAME_ACTOR_COLLIDABLE), "collidable", type);
    expect(policy(table, SC64_GAME_ACTOR_MOVABLE) ==
           policy(reference, SC64_GAME_ACTOR_MOVABLE), "movable", type);
    expect(policy(table, SC64_GAME_ACTOR_DRAWABLE) ==
           policy(reference, SC64_GAME_ACTOR_DRAWABLE), "drawable", type);
    expect(!!(table & SC64_GAME_ACTOR_HIDDEN) ==
           !!(reference & SC64_GAME_ACTOR_HIDDEN), "hidden", type);
    expect(sc64_game_unit_selectable(type) == policy(reference, SC64_GAME_ACTOR_SELECTABLE),
           "selectable", type);
}

static void building(int type)
{
    unsigned table = sc64_game_building_actor_flags(type);
    unsigned reference = sc64_game_building_actor_flags_reference(type);
    expect(table == reference, "building flags", type);
    expect(policy(table, SC64_GAME_ACTOR_COLLIDABLE) ==
           policy(reference, SC64_GAME_ACTOR_COLLIDABLE), "building collidable", type);
    expect(policy(table, SC64_GAME_ACTOR_DRAWABLE) ==
           policy(reference, SC64_GAME_ACTOR_DRAWABLE), "building drawable", type);
}

int main(void)
{
    int t, authored = 0;
    for (t = -64; t < SC_RU_COUNT + 64; ++t) unit(t);
    for (t = -64; t < SC_RB_COUNT + 64; ++t) building(t);
#ifndef SC64_TERRAN_LATE_ACTORS
    /* The late-Terran reference lookup subtracts an enum base from the type
       before its range check, which overflows at INT_MIN. */
    unit(INT_MIN); unit(INT_MAX); building(INT_MIN); building(INT_MAX);
#endif
    for (t = 0; t < SC_RU_COUNT; ++t) authored += sc64_game_unit_actor_flags(t) != 0;
    printf("fact tables exact: %d unit types (%d authored), %d building types, %lu checks\n",
           (int)SC_RU_COUNT, authored, (int)SC_RB_COUNT, checked);
    return 0;
}
