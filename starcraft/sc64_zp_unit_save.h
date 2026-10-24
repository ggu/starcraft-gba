#ifndef SC64_ZP_UNIT_SAVE_H
#define SC64_ZP_UNIT_SAVE_H

/* Proposed future ZP-profile Unit row. This is deliberately separate from
 * the accepted 38-byte codec: a future save generation must opt into it.
 * The movement implementation assigns path_timer only 0, 1, or 24 and
 * path_retry only 0, 2, 3, 6, or 12; both are countdowns. This row encodes
 * those values in 5 and 4 bits respectively and preserves every other
 * legacy field width/range. Total: exactly 280 bits / 35 bytes. */
#define SC64_ZP_PACKED_UNIT_SAVE_BYTES 35
#define SC64_ZP_PACKED_UNIT_HP_BIAS 131072
#define SC64_ZP_PACKED_UNIT_SIGNED17_BIAS 65536
#define SC64_ZP_PACKED_UNIT_SIGNED10_BIAS 512

#ifndef SC64_RACES
#error "compact ZP Unit rows require the RACES Unit layout"
#endif

typedef struct { u8 bytes[SC64_ZP_PACKED_UNIT_SAVE_BYTES]; } Sc64ZpPackedUnitSave;
_Static_assert(sizeof(Sc64ZpPackedUnitSave) == SC64_ZP_PACKED_UNIT_SAVE_BYTES,
               "packed ZP unit save row size");

static int sc64_zp_unit_put(Sc64ZpPackedUnitSave *row, unsigned *bit,
                            unsigned width, u32 value)
{
    if (!row || !bit || !width || width > 32 ||
        *bit > SC64_ZP_PACKED_UNIT_SAVE_BYTES * 8u ||
        width > SC64_ZP_PACKED_UNIT_SAVE_BYTES * 8u - *bit ||
        (width < 32 && value >= ((u32)1u << width))) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        if ((value >> i) & 1u)
            row->bytes[*bit >> 3] |= (u8)(1u << (*bit & 7u));
    return 1;
}

static int sc64_zp_unit_get(const Sc64ZpPackedUnitSave *row, unsigned *bit,
                            unsigned width, u32 *value)
{
    u32 result = 0;
    if (!row || !bit || !value || !width || width > 32 ||
        *bit > SC64_ZP_PACKED_UNIT_SAVE_BYTES * 8u ||
        width > SC64_ZP_PACKED_UNIT_SAVE_BYTES * 8u - *bit) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        result |= (u32)((row->bytes[*bit >> 3] >> (*bit & 7u)) & 1u) << i;
    *value = result;
    return 1;
}

static int sc64_zp_unit_is_zero(const Sc64ZpPackedUnitSave *row)
{
    if (!row) return 0;
    for (unsigned i = 0; i < SC64_ZP_PACKED_UNIT_SAVE_BYTES; ++i)
        if (row->bytes[i]) return 0;
    return 1;
}

static int sc64_zp_unit_fields_valid(const Unit *q, unsigned world_width,
                                     unsigned world_height)
{
    if (!q || !world_width || !world_height || world_width > 2048 ||
        world_height > 2048 || q->x < 0 || (unsigned)q->x >= world_width ||
        q->y < 0 || (unsigned)q->y >= world_height ||
        q->hp < -SC64_ZP_PACKED_UNIT_HP_BIAS ||
        q->hp >= SC64_ZP_PACKED_UNIT_HP_BIAS || (q->alive && q->hp < 0) ||
        q->maxhp < 0 || q->maxhp > 131071 || q->owner < 0 || q->owner > 4 ||
        q->type < 0 || q->type >= SC_RU_COUNT || SC_RU_COUNT > 64 ||
        (q->alive != 0 && q->alive != 1) ||
        (q->selected != 0 && q->selected != 1) || q->cd < 0 || q->cd > 65535 ||
        q->order < 0 || q->order > 15 || q->tx < 0 || q->tx > 2047 ||
        q->ty < 0 || q->ty > 2047 ||
        q->carry < -SC64_ZP_PACKED_UNIT_SIGNED17_BIAS ||
        q->carry >= SC64_ZP_PACKED_UNIT_SIGNED17_BIAS ||
        q->build < -SC64_ZP_PACKED_UNIT_SIGNED10_BIAS ||
        q->build >= SC64_ZP_PACKED_UNIT_SIGNED10_BIAS ||
        q->work_timer < 0 || q->work_timer > 65535 || q->armor < 0 ||
        q->armor > 255 || q->weapon < 0 || q->weapon > 65535 ||
        q->target < -1 || q->target > 511 || q->path_timer < 0 ||
        q->path_timer > 24 || q->path_retry < 0 || q->path_retry > 12 ||
        q->path_x < 0 || q->path_x > 2047 || q->path_y < 0 || q->path_y > 2047 ||
        q->shield < 0 || q->shield > 131071 || q->raw_type < -1 ||
        q->raw_type > 255 || q->raw_owner < -1 || q->raw_owner > 255 ||
        q->flags < 0 || q->flags > 65535 ||
        q->shield_remainder < 0 || q->shield_remainder > 1279) return 0;
    return 1;
}

static int sc64_zp_unit_save_encode(Sc64ZpPackedUnitSave *row, const Unit *q,
                                    unsigned world_width,
                                    unsigned world_height)
{
    Sc64ZpPackedUnitSave encoded;
    unsigned bit = 0;
    if (!row || !sc64_zp_unit_fields_valid(q, world_width, world_height)) return 0;
    memset(&encoded, 0, sizeof(encoded));
    static const Unit zero_unit = {0};
    if (memcmp(q, &zero_unit, sizeof(*q)) == 0) { *row = encoded; return 1; }
#define SC64_ZP_PUT(w, v) do { \
    if (!sc64_zp_unit_put(&encoded, &bit, (w), (u32)(v))) return 0; \
} while (0)
    SC64_ZP_PUT(11, q->x); SC64_ZP_PUT(11, q->y);
    SC64_ZP_PUT(18, q->hp + SC64_ZP_PACKED_UNIT_HP_BIAS);
    SC64_ZP_PUT(17, q->maxhp); SC64_ZP_PUT(3, q->owner);
    SC64_ZP_PUT(6, q->type); SC64_ZP_PUT(1, q->alive);
    SC64_ZP_PUT(1, q->selected); SC64_ZP_PUT(16, q->cd);
    SC64_ZP_PUT(4, q->order); SC64_ZP_PUT(11, q->tx); SC64_ZP_PUT(11, q->ty);
    SC64_ZP_PUT(17, q->carry + SC64_ZP_PACKED_UNIT_SIGNED17_BIAS);
    SC64_ZP_PUT(10, q->build + SC64_ZP_PACKED_UNIT_SIGNED10_BIAS);
    SC64_ZP_PUT(16, q->work_timer); SC64_ZP_PUT(8, q->armor);
    SC64_ZP_PUT(16, q->weapon);
    SC64_ZP_PUT(10, q->target + SC64_ZP_PACKED_UNIT_SIGNED10_BIAS);
    SC64_ZP_PUT(5, q->path_timer); SC64_ZP_PUT(4, q->path_retry);
    SC64_ZP_PUT(11, q->path_x); SC64_ZP_PUT(11, q->path_y);
    SC64_ZP_PUT(17, q->shield); SC64_ZP_PUT(9, q->raw_type + 1);
    SC64_ZP_PUT(9, q->raw_owner + 1); SC64_ZP_PUT(16, q->flags);
    SC64_ZP_PUT(11, q->shield_remainder);
#undef SC64_ZP_PUT
    if (bit != 280) return 0;
    *row = encoded;
    return 1;
}

static int sc64_zp_unit_save_decode(Unit *out, const Sc64ZpPackedUnitSave *row,
                                    unsigned world_width,
                                    unsigned world_height)
{
    Unit decoded;
    unsigned bit = 0;
    u32 v;
    if (!out || !row) return 0;
    memset(&decoded, 0, sizeof(decoded));
    if (sc64_zp_unit_is_zero(row)) {
        if (!sc64_zp_unit_fields_valid(&decoded, world_width, world_height)) return 0;
        *out = decoded; return 1;
    }
#define SC64_ZP_GET(w, dst) do { \
    if (!sc64_zp_unit_get(row, &bit, (w), &v)) return 0; \
    (dst) = (int)v; \
} while (0)
    SC64_ZP_GET(11, decoded.x); SC64_ZP_GET(11, decoded.y);
    SC64_ZP_GET(18, v); decoded.hp = (int)v - SC64_ZP_PACKED_UNIT_HP_BIAS;
    SC64_ZP_GET(17, decoded.maxhp); SC64_ZP_GET(3, decoded.owner);
    SC64_ZP_GET(6, decoded.type); SC64_ZP_GET(1, decoded.alive);
    SC64_ZP_GET(1, decoded.selected); SC64_ZP_GET(16, decoded.cd);
    SC64_ZP_GET(4, decoded.order); SC64_ZP_GET(11, decoded.tx);
    SC64_ZP_GET(11, decoded.ty); SC64_ZP_GET(17, v);
    decoded.carry = (int)v - SC64_ZP_PACKED_UNIT_SIGNED17_BIAS;
    SC64_ZP_GET(10, v); decoded.build = (int)v - SC64_ZP_PACKED_UNIT_SIGNED10_BIAS;
    SC64_ZP_GET(16, decoded.work_timer); SC64_ZP_GET(8, decoded.armor);
    SC64_ZP_GET(16, decoded.weapon); SC64_ZP_GET(10, v);
    decoded.target = (int)v - SC64_ZP_PACKED_UNIT_SIGNED10_BIAS;
    SC64_ZP_GET(5, decoded.path_timer); SC64_ZP_GET(4, decoded.path_retry);
    SC64_ZP_GET(11, decoded.path_x); SC64_ZP_GET(11, decoded.path_y);
    SC64_ZP_GET(17, decoded.shield); SC64_ZP_GET(9, v);
    decoded.raw_type = (int)v - 1; SC64_ZP_GET(9, v);
    decoded.raw_owner = (int)v - 1; SC64_ZP_GET(16, decoded.flags);
    SC64_ZP_GET(11, decoded.shield_remainder);
#undef SC64_ZP_GET
    if (bit != 280 || !sc64_zp_unit_fields_valid(&decoded, world_width,
                                                  world_height)) return 0;
    *out = decoded;
    return 1;
}

#endif
