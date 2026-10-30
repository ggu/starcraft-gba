/* Optional prepared-asset full-scene differential for dirty-edge transfers. */
#define main sc64_original_frame_coverage_main
#include "sc64_z3_frame_coverage.c"
#undef main

int main(void)
{
    unsigned cases = 0, partials = 0;
    for (int py = 0; py < 16; ++py)
        for (int px = 0; px < 16; ++px)
            for (int side = 0; side < 2; ++side) {
                setup_profile();
                memset(visible, 0xff, sizeof(visible));
                memset(explored, 0xff, sizeof(explored));
                int left = spawn_build(SIDE, B_BARRACKS, 400, 512, 0);
                int right = spawn_build(SIDE, B_BARRACKS, 624, 512, 0);
                assert(left >= 0 && right >= 0);
                m[0].x = 624; m[0].y = 512; m[0].amount = 1000;
                m[1].x = 400; m[1].y = 512; m[1].amount = 1000;
                cx = 512 + px; cy = 512 + py;
                sc64_terrain_cache_valid = 0;
                sc64_hud_valid = 0;
                game_render();
                cx += side ? -4 : 4;
                unsigned scenario = (unsigned)(px + 3 * py + side) % 4u;
                if (scenario == 1) {
                    int x = cx / 16 + 5, y = cy / 16 + 5;
                    SC64_GRID_SET(visible, x + y * MAP_COLS, 0);
                    SC64_GRID_SET(explored, x + y * MAP_COLS, 0);
                } else if (scenario == 2) {
                    b[left].under_construction = 1;
                    b[left].build_progress = 5;
                } else if (scenario == 3) {
                    m[0].amount = 0;
                }
                unsigned before = sc64_terrain_partial;
                game_render();
                capture(candidate_pixels);
                if (sc64_terrain_partial > before) ++partials;
                sc64_terrain_cache_valid = 0;
                sc64_hud_valid = 0;
                game_render();
                capture(reference_pixels);
                if (memcmp(candidate_pixels, reference_pixels, FB_SIZE)) {
                    for (int k = 0; k < FB_SIZE; ++k) {
                        if (candidate_pixels[k] == reference_pixels[k]) continue;
                        fprintf(stderr, "phase %d,%d side %d scenario %u: "
                                "pixel %d,%d got %u expected %u\n",
                                px, py, side, scenario, k % 240, k / 240,
                                candidate_pixels[k], reference_pixels[k]);
                        break;
                    }
                    assert(0);
                }
                ++cases;
            }
    printf("terrain scene equality: %u cases, %u partial cache reuses\n",
           cases, partials);
    assert(partials > 0);
    return 0;
}
