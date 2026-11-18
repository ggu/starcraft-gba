/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/sc64_ghost_tech_save_wire.h"
#include "../starcraft/sc64_nuke_save_rows.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const Sc64GhostSavePolicy policy = {
    .seeded = 8u << 24, /* campaign reactor grant at source slot 6 */
    .allowed = {12, 12, 0, 0, 12, 0, 4, 0},
    .bank = {0, 1, 255, 255, 4, 255, 1, 255},
    .active_banks = 0x1bu, .neutral_bank = 2
};
static const Sc64GhostSaveFacts online = {
    .mapped_raw = 117, .raw = -1, .alive = 1, .complete = 1,
    .owner = 0, .key = 0, .parent_owner = 0, .parent_key = 0,
    .attached = 1, .queue_empty = 1
};

static int ghost(void *unused, unsigned slot, Sc64NukeSaveGhostFacts *out)
{
    (void)unused;
    if (slot != 25) return 0;
    *out = (Sc64NukeSaveGhostFacts){1, 1, 0, 1};
    return 1;
}

static int cadence(void *unused, unsigned ox, unsigned oy, unsigned x, unsigned y,
                   Sc64NukeCadence *out)
{
    (void)unused;
    if (ox != 100 || oy != 120 || x != 200 || y != 220) return 0;
    *out = (Sc64NukeCadence){60, 24, 12};
    return 1;
}

/* Run the existing nuclear wire codecs on the same staged candidate. The
 * Ghost layout experiment uses no hypothetical journal tag here. */
static void verify_nuclear_rows(const void *candidate, Sc64GhostSaveRow row)
{
    const uint32_t base_outer = UINT32_C(0x7743505f), inner = UINT32_C(0x7343505f);
    const Sc64NukeSaveWorld world = {1024, 1024, 240, 160, 2, 0x1bu, 54};
    const Sc64NukeSaveSiloFacts silo = {108, 0, 1, 1, 1, 1, 0, 0};
    const Sc64NukeSaveSiloWire wire = {row.progress, row.level};
    const Sc64NukeSaveMissileWire flight = {
        .type = 54, .raw_type = 14, .owner = 0, .alive = 1, .idle = 1,
        .x = 100, .y = 120, .tx = 200, .ty = 220,
        .work_timer = 19, .target = 25, .build = SC64_NUKE_SAVE_BUILD_PAINTING
    };
    Sc64NukeSaveAuth auth;
    Sc64NukeSaveSilo armed;
    Sc64NukeSaveMissile missile;
    Sc64NukeSaveCensus census = {0};
    assert(sc64_nuke_save_authenticate(&auth, candidate, sc64_nuke_save_tag(base_outer),
                                     inner, base_outer, inner));
    assert(sc64_nuke_save_silo_decode(&armed, &wire, &silo, &world, &auth, candidate, inner));
    assert(armed.ammo.armed && armed.payer == 0 && !armed.ammo.remaining);
    assert(sc64_nuke_save_missile_decode(&missile, 120, &flight, &world, &auth,
                                       candidate, inner, cadence, ghost, 0));
    assert(missile.strike.phase == SC64_NUKE_PAINTING && missile.strike.age == 19);
    assert(sc64_nuke_save_count_silo(&census, &world, 5, &armed));
    assert(sc64_nuke_save_count_missile(&census, &world, 120, &missile));
    assert(census.reserved[0] == 2 * SC64_NUKE_MISSILE_SUPPLY && census.missiles == 1);
}

static void expect_refused(unsigned format, uint32_t word, const Sc64GhostSavePolicy *p,
    const Sc64GhostSaveFacts *facts, const Sc64GhostSaveRow *rows, unsigned count)
{
    Sc64GhostSavePlan out;
    unsigned char before[sizeof out];
    memset(&out, 0x5a, sizeof out);
    memcpy(before, &out, sizeof out);
    assert(!sc64_ghost_save_prepare(&out, format, word, p, facts, rows, count));
    assert(!memcmp(before, &out, sizeof out)); /* Caller can try the older record. */
}

static void verify_stream_refusal(void)
{
    Sc64GhostSaveScan scan;
    Sc64GhostSavePlan out;
    unsigned char untouched[sizeof out];
    const Sc64GhostSaveRow paid = {2500, 2}, malformed = {2501, 2}, empty = {0};
    const Sc64GhostSaveFacts other = {.mapped_raw = 108};
    memset(&out, 0x5a, sizeof out);
    memcpy(untouched, &out, sizeof out);
    assert(sc64_ghost_save_scan_begin(&scan, SC64_GHOST_SAVE_COVERT_CANDIDATE,
                                     SC64_GHOST_SAVE_DAMAGE_WORD, &policy, 3));
    assert(sc64_ghost_save_scan_row(&scan, &online, &paid));
    assert(sc64_ghost_save_scan_row(&scan, &other, &empty));
    assert(!sc64_ghost_save_scan_finish(&scan, &out)); /* The final slot is missing. */
    assert(!memcmp(untouched, &out, sizeof out));
    assert(!sc64_ghost_save_scan_row(&scan, &online, &malformed));
    assert(!sc64_ghost_save_scan_row(&scan, &other, &empty)); /* Poisoned candidate. */
    assert(!sc64_ghost_save_scan_finish(&scan, &out));
    assert(!memcmp(untouched, &out, sizeof out));
}

int main(void)
{
    for (uint32_t bits = 0; bits <= UINT16_MAX; ++bits)
        assert(sc64_ghost_save_rank(sc64_ghost_save_unrank(bits)) == bits);
    const unsigned base = SC64_GHOST_SAVE_BASE, extended = SC64_GHOST_SAVE_COVERT_CANDIDATE;
    const uint32_t marker = SC64_GHOST_SAVE_DAMAGE_WORD;
    enum { FIRST = 13, SECOND = 159, SILO = 5, DUPLICATE = 140, SLOTS = 160 };
    Sc64GhostSaveFacts facts[SLOTS] = {0};
    Sc64GhostSaveRow rows[SLOTS] = {0}, older[SLOTS] = {0};
    facts[FIRST] = online;
    facts[SECOND] = online;
    facts[SECOND].raw = 117;
    facts[SECOND].owner = facts[SECOND].parent_owner = 1;
    facts[SECOND].key = facts[SECOND].parent_key = 6; /* Raw key differs from bank. */
    facts[SILO].mapped_raw = facts[SILO].raw = 108;
    rows[SILO] = older[SILO] = (Sc64GhostSaveRow){SC64_NUKE_SAVE_ARMED_PROGRESS, 1};
    facts[DUPLICATE] = online;
    Sc64GhostSavePlan out;
    const uint32_t earned = sc64_ghost_tech_bit(4, SC64_GHOST_TECH_REACTOR);
    const Sc64GhostTechState live = {
        policy.seeded | earned,
        sc64_ghost_tech_bit(0, SC64_GHOST_TECH_REACTOR) |
        sc64_ghost_tech_bit(6, SC64_GHOST_TECH_OCULAR)
    };
    uint32_t word = 0xaaaa;
    assert(!sc64_ghost_save_encode_header(&word, base, marker, &policy, &live));
    assert(word == 0xaaaa);
    assert(sc64_ghost_save_encode_header(&word, extended, marker, &policy, &live));
    assert(word == (marker ^ UINT32_C(0x0200)));
    Sc64GhostSaveReceipt a = {{2500, SC64_GHOST_TECH_REACTOR, 0}, FIRST, 0};
    const Sc64GhostSaveReceipt b = {{1, SC64_GHOST_TECH_OCULAR, 6}, SECOND, 1};
    const Sc64GhostSaveRow empty = {0};
    assert(sc64_ghost_save_encode_receipt(&rows[FIRST], &empty, &policy, &facts[FIRST], &a));
    assert(rows[FIRST].progress == 2500 && rows[FIRST].level == 2);
    assert(sc64_ghost_save_encode_receipt(&rows[SECOND], &empty, &policy, &facts[SECOND], &b));
    assert(rows[SECOND].progress == 1 && rows[SECOND].level == 13);
    verify_nuclear_rows(rows, rows[SILO]);
    assert(sc64_ghost_save_prepare(&out, extended, word, &policy, facts, rows, SLOTS));
    assert(out.tech.completed == live.completed && out.tech.pending == live.pending &&
           out.canonical_damage_word == marker && out.count == 2);
    assert(out.receipts[0].building == FIRST && out.receipts[0].bank == 0 &&
           out.receipts[0].paid.kind == SC64_GHOST_TECH_REACTOR &&
           out.receipts[0].paid.payer == 0 && out.receipts[0].paid.remaining == 2500);
    assert(out.receipts[1].building == SECOND && out.receipts[1].bank == 1 &&
           out.receipts[1].paid.kind == SC64_GHOST_TECH_OCULAR &&
           out.receipts[1].paid.payer == 6 && out.receipts[1].paid.remaining == 1);
    assert(sc64_ghost_tech_tick(&out.tech, &out.receipts[1].paid, 6, 1, 1) ==
           SC64_GHOST_JOB_COMPLETE);
    assert(sc64_ghost_sight_source_pixels(&out.tech, 6) == SC64_NUKE_OCULAR_SOURCE_SIGHT);
    assert(sc64_ghost_energy_cap(&out.tech, 4) == 250 * 256);
    verify_nuclear_rows(rows, rows[SILO]); /* Preparation does not edit either wire. */

    expect_refused(base, marker, &policy, facts, rows, SLOTS); /* Paid-only masquerade. */
    expect_refused(base, word, &policy, facts, older, SLOTS);
    expect_refused(extended, word ^ 0x10000u, &policy, facts, rows, SLOTS);
    expect_refused(extended, word ^ 0x2000u, &policy, facts, rows, SLOTS); /* Redundant seed. */
    expect_refused(extended, word ^ 0x10u, &policy, facts, rows, SLOTS); /* Inactive source. */
    expect_refused(extended, marker, &policy, facts, older, SLOTS); /* Empty extension. */
    rows[DUPLICATE] = rows[FIRST];
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    rows[DUPLICATE] = (Sc64GhostSaveRow){0};
    rows[SECOND].progress = 2501;
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    rows[SECOND].progress = -1;
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    rows[SECOND].progress = 1;
    rows[SECOND].level = 14; /* T10-style seeded key cannot have a receipt for that grant. */
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    rows[SECOND].level = 13;

    Sc64GhostSaveFacts saved = facts[SECOND];
    facts[SECOND].parent_key = 1;
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    facts[SECOND] = saved;
    facts[SECOND].attached = 0;
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    facts[SECOND] = saved;
    facts[SECOND].owner = facts[SECOND].parent_owner = 4;
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    facts[SECOND] = saved;
    facts[SECOND].raw = 108;
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    facts[SECOND] = saved;
    facts[SECOND].queue_empty = 0;
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    facts[SECOND] = saved;
    facts[SECOND].complete = 0;
    expect_refused(extended, word, &policy, facts, rows, SLOTS);
    facts[SECOND] = saved;
    a.bank = 1;
    Sc64GhostSaveRow sentinel = {77, 88};
    assert(!sc64_ghost_save_encode_receipt(&sentinel, &empty, &policy, &facts[FIRST], &a));
    assert(sentinel.progress == 77 && sentinel.level == 88);
    assert(sc64_ghost_save_prepare(&out, base, marker, &policy, facts, older, SLOTS));
    assert(out.tech.completed == policy.seeded && !out.tech.pending && !out.count);
    assert(out.canonical_damage_word == marker);
    verify_nuclear_rows(older, older[SILO]);
    assert(sc64_ghost_save_prepare(&out, base, 0, &policy, facts, older, SLOTS));
    assert(!out.canonical_damage_word && !out.count);
    verify_stream_refusal();
    puts("Ghost save wire: paid and earned projections, predecessor nuclear rows and atomic refusal");
    return 0;
}
