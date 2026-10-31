/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SKIRMISH_TRANSPORT_ADAPTER_H
#define SC64_SKIRMISH_TRANSPORT_ADAPTER_H

/* Jobs are independent of larva queues. A Lair or Hive may continue producing
 * units while it researches; the job binds the exact slot and owner. */
static int race_sk_transport_producer_valid(int slot, int owner)
{
    return race_mode && slot >= 0 && slot < MAX_BUILDINGS && owner >= 0 &&
        owner < 5 && race_player[owner] == R_ZERG && b[slot].alive &&
        b[slot].hp > 0 && b[slot].owner == owner &&
        (b[slot].type == B_LAIR || b[slot].type == B_HIVE) &&
        !b[slot].under_construction;
}

static int race_sk_transport_job_uses(int slot)
{
    if (!race_mode || slot < 0 || slot >= MAX_BUILDINGS) return 0;
    for (int i = 0; i < SC64_SK_TRANSPORT_OWNERS; ++i)
        if (race_skirmish_transport_tech.owner[i].producer_plus_one == slot + 1)
            return 1;
    return 0;
}

static int race_sk_transport_start_owner(int owner, int slot)
{
    int key = sc64_sk_transport_owner_index(owner);
    int ore, vespene;
    if (key < 0 || !race_sk_transport_producer_valid(slot, owner) ||
        race_sk_transport_job_uses(slot)) return 0;
    ore = race_owner_minerals(owner); vespene = race_owner_gas(owner);
    if (!sc64_sk_transport_start(&race_skirmish_transport_tech.owner[key],
                                 (unsigned)slot, MAX_BUILDINGS, &ore, &vespene)) return 0;
    race_add_owner_resources(owner, -SC64_SK_TRANSPORT_MINERALS, -SC64_SK_TRANSPORT_GAS);
    return 1;
}

static int race_sk_transport_cancel_owner(int owner, int destroyed)
{
    int key = sc64_sk_transport_owner_index(owner), ore, vespene;
    int old_ore, old_vespene;
    if (key < 0) return 0;
    old_ore = ore = race_owner_minerals(owner);
    old_vespene = vespene = race_owner_gas(owner);
    if (!sc64_sk_transport_cancel(&race_skirmish_transport_tech.owner[key], destroyed,
                                   &ore, &vespene)) return 0;
    race_add_owner_resources(owner, ore - old_ore, vespene - old_vespene);
    return 1;
}

static int race_sk_transport_research_menu(void)
{
    int slot = selected_building, key = sc64_sk_transport_owner_index(SIDE);
    Sc64SkTransportJob *job = &race_skirmish_transport_tech.owner[key];
    return race_sk_transport_producer_valid(slot, SIDE) && !job->researched &&
        (!job->remaining || job->producer_plus_one == slot + 1);
}

static void race_sk_transport_research_tick(void)
{
    static const int owners[SC64_SK_TRANSPORT_OWNERS] = {0, 1, 3, 4};
    if (!race_mode) return;
    for (int i = 0; i < SC64_SK_TRANSPORT_OWNERS; ++i) {
        Sc64SkTransportJob *job = &race_skirmish_transport_tech.owner[i];
        if (!job->remaining) continue;
        if (!race_sk_transport_producer_valid((int)job->producer_plus_one - 1, owners[i])) {
            (void)race_sk_transport_cancel_owner(owners[i], 1);
            continue;
        }
        sc64_sk_transport_tick(job);
    }
}

static void race_sk_transport_producer_destroyed(int slot)
{
    static const int owners[SC64_SK_TRANSPORT_OWNERS] = {0, 1, 3, 4};
    if (!race_mode) return;
    for (int i = 0; i < SC64_SK_TRANSPORT_OWNERS; ++i)
        if (race_skirmish_transport_tech.owner[i].producer_plus_one == slot + 1)
            (void)race_sk_transport_cancel_owner(owners[i], 1);
}
#endif
