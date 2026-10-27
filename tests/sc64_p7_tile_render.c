/* P7 displayed-frame differential for optional rendering profiles.
 *
 * The reference uses mode 4. A candidate either uses the canonical mode-4
 * draw flags or defines SC64_TILE_RENDERER to compose emulated mode-0 layers.
 * Both run the same controller-only
 * session (menu launch, neutral play, four camera pans, pause, a controller
 * attack-move and combat) with one game_render() per update, and print the
 * RGB555 hash of every displayed frame plus a simulation-state hash. The two
 * outputs must be byte-identical. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u16 held_previous;
static u16 frame_rgb[38400];
static unsigned frames_rendered;

static unsigned fnv(unsigned h, const void *data, size_t length)
{
    const unsigned char *p = data;
    for (size_t i = 0; i < length; ++i) { h ^= p[i]; h *= 16777619u; }
    return h;
}

static unsigned state_hash(void)
{
    unsigned h = 2166136261u;
    h = fnv(h, &race_p7, sizeof(race_p7));
    h = fnv(h, u, sizeof(u));
    h = fnv(h, b, sizeof(b));
    h = fnv(h, &frame, sizeof(frame));
    return h;
}

/* One main-loop iteration: update with edge-detected presses, then render. */
static void step(u16 held, const char *phase)
{
    game_update(held, held & (u16)~held_previous);
    held_previous = held;
    game_render();
    gfx_host_frame_rgb(frame_rgb);
    {
        /* SC64_DUMP_FRAME=n writes that frame's raw RGB555 for diagnosis. */
        const char *dump = getenv("SC64_DUMP_FRAME");
        if (dump && (unsigned)atoi(dump) == frames_rendered) {
            FILE *f = fopen(getenv("SC64_DUMP_PATH") ? getenv("SC64_DUMP_PATH") : "frame.rgb", "wb");
            if (f) { fwrite(frame_rgb, sizeof(frame_rgb), 1, f); fclose(f); }
        }
    }
    printf("%s %u %08x cam %d,%d\n", phase, frames_rendered++,
           fnv(2166136261u, frame_rgb, sizeof(frame_rgb)), camx, camy);
#ifdef SC64_TILE_RENDERER
    if (getenv("SC64_TILE_TRACE"))
        fprintf(stderr, "frame %u lifts %u overflow %d fogkey %08x\n", frames_rendered - 1,
                (unsigned)sc64_tile_art_lifts, sc64_tile_art_overflow, (unsigned)sc64_tile_fog_key);
#endif
}

static void tap(u16 key, const char *phase)
{
    step(key, phase);
    step(0, phase);
}

int main(void)
{
    /* Include the normal low-index HUD palette in the displayed RGB oracle. */
    gba_init();
    game_init();
    game_render();
    tap(KEY_A, "menu");
    tap(KEY_RIGHT, "menu"); tap(KEY_RIGHT, "menu"); tap(KEY_RIGHT, "menu");
    tap(KEY_A, "menu");
    step(KEY_A, "launch");
    step(0, "launch");
    assert(screen == SCREEN_PLAY && race_selected_mission == SC64_P7_GLOBAL_ID);
    for (int t = 0; t < 300; ++t) step(0, "neutral");
    static const u16 pans[4] = { KEY_RIGHT, KEY_DOWN, KEY_LEFT, KEY_UP };
    for (int p = 0; p < 4; ++p)
        for (int t = 0; t < 160; ++t) step(pans[p], "pan");
    for (int t = 0; t < 20; ++t) step(0, "settle");
    tap(KEY_START, "pause");
    for (int t = 0; t < 10; ++t) step(0, "pause");
    tap(KEY_START, "resume");
    for (int t = 0; t < 10; ++t) step(0, "resume");
    tap(KEY_R, "command");
    tap(KEY_B, "command");
    tap(KEY_DOWN, "command");
    tap(KEY_A, "command");
    for (int guard = 0; guard < 2000 && screen == SCREEN_PLAY; ++guard) {
        int target = -1, best = 0x7fffffff;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == 1 && dist2(cx, cy, b[i].x, b[i].y) < best) {
                best = dist2(cx, cy, b[i].x, b[i].y);
                target = i;
            }
        u16 keys = 0;
        if (target < 0) break;
        if (cx < b[target].x - 2) keys |= KEY_RIGHT;
        else if (cx > b[target].x + 2) keys |= KEY_LEFT;
        if (cy < b[target].y - 2) keys |= KEY_DOWN;
        else if (cy > b[target].y + 2) keys |= KEY_UP;
        if (!keys) break;
        step(keys, "aim");
    }
    tap(KEY_A, "attack");
    for (int t = 0; t < 600 && screen == SCREEN_PLAY; ++t) step(0, "combat");
    printf("frames %u screen %d won %d lost %d units %d state %08x\n",
           frames_rendered, screen, won, lost, (int)(sizeof(u) / sizeof(u[0])), state_hash());
#ifdef SC64_TILE_RENDERER
    fprintf(stderr, "tile renderer: uploads %u, art lifts %u, art overflows %u, jump steals %u\n",
            (unsigned)sc64_tile_uploads, (unsigned)sc64_tile_art_lifts,
            (unsigned)sc64_tile_art_overflows, (unsigned)sc64_tile_jump_steals);
#endif
    return 0;
}
