/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Large unit table (SC64_ZP_UNIT_SLOTS > 254) on the Zerg release engine.
 * 400 Zerglings for one player fill the table past every 8-bit and 9-bit
 * boundary. The ordinary simulation runs them into a fight; the collision
 * buckets must index every collidable unit exactly once; an ordinary
 * pause-menu save must fit the record and reload into a fresh game that then
 * continues identically to the uninterrupted one. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !SC64_UNIT_IX_WIDE || !defined(SC64_ZP_UNIT_STREAM) || !defined(SC64_CROWD_PROBE_MANUAL)
#error "build with -DSC64_ZP_UNIT_SLOTS=408 -DSC64_CROWD_PROBE=400 -DSC64_CROWD_PROBE_MANUAL"
#endif

static unsigned held_keys;
static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}
static void press(unsigned key) { input(key); input(0); }

static u32 world_digest(void)
{
    u32 h = 2166136261u;
#define MIX(v) do { u32 x_ = (u32)(v); for (int k_ = 0; k_ < 4; ++k_) { h ^= (x_ >> (8 * k_)) & 255u; h *= 16777619u; } } while (0)
    for (int i = 0; i < MAX_UNITS; ++i) {
        const u8 *p = (const u8 *)&u[i];
        for (unsigned k = 0; k < sizeof(Unit); ++k) MIX(p[k]);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        MIX(b[i].alive);
        if (!b[i].alive) continue;
        MIX(b[i].hp); MIX(b[i].type); MIX(b[i].owner); MIX(b[i].queue_count);
    }
    MIX(frame); MIX(minerals); MIX(gas); MIX(seed);
#undef MIX
    return h;
}

static int count_alive(int owner, int type)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        n += u[i].alive && u[i].owner == owner && u[i].type == type;
    return n;
}

/* Rebuild the per-frame index and require every collidable live unit to be
 * linked in exactly its own cell, through the nine-bit head plane. */
static void check_buckets(void)
{
    static u8 seen[MAX_UNITS];
    int linked = 0, expected = 0;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    memset(seen, 0, sizeof(seen));
    for (int cell = 0; cell < MAP_COLS * MAP_ROWS; ++cell)
        for (int i = SC64_UB_HEAD(cell); i != SC64_UNIT_IX_NONE; i = sc64_unit_bucket_next[i]) {
            assert(i >= 0 && i < MAX_UNITS && !seen[i]);
            assert(sc64_unit_bucket_cell_at(u[i].x, u[i].y) == cell);
            seen[i] = 1; ++linked;
        }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && !(SC64_TRANSPORT_LOADED(&u[i]) || SC64_SCARAB_NONCOLLIDING(&u[i]))) {
            assert(seen[i]); ++expected;
        }
    assert(linked == expected);
}

int main(void)
{
    static u32 reference[4];
    unsigned saved_frame;
    game_init();
    held_keys = 0;
    sc64_crowd_probe_start();
    assert(screen == SCREEN_PLAY && race_mode);
    assert(sc64_crowd_probe_spawned == 400 && count_alive(SIDE, U_ZERGLING) == 400);
    assert(u[MAX_UNITS - 1].alive && MAX_UNITS == 408);
    check_buckets();
    for (int i = 0; i < 350; ++i) input(0);
    check_buckets();
    int lings = count_alive(SIDE, U_ZERGLING);
    printf("swarm: %d Zerglings after 350 updates, %d Marines\n", lings, count_alive(ENEMY, U_MARINE));
    assert(lings > 300);

    /* Ordinary pause-menu save of the whole table. */
    press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    press(KEY_B);
    saved_frame = (unsigned)frame;
    assert(sc64_zp_stream_canonical(&race_data.units));
    printf("swarm save: %u unit-stream bytes of %u, %u templates\n",
           race_data.units.used, (unsigned)SC64_ZPS_DATA_BYTES, race_data.units.template_count);
    press(KEY_START);
    assert(screen == SCREEN_PLAY);
    for (int k = 0; k < 4; ++k) {
        for (int i = 0; i < 150; ++i) input(0);
        reference[k] = world_digest();
        assert(!won && !lost);   /* a result screen writes its own record */
    }
    game_init();
    held_keys = 0;
    press(KEY_L);
    if (!(screen == SCREEN_PLAY && race_mode && (unsigned)frame == saved_frame + 1u))
        printf("reload: screen %d race_mode %d frame %d saved %u toast %d\n", screen, race_mode, frame, saved_frame, toast_id);
    assert(screen == SCREEN_PLAY && race_mode && (unsigned)frame == saved_frame + 1u);
    assert(count_alive(SIDE, U_ZERGLING) == lings);
    check_buckets();
    for (int k = 0; k < 4; ++k) {
        for (int i = 0; i < 150; ++i) input(0);
        assert(world_digest() == reference[k]);
    }
    check_buckets();
    printf("swarm save continuation: frame %u, 600 updates identical, %d Zerglings left\n",
           saved_frame, count_alive(SIDE, U_ZERGLING));
    return 0;
}
