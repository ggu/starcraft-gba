/* The N64 HUD's resource icons for the race HUD's top bar (SC64_HUD_ICONS).
 *
 * The mineral chunk, the gas icon the N64 draws for every race and the
 * player race's supply icon replace the MIN, GAS and SUP labels. They come
 * from the private header tools/sc64_command_icons.py writes from a verified
 * ROM, beside the command-card buttons (command_card.h). */
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
