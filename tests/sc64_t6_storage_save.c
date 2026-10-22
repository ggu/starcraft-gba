/* Combined T5-storage profile regression: exercise packed 80-slot building
 * persistence through the actual engine save/load path. This is an internal
 * fixture; it does not assert T6 mission playability. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_T6_STORAGE
#error "Build this fixture with SC64_T6_STORAGE"
#endif

_Static_assert(MAX_UNITS == 160, "combined profile keeps the 160-unit cap");
_Static_assert(MAX_BUILDINGS == 80, "combined T6 storage keeps 80 building slots");
_Static_assert(MAX_MINERALS == 48, "combined profile keeps 48 mineral fields");
_Static_assert(sizeof(Sc64PackedBuildingSave) == 28, "compact building row budget");
_Static_assert(sizeof(RaceSave) <= 16368, "packed save fits alternating SRAM record");

static Building expected_buildings[MAX_BUILDINGS];
static Unit expected_units[MAX_UNITS];
static RaceSave valid_save;

static int building_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) count += b[i].alive != 0;
    return count;
}

static void start_t5(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active() && screen == SCREEN_PLAY);
    assert(race_cols == 96 && race_rows == 96);
    for (int i = 0; i < 35; ++i) game_update(0, 0);
}

static void fill_building_capacity(void)
{
    int next = 0;
    while (building_count() < MAX_BUILDINGS) {
        int owner = next % 5;
        int x = 64 + (next % 12) * 80;
        int y = 720 + ((next / 12) % 8) * 64;
        if (x >= race_cols * 16) x = race_cols * 16 - 16;
        if (y >= race_rows * 16) y = race_rows * 16 - 16;
        int slot = spawn_build(owner, B_DEPOT, x, y, 0);
        if (slot < 0) break;
        ++next;
    }
    assert(building_count() == 80);
}

static void exercise_live_fields(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building *q = &b[i];
        if (!q->alive) continue;
        /* Keep source identity and type intact; vary every compacted mutable
         * field within its validated width. Queue entries remain inactive so
         * mission-specific PUNI restrictions are not bypassed. */
        q->queue_head = i % 5;
        q->queue_count = 0;
        for (int j = 0; j < 5; ++j) q->queue[j] = j;
        /* The codec can represent the full u16 counter, but the engine's
         * candidate validator enforces the live 28-frame queue cadence. */
        q->queue_progress = (i & 1) ? 28 : 0;
        q->build_progress = (i & 1) ? -1 : 65534;
        q->under_construction = 0;
        if (q->type != B_CONTROL_TOWER) q->level = i % 81;
        q->shield = sc64_race_t4_building_intrinsically_invincible(q->type) ? 0 :
                    ((i & 1) ? 131071 : 0);
        q->flags = (i & 1) ? 65535 : 0;
        q->larva = i % 4;
        q->larva_timer = i % 512;
        q->shield_remainder = i % 1280;
    }
}

static void remember_world(void)
{
    memcpy(expected_buildings, b, sizeof(b));
    memcpy(expected_units, u, sizeof(u));
}

static void assert_world_restored(void)
{
    assert(!memcmp(expected_buildings, b, sizeof(b)));
    assert(!memcmp(expected_units, u, sizeof(u)));
}

static void test_roundtrip_and_dead_slot_reference(void)
{
    int dead_slot = MAX_BUILDINGS - 1;
    start_t5();
    fill_building_capacity();
    exercise_live_fields();

    /* Keep a dead stable slot referenced by an ordinary order. T4 scenario
     * validation still depends on raw identity/position for dead props, so the
     * compact encoding preserves all fields rather than canonicalizing them. */
    assert(dead_slot >= 0 && b[dead_slot].alive);
    b[dead_slot].hp = 1;
    b[dead_slot].shield = 0;
    b[dead_slot].flags = 0;
    hit_target(MAX_UNITS + dead_slot, 1000, U_MARINE, SIDE);
    assert(!b[dead_slot].alive && b[dead_slot].hp < 0);
    if (MAX_UNITS > 0 && u[0].alive) {
        u[0].order = ORDER_GAS;
        u[0].target = dead_slot;
    }
    remember_world();
    assert(race_save_game());
    valid_save = race_data;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        Building decoded;
        assert(sc64_building_save_decode(&decoded, &valid_save.buildings[i]));
        assert(!memcmp(&decoded, &expected_buildings[i], sizeof(decoded)));
    }

    memset(b, 0, sizeof(b));
    memset(u, 0, sizeof(u));
    assert(race_load_game());
    assert_world_restored();
    assert(!b[dead_slot].alive && b[dead_slot].raw_type == expected_buildings[dead_slot].raw_type &&
           b[dead_slot].x == expected_buildings[dead_slot].x &&
           b[dead_slot].hp == expected_buildings[dead_slot].hp && b[dead_slot].hp < 0);
    if (expected_units[0].alive) {
        assert(u[0].order == ORDER_GAS && u[0].target == dead_slot);
    }
}

static void reject_atomically(const RaceSave *bad, u32 version)
{
    remember_world();
    save_write(bad, sizeof(*bad), version);
    /* The two-slot SRAM reader may still find a current-version backup when
     * probing an old tag. Fill both slots with the old tag to test rejection. */
    if (version != SAVE_VERSION) save_write(bad, sizeof(*bad), version);
    {
        RaceSave readback;
        assert(save_read(&readback, sizeof(readback), version));
        assert(!memcmp(&readback, bad, sizeof(readback)));
    }
    assert(!race_load_game());
    assert_world_restored();
    save_write(&valid_save, sizeof(valid_save), SAVE_VERSION);
}

static void test_bad_rows_and_old_version_are_atomic(void)
{
    RaceSave bad;
    start_t5();
    fill_building_capacity();
    exercise_live_fields();
    assert(race_save_game());
    valid_save = race_data;
    remember_world();

    bad = valid_save;
    bad.buildings[0].bytes[27] |= 0x80; /* Reserved tail bit. */
    {
        Building rejected;
        assert(!sc64_building_save_decode(&rejected, &bad.buildings[0]));
    }
    reject_atomically(&bad, SAVE_VERSION);

    bad = valid_save;
    bad.buildings[79].bytes[27] |= 0x40; /* Reserved tail bit. */
    reject_atomically(&bad, SAVE_VERSION);

    bad = valid_save;
    bad.version = SAVE_VERSION - 1;
    reject_atomically(&bad, SAVE_VERSION - 1);
}

int main(void)
{
    test_roundtrip_and_dead_slot_reference();
    test_bad_rows_and_old_version_are_atomic();
    printf("T6 storage save ok: %zu-byte RaceSave, %d stable building slots\n",
           sizeof(RaceSave), MAX_BUILDINGS);
    return 0;
}
