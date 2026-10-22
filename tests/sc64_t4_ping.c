/* Internal framebuffer fixture for the T4 minimap ping policy. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u8 frame_pixels[240 * 160] __attribute__((aligned(4)));

static void render_minimap(void)
{
    gfx_begin(C_BLACK);
    draw_minimap();
    assert(gfx_read_band(0, 160, frame_pixels));
}

static u8 pixel_at(int x, int y)
{
    assert(x >= 0 && x < 240 && y >= 0 && y < 160);
    return frame_pixels[y * 240 + x];
}

static void reset_fixture(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(&race_t4, 0, sizeof(race_t4));
    memset(sc64_minimap_visible, 0, sizeof(sc64_minimap_visible));
    memset(sc64_minimap_explored, 0, sizeof(sc64_minimap_explored));
    race_mode = 0;
    race_selected_mission = SC64_T4_GLOBAL_ID;
    race_cols = race_rows = 128;
    screen = SCREEN_PLAY;
    won = lost = 0;
    camx = camy = 0;
}

static void test_scaled_pulse_and_live_gate(void)
{
    const int map_x = 197, map_y = 129;
    const int ping_x = 512, ping_y = 768;
    const int px = (ping_x * 3) / 256, py = (ping_y * 3) / 256;

    reset_fixture();
    race_t4.ping_x = ping_x;
    race_t4.ping_y = ping_y;
    race_t4.ping_ticks = 8; /* radius one */
    render_minimap();
    assert(pixel_at(map_x + px, map_y + py) == C_WHITE);
    assert(pixel_at(map_x + px + 1, map_y + py) == C_GOLD);
    assert(pixel_at(map_x + px + 2, map_y + py) == C_BLACK);

    race_t4.ping_ticks = 16; /* radius two */
    render_minimap();
    assert(pixel_at(map_x + px, map_y + py) == C_WHITE);
    assert(pixel_at(map_x + px + 1, map_y + py) == C_GOLD);
    assert(pixel_at(map_x + px + 2, map_y + py) == C_GOLD);

    race_selected_mission = 0;
    render_minimap();
    assert(pixel_at(map_x + px, map_y + py) == C_BLACK);

    race_selected_mission = SC64_T4_GLOBAL_ID;
    screen = SCREEN_PAUSE;
    render_minimap();
    assert(pixel_at(map_x + px, map_y + py) == C_BLACK);
}

static void test_edge_clipping_and_expiry(void)
{
    reset_fixture();
    race_t4.ping_ticks = 1;
    race_t4.ping_x = race_t4.ping_y = 0;
    render_minimap();
    assert(pixel_at(197, 129) == C_WHITE);
    assert(pixel_at(196, 129) == C_BLACK);
    assert(pixel_at(197, 128) == C_BLACK);

    race_t4.ping_ticks = 0;
    render_minimap();
    assert(pixel_at(197, 129) == C_HIGHLIGHT); /* viewport frame only */
}

int main(void)
{
    test_scaled_pulse_and_live_gate();
    test_edge_clipping_and_expiry();
    puts("T4 minimap ping policy fixtures passed");
    return 0;
}
