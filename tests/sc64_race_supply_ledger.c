/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/race_rules.h"
#include "../starcraft/sc64_supply.h"

/* Only the ownership and production fields the shared tally reads. */
typedef struct { int alive, owner, type; } Unit;
typedef struct {
    int alive, owner, type, under_construction, queue_count, queue_head, queue[5];
} Building;
enum { MAX_UNITS = 5, MAX_BUILDINGS = 5 };
static int race_mode;
static Unit u[MAX_UNITS];
static Building b[MAX_BUILDINGS];
#define unit_slot_supply(q) sc64_race_unit_rule((q)->type)->supply
#include "../starcraft/sc64_race_supply.h"

int main(void)
{
    u[0] = (Unit){1, 0, U_PROBE};
    u[1] = (Unit){1, 3, U_TANK};     /* Pending rescue. */
    u[2] = (Unit){1, 0, U_MUTALISK};
    u[3] = (Unit){1, 0, U_OVERLORD};
    u[4] = (Unit){0, 0, U_OVERLORD}; /* Dead provider. */
    b[0] = (Building){1, 0, B_NEXUS, 0, 1, 4, {0, 0, 0, 0, U_PROBE}};
    b[1] = (Building){1, 3, B_COMMAND, 0, 2, 4, {U_SCV, 0, 0, 0, U_SCV}};
    b[2] = (Building){1, 0, B_PYLON, 1, 0, 0, {0}};
    b[3] = (Building){1, 0, B_LAIR, 1, 1, 0, {U_ZERGLING}};
    b[4] = (Building){0, 0, B_PYLON, 0, 0, 0, {0}};

    Sc64RaceSupplyLedger protoss = race_owner_supply_ledger(0, R_PROTOSS);
    assert(protoss.used == 2 && protoss.queued == 2 && protoss.provided == 18);
    assert(race_supply_ledger_reserved(race_owner_supply_ledger(0, R_TER)) == 0);
    Sc64RaceSupplyLedger terran = race_owner_supply_ledger(3, R_TER);
    assert(terran.used == 4 && terran.queued == 4 && terran.provided == 20);
    u[1].owner = b[1].owner = 0;
    terran = race_owner_supply_ledger(0, R_TER);
    assert(terran.used == 4 && terran.queued == 4 && terran.provided == 20);
    assert(race_owner_supply_ledger(3, R_TER).provided == 0);

    Sc64RaceSupplyLedger zerg = race_owner_supply_ledger(0, R_ZERG);
    assert(zerg.used == 4 && zerg.queued == 2 && zerg.provided == 16);
    race_mode = 1;
    zerg = race_owner_supply_ledger(0, R_ZERG);
    assert(zerg.used == 4 && zerg.queued == 2 && zerg.provided == 18);
    u[3].alive = 0;
    zerg = race_owner_supply_ledger(0, R_ZERG);
    int reserved = race_supply_ledger_reserved(zerg);
    assert(zerg.provided == 2 && reserved == 6);
    assert(!sc64_supply_allows(reserved, 1, zerg.provided));
    assert(sc64_supply_allows(reserved, 0, zerg.provided));
    assert(sc64_supply_allows(race_supply_ledger_reserved(terran), 2, terran.provided));
    assert(sc64_supply_allows(race_supply_ledger_reserved(protoss), 2, protoss.provided));
    assert(!race_owner_supply_ledger(-1, R_ZERG).provided);
    assert(!race_owner_supply_ledger(0, -1).used);
    puts("Separate Terran, Zerg and Protoss supply follows current owners and paid products.");
    return 0;
}
