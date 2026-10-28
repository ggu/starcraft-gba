/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_GAME_BRIDGE_H
#define SC64_P8_GAME_BRIDGE_H

#if !defined(SC64_P8_GAME_POOL_BRIDGE) || !defined(SC64_Z8_GAME_POOL_BRIDGE)
#error "P8 game bridge is an opt-in live-pool service fixture"
#endif

#include "sc64_p8_world_adapter.h"
#include "sc64_z8_game_bridge.h"
#include "sc64_psionic_storm.h"

/* P8 has three used UPRP rows. This is intentionally a strict decoder for
 * those authenticated rows, not a general UPRP interpreter. Native code
 * applies HP/shield/energy percentages from validFields, uses the action's
 * resolved owner when owner-valid is clear, and applies the invincible state
 * bit from validSpecial/status. */
static int
sc64_p8_game_create_default(void *opaque, const Sc64Z8Action *action,
                            unsigned source_owner, unsigned count,
                            unsigned property_id, unsigned x1, unsigned y1,
                            unsigned x2, unsigned y2)
{
    Sc64Z8GameContext *context = (Sc64Z8GameContext *)opaque;
    int raw, type, live_owner, left, top, right, bottom;
    int x, y, slot;
    if (!context || !action || action->opcode != 11u ||
        action->flags != 0x10u || action->modifier != 0u ||
        action->group2 != 0u || property_id != 0u || count != 1u ||
        !action->location || action->string_id || action->wav_id ||
        action->time || source_owner >= SC64_Z8_OWNER_LIMIT ||
        source_owner >= SC64_Z8_SOURCE_OWNER_LIMIT ||
        context->live_owner_for_source[source_owner] < SIDE ||
        context->live_owner_for_source[source_owner] > 4 ||
        action->unit >= SC64_Z8_RAW_UNIT_LIMIT ||
        x2 < x1 || y2 < y1 || x2 > context->map_pixel_width ||
        y2 > context->map_pixel_height ||
        context->map_pixel_width != (unsigned)WORLD_W * 2u ||
        context->map_pixel_height != (unsigned)WORLD_H * 2u)
        return -1;

    raw = action->unit;
    type = sc64_race_unit_from_raw(raw);
    if (type < 0 || sc64_race_building_from_raw(raw) >= 0 ||
        !sc64_z8_game_source_extents((unsigned)raw, 0, 0, 0,
                                     &left, &top, &right, &bottom))
        return -1;

    /* Native CreateUnit uses the source-rectangle midpoint, then clamps the
     * actor footprint to the map. Keep the existing Z8-bridge geometry helper
     * as the common extent source; this path does not tile-snap placement. */
    x = (int)((x1 + x2) / 2u);
    y = (int)((y1 + y2) / 2u);
    if ((unsigned)left > context->map_pixel_width ||
        (unsigned)top > context->map_pixel_height ||
        (unsigned)right >= context->map_pixel_width ||
        (unsigned)bottom >= context->map_pixel_height)
        return -1;
    if (x < left) x = left;
    if (y < top) y = top;
    if (x + right >= context->map_pixel_width)
        x = context->map_pixel_width - right - 1;
    if (y + bottom >= context->map_pixel_height)
        y = context->map_pixel_height - bottom - 1;
    if (x < left || y < top || x + right >= context->map_pixel_width ||
        y + bottom >= context->map_pixel_height || find_unit_slot() < 0)
        return -1;

    live_owner = context->live_owner_for_source[source_owner];
    slot = spawn_unit(live_owner, type, x / 2, y / 2);
    if (slot < 0) return -1;
    u[slot].raw_type = (s16)raw;
    u[slot].raw_owner = (s16)source_owner;
    return 1;
}

static void
sc64_p8_game_bind_default_create_service(Sc64Z8WorldView *world,
                                         Sc64Z8GameContext *context)
{
    if (!world || !context) return;
    world->services.create = sc64_p8_game_create_default;
    world->services.create_context = context;
}

static unsigned sc64_p8_read_le16(const u8 *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned sc64_p8_read_le32(const u8 *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) |
           ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

static int
sc64_p8_game_property_kind(unsigned property_id, unsigned raw,
                           const u8 *p, unsigned *hp_percent,
                           unsigned *shield_percent,
                           unsigned *energy_percent, int *invincible)
{
    unsigned valid_special, valid_fields, status;
    unsigned expected_raw, expected_fields, expected_special;
    unsigned expected_hp, expected_shield, expected_energy, expected_status;
    if (!p || !hp_percent || !shield_percent || !energy_percent ||
        !invincible || property_id < 1 || property_id > 3) return 0;
    valid_special = sc64_p8_read_le16(p);
    valid_fields = sc64_p8_read_le16(p + 2);
    status = sc64_p8_read_le16(p + 14);
    expected_raw = property_id == 1 ? 78u : property_id == 2 ? 196u : 79u;
    expected_fields = property_id == 1 ? 0x06u : property_id == 2 ? 0x02u : 0x0eu;
    expected_special = property_id == 2 ? 0x10u : 0x18u;
    expected_hp = 100u;
    expected_shield = property_id == 2 ? 0u : 100u;
    expected_energy = property_id == 3 ? 100u : 0u;
    expected_status = property_id == 3 ? 0x10u : 0u;

    /* The exact source rows leave owner, resource, hangar, and reserved bytes
     * unused. Reject any descriptor drift rather than silently dropping it. */
    if (raw != expected_raw || p[4] != 0xffu ||
        valid_special != expected_special || valid_fields != expected_fields ||
        p[5] != expected_hp || p[6] != expected_shield ||
        p[7] != expected_energy || sc64_p8_read_le32(p + 8) != 0u ||
        sc64_p8_read_le16(p + 12) != 0u || status != expected_status ||
        p[16] || p[17] || p[18] || p[19]) return 0;

    /* Native masks: 0x2=HP, 0x4=shield, 0x8=energy. Resource/hangar masks
     * are absent in these rows. All present percentages are exactly 100. */
    if (!(valid_fields & 0x02u) || p[5] != 100u ||
        ((valid_fields & 0x04u) != 0u) != (p[6] == 100u) ||
        ((valid_fields & 0x08u) != 0u) != (p[7] == 100u)) return 0;
    *hp_percent = p[5];
    *shield_percent = (valid_fields & 0x04u) ? p[6] : 0u;
    *energy_percent = (valid_fields & 0x08u) ? p[7] : 0u;
    *invincible = (valid_special & 0x10u) && (status & 0x10u);
    return 1;
}

static int
sc64_p8_game_create_with_property(void *opaque,
                                  const Sc64P8SourceAction *source,
                                  unsigned source_owner, unsigned count,
                                  unsigned property_id, const u8 *property,
                                  const Sc64Z8Rect *rect)
{
    Sc64Z8GameContext *context = (Sc64Z8GameContext *)opaque;
    unsigned hp_percent, shield_percent, energy_percent;
    int invincible, raw, type, live_owner, building, left, top, right, bottom;
    int x, y, slot;
    if (!context || !source || !rect || source->opcode != 11u ||
        source->flags != 0x18u || source->modifier != 0u ||
        source->group2 != property_id || source->string_id || source->wav_id ||
        source->time || count != 1u || source_owner >= SC64_Z8_OWNER_LIMIT ||
        source_owner >= SC64_Z8_SOURCE_OWNER_LIMIT ||
        context->live_owner_for_source[source_owner] < SIDE ||
        context->live_owner_for_source[source_owner] > 4 ||
        rect->left < 0 || rect->top < 0 || rect->right < rect->left ||
        rect->bottom < rect->top ||
        (unsigned)rect->right > context->map_pixel_width ||
        (unsigned)rect->bottom > context->map_pixel_height ||
        context->map_pixel_width != (unsigned)WORLD_W * 2u ||
        context->map_pixel_height != (unsigned)WORLD_H * 2u ||
        !sc64_p8_game_property_kind(property_id, source->unit, property,
                                    &hp_percent, &shield_percent,
                                    &energy_percent, &invincible)) return -1;

    raw = source->unit;
    type = sc64_race_unit_from_raw(raw);
    building = type < 0;
    if (building) type = sc64_race_building_from_raw(raw);
    if (type < 0 || !sc64_z8_game_source_extents((unsigned)raw, 0, 0,
            building, &left, &top, &right, &bottom)) return -1;

    /* Match the default create service's center/clamp geometry. P8's building
     * selector is a source building and consumes the building pool. */
    x = ((int)rect->left + (int)rect->right) / 2;
    y = ((int)rect->top + (int)rect->bottom) / 2;
    if ((unsigned)left > context->map_pixel_width ||
        (unsigned)top > context->map_pixel_height ||
        (unsigned)right >= context->map_pixel_width ||
        (unsigned)bottom >= context->map_pixel_height) return -1;
    if (x < left) x = left;
    if (y < top) y = top;
    if (x + right >= context->map_pixel_width)
        x = context->map_pixel_width - right - 1;
    if (y + bottom >= context->map_pixel_height)
        y = context->map_pixel_height - bottom - 1;
    if (x < left || y < top || x + right >= context->map_pixel_width ||
        y + bottom >= context->map_pixel_height) return -1;

    slot = building ? find_build_slot() : find_unit_slot();
    if (slot < 0) return -1;
    live_owner = context->live_owner_for_source[source_owner];
    if (building) {
        slot = spawn_build(live_owner, type, x / 2, y / 2, 0);
        if (slot < 0) return -1;
        b[slot].raw_type = (s16)raw;
        b[slot].raw_owner = (s16)source_owner;
        b[slot].hp = (int)((u32)b[slot].maxhp * hp_percent / 100u);
        if (shield_percent)
            b[slot].shield = (u16)((u32)b[slot].shield * shield_percent / 100u);
        invincible_buildings[slot] = (u8)invincible;
    } else {
        slot = spawn_unit(live_owner, type, x / 2, y / 2);
        if (slot < 0) return -1;
        u[slot].raw_type = (s16)raw;
        u[slot].raw_owner = (s16)source_owner;
        u[slot].hp = (int)((u32)u[slot].maxhp * hp_percent / 100u);
        if (shield_percent)
            u[slot].shield = (u16)((u32)u[slot].shield * shield_percent / 100u);
        if (energy_percent)
            u[slot].carry = (int)((u32)SC64_STORM_HERO_MAX_ENERGY *
                                  SC64_STORM_ENERGY_ONE * energy_percent / 100u);
        invincible_units[slot] = (u8)invincible;
    }
    return 1;
}

static void
sc64_p8_game_bind_property_create_service(Sc64P8WorldContext *world,
                                          Sc64Z8GameContext *context)
{
    if (!world || !context) return;
    world->create_with_property = sc64_p8_game_create_with_property;
    world->create_context = context;
}

#endif /* SC64_P8_GAME_BRIDGE_H */
