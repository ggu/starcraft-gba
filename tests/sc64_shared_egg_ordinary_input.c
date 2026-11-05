/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST player controller: game state changes only through game_update(). */
#define main sc64_skirmish_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <string.h>
extern void guardian_test_reset(void);
static int observe_egg(void)
{
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==SIDE &&
        u[i].type==SC64_EGG_ACTOR && u[i].target==U_DRONE) return i;
    return -1;
}
/* Labeled HOST render fixture: compare the game's draw dispatch to the
 * prepared raw-36 source pixels, without storing private art in the test. */
static void verify_egg_render(int slot)
{
    Unit saved = u[slot];
    int saved_camx = camx, saved_camy = camy, matched = 0, opaque = 0;
    const Sc64SpriteFrame *sprite = sc64_p8_unit_sprite_for_raw(36);
    assert(sprite && sprite->pixels && sprite->w && sprite->h);
    assert(card_unit_icon(&u[slot]) == card_portrait(36));
    assert(card_portrait(36) >= 0);
    assert(!strcmp(unit_name(u[slot].type), "EGG"));
    assert(hud_egg_product(&u[slot]) == U_DRONE);
    assert(hud_egg_level(&u[slot]) == hud_level(
        sc64_z7_egg_duration(U_DRONE) - u[slot].work_timer,
        sc64_z7_egg_duration(U_DRONE), CARD_CELL));
    Unit legacy = saved; legacy.type = U_OVERLORD; legacy.raw_type = 36;
    assert(hud_egg_product(&legacy) == -1);
    assert(strcmp(unit_name(legacy.type), "EGG"));
    assert(sc64_unit_art_raw(&legacy) == 42 && card_unit_icon(&legacy) == card_portrait(42));
    u[slot].x = 100; u[slot].y = 48; u[slot].selected = 0;
    camx = camy = 0;
    gfx_begin(0); gfx_set_clip_y(12, 120);
    draw_unit_pass(&u[slot], 0);
    int x = 100 - sprite->canvas_w / 2 + sprite->offset_x;
    int y = 60 - sprite->canvas_h / 2 + sprite->offset_y;
    for (int row = 0; row < sprite->h; ++row)
        for (int col = 0; col < sprite->w; ++col) {
            int px = x + col, py = y + row;
            unsigned char source = sprite->pixels[row * sprite->w + col];
            if (source && px >= 0 && px < 240 && py >= 12 && py < 120) {
                ++opaque; matched += gfx_test_pixel(px, py) == source;
            }
        }
    assert(opaque > 10 && matched == opaque);
    /* Compare the actual HUD draw against the same decoded portrait and
     * product icon, including the progress bar at a known half duration. */
    unsigned char portrait[CARD_CELL * CARD_CELL], product[CARD_CELL * CARD_CELL];
    gfx_begin(0); gfx_set_clip_y(0, 160);
    card_icon_draw(36, HUD_PORTRAIT_X + 1, HUD_PORTRAIT_Y + 1, card_green);
    card_icon_draw(sc64_race_raw_unit(U_DRONE), HUD_COLUMN, HUD_SLOT_Y, card_gold);
    for (int row = 0; row < CARD_CELL; ++row) for (int col = 0; col < CARD_CELL; ++col) {
        portrait[row * CARD_CELL + col] = gfx_test_pixel(HUD_PORTRAIT_X + 1 + col, HUD_PORTRAIT_Y + 1 + row);
        product[row * CARD_CELL + col] = gfx_test_pixel(HUD_COLUMN + col, HUD_SLOT_Y + row);
    }
    u[slot].work_timer = sc64_z7_egg_duration(U_DRONE) / 2;
    gfx_begin(0); gfx_set_clip_y(0, 160); hud_draw_unit(slot);
    int pm = 0, qm = 0;
    for (int row = 1; row < CARD_CELL - 1; ++row) for (int col = 1; col < CARD_CELL - 1; ++col) {
        int i = row * CARD_CELL + col;
        if (portrait[i]) { assert(gfx_test_pixel(HUD_PORTRAIT_X + 1 + col, HUD_PORTRAIT_Y + 1 + row) == portrait[i]); ++pm; }
        if (product[i]) { assert(gfx_test_pixel(HUD_COLUMN + col, HUD_SLOT_Y + row) == product[i]); ++qm; }
    }
    assert(pm > 10 && qm > 10 && hud_egg_level(&u[slot]) > 0);
    assert(gfx_test_pixel(HUD_COLUMN, HUD_BAR_Y) == C_GOLD);
    assert(gfx_test_pixel(HUD_COLUMN + CARD_CELL - 1, HUD_BAR_Y) == C_SHADOW);
    printf("HOST constructed render: raw 36 source pixels %d/%d, HUD portrait %d and product %d pixels PASS\n", matched, opaque, pm, qm);
    u[slot] = saved; camx = saved_camx; camy = saved_camy;
}
static int menu_to(int command)
{
    if(menu_open)tap(KEY_B);
    tap(KEY_B);
    for(int n=0;menu_open && menu_cursor!=command && n<64;n++) {
#ifdef SC64_COMMAND_ICONS
        if(card_fits()) {
            int slot=card_command_slot(menu_cursor),col=card_command_slot(command)%CARD_COLUMNS;
            int row_length=field_menu_count()-(slot/CARD_COLUMNS)*CARD_COLUMNS;
            if(row_length>CARD_COLUMNS)row_length=CARD_COLUMNS;
            if(slot%CARD_COLUMNS!=col)tap(col>=row_length?KEY_UP:KEY_RIGHT);
            else tap(KEY_DOWN);
        } else
#endif
            tap(KEY_DOWN);
    }
    return menu_open && menu_cursor==command;
}
int main(void)
{
    guardian_test_reset();option_map=0;option_human=R_ZERG;option_bots=1;
    option_bot_races[0]=R_TER;match_limit=30000;
    assert(launch_match());
    int hall=-1;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive && b[i].owner==SIDE &&
        sc64_race_is_zerg_townhall(b[i].type)){hall=i;break;}
    assert(hall>=0 && b[hall].larva>0 && minerals>=sc64_race_unit_rule(U_DRONE)->minerals);
    printf("start tick=%u bank=%d larva=%d hall=%d\n",host_ticks,minerals,b[hall].larva,hall);
    assert(select_building(hall));
    int found=0;for(int i=0;i<16;i++) {
        if(train_type==U_DRONE){found=1;break;}tap(KEY_R);
    }
    assert(found && selected_building==hall);int ore=minerals,larva=b[hall].larva;
    assert(observe_egg()<0);tap(KEY_A);int egg=observe_egg();
    assert(egg>=0 && b[hall].larva==larva-1 && minerals<=ore-sc64_race_unit_rule(U_DRONE)->minerals);
    verify_egg_render(egg);
    printf("paid tick=%u egg=%d timer=%d bank=%d\n",host_ticks,egg,u[egg].work_timer,minerals);
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_B);
    assert((pause_notice&0xc0)==UI_NOTICE_SAVED);tap(KEY_START);assert(screen==SCREEN_PLAY);
    int saved_timer=u[egg].work_timer;
    tap(KEY_SELECT);assert(selected_building<0);assert(select_unit(egg));
    int row=race_context_command_base(RACE_CONTEXT_EGG);
    assert(row<race_skirmish_command_count() && !strcmp(race_menu_label(row),"CANCEL EGG"));
    assert(menu_to(row));int before=minerals;tap(KEY_A);
    assert(!u[egg].alive && minerals>=before+sc64_race_unit_rule(U_DRONE)->minerals);
    printf("canceled tick=%u bank=%d\n",host_ticks,minerals);
    tap(KEY_START);assert(screen==SCREEN_PAUSE);tap(KEY_L);
    assert(screen==SCREEN_PLAY && u[egg].alive && u[egg].type==SC64_EGG_ACTOR &&
           u[egg].work_timer==saved_timer);
    unsigned guard=0;while(u[egg].alive && u[egg].type==SC64_EGG_ACTOR && guard++<2048)input(0);
    assert(u[egg].alive && u[egg].type==U_DRONE);
    printf("loaded and born tick=%u egg=%d\n",host_ticks,egg);
    puts("ordinary HOST input: select, pay, save, cancel, load, birth PASS");return 0;
}
