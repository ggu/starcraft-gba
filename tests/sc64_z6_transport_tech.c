#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_z6.h"
#include "../starcraft/sc64_z6_transport_tech.h"

enum { SIDE = 0, MAX_BUILDINGS = 4 };
typedef struct {
    int hp, owner, alive, under_construction, queue_count;
    int raw_type, raw_owner;
} Building;
typedef struct { Sc64Z6TransportTechState transport; } TestZ6Runtime;
static TestZ6Runtime race_z6;
static Building b[MAX_BUILDINGS];
static int selected_building = -1;
static int minerals, gas;
static int race_z6_active(void) { return 1; }
static int race_z6_source_owner(unsigned slot) { return slot == 3u ? SIDE : -1; }
#define SC64_Z6_RUNTIME 1
#define SC64_TRANSPORT_RUNTIME 1
#define SC64_Z6_ACTORS 1
#define SC64_Z6_TRANSPORT_ENABLED 1
#include "../starcraft/sc64_z6_transport_adapter.h"

static void test_facts_and_initial_state(void)
{
    Sc64Z6TransportTechState s = { 9, 2, 1 };
    assert(sc64_z6_transport_tech_facts_valid());
    assert(SC64_Z6_TRANSPORT_RESEARCH_TICKS == 3000u);
    assert(sc64_z6_transport_tech_init(&s));
    assert(s.ticks_remaining == 0 && s.producer_slot_plus1 == 0 && !s.researched);
    assert(sc64_z6_transport_tech_state_valid(&s));
}

static void test_research_gate_and_cost_once(void)
{
    Sc64Z6TransportTechState s = {0,0,0};
    int minerals = 250, gas = 250;
    assert(sc64_z6_transport_tech_init(&s));
    assert(sc64_z6_transport_tech_start(&s, 4, 131, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(minerals == 250 && gas == 250 && !s.ticks_remaining);
    assert(sc64_z6_transport_tech_start(&s, 4, 132, 2, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(sc64_z6_transport_tech_start(&s, 4, 132, 3, 0, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(sc64_z6_transport_tech_start(&s, 4, 132, 3, 1, 0, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(sc64_z6_transport_tech_start(&s, 4, 132, 3, 1, 1, 0, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(sc64_z6_transport_tech_start(&s, 4, 132, 3, 1, 1, 1, 0,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(sc64_z6_transport_tech_start(&s, 4, 132, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_RESEARCHING);
    assert(minerals == 50 && gas == 50);
    assert(s.ticks_remaining == 3000 && s.producer_slot_plus1 == 5 && !s.researched);
    assert(sc64_z6_transport_tech_state_valid(&s));
    assert(sc64_z6_transport_tech_start(&s, 4, 132, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(minerals == 50 && gas == 50);
}

static void test_pause_duration_and_completion(void)
{
    Sc64Z6TransportTechState s = {0,0,0};
    int minerals = 200, gas = 200;
    assert(sc64_z6_transport_tech_init(&s));
    assert(sc64_z6_transport_tech_start(&s, 239, 133, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_RESEARCHING);
    assert(s.producer_slot_plus1 == 240);
    assert(sc64_z6_transport_tech_tick(&s, 1, 1) == SC64_Z6_TRANSPORT_TECH_IDLE);
    assert(s.ticks_remaining == 3000 && !s.researched);
    assert(sc64_z6_transport_tech_tick(&s, 0, 0) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(s.ticks_remaining == 3000 && !s.researched);
    for (unsigned i = 0; i < 2999; ++i)
        assert(sc64_z6_transport_tech_tick(&s, 0, 1) == SC64_Z6_TRANSPORT_TECH_RESEARCHING);
    assert(s.ticks_remaining == 1 && !s.researched);
    assert(sc64_z6_transport_tech_tick(&s, 0, 1) == SC64_Z6_TRANSPORT_TECH_RESEARCHED);
    assert(!s.ticks_remaining && !s.producer_slot_plus1 && s.researched);
    assert(sc64_z6_transport_tech_state_valid(&s));
    assert(sc64_z6_transport_tech_tick(&s, 0, 1) == SC64_Z6_TRANSPORT_TECH_IDLE);
}

static void test_native_cancellation_refunds_without_unlock(void)
{
    Sc64Z6TransportTechState s = {0,0,0};
    int minerals = 200, gas = 200;
    assert(sc64_z6_transport_tech_init(&s));
    assert(sc64_z6_transport_tech_start(&s, 10, 132, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_RESEARCHING);
    assert(minerals == 0 && gas == 0);
    /* Runtime banks are nonnegative; model only permits sufficient funds. */
    minerals = gas = 100;
    assert(sc64_z6_transport_tech_cancel(&s, 1, &minerals, &gas) ==
           SC64_Z6_TRANSPORT_TECH_CANCELLED);
    assert(minerals == 250 && gas == 250 && !s.researched);
    assert(sc64_z6_transport_tech_state_valid(&s));
    minerals = gas = 0;
    assert(sc64_z6_transport_tech_start(&s, 10, 132, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    minerals = gas = 100;
    assert(sc64_z6_transport_tech_start(&s, 10, 132, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    minerals = gas = 200;
    assert(sc64_z6_transport_tech_start(&s, 10, 132, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_RESEARCHING);
    assert(sc64_z6_transport_tech_cancel(&s, 0, &minerals, &gas) ==
           SC64_Z6_TRANSPORT_TECH_CANCELLED);
    assert(minerals == 200 && gas == 200 && !s.researched);
}

static void test_corrupt_state_rejected_before_mutation(void)
{
    Sc64Z6TransportTechState s = {3001, 1, 0};
    int minerals = 200, gas = 200;
    assert(!sc64_z6_transport_tech_state_valid(&s));
    assert(sc64_z6_transport_tech_start(&s, 0, 132, 3, 1, 1, 1, 1,
                                         &minerals, &gas) == SC64_Z6_TRANSPORT_TECH_ERROR);
    assert(minerals == 200 && gas == 200);
    s = (Sc64Z6TransportTechState){0, 1, 0};
    assert(!sc64_z6_transport_tech_state_valid(&s));
    s = (Sc64Z6TransportTechState){1, 241, 0};
    assert(!sc64_z6_transport_tech_state_valid(&s));
    s = (Sc64Z6TransportTechState){1, 1, 1};
    assert(!sc64_z6_transport_tech_state_valid(&s));
    s = (Sc64Z6TransportTechState){0, 0, 1};
    assert(sc64_z6_transport_tech_state_valid(&s));
}

static void test_world_adapter_gates_and_death_refund(void)
{
    for (int i=0;i<MAX_BUILDINGS;++i) b[i]=(Building){0};
    race_z6.transport=(Sc64Z6TransportTechState){0,0,0};
    minerals=250; gas=250;
    b[1]=(Building){1000,SIDE,1,0,0,133,3};
    selected_building=1;
    assert(race_z6_transport_initialize_source());
    assert(race_z6_transport_research_menu_available());
    assert(race_z6_transport_start_research_selected()==SC64_Z6_TRANSPORT_TECH_RESEARCHING);
    assert(minerals==50 && gas==50);
    assert(race_z6_transport_state_valid());
    assert(!race_z6_transport_overlord_ready());
    assert(race_z6_transport_research_tick(1));
    assert(race_z6.transport.ticks_remaining==3000);
    for (int i=0;i<2999;++i) assert(race_z6_transport_research_tick(0));
    assert(race_z6.transport.ticks_remaining==1);
    assert(race_z6_transport_research_tick(0));
    assert(race_z6_transport_overlord_ready());
    assert(race_z6_transport_state_valid());

    assert(sc64_z6_transport_tech_init(&race_z6.transport));
    minerals=200; gas=200;
    assert(race_z6_transport_start_research_selected()==SC64_Z6_TRANSPORT_TECH_RESEARCHING);
    race_z6_transport_note_building_death(1);
    assert(minerals==150 && gas==150);
    assert(!race_z6_transport_overlord_ready());
    assert(sc64_z6_transport_tech_state_valid(&race_z6.transport));
    b[1].raw_type=131;
    assert(!race_z6_transport_research_menu_available());
}

int main(void)
{
    test_facts_and_initial_state();
    test_research_gate_and_cost_once();
    test_pause_duration_and_completion();
    test_native_cancellation_refunds_without_unlock();
    test_corrupt_state_rejected_before_mutation();
    test_world_adapter_gates_and_death_refund();
    puts("Z6 Ventral Sacs transport-tech tests passed");
    return 0;
}
