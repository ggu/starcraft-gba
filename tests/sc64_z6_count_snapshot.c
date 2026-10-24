/* Asset-free differential test for the optional Z6 streaming count cache. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_z6_count_snapshot.h"

static u32 rng_state = 0x91e10da5u;
static u32 next_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void init_context(u8 active[12], u8 forces[8],
                         Sc64Z6CountRect locations[7])
{
    memset(active, 0, 12);
    memset(locations, 0, sizeof(Sc64Z6CountRect) * 7u);
    for (unsigned i = 0; i < 8; ++i) {
        active[i] = 1;
        forces[i] = (u8)sc64_z6_source_forces[i];
    }
    /* Four locations queried by the authenticated rows; half-open runtime
       coordinates stand in for world_adapter's integer-scaled bounds. */
    locations[1] = (Sc64Z6CountRect){100, 100, 180, 180, 1};
    locations[4] = (Sc64Z6CountRect){300, 300, 420, 420, 1};
    locations[5] = (Sc64Z6CountRect){600, 250, 720, 390, 1};
    locations[6] = (Sc64Z6CountRect){900, 800, 1020, 920, 1};
}

static int reference_owner(unsigned selector, unsigned owner,
                           const u8 active[12], unsigned executor,
                           const u8 forces[8])
{
    if (owner >= 12 || !active[owner]) return 0;
    if (selector <= 11) return selector == owner;
    if (selector == 12) return 0;
    if (selector == 13) return owner == executor;
    if (selector == 17) return owner < 8;
    if (selector >= 18 && selector <= 21)
        return owner < 8 && forces[owner] == selector - 18;
    return -1;
}

static int executor_for(unsigned trigger, const u8 active[12],
                        const u8 forces[8], unsigned *out)
{
    unsigned found = 12;
    for (unsigned p = 0; p < 27; ++p) {
        if (!sc64_z6_trigger_enabled(trigger, p)) continue;
        if (p < 8) {
            if (!active[p] || found != 12) return 0;
            found = p;
        } else if (p >= 18 && p <= 21) {
            for (unsigned s = 0; s < 8; ++s) if (active[s] &&
                forces[s] == p - 18) {
                if (found != 12) return 0;
                found = s;
            }
            if (found == 12) return 0;
        } else return 0;
    }
    if (found >= 12) return 0;
    *out = found;
    return 1;
}

static void reference_count(const Sc64Z6CountActor *units, unsigned unit_count,
                            const Sc64Z6CountActor *buildings,
                            unsigned building_count, const u8 active[12],
                            const u8 forces[8],
                            const Sc64Z6CountRect locations[7],
                            u32 counts[13])
{
    memset(counts, 0, sizeof(u32) * 13u);
    for (unsigned qi = 0; qi < SC64_Z6_COUNT_QUERY_COUNT; ++qi) {
        const Sc64Z6CountQueryKey *key = &sc64_z6_count_query_keys[qi];
        const Sc64Z6SourceCondition *c;
        const Sc64Z6CountActor *banks[2] = {units, buildings};
        unsigned lengths[2] = {unit_count, building_count};
        unsigned executor;
        assert(sc64_z6_condition_row(key->trigger, key->index, &c));
        assert(executor_for(key->trigger, active, forces, &executor));
        for (unsigned bank = 0; bank < 2; ++bank) {
            for (unsigned i = 0; i < lengths[bank]; ++i) {
                const Sc64Z6CountActor *a = &banks[bank][i];
                if (!a->alive) continue;
                assert(a->raw_owner >= 0 && a->raw_owner < 12);
                if (!reference_owner(key->selector,(unsigned)a->raw_owner,
                                     active,executor,forces) || a->raw_type < 0)
                    continue;
                if (key->raw != 229u &&
                    (bank || !a->type_known ||
                     a->raw_type != (s16)key->raw)) continue;
                if (key->location) {
                    const Sc64Z6CountRect *r = &locations[key->location];
                    if (!(a->x >= r->left && a->x < r->right &&
                          a->y >= r->top && a->y < r->bottom)) continue;
                }
                ++counts[(unsigned)(c - sc64_z6_source_conditions)];
            }
        }
    }
}

static void run_differential_trial(unsigned trial)
{
    u8 active[12], forces[8];
    Sc64Z6CountRect locations[7];
    Sc64Z6CountActor units[240], buildings[160];
    Sc64Z6CountSnapshotBuilder builder;
    Sc64Z6CountSnapshot snapshot;
    u32 expected[13];
    init_context(active, forces, locations);
    /* Different query rectangles each pass also model MoveLocation changes. */
    for (unsigned id = 1; id <= 6; ++id) if (locations[id].valid) {
        int left = (int)(next_random() % 1800u);
        int top = (int)(next_random() % 1800u);
        locations[id].left = left;
        locations[id].top = top;
        locations[id].right = left + 1 + (int)(next_random() % 160u);
        locations[id].bottom = top + 1 + (int)(next_random() % 160u);
    }
    for (unsigned i = 0; i < 240; ++i) {
        u32 r = next_random();
        units[i].alive = (u8)((r >> 4) % 5u != 0);
        units[i].type_known = (u8)((r >> 11) % 7u != 0);
        units[i].x = (int)(next_random() % 2049u);
        units[i].y = (int)(next_random() % 2049u);
        units[i].raw_owner = (s8)(next_random() % 8u);
        switch (next_random() % 6u) {
        case 0: units[i].raw_type = 51; break;
        case 1: units[i].raw_type = 79; break;
        default: units[i].raw_type = (s16)(next_random() % 230u); break;
        }
    }
    for (unsigned i = 0; i < 160; ++i) {
        u32 r = next_random();
        buildings[i].alive = (u8)((r >> 5) % 4u != 0);
        buildings[i].type_known = (u8)((r >> 12) % 9u != 0);
        buildings[i].x = (int)(next_random() % 2049u);
        buildings[i].y = (int)(next_random() % 2049u);
        buildings[i].raw_owner = (s8)(next_random() % 8u);
        buildings[i].raw_type = (s16)(next_random() % 230u);
    }
    /* Force center-boundary cases in every trial. */
    units[0] = (Sc64Z6CountActor){100,100,51,3,1,1}; /* included left/top */
    units[1] = (Sc64Z6CountActor){180,180,79,3,1,1}; /* excluded right/bottom */
    units[2] = (Sc64Z6CountActor){150,150,51,1,1,1}; /* player 1, not CP */
    units[3] = (Sc64Z6CountActor){150,150,79,1,1,1}; /* explicit player 1 */
    units[4] = (Sc64Z6CountActor){150,150,217,3,1,0}; /* unknown map ID, AnyUnit */
    reference_count(units,240,buildings,160,active,forces,locations,expected);
    memset(&snapshot, 0xa5, sizeof(snapshot));
    assert(sc64_z6_count_snapshot_begin(&snapshot,&builder,active,forces,locations,7));
    for (unsigned i = 0; i < 240; ++i)
        assert(sc64_z6_count_snapshot_add_actor(&builder,&units[i],0,locations));
    for (unsigned i = 0; i < 160; ++i)
        assert(sc64_z6_count_snapshot_add_actor(&builder,&buildings[i],1,locations));
    assert(sc64_z6_count_snapshot_finish(&builder,&snapshot));
    for (unsigned qi = 0; qi < SC64_Z6_COUNT_QUERY_COUNT; ++qi) {
        const Sc64Z6CountQueryKey *key = &sc64_z6_count_query_keys[qi];
        const Sc64Z6SourceCondition *c;
        u32 actual = 0;
        assert(sc64_z6_condition_row(key->trigger,key->index,&c));
        assert(sc64_z6_count_snapshot_lookup(&snapshot,c,&actual));
        assert(actual == expected[(unsigned)(c-sc64_z6_source_conditions)]);
    }
    assert(!sc64_z6_count_snapshot_lookup(&snapshot,
        &sc64_z6_source_conditions[1],&expected[0]));
    (void)trial;
}

static void test_invalid_and_atomic_fallback(void)
{
    u8 active[12], forces[8];
    Sc64Z6CountRect locations[7];
    Sc64Z6CountSnapshotBuilder builder;
    Sc64Z6CountSnapshot snapshot;
    Sc64Z6CountActor actor = {120,120,51,12,1,1};
    init_context(active, forces, locations);
    assert(sc64_z6_count_snapshot_begin(&snapshot,&builder,active,forces,locations,7));
    assert(!sc64_z6_count_snapshot_add_actor(&builder,&actor,0,locations));
    memset(&snapshot,0x5a,sizeof(snapshot));
    snapshot.active=0;
    assert(!sc64_z6_count_snapshot_finish(&builder,&snapshot));
    active[1]=0; /* Force 1 executor becomes ambiguous/missing: fallback. */
    assert(!sc64_z6_count_snapshot_begin(&snapshot,&builder,active,forces,locations,7));
    init_context(active, forces, locations);
    locations[4].valid=0;
    assert(!sc64_z6_count_snapshot_begin(&snapshot,&builder,active,forces,locations,7));
}

static void test_current_player_is_per_trigger(void)
{
    u8 active[12], forces[8];
    Sc64Z6CountRect locations[7];
    Sc64Z6CountActor units[2];
    Sc64Z6CountSnapshotBuilder builder;
    Sc64Z6CountSnapshot snapshot;
    unsigned executor0, executor8;
    u32 count;
    init_context(active, forces, locations);
    assert(executor_for(0,active,forces,&executor0));
    assert(executor_for(8,active,forces,&executor8));
    assert(executor0 != executor8);
    units[0]=(Sc64Z6CountActor){120,120,51,(s8)executor0,1,1};
    units[1]=(Sc64Z6CountActor){120,120,51,(s8)executor8,1,1};
    assert(sc64_z6_count_snapshot_begin(&snapshot,&builder,active,forces,locations,7));
    assert(sc64_z6_count_snapshot_add_actor(&builder,&units[0],0,locations));
    assert(sc64_z6_count_snapshot_add_actor(&builder,&units[1],0,locations));
    assert(sc64_z6_count_snapshot_finish(&builder,&snapshot));
    assert(sc64_z6_count_snapshot_lookup(&snapshot,
        &sc64_z6_source_conditions[0],&count) && count==1u);
    assert(sc64_z6_count_snapshot_lookup(&snapshot,
        &sc64_z6_source_conditions[10],&count) && count==1u);
    assert(sc64_z6_count_snapshot_lookup(&snapshot,
        &sc64_z6_source_conditions[12],&count) && count==1u);
}

int main(void)
{
    for (unsigned i = 0; i < 500; ++i) run_differential_trial(i);
    test_invalid_and_atomic_fallback();
    test_current_player_is_per_trigger();
    puts("Z6 count snapshot differential tests passed");
    return 0;
}
