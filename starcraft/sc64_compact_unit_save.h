#ifndef SC64_COMPACT_UNIT_SAVE_H
#define SC64_COMPACT_UNIT_SAVE_H

/* Lossless slot-preserving Unit row for the opt-in full-campaign save layout.
 * This header is included only where the RACES Unit definition is available.
 * The 38-byte stream uses 303 bits; its final bit is reserved and must be 0.
 * Every value is range-checked and the destination is written only after a
 * complete successful encode/decode. */
#define SC64_PACKED_UNIT_SAVE_BYTES 38
#define SC64_PACKED_UNIT_HP_BIAS 131072
#define SC64_PACKED_UNIT_SIGNED17_BIAS 65536
#define SC64_PACKED_UNIT_SIGNED10_BIAS 512

#ifndef SC64_RACES
#error "compact Unit rows require the RACES Unit layout"
#endif

typedef struct {
    u8 bytes[SC64_PACKED_UNIT_SAVE_BYTES];
} Sc64PackedUnitSave;

_Static_assert(sizeof(Sc64PackedUnitSave) == SC64_PACKED_UNIT_SAVE_BYTES,
               "packed unit save row size");

static int
sc64_packed_unit_put(Sc64PackedUnitSave *row, unsigned *bit,
                     unsigned width, u32 value)
{
    if (!row || !bit || !width || width > 32 ||
        *bit > SC64_PACKED_UNIT_SAVE_BYTES * 8u ||
        width > SC64_PACKED_UNIT_SAVE_BYTES * 8u - *bit ||
        (width < 32 && value >= ((u32)1u << width))) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        if ((value >> i) & 1u)
            row->bytes[*bit >> 3] |= (u8)(1u << (*bit & 7u));
    return 1;
}

static int
sc64_packed_unit_get(const Sc64PackedUnitSave *row, unsigned *bit,
                     unsigned width, u32 *value)
{
    u32 result = 0;
    if (!row || !bit || !value || !width || width > 32 ||
        *bit > SC64_PACKED_UNIT_SAVE_BYTES * 8u ||
        width > SC64_PACKED_UNIT_SAVE_BYTES * 8u - *bit) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        result |= (u32)((row->bytes[*bit >> 3] >> (*bit & 7u)) & 1u) << i;
    *value = result;
    return 1;
}

static int
sc64_packed_unit_is_zero(const Sc64PackedUnitSave *row)
{
    if (!row) return 0;
    for (unsigned i = 0; i < SC64_PACKED_UNIT_SAVE_BYTES; ++i)
        if (row->bytes[i]) return 0;
    return 1;
}

static int
sc64_unit_save_fields_valid(const Unit *q, unsigned world_width,
                            unsigned world_height)
{
    if (!q || !world_width || !world_height || world_width > 2048 ||
        world_height > 2048 || q->x < 0 || (unsigned)q->x >= world_width ||
        q->y < 0 || (unsigned)q->y >= world_height ||
        q->hp < -SC64_PACKED_UNIT_HP_BIAS ||
        q->hp >= SC64_PACKED_UNIT_HP_BIAS || (q->alive && q->hp < 0) ||
        q->maxhp < 0 || q->maxhp > 131071 || q->owner < 0 || q->owner > 4 ||
        q->type < 0 || q->type >= SC_RU_COUNT || SC_RU_COUNT > 64 ||
        (q->alive != 0 && q->alive != 1) ||
        (q->selected != 0 && q->selected != 1) || q->cd < 0 || q->cd > 65535 ||
        q->order < 0 || q->order > 15 ||
        q->tx < 0 || q->tx > 2047 || q->ty < 0 || q->ty > 2047 ||
        q->carry < -SC64_PACKED_UNIT_SIGNED17_BIAS ||
        q->carry >= SC64_PACKED_UNIT_SIGNED17_BIAS ||
        q->build < -SC64_PACKED_UNIT_SIGNED10_BIAS ||
        q->build >= SC64_PACKED_UNIT_SIGNED10_BIAS ||
        q->work_timer < 0 || q->work_timer > 65535 ||
        q->armor < 0 || q->armor > 255 || q->weapon < 0 || q->weapon > 65535 ||
        q->target < -1 || q->target > 511 ||
        q->path_timer < 0 || q->path_timer > 65535 ||
        q->path_retry < 0 || q->path_retry > 65535 ||
        q->path_x < 0 || q->path_x > 2047 ||
        q->path_y < 0 || q->path_y > 2047 ||
        q->shield < 0 || q->shield > 131071 ||
        q->raw_type < -1 || q->raw_type > 255 ||
        q->raw_owner < -1 || q->raw_owner > 255 ||
        q->flags < 0 || q->flags > 65535 ||
        q->shield_remainder < 0 || q->shield_remainder > 1279) return 0;
    return 1;
}

static int
sc64_unit_save_encode(Sc64PackedUnitSave *row, const Unit *q,
                      unsigned world_width, unsigned world_height)
{
    Sc64PackedUnitSave encoded;
    unsigned bit = 0;
    if (!row || !sc64_unit_save_fields_valid(q, world_width, world_height)) return 0;
    memset(&encoded, 0, sizeof(encoded));
    /* Keep the all-zero unused-slot representation canonical and reversible. */
    static const Unit zero_unit = {0};
    if (memcmp(q, &zero_unit, sizeof(*q)) == 0) {
        *row = encoded;
        return 1;
    }
#define SC64_UNIT_PUT(width, value) do { \
    if (!sc64_packed_unit_put(&encoded, &bit, (width), (u32)(value))) return 0; \
} while (0)
    SC64_UNIT_PUT(11, q->x); SC64_UNIT_PUT(11, q->y);
    SC64_UNIT_PUT(18, q->hp + SC64_PACKED_UNIT_HP_BIAS);
    SC64_UNIT_PUT(17, q->maxhp); SC64_UNIT_PUT(3, q->owner);
    SC64_UNIT_PUT(6, q->type); SC64_UNIT_PUT(1, q->alive);
    SC64_UNIT_PUT(1, q->selected); SC64_UNIT_PUT(16, q->cd);
    SC64_UNIT_PUT(4, q->order); SC64_UNIT_PUT(11, q->tx);
    SC64_UNIT_PUT(11, q->ty);
    SC64_UNIT_PUT(17, q->carry + SC64_PACKED_UNIT_SIGNED17_BIAS);
    SC64_UNIT_PUT(10, q->build + SC64_PACKED_UNIT_SIGNED10_BIAS);
    SC64_UNIT_PUT(16, q->work_timer); SC64_UNIT_PUT(8, q->armor);
    SC64_UNIT_PUT(16, q->weapon);
    SC64_UNIT_PUT(10, q->target + SC64_PACKED_UNIT_SIGNED10_BIAS);
    SC64_UNIT_PUT(16, q->path_timer); SC64_UNIT_PUT(16, q->path_retry);
    SC64_UNIT_PUT(11, q->path_x); SC64_UNIT_PUT(11, q->path_y);
    SC64_UNIT_PUT(17, q->shield); SC64_UNIT_PUT(9, q->raw_type + 1);
    SC64_UNIT_PUT(9, q->raw_owner + 1); SC64_UNIT_PUT(16, q->flags);
    SC64_UNIT_PUT(11, q->shield_remainder);
#undef SC64_UNIT_PUT
    if (bit != 303) return 0;
    *row = encoded;
    return 1;
}

static int
sc64_unit_save_decode(Unit *out, const Sc64PackedUnitSave *row,
                      unsigned world_width, unsigned world_height)
{
    Unit decoded;
    unsigned bit = 0;
    u32 v;
    if (!out || !row) return 0;
    memset(&decoded, 0, sizeof(decoded));
    if (sc64_packed_unit_is_zero(row)) {
        if (!sc64_unit_save_fields_valid(&decoded, world_width, world_height)) return 0;
        *out = decoded;
        return 1;
    }
#define SC64_UNIT_GET(width, dst) do { \
    if (!sc64_packed_unit_get(row, &bit, (width), &v)) return 0; \
    (dst) = (int)v; \
} while (0)
    SC64_UNIT_GET(11, decoded.x); SC64_UNIT_GET(11, decoded.y);
    SC64_UNIT_GET(18, v); decoded.hp = (int)v - SC64_PACKED_UNIT_HP_BIAS;
    SC64_UNIT_GET(17, decoded.maxhp); SC64_UNIT_GET(3, decoded.owner);
    SC64_UNIT_GET(6, decoded.type); SC64_UNIT_GET(1, decoded.alive);
    SC64_UNIT_GET(1, decoded.selected); SC64_UNIT_GET(16, decoded.cd);
    SC64_UNIT_GET(4, decoded.order); SC64_UNIT_GET(11, decoded.tx);
    SC64_UNIT_GET(11, decoded.ty); SC64_UNIT_GET(17, v);
    decoded.carry = (int)v - SC64_PACKED_UNIT_SIGNED17_BIAS;
    SC64_UNIT_GET(10, v); decoded.build = (int)v - SC64_PACKED_UNIT_SIGNED10_BIAS;
    SC64_UNIT_GET(16, decoded.work_timer); SC64_UNIT_GET(8, decoded.armor);
    SC64_UNIT_GET(16, decoded.weapon); SC64_UNIT_GET(10, v);
    decoded.target = (int)v - SC64_PACKED_UNIT_SIGNED10_BIAS;
    SC64_UNIT_GET(16, decoded.path_timer); SC64_UNIT_GET(16, decoded.path_retry);
    SC64_UNIT_GET(11, decoded.path_x); SC64_UNIT_GET(11, decoded.path_y);
    SC64_UNIT_GET(17, decoded.shield); SC64_UNIT_GET(9, v);
    decoded.raw_type = (int)v - 1; SC64_UNIT_GET(9, v);
    decoded.raw_owner = (int)v - 1; SC64_UNIT_GET(16, decoded.flags);
    SC64_UNIT_GET(11, decoded.shield_remainder);
#undef SC64_UNIT_GET
    if (bit != 303 || !sc64_unit_save_fields_valid(&decoded, world_width,
                                                    world_height)) return 0;
    if ((row->bytes[SC64_PACKED_UNIT_SAVE_BYTES - 1] & 0x80u) != 0) return 0;
    *out = decoded;
    return 1;
}

#endif
