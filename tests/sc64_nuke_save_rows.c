/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_nuke_save_rows.h"
#include <assert.h>
#include <stdio.h>

static const uint32_t current_inner = 0x7343505fu;
static const uint32_t current_outer = 0x7743505fu;
static int candidate, other;
static const Sc64NukeSaveWorld world = {
    .width = 2048, .height = 1024, .units = 240, .buildings = 160,
    .neutral_owner = 2, .owner_mask = 0x1bu, .missile_actor_type = 54
};
static const Sc64NukeSaveSiloFacts online = {
    .raw_type = 108, .owner = 0, .alive = 1, .complete = 1,
    .has_parent = 1, .train_allowed = 1, .queue_count = 0, .queue_head = 0
};

typedef struct {
    Sc64NukeSaveGhostFacts ghost;
    unsigned slot;
    int accept_cadence;
} Fixture;

static int get_ghost(void *ctx, unsigned slot, Sc64NukeSaveGhostFacts *out)
{
    const Fixture *fixture = ctx;
    if (slot != fixture->slot) return 0;
    *out = fixture->ghost;
    return 1;
}

static int derive_cadence(void *ctx, unsigned ox, unsigned oy, unsigned tx, unsigned ty,
                          Sc64NukeCadence *out)
{
    const Fixture *fixture = ctx;
    if (!fixture->accept_cadence) return 0;
    /* Deliberately depends on coordinates so a decoder cannot use an
     * unvalidated, independent age range from the incoming row. */
    *out = (Sc64NukeCadence){
        .paint = (uint16_t)(ox + oy + 1), .descent = (uint16_t)(tx + ty + 1), .warning = 1
    };
    return 1;
}

static Sc64NukeSaveAuth test_authentication(void)
{
    Sc64NukeSaveAuth auth = {0};
    uint32_t tag = sc64_nuke_save_tag(current_outer);
    assert(tag == 0x774b505fu);
    assert(sc64_nuke_save_tag(0x7744505fu) == 0);
    assert(!sc64_nuke_save_authenticate(&auth, &candidate, current_outer, current_inner,
                                       current_outer, current_inner));
    assert(!sc64_nuke_save_authenticate(&auth, &candidate, tag + 1, current_inner,
                                       current_outer, current_inner));
    assert(!sc64_nuke_save_authenticate(&auth, &candidate, tag, current_inner + 1,
                                       current_outer, current_inner));
    assert(!sc64_nuke_save_authenticate(&auth, &candidate, tag, 0x734b505fu,
                                       current_outer, 0x734b505fu));
    assert(!sc64_nuke_save_authenticate(&auth, 0, tag, current_inner, current_outer, current_inner));
    assert(!sc64_nuke_save_authenticate(0, &candidate, tag, current_inner, current_outer, current_inner));
    assert(sc64_nuke_save_authenticate(&auth, &candidate, tag, current_inner, current_outer, current_inner));
    assert(sc64_nuke_save_auth_matches(&auth, &candidate, current_inner));
    assert(!sc64_nuke_save_auth_matches(&auth, &other, current_inner));
    assert(!sc64_nuke_save_auth_matches(&auth, &candidate, current_inner + 1));
    Sc64NukeSaveAuth invalidated = auth;
    assert(!sc64_nuke_save_authenticate(&invalidated, &candidate, tag, current_inner + 1,
                                       current_outer, current_inner));
    assert(!sc64_nuke_save_auth_matches(&invalidated, &candidate, current_inner));
    return auth;
}

static void test_silo(const Sc64NukeSaveAuth *auth)
{
    Sc64NukeSaveSilo input = {{1500, 0}, 0};
    Sc64NukeSaveSilo decoded = {{3, 0}, 4};
    Sc64NukeSaveSiloFacts facts = online;
    Sc64NukeSaveSiloWire wire;
#define ENCODE() sc64_nuke_save_silo_encode(&wire, &input, &facts, &world, auth, &candidate, current_inner)
#define DECODE() sc64_nuke_save_silo_decode(&decoded, &wire, &facts, &world, auth, &candidate, current_inner)
    assert(ENCODE() && wire.queue_progress == 1500 && wire.level == 1);
    assert(!sc64_nuke_save_legacy_silo_clear(&wire));
    assert(!sc64_nuke_save_silo_decode(&decoded, &wire, &facts, &world,
                                     auth, &other, current_inner));
    assert(!sc64_nuke_save_silo_decode(&decoded, &wire, &facts, &world, 0, &candidate, current_inner));
    assert(decoded.ammo.remaining == 3 && decoded.payer == 4);
    assert(DECODE() && decoded.ammo.remaining == 1500 && !decoded.ammo.armed && decoded.payer == 0);
    wire.queue_progress = 1;
    assert(DECODE() && decoded.ammo.remaining == 1);
    facts.train_allowed = 0;
    assert(!DECODE());
    facts = online;
    facts.complete = 0;
    assert(!DECODE());
    facts = online;
    facts.alive = 0;
    assert(!DECODE());
    facts = online;
    facts.has_parent = 0;
    assert(!DECODE());
    facts = online;
    facts.owner = 2; facts.has_parent = 0;
    assert(!DECODE()); /* detached work is canceled, not suspended */
    facts = online;
    facts.queue_count = 1;
    assert(!DECODE()); /* no second representation in the normal queue */
    facts = online;
    facts.queue_head = 1;
    assert(!DECODE());
    facts = online;
    facts.raw_type = 117;
    assert(!DECODE());
    facts = online;
    wire.level = 3;
    assert(!DECODE()); /* neutral may hold the Silo but cannot be its payer */
    wire.level = 0;
    assert(!DECODE());
    wire.level = 2;
    assert(!DECODE()); /* foreign payer cannot use an online Silo */
    wire.level = 1; wire.queue_progress = -1;
    assert(!DECODE());
    wire.queue_progress = SC64_NUKE_SAVE_ARMED_PROGRESS + 1;
    assert(!DECODE());

    input = (Sc64NukeSaveSilo){{0, 1}, 4};
    facts.owner = 2; facts.has_parent = 0; facts.train_allowed = 0;
    assert(ENCODE() && wire.queue_progress == SC64_NUKE_SAVE_ARMED_PROGRESS && wire.level == 5);
    assert(DECODE() && decoded.ammo.armed && !decoded.ammo.remaining && decoded.payer == 4);
    facts.has_parent = 1;
    assert(!DECODE());
    facts.owner = 4;
    assert(DECODE()); /* reconnect transfers an already paid missile without training */
    input.ammo.remaining = 1;
    assert(!ENCODE());
    input = (Sc64NukeSaveSilo){{0, 0}, SC64_NUKE_SAVE_NO_PAYER};
    assert(ENCODE() && sc64_nuke_save_legacy_silo_clear(&wire));
    assert(DECODE() && !sc64_nuke_silo_supply(&decoded.ammo) && decoded.payer == SC64_NUKE_SAVE_NO_PAYER);
    wire.level = 1;
    assert(!DECODE() && !sc64_nuke_save_legacy_silo_clear(&wire));
#undef DECODE
#undef ENCODE
}

static void test_missile(const Sc64NukeSaveAuth *auth)
{
    Fixture fixture = {.ghost = {1, 1, 0, 1}, .slot = 239, .accept_cadence = 1};
    Sc64NukeSaveMissile state = {
        .strike = {.x = 31, .y = 12, .spotter = 239, .age = 10, .owner = 0,
                   .phase = SC64_NUKE_PAINTING, .cadence = {31, 44, 1}},
        .origin_x = 18, .origin_y = 12
    };
    Sc64NukeSaveMissile decoded = {0};
    Sc64NukeSaveMissileWire wire;
#define ENCODE() sc64_nuke_save_missile_encode(&wire, 238, &state, &world, auth, &candidate, \
                                               current_inner, derive_cadence, get_ghost, &fixture)
#define DECODE() sc64_nuke_save_missile_decode(&decoded, 238, &wire, &world, auth, &candidate, \
                                               current_inner, derive_cadence, get_ghost, &fixture)
    assert(ENCODE() && wire.build == SC64_NUKE_SAVE_BUILD_PAINTING && wire.target == 239);
    assert(sc64_nuke_save_missile_claimed(&wire, &world));
    assert(DECODE() && decoded.strike.age == 10 && decoded.strike.spotter == 239 &&
           decoded.origin_x == 18 && decoded.strike.y == 12);
    assert(!sc64_nuke_save_missile_decode(&decoded, 238, &wire, &world, auth, &other, current_inner,
                                          derive_cadence, get_ghost, &fixture));
    assert(!sc64_nuke_save_missile_decode(&decoded, 238, &wire, &world, 0, &candidate, current_inner,
                                          derive_cadence, get_ghost, &fixture));
    Sc64NukeSaveMissileWire valid = wire;
#define REJECT(field, value) do { wire = valid; wire.field = (value); assert(!DECODE()); } while (0)
    REJECT(build, 2);
    REJECT(work_timer, 31);
    REJECT(work_timer, -1);
    REJECT(work_timer, 2048);
    REJECT(target, 238);
    REJECT(target, 240);
    REJECT(target, -1);
    REJECT(alive, 0);
    REJECT(selected, 1);
    REJECT(idle, 0);
    REJECT(owner, 2);
    REJECT(owner, 5);
    REJECT(x, -1);
    REJECT(tx, 2048);
    REJECT(ty, 1024);
    REJECT(x, 2047); /* recomputed terminal age exceeds the wire's range */
    REJECT(raw_type, 1);
    assert(sc64_nuke_save_missile_claimed(&wire, &world)); /* enum alone is suspicious */
    REJECT(type, 53);
    assert(sc64_nuke_save_missile_claimed(&wire, &world)); /* raw identity alone too */
    wire = valid;
    fixture.ghost.alive = 0;
    assert(!DECODE());
    fixture.ghost.alive = 1; fixture.ghost.ordinary = 0;
    assert(!DECODE());
    fixture.ghost.ordinary = 1; fixture.ghost.owner = 4;
    assert(!DECODE());
    fixture.ghost.owner = 0; fixture.ghost.can_track = 0;
    assert(!DECODE());
    fixture.ghost.can_track = 1; fixture.accept_cadence = 0;
    assert(!DECODE());
    fixture.accept_cadence = 1;

    state.strike.age = 0; /* first clock tick has not yet run */
    assert(ENCODE() && DECODE() && !decoded.strike.age);
    state.strike.cadence.paint--;
    assert(!ENCODE()); /* stored timing must match the canonical map-derived clock */
    state.strike.cadence.paint++;
    state.strike.age = 29;
    assert(ENCODE() && DECODE());
    assert(sc64_nuke_flight_step(&decoded.strike, 1) == 0);
    assert(sc64_nuke_flight_step(&decoded.strike, 1) == SC64_NUKE_EVENT_RELEASE);
    assert(decoded.strike.phase == SC64_NUKE_DESCENDING && decoded.strike.age == 31);
    state.strike.phase = SC64_NUKE_DESCENDING; state.strike.age = 31;
    fixture.ghost.alive = 0; /* release severs the Ghost link */
    assert(ENCODE() && wire.build == SC64_NUKE_SAVE_BUILD_DESCENDING && wire.target == -1);
    assert(DECODE() && decoded.strike.phase == SC64_NUKE_DESCENDING &&
           decoded.strike.spotter == SC64_NUKE_SAVE_NO_GHOST);
    valid = wire;
    REJECT(target, 239);
    REJECT(work_timer, 30);
    REJECT(work_timer, 75); /* terminal explosion must never recur after loading */
    wire = valid; wire.work_timer = 74;
    assert(DECODE());
    assert(sc64_nuke_save_missile_decode(&decoded, 238, &wire, &world, auth, &candidate,
                                        current_inner, derive_cadence, 0, &fixture));
    assert(sc64_nuke_flight_step(&decoded.strike, 0) == SC64_NUKE_EVENT_IMPACT);
    assert(sc64_nuke_flight_step(&decoded.strike, 0) == 0);
    wire.type = 0; wire.raw_type = 0;
    assert(!sc64_nuke_save_missile_claimed(&wire, &world));
#undef REJECT
#undef DECODE
#undef ENCODE
}

static void test_world_census(void)
{
    Sc64NukeSaveCensus census = {0};
    const Sc64NukeSaveSilo silo = {{0, 1}, 4};
    Sc64NukeSaveMissile missile = {.strike = {
        .spotter = 1, .owner = 4, .phase = SC64_NUKE_PAINTING
    }};
    assert(sc64_nuke_save_count_silo(&census, &world, 159, &silo));
    assert(!sc64_nuke_save_count_silo(&census, &world, 159, &silo));
    assert(!sc64_nuke_save_count_silo(&census, &world, 160, &silo));
    assert(sc64_nuke_save_count_missile(&census, &world, 0, &missile));
    assert(!sc64_nuke_save_count_missile(&census, &world, 0, &missile));
    assert(!sc64_nuke_save_count_missile(&census, &world, 3, &missile));
    assert(!sc64_nuke_save_count_approach(&census, &world, 1));
    assert(!sc64_nuke_save_count_approach(&census, &world, 0));
    assert(sc64_nuke_save_count_approach(&census, &world, 2));
    missile.strike.phase = SC64_NUKE_DESCENDING;
    assert(!sc64_nuke_save_count_missile(&census, &world, 3, &missile));
    missile.strike.spotter = SC64_NUKE_SAVE_NO_GHOST;
    assert(!sc64_nuke_save_count_missile(&census, &world, 2, &missile));
    for (unsigned i = 3; i < 9; ++i)
        assert(sc64_nuke_save_count_missile(&census, &world, i, &missile));
    assert(!sc64_nuke_save_count_missile(&census, &world, 10, &missile));
    assert(!sc64_nuke_save_count_approach(&census, &world, 10));
    assert(census.missiles == 7 && census.approaches == 1 && census.tracks == 2 &&
           census.reserved[4] == 8 * SC64_NUKE_MISSILE_SUPPLY && !census.reserved[0]);
    for (unsigned i = 0; i < 40; ++i)
        assert(sc64_nuke_save_count_silo(&census, &world, i, &silo));
    assert(census.reserved[4] == 48 * SC64_NUKE_MISSILE_SUPPLY);
}

int main(void)
{
    assert(sc64_nuke_save_world_valid(&world));
    Sc64NukeSaveWorld wrong = world;
    wrong.width = 2049;
    assert(!sc64_nuke_save_world_valid(&wrong));
    wrong = world; wrong.owner_mask |= 1u << world.neutral_owner;
    assert(!sc64_nuke_save_world_valid(&wrong));
    wrong = world; wrong.units = 241;
    assert(!sc64_nuke_save_world_valid(&wrong));
    wrong = world; wrong.missile_actor_type = 64;
    assert(!sc64_nuke_save_world_valid(&wrong));
    Sc64NukeSaveAuth auth = test_authentication();
    test_silo(&auth);
    test_missile(&auth);
    test_world_census();
    puts("nuke save rows: exact journal authentication, paid Silo, Ghost links, and independent descent");
    return 0;
}
