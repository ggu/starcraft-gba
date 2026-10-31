/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_battlecruiser_rules.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    Sc64BattlecruiserProduction p = {1,1,1,2,2,1,1,2,1};
    assert(!sc64_battlecruiser_production_ready(0));
    assert(sc64_battlecruiser_production_ready(&p));
#define DENY(field, value) do { int saved = p.field; p.field = (value); \
    assert(!sc64_battlecruiser_production_ready(&p)); p.field = saved; } while (0)
    DENY(starport_alive, 0); DENY(starport_complete, 0);
    DENY(starport_grounded, 0); DENY(requested_owner, -1);
    DENY(starport_owner, 1); DENY(attached_tower_alive, 0);
    DENY(attached_tower_complete, 0); DENY(attached_tower_owner, 1);
    DENY(physics_lab_complete_owned, 0);
#undef DENY
    for (int mask = 0; mask < 64; ++mask) {
        p.starport_alive = !!(mask & 1); p.starport_complete = !!(mask & 2);
        p.starport_grounded = !!(mask & 4); p.attached_tower_alive = !!(mask & 8);
        p.attached_tower_complete = !!(mask & 16);
        p.physics_lab_complete_owned = !!(mask & 32);
        assert(sc64_battlecruiser_production_ready(&p) == (mask == 63));
    }
    assert(sc64_battlecruiser_energy_initial(0) == 12800);
    assert(sc64_battlecruiser_energy_initial(1) == 16000);
    for (int reactor = 0; reactor <= 1; ++reactor) {
        int cap = sc64_battlecruiser_energy_cap(reactor);
        for (int start_phase = 0; start_phase < 5; ++start_phase) {
            int energy = sc64_battlecruiser_energy_initial(reactor);
            int expected_q8 = energy;
            unsigned saved_phase = (unsigned)start_phase;
            for (int tick = 0; tick < 10000; ++tick) {
                unsigned next = saved_phase + 4;
                int due = next >= 5;
                saved_phase = due ? next - 5 : next;
                assert(due == (saved_phase != 4));
                assert(sc64_battlecruiser_energy_tick(&energy, reactor, due));
                if (due && expected_q8 < cap) expected_q8 += 8;
                assert(energy == expected_q8);
            }
        }
        /* Test a source-style half-energy effect at every Q8 parity, EMP
         * clearing, and payment at the exact integer threshold. */
        for (int prior = 0; prior <= cap; ++prior) {
            int half = prior / 2;
            assert(sc64_battlecruiser_energy_tick(&half, reactor, 0));
            assert(half == prior / 2);
        }
        int empty = 0;
        assert(sc64_battlecruiser_energy_tick(&empty, reactor, 0) && empty == 0);
        assert(!sc64_battlecruiser_energy_pay(&empty, reactor, 150));
        int v = cap;
        assert(sc64_battlecruiser_energy_pay(&v, reactor, 150));
        assert(v == cap - 150 * 256);
        v = 150 * 256 - 1;
        assert(!sc64_battlecruiser_energy_pay(&v, reactor, 150));
        assert(v == 150 * 256 - 1);
        v = cap + 1;
        assert(!sc64_battlecruiser_energy_tick(&v, reactor, 1) && v == cap + 1);
    }
    for (unsigned original = 0; original <= 65535; ++original) {
        for (unsigned owner = 0; owner < 8; ++owner) {
            for (unsigned tech = 1; tech <= 2; ++tech) {
                unsigned bits = original, mask = tech << (2 * owner);
                assert(sc64_battlecruiser_tech_has(bits, owner, tech) == !!(bits & mask));
                assert(sc64_battlecruiser_tech_complete(&bits, owner, tech));
                assert(bits == (original | mask));
            }
        }
    }
    unsigned invalid = 0x1234;
    assert(!sc64_battlecruiser_tech_complete(&invalid, 8, 1));
    assert(!sc64_battlecruiser_tech_complete(&invalid, 0, 3));
    assert(!sc64_battlecruiser_tech_complete(0, 0, 1));
    assert(invalid == 0x1234);
    for (unsigned kind = 1; kind <= 2; ++kind) {
        Sc64BattlecruiserResearch research = {0,0};
        unsigned bits = 0; int minerals = 500, gas = 500;
        assert(!sc64_battlecruiser_research_start(&research,kind,3,bits,1,1,&minerals,&gas));
        assert(minerals == 500 && gas == 500);
        assert(sc64_battlecruiser_research_start(&research,kind,3,bits,1,0,&minerals,&gas));
        unsigned duration = sc64_battlecruiser_research_duration(kind);
        for (unsigned tick = 0; tick <= duration; ++tick) {
            assert(!sc64_battlecruiser_tech_has(bits,3,kind));
            assert(sc64_battlecruiser_research_tick(&research,3,&bits,1));
        }
        assert(!research.kind && !research.remaining);
        assert(sc64_battlecruiser_tech_has(bits,3,kind));
        assert(!sc64_battlecruiser_tech_has(bits,2,kind));
        assert(!sc64_battlecruiser_research_start(&research,kind,3,bits,1,0,&minerals,&gas));
        bits = 0; minerals = gas = 500;
        assert(sc64_battlecruiser_research_start(&research,kind,3,bits,1,0,&minerals,&gas));
        assert(sc64_battlecruiser_research_cancel(&research,0,&minerals,&gas));
        assert(minerals == 500 && gas == 500);
        assert(sc64_battlecruiser_research_start(&research,kind,3,bits,1,0,&minerals,&gas));
        assert(sc64_battlecruiser_research_cancel(&research,1,&minerals,&gas));
        assert(minerals == 500 - (int)sc64_battlecruiser_research_cost(kind) +
            (int)(sc64_battlecruiser_research_cost(kind) * 3 / 4));
        assert(gas == minerals && bits == 0);
    }
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    Sc64BcYamatoShot shots[SC64_BC_SHOT_LIMIT]={{0}};
    assert(sc64_bc_shot_reserve(shots,1,5)==0);
    assert(sc64_bc_shot_reserve(shots,2,6)==1);
    assert(sc64_bc_shot_reserve(shots,3,7)<0);
    assert(sc64_bc_shot_reserve(shots,1,7)<0);
    shots[0].state=SC64_BC_SHOT_CHARGING;
    shots[1].state=SC64_BC_SHOT_FLIGHT;
    sc64_bc_shot_caster_gone(shots,1); sc64_bc_shot_caster_gone(shots,2);
    assert(!shots[0].state && shots[1].state==SC64_BC_SHOT_FLIGHT &&
           shots[1].caster==SC64_BC_SHOT_NO_CASTER);
    sc64_bc_shot_target_gone(shots,6);
    assert(shots[1].state==SC64_BC_SHOT_FLIGHT &&
           shots[1].target==SC64_BC_SHOT_NO_TARGET);
    shots[0]=(Sc64BcYamatoShot){0}; shots[1]=(Sc64BcYamatoShot){0};
    assert(sc64_bc_shot_reserve(shots,1,5)==0);
    shots[0].state=SC64_BC_SHOT_FLIGHT;
    assert(sc64_bc_shot_reserve(shots,1,6)==1); /* same caster's prior missile survives */
#endif
    puts("battlecruiser prerequisite predicate: 64 combinations and owner checks pass");
    return 0;
}
