/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../starcraft/sc64_skirmish_transport_tech.h"
#define SC64_SHARED_TRANSPORT 1
#include "../starcraft/sc64_transport_rules.h"

int main(void)
{
    Sc64SkTransportTech tech = {0};
    unsigned space;
    assert(sc64_transport_shared_space_provided_raw(11) == 8);
    assert(sc64_transport_shared_space_provided_raw(42) == 8);
    assert(sc64_transport_shared_space_provided_raw(69) == 8);
    assert(sc64_transport_shared_space_required_raw(17, &space) && space == 2);
    assert(sc64_transport_shared_space_required_raw(77, &space) && space == 2);
    assert(sc64_transport_shared_space_required_raw(78, &space) && space == 4);
    assert(sc64_transport_shared_space_required_raw(83, &space) && space == 4);
    assert(sc64_transport_shared_space_required_raw(42, &space) && space == 255);
    assert(!sc64_transport_shared_space_required_raw(228, &space));
    uint8_t bytes[16];
    Sc64SkTransportTech decoded;
    int minerals[4] = {300, 300, 300, 300};
    int gas[4] = {250, 250, 250, 250};
    assert(sc64_sk_transport_owner_index(0) == 0);
    assert(sc64_sk_transport_owner_index(1) == 1);
    assert(sc64_sk_transport_owner_index(3) == 2);
    assert(sc64_sk_transport_owner_index(4) == 3);
    assert(sc64_sk_transport_owner_index(2) == -1);
    for (unsigned i = 0; i < 4; ++i) {
        assert(sc64_sk_transport_start(&tech.owner[i], i, 200, &minerals[i], &gas[i]));
        assert(minerals[i] == 100 && gas[i] == 50);
        assert(sc64_sk_transport_job_valid(&tech.owner[i], 200));
    }
    sc64_sk_transport_encode(bytes, &tech);
    assert(sc64_sk_transport_decode(&decoded, bytes, 200));
    for (unsigned i = 0; i < 4; ++i)
        assert(decoded.owner[i].remaining == tech.owner[i].remaining &&
               decoded.owner[i].producer_plus_one == tech.owner[i].producer_plus_one);
    for (unsigned i = 0; i < SC64_SK_TRANSPORT_RESEARCH_TICKS; ++i)
        sc64_sk_transport_tick(&tech.owner[0]);
    assert(tech.owner[0].researched && !tech.owner[0].remaining && !tech.owner[0].producer_plus_one);
    assert(tech.owner[1].remaining == SC64_SK_TRANSPORT_RESEARCH_TICKS);
    assert(sc64_sk_transport_cancel(&tech.owner[1], 0, &minerals[1], &gas[1]));
    assert(minerals[1] == 300 && gas[1] == 250);
    assert(sc64_sk_transport_cancel(&tech.owner[2], 1, &minerals[2], &gas[2]));
    assert(minerals[2] == 250 && gas[2] == 200);
    assert(tech.owner[3].remaining == SC64_SK_TRANSPORT_RESEARCH_TICKS);
    return 0;
}
