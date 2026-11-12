/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_ghost_tech.h"

static void check_jobs(void)
{
    const int expected[4][6] = {
        {100, 100, 1200, 252, 10, 25}, {200, 200, 1500, 240, 1, 100},
        {100, 100, 2500, 249, 20, 0}, {150, 150, 2500, 256, 21, 0}
    };
    Sc64GhostTechState state = {0};
    int ore = 1000, gas = 1000;
    for (unsigned kind = SC64_GHOST_TECH_CLOAK; kind <= SC64_GHOST_TECH_REACTOR; ++kind) {
        Sc64GhostTechJob lab = {0}, second_lab = {0};
        const Sc64GhostTechFact *fact = sc64_ghost_tech_fact(kind);
        const int *row = expected[kind - SC64_GHOST_TECH_CLOAK];
        assert(fact && fact->minerals == row[0] && fact->gas == row[1] &&
               fact->source_ticks == row[2] && fact->icon == row[3] &&
               fact->raw == row[4] && fact->energy == row[5] && fact->upgrade == (row[5] == 0));
        int prev_ore = ore, prev_gas = gas;
        assert(!sc64_ghost_tech_begin(&state, &lab, 1, kind, 0, 1, &ore, &gas));
        assert(!sc64_ghost_tech_begin(&state, &lab, 1, kind, SC64_GHOST_TECH_ALL, 0, &ore, &gas));
        assert(ore == prev_ore && gas == prev_gas && !state.pending && !lab.kind);
        assert(sc64_ghost_tech_begin(&state, &lab, 1, kind, SC64_GHOST_TECH_ALL, 1, &ore, &gas));
        assert(ore == prev_ore - row[0] && gas == prev_gas - row[1]);
        assert(!sc64_ghost_tech_begin(&state, &second_lab, 1, kind, SC64_GHOST_TECH_ALL, 1, &ore, &gas));
        assert(lab.remaining == fact->source_ticks && !sc64_ghost_tech_has(&state, 1, kind));
        assert(!sc64_ghost_tech_seed(&state, 1, 1u << (kind - 1u)));

        assert(sc64_ghost_tech_tick(&state, &lab, 1, 1, 0) == SC64_GHOST_JOB_WAITING);
        for (int i = 0; i < row[2] / 2; ++i)
            assert(sc64_ghost_tech_tick(&state, &lab, 1, 1, 1) == SC64_GHOST_JOB_PROGRESS);
        int remaining = lab.remaining;
        for (int i = 0; i < row[2]; ++i) {
            assert(sc64_ghost_tech_tick(&state, &lab, 1, 0, 1) == SC64_GHOST_JOB_WAITING);
            assert(sc64_ghost_tech_tick(&state, &lab, 6, 1, 1) == SC64_GHOST_JOB_WAITING);
        }
        assert(lab.remaining == remaining && !sc64_ghost_tech_has(&state, 1, kind));
        for (int i = 1; i < remaining; ++i)
            assert(sc64_ghost_tech_tick(&state, &lab, 1, 1, 1) == SC64_GHOST_JOB_PROGRESS);
        assert(sc64_ghost_tech_tick(&state, &lab, 1, 1, 1) == SC64_GHOST_JOB_COMPLETE);
        assert(sc64_ghost_tech_has(&state, 1, kind) && !sc64_ghost_tech_has(&state, 6, kind));
        assert(!state.pending && !lab.kind && !lab.payer && !lab.remaining);
        assert(!sc64_ghost_tech_begin(&state, &lab, 1, kind, SC64_GHOST_TECH_ALL, 1, &ore, &gas));
        assert(ore == prev_ore - row[0] && gas == prev_gas - row[1]);
    }
}

static void check_concurrent_wallets_and_refunds(void)
{
    Sc64GhostTechState state = {0};
    Sc64GhostTechJob a = {0}, b = {0}, c = {0};
    int ore0 = 400, gas0 = 400, ore7 = 100, gas7 = 99;
    unsigned any = SC64_GHOST_TECH_ALL, cloak = SC64_GHOST_TECH_CLOAK, lock = SC64_GHOST_TECH_LOCKDOWN;
    assert(sc64_ghost_tech_begin(&state, &a, 0, cloak, any, 1, &ore0, &gas0));
    assert(sc64_ghost_tech_begin(&state, &b, 0, lock, any, 1, &ore0, &gas0));
    assert(!sc64_ghost_tech_begin(&state, &b, 0, cloak, any, 1, &ore0, &gas0));
    assert(!sc64_ghost_tech_begin(&state, &c, 7, cloak, any, 1, &ore7, &gas7));
    assert(ore7 == 100 && gas7 == 99 && !c.kind);
    ++gas7;
    assert(sc64_ghost_tech_begin(&state, &c, 7, cloak, any, 1, &ore7, &gas7));
    assert(!ore7 && !gas7 && ore0 == 100 && gas0 == 100);
    assert(sc64_ghost_tech_tick(&state, &a, 0, 1, 1) == SC64_GHOST_JOB_PROGRESS);
    assert(!sc64_ghost_tech_cancel(&state, &a, 7, 0, &ore7, &gas7));
    assert(!ore7 && !gas7 && a.kind);
    assert(sc64_ghost_tech_cancel(&state, &a, 0, 0, &ore0, &gas0));
    assert(ore0 == 200 && gas0 == 200 && !a.kind);
    assert(sc64_ghost_tech_cancel(&state, &b, 0, 1, &ore0, &gas0));
    assert(ore0 == 350 && gas0 == 350 && !b.kind);
    assert(state.pending == sc64_ghost_tech_bit(7, cloak));
    assert(sc64_ghost_tech_cancel(&state, &c, 7, 1, &ore7, &gas7));
    assert(ore7 == 75 && gas7 == 75 && !state.pending && !state.completed);
    assert(!sc64_ghost_tech_cancel(&state, &c, 7, 1, &ore7, &gas7));
    assert(!sc64_ghost_tech_bit(8, cloak) && !sc64_ghost_tech_bit(1, 5));
}

static void check_effects(void)
{
    Sc64GhostTechState state = {0};
    assert(sc64_ghost_sight_source_pixels(&state, 0) == 9 * 32);
    assert(sc64_ghost_energy_initial(&state, 0, -1) == 50 * 256);
    assert(sc64_ghost_energy_initial(&state, 0, 100) == 200 * 256); /* T8 placements. */
    assert(sc64_ghost_energy_initial(&state, 0, 101) < 0);
    assert(sc64_ghost_energy_initial(&state, 8, -1) < 0);
    assert(sc64_ghost_tech_seed(&state, 7, SC64_GHOST_TECH_ALL));
    assert(sc64_ghost_tech_seed(&state, 7, SC64_GHOST_TECH_ALL));
    assert(sc64_ghost_energy_cap(&state, 0) == 200 * 256);
    assert(sc64_ghost_energy_cap(&state, 7) == 250 * 256);
    assert(sc64_ghost_energy_initial(&state, 7, -1) == 625 * 256 / 10);
    assert(sc64_ghost_energy_initial(&state, 7, 100) == 250 * 256);
    assert(sc64_ghost_sight_source_pixels(&state, 7) == 11 * 32);
    assert(sc64_ghost_sight_source_pixels(&state, 0) == 9 * 32);
    int energy = 125 * 256;
    assert(!sc64_ghost_energy_pay(&state, 0, SC64_GHOST_TECH_CLOAK, &energy));
    assert(!sc64_ghost_energy_pay(&state, 7, SC64_GHOST_TECH_OCULAR, &energy));
    assert(energy == 125 * 256);
    assert(sc64_ghost_energy_pay(&state, 7, SC64_GHOST_TECH_CLOAK, &energy));
    assert(energy == 100 * 256);
    assert(sc64_ghost_energy_pay(&state, 7, SC64_GHOST_TECH_LOCKDOWN, &energy));
    assert(!energy && !sc64_ghost_energy_pay(&state, 7, SC64_GHOST_TECH_LOCKDOWN, &energy));
    assert(sc64_ghost_energy_tick(&state, 7, &energy, 1, 0, 1) == SC64_GHOST_ENERGY_EXHAUSTED);
    energy = 10;
    assert(sc64_ghost_energy_tick(&state, 7, &energy, 1, 0, 0) == SC64_GHOST_ENERGY_IDLE);
    assert(energy == 10);
    assert(sc64_ghost_energy_tick(&state, 7, &energy, 1, 0, 1) == SC64_GHOST_ENERGY_UPDATED);
    assert(!energy);
    assert(sc64_ghost_energy_tick(&state, 7, &energy, 1, 1, 1) == SC64_GHOST_ENERGY_UPDATED);
    assert(energy == 8); /* Arbiter/passive cloak doesn't debit active drain. */
    energy = 250 * 256 - 1;
    assert(sc64_ghost_energy_tick(&state, 7, &energy, 0, 0, 1) == SC64_GHOST_ENERGY_UPDATED);
    assert(energy == 250 * 256);
    assert(sc64_ghost_energy_tick(&state, 7, &energy, 0, 0, 1) == SC64_GHOST_ENERGY_IDLE);
    Sc64GhostTechState malformed = {1, 1};
    assert(sc64_ghost_energy_tick(&malformed, 7, &energy, 1, 0, 1) == SC64_GHOST_ENERGY_ERROR);
    assert(energy == 250 * 256);
}

int main(void)
{
    check_jobs();
    check_concurrent_wallets_and_refunds();
    check_effects();
    puts("Covert Ops: source costs, paid timers, owner/attachment, cancellation, sight and Q8 energy pass");
    return 0;
}
