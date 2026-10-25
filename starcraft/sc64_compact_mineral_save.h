#ifndef SC64_COMPACT_MINERAL_SAVE_H
#define SC64_COMPACT_MINERAL_SAVE_H

/* Five-byte, lossless save row for the campaign Mineral value. Include this
 * after Mineral is defined. Runtime coordinates are pixel values on maps up
 * to 128x128 tiles (0..2047); source mineral styles are 0, 176, 177, or 178. */
#define SC64_PACKED_MINERAL_SAVE_BYTES 5

typedef struct {
    u8 bytes[SC64_PACKED_MINERAL_SAVE_BYTES];
} Sc64PackedMineralSave;

_Static_assert(sizeof(Sc64PackedMineralSave) == SC64_PACKED_MINERAL_SAVE_BYTES,
               "packed mineral save row size");

static int
sc64_mineral_save_put(Sc64PackedMineralSave *row, unsigned *bit,
                      unsigned width, u32 value)
{
    if (!row || !bit || !width || width > 16 ||
        *bit > SC64_PACKED_MINERAL_SAVE_BYTES * 8u ||
        width > SC64_PACKED_MINERAL_SAVE_BYTES * 8u - *bit ||
        value >= ((u32)1u << width)) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        if ((value >> i) & 1u)
            row->bytes[*bit >> 3] |= (u8)(1u << (*bit & 7u));
    return 1;
}

static int
sc64_mineral_save_get(const Sc64PackedMineralSave *row, unsigned *bit,
                      unsigned width, u32 *value)
{
    u32 decoded = 0;
    if (!row || !bit || !value || !width || width > 16 ||
        *bit > SC64_PACKED_MINERAL_SAVE_BYTES * 8u ||
        width > SC64_PACKED_MINERAL_SAVE_BYTES * 8u - *bit) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        decoded |= (u32)((row->bytes[*bit >> 3] >> (*bit & 7u)) & 1u) << i;
    *value = decoded;
    return 1;
}

static int
sc64_mineral_style_code(int style, u32 *code)
{
    if (!code) return 0;
    switch (style) {
    case 0: *code = 0; return 1;
    case 176: *code = 1; return 1;
    case 177: *code = 2; return 1;
    case 178: *code = 3; return 1;
    default: return 0;
    }
}

static int
sc64_mineral_style_value(u32 code, int *style)
{
    static const int values[4] = {0, 176, 177, 178};
    if (!style || code >= 4) return 0;
    *style = values[code];
    return 1;
}

static int
sc64_mineral_save_encode(Sc64PackedMineralSave *row, const Mineral *mineral,
                         unsigned world_width, unsigned world_height)
{
    Sc64PackedMineralSave encoded = {{0}};
    unsigned bit = 0;
    u32 style_code;
    if (!row || !mineral || !world_width || !world_height ||
        world_width > 2048 || world_height > 2048 ||
        mineral->x < 0 || (unsigned)mineral->x >= world_width ||
        mineral->y < 0 || (unsigned)mineral->y >= world_height ||
        mineral->amount < 0 || mineral->amount > 65535 ||
        !sc64_mineral_style_code(mineral->style, &style_code)) return 0;
    if (!sc64_mineral_save_put(&encoded, &bit, 11, (u32)mineral->x) ||
        !sc64_mineral_save_put(&encoded, &bit, 11, (u32)mineral->y) ||
        !sc64_mineral_save_put(&encoded, &bit, 16, (u32)mineral->amount) ||
        !sc64_mineral_save_put(&encoded, &bit, 2, style_code) || bit != 40)
        return 0;
    *row = encoded;
    return 1;
}

static int
sc64_mineral_save_decode(Mineral *out, const Sc64PackedMineralSave *row,
                         unsigned world_width, unsigned world_height)
{
    Mineral decoded = {0};
    unsigned bit = 0;
    u32 x, y, amount, style_code;
    if (!out || !row || !world_width || !world_height ||
        world_width > 2048 || world_height > 2048 ||
        !sc64_mineral_save_get(row, &bit, 11, &x) ||
        !sc64_mineral_save_get(row, &bit, 11, &y) ||
        !sc64_mineral_save_get(row, &bit, 16, &amount) ||
        !sc64_mineral_save_get(row, &bit, 2, &style_code) || bit != 40 ||
        x >= world_width || y >= world_height ||
        !sc64_mineral_style_value(style_code, &decoded.style)) return 0;
    decoded.x = (int)x;
    decoded.y = (int)y;
    decoded.amount = (int)amount;
    *out = decoded;
    return 1;
}

#endif
