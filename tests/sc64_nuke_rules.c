/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_nuke_rules.h"

static void test_paid_arm_and_interrupt(void)
{
    Sc64NukeSilo silo = {0};
    Sc64NukeFlight flight = {0};
    const Sc64NukeCadence cadence = {5, 3, 2}; /* test clock, not game timing */
    int ore = 600, gas = 600;
    assert(!sc64_nuke_train(&silo, &ore, &gas, 385, 400, 3));
    assert(ore == 600 && gas == 600 && !sc64_nuke_silo_supply(&silo));
    gas = 199;
    assert(!sc64_nuke_train(&silo, &ore, &gas, 384, 400, 3));
    assert(ore == 600 && gas == 199 && !silo.remaining);
    gas = 600;
    assert(sc64_nuke_train(&silo, &ore, &gas, 384, 400, 3));
    assert(ore == 400 && gas == 400 && sc64_nuke_silo_supply(&silo) == 16);
    assert(!sc64_nuke_train(&silo, &ore, &gas, 0, 400, 3));
    assert(!sc64_nuke_launch(&silo, &flight, 1, 190, 100, 1024, cadence));
    assert(!sc64_nuke_train_step(&silo));
    assert(!sc64_nuke_train_step(&silo));
    assert(sc64_nuke_train_step(&silo) && silo.armed);
    assert(!sc64_nuke_train_step(&silo));
    assert(!sc64_nuke_launch(&silo, &flight, 1, 190, 100, 1024, (Sc64NukeCadence){3, 2, 4}));
    assert(sc64_nuke_launch(&silo, &flight, 1, 190, 100, 1024, cadence));
    assert(!sc64_nuke_silo_supply(&silo) && sc64_nuke_flight_supply(&flight) == 16);
    assert(flight.x == 100 && flight.y == 1024 && flight.spotter == 190 && flight.owner == 1);
    assert(!sc64_nuke_flight_step(&flight, 1));
    assert(sc64_nuke_flight_step(&flight, 1) == SC64_NUKE_EVENT_WARNING);
    assert(sc64_nuke_flight_step(&flight, 0) == SC64_NUKE_EVENT_ABORT);
    assert(!sc64_nuke_flight_supply(&flight));
    assert(!sc64_nuke_flight_step(&flight, 0));
    assert(ore == 400 && gas == 400 && !silo.armed);
}

static void test_detached_descent(void)
{
    Sc64NukeSilo silo = {0, 1};
    Sc64NukeFlight flight = {0};
    assert(sc64_nuke_launch(&silo, &flight, 0, 99, 20, 30, (Sc64NukeCadence){3, 2, 1}));
    assert(sc64_nuke_flight_step(&flight, 1) == SC64_NUKE_EVENT_WARNING);
    assert(!sc64_nuke_flight_step(&flight, 1));
    assert(sc64_nuke_flight_step(&flight, 1) == SC64_NUKE_EVENT_RELEASE);
    assert(flight.phase == SC64_NUKE_DESCENDING);
    assert(!sc64_nuke_flight_step(&flight, 0));
    assert(sc64_nuke_flight_step(&flight, 0) == SC64_NUKE_EVENT_IMPACT);
    assert(!sc64_nuke_flight_step(&flight, 0));
}

static void test_stock_blast(void)
{
    assert(sc64_nuke_source_distance(5, 6, 0, 0, 8, 8) == 0);
    assert(sc64_nuke_source_distance(136, 0, 0, 0, 8, 8) == 128);
    assert(sc64_nuke_source_distance(64, 64, 0, 0, 0, 0) == 83);
    assert(sc64_nuke_source_distance(-64, -64, 0, 0, 0, 0) == 83);
    assert(sc64_nuke_damage_packet_q8(600, 0, 128, 0) == 500u * 256u);
    assert(sc64_nuke_damage_packet_q8(750, 750, 128, 0) == 1000u * 256u);
    assert(sc64_nuke_damage_packet_q8(750, 750, 129, 0) == 500u * 256u);
    assert(sc64_nuke_damage_packet_q8(750, 750, 192, 0) == 500u * 256u);
    assert(sc64_nuke_damage_packet_q8(750, 750, 193, 0) == 250u * 256u);
    assert(sc64_nuke_damage_packet_q8(750, 750, 256, 0) == 250u * 256u);
    assert(!sc64_nuke_damage_packet_q8(750, 750, 257, 0));
    assert(sc64_nuke_damage_packet_q8(35, 0, 128, 1) == 500u * 256u);
    assert(!sc64_nuke_damage_packet_q8(35, 0, 129, 1));
    assert(sc64_nuke_damage_packet_q8(752, 0, 193, 0) == 501u * 64u);
}

int main(void)
{
    test_paid_arm_and_interrupt();
    test_detached_descent();
    test_stock_blast();
    puts("nuke admission, interruption, flight and blast checks passed");
    return 0;
}
