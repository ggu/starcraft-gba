/* Moving imported units turn toward their destination and cycle their
 * source walk poses; standing ground units keep their facing frame, while
 * idle flyers keep beating their wings.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#if !defined(SC64_RACES) || !defined(SC64_UNIT_FACING) || !defined(SC64_UNIT_WALK_FRAMES)
#error "Use a race-engine production vector with the facing and walk banks"
#endif

static unsigned framebuffer_hash(void)
{
    unsigned h = 2166136261u;
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            h = (h ^ gfx_test_pixel(x, y)) * 16777619u;
    return h;
}

static void setup(void)
{
    gba_init(); game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
#endif
    race_player[SIDE] = R_TER; race_player[ENEMY] = R_ZERG;
    selected_building = -1; menu_open = 0; mode = MODE_NORMAL;
    ping_life = 0; camx = camy = 0;
    sc64_walk_reset_all();
    sc64_facing_reset_all();
}

/* Draw one unit with the camera centred on it, so only its art changes. */
static unsigned draw_centred(int slot)
{
    camx = u[slot].x - 120; camy = u[slot].y - 68;
    gfx_begin(0);
    draw_unit(&u[slot]);
    return framebuffer_hash();
}

/* The same unit drawn from its expected frame alone. */
static unsigned expected_centred(const Sc64SpriteFrame *frames, int dir)
{
    gfx_begin(0);
    sc64_facing_blit_frame(&frames[dir <= 16 ? dir : 32 - dir], dir, 120, 80);
    return framebuffer_hash();
}

/* A walk pose drawn alone; its 32 headings need no mirroring. */
static unsigned expected_pose(const Sc64SpriteFrame *pose, int dir)
{
    const Sc64SpriteFrame *s = &pose[dir];
    gfx_begin(0);
    gfx_blit_sprite(120 - s->canvas_w / 2 + s->offset_x, 80 - s->canvas_h / 2 + s->offset_y,
                    s->w, s->h, s->pixels);
    return framebuffer_hash();
}

/* Each stored western heading is its eastern twin, mirrored on the canvas. */
static void check_mirrored_headings(const Sc64UnitWalkCycle *cycle)
{
    for (unsigned k = 0; k < cycle->count; ++k)
        for (int dir = 17; dir < 32; ++dir) {
            const Sc64SpriteFrame *west = &cycle->steps[k].frames[dir];
            const Sc64SpriteFrame *east = &cycle->steps[k].frames[32 - dir];
            assert(west->w == east->w && west->h == east->h);
            assert(west->canvas_w == east->canvas_w && west->canvas_h == east->canvas_h);
            assert(west->offset_x == east->canvas_w - east->offset_x - east->w);
            unsigned stored = expected_pose(cycle->steps[k].frames, dir);
            gfx_begin(0);
            gfx_blit_sprite_hflip(120 + east->canvas_w / 2 - east->offset_x - east->w,
                                  80 - east->canvas_h / 2 + east->offset_y,
                                  east->w, east->h, east->pixels);
            assert(stored == framebuffer_hash());
        }
}

static void order_move(int slot, int x, int y)
{
    u[slot].selected = 1;
    assert(issue_order(ORDER_MOVE, x, y) == 1);
    u[slot].selected = 0;
}

static int walk_leg(int slot, int tx, int ty, int want_dir, const Sc64UnitWalkCycle *cycle)
{
    const Sc64SpriteFrame *seen[SC64_UNIT_WALK_MAX_STEPS] = {0};
    int poses = 0;
    order_move(slot, tx, ty);
    /* Stop short of the destination, where arrival steering takes over. */
    for (int tick = 0; tick < 48 && FAST_ABS(u[slot].x - tx) + FAST_ABS(u[slot].y - ty) > 8; ++tick) {
        int x = u[slot].x, y = u[slot].y;
        screen = SCREEN_PLAY;
        game_update(0, 0);
        if (u[slot].x == x && u[slot].y == y) continue;
        unsigned drawn = draw_centred(slot);
        int dir = sc64_facing[slot] & SC64_FACING_MASK;
        assert(dir == want_dir);
        const Sc64SpriteFrame *pose = sc64_walk_pose((unsigned)sc64_race_raw_unit(u[slot].type), slot);
        assert(pose);
        /* The walk pose replaces the one-pixel stride lift. */
        assert(drawn == expected_pose(pose, dir));
        int known = 0;
        for (int i = 0; i < poses; ++i) known |= seen[i] == pose;
        if (!known && poses < (int)SC64_UNIT_WALK_MAX_STEPS) seen[poses++] = pose;
    }
    assert(poses == (int)cycle->count);
    return poses;
}

static void test_walk(int type, const char *name)
{
    int raw = sc64_race_raw_unit(type);
    const Sc64UnitWalkCycle *cycle = raw >= 0 ? sc64_unit_walk_cycle((unsigned short)raw) : 0;
    if (!cycle) {
        printf("SKIP %s: no source walk cycle in this bank\n", name);
        return;
    }
    check_mirrored_headings(cycle);
    setup();
    int slot = spawn_unit(SIDE, type, 200, 200);
    assert(slot >= 0);
    const Sc64SpriteFrame *idle = sc64_unit_facing_frames((unsigned short)raw);
    assert(idle);
    unsigned standing = draw_centred(slot);
    assert(standing == draw_centred(slot));
    assert(standing == expected_centred(idle, 0));

    int east = walk_leg(slot, 300, 200, 8, cycle);
    int north_west = walk_leg(slot, u[slot].x - 60, u[slot].y - 60, 28, cycle);

    /* Arrived and standing: the facing frame of the last heading, no pose. */
    for (int tick = 0; tick < 64 && u[slot].order != ORDER_IDLE; ++tick) {
        screen = SCREEN_PLAY;
        game_update(0, 0);
    }
    assert(u[slot].order == ORDER_IDLE);
    /* Undrawn arrival ticks count as a step, then the tracker holds one draw. */
    draw_centred(slot);
    draw_centred(slot);
    unsigned still = draw_centred(slot);
    assert(still == expected_centred(idle, sc64_facing[slot] & SC64_FACING_MASK));
    printf("PASS %s: %d poses east, %d north-west, standing frame restored\n",
           name, east, north_west);
}

/* An idle flyer with a source cycle keeps playing it in place: every pose
 * shows across one cycle, drawn at its facing, with no stride lift. A flyer
 * without one holds its facing frame. Nothing it draws moves or orders it. */
static void test_hover(int type, const char *name)
{
    int raw = sc64_race_raw_unit(type);
    if (raw < 0) {
        printf("SKIP %s hover: not in this race bank\n", name);
        return;
    }
    assert(sc64_race_unit_flying(type));
    const Sc64UnitWalkCycle *cycle = sc64_unit_walk_cycle((unsigned short)raw);
    setup();
    int slot = spawn_unit(SIDE, type, 200, 200);
    assert(slot >= 0 && u[slot].order == ORDER_IDLE);
    int x = u[slot].x, y = u[slot].y;
    if (!cycle) {
        const Sc64SpriteFrame *idle = sc64_unit_facing_frames((unsigned short)raw);
        assert(idle);
        frame = 0;
        unsigned first = draw_centred(slot);
        for (frame = 1; frame < 40; ++frame) assert(draw_centred(slot) == first);
        assert(first == expected_centred(idle, 0));
        printf("PASS %s hover: no source cycle, facing frame held\n", name);
        return;
    }
    const Sc64SpriteFrame *seen[SC64_UNIT_WALK_MAX_STEPS] = {0};
    int poses = 0, changes = 0;
    unsigned last = 0;
    /* Five game frames make four source ticks. */
    for (frame = 0; frame < (int)cycle->cycle_ticks * 5 / 4 + 5; ++frame) {
        unsigned drawn = draw_centred(slot);
        const Sc64SpriteFrame *pose = sc64_walk_pose((unsigned)raw, slot);
        assert(pose && drawn == expected_pose(pose, sc64_facing[slot] & SC64_FACING_MASK));
        changes += frame && drawn != last;
        last = drawn;
        int known = 0;
        for (int i = 0; i < poses; ++i) known |= seen[i] == pose;
        if (!known && poses < (int)SC64_UNIT_WALK_MAX_STEPS) seen[poses++] = pose;
    }
    assert(poses == (int)cycle->count);
    assert(cycle->count < 2 || changes > 0);
    assert(u[slot].order == ORDER_IDLE && u[slot].x == x && u[slot].y == y);
    printf("PASS %s hover: %d poses while idle, %d changes\n", name, poses, changes);
}

/* A moving Tank rolls its tread poses; the turret stays level with them. */
static void test_tank(void)
{
    const Sc64UnitWalkCycle *treads = sc64_unit_walk_cycle(5u);
    if (!treads) {
        printf("SKIP tank: no source tread cycle in this bank\n");
        return;
    }
    setup();
    int slot = spawn_unit(SIDE, U_TANK, 200, 200);
    assert(slot >= 0);
    order_move(slot, 300, 200);
    int checked = 0;
    for (int tick = 0; tick < 24; ++tick) {
        int x = u[slot].x;
        screen = SCREEN_PLAY;
        game_update(0, 0);
        if (u[slot].x == x) continue;
        unsigned drawn = draw_centred(slot);
        int dir = sc64_facing[slot] & SC64_FACING_MASK;
        const Sc64SpriteFrame *pose = sc64_walk_pose(5u, slot);
        assert(dir == 8 && pose);
        const Sc64SpriteFrame *hull = &pose[dir];
        const Sc64SpriteFrame *turret = &sc64_unit_facing_frames(6u)[dir];
        gfx_begin(0);
        gfx_blit_sprite(120 - hull->canvas_w / 2 + hull->offset_x,
                        80 - hull->canvas_h / 2 + hull->offset_y, hull->w, hull->h, hull->pixels);
        gfx_blit_sprite(120 - turret->canvas_w / 2 + turret->offset_x,
                        80 - turret->canvas_h / 2 + turret->offset_y,
                        turret->w, turret->h, turret->pixels);
        assert(drawn == framebuffer_hash());
        ++checked;
    }
    assert(checked > 0);
    printf("PASS tank: %d frames of rolling treads under a level turret\n", checked);
}

int main(void)
{
    test_walk(U_ZERGLING, "zergling");
    test_walk(U_MARINE, "marine");
    test_walk(U_ZEALOT, "zealot");
    test_walk(U_HYDRALISK, "hydralisk");
    test_walk(U_DRONE, "drone");
    test_hover(U_MUTALISK, "mutalisk");
    test_hover(U_OVERLORD, "overlord");
    test_hover(U_WRAITH, "wraith");
    test_tank();
    return 0;
}
