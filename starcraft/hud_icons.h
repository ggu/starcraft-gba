/* The N64 HUD's icons for the race HUD (SC64_HUD_ICONS).
 *
 * The mineral chunk, the gas icon the N64 draws for every race and the
 * player race's supply icon replace the MIN, GAS and SUP labels. The portrait
 * and upgrade buttons, tinted as StarCraft tints its buttons, fill the
 * selection panel (hud_panel.h) and the command card (command_card.h). They
 * come from the private header tools/sc64_command_icons.py writes from a
 * verified ROM. */
#ifndef SC64_HUD_ICONS
#error "hud_icons.h needs the imported HUD icons"
#endif

#include "sc64_command_icons_generated.h"

/* A resource-bar icon centred in the 12-pixel slot at x on the top band. */
static void
card_resource_icon(int sprite, int x)
{
    int w = sc64_hud_sprite_size[sprite][0], h = sc64_hud_sprite_size[sprite][1];
    gfx_blit_sprite(x + (12 - w) / 2, (12 - h) / 2, w, h,
                    sc64_hud_sprite_pixels + sc64_hud_sprite_offset[sprite]);
}

/* The race's supply icon: Terran, Zerg, Protoss. */
static int
card_supply_sprite(void)
{
    return SC64_HUD_SPRITE_SUPPLY_TERRAN + race_human;
}

enum { CARD_CELL = 20 };

/* The portrait of a raw unit id, or -1 when the ROM has no button-sized
 * frame for it. */
static int
card_portrait(int raw)
{
    return raw >= 0 && raw < SC64_CMD_ICON_PORTRAITS && sc64_command_icon_size[raw][0] > 1 ? raw : -1;
}

static int
card_building_icon(int type)
{
    return card_portrait(sc64_race_raw_building(type));
}

static int
card_unit_icon(const Unit *q)
{
    return card_portrait(q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type));
}

static int
card_weapons_icon(void)
{
    return race_human == R_ZERG ? SC64_CMD_ICON_ZERG_WEAPONS :
        race_human == R_PROTOSS ? SC64_CMD_ICON_PROTOSS_WEAPONS : SC64_CMD_ICON_TERRAN_WEAPONS;
}

static int
card_armor_icon(void)
{
    return race_human == R_ZERG ? SC64_CMD_ICON_ZERG_ARMOR :
        race_human == R_PROTOSS ? SC64_CMD_ICON_PROTOSS_ARMOR : SC64_CMD_ICON_TERRAN_ARMOR;
}

/* An icon centred in the cell at x, y: decoded once, then blitted. The panel
 * draws seven of these on a redraw, and a rectangle per run of a shade cost
 * most of a frame. Its colour 0 stays transparent. */
static void
card_icon_draw(int icon, int x, int y, const u8 *ramp)
{
    int w = sc64_command_icon_size[icon][0], h = sc64_command_icon_size[icon][1];
    int stride = (w + 3) / 4, row, col;
    const unsigned char *bits = sc64_command_icon_bits + sc64_command_icon_offset[icon];
    const u8 shade[4] = {0, ramp[0], ramp[1], ramp[2]};
    /* A row decodes whole bytes, so it can run three pixels past its width. */
    u8 pixels[CARD_CELL * CARD_CELL + 3];
    for (row = 0; row < h; row++, bits += stride) {
        u8 *out = pixels + row * w;
        for (col = 0; col < stride; col++) {
            unsigned v = bits[col];
            *out++ = shade[v & 3];
            *out++ = shade[v >> 2 & 3];
            *out++ = shade[v >> 4 & 3];
            *out++ = shade[v >> 6];
        }
    }
    gfx_blit_sprite(x + (CARD_CELL - w) / 2, y + (CARD_CELL - h) / 2, w, h, pixels);
}

static const u8 card_green[3] = {C_GREEN_DARK, C_GREEN, C_HIGHLIGHT};
static const u8 card_gold[3] = {C_GOLD_DARK, C_GOLD, C_HIGHLIGHT};
static const u8 card_grey[3] = {C_EDGE, C_DIM, C_SILVER};
