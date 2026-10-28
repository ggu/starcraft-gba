/* Opt-in full Z/P storage probe. This is an actual-engine host test for the
 * compact runtime pools and transactional generation-22 save schema. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_ZP_FULL_STORAGE
#error "compile with SC64_ZP_FULL_STORAGE"
#endif

_Static_assert(MAX_UNITS == 240, "full probe unit capacity");
_Static_assert(MAX_BUILDINGS == 160, "full probe building capacity");
_Static_assert(MAX_MINERALS == 144, "full probe mineral capacity");
_Static_assert(MAX_RACE_GEYSERS == 12, "full probe geyser capacity");
_Static_assert(sizeof(Unit) == 56, "full probe leaves Unit ABI unchanged");
_Static_assert(sizeof(Building) == 48, "full probe compact Building size");
_Static_assert(sizeof(Sc64ZpPackedUnitSave) == 35, "full probe Unit save row");
_Static_assert(sizeof(Sc64PackedMineralSave) == 5, "full probe Mineral save row");
_Static_assert(sizeof(RaceSave) <= 16368, "full probe save fits alternating SRAM payload");

static u16 prior;
static Unit units_before[MAX_UNITS];
static Building buildings_before[MAX_BUILDINGS];
static Mineral minerals_before[MAX_MINERALS];

static void input(u16 keys) {
    game_update(keys, keys & (u16)~prior);
    prior = keys;
}
static void press(u16 key) { input(key); input(0); }
static void launch_p4(void) {
    game_init(); prior = 0;
    press(KEY_A);             /* Protoss mission selection */
    press(KEY_RIGHT);         /* page 2 */
    press(KEY_DOWN);          /* mission 23 */
    press(KEY_A);
    input(KEY_START);         /* explicitly deploy the mission */
    assert(screen == SCREEN_PLAY && !race_mode && race_selected_mission == 23);
    assert(race_p4_active());
    unsigned live_units = 0, live_buildings = 0, live_minerals = 0;
    for (int i = 0; i < MAX_UNITS; ++i) live_units += u[i].alive != 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) live_buildings += b[i].alive != 0;
    for (int i = 0; i < MAX_MINERALS; ++i) live_minerals += m[i].amount > 0;
    assert(live_units == 137 && live_buildings == 33 && live_minerals == 26);
    assert(live_units + SC64_P4_SOURCE_CREATE_COUNT <= MAX_UNITS);
    geyser_count = MAX_RACE_GEYSERS;
    for (int i = 0; i < MAX_RACE_GEYSERS; ++i) {
        geysers[i].x = (i * 8) % (race_cols * 16);
        geysers[i].y = (i * 12) % (race_rows * 16);
    }
    invincible_units[0] = 1; /* Exercise packed one-bit persistence. */
}
static void snapshot_world(void) {
    memcpy(units_before, u, sizeof(u));
    memcpy(buildings_before, b, sizeof(b));
    memcpy(minerals_before, m, sizeof(m));
}
static void assert_world_unchanged(void) {
    assert(!memcmp(units_before, u, sizeof(u)));
    assert(!memcmp(buildings_before, b, sizeof(b)));
    assert(!memcmp(minerals_before, m, sizeof(m)));
}

int main(void) {
    launch_p4();
    assert(race_save_game());
    assert(race_data.version == SAVE_VERSION);
    assert(SC64_RACE_SAVE_GENERATION == 22u);
    RaceSave expected = race_data;
    snapshot_world();
    assert(race_load_game());
    assert(!memcmp(units_before, u, sizeof(u)));
    assert(!memcmp(buildings_before, b, sizeof(b)));
    assert(!memcmp(minerals_before, m, sizeof(m)));
    assert(race_save_game());
    assert(!memcmp(&expected, &race_data, sizeof(expected)));

    /* Invalid live save inputs fail before the SRAM writer, preserving the
     * previous valid generation-22 record byte for byte. */
    RaceSave retained;
    invincible_units[0] = 2;
    assert(!race_save_game());
    assert(save_read(&retained, sizeof(retained), SAVE_VERSION));
    assert(!memcmp(&expected, &retained, sizeof(expected)));
    invincible_units[0] = 1;
    int old_geyser_count = geyser_count;
    geyser_count = MAX_RACE_GEYSERS + 1;
    assert(!race_save_game());
    assert(save_read(&retained, sizeof(retained), SAVE_VERSION));
    assert(!memcmp(&expected, &retained, sizeof(expected)));
    geyser_count = old_geyser_count;
    int old_geyser_x = geysers[0].x;
    geysers[0].x = race_cols * 16;
    assert(!race_save_game());
    assert(save_read(&retained, sizeof(retained), SAVE_VERSION));
    assert(!memcmp(&expected, &retained, sizeof(expected)));
    geysers[0].x = old_geyser_x;

    /* Corrupt a reserved building-row bit in the final slot. Validation must
     * reject before applying any earlier valid rows to the live world. */
    RaceSave corrupt = race_data;
    corrupt.buildings[MAX_BUILDINGS - 1].bytes[27] |= 0x80u;
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    snapshot_world();
    assert(!race_load_game());
    assert_world_unchanged();

    /* A correctly tagged generation-22 record with an invalid high-capacity
     * geyser coordinate must fail before live state is changed. */
    corrupt = expected;
    corrupt.geysers[MAX_RACE_GEYSERS - 1].x = race_cols * 16;
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    snapshot_world();
    assert(!race_load_game());
    assert_world_unchanged();

    /* Historical generation tags cannot be loaded as generation 22. */
    corrupt = expected;
    corrupt.version = (u32)(0x53435030u + 20u);
    save_write(&corrupt, sizeof(corrupt), corrupt.version);
    snapshot_world();
    assert(!race_load_game());
    assert_world_unchanged();

    /* A current tag cannot make a legacy-sized/truncated payload look like
     * the new generation-22 record. save_read checks the exact record length. */
    u32 old_payload_with_current_tag = SAVE_VERSION;
    save_write(&old_payload_with_current_tag, sizeof(old_payload_with_current_tag),
               SAVE_VERSION);
    snapshot_world();
    assert(!race_load_game());
    assert_world_unchanged();
    printf("Z/P full-storage probe passed: units=%d buildings=%d minerals=%d geysers=%d Building=%zu Unit=%zu RaceSave=%zu\n", MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS, MAX_RACE_GEYSERS, sizeof(Building), sizeof(Unit), sizeof(RaceSave));
    return 0;
}
