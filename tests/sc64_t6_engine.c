/* Full-roster T6 world initialization and neutral-controller smoke checks.
 * This is not an ordinary-controller campaign completion proof. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

int main(void)
{
    unsigned units = 0, buildings = 0, minerals_count = 0, burrowed = 0;
    int raynor = -1, norad = -1;
    game_init();
    race_selected_mission = SC64_T6_GLOBAL_ID;
    race_mode = 0;
    init_state();
    assert(race_t6_active() && screen == SCREEN_PLAY);
    assert(race_cols == 96 && race_rows == 96);
    assert(MAX_UNITS == 160 && MAX_BUILDINGS == 80 && MAX_MINERALS == 48);
    assert(sizeof(RaceSave) == 15644 && SC64_RACE_SAVE_GENERATION == 16u);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        ++units;
        burrowed += race_t6_unit_burrowed(i) != 0;
        if (u[i].raw_type == 19) raynor = i;
        if (u[i].raw_owner == 6 || u[i].raw_owner == 2)
            assert(u[i].owner == SIDE || u[i].owner == RESCUABLE);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        ++buildings;
        if (b[i].raw_type == 126) norad = i;
    }
    for (int i = 0; i < MAX_MINERALS; ++i)
        minerals_count += m[i].amount > 0;
    assert(units == 104 && buildings == 74 && minerals_count == 35);
    assert(geyser_count == 3 && !mission_unsupported_count);
    assert(burrowed == 4);
    assert(raynor >= 0 && u[raynor].type == U_RAYNOR_VULTURE && u[raynor].raw_owner == 2);
    assert(norad >= 0 && b[norad].type == B_CRASHED_NORAD_II && b[norad].raw_owner == 2);
    assert(!owners_hostile(SIDE, RESCUABLE));
    assert(!owners_hostile(ENEMY, 4));
    assert(owners_hostile(SIDE, ENEMY) && owners_hostile(SIDE, 4));
    for (unsigned tick = 0; tick < 300 && screen == SCREEN_PLAY; ++tick)
        game_update(0, 0);
    if (screen != SCREEN_PLAY || frame != 300) {
        fprintf(stderr, "T6 smoke stopped: frame=%d screen=%d phase=%u won=%d lost=%d\n",
                frame, screen, race_campaign.phase, won, lost);
        for (unsigned owner = 0; owner < 8; ++owner) {
            const Sc64T4TriggerState *p = &race_t6.triggers.player[owner];
            fprintf(stderr, "owner%u clock%u fault%u trigger%u action%u outcome%u\n",
                    owner, p->elapsed_ticks, p->fault, p->fault_trigger,
                    p->fault_slot, p->outcome);
        }
    }
    assert(screen == SCREEN_PLAY && frame == 300);
    assert(race_campaign.elapsed_frames == (u32)frame);
    for (unsigned owner = 0; owner < 8; ++owner) {
        assert(!race_t6.triggers.player[owner].fault);
        assert(race_t6.triggers.player[owner].elapsed_ticks == (u32)frame);
    }
    assert(!won && !lost);
    puts("T6 engine: complete source roster, ownership, burrow state and 300 live ticks pass");
    return 0;
}
