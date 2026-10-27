/* Host coverage for the hidden unified-profile showcase scenes.
 * Compile with SC64_SHOWCASE_ENABLED and the full P7 typed/rect profile. */
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 previous;

static void frame_input(u16 held)
{
    game_update(held, held & (u16)~previous);
    previous = held;
}

static void press(u16 key)
{
    frame_input(key);
    frame_input(0);
}

static int fail(const char *message)
{
    fprintf(stderr, "Showcase: %s\n", message);
    return 1;
}

#ifdef SC64_UNIT_FACING
static void facing_test_clip(int top, int bottom)
{
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(top, bottom);
#else
    (void)top;
    (void)bottom;
#endif
}

static int facing_is_zero(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (sc64_facing[i]) return 0;
    return 1;
}

static unsigned framebuffer_hash(void)
{
    unsigned hash = 2166136261u;
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            hash = (hash ^ gfx_test_pixel(x, y)) * 16777619u;
    return hash;
}

static int check_facing_word_bank(void)
{
#if defined(SC64_UNIT_WORD_SPRITES) && defined(SC64_UNIT_FACING_WORD_METADATA)
    static const int clip[][2] = {{0,160},{12,120},{35,42},{150,160},{0,0}};
    const int raws[] = {0, 5, 30}; /* Marine, Tank hull, static siege hull */
    unsigned direction_hash[32];
    for (unsigned ri = 0; ri < sizeof(raws)/sizeof(raws[0]); ++ri) {
        const Sc64SpriteFrame *frames = sc64_unit_facing_frames((unsigned short)raws[ri]);
        if (!frames) return fail("missing directional source bank");
        for (int direction = 0; direction < 32; ++direction) {
            const Sc64UnitFacingWordFrame *word =
                sc64_unit_facing_word_frame((unsigned short)raws[ri], (unsigned char)direction);
            if (!word || word->sprite->width != frames[direction <= 16 ? direction : 32-direction].w ||
                word->sprite->height != frames[direction <= 16 ? direction : 32-direction].h)
                return fail("missing/mismatched packed direction sidecar");
            for (unsigned ci = 0; ci < sizeof(clip)/sizeof(clip[0]); ++ci) {
                gfx_begin(0);
                facing_test_clip(clip[ci][0], clip[ci][1]);
                if (!sc64_facing_blit((unsigned)raws[ri], direction, 119, 79))
                    return fail("packed source direction did not draw");
                unsigned actual[240*160];
                for (int y = 0; y < 160; ++y)
                    for (int x = 0; x < 240; ++x)
                        actual[y*240+x] = gfx_test_pixel(x,y);

                gfx_begin(0);
                facing_test_clip(clip[ci][0], clip[ci][1]);
                const Sc64SpriteFrame *f = &frames[direction <= 16 ? direction : 32-direction];
                if (direction <= 16)
                    gfx_blit_sprite(119-f->canvas_w/2+f->offset_x,
                                    79-f->canvas_h/2+f->offset_y,f->w,f->h,f->pixels);
                else
                    gfx_blit_sprite_hflip(119+f->canvas_w/2-f->offset_x-f->w,
                                          79-f->canvas_h/2+f->offset_y,f->w,f->h,f->pixels);
                for (int y = 0; y < 160; ++y)
                    for (int x = 0; x < 240; ++x)
                        if (actual[y*240+x] != gfx_test_pixel(x,y))
                            return fail("packed direction differs from indexed/mirrored source pixels");
            }
            gfx_begin(0);
            facing_test_clip(0,160);
            (void)sc64_facing_blit((unsigned)raws[ri], direction, 119, 79);
            direction_hash[direction] = framebuffer_hash();
        }
        if (ri < 2 && (direction_hash[0] == direction_hash[8] ||
                       direction_hash[8] == direction_hash[24]))
            return fail("directional frame pixels did not change across headings");
    }
#endif
    return 0;
}

static int check_movement_facing(void)
{
    static const int headings[8] = {0,4,8,12,16,20,24,28};
    const int types[3] = {U_MARINE, U_SCOUT, U_SCV};
    for (unsigned ti = 0; ti < 3; ++ti) {
        Unit *q = &u[0];
        memset(q, 0, sizeof(*q));
        q->alive = 1; q->owner = SIDE; q->type = types[ti]; q->raw_type = -1;
        q->x = q->y = 128;
        q->order = types[ti] == U_SCV ? ORDER_MINE : ORDER_MOVE;
        q->cd = 20;
        int raw = sc64_race_raw_unit(q->type);
        if (raw < 0 || !sc64_unit_facing_frames((unsigned short)raw))
            return fail("movement-facing test unit lacks imported source frames");
        unsigned hashes[8];
        for (unsigned hi = 0; hi < 8; ++hi) {
            int dir = headings[hi];
            int dx = sc64_facing_vector[dir][0], dy = sc64_facing_vector[dir][1];
            if (sc64_race_unit_flying(q->type)) {
                q->tx = q->x + dx; q->ty = q->y + dy;
            } else {
                q->path_x = q->x + dx; q->path_y = q->y + dy;
            }
            sc64_facing_track(q);
            if (sc64_facing[0] != dir)
                return fail("moving unit did not track one of eight headings during cooldown");
            gfx_begin(0);
            if (!sc64_facing_draw(q, raw, 120, 80))
                return fail("moving unit did not render its selected heading");
            hashes[hi] = framebuffer_hash();
        }
        for (unsigned hi = 1; hi < 8; ++hi)
            if (hashes[hi] == hashes[hi-1])
                return fail("movement heading did not alter the rendered source frame");
        q->order = ORDER_ATTACK_TARGET;
        sc64_facing[0] = 3;
        sc64_facing_track(q);
        if (sc64_facing[0] != 3)
            return fail("stationary firing cooldown lost its aimed direction");
    }
    return 0;
}

#endif

static int unit_count(int type)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == type) ++count;
    return count;
}

static int owner_count(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++count;
    return count;
}

static int building_count(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && (owner < 0 || b[i].owner == owner)) ++count;
    return count;
}

static int showcase_has_building(int owner, int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return 1;
    return 0;
}

static int valid_world(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].hp <= 0 || u[i].hp > u[i].maxhp ||
            u[i].x < 0 || u[i].x >= WORLD_W ||
            u[i].y < 0 || u[i].y >= WORLD_H ||
            u[i].owner < SIDE || u[i].owner > 4 ||
            u[i].type < 0 || u[i].type >= SC_RU_COUNT) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].hp <= 0 || b[i].hp > b[i].maxhp ||
            b[i].x < 0 || b[i].x >= WORLD_W ||
            b[i].y < 0 || b[i].y >= WORLD_H ||
            b[i].owner < SIDE || b[i].owner > 4) return 0;
    }
    return 1;
}

static int check_scene_contents(int scene)
{
    switch (scene) {
    case 0:
        return unit_count(U_P7_CARRIER) == 3 &&
               unit_count(U_HYDRALISK) >= 12 &&
               unit_count(U_ZP_ULTRALISK) >= 1;
    case 1:
        return unit_count(U_SIEGE_TANK) == 6 &&
               unit_count(U_ZERGLING) == 32;
    case 2: /* Terran last stand */
        return showcase_has_building(SIDE, B_COMMAND) && unit_count(U_SCV) == 4 &&
               owner_count(ENEMY) == 5 && unit_count(U_MARINE) == 1 &&
               unit_count(U_ZERGLING) == 4;
    case 3: /* Zerg last stand */
        return showcase_has_building(SIDE, B_HATCHERY) && unit_count(U_DRONE) == 4 &&
               owner_count(ENEMY) == 5 && unit_count(U_MARINE) == 1 &&
               unit_count(U_ZERGLING) == 4;
    case 4: /* Protoss last stand */
        return showcase_has_building(SIDE, B_NEXUS) && unit_count(U_PROBE) == 4 &&
               owner_count(ENEMY) == 5 && unit_count(U_MARINE) == 1 &&
               unit_count(U_ZERGLING) == 4;
    case 5: /* Protoss mixed force */
        return unit_count(U_ZEALOT) > 0 && unit_count(U_DRAGOON) > 0;
    case 6: /* Three-way battle */
        return owner_count(SIDE) > 0 && owner_count(ENEMY) > 0 &&
               owner_count(RESCUABLE) > 0;
    case 7: /* Base siege */
        return unit_count(U_SIEGE_TANK) > 0 && building_count(-1) > 0;
    default:
        return 0;
    }
}

static int enter_showcase_menu(void)
{
    game_init();
    previous = 0;
    frame_input(KEY_L | KEY_R);
    frame_input(KEY_L | KEY_R | KEY_SELECT);
    frame_input(0);
    return screen == SCREEN_TITLE && race_front.page == SC_FRONT_SHOWCASE;
}

#ifdef SC64_FIELD_CONTROLS
/* Default-profile controls ported to the race engine (field_controls.h),
 * exercised on the live Carrier Approach scene. */
static int check_field_controls(void)
{
    if (!enter_showcase_menu()) return fail("controls: could not open showcase");
    press(KEY_A); press(KEY_A);
    if (screen != SCREEN_PLAY || sc64_showcase_scene != 0) return fail("controls: scene 0 did not start");

    clear_selection();
    press(KEY_R);
    if (selected_count() < 2) return fail("controls: R tap did not select the army");

    int carrier = -1;
    for (int i = 0; i < MAX_UNITS && carrier < 0; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_P7_CARRIER) carrier = i;
    if (carrier < 0) return fail("controls: no Carrier in scene 0");
    clear_selection();
    u[carrier].selected = 1;
    frame_input(KEY_R);
    for (int i = 0; i < 6; ++i) frame_input(KEY_R | KEY_RIGHT);
    frame_input(0);
    if (selected_count() != 1 || !u[carrier].selected)
        return fail("controls: holding R to steer replaced the selection");

    int nexus = -1;
    for (int i = 0; i < MAX_BUILDINGS && nexus < 0; ++i)
        if (b[i].alive && b[i].owner == SIDE && !b[i].under_construction) nexus = i;
    if (nexus < 0) return fail("controls: no friendly structure in scene 0");
    clear_selection();
    selected_building = nexus;
    int queued = b[nexus].queue_count;
    cx = b[nexus].x + 40; cy = b[nexus].y + 40;
    if (selection_target() >= 0) return fail("controls: deselect fixture is not empty ground");
    press(KEY_A);
    if (selected_building >= 0 || b[nexus].queue_count != queued)
        return fail("controls: A on empty ground trained instead of deselecting");

    clear_selection();
    press(KEY_R);
    press(KEY_B);
    if (!menu_open || menu_cursor != MENU_MOVE) return fail("controls: order list did not open on Move");
    press(KEY_DOWN);
    press(KEY_A);
    field_ui_refresh();
    if (mode != MODE_ATTACK || field_controls_title_color() != C_RED ||
        !field_controls_hint() || strcmp(field_controls_hint(), "A ATTACK HERE  B BACK"))
        return fail("controls: armed Attack Move is not shown in the HUD");
    press(KEY_A);
    if (mode != MODE_NORMAL || !ping_life || ping_color != C_RED)
        return fail("controls: Attack Move left no red ground marker");
    press(KEY_B);
    if (!menu_open || menu_cursor != MENU_ATTACK) return fail("controls: B did not reopen on the last command");
    press(KEY_B);

    int hydra = -1;
    for (int i = 0; i < MAX_UNITS && hydra < 0; ++i)
        if (u[i].alive && u[i].owner == ENEMY && currently_visible(u[i].x, u[i].y)) hydra = i;
    if (hydra < 0) return fail("controls: no visible enemy in scene 0");
    clear_selection();
    press(KEY_R);
    cx = u[hydra].x; cy = u[hydra].y;
    frame_input(0); /* the HUD resolves once per update, as the cursor moves */
    ping_life = 0;
    field_ui_refresh();
    if (ui_action != UI_A_ATTACK) return fail("controls: hint does not offer an attack on the enemy");
    press(KEY_A);
    int attacking = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && u[i].order == ORDER_ATTACK_MOVE) attacking = 1;
    if (!attacking || !ping_life || ping_color != C_RED)
        return fail("controls: A on a visible enemy did not attack");
    return 0;
}
#endif

int main(void)
{
    gba_init();
#ifdef SC64_UNIT_FACING
    game_init();
    memset(sc64_facing, 0x17, sizeof(sc64_facing));
    game_init();
    if (!facing_is_zero()) return fail("game_init retained stale facing bytes");
    if (check_facing_word_bank() || check_movement_facing()) return 1;
#endif
    if (!enter_showcase_menu()) return fail("HOME shoulder chord did not open showcase");
    press(KEY_A);
    if (screen != SCREEN_PAUSE || sc64_showcase_scene != 0)
        return fail("launch did not start scene 0 in paused state");

    /* Seed an actual ordinary race-skirmish save, then prove showcase mode
     * cannot replace it. The menu's default row is not the skirmish row. */
    game_init();
    previous = 0;
    race_mode = 1;
    race_human = R_TER;
    race_bots = 1;
    race_skirmish_map = 0;
    init_state();
#ifdef SC64_UNIT_FACING
    int reused = spawn_unit(SIDE, U_MARINE, 210, 210);
    if (reused < 0) return fail("could not allocate slot-reuse fixture");
    u[reused].raw_type = -1;
    sc64_facing[reused] = 29;
    u[reused].alive = 0;
    int reclaimed = spawn_unit(SIDE, U_MARINE, 210, 210);
    if (reclaimed != reused || sc64_facing[reclaimed] != 0)
        return fail("spawn reused stale per-slot facing");
    unsigned marine_raw = (unsigned)sc64_race_raw_unit(U_MARINE);
    sc64_facing[reclaimed] = 8;
    gfx_begin(0);
    if (!sc64_facing_draw(&u[reclaimed], (int)marine_raw, 120, 80) ||
        framebuffer_hash() == 2166136261u)
        return fail("raw_type=-1 generic unit did not resolve its directional sprite");
#endif
    if (!race_mode || !save_game()) return fail("could not seed ordinary skirmish save");
#ifdef SC64_UNIT_FACING
    memset(sc64_facing, 0x1b, sizeof(sc64_facing));
    static Unit rejected_save_world[MAX_UNITS];
    memcpy(rejected_save_world, u, sizeof(u));
    save_test_corrupt_latest(sizeof(race_data), SAVE_VERSION);
    if (load_game() || sc64_facing[0] != 0x1b ||
        memcmp(rejected_save_world, u, sizeof(u)))
        return fail("rejected save changed presentation-facing state");
    if (!save_game()) return fail("could not reseed save after rejection fixture");
#endif
    int saved_minerals = minerals;
    int saved_side_units = owner_count(SIDE);

    if (!enter_showcase_menu()) return fail("could not reopen showcase menu");
    press(KEY_A);
    if (screen != SCREEN_PAUSE || sc64_showcase_scene != 0 || save_game())
        return fail("showcase save guard failed");

    for (int scene = 0; scene < 8; ++scene) {
        if (sc64_showcase_scene != scene || screen != SCREEN_PAUSE ||
            !check_scene_contents(scene) || !valid_world()) {
            fprintf(stderr, "Showcase: bad initial scene %d (scene=%d, screen=%d, units=%d)\n",
                    scene, sc64_showcase_scene, screen, owner_count(SIDE) + owner_count(ENEMY));
            return 1;
        }
        static Unit replay_units[MAX_UNITS];
        static Building replay_buildings[MAX_BUILDINGS];
        memcpy(replay_units,u,sizeof(u));
        memcpy(replay_buildings,b,sizeof(b));
        unsigned frozen_ticks = sc64_showcase_ticks;
        for (int i = 0; i < 30; ++i) frame_input(0);
        if (sc64_showcase_ticks != frozen_ticks)
            return fail("scene advanced while paused");
        game_render();
#ifdef SC64_UNIT_FACING
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive) { sc64_facing[i] = 31; break; }
#endif
        press(KEY_R); /* replay current scene */
        if (screen != SCREEN_PAUSE || sc64_showcase_scene != scene ||
            !check_scene_contents(scene) || !valid_world() ||
            memcmp(replay_units,u,sizeof(u)) ||
            memcmp(replay_buildings,b,sizeof(b)))
            return fail("pause replay did not deterministically reseed scene");
#ifdef SC64_UNIT_FACING
        if (!facing_is_zero()) return fail("scene replay retained old facing state");
#endif
        press(KEY_A); /* resume; release update is simulation tick 1 */
        if (screen != SCREEN_PLAY) return fail("A did not resume the scene");
        for (int tick = 0; tick < 300 && screen == SCREEN_PLAY; ++tick) {
            frame_input(0);
            if (!valid_world()) return fail("invalid actor state during scene simulation");
        }
        game_render();

        if (scene >= 2 && scene <= 4) {
            if (screen != SCREEN_RESULT || !lost || sc64_showcase_ticks != 150) {
                fprintf(stderr, "Showcase: last-stand scene %d result=%d won=%d lost=%d tick=%u\n",
                        scene, screen, won, lost, sc64_showcase_ticks);
                return 1;
            }
        }
        if (scene < 7) {
            if (screen == SCREEN_RESULT) press(KEY_B);
            else if (screen == SCREEN_PAUSE) press(KEY_B);
            else { press(KEY_START); press(KEY_B); }
            if (screen != SCREEN_TITLE || race_front.page != SC_FRONT_SHOWCASE ||
                race_front.row != scene)
                return fail("B did not return to the current showcase menu row");
            press(KEY_DOWN);
            press(KEY_A);
            if (screen != SCREEN_PAUSE || sc64_showcase_scene != scene + 1)
                return fail("DOWN/A did not select the next paused scene");
        }
    }

    /* SELECT exits directly to the showcase list and preserves its row. */
    press(KEY_SELECT);
    if (screen != SCREEN_TITLE || race_front.page != SC_FRONT_SHOWCASE ||
        race_front.row != 7) return fail("SELECT did not preserve the current showcase row");

    game_init();
    previous = 0;
#ifdef SC64_UNIT_FACING
    memset(sc64_facing, 0x1d, sizeof(sc64_facing));
#endif
    press(KEY_B); /* HOME load: restore the actual ordinary skirmish save */
    if (!race_mode || minerals != saved_minerals || owner_count(SIDE) != saved_side_units)
        return fail("showcase changed or invalidated the ordinary skirmish save");
#ifdef SC64_UNIT_FACING
    if (!facing_is_zero()) return fail("successful load restored/retained stale facing bytes");
#endif
#ifdef SC64_FIELD_CONTROLS
    if (check_field_controls()) return 1;
#endif
    puts("Showcase menu, scene runtime, pause controls, and save isolation: ok");
    return 0;
}
