/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Protoss shield hit overlay on the real engine, in the Terran product (its
 * custom games field every race). Checks when a flash starts, which side it
 * shows on, how queued impacts carry their direction, slot reuse, expiry and
 * that it paints source art. Presentation only: no simulation state is read
 * back from the overlay. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_SHIELD_FLASH_ENABLED
#error "Compile this fixture with a race product's flags and a shield-overlay weapon bank"
#endif

static int zealot, marine;

static void
fixture(void)
{
    game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init();
    screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    sc64_shield_flash_reset();
    sc64_facing_reset_all();
    zealot = spawn_unit(ENEMY, U_ZEALOT, 300, 200);
    marine = spawn_unit(SIDE, U_MARINE, 240, 200);       /* due west of the Zealot */
    assert(zealot >= 0 && marine >= 0);
    assert(u[zealot].shield > 10 && u[marine].shield == 0);
    camx = 300 - 120; camy = 200 - 54;
    memset(visible, 0xff, sizeof(visible)); memset(explored, 0xff, sizeof(explored));
}

static int
live_flashes(void)
{
    int n = 0;
    for (unsigned i = 0; i < SC64_SHIELD_FLASH_SLOTS; ++i) n += sc64_shield_flashes[i].live;
    return n;
}

static const Sc64ShieldFlash *
last_flash(void)
{
    return &sc64_shield_flashes[(sc64_shield_flash_next + SC64_SHIELD_FLASH_SLOTS - 1u) % SC64_SHIELD_FLASH_SLOTS];
}

static void
marine_hits(int target, int damage)
{
    SC64_SHIELD_HIT_FROM(u[marine].x, u[marine].y, target);
    hit_target(target, damage, U_MARINE, SIDE);
    SC64_SHIELD_HIT_DONE();
}

/* The OpenBW rule: a flash only when shields took damage and some remain. */
static void
starts_only_on_absorbed_hits_that_leave_shields(void)
{
    fixture();
    marine_hits(zealot, 6);
    assert(live_flashes() == 1 && last_flash()->actor == zealot);
    u[zealot].shield = 4;
    marine_hits(zealot, 6);                 /* breaks the shield: no flash */
    assert(u[zealot].shield == 0 && live_flashes() == 1);
    marine_hits(zealot, 6);                 /* hull damage only */
    assert(live_flashes() == 1);
    marine_hits(marine, 6);                 /* a target without shields */
    assert(live_flashes() == 1);
}

/* The flash sits on the side facing the attacker. A Marine to the west
 * (heading east, 32-way 8) gives offset index 24, left of the Zealot. */
static void
faces_the_attacker(void)
{
    int16_t x, y;
    int image;
    fixture();
    assert(sc64_shield_actor_row(zealot) && sc64_shield_actor_row(zealot)->directional);
    image = sc64_shield_actor_row(zealot)->image_id;
    marine_hits(zealot, 6);
    /* Index 24 of the ROM table, halved for the half-scale world. */
    assert(sc64_weapon_source_lo_lookup((uint16_t)image, 5u, 0u, 24u, &x, &y) && x < 0);
    assert(last_flash()->dx == x / 2 && last_flash()->dy == y / 2);
    /* From the north (heading south, 16): offset index 0, above the unit. */
    u[marine].x = 300; u[marine].y = 120;
    marine_hits(zealot, 6);
    assert(sc64_weapon_source_lo_lookup((uint16_t)image, 5u, 0u, 0u, &x, &y) && y < 0);
    assert(last_flash()->dx == x / 2 && last_flash()->dy == y / 2);
    /* Resolved at the hit: turning the unit afterwards does not move it. */
    SC64_FACING_SET(zealot, 24);
    assert(last_flash()->dy == y / 2);
}

/* A new hit takes a free slot rather than cutting a live overlay short. */
static void
prefers_free_slots(void)
{
    fixture();
    for (int i = 0; i < (int)SC64_SHIELD_FLASH_SLOTS; ++i) marine_hits(zealot, 6);
    sc64_shield_flashes[2].live = 0;
    sc64_shield_flash_next = 5;
    marine_hits(zealot, 6);
    assert(sc64_shield_flashes[2].live && live_flashes() == (int)SC64_SHIELD_FLASH_SLOTS);
    /* Full: the oldest overlay (slot 0, born first) is replaced, not slot 3. */
    /* Full, all started on this same update: each extra hit replaces the
     * oldest insertion, so the last eight hits survive. */
    fixture();
    for (int i = 0; i < 10; ++i) { u[zealot].shield = 60; marine_hits(zealot, 6); }
    {
        u8 newest = (u8)(sc64_shield_flash_seq - 1u);
        for (unsigned k = 0; k < SC64_SHIELD_FLASH_SLOTS; ++k)
            assert(sc64_shield_flashes[k].live && (u8)(newest - sc64_shield_flashes[k].seq) < SC64_SHIELD_FLASH_SLOTS);
    }
}

/* A queued impact keeps its own launch direction even when a later launch
 * from another side lands first; a row the directions did not follow falls
 * back to the target's facing. */
static void
queued_impacts_keep_their_launch_direction(void)
{
    fixture();
    SC64_SHIELD_NOTE_LAUNCH(zealot, 240, 200);                 /* west, slow */
    race_impact_launch(zealot, 6, 1, SIDE, 0, U_DRAGOON, 5);
    SC64_SHIELD_NOTE_LAUNCH(zealot, 300, 120);                 /* north, fast */
    race_impact_launch(zealot, 6, 1, SIDE, 0, U_DRAGOON, 2);
    race_impact_tick(); race_impact_tick();
    assert(live_flashes() == 1 && last_flash()->dx == 0 && last_flash()->dy < 0);          /* north */
    race_impact_tick(); race_impact_tick(); race_impact_tick();
    assert(live_flashes() == 2 && last_flash()->dx < 0);                                   /* west */
    /* A row changed behind the directions' back (as a save load does). */
    SC64_SHIELD_NOTE_LAUNCH(zealot, 240, 200);
    race_impact_launch(zealot, 6, 1, SIDE, 0, U_DRAGOON, 1);
    race_impact_dir_target[race_impact_count - 1u] = (u16)marine;
    race_impact_tick();
    assert(live_flashes() == 3 && last_flash()->dx == 0 && last_flash()->dy < 0);          /* facing north: index 0 */
}

/* Buildings with shields flash; their image is not directional. */
static void
buildings_flash(void)
{
    int cannon;
    fixture();
    cannon = spawn_build(ENEMY, B_PHOTON_CANNON, 360, 200, 0);
    assert(cannon >= 0);
    b[cannon].shield = 50;
    marine_hits(MAX_UNITS + cannon, 6);
    assert(live_flashes() == 1 && last_flash()->actor == MAX_UNITS + cannon);
    assert(sc64_shield_actor_row(MAX_UNITS + cannon) && !sc64_shield_actor_row(MAX_UNITS + cannon)->directional);
}

/* Bounded slots, slot-reuse guard and expiry after the source Init stream. */
static void
bounded_reused_and_expiring(void)
{
    const Sc64WeaponSourceAnimation *init =
        sc64_weapon_source_animation_lookup(SC64_WEAPON_SOURCE_SHIELD_OVERLAY_IMAGE, 0u);
    assert(init && init->duration_ticks >= 1u);
    fixture();
    for (int i = 0; i < 20; ++i) { u[zealot].shield = 60; marine_hits(zealot, 6); }
    assert(live_flashes() == (int)SC64_SHIELD_FLASH_SLOTS);
    gfx_begin(C_BLACK);
    sc64_shield_flash_prepare();
    assert(live_flashes() == (int)SC64_SHIELD_FLASH_SLOTS);
    frame += (int)sc64_weapon_source_vfx_gba_duration(init->duration_ticks);
    sc64_shield_flash_prepare();
    assert(live_flashes() == 0);

    fixture();
    marine_hits(zealot, 6);
    u[zealot].type = U_DRAGOON;            /* the slot now holds another actor */
    sc64_shield_flash_prepare();
    assert(live_flashes() == 0);

    /* A same-type actor spawned into a recycled slot inherits nothing. */
    fixture();
    marine_hits(zealot, 6);
    u[zealot].alive = 0;
    assert(spawn_unit(ENEMY, U_ZEALOT, 300, 200) == zealot);
    assert(live_flashes() == 0);
}

/* A live flash paints source pixels near the Zealot. */
static void
paints_source_art(void)
{
    static u8 before[240 * 160], after[240 * 160];
    int changed = 0;
    fixture();
    gfx_begin(C_BLACK);
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) before[y * 240 + x] = gfx_test_pixel(x, y);
    marine_hits(zealot, 6);
    sc64_shield_flash_prepare();
    sc64_shield_flash_draw_actor(zealot);                 /* the Zealot was not drawn: nothing */
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) assert(gfx_test_pixel(x, y) == before[y * 240 + x]);
    sc64_shield_mark_drawn(zealot);
    sc64_shield_flash_draw_actor(zealot);
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) {
        after[y * 240 + x] = gfx_test_pixel(x, y);
        if (after[y * 240 + x] != before[y * 240 + x]) {
            ++changed;
            assert(x >= 120 - 40 && x <= 120 + 40 && y >= 66 - 40 && y <= 66 + 40);
        }
    }
    assert(changed > 0);
}

/* Empty frames do not touch the cached index; offscreen and recycled slots
 * are pruned at preparation, independently of actor admission. */
static void
preparation_fast_path_and_offscreen_expiry(void)
{
    const Sc64WeaponSourceAnimation *init =
        sc64_weapon_source_animation_lookup(SC64_WEAPON_SOURCE_SHIELD_OVERLAY_IMAGE, 0u);
    fixture();
    sc64_shield_affected[zealot >> 3] = 0xa5;
    sc64_shield_drawn[zealot >> 3] = 0x5a;
    sc64_shield_flash_prepare();
    sc64_shield_flash_draw_actor(zealot);
    assert(sc64_shield_affected[zealot >> 3] == 0xa5);
    assert(sc64_shield_drawn[zealot >> 3] == 0x5a);
    marine_hits(zealot, 6);
    sc64_shield_flash_prepare();
    assert(sc64_shield_affected[zealot >> 3] & (1u << (zealot & 7)));
    assert(!(sc64_shield_affected[marine >> 3] & (1u << (marine & 7))));
    u[zealot].x = -1000;
    frame += (int)sc64_weapon_source_vfx_gba_duration(init->duration_ticks);
    sc64_shield_flash_prepare();
    assert(!sc64_shield_live_mask && !live_flashes());
    fixture();
    marine_hits(zealot, 6);
    sc64_shield_flash_prepare();
    sc64_shield_flash_clear_actor(zealot);
    assert(!sc64_shield_live_mask && !live_flashes());
}

/* Compare four actual painter renders. Pixels covered by the foreground body
 * must remain identical with and without the rear actor's shield flash. */
static void
foreground_occludes_rear_flash(void)
{
    static u8 rear[240 * 160], rear_flash[240 * 160], full[240 * 160];
    int covered = 0;
    fixture();
    u[marine].x = 292; u[marine].y = 201;
    marine_hits(zealot, 6);
    for (int pass = 0; pass < 4; ++pass) {
        u[marine].alive = pass >= 2;
        sc64_shield_live_mask = (u8)(pass & 1);
        gfx_begin(C_BLACK);
        sc64_shield_flash_prepare();
        sc64_draw_depth_ordered_actors();
        for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x) {
            int at = y * 240 + x;
            u8 pixel = gfx_test_pixel(x, y);
            if (pass == 0) rear[at] = pixel;
            else if (pass == 1) rear_flash[at] = pixel;
            else if (pass == 2) full[at] = pixel;
            else if (rear[at] != rear_flash[at] && full[at] != rear[at]) {
                ++covered;
                assert(pixel == full[at]);
            }
        }
    }
    assert(covered > 0);
}

int
main(void)
{
    gba_init();
    starts_only_on_absorbed_hits_that_leave_shields();
    faces_the_attacker();
    queued_impacts_keep_their_launch_direction();
    buildings_flash();
    prefers_free_slots();
    bounded_reused_and_expiring();
    paints_source_art();
    preparation_fast_path_and_offscreen_expiry();
    foreground_occludes_rear_flash();
    puts("PASS shield hit overlay: absorbed hits only, attacker side, queued directions, buildings, bounded, reuse, expiry, drawn-only source art");
    return 0;
}
