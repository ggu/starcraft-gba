/* Actual-engine controls, target eligibility and transactional cloak saves.
 * Synthetic actor positions isolate detector boundaries; this is not a
 * controller win or native N64 execution equivalence test. */
#define main sc64_z5_base_runtime_checks
#include "sc64_z5_runtime.c"
#undef main

#ifndef SC64_Z5_CLOAK_ENABLED
#error "Compile with the optional Z5 cloak profile"
#endif

static int selected_hero(void)
{
    int hero = race_z5_cloak_hero_slot();
    assert(hero >= 0);
    clear_selection();
    u[hero].selected = 1;
    return hero;
}

static void open_command(unsigned row)
{
    press(KEY_B);
    assert(menu_open && menu_cursor == 0);
    for (unsigned i = 0; i < row; ++i) press(KEY_DOWN);
    assert(menu_cursor == (int)row);
}

static void controls_energy_pause_and_stop(void)
{
    launch_z5_from_normal_menu();
    int hero = selected_hero();
    assert(u[hero].carry == 64000 && !race_z5.cloak.active);
    assert(race_z5_cloak_menu_available() && field_menu_count() == 11);
    open_command(MENU_STOP + 1);
    int before = u[hero].carry;
    input(KEY_A);
    assert(!menu_open && race_z5.cloak.active);
    assert(u[hero].carry == before - 6400);
    input(0);
    assert(u[hero].carry == before - 6400 - 8);

    open_command(MENU_STOP);
    before = u[hero].carry;
    input(KEY_A);
    assert(u[hero].order == ORDER_IDLE && u[hero].carry == before);
    input(0);
    assert(u[hero].carry == before - 8);

    input(KEY_START);
    assert(screen == SCREEN_PAUSE);
    before = u[hero].carry;
    Sc64Z5CloakState frozen = race_z5.cloak;
    for (unsigned i = 0; i < 120; ++i) input(0);
    assert(u[hero].carry == before &&
           memcmp(&frozen, &race_z5.cloak, sizeof(frozen)) == 0);
    input(KEY_A); /* Resume is A in the pause menu. */
    assert(screen == SCREEN_PLAY && u[hero].carry == before);
    input(0);
    assert(u[hero].carry == before - 8);

    open_command(MENU_STOP + 1);
    input(KEY_A);
    assert(!race_z5.cloak.active);
    before = u[hero].carry;
    for (unsigned i = 0; i < 5; ++i) input(0);
    assert(u[hero].carry == before + 32 && !race_z5.cloak.regen_remainder);
    /* A denied activation leaves energy and state intact. */
    u[hero].carry = 6399;
    open_command(MENU_STOP + 1);
    before = u[hero].carry;
    input(KEY_A);
    assert(!race_z5.cloak.active && u[hero].carry == before);
    input(0);
    u[hero].carry = 64000;
    race_z5.cloak.regen_remainder = 0;
    assert(race_z5_cloak_world_state_valid());
}

static void detector_target_eligibility(void)
{
    launch_z5_from_normal_menu();
    int hero = selected_hero(), row = -1;
    u8 enabled[SC64_Z5_DOODAD_CAPACITY];
    int old_x = u[hero].x, old_y = u[hero].y;
    for (unsigned i = 0; i < SC64_Z5_DOODAD_CAPACITY; ++i) {
        enabled[i] = race_z5_doodads[i].enabled;
        race_z5_doodads[i].enabled = 0;
        if (row < 0 && sc64_z5_detector_raw(race_z5_doodads[i].raw_type))
            row = (int)i;
    }
    assert(row >= 0 && race_can_attack(U_MARINE, hero));
    race_z5.cloak.active = 1;
    assert(!race_can_attack(U_MARINE, hero));
    Sc64Z5DoodadState *d = &race_z5_doodads[row];
    Building *trap = &b[d->building_slot];
    u[hero].x = trap->x; u[hero].y = trap->y;
    d->enabled = 1;
    assert(race_can_attack(U_MARINE, hero));
    int old_hp = trap->hp;
    trap->alive = 0; trap->hp = 0;
    assert(!race_can_attack(U_MARINE, hero));
    trap->alive = 1; trap->hp = old_hp;
    d->enabled = 0;
    assert(!race_can_attack(U_MARINE, hero));
    race_z5.cloak.active = 0;
    assert(race_can_attack(U_MARINE, hero));
    u[hero].x = old_x; u[hero].y = old_y;
    for (unsigned i = 0; i < SC64_Z5_DOODAD_CAPACITY; ++i)
        race_z5_doodads[i].enabled = enabled[i];
}

static void reject_candidate_without_world_changes(const RaceSave *bad)
{
    Unit old_units[MAX_UNITS]; Building old_buildings[MAX_BUILDINGS];
    memcpy(old_units, u, sizeof(u)); memcpy(old_buildings, b, sizeof(b));
    Sc64Z5PersistentState old_z5 = race_z5;
    Sc64RaceCampaignState old_campaign = race_campaign;
    int old_frame = frame, old_screen = screen;
    save_write(bad, sizeof(*bad), SAVE_VERSION);
    assert(!race_load_game());
    assert(memcmp(old_units, u, sizeof(u)) == 0);
    assert(memcmp(old_buildings, b, sizeof(b)) == 0);
    assert(memcmp(&old_z5, &race_z5, sizeof(old_z5)) == 0);
    assert(memcmp(&old_campaign, &race_campaign, sizeof(old_campaign)) == 0);
    assert(frame == old_frame && screen == old_screen);
}

static void cloak_save_roundtrip_and_rejection(void)
{
    launch_z5_from_normal_menu();
    int hero = selected_hero();
    race_z5.cloak.active = 1; race_z5.cloak.regen_remainder = 0;
    u[hero].carry = 50000;
    assert(SC64_RACE_SAVE_GENERATION == 25u && race_save_game());
    RaceSave good = race_data;
    assert(race_load_game() && race_z5.cloak.active && u[hero].carry == 50000);
    assert(race_save_game() && memcmp(&good, &race_data, sizeof(good)) == 0);

    u[hero].carry = 64001;
    assert(!race_save_game());
    RaceSave intact;
    assert(save_read(&intact, sizeof(intact), SAVE_VERSION));
    assert(memcmp(&good, &intact, sizeof(good)) == 0);
    u[hero].carry = 50000;

    RaceSave bad = good;
    bad.z5.cloak.regen_remainder = 1; /* Active cloak has no regen phase. */
    reject_candidate_without_world_changes(&bad);
    bad = good;
    Unit decoded;
    assert(race_save_decode_unit(&bad, (unsigned)hero, WORLD_W, WORLD_H, &decoded));
    decoded.carry = 64001; /* Codec-representable, above the hero energy cap. */
    assert(sc64_zp_unit_save_encode(&bad.units[hero], &decoded, WORLD_W, WORLD_H));
    reject_candidate_without_world_changes(&bad);
    bad = good;
    bad.version = 0x53435a30u + 23u;
    assert(bad.version != SAVE_VERSION);
    reject_candidate_without_world_changes(&bad);
    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game());

    /* A genuine terminal hero death clears active cloak on its next tick. */
    u[hero].alive = 0; u[hero].hp = 0;
    assert(race_z5_cloak_tick() && !race_z5.cloak.active &&
           !race_z5.cloak.regen_remainder);
}

int main(void)
{
    assert(sc64_z5_base_runtime_checks() == 0);
    controls_energy_pause_and_stop();
    detector_target_eligibility();
    cloak_save_roundtrip_and_rejection();
    puts("Z5 cloak controls, detector eligibility, energy and transactional saves passed");
    return 0;
}
